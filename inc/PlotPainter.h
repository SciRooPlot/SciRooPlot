// PlottingFramework
//
// Copyright (C) 2019-2021  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see doc/CONTRIBUTORS.md
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

#include "Plot.h"
class TH1;
class TH2;
class TGraph;
class TGraph2D;
class TProfile;
class TProfile2D;
class TF1;
class TF2;
class TCanvas;
class TObject;
class TLatex;
class TPad;
class TObjArray;
class TPave;
class TAxis;

namespace PlottingFramework
{

// supported input data types
using data_ptr_t = variant<TH1*, TH2*, TGraph*, TGraph2D*, TProfile*, TProfile2D*, TF2*, TF1*>;
using data_ptr_t_1d = variant<TH1*, TGraph*, TProfile*, TF1*>;
using data_ptr_t_2d = variant<TH2*, TGraph2D*, TProfile2D*, TF2*>;
using data_ptr_t_hist = variant<TH1*, TH2*, TProfile*, TProfile2D*>;
using data_ptr_t_hist_1d = variant<TH1*, TProfile*>;
using data_ptr_t_hist_2d = variant<TH2*, TProfile2D*>;
using data_ptr_t_graph = variant<TGraph*, TGraph2D*>;
using data_ptr_t_graph_1d = variant<TGraph*>;
using data_ptr_t_graph_2d = variant<TGraph2D*>;
using data_ptr_t_func = variant<TF1*, TF2*>;
using data_ptr_t_func_1d = variant<TF1*>;
using data_ptr_t_func_2d = variant<TF2*>;

const map<drawing_options_t, string> defaultDrawingOptions_Hist2d{
  {colz, "COLZ"},
  {surf, "SURF"},
  {cont, "CONT3"},
  {text, "TEXT"},
};

const map<drawing_options_t, string> defaultDrawingOptions_Hist{
  {points, "X0 EP"},
  {points_xerr, "EP"},
  {points_endcaps, "E1"},
  {curve, "HIST C"},
  {line, "HIST L"},
  {bar, "HIST B"},
  {boxes, "E2"},
  {band, "E6"},
  {band_line, "E5"},
  {area, "HIST F"},
  {area_curve, "HIST CF"},
  {area_line, "HIST LF"},
  {hist, "HIST"},
  {bar, "HIST B"},
  {hist_no_borders, "HIST ]["},
  {stars, "*H"},
  {text, "TEXT"},
};

const map<drawing_options_t, string> defaultDrawingOptions_Graph{
  {points, "P Z"},
  {points_line, "P Z L"},
  {points_endcaps, "P"},
  {curve, "X C"},
  {line, "X L"},
  {bar, "X B"},
  {boxes, "P2"},
  {band, "4"},
  {band_line, "3"},
  {area, "X CF"},
  {area_line, "X LC"},
  {boxes_only, "2"},
  {brackets, "[]"},
};

//**************************************************************************************************
/**
 * Class that contains functionality to generate plots using the ROOT framework.
 */
//**************************************************************************************************
class PlotPainter
{
public:
  shared_ptr<TCanvas> GeneratePlot(Plot& plot, const unordered_map<string, unordered_map<string, std::unique_ptr<TObject>>>& dataBuffer);

private:
  optional<data_ptr_t> GetDataClone(TObject* obj, const std::optional<Plot::Pad::Data::proj_info_t>& projInfo = std::nullopt);
  template <typename T>
  optional<data_ptr_t> GetDataClone(TObject* obj);
  template <typename T, typename Next, typename... Rest>
  optional<data_ptr_t> GetDataClone(TObject* obj);
  optional<data_ptr_t> GetProjection(TObject* obj, Plot::Pad::Data::proj_info_t projInfo);

  void SetGraphRange(TGraph* graph, optional<double_t> min, optional<double_t> max);
  void ScaleGraph(TGraph* graph, double_t scale);
  void SmoothGraph(TGraph* graph, optional<double_t> min = std::nullopt, optional<double_t> = std::nullopt);
  void SmoothHist(TH1* hist, optional<double_t> min = std::nullopt, optional<double_t> max = std::nullopt);
  bool DivideGraphs(TGraph* numerator, TGraph* denominator);
  void DivideGraphsInterpolated(TGraph* numerator, TGraph* denominator);
  void DivideHistosInterpolated(TH1* numerator, TH1* denominator);
  void DivideHistGraphInterpolated(TH1* numerator, TGraph* denominator);
  void DivideGraphHistInterpolated(TGraph* numerator, TH1* denominator);
  std::tuple<uint32_t, uint32_t> GetTextDimensions(TLatex& text);
  void ReplacePlaceholders(string& str, TNamed* data_ptr);
  TPave* GenerateBox(variant<shared_ptr<Plot::Pad::LegendBox>, shared_ptr<Plot::Pad::TextBox>> box, TPad* pad);
  float_t GetTextSizePixel(float_t textSizeNDC);

  template <typename T>
  TAxis* GetAxis(T* histPtr, int16_t i);
  std::string GetAxisStr(int16_t i);

  vector<int16_t> GenerateGradientColors(int32_t nColors, const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha = 1., bool savePalette = false);
};
} // end namespace PlottingFramework
#endif /* PlotGenerator_h */
