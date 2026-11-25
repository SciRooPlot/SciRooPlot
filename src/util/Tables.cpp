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

#include "util/Tables.h"

#include "SciRooPlot/Logger.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "util/Strings.h"

using std::nullopt;
using std::optional;
using std::string;
using std::vector;

namespace SciRooPlot::util
{
//**************************************************************************************************
/**
 * How ROOT's csv reader is to read a table file, checked as the data sources read it (see the header).
 */
//**************************************************************************************************
optional<ROOT::RDF::RCsvDS::ROptions> table_options(const string& inputFileName)
{
  std::error_code existsError;
  if (!std::filesystem::exists(inputFileName, existsError)) {
    logger::error("File {} does not exist{}.", inputFileName, existsError ? " or cannot be read (" + existsError.message() + ")" : "");
    return nullopt;
  }
  auto trim = [](const string& line) {
    const auto begin = line.find_first_not_of(" \t\r");
    if (begin == string::npos) return string{};
    return line.substr(begin, line.find_last_not_of(" \t\r") - begin + 1);
  };
  // split a line into fields (delimiters within double quotes do not count)
  auto split = [](const string& line, char delimiter) {
    vector<string> fields(1);
    bool quoted = false;
    for (char c : line) {
      if (c == '"') quoted = !quoted;
      if (c == delimiter && !quoted) {
        fields.emplace_back();
      } else {
        fields.back() += c;
      }
    }
    return fields;
  };

  // first data lines (incl. header) with their line numbers, skipping blank lines and comments
  std::ifstream file(inputFileName);
  vector<std::pair<size_t, string>> lines;
  string line;
  size_t lineNumber = 0;
  size_t commentAmongRows = 0;  // the first comment line after the header: ROOT then needs '#' as its comment character
  size_t hashInLine = 0;        // the first line of the header or the rows with a '#' within it (a value such as a label in ROOT's latex)
  bool hasBom = false;          // Excel's "CSV UTF-8" starts the file with a byte order mark, which ROOT makes part of the first column name
  while (lines.size() < 100 && std::getline(file, line)) {
    ++lineNumber;
    if (lineNumber == 1 && str_starts_with(line, "\xEF\xBB\xBF")) {
      hasBom = true;
      line.erase(0, 3);
    }
    line = trim(line);
    if (line.empty()) continue;
    if (line[0] == '#') {
      if (!lines.empty() && !commentAmongRows) commentAmongRows = lineNumber;
      continue;
    }
    if (!hashInLine && line.find('#') != string::npos) hashInLine = lineNumber;
    lines.emplace_back(lineNumber, line);
  }
  if (lines.empty()) {
    logger::error("Table {} does not contain any data.", inputFileName);
    return nullopt;
  }

  // the delimiter must give the same number of fields in all lines, the one giving most fields wins
  char delimiter = ',';
  size_t nFields = 1;
  for (char candidate : {',', ';', '\t', '|', ' '}) {
    const size_t nHeaderFields = split(lines[0].second, candidate).size();
    if (nHeaderFields <= nFields) continue;
    bool consistent = std::all_of(lines.begin(), lines.end(), [&](const auto& entry) { return split(entry.second, candidate).size() == nHeaderFields; });
    if (consistent) {
      delimiter = candidate;
      nFields = nHeaderFields;
    }
  }
  auto delimiterName = [](char c) -> string {
    if (c == '\t') return "tab";
    if (c == ' ') return "space";
    return string("'") + c + "'";
  };
  auto describeMismatch = [&](char c, size_t nExpected, size_t badLine, size_t nFound) {
    return fmt::format("the header has {} fields separated by {}, but line {} has {}", nExpected, delimiterName(c), badLine, nFound);
  };
  if (nFields == 1) {
    // no consistent delimiter: report what goes wrong with the delimiter that is most frequent in the header
    char guess = ',';
    size_t maxCount = 0;
    for (char candidate : {',', ';', '\t', '|', ' '}) {
      size_t count = split(lines[0].second, candidate).size() - 1;
      if (count > maxCount) {
        maxCount = count;
        guess = candidate;
      }
    }
    if (maxCount) {
      auto nWords = [](const string& text) {
        std::istringstream words(text);
        return std::distance(std::istream_iterator<string>(words), std::istream_iterator<string>());
      };
      for (const auto& [number, content] : lines) {
        const size_t n = split(content, guess).size();
        if (n == maxCount + 1) continue;
        string hint;
        // as many words as the header, but other gaps between them (the header may be aligned too)
        if ((guess == ' ' || guess == '\t') && nWords(content) == nWords(lines[0].second)) {
          hint = " (columns seem to be aligned with several spaces or tabs; use exactly one delimiter between the values)";
        }
        logger::error("Cannot read table {}: {}{}.", inputFileName, describeMismatch(guess, maxCount + 1, number, n), hint);
        return nullopt;
      }
    }
  }

  // ROOT takes the type of a column from its first value: an integer makes it an integer column, so later decimals are truncated
  // (0.5 is read as 0) and inf or nan cannot be read, while numbers it does not recognise as such (1e-3, inf, nan) make it a text column.
  // Such columns are declared as floating point, from all values of the table (checked as long as the type of the column is open).
  enum class column_kind_t { open,
                             integer,
                             text_like,
                             done };
  const auto columnNames = split(lines[0].second, delimiter);
  vector<column_kind_t> columnKinds(columnNames.size(), column_kind_t::open);
  vector<bool> forceDouble(columnNames.size(), false);
  vector<bool> hasEmptyCell(columnNames.size(), false);  // an integer column with an empty cell is read as floating point: ROOT stores 0 for it otherwise
  auto isInteger = [](const string& value) {
    const size_t start = (value[0] == '+' || value[0] == '-') ? 1 : 0;
    return value.size() > start && value.find_first_not_of("0123456789", start) == string::npos;
  };
  auto isNumber = [](const string& value) {
    char* end{};
    std::strtod(value.data(), &end);
    return end == value.data() + value.size();
  };
  // a number followed by a comment ("2 # first") where ROOT takes the column type from it (the first value) or where it ends a column of
  // text-like numbers makes the column text: '#' starts a comment only on a line of its own (it is kept within values)
  struct comment_after_value_t {
    size_t line = 0;
    size_t column = 0;
    string value;
  } commentAfterValue;
  auto noteCommentAfterValue = [&](const string& value, size_t column, size_t number) {
    if (commentAfterValue.line) return;
    const size_t hash = value.find('#');
    if (hash == string::npos || hash == 0 || !std::isspace(static_cast<unsigned char>(value[hash - 1]))) return;
    const string before = trim(value.substr(0, hash));
    if (!before.empty() && isNumber(before)) commentAfterValue = {number, column, value};
  };
  auto checkTypes = [&](const vector<string>& fields, size_t number) {
    for (size_t column = 0; column < std::min(fields.size(), columnKinds.size()); ++column) {
      column_kind_t& kind = columnKinds[column];
      if (kind == column_kind_t::done) continue;
      const string value = trim(fields[column]);
      if (value.empty()) {  // the type comes from the values that are there
        hasEmptyCell[column] = true;
        continue;
      }
      if (kind == column_kind_t::open) {
        // as ROOT: integers and numbers with a decimal point are numeric, everything else text
        static const std::regex decimal(R"([+-]?(\d+\.\d*|\d*\.\d+)([eE][+-]?\d+)?)");
        if (isInteger(value)) {
          kind = column_kind_t::integer;
        } else if (std::regex_match(value, decimal) || !isNumber(value)) {
          if (!isNumber(value)) noteCommentAfterValue(value, column, number);
          kind = column_kind_t::done;
        } else {
          kind = column_kind_t::text_like;
          forceDouble[column] = true;
        }
      } else if (kind == column_kind_t::integer) {
        if (!isInteger(value)) {
          forceDouble[column] = isNumber(value);  // any other value fails with an integer as well as with a floating point type
          kind = column_kind_t::done;
        }
      } else if (!isNumber(value)) {  // text-like numbers: a column with real text stays text
        noteCommentAfterValue(value, column, number);
        forceDouble[column] = false;
        kind = column_kind_t::done;
      }
    }
  };
  for (size_t i = 1; i < lines.size(); ++i) {
    checkTypes(split(lines[i].second, delimiter), lines[i].first);
  }
  // check the remaining lines as well, ROOT's csv reader does not tolerate a varying number of fields
  while (std::getline(file, line)) {
    ++lineNumber;
    line = trim(line);
    if (line.empty()) continue;
    if (line[0] == '#') {  // a comment also after the lines above (ROOT then needs to know)
      if (!commentAmongRows) commentAmongRows = lineNumber;
      continue;
    }
    if (!hashInLine && line.find('#') != string::npos) hashInLine = lineNumber;
    const auto fields = split(line, delimiter);
    if (fields.size() != nFields) {
      logger::error("Cannot read table {}: {}.", inputFileName, describeMismatch(delimiter, nFields, lineNumber, fields.size()));
      return nullopt;
    }
    checkTypes(fields, lineNumber);
  }
  if (commentAfterValue.line && columnKinds[commentAfterValue.column] == column_kind_t::done && !forceDouble[commentAfterValue.column]) {
    logger::warning("Table {}: line {} has a comment after a number ('{}'): column {} is read as text, since '#' starts a comment only on a line of its own.",
                    inputFileName, commentAfterValue.line, commentAfterValue.value, trim(columnNames[commentAfterValue.column]));
  }
  for (size_t column = 0; column < columnKinds.size(); ++column) {
    if (columnKinds[column] == column_kind_t::integer && hasEmptyCell[column]) forceDouble[column] = true;  // the empty cells become NaN
  }

  // ROOT does not trim the fields: names and values with surrounding spaces end up as such (values as text)
  if (delimiter != ' ' && delimiter != '\t') {
    for (const auto& [number, content] : {lines[0], lines[std::min<size_t>(1, lines.size() - 1)]}) {
      const auto fields = split(content, delimiter);
      if (std::any_of(fields.begin(), fields.end(),
                      [&](const string& field) { return field != trim(field); })) {
        logger::warning("Table {} has spaces around its delimiters (line {}), which become part of the column names and values (read as text); remove them.", inputFileName, number);
        break;
      }
    }
  }

  // numbers with decimal commas cannot be read by ROOT (they end up as text)
  if (delimiter != ',' && lines.size() > 1) {
    static const std::regex decimalComma(R"(\s*[+-]?\d+,\d+\s*)");
    for (const auto& field : split(lines[1].second, delimiter)) {
      if (std::regex_match(field, decimalComma)) {
        logger::warning("Table {} seems to use decimal commas (e.g. '{}'), which are not read as numbers; use decimal points instead.", inputFileName, trim(field));
        break;
      }
    }
  }

  // ROOT cuts every line at its first comment character: comment lines before the header are skipped by number instead, which keeps a '#'
  // within a value; with comment lines among the rows that cannot be read
  if (commentAmongRows && hashInLine) {
    logger::error("Cannot read table {}: line {} contains a '#', which starts a comment in a table with comment lines among its rows (line {}); remove the comment lines or the '#'.",
                  inputFileName, hashInLine, commentAmongRows);
    return nullopt;
  }

  ROOT::RDF::RCsvDS::ROptions options;
  auto columnName = [&](const string& field) {
    string name = trim(field);
    if (name.size() > 1 && name.front() == '"' && name.back() == '"') name = name.substr(1, name.size() - 2);
    return name;
  };
  for (size_t column = 0; column < columnNames.size(); ++column) {
    const string name = columnName(columnNames[column]);
    if (forceDouble[column] && !name.empty()) options.fColumnTypes[name] = 'D';
  }
  options.fSkipFirstNLines = lines[0].first - 1;  // the blank and comment lines before the header (also one with the byte order mark)
  if (hasBom && lines[0].first == 1) {
    for (const auto& field : columnNames) {
      options.fColumnNames.push_back(columnName(field));
    }
  }
  options.fDelimiter = delimiter;
  options.fLeftTrim = true;  // as for the checks above: only the line as a whole is trimmed
  options.fRightTrim = true;
  if (commentAmongRows) options.fComment = '#';
  options.fLinesChunkSize = 50000;
  return options;
}
}  // namespace SciRooPlot::util
