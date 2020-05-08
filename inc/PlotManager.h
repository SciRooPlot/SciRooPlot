// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see docs/Credits
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
#include "PlotPainter.h"
#include "Plot.h"

using namespace PlottingFramework;
namespace PlottingFramework
{
//****************************************************************************************
/**
 * Central manager class.
  */
//****************************************************************************************
class PlotManager
{
public:
  PlotManager();
  virtual ~PlotManager();
  
  void ReadDataFromCSVFiles(TObjArray& outputDataArray, vector<string> fileNames, string inputIdentifier);
  void ReadDataFromFiles(TObjArray& outputDataArray, vector<string> fileNames, vector<string> dataNames, vector<string> newDataNames = {});
  void ReadData(TObject* folder, TObjArray& outputDataArray, vector<string>& dataNames, vector<string>& newDataNames);
  
  
  // -> files
  void SetUseUniquePlotNames(bool useUniquePlotNames = true){mUseUniquePlotNames = useUniquePlotNames;}
  void SetOutputDirectory(string path);
  void AddInputDataFiles(string inputIdentifier, vector<string> inputFilePathList);
  void AddInputDataFile(string inputIdentifier, string inputFilePath);
  void DumpInputDataFiles(string configFileName);
  void LoadInputDataFiles(string configFileName);
  void ClearLoadedData() {mDataLedger->Delete(); mLoadedData.clear();};
  
  // -> styles
  void DumpPlottingStyles(string configFileName){} // todo: possibility to also dump only specific style to the file
  void LoadPlottingStyles(string configFileName){} // todo: possibility to load only specific style from the file
  
  // -> plots
  void AddPlot(Plot& plot);
  void AddPlotTemplate(Plot& plotTemplate);

  void DumpPlots(string plotFileName, string figureGroup = "", vector<string> plotNames = {});
  void DumpPlot(string plotFileName, string figureGroup, string plotName);
  void CreatePlots(string figureGroup = "", string figureCategory = "", vector<string> plotNames = {}, string outputMode = "pdf");
  void CreatePlot(string name, string figureGroup, string figureCategory = "", string outputMode = "pdf");
    
  // --- status accessors --
  void ListPlots(bool verbose = false){for(auto& plot : mPlots) {PRINT("{}", plot.GetName());}} // list all plots available in the manager
  void ListData(); // list all input data (histos, graphs, etc) available in the manager
  
  void SetOutputFileName(string fileName = "ResultPlots.root") {mOutputFileName = fileName;}  
  void ExtractPlotsFromFile(string plotFileName, vector<string> figureGroupsWithCategoryUser, vector<string> plotNamesUser, string mode = "load");

private:
  vector<string> splitString(string argString, char deliminator = ':');
  
  map<string, int> mNameRegister; // bi-directional mapping between name and unique id
  inline int GetNameRegisterID(const string& name);
  inline const string& GetNameRegisterName(int nameID);
  map<int, set<int>> mLoadedData;
  void LoadData(map<int, set<int>>& requiredData);
  
  vector<string> mPlotViewHistory;
  
  TApplication mApp;
  // IO management related
  bool mSaveToRootFile;
  string mOutputFileName;
  map<string, shared_ptr<TCanvas>> mPlotLedger;
  
  string mOutputDirectory;    /// directory to store output
  bool mUseUniquePlotNames; // wether to use plot names or unique plot names for output files
  map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
  TObjArray* mDataLedger;     /// all external data representations currently loaded by the manager
  TObject* FindSubDirectory(TObject* folder, vector<string> subDirs);
  // content related members
  void GeneratePlot(Plot& plot, string outputMode = "pdf");
  bool IsPlotPossible(Plot &plot);
  bool IsPlotAlreadyBooked(string plotName){for(auto& plot : mPlots){if(plot.GetUniqueName() == plotName) return true;} return false;};
  
  // style related members
  vector<Plot> mPlots;
  vector<Plot> mPlotTemplates;

  map<string, ptree> mPlotTemplateCache; // FIXME: find better name for this or delete...
  ptree& ReadPlotTemplatesFromFile(string& plotFileName);
  
};

} // end namespace PlottingFramework
#endif /* PlotManager_h */
