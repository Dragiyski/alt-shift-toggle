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
 * File:   SubscriberList.cpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 * 
 * Created on March 8, 2018, 4:34 PM
 */

#include "SubscriberList.hpp"
#include "process-tag.hpp"

SubscriberList::~SubscriberList() {
}

void SubscriberList::subscribe(pid_t process) {
    std::unique_lock lock(m_lock_data);
    std::optional<std::string> tag = processTag(process);
    if(!tag) {
        return;
    }
    m_data.insert(std::make_pair(process, tag.value()));
}

void SubscriberList::unsubscribe(pid_t process) {
    std::unique_lock lock(m_lock_data);
    m_data.erase(process);
}

std::set<pid_t> SubscriberList::getSubscribers() {
    std::unique_lock lock(m_lock_data);
    std::set<pid_t> result;
    for(auto& kv : m_data) {
        std::optional<std::string> currentTag = processTag(kv.first);
        if(currentTag && currentTag.value() == kv.second) {
            result.insert(kv.first);
        }
    }
    return result;
}