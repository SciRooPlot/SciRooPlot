/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot --------------------------------------
 * Copyright (c) 2019-2025 Mario Krüger
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

#include "SciRooPlot.h"
#include "Plot.h"
#include <tuple>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

class TApplication;
class TCanvas;

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

  static Plot GetPlotTemplate(const string& plotTemplateName = "1d", double_t screenResolution = 100);
  static tuple<string, string, string> GetProjectSettings(string projectName = "");
  void SaveProject(const string& projectName);

  // settings for output
  void SetOutputDirectory(const string& path);
  void SetUseUniquePlotNames(bool useUniquePlotNames = true);           // if true plot names are set to plotName_IN_figureGroup[.pdf,...]
  void SetOutputFileName(const string& fileName = "ResultPlots.root");  // in case canvases should be saved in .root file

  // settings related to the input root files
  void AddInputDataFiles(const string& inputIdentifier, const vector<string>& inputFilePathList);
  void AddInputDataFile(const string& inputIdentifier, const string& inputFilePath);
  void DumpInputDataFiles(const string& configFileName) const;  // save input file paths to config file
  void LoadInputDataFiles(const string& configFileName);        // load the input file paths from config file

  // remove all loaded input data (histograms, graphs, ...) from the manager (usually not needed)
  void ClearDataBuffer();

  // add plots or templates for plots to the manager
  void AddPlot(Plot& plot);
  void AddPlotTemplate(Plot plotTemplate);

  // saving plot definitions to external file (which can e.g. be read by the command-line plotting app included in the framework)
  void DumpPlots(const string& plotFileName, const string& figureGroup = "", const vector<string>& plotNames = {}) const;
  void DumpPlot(const string& plotFileName, const string& figureGroup, const string& plotName) const;

  // read plots from plot definition file created by the above functions (regular expressions are
  // allowed); the mode variable can be "load" to add these plots to the manager, or "find" to check
  // only if the specified plots exist (prints out this info)
  void ExtractPlotsFromFile(const string& plotFileName,
                            const string& mode = "load",
                            const string& plotName = ".*",
                            const string& group = ".*",
                            const string& category = ".*");

  // after desired plots were added to the manager they can be created
  // the program then will try to extract the required input data (TH1,TGraph,..) from the specified
  // input files (.root output of your analysis) there are several modes: "interactive": you will be
  // prompted the plot and can scroll through all plots by double clicking on the right side of the
  // canvas "pdf", "png": plots will be stored as such files in the specified output directory
  // (subdirectories are created for the figure groups and categories) "macro": plots are saved as
  // root macros (.C) "file": all plots (canvases) are put in a .root file with a directory
  // structure corresponding to figure groups and categories
  void CreatePlots(const string& figureGroup = "", const string& figureCategory = "",
                   vector<string> plotNames = {}, const string& outputMode = "pdf");
  void CreatePlot(const string& name, const string& figureGroup, const string& figureCategory = "",
                  const string& outputMode = "pdf");
  void PrintLoadedPlots() const;

 private:
  TObject* FindSubDirectory(TObject* folder, vector<string>& subDirs) const;
  bool GeneratePlot(const Plot& plot, const string& outputMode = "pdf");
  ptree& ReadPlotTemplatesFromFile(const string& plotFileName);
  void SavePlotsToFile() const;

  unique_ptr<TApplication> mApp;
  bool mSaveToRootFile{};
  string mOutputFileName;
  map<string, shared_ptr<TCanvas>> mPlotLedger;
  string mOutputDirectory;
  bool mUseUniquePlotNames{};
  vector<Plot> mPlots;
  vector<Plot> mPlotTemplates;
  map<string, ptree> mPropertyTreeCache;
  vector<const string*> mPlotViewHistory;
  int32_t mWindowOffsetY{};

  unordered_map<string, unordered_map<string, unique_ptr<TObject>>> mDataBuffer;
  map<string, vector<string>> mInputFiles;  // inputFileIdentifier, inputFilePaths
  void PrintBufferStatus(bool missingOnly = false) const;
  bool FillBuffer();
  void ReadData(TObject* folder, vector<string>& dataNames, const string& prefix, const string& suffix, const string& inputID);
  void ReadDataCSV(const string& inputFileName, const string& graphName, const string& inputIdentifier);
};

}  // end namespace SciRooPlot
#endif  // INCLUDE_PLOTMANAGER_H_
