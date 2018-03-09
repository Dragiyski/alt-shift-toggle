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
 * File:   SubscriberList.hpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 *
 * Created on March 8, 2018, 4:34 PM
 */

#ifndef SUBSCRIBERLIST_HPP
#define SUBSCRIBERLIST_HPP

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <signal.h>

class SubscriberList {
    std::map<pid_t, std::string> m_data;
    std::mutex m_lock_data;
public:
    SubscriberList() = default;
    SubscriberList(const SubscriberList&) = delete;
    SubscriberList(SubscriberList&&) = default;
    virtual ~SubscriberList();
public:
    std::set<pid_t> getSubscribers();
    void subscribe(pid_t);
    void unsubscribe(pid_t);
};

#endif /* SUBSCRIBERLIST_HPP */

