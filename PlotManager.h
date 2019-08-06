#ifndef PlotManager_h
#define PlotManager_h

#include "TFile.h"
#include "TStyle.h"
#include "TGraph.h"
#include "TF1.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "THn.h"
#include "TIterator.h"
#include "TKey.h"
#include "TObjArray.h"
#include "TObject.h"
#include "TSystem.h"
#include <TROOT.h>
#include <TObjectTable.h>
#include "TExec.h"
#include "TTimeStamp.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "TLegend.h"
#include "TText.h"
#include "TLatex.h"
#include "TError.h"
#include "TSpline.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TPaletteAxis.h"
#include <iostream>
#include "Plot.h"

namespace PlottingFramework {
  
  using std::cout;
  using std::endl;
  using std::flush;
  using std::string;
  using std::vector;
  using std::tuple;
  using std::pair;
  using std::map;
  using std::set;


  
  /** \class PlotManager
   This is a generic plotting framework for managing your histograms and plotting from different sources using ROOT functionality.
   */
  class PlotStyle
  {
  public:
    
    struct GlobalStyle{
      int pixelBaseLength;
      int fillStyleCanvas;
      double aspectRatio; // for asymmetric plots
      vector<int> defaultMarkers;
      vector<int> defaultMarkersFull;
      vector<int> defaultMarkersOpen;
      vector<int> defaultColors;
      bool useCMYK;
      int palette;
      bool drawTimestamps;
      vector<double> timestampPosition;
      int textFont;
      int lableFont;
      int titleFont;
      double markerSize;
      double markerSizeThick;
      double lineWidth;
      double lineWidthThick;
      double textSize;
      double lableSizeScaling;
      double titleSizeScaling;
    };
    
    typedef struct PadStyle{
      vector<double> position;  // xlow, ylow, xup, yup (relative to canvas)
      vector<double> margin; // top, bottom, left, right
      vector<double> titleOffset; // x, y, z
    } PadStyle;

    static GlobalStyle mGlobalStyle;

    vector<PadStyle>& GetPads() {};
    
    private:
      string mStyleName;
      double mWidth;
      double mHeight;
      vector<PadStyle> pads;

  };
  
  // instanciate canvas from template and plot specifications
  // collection of functionality, only used to encapsulate
  // returns canvas
  class PlotCreator
  {
    public:
      void CreatePlot() {};
    private:
      void MakeTextBox(){};
  };
  
  class PlotManager
  {
  public:
    
    struct GlobalStyle{
      int pixelBaseLength;
      int fillStyleCanvas;
      double aspectRatio; // for asymmetric plots
      vector<int> defaultMarkers;
      vector<int> defaultMarkersFull;
      vector<int> defaultMarkersOpen;
      vector<int> defaultColors;
      bool useCMYK;
      int palette;
      bool drawTimestamps;
      vector<double> timestampPosition;
      int textFont;
      int lableFont;
      int titleFont;
      double markerSize;
      double markerSizeThick;
      double lineWidth;
      double lineWidthThick;
      double textSize;
      double lableSizeScaling;
      double titleSizeScaling;
    };
    
    typedef struct PadStyle{
      vector<double> position;  // xlow, ylow, xup, yup (relative to canvas)
      vector<double> margin; // top, bottom, left, right
      vector<double> titleOffset; // x, y, z
    } PadStyle;
    
    typedef struct CanvasStyle{
      string styleName;
      double canvasWidth;
      double canvasHeight;
      vector<PadStyle> pads;
    } CanvasStyle;
    
    PlotManager();
    virtual ~PlotManager();
    
    TPaveText* MakeText(Plot::TextBox& textBoxTemplate);
    TLegend* MakeLegend(TPad* pad, Plot::TextBox& legendBoxTemplate, vector<Plot::Histogram>& histos);
    
    int GetDefaultColor(int colorIndex);
    int GetDefaultMarker(int markerIndex);
    
    void SetPalette(int palette);
    void AddHistosFromInputFile(string inputIdentifier, string inputFileName);
    void SetOutputDirectory(string path);
    void SavePlot(string plotName, string figureGroup, string subFolder = "", bool deletePlot = true);
    void WritePlotsToFile(string outputFileName); //todo: also file structure
    void ListPlots(){mPlotLedger->ls();}
    void ListHistos(){mHistoLedger->ls();}
    
    // load all or one specific plot defined in conf file
    void LoadAvailablePlots(string configFileName){}
    // TODO gibts schon...
    void LoadPlotTemplate(string plotName, string configFileName){}

    void CreatePlot(Plot &plot, string canvasStyleName = "");
    void CreatePlot(string name, string figureGroup, string canvasStyleName = "");
    void CreatePlots(string figureGroup = "");
    
    void AddPlotToConfig(Plot &myPlot){}
    void RemovePlotFromConfig(string plotName){}
    void DumpPlots(string fileName);
    void AdjustLableOffsets();
    
    TGraphErrors* GetGraphClone(string graphName);
    TH1* GetHistClone(string histName);
    TH1* GetRatio(string ratioName);
    
    // setters to change global style
    void SetTextSize(int size){}
    void SetPixelBase(int size = 710){mStyle.pixelBaseLength = size;}
    
    void SetAxes(TPad* pad, Plot &plot);
    void SetTransparentCanvas(bool transparent = true);
    
    void ListAvailableCanvasStyles();
    
    void SetDrawTimestamps(bool drawTimestamps = true) {mStyle.drawTimestamps = drawTimestamps;}
    
    bool ContainsDatasets(std::initializer_list<string> requiredDatasets);
    void CutHistogram(TH1* hist, double cutoff, double cutoffLow);
    
    void PrintErrors(bool printMissingPlots = false);

    void AddInputFilePaths(string inputIdentifier, vector<string> inputFilePathList);
    void DumpInputFiles(string configFileName);
    void LoadInputFiles(string configFileName);

    void AddPlot(Plot& plot) {
      mSavedPlots.push_back(std::move(plot));
      for(auto& hist : plot.GetHistoTemplates()){
        mRequiredInput[hist.inputIdentifier].insert(hist.name);
      }
        
      for(auto& reqInput : mRequiredInput){
        cout << "InputID: " << reqInput.first << endl;
        cout << "Content:" << endl;
        for(auto& setContent : reqInput.second){
          cout << "   - " << setContent << endl;
        }
      }
      
    }

  private:

    void OpenInputFiles(string inputIdentifier);
    void OpenRequiredFiles(Plot& plot);
    map<string, vector<string>> mInputFiles; // inputFileIdentifier, inputFilePaths
    map<string, set<string>> mRequiredInput;
    map<string, set<string>> mLoadeddInput;

    
    vector<Plot> mSavedPlots;

    GlobalStyle mStyle;
    void DefineDefaultPlottingStyle();
    void SetStyle(TCanvas* canvas);
    
    vector<CanvasStyle> mCanvStyles;
    CanvasStyle* GetCanvasStyle(string styleName);
    
    TCanvas* MakeCanvas(string name, CanvasStyle& canvasStyle);
    void ApplyStyleSettings(CanvasStyle& canvasStyle, TCanvas* canvas, string controlString);
    
    TObjArray* mHistoLedger;   ///<!  Container to store input histograms
    TObjArray* mPlotLedger;    ///<!  Container to store output plots
    string mOutputDirectory;  ///<!  Directory to store output
    // actually vector<Plot>
    
    vector<string> mDatasetIdentifiers;
    
    vector<string> mListOfMissingFiles;
    vector<string> mListOfMissingPlots;
    vector<string> mListOfMissingHistograms;
        
    // Bookkeeping functions
    void BookHistos(TDirectoryFile& folder, string inputIdentifier);
    //   void RemoveHisto(string histName);
    
    bool IsPlotPossible(Plot &plot);
    
    void SetGlobalStyle();
    void DrawPlotInCanvas(Plot &plot, TCanvas* canvas);
    
    // Helper functions
    // string GetDatasetSuffix(TH1* hist, string inputIdentifier);
    TH1* DivideWithTSpline(TH1* numerator, TH1* denominator);
    void ApplyHistoSettings(TH1* histo, Plot::Histogram &histoTemplate, string &drawingOptions, int defaultValueIndex, string controlString);
    
    PlotManager(const PlotManager&) = default;
    PlotManager& operator=(const PlotManager&) = default;
  };
}
#endif
