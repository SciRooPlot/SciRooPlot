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

// ROOT drawing options behind the aliases, per data type (an alias missing here cannot be drawn for that type)
inline const std::map<drawing_options_t, std::string> defaultDrawingOptions_Hist2d{
  {col, "COL"},
  {colz, "COLZ"},
  {box, "BOX"},
  {text, "TEXT"},
  {lego, "LEGO1 0"},
  {lego_col, "LEGO2 0"},
  {lego_colz, "LEGO2Z 0"},
  {lego_noborders, "LEGO3 0"},
  {surf, "SURF"},
  {surf_col, "SURF1"},
  {surf_colz, "SURF1Z"},
  {surf_fill, "SURF2"},
  {surf_fillz, "SURF2Z"},
  {surf_contours, "SURF3"},
  {surf_shaded, "SURF4"},
  {cont, "CONT3"},
  {cont_col, "CONT1"},
  {cont_colz, "CONT1Z"},
  {cont_fill, "CONT0"},
  {cont_fillz, "CONTZ"},
  {cont_smooth, "CONT4"},
  {cont_smoothz, "CONT4Z"},
  {candle, "CANDLEX2"},                  // box, median, whiskers up to 1.5 IQR, outliers
  {candle_minmax, "CANDLEX1"},           // whiskers up to the extreme values, mean as circle
  {candle_mean, "CANDLEX3"},             // candle with the mean as circle
  {candle_notched, "CANDLEX4"},          // candle_mean with the uncertainty of the median
  {candle_points, "CANDLEX5"},           // candle with all points
  {candle_scatter, "CANDLEX6"},          // candle with all points scattered
  {candle_meanline, "CANDLEX(111101)"},  // box, mean as line (no median), whiskers up to the extreme values, outliers
  {violin, "VIOLINX2"},
  {violin_minmax, "VIOLINX1"},
};

inline const std::map<drawing_options_t, std::string> defaultDrawingOptions_Hist{
  {points, "X0 E P"},
  {points_xerr, "E P"},
  {points_endcaps, "X0 E1 P"},
  {points_xerr_endcaps, "E1 P"},
  {points_text, "X0 E P"},  // the values are written above the error bars
  {line, "HIST L"},
  {curve, "HIST C"},
  {hist, "HIST"},
  {hist_open, "HIST ]["},
  {hbars, "HIST HBAR"},
  {area, "HIST LF2"},
  {area_smooth, "HIST CF"},
  {band, "E5"},
  {band_smooth, "E6"},
  {boxes, "E2"},
  {boxes_nomarkers, "E2"},  // the markers are removed before drawing
  {text, "HIST TEXT"},
};
// aliases missing for one of the 1d types are drawn by converting to the other type (bars: graph bars with gaps, text: histogram with the values)

// the x errors of graphs are removed before drawing for all points aliases without _xerr (as for histograms with X0),
// and area is drawn as the polygon between the points and zero, with its outline (F alone would close the polygon from the last point to the first)
inline const std::map<drawing_options_t, std::string> defaultDrawingOptions_Graph{
  {points, "P Z"},
  {points_xerr, "P Z"},
  {points_endcaps, "P"},
  {points_xerr_endcaps, "P"},
  {points_line, "P Z L"},
  {points_text, "P Z"},  // the values are written above the error bars
  {points_arrows, "P Z |>"},
  {line, "X L"},
  {curve, "X C"},
  {bars, "X B"},
  {area, "LF"},
  {band, "3"},
  {band_smooth, "4"},
  {boxes, "P2"},
  {boxes_nomarkers, "2"},
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

  static TGraph* ToGraph(TH1* hist);
  static TH1* ToHist(TGraph* graph, bool warn = true);
  bool CheckFontSizes(TList* list);
  std::tuple<uint32_t, uint32_t> GetTextDimensions(TLatex& text, TPad* pad);
  void ReplacePlaceholders(std::string& str, TNamed* data_ptr);
  static std::string LegendDrawStyle(TObject* obj, std::string option);
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
