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
 * File:   KeyScanner.cpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 * 
 * Created on March 7, 2018, 11:10 PM
 */

#include "KeyScanner.hpp"
#include "global.hpp"
#include <linux/input.h>
#include <iostream>
#include <pthread.h>
#include <cstring>
#include <sstream>

pthread_key_t KeyScanner::_thread_self_key = 0;
size_t KeyScanner::_instance_count = 0;

void KeyScanner::registerExitHandler(KeyScanner *self) {
    if(_thread_self_key == 0) {
        RETURN_CHECK_CALL(pthread_key_create, &_thread_self_key, nullptr);
    }
    RETURN_CHECK_CALL(pthread_setspecific, _thread_self_key, self);
    RETURN_CHECK_CALL(pthread_setspecific, exit_signal_handler, reinterpret_cast<void*>(KeyScanner::signalExit));
    _instance_count++;
    {
        struct sigaction exitAction;
        memset(&exitAction, 0, sizeof(exitAction));
        sigemptyset(&exitAction.sa_mask);
        sigaddset(&exitAction.sa_mask, SIGINT);
        sigaddset(&exitAction.sa_mask, SIGQUIT);
        sigaddset(&exitAction.sa_mask, SIGTERM);
        exitAction.sa_handler = ::exitHandler;
        sigaction(SIGINT, &exitAction, nullptr);
        sigaction(SIGQUIT, &exitAction, nullptr);
        sigaction(SIGTERM, &exitAction, nullptr);
    }
}

void KeyScanner::signalExit(int signal) {
    KeyScanner *self = reinterpret_cast<KeyScanner*>(pthread_getspecific(_thread_self_key));
    if(self == nullptr) {
        pthread_exit(0);
    }
    self->m_scanner.close();
    pthread_setspecific(_thread_self_key, nullptr);
    pthread_setspecific(exit_signal_handler, nullptr);
    pthread_exit(0);
}

KeyScanner::KeyScanner(std::string path, std::shared_ptr<SubscriberList> subscribers) :
m_subscribers(subscribers),
altState{false},
shiftState{false},
groupState{false},
failState{false}
{
    m_scanner.exceptions(std::ifstream::badbit);
    m_scanner.open(path, std::ifstream::binary);
    
    if(m_scanner.fail()) {
        std::stringstream msg;
        msg << "Unable to open device: " << path;
        throw std::runtime_error(msg.str());
    }
    
    m_thread_listen = std::thread(&KeyScanner::_thread, this);
}

KeyScanner::~KeyScanner() {
    if(_instance_count > 0) {
        if(--_instance_count == 0) {
            pthread_key_delete(_thread_self_key);
            _thread_self_key = 0;
        }
    }
}

void KeyScanner::_thread() {
    registerExitHandler(this);
    struct input_event event;
    while(m_scanner.is_open()) {
        m_scanner.read(reinterpret_cast<std::ifstream::char_type*>(&event), sizeof(event));
        if(m_scanner.fail()) {
            return;
        }
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
                anotherKey = true;
            }
        }
        if(hasChange) {
            if(altState && shiftState) {
                groupState = true;
            } else if(groupState) {
                groupState = false;
                if(!failState) {
                    for(pid_t subscriber : m_subscribers->getSubscribers()) {
                        kill(subscriber, SIGUSR1);
                    }
                }
            }
            if(!altState && !shiftState) {
                failState = false;
            }
        } else if(anotherKey && groupState) {
            failState = true;
        }
    }
}

void KeyScanner::interrupt() {
    pthread_kill(m_thread_listen.native_handle(), SIGQUIT);
    m_thread_listen.join();
}
