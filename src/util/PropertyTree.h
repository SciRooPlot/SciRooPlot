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

#ifndef SRC_UTIL_PROPERTYTREE_H_
#define SRC_UTIL_PROPERTYTREE_H_

#include <boost/property_tree/ptree.hpp>

#include <optional>
#include <string>
#include <type_traits>

#include "util/Strings.h"
#include "util/TypeTraits.h"

namespace SciRooPlot::util
{
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
std::optional<T> get_from_tree(const boost::property_tree::ptree& tree, const std::string& label)
{
  std::optional<T> var;
  read_from_tree(tree, var, label);
  return var;
}

}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_PROPERTYTREE_H_
