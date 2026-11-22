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

#ifndef SRC_UTIL_HEPDATA_H_
#define SRC_UTIL_HEPDATA_H_

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

namespace SciRooPlot::util
{
// an input hepdata:<record>, optionally restricted to a folder (hepdata:ins1419244:Table 1): a record of hepdata.net
bool is_hepdata_input(const std::string& input);
// a record id: letters and digits only (ins1419244, 72886); anything else after hepdata: is a file
bool is_hepdata_record_id(const std::string& recordOrFile);
// the record or file and the folder of an input hepdata:<record>[:folder] or hepdata:<file>[:folder] (file: .root, .tar.gz, .tgz)
std::pair<std::string, std::string> split_hepdata_input(const std::string& input);
// the name of a table in the local copy of a record: ':' cannot be part of a data name and is replaced by '_'
std::string hepdata_table_name(std::string table);
// the local copy of a HEPData record (ins1419244 or a record number) in <folder>/hepdata, downloaded in its latest version when missing,
// with an object per dependent variable and error source next to HEPData's own (nullopt with a warning if it cannot be made)
std::optional<std::string> hepdata_file(const std::string& recordId, const std::filesystem::path& folder);
// the local copy of a downloaded HEPData file (a ROOT file of a table or record, or the record's .tar.gz) in <folder>/hepdata/files, made
// again when the file is newer (nullopt with a warning if it cannot be made)
std::optional<std::string> hepdata_local_file(const std::string& file, const std::filesystem::path& folder);
// the local copy of a record (record id) or of a downloaded file, made when missing, in <folder>/hepdata
std::optional<std::string> hepdata_copy(const std::string& recordOrFile, const std::filesystem::path& folder);
// srp print hepdata:<record or file>[:<table>]: the data names of the copy per table, with what the record's ROOT file does not hold
// (false with an error)
bool print_hepdata(const std::string& input, const std::filesystem::path& folder);
}  // namespace SciRooPlot::util

#endif  // SRC_UTIL_HEPDATA_H_
