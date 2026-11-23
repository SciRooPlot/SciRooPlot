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

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
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

string join_strings(const vector<string>& items, const string& separator)
{
  string joined;
  for (size_t i = 0; i < items.size(); ++i) {
    joined += ((i) ? separator : "") + items[i];
  }
  return joined;
}

size_t edit_distance(const string& a, const string& b)
{
  // rows i - 2, i - 1 and i of the table of distances between the first i characters of a and the first j of b
  vector<size_t> beforePrevious(b.size() + 1), previous(b.size() + 1), current(b.size() + 1);
  for (size_t j = 0; j <= b.size(); ++j)
    previous[j] = j;
  for (size_t i = 1; i <= a.size(); ++i) {
    current[0] = i;
    for (size_t j = 1; j <= b.size(); ++j) {
      const size_t replaced = previous[j - 1] + (a[i - 1] != b[j - 1]);
      current[j] = std::min({previous[j] + 1, current[j - 1] + 1, replaced});
      if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) current[j] = std::min(current[j], beforePrevious[j - 2] + 1);
    }
    std::swap(beforePrevious, previous);
    std::swap(previous, current);
  }
  return previous[b.size()];
}
}  // end namespace SciRooPlot::util
