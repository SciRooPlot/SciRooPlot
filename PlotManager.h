// Plotting Framework
//
// Copyright (C) 2019  Mario Krüger
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
#include "PlotGenerator.h"
#include "PlotStyle.h"
#include "Plot.h"

using namespace PlottingFramework;
namespace PlottingFramework
  {
  
  class PlotManager
  {
  public:
    PlotManager();
    virtual ~PlotManager();
    
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
    void DumpPlots(string plotFileName, string figureGroup = "", vector<string> plotNames = {});
    void DumpPlot(string plotFileName, string figureGroup, string plotName);
    void LoadPlots(string plotFileName, string figureGroup = "", vector<string> plotNames = {});
    void LoadPlot(string plotFileName, string figureGroup, string plotName);
    void CreatePlots(string figureGroup = "", vector<string> plotNames = {}, string outputMode = "pdf");
    void CreatePlot(string name, string figureGroup, string outputMode = "pdf");
    
    void CreatePlotsFromFile(string plotFileName, string figureGroup = "", vector<string> plotNames = {}, string outputMode = "pdf");
    void CreatePlotFromFile(string plotFileName, string figureGroup, string plotName , string outputMode = "pdf");

    
    // --- status accessors --
    void PrintStatus();
    void ListPlots(bool verbose = false){for(auto& plot : mPlots) {cout << plot.GetName() << endl;}} // list all plots available in the manager
    void ListData(); // list all input data (histos, graphs, etc) available in the manager
    void ListPlotStyles(); // list all available plot styles
    
    void SetOutputFileName(string fileName = "ResultPlots.root") {mOutputFileName = fileName;}

  private:

    map<string, int> mNameRegister; // bi-directional mapping between name and unique id
    inline int GetNameRegisterID(const string& name);
    inline const string& GetNameRegisterName(int nameID);
    map<int, set<int>> mLoadedData;
    void LoadData(map<int, set<int>>& requiredData);

    
    TApplication mApp;
    // IO management related
    string mOutputFileName;
    map<string, shared_ptr<TCanvas>> mPlotLedger;
    
    string mOutputDirectory;    /// directory to store output
    bool mUseUniquePlotNames; // wether to use plot names or unique plot names for output files
    map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
    TObjArray* mDataLedger;     /// all external data representations currently loaded by the manager
    TObject* GetDataClone(TObject* folder, string dataName);
    TObject* FindSubDirectory(TObject* folder, vector<string> subDirs);
    // content related members
    void GeneratePlot(Plot& plot, string outputMode = "pdf");
    bool IsPlotPossible(Plot &plot);
    bool IsPlotAlreadyBooked(string plotName){for(auto& plot : mPlots){if(plot.GetUniqueName() == plotName) return true;} return false;};

    // style related members
    vector<Plot> mPlots;   /// internal representation of of plots known to the manager
    vector<PlotStyle> mPlotStyles;
    void DefineDefaultPlottingStyles();
    PlotStyle& GetPlotStyle(string plotStyleName);
    //    bool IsPlotStyleAvailable(string "plot");

    PlotManager(const PlotManager&) = default;
    PlotManager& operator=(const PlotManager&) = default;
  };
  }
#endif
