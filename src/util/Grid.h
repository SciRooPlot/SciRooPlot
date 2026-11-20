/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot -------------------------------------
 * Copyright (c) 2019-2026 Mario Krüger
 * Contact: mario.kruger@cern.ch
 * For a full list of contributors please see doc/CONTRIBUTORS.md.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation in version 3 (or later) of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * The GNU General Public License can be found here: <https://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef SRC_UTIL_GRID_H_
#define SRC_UTIL_GRID_H_

#include <optional>
#include <string>
#include <vector>

namespace SciRooPlot::util
{
// a path on the grid (alien://): reached through ROOT's JAliEn plugin and a connection to the grid
bool is_grid_path(const std::string& path);
// connects to the grid once per process when the first grid path is needed; true if the path is none or the connection is open
bool connect_grid_for(const std::string& path);
// why a grid path cannot be read, if the reason is known (no connection and why, else what to check): ": reason" or empty
std::string grid_failure_reason(const std::string& path);
// the sorted names in a grid directory (directories without their trailing /); nullopt if the path is no directory or cannot be listed
std::optional<std::vector<std::string>> list_grid_directory(const std::string& url);
// whether a file or directory exists on the grid
bool grid_path_exists(const std::string& url);
}  // namespace SciRooPlot::util

#endif  // SRC_UTIL_GRID_H_
