#include <cstdlib>
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <signal.h>
#include <sstream>
#include <string>
#include <string.h>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <X11/XKBlib.h>

namespace fs = std::filesystem;

class Settings {
public:
    bool forking;
    std::string pidFile;
    std::string display;

    Settings() : forking{ false }, pidFile("/var/run/alt-shift-notify/service.pid"), display("") {}
};

void showHelp(int argc, char* argv[]) {
    std::cerr << "Usage: " << argv[0] << " [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --help                Show this help and exits" << std::endl;
    std::cerr << "  --fork                Start in a detached process after forking," << std::endl;
    std::cerr << "                        while the main process exits immediately." << std::endl;
    std::cerr << "  --pidfile <file>      [required] Specify the PID file of the service" << std::endl;
    std::cerr << "                        to subscribe to." << std::endl;
    std::cerr << "  --display <string>    [default=\"\"] Specify the displayto connect to." << std::endl;
    std::cerr << "                        Defaults to connecting to the main display." << std::endl;
    std::cerr << std::endl;
    std::cerr << "The process specified in the PID file will receive SIGUSR1. That signal should" << std::endl;
    std::cerr << "be interpted as \"subscribe\". If possible, when terminating, SIGUSR2 will be" << std::endl;
    std::cerr << "send, indicating \"opt out\"." << std::endl;
    std::cerr << std::endl;
    std::cerr << "This process will initiate keyboard layout rotation when SIGUSR1 is received." << std::endl;
}

class XkbConnection {
    Display* m_display;
    int m_device_id;
public:
    XkbConnection() : m_display(nullptr), m_device_id(XkbUseCoreKbd) {
        char* displayName = strdup("");
        int eventCode;
        int errorCode;
        int major = XkbMajorVersion;
        int minor = XkbMinorVersion;
        int reason;

        m_display = XkbOpenDisplay(displayName, &eventCode, &errorCode, &major, &minor, &reason);

        switch (reason) {
        case XkbOD_BadLibraryVersion:
            throw std::runtime_error("The requested xkb version is not found");
        case XkbOD_ConnectionRefused:
            throw std::runtime_error("Unable to connect to X server");
        case XkbOD_BadServerVersion:
            throw std::runtime_error("X server does not support current xkb version");
        case XkbOD_NonXkbServer:
            throw std::runtime_error("The library xkb is not found");
        case XkbOD_Success:
            break;
        default:
            throw std::runtime_error("Unknown failure from XkbOpenDisplay");
        }
    }
    virtual ~XkbConnection() {
        XCloseDisplay(m_display);
    }
public:
    std::vector<std::pair<int, std::string>> groups() const {
        std::vector<std::pair<int, std::string>> groups;
        XkbDescPtr kb = XkbAllocKeyboard();
        XkbGetNames(m_display, XkbGroupNamesMask, kb);
        for (int i = 0; i < XkbNumKbdGroups; ++i) {
            if (kb->names->groups[i] > 0) {
                char* name = XGetAtomName(m_display, kb->names->groups[i]);
                groups.push_back(std::make_pair(i, std::string(name)));
                XFree(name);
            }
        }
        XkbFreeKeyboard(kb, 0, True);
        return groups;
    }
    int groupIndex() const {
        XkbStateRec state;
        XkbGetState(m_display, m_device_id, &state);
        return static_cast<int>(state.group);
    }
    void groupIndex(int index) {
        bool result = XkbLockGroup(m_display, m_device_id, index);
        if (!result) {
            throw std::runtime_error("Unable to send XkbLockGroup to X server");
        }
        XFlush(m_display);
    }
    void nextGroup() {
        auto g = groups();
        auto xi = groupIndex();
        int minGreaterIndex = XkbNumKbdGroups, minIndex = XkbNumKbdGroups;
        for (auto sg : g) {
            if (sg.first > xi && sg.first < minGreaterIndex) {
                minGreaterIndex = sg.first;
            }
            if (sg.first < minIndex) {
                minIndex = sg.first;
            }
        }
        if (minGreaterIndex < XkbNumKbdGroups) {
            groupIndex(minGreaterIndex);
        } else {
            groupIndex(minIndex);
        }
    }
};

XkbConnection* connection = nullptr;

class errno_runtime_error : public std::runtime_error {
private:
    static std::string make_error_message(int error, const char* prefix) {
        return std::string(prefix) + ": " + std::string(strerror(error));
    }
    static std::string make_error_message(int error, const std::string& prefix) {
        return prefix + ": " + std::string(strerror(error));
    }
public:
    explicit errno_runtime_error(int error) : std::runtime_error(strerror(error)) {
    }

    explicit errno_runtime_error(int error, const char* prefix) : std::runtime_error(make_error_message(error, prefix)) {
    }

    explicit errno_runtime_error(int error, const std::string& prefix) : std::runtime_error(make_error_message(error, prefix)) {
    }
};

class auto_close_watch {
    int fd;
    int wd;
public:
    explicit auto_close_watch(int fd, int wd) : fd(fd), wd(wd) {}
    ~auto_close_watch() {
        if (wd >= 0) {
            inotify_rm_watch(fd, wd);
        }
    }
    void close() {
        if (wd >= 0) {
            inotify_rm_watch(fd, wd);
            wd = -1;
        }
    }
};

class pid_file_watch {
    int inotify_fd;
    fs::path pid_file;

public:
    explicit pid_file_watch(fs::path pid_file) : inotify_fd{ -1 }, pid_file(pid_file) {
        inotify_fd = inotify_init();
        if (inotify_fd < 0) {
            throw errno_runtime_error(errno, "inotify_init()");
        }
    }

    /**
     * @brief Wait for PID to change (and be non-zero).
     *
     * This attempt to read the PID file.
     * If the file does not exists and/or does not contain PID, wait for the file to change.
     * Inotify can only be used on existing file, if the file does not exist, we wait
     * until the file is created within the directory.
     * If the directory does not exist, we wait on parent directory if there is such,
     * we assume there is at least one existing directory (i.e. root).
     *
     * Once the file is read and it contain non-zero PID different from the current one,
     * the function returns the new PID. If the file change, but the PID is the same
     * (i.e. updated last-modified time), we still wait for inotify events.
     *
     * @param old_pid The previously known PID, if any
     * @return int
     */
    int wait_for_pid_change(int old_pid = 0) {
    read_file_or_wait:
        auto new_pid = try_read_pid();
        if (new_pid > 0 && new_pid != old_pid) {
            return new_pid;
        }
        int watching_wd;
        while (1) {
            watching_wd = inotify_add_watch(inotify_fd, pid_file.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF);
            if (watching_wd < 0) {
                auto err = errno;
                if (err != ENOENT) {
                    std::stringstream ss;
                    ss << "inotify_add_watch" << "(" << pid_file << ")";
                    throw errno_runtime_error(err, ss.str());
                }
                wait_for_directory(pid_file.parent_path(), pid_file.filename().string());
                goto read_file_or_wait;
            }
            break;
        }
        {
            auto_close_watch watching_wd_close(inotify_fd, watching_wd);
            while (1) {
                char buffer[sizeof(inotify_event) + NAME_MAX + 1];
                inotify_event* event = reinterpret_cast<inotify_event*>(&buffer);
                auto read_size = read(inotify_fd, &buffer, sizeof(inotify_event) + NAME_MAX + 1);
                if (read_size < 0) {
                    throw errno_runtime_error(errno, "read()");
                }
                std::cout << "read(" << pid_file << ") = " << read_size << " (" << event->mask << ": ";
                if (event->mask & IN_MODIFY) {
                    std::cout << "IN_MODIFY ";
                }
                if (event->mask & IN_CLOSE_WRITE) {
                    std::cout << "IN_CLOSE_WRITE ";
                }
                if (event->mask & IN_DELETE_SELF) {
                    std::cout << "IN_DELETE_SELF ";
                }
                if (event->mask & IN_MOVE_SELF) {
                    std::cout << "IN_MOVE_SELF ";
                }
                std::cout << ")" << " (" << event->wd << ")" << std::endl;
                if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
                    return 0;
                }
                auto new_pid = try_read_pid();
                if (new_pid > 0 && new_pid != old_pid) {
                    return new_pid;
                }
            }
        }
    }

private:
    void wait_for_directory(fs::path directory, std::string entity) {
        int watching_wd = inotify_add_watch(inotify_fd, directory.c_str(), IN_CREATE | IN_DELETE_SELF | IN_MOVE_SELF);
        if (watching_wd < 0) {
            auto err = errno;
            if (!directory.has_parent_path() || err != ENOENT) {
                std::stringstream ss;
                ss << "inotify_add_watch" << "(" << directory << ")";
                throw errno_runtime_error(err, ss.str());
            }
            wait_for_directory(directory.parent_path(), directory.filename().string());
            return;
        }
        auto_close_watch watching_wd_close(inotify_fd, watching_wd);
        while (1) {
            char buffer[sizeof(inotify_event) + NAME_MAX + 1];
            inotify_event* event = reinterpret_cast<inotify_event*>(&buffer);
            auto read_size = read(inotify_fd, &buffer, sizeof(inotify_event) + NAME_MAX + 1);
            if (read_size < 0) {
                throw errno_runtime_error(errno, "read()");
            }
            std::cout << "read(" << directory << ") = " << read_size << " (" << event->mask << ": ";
            if (event->mask & IN_MODIFY) {
                std::cout << "IN_CREATE(" << event->name << ") ";
            }
            if (event->mask & IN_DELETE_SELF) {
                std::cout << "IN_DELETE_SELF ";
            }
            if (event->mask & IN_MOVE_SELF) {
                std::cout << "IN_MOVE_SELF ";
            }
            std::cout << ")" << " (" << event->wd << ")" << std::endl;
            if (event->mask & IN_CREATE) {
                if (std::string(event->name).compare(entity) == 0) {
                    return;
                } else {
                    continue;
                }
            } else if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
                return;
            }
        }
    }

    pid_t try_read_pid() {
        std::ifstream pid_file_stream(pid_file);
        std::string pid_string;
        std::getline(pid_file_stream, pid_string);
        try {
            auto pid = std::stoi(pid_string);
            return pid;
        } catch (const std::invalid_argument& e) {
        } catch (const std::out_of_range& e) {
        }
        return 0;
    }
};

pid_t observer_pid = 0;
std::recursive_mutex observer_pid_lock;
bool is_running = true;

pid_t get_observer_pid() {
    std::lock_guard guard(observer_pid_lock);
    return observer_pid;
}

pid_t set_current_pid(pid_t new_pid) {
    std::lock_guard guard(observer_pid_lock);
    auto old_pid = get_observer_pid();
    if (old_pid != new_pid) {
        observer_pid = new_pid;
    }
    return old_pid;
}

void observer_pid_file_thread(fs::path pid_file) {
    pid_file_watch watcher(pid_file);
    while (1) {
        try {
            auto old_pid = get_observer_pid();
            auto new_pid = watcher.wait_for_pid_change(old_pid);
            std::cout << "Updating PID from " << old_pid << " to " << new_pid << std::endl;
            if (new_pid > 0) {
                if (kill(new_pid, SIGUSR1) < 0) {
                    throw errno_runtime_error(errno, "kill()");
                }
            }
            set_current_pid(new_pid);
        } catch (...) {
            if (!is_running) {
                return;
            }
            throw;
        }
    }
}

void on_exit() {
    auto pid = get_observer_pid();
    if (pid > 0) {
        kill(pid, SIGUSR2);
    }
}

int main(int argc, char* argv[]) {
    std::string errorMessage;
    bool isHelpCall = false;
    Settings settings;
    for (int i = 1; i < argc; ++i) {
        auto arg = std::string(argv[i]);
        if (arg == "--help" || arg == "-h") {
            isHelpCall = true;
        } else if (arg == "--fork") {
            settings.forking = true;
        } else if (arg == "--pid-file" || arg == "-p") {
            if (i + 1 >= argc) {
                errorMessage = "Missing pid-file argument attribute";
            } else {
                settings.pidFile = argv[++i];
            }
        } else if (arg == "--display" || arg == "-d") {
            if (i + 1 >= argc) {
                errorMessage = "Missing display argument attribute";
            } else {
                settings.display = argv[++i];
            }
        } else {
            errorMessage = std::string("Unknown argument: ") + argv[i];
        }
    }
    if (!errorMessage.empty()) {
        std::cerr << errorMessage << std::endl;
        isHelpCall = true;
    }
    if (isHelpCall) {
        showHelp(argc, argv);
        return 0;
    }
    if (settings.forking) {
        auto pid = fork();
        if (pid < 0) {
            std::cerr << "Unable to fork the main process" << std::endl;
            return 0;
        } else if (pid != 0) {
            return 0;
        }
    }
    auto pidFilePath = fs::path(settings.pidFile);
    if (!pidFilePath.is_absolute()) {
        pidFilePath = fs::current_path() / pidFilePath;
    }
    std::cout << "PID: " << getpid() << std::endl;
    std::cout << "PID File: " << pidFilePath << std::endl;
    std::thread pid_watcher_thread(observer_pid_file_thread, pidFilePath);
    pid_watcher_thread.detach();
    while (1) {
        sigset_t set;
        siginfo_t sinfo;
        sigaddset(&set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &set, nullptr);
        auto received = sigwaitinfo(&set, &sinfo);
        if (received < 0) {
            std::cerr << "sigwaitinfo() failed: " << strerror(errno) << std::endl;
            continue;
        }
        std::cout << "Received signal: " << received << " from " << sinfo.si_pid << std::endl;
        if (received == SIGTERM || received == SIGINT) {
            exit(0);
        }
        if (received == SIGUSR1) {
            if (connection == nullptr) {
                connection = new XkbConnection();
            }
            bool is_authenticated = false;
            {
                std::lock_guard guard(observer_pid_lock);
                std::cout << "Observing PID: " << observer_pid << std::endl;
                std::cout << "Receiving PID: " << sinfo.si_pid << std::endl;
                if (observer_pid > 0 && sinfo.si_pid == observer_pid) {
                    is_authenticated = true;
                }
            }
            if (is_authenticated) {
                std::cout << "Signal SIGUSR1 authenticated: switcing keyboard layout" << std::endl;
                connection->nextGroup();
            }
        }
    }
    return 0;
}

