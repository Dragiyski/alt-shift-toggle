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
 * File:   process-tag.hpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 *
 * Created on March 8, 2018, 5:16 PM
 */

#ifndef PROCESS_TAG_HPP
#define PROCESS_TAG_HPP

#include <signal.h>
#include <string>
#include <optional>

std::optional<std::string> processTag(pid_t process);

#endif /* PROCESS_TAG_HPP */

