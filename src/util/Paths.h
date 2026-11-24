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
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

#include "util/Strings.h"

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

// the file name of an input without the query of a remote file (for its file type)
inline std::string input_file_name(const std::string& filePart)
{
  return is_remote(filePart) ? filePart.substr(0, filePart.find('?')) : filePart;
}

// an input that is a ROOT file (otherwise a table)
inline bool is_root_input(const std::string& filePart)
{
  const std::string name = input_file_name(filePart);
  return str_ends_with(name, ".root");
}

// a local file that does not exist (whether a remote file exists shows when it is opened); ec tells why a file could not be checked
inline bool is_missing_local_file(const std::string& fileName, std::error_code& ec)
{
  return !is_remote(fileName) && !std::filesystem::exists(fileName, ec);
}

// an input file.root:folder/sub is split after the .root ending (a directory in the path may contain ':', e.g. a time stamp, or
// even '.root:': split after an existing file, an existing path is not split, otherwise after the first .root); a remote file after
// its ending or the query following it
std::vector<std::string> split_input_path(const std::string& input);
// creates the folder of the local copies (srp cache) when missing and marks it as cache (CACHEDIR.TAG, skipped by backup tools); false if
// it cannot be made
bool make_cache_folder(const std::string& folder, std::error_code& ec);

// folder ("" if none) and name of a path within a file or data source (a/b/c: a/b and c)
inline std::pair<std::string, std::string> split_path(const std::string& path)
{
  const size_t pos = path.rfind('/');
  if (pos == std::string::npos) return {"", path};
  return {path.substr(0, pos), path.substr(pos + 1)};
}

// the file written for an output file: where it points if it is a symbolic link, also to a file that does not exist yet (the target
// replaced, the link kept; links to links followed), itself otherwise or for links in a loop (warned about if warn is set)
std::string written_file(const std::string& file, bool warn = false);

// an output file is written to a temporary file beside it (beside the target of a link) and renamed once complete (replace_file), so
// that an interrupted write, a failed one or two runs finishing at the same time cannot leave a truncated file behind
inline std::string temporary_file(const std::string& file)
{
  return written_file(file, true) + "." + std::to_string(getpid()) + ".tmp";
}
inline bool replace_file(const std::string& tmpFile, const std::string& file, std::error_code& ec)
{
  std::filesystem::rename(tmpFile, written_file(file), ec);
  if (!ec) return true;
  std::error_code removeError;
  std::filesystem::remove(tmpFile, removeError);
  return false;
}
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_PATHS_H_
