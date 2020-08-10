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

#ifndef PlotGenerator_h
#define PlotGenerator_h

#include "PlottingFramework.h"
#include "Plot.h"
using namespace PlottingFramework;
namespace PlottingFramework {

//**************************************************************************************************
/**
 * Class that contains functionality to generate plots using the ROOT framework.
 */
//**************************************************************************************************
class PlotPainter
{
public:
  shared_ptr<TCanvas> GeneratePlot(Plot& plot, TObjArray* availableData);
private:
  optional<data_ptr_t> GetDataClone(const string& dataName, TObjArray* availableData);
  template <typename T> optional<data_ptr_t> GetDataClone(TObject* obj);
  template <typename T, typename Next, typename... Rest> optional<data_ptr_t> GetDataClone(TObject* obj);
  
  void SetGraphRange(TGraph* graph, optional<double_t> min, optional<double_t> max);
  void ScaleGraph(TGraph* graph, double_t scale);
  void SmoothGraph(TGraph* graph, optional<double_t> min = std::nullopt, optional<double_t> = std::nullopt);
  void SmoothHist(TH1* hist, optional<double_t> min = std::nullopt, optional<double_t> max = std::nullopt);
  bool DivideGraphs(TGraph* numerator, TGraph* denominator);
  void DivideTSpline(TGraph* numerator, TGraph* denominator);
  void DivideTSpline(TH1* numerator, TH1* denominator);
  std::tuple<uint32_t, uint32_t> GetTextDimensions(TLatex& text);
  TPave* GenerateBox(variant<shared_ptr<Plot::Pad::LegendBox>, shared_ptr<Plot::Pad::TextBox>> box, TPad* pad, vector<string> lines, vector<TObject*> legendEntries);
};
} // end namespace PlottingFramework
#endif /* PlotGenerator_h */
