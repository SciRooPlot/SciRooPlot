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

#ifndef SRC_UTIL_PATHS_H_
#define SRC_UTIL_PATHS_H_

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <utility>

namespace SciRooPlot::util
{
std::optional<std::string> expand_path(const std::string& path);

// a URL like root://host//path or https://host/path: read through ROOT's network plugins, not the local file system
inline bool is_remote(const std::string& path)
{
  const size_t schemeEnd = path.find("://");
  if (schemeEnd == std::string::npos || schemeEnd == 0 || !std::isalpha(static_cast<unsigned char>(path[0]))) return false;
  return std::all_of(path.begin(), path.begin() + schemeEnd, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.'; });
}

// folder ("" if none) and name of a path within a file or data source (a/b/c: a/b and c)
inline std::pair<std::string, std::string> split_path(const std::string& path)
{
  const size_t pos = path.rfind('/');
  if (pos == std::string::npos) return {"", path};
  return {path.substr(0, pos), path.substr(pos + 1)};
}
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_PATHS_H_
