#ifndef PlotManager_h
#define PlotManager_h

#include "PlottingFramework.h"
#include "PlotGenerator.h"
#include "PlotStyle.h"
#include "Plot.h"

using namespace PlottingFramework;
namespace PlottingFramework {
  
  class PlotManager
  {
  public:

    PlotManager();
    virtual ~PlotManager();

    void AddHistosFromInputFile(string inputIdentifier, string inputFileName);
    void SetOutputDirectory(string path);

    void ListPlots(bool verbose = false){for(auto& plot : mSavedPlots) {cout << plot.GetName() << endl;}} // list all plots available in the manager
    void ListData(){} // list all input data (histos, graphs, etc) available in the manager
    void ListPlotStyles(); // list all available plot styles

    
    void CreatePlot(Plot& plot, string plotStyleName = "");
    void CreatePlot(string name, string figureGroup, string plotStyleName = "");
    void CreatePlots(string figureGroup = "");
    void SavePlot(string plotName, string figureGroup, string subFolder = "", bool deletePlot = true); // make private?

    void DumpPlots(string plotFileName);
    void LoadPlots(string plotFileName, string figureGroup = "", vector<string> plots = {});
    void LoadPlot(string plotName, string ploFileName){} // load specific plot from file
    //TH1* GetDataClone(string dataName);
    
    
    bool ContainsDatasets(std::initializer_list<string> requiredDatasets);
    
    void PrintErrors(bool printMissingPlots = false);
    
    void AddInputFilePaths(string inputIdentifier, vector<string> inputFilePathList);

    void DumpInputFiles(string configFileName);
    void LoadInputFiles(string configFileName);

    void AddPlot(ptree plotTree){
      try{
        string plotName = plotTree.get<string>("name") + "_@_" + plotTree.get<string>("figureGroup");
        Plot plot(plotTree);
        AddPlot(plot);
      }catch(...)
      {
        cout << "ERROR: could not store tree." << endl;
      }
    }
    
    void AddPlot(Plot& plot) {
      if(IsPlotAlreadyBooked(plot.GetUniqueName())) {cout << plot.GetUniqueName() << " booked already" << endl; return;}
      for(auto& padData : plot.GetData()){
        for(auto data : padData.second){
          mRequiredInput[data->GetInputIdentifier()].insert(data->GetName());
        }
      }
      /*
      for(auto& reqInput : mRequiredInput){
        cout << "InputID: " << reqInput.first << endl;
        cout << "Content:" << endl;
        for(auto& setContent : reqInput.second){
          cout << "   - " << setContent << endl;
        }
      }
       */
      mSavedPlots.push_back(std::move(plot));
    }
    
    PlotStyle GetCopyOfStyle(string plotStyleName)
    {
      for(auto& plotStyle : mPlotStyles)
      {
        if(plotStyle.GetName() == plotStyleName) return plotStyle;
      }
      cout << "ERROR: PlotStyle " << plotStyleName << " is not loaded (crashing now...)." << endl;
    }
    
  private:
    
    // IO management related
    void OpenInputFiles(string inputIdentifier);
    void OpenRequiredFiles(Plot& plot);
    map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
    map<string, set<string>> mRequiredInput;
    map<string, set<string>> mLoadeddInput;

    // style related
    vector<PlotStyle> mPlotStyles;
    void DefineDefaultPlottingStyles();
    PlotStyle& GetPlotStyle(string plotStyleName);

    // content related
    vector<Plot> mSavedPlots;
    TObjArray* mHistoLedger;   ///<!  Container to store input histograms
    TObjArray* mPlotLedger;    ///<!  Container to store output plots
    string mOutputDirectory;  ///<!  Directory to store output
    
    vector<string> mDatasetIdentifiers;
    
    vector<string> mListOfMissingFiles;
    vector<string> mListOfMissingPlots;
    vector<string> mListOfMissingHistograms;
    
    // Bookkeeping functions
    void BookHistos(TDirectoryFile& folder, string inputIdentifier);
    //   void RemoveHisto(string histName);
    
    bool IsPlotPossible(Plot &plot);
    bool IsPlotAlreadyBooked(string plotName){for(auto& plot : mSavedPlots){if(plot.GetUniqueName() == plotName) return true;} return false;};

    PlotManager(const PlotManager&) = default;
    PlotManager& operator=(const PlotManager&) = default;
  };
}
#endif
