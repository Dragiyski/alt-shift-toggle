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
 * Created on March 6, 2018, 11:43 PM
 */

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <X11/XKBlib.h>
#include <unistd.h>

static const char *pathPidFile = "/var/run/alt-shift-notify.pid";

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

        switch(reason) {
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
        for(int i = 0; i < XkbNumKbdGroups; ++i) {
            if(kb->names->groups[i] > 0) {
                char *name = XGetAtomName(m_display, kb->names->groups[i]);
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
        if(!result) {
            throw std::runtime_error("Unable to send XkbLockGroup to X server");
        }
        XFlush(m_display);
    }
    void nextGroup() {
        auto g = groups();
        auto xi = groupIndex();
        int minGreaterIndex = XkbNumKbdGroups, minIndex = XkbNumKbdGroups;
        for(auto sg : g) {
            if(sg.first > xi && sg.first < minGreaterIndex) {
                minGreaterIndex = sg.first;
            }
            if(sg.first < minIndex) {
                minIndex = sg.first;
            }
        }
        if(minGreaterIndex < XkbNumKbdGroups) {
            groupIndex(minGreaterIndex);
        } else {
            groupIndex(minIndex);
        }
    }
};

pid_t toggler = 0;
XkbConnection *connection = nullptr;

void exitHandler(int signal) {
    if(connection != nullptr) {
        delete connection;
        connection = nullptr;
    }
    if(toggler != 0) {
        kill(toggler, SIGUSR2);
        toggler = 0;
    }
    exit(0);
}

void receivedToggle(int signal, siginfo_t *info, void*) {
    if(connection == nullptr) {
        connection = new XkbConnection();
        {
            struct sigaction action;
            memset(&action, 0, sizeof(action));
            sigset_t mask;
            sigemptyset(&mask);
            action.sa_handler = exitHandler;
            action.sa_flags = SA_RESETHAND;
            sigaction(SIGINT, &action, nullptr);
        }
    }
    if(toggler == 0) {
        return;
    }
    if(info->si_pid != toggler) {
        return;
    }
    connection->nextGroup();
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

int main(int argc, char** argv) {
    bool flagFork = false;
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--fork") == 0) {
            flagFork = true;
        }
    }
    if(flagFork) {
        pid_t result = fork();
        if(result == -1) {
            // No child has been created
            std::stringstream msg;
            msg << "Unable to fork process: " << strerror(errno);
            throw std::runtime_error(msg.str());
        } else if(result != 0) {
            // This if happens only in the parent process, result is the child PID.
            // We just exit from here.
            return 0;
        }
        // If we are in the child we do whatever we would do in no fork (e.g. waiting)
    }
    
    {
        std::ifstream pidFile(pathPidFile);
        if(pidFile.fail()) {
            std::cout << "Unable to locate alt-shift-toggle program" << std::endl;
            return 1;
        }
        std::string pid;
        std::getline(pidFile, pid);
        if(!isValidPid(pid)) {
            std::cout << "The PID file for alt-shift-toggle program contains invalid string" << std::endl;
            return 1;
        }
        struct stat fileStat;
        if(cppStat("/proc/" + pid, &fileStat) != 0) {
            std::cout << "Unable to locate alt-shift-toggle program, PID file suggests " << pid << " but the process does not exists" << std::endl;
            return 1;
        }
        toggler = std::stoi(pid);
    }
    
    // Subscribe
    int subscribeResult = kill(toggler, SIGUSR1);
    if(subscribeResult < 0) {
        std::cout << "Unable to subscribe to alt-shift-toggle program: " << strerror(errno) << std::endl;
        return 1;
    }
    
    {
        struct sigaction action;
        sigset_t mask;
        sigemptyset(&mask);
        memset(&action, 0, sizeof(action));
        action.sa_sigaction = receivedToggle;
        action.sa_flags = SA_SIGINFO;
        action.sa_mask = mask;
        sigaction(SIGUSR1, &action, nullptr);
    }
    
    while(true) {
        pause();
    }
    
    return 0;
}

