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

#ifndef INCLUDE_SCIROOPLOT_PLOTMANAGER_H_
#define INCLUDE_SCIROOPLOT_PLOTMANAGER_H_

#include "SciRooPlot/Plot.h"

#include <TApplication.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class TCanvas;
class TDirectory;
class TChain;
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
  explicit PlotManager(const std::string& projectName = "");

  ~PlotManager();
  PlotManager(const PlotManager& other) = delete;
  PlotManager(PlotManager&&) = delete;
  PlotManager& operator=(const PlotManager& other) = delete;
  PlotManager& operator=(PlotManager&& other) = delete;

  static Plot MakeBasePlot(const std::string& name = "1d", double_t screenResolution = 100);

  void SaveProject() const;

  // create or append to a dataSource; replace = true clears existing entries from the dataSource first
  void AddDataSource(const std::string& dataSource, const std::vector<std::string>& inputs, bool replace = false);
  void AddDataSource(const std::string& dataSource, std::initializer_list<std::string> inputs, bool replace = false);
  void AddDataSource(const std::string& dataSource, const std::string& input, bool replace = false);
  void AddDataSource(const std::string& dataSource, const std::vector<TObject*>& inputs, bool replace = false);
  void AddDataSource(const std::string& dataSource, TObject* input, bool replace = false);

  void SaveDataSources(const std::optional<std::string>& file = {}) const;
  void LoadDataSources(const std::optional<std::string>& file = {}, bool replace = false);

  void AddPlot(Plot plot);
  void AddBasePlot(Plot basePlot);
  void AddColorOverview(const std::string& name, const std::string& group, const std::vector<int32_t>& colors = {});

  void SavePlots(const std::string& name = ".+", const std::string& group = ".+", const std::optional<std::string>& file = {}) const;
  bool LoadPlots(const std::string& name = ".+", const std::string& group = ".+", const std::optional<std::string>& file = {});
  bool GeneratePlots(const std::string& mode = "show", const std::string& name = ".+", const std::string& group = ".+");
  void ListPlots() const;
  void ClearDataBuffer();
  void ClearCanvasRegistry();

  std::string GetProjectProperty(const std::string& property) const;

  void SetOutputDirectory(const std::string& path);

 private:
  TObject* FindSubDirectory(TObject* folder, std::vector<std::string>& subDirs) const;
  bool GeneratePlot(const Plot& plot, const std::string& mode = "pdf");
  bool SavePlotsToRootFile() const;
  bool CreateOutputDirectory() const;
  bool SaveDataToRootFile() const;

  std::string mProjectName;
  const std::string mPlotsRootFile{"Plots.root"};
  const std::string mDataRootFile{"Data.root"};
  const std::string mUserDataFile;
  bool mUserDataFileInitialized{false};
  std::map<std::string, std::shared_ptr<TCanvas>> mCanvasRegistry;
  std::string mOutputDirectory;
  std::vector<Plot> mPlots;
  std::vector<Plot> mBasePlots;
  std::vector<std::shared_ptr<TCanvas>> mPlotViewHistory;
  std::string mGifName;
  bool mExitInteractiveBrowsing{false};
  int32_t mWindowOffsetY{};
  bool mHasDisplay{false};  // a graphical display is available (required for modes "show" and "macro")
  int32_t mFirstFreeColorIndex{TColor::GetFreeColorIndex()};
  const std::vector<std::string> mTableFileEndings = {".csv", ".dat", ".txt", ".tsv", ".tab"};

  std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<TObject>>> mDataBuffer;
  std::unordered_map<std::string, std::unordered_map<std::string, std::vector<Plot::Pad::Data::data_info_t>>> mDataInfoBuffer;
  std::map<std::string, std::vector<std::string>> mInputs;          // dataSource name -> inputs as added (files, file.root:folder, directories, wildcard patterns)
  std::map<std::string, std::vector<std::string>> mExpandedInputs;  // dataSource name -> its expanded inputs, determined once per GeneratePlots call
  struct tree_input_t {
    std::string file;      // file containing this part of the tree
    std::string treePath;  // path of the tree within the file
    bool isFolderInput;    // the input is a folder within the file (file.root:folder) rather than the whole file
  };
  std::map<std::string, std::map<std::string, std::vector<tree_input_t>>> mTreeInputs;  // data source -> tree name -> where it was found in the inputs of the data source currently read
  bool mIsFolderInput{};                                                                // the input currently read is a folder within a file
  void PrintBufferStatus(bool onlyMissing = false) const;
  std::vector<std::tuple<std::string, std::string, Plot::Pad::Data::data_info_t>> GetMissingData(Plot& plot);
  bool FillBuffer();
  std::vector<std::string> ExpandInputs(const std::string& dataSource) const;
  bool IsDataSourceDefined(const std::string& dataSource) const;
  static std::string DataLocation(const std::string& dataSource, const std::string& name);
  static std::vector<std::string> MatchFolders(TDirectory* topDir, const std::string& pattern);
  void ReadData(TObject* folder, std::vector<std::string>& dataNames, const std::string& prefix, const std::string& suffix, const std::string& dataSource);
  void ReadTableData(const std::string& inputFileName, const std::string& name, const std::string& dataSource);
  void ProcessDataRequests(const std::string& type, const std::string& dataSource, const std::string& name, const std::string& objNameSuffix,
                           const std::function<std::unique_ptr<ROOT::RDataFrame>()>& makeDataFrame, const std::string& inputsDescription = "",
                           const std::optional<std::vector<Plot::Pad::Data::data_info_t::join_t>>& joins = std::nullopt);
  std::vector<tree_input_t> FindTreeInputs(const std::string& dataSource, const std::string& treeName);
  std::optional<std::string> AttachJoin(TChain& chain, const std::string& treeName, const std::vector<tree_input_t>& treeInputs,
                                        const Plot::Pad::Data::data_info_t::join_t& join, std::vector<std::shared_ptr<TChain>>& joinedChains);
};

}  // end namespace SciRooPlot
#endif  // INCLUDE_SCIROOPLOT_PLOTMANAGER_H_
