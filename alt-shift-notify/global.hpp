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
 * File:   error.hpp
 * Author: Plamen Dragiyski <plamen@dragiyski.org>
 *
 * Created on March 8, 2018, 10:54 PM
 */

#ifndef ERROR_HPP
#define ERROR_HPP

#define RETURN_CHECK_CALL(name, ...) \
{ \
    auto code = name(__VA_ARGS__); \
    if(code != 0) { \
        std::stringstream msg; \
        msg << #name << "() failed: " << strerror(code); \
        throw std::runtime_error(msg.str()); \
    } \
}

extern pthread_key_t exit_signal_handler;
void exitHandler(int);

#endif /* ERROR_HPP */

