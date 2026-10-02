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

#include "util/Strings.h"

#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace SciRooPlot::util
{
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
}  // end namespace SciRooPlot::util
