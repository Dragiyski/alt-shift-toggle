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
 * File:   UDevDevice.cpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 * 
 * Created on March 8, 2018, 7:16 PM
 */

#include "UDevDevice.hpp"

std::map<std::string, std::string> UDevDevice::listToMap(struct udev_list_entry *entry) const {
    std::map<std::string, std::string> result;
    while (entry != nullptr) {
        const char *c_name = udev_list_entry_get_name(entry);
        if (c_name == nullptr) {
            throw std::runtime_error("udev_list_entry_get_name() failed");
        }
        std::string name(c_name);
        const char *c_value = udev_list_entry_get_value(entry);
        if (c_value == nullptr) {
            result.insert(std::make_pair(name, std::string{}));
        } else {
            result.insert(std::make_pair(name, std::string(c_value)));
        }
        entry = udev_list_entry_get_next(entry);
    }
    return result;
}

UDevDevice::UDevDevice(const UDevContext& context, filesystem::path syspath) : m_device{udev_device_new_from_syspath(context.m_context, syspath.c_str())}
{
    if (m_device == nullptr) {
        throw std::runtime_error("udev_device_new_from_syspath() failed");
    }
}
UDevDevice::UDevDevice(const UDevDevice& source) : m_device{udev_device_ref(source.m_device)}
{
}
UDevDevice::UDevDevice(UDevDevice&& source) : m_device{source.m_device}
{
    source.m_device = nullptr;
}

UDevDevice::~UDevDevice() {
    if (m_device != nullptr) {
        udev_device_unref(m_device);
    }
}

std::map<std::string, std::string> UDevDevice::sysattr() const {
    return listToMap(udev_device_get_sysattr_list_entry(m_device));
}

std::map<std::string, std::string> UDevDevice::tags() const {
    return listToMap(udev_device_get_tags_list_entry(m_device));
}

std::map<std::string, std::string> UDevDevice::properties() const {
    return listToMap(udev_device_get_properties_list_entry(m_device));
}

std::map<std::string, std::string> UDevDevice::devlinks() const {
    return listToMap(udev_device_get_devlinks_list_entry(m_device));
}

std::string UDevDevice::getProperty(std::string name) const {
    const char *c_value = udev_device_get_property_value(m_device, name.c_str());
    if (c_value == nullptr) {
        return "";
    }
    return c_value;
}
