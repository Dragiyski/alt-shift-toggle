/*
 * Copyright (C) 2018 Plamen Dragiyski <plamen@dragiyski.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   main.cpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 *
 * Created on March 6, 2018, 11:15 AM
 */

#include <experimental/filesystem>
#include <iostream>
#include <fstream>

static const char *pathPidFile = "/var/run/alt-shift-toggle.pid";

bool altState = false, shiftState = false, groupState = false, failState = false;
std::set<pid_t> subscribers;
std::mutex subscribersLock;
std::thread listenThread;
bool shouldExit = false;
pid_t mainProcess;

void subscribeHandler(int signal, siginfo_t *info, void*) {
    subscribersLock.lock();
    subscribers.insert(info->si_pid);
    subscribersLock.unlock();
}

void unsubscribeHandler(int signal, siginfo_t *info, void*) {
    subscribersLock.lock();
    subscribers.erase(info->si_pid);
    subscribersLock.unlock();
}

std::ifstream evt;

void threadExitHandler(int signal) {
    if(evt.is_open()) {
        evt.close();
    }
    shouldExit = true;
}

void threadTerminate() {
    if(evt.is_open()) {
        evt.close();
    }
    exit(0);
}

void makeHandlerEvents() {
    struct sigaction subscribeAction, unsubscribeAction, exitAction;
    memset(&subscribeAction, 0, sizeof(subscribeAction));
    memset(&unsubscribeAction, 0, sizeof(unsubscribeAction));
    memset(&exitAction, 0, sizeof(exitAction));
    sigset_t mask, exitMask;
    sigemptyset(&mask);
    sigemptyset(&exitMask);
    sigaddset(&exitMask, SIGINT);
    sigaddset(&exitMask, SIGQUIT);
    sigaddset(&exitMask, SIGTERM);
    subscribeAction.sa_mask = unsubscribeAction.sa_mask = mask;
    subscribeAction.sa_flags = unsubscribeAction.sa_flags = SA_SIGINFO;
    subscribeAction.sa_sigaction = subscribeHandler;
    unsubscribeAction.sa_sigaction = unsubscribeHandler;
    exitAction.sa_handler = threadExitHandler;
    exitAction.sa_mask = exitMask;
    sigaction(SIGUSR1, &subscribeAction, nullptr);
    sigaction(SIGUSR2, &unsubscribeAction, nullptr);
    sigaction(SIGINT, &exitAction, nullptr);
    sigaction(SIGQUIT, &exitAction, nullptr);
    sigaction(SIGTERM, &exitAction, nullptr);
    std::set_terminate(threadTerminate);
}
void listenKeys(std::string device) {
    makeHandlerEvents();
    struct input_event event;
    evt.open(device, std::ios_base::binary);
    evt.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    while(!shouldExit) {
        evt.read(reinterpret_cast<std::ifstream::char_type*>(&event), sizeof(event));
        // if(evt.gcount() != sizeof(event)) {
        //     throw std::runtime_error("The number of bytes in event does not match the number of bytes in input_event struct");
        // }
        // std::cout << "Event at " << event.time.tv_sec << "." << std::setw(6) << std::setfill('0') << event.time.tv_usec << std::endl;
        // std::cout << " - Type: " << event.type << std::endl;
        // std::cout << " - Code: " << event.code << std::endl;
        // std::cout << " - Value: " << event.value << std::endl;
        bool hasChange = false, anotherKey = false;
        if(event.type == EV_KEY) {
            if(event.code == KEY_LEFTALT) {
                if(event.value == 0) {
                    altState = false;
                    hasChange = true;
                } else if(event.value == 1) {
                    altState = true;
                    hasChange = true;
                }
            } else if(event.code == KEY_LEFTSHIFT) {
                if(event.value == 0) {
                    shiftState = false;
                    hasChange = true;
                } else if (event.value == 1) {
                    shiftState = true;
                    hasChange = true;
                }
            } else {
                //std::cout << "Key: " << event.code << " -> " << event.value << std::endl;
                anotherKey = true;
            }
        }
        if(hasChange) {
            // std::cout << "has change on <shift> or <alt>" << std::endl;
            if(altState && shiftState) {
                // std::cout << "<shift> and <alt> => group: true" << std::endl;
                groupState = true;
            } else if(groupState) {
                // std::cout << "!<shift> or !<alt> => group: false" << std::endl;
                groupState = false;
                if(!failState) {
                    // std::cout << "<shift> or <alt> and group set, changing language" << std::endl;
                    subscribersLock.lock();
                    for(pid_t subscriber : subscribers) {
                        kill(subscriber, SIGUSR1);
                    }
                    subscribersLock.unlock();
                }
            }
            if(!altState && !shiftState) {
                // std::cout << "!<shift> and !<alt> => fail: false" << std::endl;
                failState = false;
            }
        } else if(anotherKey && groupState) {
            // std::cout << "another key => fail: true" << std::endl;
            failState = true;
        }
    }
}

void assignExitHandlers();

int application() {
    int eventIndex = -1;
    {
        std::ifstream file("/proc/bus/input/handlers");
        std::string line;
        while(std::getline(file, line)) {
            std::map<std::string, std::string> kvPairs;
            std::stringstream lineStream(line);
            std::string keyValue;
            while(std::getline(lineStream, keyValue, ' ')) {
                std::string::size_type eq_pos = keyValue.find('=');
                if(eq_pos == std::string::npos) {
                    continue;
                }
                std::string key(keyValue.substr(0, eq_pos));
                std::string value(keyValue.substr(eq_pos + 1));
                kvPairs.insert_or_assign(key, value);
            }
            if(kvPairs.size() > 0) {
                if(kvPairs.at("Name") == "evdev") {
                    eventIndex = std::stoi(kvPairs.at("Number"));
                }
            }
        }
    }

    if(eventIndex < 0) {
        std::cout << "Unable to find keyboard event index" << std::endl;
        return 1;
    }

    std::stringstream file;
    file << "/dev/input/event" << eventIndex;
    listenThread = std::thread(listenKeys, file.str());
    
    assignExitHandlers();
    while(!shouldExit) {
        pause();
    }
    return 0;
}

int cppStat(std::string filename, struct stat* buffer) {
    return stat(filename.c_str(), buffer);
}

bool isValidPid(std::string pid) {
    if(pid.size() <= 0) {
        return false;
    }
    if(pid.size() > 20) {
        return false;
    }
    for(auto c : pid) {
        if(c < 0x30 || c > 0x39) {
            return false;
        }
    }
    return true;
}

void exitHandler(int signal) {
    std::cout << "ExitHandler called for " << getpid() << std::endl;
    if(getpid() != mainProcess) {
        exit(0);
    }
    std::experimental::filesystem::path path(pathPidFile);
    try {
        std::experimental::filesystem::remove(path);
    } catch(std::experimental::filesystem::filesystem_error& e) {
    }
    shouldExit = true;
    pthread_kill(listenThread.native_handle(), SIGTERM);
    listenThread.join();
    if(signal == SIGINT) {
        exit(0);
    }
    exit(1);
}

void assignExitHandlers() {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    action.sa_handler = exitHandler;
    action.sa_mask = mask;
    action.sa_flags = SA_NOCLDSTOP;
    
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGQUIT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}

int main(int argc, char *argv[]) {
    mainProcess = getpid();
    std::experimental::filesystem::path path(pathPidFile);
    std::ifstream file(path);
    if(!file.fail()) {
        std::string pid;
        std::getline(file, pid);
        if(isValidPid(pid)) {
            struct stat fileStat;
            if(cppStat("/proc/" + pid, &fileStat) == 0) {
                if(fileStat.st_mode & S_IFDIR) {
                    // Application already running
                    std::cout << "Application already running with pid: " << pid << std::endl;
                    return 0;
                }
            }
        }
    }

    {
        std::ofstream pidFile(path);
        pidFile.exceptions(std::ofstream::badbit | std::ofstream::failbit);
        pidFile << getpid() << std::endl;
    }

    int exitCode = application();

    try {
        std::experimental::filesystem::remove(path);
    } catch(std::experimental::filesystem::filesystem_error& e) {
    }

    return exitCode;
}