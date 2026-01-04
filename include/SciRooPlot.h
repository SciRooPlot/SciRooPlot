/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot --------------------------------------
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
 * The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef INCLUDE_SCIROOPLOT_H_
#define INCLUDE_SCIROOPLOT_H_

#include <iostream>
#include <map>
#include <set>
#include <variant>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <string>
#include <boost/property_tree/ptree.hpp>

namespace SciRooPlot
{

// aliases
using boost::property_tree::ptree;
using std::array;
using std::cout;
using std::endl;
using std::map;
using std::nullopt;
using std::optional;
using std::set;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::variant;
using std::vector;

const string gNameGroupSeparator = "_IN_";

}  // end namespace SciRooPlot
#endif  // INCLUDE_SCIROOPLOT_H_
