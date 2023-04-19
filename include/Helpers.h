// SciRooPlot
//
// Copyright (C) 2019-2023  Mario Krüger
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

#ifndef Helpers_h
#define Helpers_h

#include "SciRooPlot.h"
#include "TSystem.h"

namespace SciRooPlot
{
string expand_path(const string& path);
vector<string> split_string(const string& argString, char delimiter, bool onlyFirst = false);
bool file_exists(const string& name);

inline bool str_contains(const string& str, const string& substr, bool reverseSearch = false)
{
  if (reverseSearch) {
    return (str.rfind(substr) != string::npos);
  } else {
    return (str.find(substr) != string::npos);
  }
}

template <typename T>
void set_if(const optional<T>& origin, optional<T>& target)
{
  if (origin) {
    target = origin;
  }
}

template <typename T>
struct is_vector : public std::false_type {
};
template <typename T, typename A>
struct is_vector<vector<T, A>> : public std::true_type {
};

template <typename>
struct is_tuple : std::false_type {
};
template <typename... T>
struct is_tuple<tuple<T...>> : std::true_type {
};

template <typename... Ts>
string tuple_to_string(const tuple<Ts...>& items)
{
  string itemString;
  std::apply([&](auto&&... item) { ((itemString += std::to_string(item) + ","), ...); }, items);
  itemString.pop_back();
  return itemString;
}

template <typename T>
string vector_to_string(vector<T> items)
{
  string itemString;
  for (auto& item : items) {
    if constexpr (is_tuple<T>::value) {
      itemString += tuple_to_string(item);
      if (&item != &items.back()) itemString += ";";
    } else {
      itemString += std::to_string(item);
      if (&item != &items.back()) itemString += ",";
    }
  }
  return itemString;
}

template <typename T>
T string_to_type(const string& str)
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
tuple<Ts...> string_to_tuple(string itemString)
{
  // split string
  string curItemStr;
  std::istringstream stream(itemString);
  string numbers[sizeof...(Ts)];
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
vector<T> string_to_vector(string itemString)
{
  // safety in case user put some blank spaces between numbers
  std::remove_if(itemString.begin(), itemString.end(), ::isspace);
  vector<T> items;

  string curItemStr;
  std::istringstream stream(itemString);
  if constexpr (is_tuple<T>::value) {
    while (std::getline(stream, curItemStr, ';')) {
      // ugly hack...
      if constexpr (std::is_same_v<T, tuple<uint8_t, double_t, double_t>>) {
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
void put_in_tree(ptree& tree, const optional<T>& var, const string& label)
{
  if constexpr (is_vector<T>{}) // vectors are stored as comma separated strings
  {
    if (var) tree.put(label, vector_to_string(*var));
  } else if constexpr (std::is_enum<T>::value) // in case using enum types of the framework
  {
    if (var) tree.put(label, static_cast<typename std::underlying_type<T>::type>(*var));
  } else {
    if (var) tree.put(label, *var);
  }
}

template <typename T>
void read_from_tree(const ptree& tree, optional<T>& var, const string& label)
{
  if constexpr (is_vector<T>{}) // vectors are stored as comma separated strings
  {
    if (auto tmp = tree.get_optional<string>(label))
      var = string_to_vector<typename T::value_type>(*tmp);
  } else if constexpr (std::is_enum<T>::value) // in case using enum types of the framework
  {
    if (auto tmp = tree.get_optional<typename std::underlying_type<T>::type>(label))
      var = static_cast<T>(*tmp);
  } else {
    if (auto tmp = tree.get_optional<T>(label)) var = *tmp;
  }
}

template <typename T>
optional<T> pick(int32_t i, const optional<vector<T>>& vec)
{
  if (!vec || vec->empty()) return nullopt;
  return optional((*vec)[(i - 1) % vec->size()]);
}

template <typename T, typename... Ts>
const optional<T>& get_first(const optional<T>& property, const Ts&... properties)
{
  for (const auto ptr : {&property, &properties...}) {
    if (*ptr) return *ptr;
  }
  // if all properties are nullopt, simply return reference to the first
  return property;
}

template <typename T, typename... Ts>
T get_first_or(const T& fallback, const optional<T>& property, const Ts&... properties)
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

} // end namespace SciRooPlot
#endif /* Helpers_h */
