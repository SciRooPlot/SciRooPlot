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
 * The GNU General Public License can be found here: <https://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef SRC_PLOTPAINTER_H_
#define SRC_PLOTPAINTER_H_

#include "SciRooPlot/Plot.h"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

class TH1;
class TH2;
class TH3;
class TGraph;
class TGraph2D;
class TProfile;
class TProfile2D;
class TF1;
class TF2;
class TF3;
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
using data_ptr_t = std::variant<TH1*, TH2*, TH3*, TGraph*, TGraph2D*, TProfile*, TProfile2D*, TF3*, TF2*, TF1*>;

inline const std::map<drawing_options_t, std::string> defaultDrawingOptions_Hist2d{
  {box, "BOX"},
  {box1, "BOX1"},
  {colz, "COLZ"},
  {lego, "LEGO1 0"},
  {lego_no_borders, "LEGO3 0"},
  {legoz, "LEGO2Z 0"},
  {surf, "SURF"},
  {surf1, "SURF1"},
  {surf1z, "SURF1Z"},
  {surf2, "SURF2"},
  {surf2z, "SURF2Z"},
  {surf3, "SURF3"},
  {surf3z, "SURF3Z"},
  {surf4, "SURF4"},
  {surf7, "SURF7"},
  {surf7z, "SURF7Z"},
  {cont, "CONT3"},
  {contz, "CONTZ"},
  {cont1z, "CONT1Z"},
  {cont4z, "CONT4Z"},
  {text, "TEXT"},
  {candle1, "CANDLEX1"},
  {candle2, "CANDLEX2"},
  {candle3, "CANDLEX3"},
  {candle4, "CANDLEX4"},
  {candle5, "CANDLEX5"},
  {candle6, "CANDLEX6"},
  {candle7, "CANDLEX(111101)"},  // no median but mean as line
};

inline const std::map<drawing_options_t, std::string> defaultDrawingOptions_Hist{
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
  {hbar_no_borders, "HBAR ]["},
  {hbar1, "HBAR1"},
  {hbar2, "HBAR2"},
  {hbar3, "HBAR3"},
  {hbar4, "HBAR4"},
};

inline const std::map<drawing_options_t, std::string> defaultDrawingOptions_Graph{
  {points, "P Z"},  // x errors are removed before drawing (as for histograms)
  {points_xerr, "P Z"},
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
  // scale: factor for the canvas size and all absolute sizes (line widths, marker sizes, pixel fonts),
  //        i.e. the plot looks the same at any scale, only with more or less pixels (used on screen and for bitmap files)
  explicit PlotPainter(double_t scale = 1.) : mScale(scale) {}
  std::unique_ptr<TCanvas> GeneratePlot(Plot& plot, const std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<TObject>>>& dataBuffer);

 private:
  void ApplyScale(TObject* obj, std::set<TObject*>& done, const std::string& drawOption = "");
  Width_t ScaleLineWidth(Width_t width) const;
  static TH1* ExtendFrameAxis(const TH1* frame, char axisLabel, std::optional<double_t> newMin, std::optional<double_t> newMax);
  std::optional<data_ptr_t> GetDataClone(TObject* obj, const std::optional<Plot::Pad::Data::proj_info_t>& projInfo = std::nullopt);
  template <typename T>
  std::optional<data_ptr_t> GetDataClone(TObject* obj);
  template <typename T, typename Next, typename... Rest>
  std::optional<data_ptr_t> GetDataClone(TObject* obj);
  std::optional<data_ptr_t> GetProjection(TObject* obj, Plot::Pad::Data::proj_info_t projInfo);

  static void RemoveGraphPointsOutside(TGraph* graph, std::optional<double_t> minX, std::optional<double_t> maxX, std::optional<double_t> minY, std::optional<double_t> maxY);
  void ScaleGraphAxis(TGraph* graph, int16_t axis, double_t scaleFactor);
  void ScaleGraphAxis(TGraph2D* graph, int16_t axis, double_t scaleFactor);
  void ScaleAxis(TH1* hist, int16_t axisIndex, double_t scaleFactor);
  TF1* ScaleFunc(TF1* func, double_t domainFactorX, double_t contentFactor);
  TF2* ScaleFunc(TF2* func, double_t domainFactorX, double_t domainFactorY, double_t contentFactor);
  TF3* ScaleFunc(TF3* func, double_t domainFactorX, double_t domainFactorY, double_t domainFactorZ, double_t contentFactor);

  bool Divide(TGraph* numerator, TGraph* denominator, bool binomialErrors = false);
  bool Divide(TH1* numerator, TH1* denominator, bool binomialErrors = false);
  bool Divide(TH1* numerator, TGraph* denominator, bool binomialErrors = false);
  bool Divide(TGraph* numerator, TH1* denominator, bool binomialErrors = false);
  bool Divide(TH1* numerator, TF1* denominator, bool binomialErrors = false);
  bool Divide(TGraph* numerator, TF1* denominator, bool binomialErrors = false);

  bool CheckFontSizes(TList* list);
  std::tuple<uint32_t, uint32_t> GetTextDimensions(TLatex& text, TPad* pad);
  void ReplacePlaceholders(std::string& str, TNamed* data_ptr);
  TPave* GenerateBox(std::variant<std::shared_ptr<Plot::Pad::LegendBox>, std::shared_ptr<Plot::Pad::TextBox>> box, TPad* pad);
  bool FindFreeSpace(TPad* pad, const std::array<double_t, 4>& freeArea, double_t width, double_t height, box_placement_t placement, double_t& lowerLeftX, double_t& lowerLeftY);
  float_t GetTextSizePixel(float_t textSizeNDC);

  template <typename T>
  TAxis* GetAxis(T* histPtr, int16_t i);
  std::string GetAxisStr(int16_t i);

  std::vector<int16_t> GenerateGradientColors(int32_t nColors, const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha = 1.);
  static void CollectCustomColors(TObject* obj, std::set<int32_t>& colors);
  static std::string ColorDefinitionCommand(const std::set<int32_t>& colors);
  static std::string GradientPaletteCommand(int32_t nColors, const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha);

  double_t mScale{1.};
  struct StyleSizes {
    Width_t lineWidth;
    int32_t hatchesLineWidth;
    float_t endErrorSize;
    std::vector<std::string> lineStyles;
  };
  static inline std::optional<StyleSizes> sUnscaledStyle;  // absolute sizes in gStyle before the scale of the last plot was applied
};
}  // end namespace SciRooPlot
#endif  // SRC_PLOTPAINTER_H_
