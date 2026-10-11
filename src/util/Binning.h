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

#ifndef SRC_UTIL_BINNING_H_
#define SRC_UTIL_BINNING_H_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <utility>

namespace SciRooPlot::util
{
// axis range containing all values from min to max: the upper edge of a bin is exclusive, so the range is padded by a
// fraction of the span (of the value if all values are equal); without values (min > max) the range is [0, 1)
inline std::pair<double, double> auto_axis_range(double min, double max, double margin = 0.01)
{
  if (!(min <= max)) return {0., 1.};
  const double pad = margin * ((max > min) ? (max - min) : std::max(std::abs(max), 1.));
  return {min - pad, max + pad};
}

// binning for integer values from min to max: bins of integer width centred on the integers, at most maxBins of them
inline std::tuple<int32_t, double, double> integer_binning(double min, double max, int32_t maxBins)
{
  const double nValues = std::round(max) - std::round(min) + 1.;
  const double width = std::ceil(nValues / std::max(maxBins, 1));
  const auto nBins = static_cast<int32_t>(std::ceil(nValues / width));
  const double low = std::round(min) - 0.5;
  return {nBins, low, low + nBins * width};
}
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_BINNING_H_
