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
  
  void SetUseUniquePlotNames(bool useUniquePlotNames = true){mUseUniquePlotNames = useUniquePlotNames;}
  void SetOutputDirectory(string path);
  void AddInputDataFiles(string inputIdentifier, vector<string> inputFilePathList);
  void AddInputDataFile(string inputIdentifier, string inputFilePath);
  void DumpInputDataFiles(string configFileName);
  void LoadInputDataFiles(string configFileName);
  void ClearLoadedData() {mDataLedger->Delete(); mLoadedData.clear();};
    
  void AddPlot(Plot& plot);
  void AddPlotTemplate(Plot& plotTemplate);

  void DumpPlots(string plotFileName, string figureGroup = "", vector<string> plotNames = {});
  void DumpPlot(string plotFileName, string figureGroup, string plotName);
  void CreatePlots(string figureGroup = "", string figureCategory = "", vector<string> plotNames = {}, string outputMode = "pdf");
  void CreatePlot(string name, string figureGroup, string figureCategory = "", string outputMode = "pdf");
    
  void SetOutputFileName(string fileName = "ResultPlots.root") {mOutputFileName = fileName;}  
  void ExtractPlotsFromFile(string plotFileName, vector<string> figureGroupsWithCategoryUser, vector<string> plotNamesUser, string mode = "load");

private:
  TObject* FindSubDirectory(TObject* folder, vector<string> subDirs);
  inline int GetNameRegisterID(const string& name);
  inline const string& GetNameRegisterName(int nameID);
  void LoadData(map<int, set<int>>& requiredData);
  void GeneratePlot(Plot& plot, string outputMode = "pdf");
  bool IsPlotPossible(Plot &plot);
  bool IsPlotAlreadyBooked(string plotName){for(auto& plot : mPlots){if(plot.GetUniqueName() == plotName) return true;} return false;};
  ptree& ReadPlotTemplatesFromFile(string& plotFileName);

  TApplication mApp;
  vector<string> splitString(string argString, char deliminator = ':');
  map<string, int> mNameRegister; // bi-directional mapping between name and unique id
  map<int, set<int>> mLoadedData;
  bool mSaveToRootFile;
  string mOutputFileName;
  map<string, shared_ptr<TCanvas>> mPlotLedger;
  string mOutputDirectory;    /// directory to store output
  bool mUseUniquePlotNames; // wether to use plot names or unique plot names for output files
  map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
  TObjArray* mDataLedger;     /// all external data representations currently loaded by the manager
  vector<Plot> mPlots;
  vector<Plot> mPlotTemplates;
  map<string, ptree> mPropertyTreeCache;
  vector<string> mPlotViewHistory;
};

} // end namespace PlottingFramework
#endif /* PlotManager_h */
