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

#include "process-tag.hpp"

#include <experimental/filesystem>
#include <fstream>
#include <sstream>

namespace filesystem = std::experimental::filesystem;

std::optional<std::string> processTag(pid_t process) {
    filesystem::path processPath;
    std::string tag;
    {
        std::stringstream pp;
        pp << "/proc/" << process;
        processPath = pp.str();
    }
    {
        std::ifstream s(processPath / "stat");
        std::string buf;
        s.exceptions(std::ifstream::badbit);
        if(s.fail()) {
            return std::optional<std::string>{};
        }
        for(int i = 0; i < 22; ++i) {
            std::getline(s, buf, ' ');
        }
        return buf;
    }
}