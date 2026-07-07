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
 * The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef INCLUDE_PLOTMANAGER_H_
#define INCLUDE_PLOTMANAGER_H_

#include <tuple>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include "TApplication.h"
#include "Plot.h"

class TCanvas;
namespace ROOT
{
class RDataFrame;
}

namespace SciRooPlot
{
//**************************************************************************************************
/**
 * Central manager class.
 */
//**************************************************************************************************
class PlotManager
{
 public:
  PlotManager();

  ~PlotManager() = default;
  PlotManager(const PlotManager& other) = delete;
  PlotManager(PlotManager&&) = delete;
  PlotManager& operator=(const PlotManager& other) = delete;
  PlotManager& operator=(PlotManager&& other) = delete;

  static Plot MakeBasePlot(const std::string& name = "1d", double_t screenResolution = 100);

  void SaveProject(const std::string& projectName);

  // create or append to a dataset.
  void AddDataset(const std::string& dataset, const std::vector<std::string>& inputFiles);
  void AddDataset(const std::string& dataset, std::initializer_list<std::string> inputFiles);
  void AddDataset(const std::string& dataset, const std::string& inputFile);
  void AddDataset(const std::string& dataset, const std::vector<TObject*>& inputData);
  void AddDataset(const std::string& dataset, TObject* inputData);

  void SaveDatasets(const std::string& datasetFile) const;
  void LoadDatasets(const std::string& datasetFile);

  void AddPlot(Plot plot);
  void AddBasePlot(Plot basePlot);
  void AddColorOverview(const std::string& name, const std::string& group, const std::vector<int32_t>& colors = {});

  void SavePlots(const std::string& plotFile, const std::string& name = ".+", const std::string& group = ".+") const;
  void LoadPlots(const std::string& plotFile, const std::string& name = ".+", const std::string& group = ".+");
  void GeneratePlots(const std::string& mode = "pdf", const std::string& name = ".+", const std::string& group = ".+");
  void ListPlots() const;

  void SetOutputDirectory(const std::string& path);
  static std::tuple<std::string, std::string, std::string> GetProjectSettings(std::string projectName = "");

 private:
  TObject* FindSubDirectory(TObject* folder, std::vector<std::string>& subDirs) const;
  bool GeneratePlot(const Plot& plot, const std::string& mode = "pdf");
  void SavePlotsToRootFile() const;
  void SaveDataToRootFile() const;

  std::unique_ptr<TApplication> mApp;
  const std::string mPlotsRootFile{"Plots.root"};
  const std::string mDataRootFile{"Data.root"};
  std::map<std::string, std::shared_ptr<TCanvas>> mCanvasRegistry;
  std::string mOutputDirectory;
  std::vector<Plot> mPlots;
  std::vector<Plot> mBasePlots;
  std::map<std::string, boost::property_tree::ptree> mPropertyTreeCache;
  std::vector<const std::string*> mPlotViewHistory;
  int32_t mWindowOffsetY{};
  int32_t mFirstFreeColorIndex{TColor::GetFreeColorIndex()};
  const std::vector<std::string> mTableFileEndings = {".csv", ".dat", ".txt", ".tsv", ".tab"};

  std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<TObject>>> mDataBuffer;
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Plot::Pad::Data::data_info_t>>> mDataInfoBuffer;
  std::map<std::string, std::vector<std::string>> mInputFiles;  // dataset name -> input file paths
  void PrintBufferStatus(bool onlyMissing = false) const;
  bool FillBuffer();
  void ReadData(TObject* folder, std::vector<std::string>& dataNames, const std::string& prefix, const std::string& suffix, const std::string& dataset);
  void ReadTableData(const std::string& inputFileName, const std::string& name, const std::string& dataset);
  TObject* ProcessData(ROOT::RDataFrame& df, const std::string& dfName, const Plot::Pad::Data::data_info_t& treeInfo, const std::string& name) const;
};

}  // end namespace SciRooPlot
#endif  // INCLUDE_PLOTMANAGER_H_
