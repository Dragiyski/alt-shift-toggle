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
 * File:   UDevContext.cpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 * 
 * Created on March 8, 2018, 7:15 PM
 */

#include "UDevContext.hpp"
#include <stdexcept>

UDevContext::UDevContext() : m_context{udev_new()}
{
    if (m_context == nullptr) {
        throw std::runtime_error("udev_new() failed");
    }
}

UDevContext::UDevContext(const UDevContext& source) : m_context{udev_ref(source.m_context)}
{
}

UDevContext::UDevContext(UDevContext&& source) : m_context{source.m_context}
{
    source.m_context = nullptr;
}

UDevContext::~UDevContext() {
    if (m_context != nullptr) {
        udev_unref(m_context);
    }
}