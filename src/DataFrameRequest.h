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

#include "SciRooPlot/Logging.h"
#include "SciRooPlot/Plot.h"

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RResultHandle.hxx>
#include <TGraph.h>
#include <TH1.h>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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

  bool Prepare(ROOT::RDF::RNode node);
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

  const DataInfo* mInfo{};
  std::string mContext;  // e.g. 'tree source:name', used in messages
  std::string mObjName;  // name of the resulting object
  std::optional<ROOT::RDF::RNode> mNode;
  std::vector<data_dim_t> mDims;
  std::string mHistTitle;
  bool mIsProfile{};
  bool mIsScatter{};
  bool mHasWeights{};
  std::optional<ROOT::RDF::RResultPtr<ULong64_t>> mEntriesPreFilter;
  std::optional<ROOT::RDF::RResultPtr<ULong64_t>> mEntriesPostFilter;
  std::vector<std::optional<std::pair<ROOT::RDF::RResultPtr<double_t>, ROOT::RDF::RResultPtr<double_t>>>> mAutoRanges;  // (min, max) per axis
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
bool DataFrameRequest<DataInfo>::Prepare(ROOT::RDF::RNode node)
{
  const auto& dataInfo = *mInfo;
  if (dataInfo.isProfileNoScatter) {
    mIsProfile = *dataInfo.isProfileNoScatter;
    mIsScatter = !mIsProfile;
  }
  bool isProjection = (!mIsProfile && !mIsScatter);

  mDims = dataInfo.dataDims;  // copy since it will be modified
  if (mIsProfile && mDims.size() > 3) {
    ERROR("Too many dimensions specified for profile of {} ({}).", mContext, dataInfo.GetDescription());
    return false;
  }
  size_t axisID = 1;
  for (auto& dataDim : mDims) {
    if (isProjection || (mIsProfile && axisID < mDims.size())) {
      // sanity check for binned axes
      if ((dataDim.nBins && dataDim.edges.size() != 2) || (!dataDim.nBins && dataDim.edges.size() <= 1)) {
        ERROR("Ill defined binning for {} in {} ({}).", dataDim.var, mContext, dataInfo.GetDescription());
        return false;
      }
      if (!std::is_sorted(dataDim.edges.begin(), dataDim.edges.end())) {
        if (!(dataDim.edges.size() == 2 && !dataDim.edges[0] && !dataDim.edges[1])) {
          ERROR("Ill defined binning for {} in {} ({}).", dataDim.var, mContext, dataInfo.GetDescription());
          return false;
        }
      }
    }
    ++axisID;
  }

  if (dataInfo.definitions.keys && dataInfo.definitions.values) {
    for (size_t i = 0; i < dataInfo.definitions.keys->size(); ++i) {
      const auto& column = dataInfo.definitions.keys->at(i);
      const auto& expression = dataInfo.definitions.values->at(i);
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
  if (dataInfo.filters) {
    mEntriesPreFilter = node.Count();
    for (const auto& filter : *dataInfo.filters) {
      try {
        node = node.Filter(filter);
      } catch (const std::runtime_error&) {
        ERROR("Illegal filter expression {} for {}{}.", filter, mContext, AvailableColumns(node));
        return false;
      }
    }
  }
  mEntriesPostFilter = node.Count();

  axisID = 1;
  mAutoRanges.resize(mDims.size());
  for (auto& dataDim : mDims) {
    std::string colName = "SRP_AXIS_" + std::to_string(axisID);
    try {
      node = node.Define(colName, dataDim.var);
    } catch (const std::runtime_error&) {
      ERROR("Illegal expression {} for {}{}.", dataDim.var, mContext, AvailableColumns(node));
      return false;
    }
    if (node.GetColumnType(colName).find("string") != std::string::npos) {
      ERROR("Expression {} for {} is not numeric.", dataDim.var, mContext);
      return false;
    }
    const bool isBinned = isProjection || (mIsProfile && axisID < mDims.size());
    if (isBinned && dataDim.nBins && dataDim.edges.size() == 2 && !dataDim.edges[0] && !dataDim.edges[1]) {
      // auto-detect bin edges given the data
      mAutoRanges[axisID - 1] = std::make_pair(node.Min(colName), node.Max(colName));
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
      node = node.Define("SRP_AXIS_W", *dataInfo.weight);
    } catch (const std::runtime_error&) {
      ERROR("Illegal weight expression {} for {}{}.", *dataInfo.weight, mContext, AvailableColumns(node));
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
  return std::any_of(mAutoRanges.begin(), mAutoRanges.end(), [](const auto& range) { return range.has_value(); });
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
  auto& node = *mNode;
  auto& dataDims = mDims;
  for (size_t i = 0; i < dataDims.size(); ++i) {
    if (auto& range = mAutoRanges[i]) {
      double_t min = *range->first;
      double_t max = *range->second;
      double_t margin = 0.01;
      dataDims[i].edges[0] = min * (min > 0 ? (1. - margin) : (1. + margin));
      dataDims[i].edges[1] = max * (max > 0 ? (1. + margin) : (1. - margin));
    }
  }
  const char* title = mHistTitle.data();

  if (mIsScatter) {
    if (dataDims.size() == 2) {
      SetResult(node.Graph("SRP_AXIS_1", "SRP_AXIS_2"));
    } else if (dataDims.size() == 4) {
      SetResult(node.GraphAsymmErrors("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_4"));
    } else if (dataDims.size() == 6) {
      SetResult(node.GraphAsymmErrors("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_5", "SRP_AXIS_6"));
    } else {
      ERROR("Invalid number of columns for scatter plot of {}.", mContext);
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
      SetResult(node.Histo1D(histModel, "SRP_AXIS_1", "SRP_AXIS_W"));
    } else {
      SetResult(node.Histo1D(histModel, "SRP_AXIS_1"));
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
        SetResult(node.Profile1D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W"));
      } else {
        SetResult(node.Profile1D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2"));
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
        SetResult(node.Histo2D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W"));
      } else {
        SetResult(node.Histo2D(histModel, "SRP_AXIS_1", "SRP_AXIS_2"));
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
        SetResult(node.Profile2D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W"));
      } else {
        SetResult(node.Profile2D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3"));
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
        SetResult(node.Histo3D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W"));
      } else {
        SetResult(node.Histo3D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3"));
      }
    }
  } else {
    if (mIsProfile) {
      ERROR("Too many dimensions for a profile of {}.", mContext);
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
    SetResult(node.HistoND(histModel, colNames));
  }
  return true;
}
}  // end namespace SciRooPlot
#endif  // SRC_DATAFRAMEREQUEST_H_
