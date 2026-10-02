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

#ifndef SRC_UTIL_REGEX_H_
#define SRC_UTIL_REGEX_H_

#include <regex>
#include <string>

namespace SciRooPlot::util
{
// matches a text against a regular expression (the whole text or, if contains is set, any part of it)
class RegexMatcher
{
 public:
  RegexMatcher(const std::string& pattern, bool contains, bool ignoreCase) : mContains(contains)
  {
    try {
      auto flags = std::regex_constants::ECMAScript;
      if (ignoreCase) {
        flags |= std::regex_constants::icase;
      }

      mRegex = std::regex(pattern, flags);
      mValid = true;
    } catch (const std::regex_error&) {
      mValid = false;
    }
  }

  // matches also all subpaths of the matching paths (e.g. pattern 'a/b' matches 'a/b/c')
  static RegexMatcher WithSubpaths(const std::string& pattern, bool contains, bool ignoreCase)
  {
    RegexMatcher matcher(pattern, contains, ignoreCase);
    if (!matcher.IsValid()) return matcher;  // the pattern must be valid by itself, not only once it is wrapped
    return RegexMatcher("(?:" + pattern + ")(?:/.*)?", contains, ignoreCase);
  }

  bool IsValid() const { return mValid; }
  bool Matches(const std::string& text) const
  {
    if (!mValid) return false;
    if (mContains) {
      return std::regex_search(text, mRegex);
    }
    return std::regex_match(text, mRegex);
  }

 private:
  bool mValid = false;
  bool mContains = false;
  std::regex mRegex;
};
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_REGEX_H_
