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

#ifndef SRC_VALIDATION_H_
#define SRC_VALIDATION_H_

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SciRooPlot
{
// output modes of GeneratePlots (in addition: gif+<centiseconds between frames>)
inline const std::vector<std::string>& plot_modes()
{
  static const std::vector<std::string> modes{"show", "print", "file", "data", "macro", "pdf", "eps", "ps", "svg", "png", "jpg", "gif", "html", "json", "xml", "root"};
  return modes;
}

inline bool is_valid_plot_mode(const std::string& mode)
{
  if (std::find(plot_modes().begin(), plot_modes().end(), mode) != plot_modes().end()) return true;
  return mode.size() > 4 && mode.rfind("gif+", 0) == 0 && mode.find_first_not_of("0123456789", 4) == std::string::npos;
}

// returns the first illegal character (for non-ASCII characters the complete UTF-8 sequence, so it can be printed)
inline std::optional<std::string> find_illegal_name_char(const std::string& name, bool allowSlash = false)
{
  static constexpr std::string_view kAllowedChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-/";
  const std::string_view allowed =
    allowSlash ? kAllowedChars : kAllowedChars.substr(0, kAllowedChars.size() - 1);
  const auto pos = name.find_first_not_of(allowed);
  if (pos == std::string::npos) return std::nullopt;
  size_t length = 1;
  while (static_cast<unsigned char>(name[pos]) >= 0x80 && pos + length < name.size() && (static_cast<unsigned char>(name[pos + length]) & 0xC0) == 0x80) {
    ++length;  // continuation bytes of a multi-byte UTF-8 character
  }
  return name.substr(pos, length);
}
}  // end namespace SciRooPlot
#endif  // SRC_VALIDATION_H_
