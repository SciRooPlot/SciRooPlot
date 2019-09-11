#ifndef PlotGenerator_h
#define PlotGenerator_h

#include "PlottingFramework.h"
//#include "PlotManager.h"
#include "PlotStyle.h"
#include "Plot.h"
using namespace PlottingFramework;
namespace PlottingFramework {

  // instanciate canvas from template and plot specifications
  // collection of functionality, only used to encapsulate
  // returns canvas
  // at this point manager has loaded all necessary input data and checked that plot style exists and matches plot properties
  
  // collection of functionality to generate actual plot
  class PlotGenerator
  {
  public:
    //static TCanvas* GeneratePlot(Plot& plot, PlotStyle& plotStyle) {return new TCanvas(plot.GetUniqueName().c_str(), "b");}

    static TCanvas* GeneratePlot(Plot& plot, PlotStyle& plotStyle, TObjArray* availableData);

  private:

    template<typename T>
    static T* GetDataClone(string dataName, TObjArray* availableData);
    static TLegend* MakeLegend(shared_ptr<Plot::LegendBox> legendBox, TPad* pad, TObjArray& legendEntries, vector<string> legendTitles, vector<string>& errorStyles);
    static TPaveText* MakeText(shared_ptr<Plot::TextBox> textBox);
    static void CutHistogram(TH1* hist, double cutoff, double cutoffLow);

    //void MakeTextBox(){};
    
    //TPaveText* MakeText(Plot::TextBox& textBoxTemplate);
    //TLegend* MakeLegend(TPad* pad, Plot::TextBox& legendBoxTemplate, vector<Plot::Histogram>& histos);
    //TGraphErrors* GetGraphClone(string graphName);
    //TH1* GetHistClone(string histName);
    //TH1* GetRatio(string ratioName);
    //TCanvas* MakeCanvas(string name, CanvasStyle& canvasStyle);
    //void ApplyStyleSettings(CanvasStyle& canvasStyle, TCanvas* canvas, string controlString);
    //void DrawPlotInCanvas(Plot &plot, TCanvas* canvas);
    
    //TH1* DivideWithTSpline(TH1* numerator, TH1* denominator);
    //void ApplyHistoSettings(TH1* histo, Plot::Histogram &histoTemplate, string &drawingOptions, int defaultValueIndex, string controlString);
    
    
    // functions that implement properties stored in plots
    //void SetAxes(TPad* pad, Plot &plot);
    //setglobalstyle
  };

  
  
  
  
}

#endif /* PlotGenerator_h */
