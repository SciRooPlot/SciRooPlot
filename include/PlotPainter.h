/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot -------------------------------------
 * Copyright (c) 2019-2026 Mario Krüger
 * Contact: mario.kruger@cern.ch
 * For a full list of contributors please see doc/CONTRIBUTORS.md.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation in version 3 (or later) of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef INCLUDE_PLOTPAINTER_H_
#define INCLUDE_PLOTPAINTER_H_

#include "Plot.h"
#include <unordered_map>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

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

namespace SciRooPlot
{

// supported input data types
using data_ptr_t = std::variant<TH1*, TH2*, TGraph*, TGraph2D*, TProfile*, TProfile2D*, TF2*, TF1*>;

const std::map<drawing_options_t, std::string> defaultDrawingOptions_Hist2d{
  {colz, "COLZ"},
  {surf, "SURF"},
  {cont, "CONT3"},
  {text, "TEXT"},
  {candle1, "CANDLEX1"},
  {candle2, "CANDLEX2"},
  {candle3, "CANDLEX3"},
  {candle4, "CANDLEX4"},
  {candle5, "CANDLEX5"},
  {candle6, "CANDLEX6"},
  {candle7, "CANDLEX(111101)"},  // no median but mean as line
};

const std::map<drawing_options_t, std::string> defaultDrawingOptions_Hist{
  {points, "X0 EP"},
  {points_xerr, "EP"},
  {points_endcaps, "E1"},
  {curve, "HIST C"},
  {line, "HIST L"},
  {bar, "HIST B"},
  {hbar, "HIST HBAR"},
  {boxes, "E2"},
  {band, "E5"},
  {band_smooth, "E6"},
  {area, "HIST F"},
  {area_curve, "HIST CF"},
  {area_line, "HIST LF"},
  {hist, "HIST"},
  {hist_no_borders, "HIST ]["},
  {stars, "*H"},
  {text, "TEXT"},
  {hbar, "HBAR ]["},
  {hbar1, "HBAR1"},
  {hbar2, "HBAR2"},
  {hbar3, "HBAR3"},
  {hbar4, "HBAR4"},
};

const std::map<drawing_options_t, std::string> defaultDrawingOptions_Graph{
  {points, "P Z"},
  {points_line, "P Z L"},
  {points_endcaps, "P"},
  {curve, "X C"},
  {line, "X L"},
  {bar, "X B"},
  {boxes, "P2"},
  {band, "3"},
  {band_smooth, "4"},
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
  std::unique_ptr<TCanvas> GeneratePlot(Plot& plot, const std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<TObject>>>& dataBuffer);

 private:
  std::optional<data_ptr_t> GetDataClone(TObject* obj, const std::optional<Plot::Pad::Data::proj_info_t>& projInfo = std::nullopt);
  template <typename T>
  std::optional<data_ptr_t> GetDataClone(TObject* obj);
  template <typename T, typename Next, typename... Rest>
  std::optional<data_ptr_t> GetDataClone(TObject* obj);
  std::optional<data_ptr_t> GetProjection(TObject* obj, Plot::Pad::Data::proj_info_t projInfo);

  void SetGraphRange(TGraph* graph, std::optional<double_t> min, std::optional<double_t> max);
  void ScaleGraph(TGraph* graph, double_t scale);
  void SmoothGraph(TGraph* graph, std::optional<double_t> min = std::nullopt, std::optional<double_t> = std::nullopt);
  void SmoothHist(TH1* hist, std::optional<double_t> min = std::nullopt, std::optional<double_t> max = std::nullopt);
  bool DivideGraphs(TGraph* numerator, TGraph* denominator);
  void DivideGraphsInterpolated(TGraph* numerator, TGraph* denominator);
  void DivideHistosInterpolated(TH1* numerator, TH1* denominator);
  void DivideHistGraphInterpolated(TH1* numerator, TGraph* denominator);
  void DivideGraphHistInterpolated(TGraph* numerator, TH1* denominator);
  std::tuple<uint32_t, uint32_t> GetTextDimensions(TLatex& text, TPad* pad);
  void ReplacePlaceholders(std::string& str, TNamed* data_ptr);
  TPave* GenerateBox(std::variant<std::shared_ptr<Plot::Pad::LegendBox>, std::shared_ptr<Plot::Pad::TextBox>> box, TPad* pad);
  float_t GetTextSizePixel(float_t textSizeNDC);

  template <typename T>
  TAxis* GetAxis(T* histPtr, int16_t i);
  std::string GetAxisStr(int16_t i);

  std::vector<int16_t> GenerateGradientColors(int32_t nColors, const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha = 1., bool savePalette = false);
};
}  // end namespace SciRooPlot
#endif  // INCLUDE_PLOTPAINTER_H_
