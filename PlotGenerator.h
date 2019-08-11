#ifndef PlotCreator_h
#define PlotCreator_h


// root headers
#include "TFile.h"
#include "TIterator.h"
#include "TKey.h"
#include "TObjArray.h"
#include "TObject.h"

#include "TStyle.h"
#include "TGraph.h"
#include "TF1.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "THn.h"
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

// std headers
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
  using std::array;
 
  
  // instanciate canvas from template and plot specifications
  // collection of functionality, only used to encapsulate
  // returns canvas
  // at this point manager has loaded all necessary input data and checked that plot style exists and matches plot properties
  class PlotGenerator
  {
  public:
    TCanvas* GeneratePlot(Plot& plot, PlotStyle& plotStyle) {};
    
  private:
    void MakeTextBox(){};
    
    TPaveText* MakeText(Plot::TextBox& textBoxTemplate);
    TLegend* MakeLegend(TPad* pad, Plot::TextBox& legendBoxTemplate, vector<Plot::Histogram>& histos);
    TGraphErrors* GetGraphClone(string graphName);
    TH1* GetHistClone(string histName);
    TH1* GetRatio(string ratioName);
    TCanvas* MakeCanvas(string name, CanvasStyle& canvasStyle);
    void ApplyStyleSettings(CanvasStyle& canvasStyle, TCanvas* canvas, string controlString);
    void DrawPlotInCanvas(Plot &plot, TCanvas* canvas);
    
    TH1* DivideWithTSpline(TH1* numerator, TH1* denominator);
    void ApplyHistoSettings(TH1* histo, Plot::Histogram &histoTemplate, string &drawingOptions, int defaultValueIndex, string controlString);
    
    
    // functions that implement properties stored in plots
    void SetAxes(TPad* pad, Plot &plot);
    //setglobalstyle
  };
  
  
  
  
  
  
}

#endif /* PlotCreator_h */
