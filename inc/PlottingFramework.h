// PlottingFramework
//
// Copyright (C) 2019-2021  Mario Krüger
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

#ifndef PlottingFramework_h
#define PlottingFramework_h

#include <iostream>
#include <map>
#include <set>
#include <variant>
#include <cmath>
#include <cstdint>
#include <boost/property_tree/ptree.hpp>
#include <unordered_map>

namespace PlottingFramework
{

// aliases
using boost::property_tree::ptree;
using std::array;
using std::cout;
using std::endl;
using std::map;
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

} // end namespace PlottingFramework
#endif /* PlottingFramework_h */
