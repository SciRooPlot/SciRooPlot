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

#include "util/Paths.h"

#include "SciRooPlot/Logger.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <sys/stat.h>

using std::string;

namespace SciRooPlot::util
{
// expands a leading ~ and environment variables ($VAR, ${VAR}) like bash; wildcards are matched where the paths are used
// returns an empty string if a variable is not set
string expand_path(const string& path)
{
  string expandedPath;
  size_t pos = 0;
  if (path == "~" || path.rfind("~/", 0) == 0) {
    if (const char* home = std::getenv("HOME")) {
      expandedPath = home;
      pos = 1;
    }
  }
  while (pos < path.size()) {
    const size_t dollar = path.find('$', pos);
    expandedPath += path.substr(pos, dollar - pos);
    if (dollar == string::npos) break;
    size_t nameStart = dollar + 1;
    size_t nameEnd = nameStart;
    size_t next = 0;
    if (nameStart < path.size() && path[nameStart] == '{') {
      nameEnd = path.find('}', ++nameStart);
      if (nameEnd == string::npos) nameEnd = nameStart;  // unterminated: kept as it is
      next = nameEnd + 1;
    } else if (nameStart < path.size() && (std::isalpha(static_cast<unsigned char>(path[nameStart])) || path[nameStart] == '_')) {
      while (nameEnd < path.size() && (std::isalnum(static_cast<unsigned char>(path[nameEnd])) || path[nameEnd] == '_'))
        ++nameEnd;
      next = nameEnd;
    }
    if (nameEnd == nameStart) {
      // no variable name: the $ is part of the path
      expandedPath += '$';
      pos = dollar + 1;
      continue;
    }
    const string name = path.substr(nameStart, nameEnd - nameStart);
    const char* value = std::getenv(name.data());
    if (!value) {
      logger::error("Environment variable {} used in path {} is not set.", name, path);
      return "";
    }
    expandedPath += value;
    pos = next;
  }
  return expandedPath;
}

bool file_exists(const string& name)
{
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}
}  // end namespace SciRooPlot::util
