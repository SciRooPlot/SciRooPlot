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

#ifndef INCLUDE_SCIROOPLOT_HELPERS_H_
#define INCLUDE_SCIROOPLOT_HELPERS_H_

#include <TSystem.h>

#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace SciRooPlot
{
std::string expand_path(const std::string& path);
std::vector<std::string> split_string(const std::string& argString, char delimiter, bool onlyFirst = false);
bool file_exists(const std::string& name);

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
void set_if(const std::optional<T>& origin, std::optional<T>& target)
{
  if (origin) {
    target = origin;
  }
}

template <typename T>
struct is_vector : public std::false_type {
};
template <typename T, typename A>
struct is_vector<std::vector<T, A>> : public std::true_type {
};

template <typename>
struct is_tuple : std::false_type {
};
template <typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type {
};

template <typename... Ts>
std::string tuple_to_string(const std::tuple<Ts...>& items)
{
  std::string itemString;
  std::apply([&](auto&&... item) { ((itemString += std::to_string(item) + ","), ...); }, items);
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
        itemString += std::to_string(item);
        if (&item != &items.back()) itemString += ",";
      }
    }
  }
  return itemString;
}

template <typename T>
T string_to_type(const std::string& str)
{
  if constexpr (std::is_same_v<double_t, T>) {
    return std::stod(str);
  } else if constexpr (std::is_same_v<float_t, T>) {
    return std::stof(str);
  } else {
    return std::stoi(str);
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
  while (std::getline(stream, curItemStr, ',')) {
    numbers[i] = curItemStr;
    ++i;
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

template <typename T>
void put_in_tree(boost::property_tree::ptree& tree, const std::optional<T>& var, const std::string& label)
{
  if constexpr (is_vector<T>{})  // vectors are stored as comma separated strings
  {
    if (var) tree.put(label, vector_to_string(*var));
  } else if constexpr (std::is_enum<T>::value) {  // in case using enum types of the framework
    if (var) tree.put(label, static_cast<typename std::underlying_type<T>::type>(*var));
  } else {
    if (var) tree.put(label, *var);
  }
}

template <typename T>
void read_from_tree(const boost::property_tree::ptree& tree, std::optional<T>& var, const std::string& label)
{
  if constexpr (is_vector<T>{})  // vectors are stored as comma separated strings
  {
    if (auto tmp = tree.get_optional<std::string>(label))
      var = string_to_vector<typename T::value_type>(*tmp);
  } else if constexpr (std::is_enum<T>::value) {  // in case using enum types of the framework
    if (auto tmp = tree.get_optional<typename std::underlying_type<T>::type>(label))
      var = static_cast<T>(*tmp);
  } else {
    if (auto tmp = tree.get_optional<T>(label)) var = *tmp;
  }
}

template <typename T>
std::optional<T> pick(uint16_t i, const std::optional<std::vector<T>>& vec)
{
  if (!vec || vec->empty()) return std::nullopt;
  return std::optional((*vec)[i % vec->size()]);
}

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

template <typename T, typename... Ts>
constexpr bool is_one_of_v()
{
  return (... || std::is_same_v<T, Ts>);
}

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

template <typename F>
class ScopeGuard
{
 public:
  explicit ScopeGuard(F onExit) : mOnExit(std::move(onExit)) {}
  ~ScopeGuard()
  {
    if (mActive) mOnExit();
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  ScopeGuard(ScopeGuard&& other) noexcept : mOnExit(std::move(other.mOnExit)), mActive(other.mActive)
  {
    other.mActive = false;
  }

  void Dismiss() { mActive = false; }

 private:
  F mOnExit;
  bool mActive{true};
};

template <typename F>
ScopeGuard<std::decay_t<F>> make_scope_guard(F&& onExit)
{
  return ScopeGuard<std::decay_t<F>>(std::forward<F>(onExit));
}

}  // end namespace SciRooPlot
#endif  // INCLUDE_SCIROOPLOT_HELPERS_H_
