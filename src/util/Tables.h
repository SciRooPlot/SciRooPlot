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

#ifndef SRC_UTIL_TABLES_H_
#define SRC_UTIL_TABLES_H_

#include <ROOT/RCsvDS.hxx>

#include <optional>
#include <string>
#include <vector>

namespace SciRooPlot::util
{
// the endings of table files (text files of columns) among the inputs of a data source
inline const std::vector<std::string> kTableFileEndings = {".csv", ".dat", ".txt", ".tsv", ".tab"};
// how ROOT's csv reader reads a table file: the delimiter is the candidate that splits all lines (incl. header) into the same number of
// fields; lines starting with '#' are comments if the first lines contain any; columns of numbers ROOT would truncate or read as text are
// floating point. Files with a varying number of fields are refused, since ROOT's csv reader cannot handle them (nullopt with an error).
std::optional<ROOT::RDF::RCsvDS::ROptions> table_options(const std::string& inputFileName);
}  // namespace SciRooPlot::util

#endif  // SRC_UTIL_TABLES_H_
