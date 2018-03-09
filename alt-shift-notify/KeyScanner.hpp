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
 * File:   KeyScanner.hpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 *
 * Created on March 7, 2018, 11:10 PM
 */

#ifndef KEYSCANNER_HPP
#define KEYSCANNER_HPP

#include "SubscriberList.hpp"

#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <signal.h>
#include <thread>

class KeyScanner {
    std::thread m_thread_listen;
    std::ifstream m_scanner;
    std::shared_ptr<SubscriberList> m_subscribers;
    bool altState, shiftState, groupState, failState;
    
private:
    static pthread_key_t _thread_self_key;
    static size_t _instance_count;
    static void registerExitHandler(KeyScanner*);
    static void signalExit(int);
private:
    void _thread();
public:
    explicit KeyScanner(std::string path, std::shared_ptr<SubscriberList> subscribers);
    KeyScanner(const KeyScanner&) = delete;
    KeyScanner(KeyScanner&&) = default;
    virtual ~KeyScanner();
public:
    void interrupt();
};

#endif /* KEYSCANNER_HPP */

