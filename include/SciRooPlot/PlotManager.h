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
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

class TCanvas;
class TDirectory;
class TChain;
class TKey;
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

  void SaveProject() const;

  // create or append to a dataSource; replace = true clears existing entries from the dataSource first
  void AddDataSource(const std::string& dataSource, const std::vector<std::string>& inputs, bool replace = false);
  void AddDataSource(const std::string& dataSource, std::initializer_list<std::string> inputs, bool replace = false);
  void AddDataSource(const std::string& dataSource, const std::string& input, bool replace = false);
  // in-memory objects are copied into the project: a named list or a directory becomes a folder of the dataSource,
  // the contents of a nameless list or of a file are added directly
  void AddDataSource(const std::string& dataSource, const std::vector<TObject*>& inputs, bool replace = false);
  void AddDataSource(const std::string& dataSource, TObject* input, bool replace = false);

  void SaveDataSources(const std::optional<std::string>& file = {}) const;
  void LoadDataSources(const std::optional<std::string>& file = {}, bool replace = false);

  void AddPlot(Plot plot);
  void AddBasePlot(Plot basePlot);
  // Base plots with consistent layout among them: text sizes, tick lengths, distances, line widths and marker sizes are the
  // same in every panel (see PanelLayout.h), so that plots of different layouts look alike. The name describes the layout:
  //   <type>[_wide|_tall][_<columns>x<rows>][_ratio][_gap]
  // type 1d (plain panels) or 2d (panels with colour scale); wide/tall panels with the golden ratio; a grid of main panels
  // (default 1x1) that share their axes; a ratio panel below each main panel; gaps between the panels (own axes).
  // E.g. 1d, 1d_ratio, 2d, 1d_wide, 1d_2x1, 1d_3x1, 1d_2x1_ratio, 1d_2x2, 2d_2x1, 1d_2x1_gap. Pads are numbered row by row.
  // The code of this function is the reference for defining further layouts with PanelLayout.
  static Plot MakeBasePlot(const std::string& name = "1d");
  // The same base plot with other lengths, taken from sizes (its panels are not used): text and title sizes, tick lengths,
  // margins, line width and marker size in pixels of the plot, e.g. MakeBasePlot("1d_ratio", PanelLayout().SetTextSize(24).SetTitleSize(30)).
  // They are converted for every pad, so they stay equal in all panels; changing the text size of single pads afterwards does not.
  static Plot MakeBasePlot(const std::string& name, const PanelLayout& sizes);
  void AddColorOverview(const std::string& name, const std::string& group, const std::vector<int32_t>& colors = {});

  void SavePlots(const std::string& name = ".+", const std::string& group = ".+", const std::optional<std::string>& file = {}) const;
  bool LoadPlots(const std::string& name = ".+", const std::string& group = ".+", const std::optional<std::string>& file = {});
  bool GeneratePlots(const std::string& mode = "show", const std::string& name = ".+", const std::string& group = ".+");
  void ListPlots() const;
  void ClearDataBuffer();
  void ClearCanvasRegistry();

  std::string GetProjectProperty(const std::string& property) const;

  void SetOutputDirectory(const std::string& path);
  // names of the ROOT files written to the output directory in the modes file (plots) and data (input data)
  void SetPlotsRootFile(const std::string& fileName);
  void SetDataRootFile(const std::string& fileName);

 private:
  bool GeneratePlot(const Plot& plot, const std::string& mode = "pdf");
  static void KeepOnClose(TCanvas* canvas);
  static bool IsClosed(const TCanvas* canvas);
  void ShowAt(TCanvas* canvas, int32_t topX, int32_t topY) const;
  static void Close(TCanvas* canvas);
  void OpenWindow(std::shared_ptr<TCanvas> canvas, std::optional<std::pair<int32_t, int32_t>> followTop = std::nullopt, bool cascade = true);
  void ShowDetached();
  std::optional<size_t> NextBrowseIndex(size_t index, bool forward) const;
  bool HandleOpenWindows(bool& closeAll);
  void WaitForOpenWindows();
  static void SetTerminalKeys(bool on);
  static char TerminalKey();
  bool SavePlotsToRootFile() const;
  bool CreateOutputDirectory() const;
  bool SaveDataToRootFile() const;

  std::string mProjectName;
  std::string mPlotsRootFile{"Plots.root"};
  std::string mDataRootFile{"Data.root"};
  const std::string mUserDataFile;
  bool mUserDataFileInitialized{false};
  std::map<std::string, std::shared_ptr<TCanvas>> mCanvasRegistry;
  std::string mOutputDirectory;
  std::vector<Plot> mPlots;
  std::vector<Plot> mBasePlots;
  std::set<std::string> mMissingBasePlots;                     // reported once each
  std::map<std::string, std::set<std::string>> mNotFoundData;  // data source -> names not found in any of its inputs (last reading)
  struct open_window_t {
    std::shared_ptr<TCanvas> canvas;
    std::tuple<int32_t, int32_t, int32_t> handledEvent;  // event, x, y: the last event handled (it stays the canvas' current one until the next)
  };
  std::vector<open_window_t> mOpenWindows;  // windows open until the user closes them: all plots in mode "spread", plots detached in mode "show"
  std::string mGifName;
  bool mExitInteractiveBrowsing{false};
  std::optional<std::pair<std::shared_ptr<TCanvas>, std::pair<int32_t, int32_t>>> mDetached;  // in mode "show": plot detached, shown once the browsing window shows another one (with the browsing window's top)
  // mode "show": the selected plots, made when browsing reaches them
  std::vector<std::shared_ptr<TCanvas>> mBrowseCanvases;       // per selected plot, null if not made (or detached)
  std::vector<bool> mBrowseSkip;                               // per selected plot: detached or could not be made
  size_t mBrowseIndex{};                                       // the plot to show next
  bool mBrowseForward{true};                                   // the direction browsing went last
  std::optional<size_t> mShownIndex;                           // the plot in the browsing window
  std::vector<size_t> mBrowseHistory;                          // the plots shown, the one shown last at the end (each once)
  std::optional<std::pair<int32_t, int32_t>> mBrowsePosition;  // the top left corner of the browsing window
  int32_t mWindowOffsetY{};
  std::optional<bool> mHasDisplay;  // a graphical display is available (required for modes "show", "spread" and "macro"), determined by the first GeneratePlots
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
  struct container_deleter_t {
    void operator()(TObject* container) const;  // deletes a list or folder read from a file with all objects in it
  };
  using read_cache_t = std::map<TKey*, std::unique_ptr<TObject, container_deleter_t>>;  // lists and folders read from the keys of the input file currently read
  static TObject* FindSubDirectory(TObject* folder, const std::vector<std::string>& subDirs, read_cache_t& readCache);
  static bool IsContainer(const TClass* cl);
  static TClass* ReadableClass(TKey* key);
  static void ForEachItem(TObject* container, const std::function<bool(const std::string&, TObject*)>& func);
  void PrintBufferStatus(bool onlyMissing = false) const;
  std::shared_ptr<Plot::Pad::Data> RefFuncDrawnIn(Plot& plot, uint8_t padID);
  std::vector<std::tuple<std::string, std::string, Plot::Pad::Data::data_info_t>> GetMissingData(Plot& plot);
  bool FillBuffer();
  bool IsBuffered(const std::string& dataSource, const std::string& name) const;
  std::vector<std::string> ExpandInputs(const std::string& dataSource) const;
  bool IsDataSourceDefined(const std::string& dataSource) const;
  static std::string DataLocation(const std::string& dataSource, const std::string& name);
  static void ReserveColorIndices(TCanvas* canvas);
  static std::vector<std::string> MatchFolders(TDirectory* topDir, const std::string& pattern);
  void ReadData(TObject* folder, std::set<std::string>& dataNames, const std::string& prefix, const std::string& suffix, const std::string& dataSource, read_cache_t& readCache);
  static bool PrepareInput(TObject* obj, const std::string& location);
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
