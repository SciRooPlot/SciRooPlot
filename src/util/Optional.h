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

#ifndef SRC_UTIL_OPTIONAL_H_
#define SRC_UTIL_OPTIONAL_H_

#include <cstdint>
#include <optional>
#include <vector>

namespace SciRooPlot::util
{
template <typename T>
void set_if(const std::optional<T>& origin, std::optional<T>& target)
{
  if (origin) {
    target = origin;
  }
}

// element i of the vector (cyclic)
template <typename T>
std::optional<T> pick(uint16_t i, const std::optional<std::vector<T>>& vec)
{
  if (!vec || vec->empty()) return std::nullopt;
  return std::optional((*vec)[i % vec->size()]);
}

// first of the properties that is set
template <typename T, typename... Ts>
std::optional<T> get_first(const std::optional<T>& property, const Ts&... properties)
{
  for (const auto ptr : {&property, &properties...}) {
    if (*ptr) return *ptr;
  }
  return std::nullopt;
}

template <typename T, typename... Ts>
T get_first_or(const T& fallback, const std::optional<T>& property, const Ts&... properties)
{
  if (const auto& match = get_first(property, properties...)) {
    return *match;
  } else {
    return fallback;
  }
}
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_OPTIONAL_H_
