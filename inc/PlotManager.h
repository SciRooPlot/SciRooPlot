// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
// Contact: mario.kruger@cern.ch
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PlotManager_h
#define PlotManager_h

#include "PlottingFramework.h"
#include "Plot.h"

class TApplication;
class TCanvas;
class TObjArray;

namespace PlottingFramework
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

  // settings for output
  void SetOutputDirectory(const string& path);
  void SetUseUniquePlotNames(bool useUniquePlotNames = true)
  {
    mUseUniquePlotNames = useUniquePlotNames;
  } // if true plot names are set to plotName_IN_figureGroup[.pdf,...]
  void SetOutputFileName(const string& fileName = "ResultPlots.root")
  {
    mOutputFileName = fileName;
  } // in case canvases should be saved in .root file

  // settings related to the input root files
  void AddInputDataFiles(const string& inputIdentifier, const vector<string>& inputFilePathList);
  void AddInputDataFile(const string& inputIdentifier, const string& inputFilePath);
  void DumpInputDataFiles(const string& configFileName); // save input file paths to config file
  void LoadInputDataFiles(
    const string& configFileName); // load the input file paths from config file

  // remove all loaded input data (histograms, graphs, ...) from the manager (usually not needed)
  void ClearLoadedData();

  // add plots or templates for plots to the manager
  void AddPlot(Plot& plot);
  void AddPlotTemplate(Plot& plotTemplate);

  // saving plot definitions to external file (which can e.g. be read by the commandlne plotting app
  // included in the framework)
  void DumpPlots(const string& plotFileName, const string& figureGroup = "",
                 const vector<string>& plotNames = {});
  void DumpPlot(const string& plotFileName, const string& figureGroup, const string& plotName);

  // read plots from plot definition file created by the above functions (regular expressions are
  // allowed); the mode variable can be "load" to add these plots to the manager, or "find" to check
  // only if the specified plots exist (prints out this info)
  void ExtractPlotsFromFile(const string& plotFileName,
                            const vector<string>& figureGroupsWithCategoryUser = {},
                            const vector<string>& plotNamesUser = {}, const string& mode = "load");

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

private:
  void ReadDataFromCSVFiles(TObjArray& outputDataArray, const vector<string>& fileNames,
                            const string& inputIdentifier);
  void ReadDataFromFiles(TObjArray& outputDataArray, const vector<string>& fileNames,
                         vector<string> dataNames, vector<string> newDataNames = {});
  void ReadData(TObject* folder, TObjArray& outputDataArray, vector<string>& dataNames,
                vector<string>& newDataNames);

  TObject* FindSubDirectory(TObject* folder, vector<string> subDirs);
  inline int GetNameRegisterID(const string& name);
  inline const string& GetNameRegisterName(int32_t nameID);
  void LoadData(map<int32_t, set<int32_t>>& requiredData);
  void GeneratePlot(Plot& plot, const string& outputMode = "pdf");
  bool IsPlotPossible(Plot& plot);
  bool IsPlotAlreadyBooked(const string& plotName);
  ptree& ReadPlotTemplatesFromFile(const string& plotFileName);

  std::unique_ptr<TApplication> mApp;
  vector<string> splitString(const string& argString, char deliminator = ':');
  map<string, int32_t> mNameRegister; // bi-directional mapping between name and unique id
  map<int32_t, set<int32_t>> mLoadedData;
  bool mSaveToRootFile;
  string mOutputFileName;
  map<string, shared_ptr<TCanvas>> mPlotLedger;
  string mOutputDirectory;  // directory to store output
  bool mUseUniquePlotNames; // wether to use plot names or unique plot names for output files
  map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
  TObjArray* mDataLedger; // all external data representations currently loaded by the manager
  vector<Plot> mPlots;
  vector<Plot> mPlotTemplates;
  map<string, ptree> mPropertyTreeCache;
  vector<string> mPlotViewHistory;
};

} // end namespace PlottingFramework
#endif /* PlotManager_h */
