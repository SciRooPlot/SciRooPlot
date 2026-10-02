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

#include <TSystem.h>

#include <string>
#include <sys/stat.h>

using std::string;

namespace SciRooPlot::util
{
string expand_path(const string& path)
{
  char* raw = gSystem->ExpandPathName(path.data());
  string expandedPath(raw ? raw : "");
  delete[] raw;
  return expandedPath;
}

bool file_exists(const string& name)
{
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}
}  // end namespace SciRooPlot::util
