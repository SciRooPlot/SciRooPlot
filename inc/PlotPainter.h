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

#ifndef PlotGenerator_h
#define PlotGenerator_h

#include "PlottingFramework.h"
#include "Plot.h"
using namespace PlottingFramework;
namespace PlottingFramework {

//****************************************************************************************
/**
 * Class that contains functionality to generate generate plots using the ROOT framework.
 */
//****************************************************************************************
class PlotPainter
{
public:
  shared_ptr<TCanvas> GeneratePlot(Plot& plot, TObjArray* availableData);
  
private:
  
  optional<data_ptr_t> GetDataClone(string dataName, TObjArray* availableData);
  template <typename T> optional<data_ptr_t> GetDataClone(TObject* obj);
  template <typename T, typename Next, typename... Rest> optional<data_ptr_t> GetDataClone(TObject* obj);
  
  void SetGraphRange(TGraph* graph, optional<double_t> min, optional<double_t> max);
  void ScaleGraph(TGraph* graph, double_t scale);
  // internal helpers
  TLegend* MakeLegend(shared_ptr<Plot::Pad::LegendBox> legendBox, TPad* pad, TObjArray& legendEntries, vector<string> legendTitles, vector<string>& errorStyles);
  TPaveText* MakeText(shared_ptr<Plot::Pad::TextBox> textBox);
  TGraph* DivideTSpline(TGraph* numerator, TGraph* denominator);
  TH1* DivideTSpline(TH1* numerator, TH1* denominator);
  
  int16_t GetDefaultColor(uint8_t colorID) {return mDefaultColors[colorID % mDefaultColors.size()];}
  int16_t GetDefaultMarker(uint8_t markerID)  {return mDefaultColors[markerID % mDefaultColors.size()];}
  
  vector<int16_t> mDefaultColors = {kBlack, kBlue+1, kRed+1, kYellow+1,
    kMagenta-4, kGreen+3, kOrange+1, kViolet-3, kCyan+2, kPink+3, kTeal-7, kMagenta+1, kPink+8, kCyan-6, kMagenta, kRed+2, kGreen+2,
    kOrange+2, kMagenta+2, kYellow+3, kGray+2, kBlue+2, kYellow+2, kRed, kBlue, kMagenta+3, kGreen+4, 28, 8, 15, 17, 12};

  vector<int16_t> mDefaultMarkers = {kFullCircle, kFullSquare, kFullDiamond, kFullCross,
    kFullStar, kOpenCircle, kOpenSquare, kOpenCross,
    kOpenDiamond, kOpenStar};

};
} // end namespace PlottingFramework
#endif /* PlotGenerator_h */
