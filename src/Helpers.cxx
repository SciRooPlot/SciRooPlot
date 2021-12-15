// PlottingFramework
//
// Copyright (C) 2019-2022  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see doc/CONTRIBUTORS.md
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Helpers.h"
#include <sys/stat.h>

namespace PlottingFramework
{

string expand_path(const string& path)
{
  char* raw = gSystem->ExpandPathName(path.data());
  string expandedPath(raw ? raw : "");
  delete[] raw;
  return expandedPath;
}

vector<string> split_string(const string& argString, char delimiter, bool onlyFirst)
{
  vector<string> arguments;
  if (onlyFirst) {
    auto delimiterPos = argString.find(delimiter);
    arguments.push_back(argString.substr(0, delimiterPos));
    if (delimiterPos != string::npos) {
      arguments.push_back(argString.substr(delimiterPos + 1));
    }
  } else {
    string curArg;
    std::istringstream argStream(argString);
    while (std::getline(argStream, curArg, delimiter)) {
      arguments.push_back(curArg);
    }
  }

  return arguments;
}

bool file_exists(const std::string& name)
{
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

} // end namespace PlottingFramework
