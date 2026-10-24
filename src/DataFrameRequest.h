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

#ifndef SRC_DATAFRAMEREQUEST_H_
#define SRC_DATAFRAMEREQUEST_H_

#include "SciRooPlot/Logger.h"
#include "SciRooPlot/Plot.h"

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RResultHandle.hxx>
#include <TGraph.h>
#include <TH1.h>

#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "util/Binning.h"

namespace SciRooPlot
{
//**************************************************************************************************
/**
 * One data request (projection, profile, scatter plot) of a tree or table, booked on an RDataFrame.
 * All requests of one tree or table are booked on the same data frame so the data is read only once.
 * Template since the type of the request info is only accessible to PlotManager.
 */
//**************************************************************************************************
template <typename DataInfo>
class DataFrameRequest
{
 public:
  DataFrameRequest(const DataInfo* info, std::string context, std::string objName) : mInfo(info), mContext(std::move(context)), mObjName(std::move(objName)) {}

  bool Prepare(ROOT::RDF::RNode node, const std::vector<std::string>& requiredColumns = {});
  bool NeedsAutoRange() const;
  bool Book();

  const DataInfo* GetInfo() const { return mInfo; }
  const std::vector<ROOT::RDF::RResultHandle>& GetHandles() const { return mHandles; }
  bool IsBooked() const { return static_cast<bool>(mGetResult); }
  TObject* GetResult() const { return mGetResult(); }  // the caller takes ownership
  std::optional<ULong64_t> GetEntriesPreFilter() { return (mEntriesPreFilter) ? std::optional<ULong64_t>(**mEntriesPreFilter) : std::nullopt; }
  ULong64_t GetEntriesPostFilter() { return **mEntriesPostFilter; }

 private:
  using data_dim_t = Plot::Pad::Data::data_dim_t;

  static std::string AvailableColumns(ROOT::RDF::RNode& node);
  template <typename T>
  void SetResult(ROOT::RDF::RResultPtr<T> result);

  // range of the finite values of an axis, and the number of the others (inf, nan), for the automatic binning
  struct auto_range_t {
    double_t min{std::numeric_limits<double_t>::infinity()};
    double_t max{-std::numeric_limits<double_t>::infinity()};
    ULong64_t nNonFinite{};
  };

  const DataInfo* mInfo{};
  std::string mContext;  // e.g. 'tree source:name', used in messages
  std::string mObjName;  // name of the resulting object
  std::optional<ROOT::RDF::RNode> mNode;
  std::vector<data_dim_t> mDims;
  std::string mHistTitle;
  bool mIsProfile{};
  bool mIsScatter{};
  bool mHasWeights{};
  bool mIsTyped{true};  // all values are stored as double: the actions are booked with their types (not compiled by the interpreter)
  std::optional<ROOT::RDF::RResultPtr<ULong64_t>> mEntriesPreFilter;
  std::optional<ROOT::RDF::RResultPtr<ULong64_t>> mEntriesPostFilter;
  std::vector<std::function<auto_range_t()>> mAutoRanges;  // per axis: gives the result of the range pass (empty if the axis has its own range)
  std::vector<bool> mIsIntegerAxis;                        // the values of the axis are integers
  std::function<TObject*()> mGetResult;
  std::vector<ROOT::RDF::RResultHandle> mHandles;
};

// list of the columns available in a data frame, to be appended to messages about invalid expressions
template <typename DataInfo>
std::string DataFrameRequest<DataInfo>::AvailableColumns(ROOT::RDF::RNode& node)
{
  constexpr size_t kMaxColumns = 30;
  std::vector<std::string> columns;
  for (const auto& column : node.GetColumnNames()) {
    if (column.rfind("SRP_", 0) != 0) columns.push_back(column);
  }
  std::sort(columns.begin(), columns.end());
  std::string list;
  for (size_t i = 0; i < std::min(columns.size(), kMaxColumns); ++i) {
    list += ((i) ? ", " : "") + columns[i];
  }
  if (columns.size() > kMaxColumns) list += fmt::format(", ... ({} more)", columns.size() - kMaxColumns);
  return " (available columns: " + list + ")";
}

// book everything that is needed before the final result can be booked (defines, filters, entry counts, auto-detection of axis ranges)
template <typename DataInfo>
bool DataFrameRequest<DataInfo>::Prepare(ROOT::RDF::RNode node, const std::vector<std::string>& requiredColumns)
{
  const auto& dataInfo = *mInfo;
  if (dataInfo.isProfileNoScatter) {
    mIsProfile = *dataInfo.isProfileNoScatter;
    mIsScatter = !mIsProfile;
  }
  bool isProjection = (!mIsProfile && !mIsScatter);

  mDims = dataInfo.dataDims;  // copy since it will be modified
  if (mIsProfile && mDims.size() > 3) {
    logger::error("Too many dimensions specified for profile of {} ({}).", mContext, dataInfo.GetDescription());
    return false;
  }
  size_t axisID = 1;
  for (auto& dataDim : mDims) {
    if (isProjection || (mIsProfile && axisID < mDims.size())) {
      // sanity check for binned axes
      if ((dataDim.nBins && dataDim.edges.size() != 2) || (!dataDim.nBins && dataDim.edges.size() <= 1)) {
        logger::error("Ill defined binning for {} in {} ({}).", dataDim.var, mContext, dataInfo.GetDescription());
        return false;
      }
      if (!std::is_sorted(dataDim.edges.begin(), dataDim.edges.end())) {
        if (!(dataDim.edges.size() == 2 && !dataDim.edges[0] && !dataDim.edges[1])) {
          logger::error("Ill defined binning for {} in {} ({}).", dataDim.var, mContext, dataInfo.GetDescription());
          return false;
        }
      }
    }
    ++axisID;
  }

  // ROOT takes any code containing "return" as the body of a function instead of an expression to be returned: an expression
  // like "[](double a) { double b = 2 * a; return b; }(pt)" would get the type void, which breaks the event loop of all requests
  // and the interpreter. Such an expression is given an explicit return statement; code with a statement outside of all braces
  // (a ';' followed by more code or a statement keyword, e.g. "double b = 2 * pt; return b;") stays a function body.
  auto toRootCode = [](const std::string& code) {
    if (code.find("return") == std::string::npos) return code;
    auto isWord = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    int32_t depth{};
    char quote{};  // inside a string or character literal
    bool hadSemicolon{};
    for (size_t i = 0; i < code.size(); ++i) {
      const char c = code[i];
      if (quote) {
        if (c == '\\') {
          ++i;
        } else if (c == quote) {
          quote = 0;
        }
        continue;
      }
      if (c == '"' || (c == '\'' && (i == 0 || !std::isdigit(static_cast<unsigned char>(code[i - 1]))))) {
        quote = c;
      } else if (c == '{') {
        ++depth;
      } else if (c == '}') {
        --depth;
      } else if (!depth && c == ';') {
        hadSemicolon = true;
      } else if (!depth && hadSemicolon && !std::isspace(static_cast<unsigned char>(c))) {
        return code;
      } else if (!depth && isWord(c) && (i == 0 || !isWord(code[i - 1]))) {
        size_t end = i;
        while (end < code.size() && isWord(code[end]))
          ++end;
        const std::string word = code.substr(i, end - i);
        if (word == "return" || word == "if" || word == "for" || word == "while" || word == "do" || word == "switch") return code;
        i = end - 1;
      }
    }
    return "return " + code + ";";
  };

  if (dataInfo.definitions.keys && dataInfo.definitions.values) {
    for (size_t i = 0; i < dataInfo.definitions.keys->size(); ++i) {
      const auto& column = dataInfo.definitions.keys->at(i);
      const auto expression = toRootCode(dataInfo.definitions.values->at(i));
      // an existing column is replaced (e.g. to correct or recalibrate it)
      node = (node.HasColumn(column)) ? node.Redefine(column, expression) : node.Define(column, expression);
    }
  }
  if (dataInfo.entries.max) {
    if (dataInfo.entries.min) {
      node = node.Range(*dataInfo.entries.min, *dataInfo.entries.max);
    } else {
      node = node.Range(*dataInfo.entries.max);
    }
  }
  // rows without values in these columns (no matching row in a tree joined by key) are skipped like filtered rows, after the entry range
  for (const auto& column : requiredColumns) {
    node = node.FilterAvailable(column);
  }
  if (dataInfo.filters) {
    mEntriesPreFilter = node.Count();
    for (const auto& filter : *dataInfo.filters) {
      try {
        node = node.Filter(toRootCode(filter));
      } catch (const std::runtime_error&) {
        logger::error("Illegal filter expression {} for {}{}.", filter, mContext, AvailableColumns(node));
        return false;
      }
    }
  }
  mEntriesPostFilter = node.Count();

  axisID = 1;
  mAutoRanges.resize(mDims.size());
  mIsIntegerAxis.resize(mDims.size());
  auto isIntegerType = [](std::string type) {
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
    if (type.find("float") != std::string::npos || type.find("double") != std::string::npos) return false;
    return type.find("int") != std::string::npos || type.find("long") != std::string::npos || type.find("short") != std::string::npos || type.find("char") != std::string::npos || type.find("bool") != std::string::npos;
  };
  // a column holding the value of an expression: a scalar is stored as double, so that the actions below can be booked with their types known
  // (ROOT compiles an action booked by column names with its interpreter, which takes about half a second per combination of
  // action and value types; an action with its types given is compiled into the library). The type of the expression itself is read
  // from a column that nothing evaluates (an unused column costs no time in the event loop). Collections keep their type (see below).
  auto defineValue = [&](const std::string& colName, const std::string& expression, std::string& columnType) {
    // a plain column of a basic type is converted with compiled code, so that such requests do not involve the interpreter at all
    // (its first use in a process costs about half a second, every further compilation a tenth)
    const bool isIdentifier = !expression.empty() && !std::isdigit(static_cast<unsigned char>(expression[0])) &&
                              std::all_of(expression.begin(), expression.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; });
    if (isIdentifier && node.HasColumn(expression)) {
      columnType = node.GetColumnType(expression);
      auto defineTyped = [&](auto value) {
        using value_t = decltype(value);
        node = node.Define(colName, [](value_t v) { return static_cast<double>(v); }, {expression});
        return true;
      };
      // ROOT names the types of branches like Float_t, those of defined columns like float
      if (columnType == "Float_t" || columnType == "float") return defineTyped(float{});
      if (columnType == "Double_t" || columnType == "double") return defineTyped(double{});
      if (columnType == "Int_t" || columnType == "int") return defineTyped(int{});
      if (columnType == "UInt_t" || columnType == "unsigned int") return defineTyped(unsigned{});
      if (columnType == "Bool_t" || columnType == "bool") return defineTyped(bool{});
      if (columnType == "Short_t" || columnType == "short") return defineTyped(Short_t{});
      if (columnType == "UShort_t" || columnType == "unsigned short") return defineTyped(UShort_t{});
      if (columnType == "Char_t" || columnType == "char") return defineTyped(char{});
      if (columnType == "UChar_t" || columnType == "unsigned char") return defineTyped(static_cast<unsigned char>(0));
      if (columnType == "Long64_t" || columnType == "long long") return defineTyped(Long64_t{});
      if (columnType == "ULong64_t" || columnType == "unsigned long long") return defineTyped(ULong64_t{});
      if (columnType == "Long_t" || columnType == "long") return defineTyped(Long_t{});
      if (columnType == "ULong_t" || columnType == "unsigned long") return defineTyped(ULong_t{});
      // other types (collections, strings, long double, classes) take the general way below
    }
    const std::string typeColumn = colName + "_TYPE";
    node = node.Define(typeColumn, toRootCode(expression));
    columnType = node.GetColumnType(typeColumn);
    const bool isScalar = columnType.find("RVec") == std::string::npos && columnType.find("vector") == std::string::npos && columnType.find("string") == std::string::npos;
    const std::string code = toRootCode(expression);
    if (!isScalar) {
      node = node.Define(colName, code);
      return false;
    }
    // an expression is cast directly, code with a return (a function body) is called as a lambda
    node = node.Define(colName, (code.find("return") == std::string::npos) ? fmt::format("static_cast<double>({})", code) : fmt::format("return static_cast<double>([&]() {{ {} }}());", code));
    return true;
  };
  for (auto& dataDim : mDims) {
    std::string colName = "SRP_AXIS_" + std::to_string(axisID);
    std::string columnType;
    bool isDouble{};
    try {
      isDouble = defineValue(colName, dataDim.var, columnType);
    } catch (const std::runtime_error&) {
      logger::error("Illegal expression {} for {}{}.", dataDim.var, mContext, AvailableColumns(node));
      return false;
    }
    if (columnType.find("string") != std::string::npos) {
      logger::error("Expression {} for {} is not numeric.", dataDim.var, mContext);
      return false;
    }
    mIsIntegerAxis[axisID - 1] = isIntegerType(columnType);
    mIsTyped = mIsTyped && isDouble;
    const bool isBinned = isProjection || (mIsProfile && axisID < mDims.size());
    if (isBinned && dataDim.nBins && dataDim.edges.size() == 2 && !dataDim.edges[0] && !dataDim.edges[1]) {
      // auto-detect bin edges given the data
      if (mIsIntegerAxis[axisID - 1] && !isDouble) {
        auto min = node.Min(colName);
        auto max = node.Max(colName);
        mAutoRanges[axisID - 1] = [min, max]() mutable { return auto_range_t{*min, *max, 0}; };
      } else {
        // a single inf would make the range infinite, a nan makes it depend on the order of the rows (which varies with multi-threading):
        // the range is taken from the finite values only, the others are counted for a warning
        auto bookFiniteRange = [&](auto value) {
          using value_t = decltype(value);
          auto range = node.Aggregate(
            [](auto_range_t& acc, value_t v) {
              if (std::isfinite(v)) {
                acc.min = std::min<double_t>(acc.min, v);
                acc.max = std::max<double_t>(acc.max, v);
              } else {
                ++acc.nNonFinite;
              }
            },
            [](const auto_range_t& a, const auto_range_t& b) { return auto_range_t{std::min(a.min, b.min), std::max(a.max, b.max), a.nNonFinite + b.nNonFinite}; },
            colName, auto_range_t{});
          mAutoRanges[axisID - 1] = [range]() mutable { return *range; };
        };
        if (isDouble) {
          bookFiniteRange(double_t{});  // scalars of any type are stored as double
        } else {
          // collections (RVec<float> etc.) with jitted code, the finite values as a column of their own
          const bool isCollection = columnType.find("RVec") != std::string::npos || columnType.find("vector") != std::string::npos;
          const std::string finiteName = colName + "_FINITE";
          const std::string nonFiniteName = colName + "_NONFINITE";
          // the explicit return is needed, since ROOT takes code containing a return (here the one of the lambda) as a function body
          auto finiteNode = (isCollection) ? node.Define(finiteName, fmt::format("return ROOT::VecOps::Filter({}, [](double x) {{ return std::isfinite(x); }});", colName))
                                           : node.Define(finiteName, colName).Filter(fmt::format("std::isfinite({})", finiteName));
          auto min = finiteNode.Min(finiteName);
          auto max = finiteNode.Max(finiteName);
          auto nNonFinite = node.Define(nonFiniteName, (isCollection) ? fmt::format("return static_cast<ULong64_t>(ROOT::VecOps::Sum(!ROOT::VecOps::Map({}, [](double x) {{ return std::isfinite(x); }})));", colName)
                                                                      : fmt::format("static_cast<ULong64_t>(!std::isfinite({}))", colName))
                              .Sum<ULong64_t>(nonFiniteName);
          mAutoRanges[axisID - 1] = [min, max, nNonFinite]() mutable { return auto_range_t{*min, *max, *nNonFinite}; };
        }
      }
    }
    if (mIsProfile && axisID == mDims.size()) {
      mHistTitle += ";#LT " + dataDim.var + " #GT";
    } else {
      mHistTitle += ";" + dataDim.var;
    }
    ++axisID;
  }
  if (!mIsProfile && !mIsScatter) {
    mHistTitle += (dataInfo.weight) ? ";weighted counts" : ";counts";
  }
  if (dataInfo.weight && !mIsScatter) {
    try {
      std::string columnType;
      mIsTyped = defineValue("SRP_AXIS_W", *dataInfo.weight, columnType) && mIsTyped;
    } catch (const std::runtime_error&) {
      logger::error("Illegal weight expression {} for {}{}.", *dataInfo.weight, mContext, AvailableColumns(node));
      return false;
    }
    mHasWeights = true;
  }
  mNode = node;
  return true;
}

// whether an axis range needs to be determined from the data before the result can be booked
template <typename DataInfo>
bool DataFrameRequest<DataInfo>::NeedsAutoRange() const
{
  return std::any_of(mAutoRanges.begin(), mAutoRanges.end(), [](const auto& range) { return static_cast<bool>(range); });
}

// store the result of the booked action and its handle
template <typename DataInfo>
template <typename T>
void DataFrameRequest<DataInfo>::SetResult(ROOT::RDF::RResultPtr<T> result)
{
  mHandles.emplace_back(result);
  mGetResult = [result, name = mObjName, title = mHistTitle]() mutable -> TObject* {
    auto obj = static_cast<T*>(result->Clone(name.data()));
    if constexpr (std::is_base_of_v<TH1, T>) {
      obj->SetDirectory(nullptr);
    } else if constexpr (std::is_base_of_v<TGraph, T>) {
      obj->SetTitle(title.data());
    }
    return obj;
  };
}

// book the final result (once the axis ranges are known)
template <typename DataInfo>
bool DataFrameRequest<DataInfo>::Book()
{
  ROOT::RDF::RNode& node = *mNode;  // a non-dependent type: the typed actions below need no template keyword (clang)
  auto& dataDims = mDims;
  for (size_t i = 0; i < dataDims.size(); ++i) {
    if (mAutoRanges[i]) {
      const auto [min, max, nNonFinite] = mAutoRanges[i]();
      if (nNonFinite) {
        logger::warning("{} value{} of {} in {} {} not finite (inf or nan): the axis range is determined without {}.", nNonFinite, (nNonFinite > 1) ? "s" : "", dataDims[i].var, mContext, (nNonFinite > 1) ? "are" : "is", (nNonFinite > 1) ? "them" : "it");
      }
      if (mIsIntegerAxis[i] && min <= max) {
        std::tie(dataDims[i].nBins, dataDims[i].edges[0], dataDims[i].edges[1]) = util::integer_binning(min, max, dataDims[i].nBins);
      } else {
        std::tie(dataDims[i].edges[0], dataDims[i].edges[1]) = util::auto_axis_range(min, max);
      }
    }
  }
  const char* title = mHistTitle.data();
  using D = double_t;
  const bool typed = mIsTyped;

  if (mIsScatter) {
    if (dataDims.size() == 2) {
      SetResult((typed) ? node.Graph<D, D>("SRP_AXIS_1", "SRP_AXIS_2") : node.Graph("SRP_AXIS_1", "SRP_AXIS_2"));
    } else if (dataDims.size() == 4) {
      SetResult((typed) ? node.GraphAsymmErrors<D, D, D, D, D, D>("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_4")
                        : node.GraphAsymmErrors("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_4"));
    } else if (dataDims.size() == 6) {
      SetResult((typed) ? node.GraphAsymmErrors<D, D, D, D, D, D>("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_5", "SRP_AXIS_6")
                        : node.GraphAsymmErrors("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_5", "SRP_AXIS_6"));
    } else {
      logger::error("Invalid number of columns for scatter plot of {}.", mContext);
      return false;
    }
    return true;
  }

  const bool hasWeights = mHasWeights;
  if (dataDims.size() == 1) {
    auto histModel = ROOT::RDF::TH1DModel();
    auto& dataDim1 = dataDims.at(0);

    if (!dataDim1.nBins) {
      histModel = ROOT::RDF::TH1DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data());
    } else {
      histModel = ROOT::RDF::TH1DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1]);
    }
    if (hasWeights) {
      SetResult((typed) ? node.Histo1D<D, D>(histModel, "SRP_AXIS_1", "SRP_AXIS_W") : node.Histo1D(histModel, "SRP_AXIS_1", "SRP_AXIS_W"));
    } else {
      SetResult((typed) ? node.Histo1D<D>(histModel, "SRP_AXIS_1") : node.Histo1D(histModel, "SRP_AXIS_1"));
    }
  } else if (dataDims.size() == 2) {
    auto& dataDim1 = dataDims.at(0);
    auto& dataDim2 = dataDims.at(1);
    if (mIsProfile) {
      auto profileModel = ROOT::RDF::TProfile1DModel();
      if (!dataDim1.nBins) {
        profileModel = ROOT::RDF::TProfile1DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data());
      } else {
        profileModel = ROOT::RDF::TProfile1DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1]);
      }
      if (hasWeights) {
        SetResult((typed) ? node.Profile1D<D, D, D>(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W") : node.Profile1D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W"));
      } else {
        SetResult((typed) ? node.Profile1D<D, D>(profileModel, "SRP_AXIS_1", "SRP_AXIS_2") : node.Profile1D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2"));
      }
    } else {
      auto histModel = ROOT::RDF::TH2DModel();
      if (!dataDim1.nBins && !dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (dataDim1.nBins && dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      } else if (dataDim1.nBins && !dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (!dataDim1.nBins && dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      }
      if (hasWeights) {
        SetResult((typed) ? node.Histo2D<D, D, D>(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W") : node.Histo2D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W"));
      } else {
        SetResult((typed) ? node.Histo2D<D, D>(histModel, "SRP_AXIS_1", "SRP_AXIS_2") : node.Histo2D(histModel, "SRP_AXIS_1", "SRP_AXIS_2"));
      }
    }
  } else if (dataDims.size() == 3) {
    auto& dataDim1 = dataDims.at(0);
    auto& dataDim2 = dataDims.at(1);
    auto& dataDim3 = dataDims.at(2);

    if (mIsProfile) {
      auto profileModel = ROOT::RDF::TProfile2DModel();
      if (!dataDim1.nBins && !dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (dataDim1.nBins && dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      } else if (dataDim1.nBins && !dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (!dataDim1.nBins && dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      }
      if (hasWeights) {
        SetResult((typed) ? node.Profile2D<D, D, D, D>(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W") : node.Profile2D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W"));
      } else {
        SetResult((typed) ? node.Profile2D<D, D, D>(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3") : node.Profile2D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3"));
      }
    } else {
      auto histModel = ROOT::RDF::TH3DModel();
      if (dataDim1.nBins && dataDim2.nBins && dataDim3.nBins) {
        histModel = ROOT::RDF::TH3DModel("tmp", title, dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1], dataDim3.nBins, dataDim3.edges[0], dataDim3.edges[1]);
      } else {
        // first convert all fixed size bins to variable size bining
        for (auto& dataDim : dataDims) {
          if (dataDim.nBins) {
            double_t binWidth = (dataDim.edges[1] - dataDim.edges[0]) / dataDim.nBins;
            std::vector<double_t> edges = {dataDim.edges[0]};
            for (int32_t i = 1; i <= dataDim.nBins; ++i) {
              edges.push_back(dataDim.edges[0] + i * binWidth);
            }
            dataDim.edges = edges;
            dataDim.nBins = 0;
          }
        }
        histModel = ROOT::RDF::TH3DModel("tmp", title, static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data(), static_cast<int32_t>(dataDim3.edges.size()) - 1, dataDim3.edges.data());
      }
      if (hasWeights) {
        SetResult((typed) ? node.Histo3D<D, D, D, D>(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W") : node.Histo3D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W"));
      } else {
        SetResult((typed) ? node.Histo3D<D, D, D>(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3") : node.Histo3D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3"));
      }
    }
  } else {
    if (mIsProfile) {
      logger::error("Too many dimensions for a profile of {}.", mContext);
      return false;
    }
    auto histModel = ROOT::RDF::THnDModel();
    bool allFixed = true;
    std::vector<int32_t> nBinsVec;
    std::vector<double_t> xMinVec;
    std::vector<double_t> xMaxVec;
    for (auto& dataDim : dataDims) {
      if (!dataDim.nBins) {
        allFixed = false;
        nBinsVec.push_back(static_cast<int32_t>(dataDim.edges.size()) - 1);
      } else {
        nBinsVec.push_back(dataDim.nBins);
        xMinVec.push_back(dataDim.edges[0]);
        xMaxVec.push_back(dataDim.edges[1]);
      }
    }

    if (allFixed) {
      histModel = ROOT::RDF::THnDModel("tmp", title, static_cast<int32_t>(dataDims.size()), nBinsVec, xMinVec, xMaxVec);
    } else {
      std::vector<std::vector<double_t>> xBins;
      for (auto& dataDim : dataDims) {
        if (dataDim.nBins) {
          double_t binWidth = (dataDim.edges[1] - dataDim.edges[0]) / dataDim.nBins;
          std::vector<double_t> edges = {dataDim.edges[0]};
          for (int32_t i = 1; i <= dataDim.nBins; ++i) {
            edges.push_back(dataDim.edges[0] + i * binWidth);
          }
          dataDim.edges = edges;
          dataDim.nBins = 0;
        }
        xBins.push_back(dataDim.edges);
      }
      histModel = ROOT::RDF::THnDModel("tmp", title, static_cast<int32_t>(dataDims.size()), nBinsVec, xBins);
    }
    std::vector<std::string> colNames;
    for (size_t i = 0; i < dataDims.size(); ++i) {
      colNames.push_back("SRP_AXIS_" + std::to_string(i + 1));
    }
    if (hasWeights) {
      colNames.push_back("SRP_AXIS_W");
    }
    // up to 7 columns typed (more dimensions are rare)
    switch ((typed) ? colNames.size() : 0) {
      case 4:
        SetResult(node.HistoND<D, D, D, D>(histModel, colNames));
        break;
      case 5:
        SetResult(node.HistoND<D, D, D, D, D>(histModel, colNames));
        break;
      case 6:
        SetResult(node.HistoND<D, D, D, D, D, D>(histModel, colNames));
        break;
      case 7:
        SetResult(node.HistoND<D, D, D, D, D, D, D>(histModel, colNames));
        break;
      default:
        SetResult(node.HistoND(histModel, colNames));
    }
  }
  return true;
}
}  // end namespace SciRooPlot
#endif  // SRC_DATAFRAMEREQUEST_H_
