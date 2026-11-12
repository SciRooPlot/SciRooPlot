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
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "util/Strings.h"

using std::string;
using std::vector;

namespace SciRooPlot::util
{
vector<string> split_input_path(const string& input)
{
  if (is_remote(input)) {
    // a remote file may carry a query (file.root?authz=...): the folder follows after it
    for (size_t pos = input.find(".root"); pos != string::npos; pos = input.find(".root", pos + 1)) {
      size_t end = pos + 5;
      if (end < input.size() && input[end] == '?') end = input.find(':', end);
      if (end == string::npos || end == input.size()) return {input};
      if (input[end] == ':') return {input.substr(0, end), input.substr(end + 1)};
    }
    return {input};
  }
  const size_t first = input.find(".root:");
  if (first == string::npos) return {input};
  std::error_code ec;
  for (size_t pos = first; pos != string::npos; pos = input.find(".root:", pos + 1)) {
    if (std::filesystem::is_regular_file(input.substr(0, pos + 5), ec)) return {input.substr(0, pos + 5), input.substr(pos + 6)};
  }
  if (std::filesystem::exists(input, ec)) return {input};
  return {input.substr(0, first + 5), input.substr(first + 6)};
}

// expands a leading ~ and environment variables ($VAR, ${VAR}) like bash; wildcards are matched where the paths are used
// nullopt if a variable is not set (reported here)
std::optional<string> expand_path(const string& path)
{
  string expandedPath;
  size_t pos = 0;
  if (path == "~" || str_starts_with(path, "~/")) {
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
      return std::nullopt;
    }
    expandedPath += value;
    pos = next;
  }
  return expandedPath;
}
}  // end namespace SciRooPlot::util
