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

    static shared_ptr<TCanvas> GeneratePlot(Plot& plot, PlotStyle& plotStyle, TObjArray* availableData);

  private:

    template<typename T>
    static T* GetDataClone(string dataName, TObjArray* availableData);
    static TLegend* MakeLegend(shared_ptr<Plot::LegendBox> legendBox, TPad* pad, TObjArray& legendEntries, vector<string> legendTitles, vector<string>& errorStyles);
    static TPaveText* MakeText(shared_ptr<Plot::TextBox> textBox);
    static void CutHistogram(TH1* hist, double cutoff, double cutoffLow);
    static void CutGraph(TGraph* graph, double cutoff, double cutoffLow);
    static TGraph* DivideTSpline(TGraph* numerator, TGraph* denominator);
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
