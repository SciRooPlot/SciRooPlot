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
    
    // --- status accessors --
    void PrintStatus();
    void ListPlots(bool verbose = false){for(auto& plot : mPlotLedger) {cout << plot.GetName() << endl;}} // list all plots available in the manager
    void ListData(); // list all input data (histos, graphs, etc) available in the manager
    void ListPlotStyles(); // list all available plot styles
    

  private:
    TApplication mApp;
    // IO management related
    string mOutputDirectory;    /// directory to store output
    bool mUseUniquePlotNames; // wether to use plot names or unique plot names for output files
    map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
    map<string, set<string>> mLoadedData;
    void LoadData(map<string, set<string>>& requiredData);
    TObjArray* mDataLedger;     /// all external data representations currently loaded by the manager
    TObject* GetDataClone(TObject* folder, string dataName);
    TObject* FindSubDirectory(TObject* folder, vector<string> subDirs);
    // content related members
    vector<Plot> mPlotLedger;   /// internal representation of of plots known to the manager
    void GeneratePlot(Plot& plot, string outputMode = "pdf");
    bool IsPlotPossible(Plot &plot);
    bool IsPlotAlreadyBooked(string plotName){for(auto& plot : mPlotLedger){if(plot.GetUniqueName() == plotName) return true;} return false;};

    // style related members
    vector<PlotStyle> mPlotStyles;
    void DefineDefaultPlottingStyles();
    PlotStyle& GetPlotStyle(string plotStyleName);
    //    bool IsPlotStyleAvailable(string "plot");

    PlotManager(const PlotManager&) = default;
    PlotManager& operator=(const PlotManager&) = default;
  };
  }
#endif
