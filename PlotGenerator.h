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

  // collection of functionality to generate actual plot
  class PlotGenerator
  {
  public:
    static shared_ptr<TCanvas> GeneratePlot(Plot& plot, PlotStyle& plotStyle, TObjArray* availableData);

  private:
    template<typename T>
    static T* GetDataClone(string dataName, TObjArray* availableData);
    static TLegend* MakeLegend(shared_ptr<Plot::LegendBox> legendBox, TPad* pad, TObjArray& legendEntries, vector<string> legendTitles, vector<string>& errorStyles);
    static TPaveText* MakeText(shared_ptr<Plot::TextBox> textBox);
    static void CutHistogram(TH1* hist, double cutoff, double cutoffLow);
    static void CutGraph(TGraph* graph, double cutoff, double cutoffLow);
    static TGraph* DivideTSpline(TGraph* numerator, TGraph* denominator);
    static TH1* DivideTSpline(TH1* numerator, TH1* denominator);
  };

  
  
  
  
}

#endif /* PlotGenerator_h */
