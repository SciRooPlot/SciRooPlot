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

#include "Plot.h"
#include <tuple>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

class TApplication;
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

  ~PlotManager();
  PlotManager(const PlotManager& other) = delete;
  PlotManager(PlotManager&&) = delete;
  PlotManager& operator=(const PlotManager& other) = delete;
  PlotManager& operator=(PlotManager&& other) = delete;

  static Plot GetPlotTemplate(const std::string& plotTemplateName = "1d", double_t screenResolution = 100);

  void SaveProject(const std::string& projectName);

  void SetOutputDirectory(const std::string& path);
  void SetOutputFileName(const std::string& fileName);  // in case canvases should be saved in .root file

  // create or append to a dataset.
  void AddDataset(const std::string& dataset, const std::vector<std::string>& inputFilePathList);
  void AddDataset(const std::string& dataset, std::initializer_list<std::string> inputFilePathList);
  void AddDataset(const std::string& dataset, const std::string& inputFilePath);
  void AddDataset(const std::string& dataset, const std::vector<TObject*>& inputDataList);
  void AddDataset(const std::string& dataset, TObject* inputData);

  void SaveDatasets(const std::string& configFileName) const;  // save datasets to config file
  void LoadDatasets(const std::string& configFileName);        // load datasets from config file

  // add plots or templates for plots to the manager
  void AddPlot(Plot& plot);
  void AddPlotTemplate(Plot plotTemplate);
  void AddColorPlot(const std::string& plotName, const std::string& group, const std::vector<int32_t>& colors = {});

  // saving plot definitions to external file (which can e.g. be read by the command-line plotting app included in the framework)
  void SavePlots(const std::string& plotsFile, const std::string& group = "", const std::vector<std::string>& plotNames = {}) const;
  void SavePlot(const std::string& plotsFile, const std::string& group, const std::string& plotName) const;

  void LoadPlots(const std::string& plotsFile, const std::string& plotName = ".*", const std::string& group = ".*", const std::string& subgroup = ".*");

  void CreatePlots(const std::string& mode = "pdf", const std::string& group = "", const std::string& subgroup = "", std::vector<std::string> plotNames = {});

  void ListPlots() const;

  void ClearDataBuffer();
  static std::tuple<std::string, std::string, std::string> GetProjectSettings(std::string projectName = "");
  static std::string GetDatasetsFile(std::string projectName = "");

 private:
  TObject* FindSubDirectory(TObject* folder, std::vector<std::string>& subDirs) const;
  bool GeneratePlot(const Plot& plot, const std::string& mode = "pdf");
  boost::property_tree::ptree& ReadPlotTemplatesFromFile(const std::string& plotFileName);
  void SavePlotsToFile() const;
  void SaveDataToFile() const;

  std::unique_ptr<TApplication> mApp;
  bool mSavePlotsToRootFile{};
  bool mSaveDataToRootFile{};
  std::string mPlotsRootFile{"Plots.root"};
  std::string mInputsRootFile{"Inputs.root"};
  std::map<std::string, std::shared_ptr<TCanvas>> mPlotLedger;
  std::string mOutputDirectory;
  std::vector<Plot> mPlots;
  std::vector<Plot> mPlotTemplates;
  std::map<std::string, boost::property_tree::ptree> mPropertyTreeCache;
  std::vector<const std::string*> mPlotViewHistory;
  int32_t mWindowOffsetY{};
  int32_t mFirstFreeColorIndex{TColor::GetFreeColorIndex()};
  std::vector<std::string> mTableFileEndings = {".csv", ".dat", ".txt", ".tsv", ".tab"};

  std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<TObject>>> mDataBuffer;
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Plot::Pad::Data::data_info_t>>> mDataInfoBuffer;
  std::map<std::string, std::vector<std::string>> mInputFiles;  // inputFileIdentifier, inputFilePaths
  void PrintBufferStatus(bool missingOnly = false) const;
  bool FillBuffer();
  void ReadData(TObject* folder, std::vector<std::string>& dataNames, const std::string& prefix, const std::string& suffix, const std::string& dataset);
  void ReadDataCSV(const std::string& inputFileName, const std::string& name, const std::string& dataset);
  TObject* ProcessData(ROOT::RDataFrame& df, const std::string& dfName, const Plot::Pad::Data::data_info_t& treeInfo, const std::string& name) const;
};

}  // end namespace SciRooPlot
#endif  // INCLUDE_PLOTMANAGER_H_
