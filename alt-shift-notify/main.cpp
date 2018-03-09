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

#include "KeyScanner.hpp"
#include "filesystem.hpp"
#include "global.hpp"
#include "UDevContext.hpp"
#include "UDevDevice.hpp"
#include "SubscriberList.hpp"
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

namespace {
    const char *pathPidFile = "/var/run/alt-shift-notify.pid";
    std::vector<std::unique_ptr<KeyScanner>> listeners;
    std::shared_ptr<SubscriberList> subscribers;
}

pthread_key_t exit_signal_handler;

std::vector<std::string> getKeyboardDevices() {
    std::vector<std::string> result;
    UDevContext udevContext;
    filesystem::path::string_type str_event = "event";
    for (auto& dir : filesystem::directory_iterator("/sys/class/input")) {
        filesystem::path::string_type filename = dir.path().filename();
        if (filename.substr(0, 0 + str_event.size()) == str_event) {
            UDevDevice udevDevice(udevContext, dir);
            if (udevDevice.getProperty("ID_INPUT_KEYBOARD") == "1") {
                result.push_back(std::string("/dev/input/") + filename);
            }
        }
    }
    return result;
}

pid_t getMainInstancePid() {
    pid_t instance;
    std::ifstream pidRead(pathPidFile);
    if (pidRead.fail()) {
        return 0;
    }
    pidRead >> instance;
    if (pidRead.fail()) {
        return 0;
    }
    std::stringstream instanceString;
    instanceString << instance;
    filesystem::path procPath = filesystem::path("/proc") / instanceString.str();
    if (!filesystem::is_directory(procPath)) {
        return 0;
    }
    return instance;
}

bool saveMainInstancePid() {
    std::ofstream pidWrite(pathPidFile);
    if (pidWrite.fail()) {
        return false;
    }
    pid_t pid = getpid();
    pidWrite << pid;
    if (pidWrite.fail()) {
        return false;
    }
    return true;
}

void signalSubscribe(int signal, siginfo_t *info, void*) {
    subscribers->subscribe(info->si_pid);
}

void signalUnsubscribe(int signal, siginfo_t *info, void*) {
    subscribers->unsubscribe(info->si_pid);
}

void signalExit(int signal) {
    for(auto& listener : listeners) {
        listener->interrupt();
    }
    listeners.clear();
    subscribers.reset();
    pthread_setspecific(exit_signal_handler, nullptr);
}

void exitHandler(int signal) {
    auto func = reinterpret_cast<void(*)(int)>(pthread_getspecific(exit_signal_handler));
    if(func == nullptr) {
        pthread_exit(nullptr);
    }
    func(signal);
}

void registerSignalHandlers() {
    struct sigaction subscribe, unsubscribe, exit;
    memset(&subscribe, 0, sizeof(subscribe));
    memset(&unsubscribe, 0, sizeof(unsubscribe));
    memset(&exit, 0, sizeof(exit));
    sigemptyset(&exit.sa_mask);
    sigaddset(&exit.sa_mask, SIGINT);
    sigaddset(&exit.sa_mask, SIGQUIT);
    sigaddset(&exit.sa_mask, SIGTERM);
    sigemptyset(&subscribe.sa_mask);
    sigemptyset(&unsubscribe.sa_mask);
    subscribe.sa_flags = unsubscribe.sa_flags = SA_NODEFER | SA_SIGINFO;
    subscribe.sa_sigaction = signalSubscribe;
    unsubscribe.sa_sigaction = signalUnsubscribe;
    exit.sa_handler = exitHandler;
    sigaction(SIGINT, &exit, nullptr);
    sigaction(SIGTERM, &exit, nullptr);
    sigaction(SIGQUIT, &exit, nullptr);
    sigaction(SIGUSR1, &subscribe, nullptr);
    sigaction(SIGUSR2, &unsubscribe, nullptr);
}

void waitForExitSignal() {
    sigset_t mask, procMask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    while(true) {
        sigprocmask(SIG_BLOCK, &mask, &procMask);
        sigsuspend(&procMask);
        sigprocmask(SIG_UNBLOCK, &mask, nullptr);
        if(listeners.size() == 0) {
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    {
        pid_t mainInstance = getMainInstancePid();
        if (mainInstance != 0) {
            std::cout << "Alt+Shift toggle notifier already running at PID: " << mainInstance << std::endl;
            return 0;
        }
        if (!saveMainInstancePid()) {
            std::cout << "Failed to create PID file: " << pathPidFile << std::endl;
            return 1;
        }
    }
    
    RETURN_CHECK_CALL(pthread_key_create, &exit_signal_handler, nullptr);
    RETURN_CHECK_CALL(pthread_setspecific, exit_signal_handler, reinterpret_cast<void*>(signalExit));

    subscribers = std::make_shared<SubscriberList>();
    std::vector<std::string> pathList = getKeyboardDevices();
    for (auto& path : pathList) {
        listeners.push_back(std::make_unique<KeyScanner>(path, subscribers));
    }
    registerSignalHandlers();
    waitForExitSignal();
    filesystem::remove(pathPidFile);
    pthread_key_delete(exit_signal_handler);
    return 0;
}