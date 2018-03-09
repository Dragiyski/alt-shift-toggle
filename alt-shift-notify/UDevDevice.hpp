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
 * File:   UDevDevice.hpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 *
 * Created on March 8, 2018, 7:16 PM
 */

#ifndef UDEVDEVICE_HPP
#define UDEVDEVICE_HPP

class UDevContext;
class UDevDevice;

#include "filesystem.hpp"
#include "UDevContext.hpp"
#include <libudev.h>
#include <string>
#include <map>

class UDevDevice {
    friend class UDevContext;
    struct udev_device *m_device;
private:
    std::map<std::string, std::string> listToMap(struct udev_list_entry *entry) const;
public:
    UDevDevice(const UDevContext& context, filesystem::path syspath) ;
    UDevDevice(const UDevDevice& source);
    UDevDevice(UDevDevice&& source);
    ~UDevDevice();
public:
    std::map<std::string, std::string> sysattr() const;
    std::map<std::string, std::string> tags() const;
    std::map<std::string, std::string> properties() const;
    std::map<std::string, std::string> devlinks() const;
    std::string getProperty(std::string name) const;
};

#endif /* UDEVDEVICE_HPP */

