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

#ifndef SRC_UTIL_TYPETRAITS_H_
#define SRC_UTIL_TYPETRAITS_H_

#include <tuple>
#include <type_traits>
#include <vector>

class TH1;
class TH2;
class TH3;
class TGraph;
class TGraph2D;
class TProfile;
class TProfile2D;
class TF1;
class TF2;
class TF3;

namespace SciRooPlot::util
{
template <typename T, typename... Ts>
constexpr bool is_one_of_v()
{
  return (... || std::is_same_v<T, Ts>);
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

// introspection of the ROOT data types
template <typename T>
constexpr bool is_hist_1d()
{
  return is_one_of_v<T, TH1*, TProfile*>();
}
template <typename T>
constexpr bool is_hist_2d()
{
  return is_one_of_v<T, TH2*, TProfile2D*>();
}
template <typename T>
constexpr bool is_hist_3d()
{
  return is_one_of_v<T, TH3*>();
}
template <typename T>
constexpr bool is_hist()
{
  return is_hist_1d<T>() || is_hist_2d<T>() || is_hist_3d<T>();
}
template <typename T>
constexpr bool is_graph_1d()
{
  return is_one_of_v<T, TGraph*>();
}
template <typename T>
constexpr bool is_graph_2d()
{
  return is_one_of_v<T, TGraph2D*>();
}
template <typename T>
constexpr bool is_graph()
{
  return is_graph_1d<T>() || is_graph_2d<T>();
}
template <typename T>
constexpr bool is_func_1d()
{
  return is_one_of_v<T, TF1*>();
}
template <typename T>
constexpr bool is_func_2d()
{
  return is_one_of_v<T, TF2*>();
}
template <typename T>
constexpr bool is_func_3d()
{
  return is_one_of_v<T, TF3*>();
}
template <typename T>
constexpr bool is_func()
{
  return is_func_1d<T>() || is_func_2d<T>() || is_func_3d<T>();
}
template <typename T>
constexpr bool is_1d()
{
  return is_hist_1d<T>() || is_graph_1d<T>() || is_func_1d<T>();
}
template <typename T>
constexpr bool is_2d()
{
  return is_hist_2d<T>() || is_graph_2d<T>() || is_func_2d<T>();
}
template <typename T>
constexpr bool is_3d()
{
  return is_hist_3d<T>() || is_func_3d<T>();
}
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_TYPETRAITS_H_
