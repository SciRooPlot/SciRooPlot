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

#ifndef SRC_UTIL_STRINGS_H_
#define SRC_UTIL_STRINGS_H_

#include "SciRooPlot/Logging.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "util/TypeTraits.h"

namespace SciRooPlot::util
{
std::vector<std::string> split_string(const std::string& argString, char delimiter, bool onlyFirst = false);

inline bool str_contains(const std::string& str, const std::string& substr)
{
  return (str.find(substr) != std::string::npos);
}

inline bool str_contains(const std::string& str, const std::vector<std::string>& substrs)
{
  return std::any_of(substrs.begin(), substrs.end(),
                     [&](const std::string& substr) { return str_contains(str, substr); });
}

inline bool str_starts_with(const std::string& str, const std::string& prefix)
{
  return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool str_starts_with(const std::string& str, const std::vector<std::string>& prefixes)
{
  return std::any_of(prefixes.begin(), prefixes.end(),
                     [&](const std::string& prefix) { return str_starts_with(str, prefix); });
}

inline bool str_ends_with(const std::string& str, const std::string& suffix)
{
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline bool str_ends_with(const std::string& str, const std::vector<std::string>& suffixes)
{
  return std::any_of(suffixes.begin(), suffixes.end(),
                     [&](const std::string& suffix) { return str_ends_with(str, suffix); });
}

template <typename T>
inline std::string number_to_string(const T& value)
{
  if constexpr (std::is_floating_point_v<T>) {
    return fmt::format("{}", value);
  } else {
    return std::to_string(value);
  }
}

template <typename... Ts>
std::string tuple_to_string(const std::tuple<Ts...>& items)
{
  std::string itemString;
  std::apply([&](auto&&... item) { ((itemString += number_to_string(item) + ","), ...); }, items);
  itemString.pop_back();
  return itemString;
}

template <typename T>
std::string vector_to_string(std::vector<T> items)
{
  std::string itemString;
  for (auto& item : items) {
    if constexpr (is_tuple<T>::value) {
      itemString += tuple_to_string(item);
      if (&item != &items.back()) itemString += ";";
    } else {
      if constexpr (std::is_same_v<std::string, T>) {
        itemString += item;
        if (&item != &items.back()) itemString += "$";
      } else {
        itemString += number_to_string(item);
        if (&item != &items.back()) itemString += ",";
      }
    }
  }
  return itemString;
}

template <typename T>
T string_to_type(const std::string& str)
{
  if constexpr (std::is_floating_point_v<T>) {
    std::istringstream stream(str);
    stream.imbue(std::locale::classic());
    T value{};
    if (!(stream >> value)) {
      throw std::invalid_argument("Cannot parse '" + str + "' as a number.");
    }
    return value;
  } else {
    static_assert(sizeof(T) < sizeof(int64_t), "string_to_type: integer type too wide for this parser");
    const int64_t parsed = std::stoll(str);
    if (parsed < static_cast<int64_t>(std::numeric_limits<T>::min()) ||
        parsed > static_cast<int64_t>(std::numeric_limits<T>::max())) {
      throw std::out_of_range("Value " + str + " is out of range for this field.");
    }
    return static_cast<T>(parsed);
  }
}

template <typename... Ts>
std::tuple<Ts...> string_to_tuple(std::string itemString)
{
  // split string
  std::string curItemStr;
  std::istringstream stream(itemString);
  std::string numbers[sizeof...(Ts)];
  uint8_t i = 0;
  while (i < sizeof...(Ts) && std::getline(stream, curItemStr, ',')) {
    numbers[i] = curItemStr;
    ++i;
  }
  if (i != sizeof...(Ts)) {
    logger::throw_invalid_argument("Expected {} comma-separated values but found {} in '{}'.", sizeof...(Ts), i, itemString);
  }

  // ugly hack...
  if constexpr (sizeof...(Ts) == 3) {
    return {string_to_type<uint8_t>(numbers[0]), string_to_type<double_t>(numbers[1]), string_to_type<double_t>(numbers[2])};
  } else {
    return {string_to_type<float_t>(numbers[0]), string_to_type<float_t>(numbers[1]), string_to_type<float_t>(numbers[2]), string_to_type<float_t>(numbers[3])};
  }
}

template <typename T>
std::vector<T> string_to_vector(std::string itemString)
{
  // safety in case user put some blank spaces between numbers
  if constexpr (!std::is_same_v<T, std::string>) {
    itemString.erase(
      std::remove_if(itemString.begin(), itemString.end(),
                     [](unsigned char c) { return std::isspace(c); }),
      itemString.end());
  }
  std::vector<T> items;

  std::string curItemStr;
  std::istringstream stream(itemString);
  if constexpr (std::is_same_v<T, std::string>) {
    while (std::getline(stream, curItemStr, '$')) {
      items.push_back(curItemStr);
    }
  } else if constexpr (is_tuple<T>::value) {
    while (std::getline(stream, curItemStr, ';')) {
      // ugly hack...
      if constexpr (std::is_same_v<T, std::tuple<uint8_t, double_t, double_t>>) {
        items.push_back(string_to_tuple<uint8_t, double_t, double_t>(curItemStr));
      } else {
        items.push_back(string_to_tuple<float_t, float_t, float_t, float_t>(curItemStr));
      }
    }
  } else {
    while (std::getline(stream, curItemStr, ',')) {
      items.push_back(string_to_type<T>(curItemStr));
    }
  }
  return items;
}

}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_STRINGS_H_
