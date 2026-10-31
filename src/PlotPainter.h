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
#include <utility>
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
inline const std::map<draw_option_t, std::string> defaultDrawOptions_Hist2d{
  {col, "COL"},
  {colz, "COLZ"},
  {col_clamped, "COL0"},  // contents below the minimum of the colour scale in its lowest colour, not left out (as above the maximum)
  {colz_clamped, "COLZ0"},
  {colz_text, "COLZ TEXT"},  // the value written into each cell
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
  {col_polar, "COL POL"},
  {colz_polar, "COLZ POL"},
  {lego_polar, "LEGO1 0 POL"},
  {lego_col_polar, "LEGO2 0 POL"},
  {surf_polar, "SURF POL"},
  {surf_col_polar, "SURF1 POL"},
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

// 3d histograms are drawn in 3d: a box per cell with a volume following the content (box), all cells as equal boxes coloured by the
// content (col, colz), or the iso surface at the mean content of the cells (iso)
inline const std::map<draw_option_t, std::string> defaultDrawOptions_Hist3d{
  {box, "BOX"},
  {col, "BOX2"},
  {colz, "BOX2Z"},
  {iso, "ISO"},
};

inline const std::map<draw_option_t, std::string> defaultDrawOptions_Hist{
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
// functions are sampled at SetNpx points and drawn like a histogram of them: a fill set for them fills the area between the curve and zero
inline const std::map<draw_option_t, std::string> defaultDrawOptions_Func{
  {points, "P"},
  {line, "L"},
  {curve, "C"},
  {area, "L"},
  {area_smooth, "C"},
};
// aliases missing for one of the 1d types are drawn by converting to the other type (bars: graph bars with gaps, text: histogram with the values)

// the x errors of graphs are removed before drawing for all points aliases without _xerr (as for histograms with X0),
// and area is drawn as the polygon between the points and zero, with its outline (F alone would close the polygon from the last point to the first)
inline const std::map<draw_option_t, std::string> defaultDrawOptions_Graph{
  {points, "P Z"},
  {points_xerr, "P Z"},
  {points_endcaps, "P"},
  {points_xerr_endcaps, "P"},
  {points_line, "P Z L"},
  {points_text, "P Z"},  // the values are written above the error bars
  {points_arrows, "P Z |>"},
  {line, "X L"},
  {curve, "X C"},
  {curve_clipped, "X C"},    // histograms drawn as graph through the bin centres (a graph drawn as curve is cut at the frame anyway)
  {hist_clipped, "L"},       // histograms (also graphs converted to histograms) drawn as their steps
  {hist_open_clipped, "L"},  // without the vertical lines at the outer edges
  {bars, "X B"},
  {area, "LF"},
  {band, "3"},
  {band_smooth, "4"},
  {boxes, "P2"},
  {boxes_nomarkers, "2"},
  {brackets, "[]"},
};

// 2d graphs and 2d functions are drawn from a histogram interpolated (or sampled) on a grid, as 2d histograms; box and text would show
// the grid instead of the data, candles and violins a distribution that does not exist. Only graphs have their points and triangles:
// the Delaunay triangles as a filled surface (with the data's fill colour), as wire frame, or coloured by z, and the points as markers
// (points_col: coloured by z), all in 3d
inline const std::map<draw_option_t, std::string> defaultDrawOptions_Graph2d{
  {col, "COL"},
  {colz, "COLZ"},
  {col_clamped, "COL0"},
  {colz_clamped, "COLZ0"},
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
  {col_polar, "COL POL"},
  {colz_polar, "COLZ POL"},
  {lego_polar, "LEGO1 0 POL"},
  {lego_col_polar, "LEGO2 0 POL"},
  {surf_polar, "SURF POL"},
  {surf_col_polar, "SURF1 POL"},
  {cont, "CONT3"},
  {cont_col, "CONT1"},
  {cont_colz, "CONT1Z"},
  {cont_fill, "CONT0"},
  {cont_fillz, "CONTZ"},
  {cont_smooth, "CONT4"},
  {cont_smoothz, "CONT4Z"},
  {triangles, "TRI"},
  {triangles_wire, "TRIW"},
  {triangles_col, "TRI1"},
  {triangles_colz, "TRI1Z"},
  {triangles_fill, "TRI2"},
  {triangles_fillz, "TRI2Z"},
  {points, "P"},
  {points_col, "PCOL"},
  {points_colz, "PCOLZ"},
  {line, "LINE"},
};

inline const std::map<draw_option_t, std::string> defaultDrawOptions_Func2d{
  {col, "COL"},
  {colz, "COLZ"},
  {col_clamped, "COL0"},
  {colz_clamped, "COLZ0"},
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
  {col_polar, "COL POL"},
  {colz_polar, "COLZ POL"},
  {lego_polar, "LEGO1 0 POL"},
  {lego_col_polar, "LEGO2 0 POL"},
  {surf_polar, "SURF POL"},
  {surf_col_polar, "SURF1 POL"},
  {cont, "CONT3"},
  {cont_col, "CONT1"},
  {cont_colz, "CONT1Z"},
  {cont_fill, "CONT0"},
  {cont_fillz, "CONTZ"},
  {cont_smooth, "CONT4"},
  {cont_smoothz, "CONT4Z"},
};

// 3d functions are drawn as the surface f(x, y, z) = 0, ROOT's only drawing of them (TF3::Paint), as the iso surface of 3d histograms
inline const std::map<draw_option_t, std::string> defaultDrawOptions_Func3d{
  {iso, "TF3"},
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
  // onScreen: the canvas is shown in a window, whose X11 backend has fixed dash patterns for line styles 2-4
  explicit PlotPainter(double_t scale = 1., bool onScreen = false) : mScale(scale), mOnScreen(onScreen) {}
  std::unique_ptr<TCanvas> GeneratePlot(Plot& plot, const std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<TObject>>>& dataBuffer);

  // axes shared between pads (Plot::ShareAxis): the groups, and the axes of pads whose range is the common one of their group
  // (set explicitly in the plot by the caller: they get no further room, which the common range includes already)
  void SetSharedAxes(std::map<char, std::vector<std::vector<uint8_t>>> groups, std::set<std::pair<uint8_t, char>> commonRanges = {})
  {
    mSharedAxes = std::move(groups);
    mCommonRanges = std::move(commonRanges);
  }
  // the ranges of the frames as drawn (x and y of flat frames, z of 2d frames), keyed by pad and axis
  const std::map<std::pair<uint8_t, char>, std::pair<double_t, double_t>>& GetFrameRanges() const { return mFrameRanges; }

 private:
  void AddAxisRoom(TPad* pad, TH1* frame, const Plot::Pad& padDefaults, const Plot::Pad& padSettings, uint8_t padID, const Plot::Pad::Data* refFunc, std::string& padSetup);
  void ApplyScale(TObject* obj, std::set<TObject*>& done, const std::string& drawOption = "");
  Width_t ScaleLineWidth(Width_t width) const;
  Style_t ScaleMarkerStyle(Style_t style) const;
  Style_t ScreenLineStyle(Style_t style) const;
  static TH1* ExtendFrameAxis(const TH1* frame, char axisLabel, std::optional<double_t> newMin, std::optional<double_t> newMax);
  std::optional<data_ptr_t> GetDataClone(TObject* obj, const std::optional<Plot::Pad::Data::proj_info_t>& projInfo = std::nullopt, bool keepStoredRanges = false);
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

  // warnings about the division are given only with verbose (the frame of a ratio pad is a second copy of the same division)
  bool Divide(TGraph* numerator, TGraph* denominator, bool binomialErrors = false, bool verbose = true);
  bool Divide(TH1* numerator, TH1* denominator, bool binomialErrors = false, bool verbose = true);
  bool Divide(TH1* numerator, TGraph* denominator, bool binomialErrors = false, bool verbose = true);
  bool Divide(TGraph* numerator, TH1* denominator, bool binomialErrors = false, bool verbose = true);
  bool Divide(TH1* numerator, TF1* denominator, bool binomialErrors = false, bool verbose = true);
  bool Divide(TGraph* numerator, TF1* denominator, bool binomialErrors = false, bool verbose = true);

  static TGraph* ToGraph(const TH1* hist);
  static TH1* ToHist(TGraph* graph, bool warn = true);
  static TGraph* ToClippedGraph(const TH1* hist, draw_option_t alias, bool logY);
  static double_t ClippedBase(const TGraph* graph, bool logY);
  static void SetBinRange(TAxis* axis, const std::optional<double_t>& min, const std::optional<double_t>& max);
  static std::pair<int32_t, int32_t> BinRange(const TAxis* axis, const std::optional<double_t>& min, const std::optional<double_t>& max);
  static void LimitToFrame(TAxis* axis, const TAxis* frameAxis);
  bool CheckFontSizes(TList* list);
  std::tuple<uint32_t, uint32_t> GetTextDimensions(TLatex& text, TPad* pad);
  void HideClippedLabels(TPad* pad, TAxis* axis, char axisLabel);
  static std::vector<double_t> LabelValues(TPad* pad, const TAxis* axis, char axisLabel);
  double_t MaxLabelWidth(TPad* pad, const TAxis* axis, char axisLabel);
  void DrawColorScaleCell(TCanvas* canvas, Plot& plot, uint8_t padID);
  void DrawCommonTitle(TCanvas* canvas, char axisLabel, const std::vector<uint8_t>& pads, size_t index);
  void ReplacePlaceholders(std::string& str, TNamed* data_ptr);
  static std::string LegendSymbolElements(TObject* obj, std::string option);
  static bool HasErrors(const TObject* obj);
  void DrawImage(const Plot::Pad::ImageBox& box, TPad* pad, const std::string& name);
  TPave* GenerateBox(std::variant<std::shared_ptr<Plot::Pad::LegendBox>, std::shared_ptr<Plot::Pad::TextBox>> box, TPad* pad);
  void DrawShapes(TPad* pad, const Plot::Pad& padSettings, const Plot::Pad& padDefaults, Plot::Pad::Shape::layer_t layer, TObject* insertAfter, uint8_t padID);
  bool FindFreeSpace(TPad* pad, const std::array<double_t, 4>& freeArea, double_t width, double_t height, box_placement_t placement, double_t& lowerLeftX, double_t& lowerLeftY);
  void FindBoxPosition(TPad* pad, double_t width, double_t height, box_placement_t placement, const std::string& boxType, double_t& lowerLeftX, double_t& lowerLeftY);
  template <typename BoxType>
  std::pair<double_t, double_t> PlaceBox(const BoxType& box, TPad* pad, double_t widthNDC, double_t heightNDC, double_t borderX, double_t borderY, const std::string& boxType);
  template <typename BoxType>
  static void ApplyBoxStyle(const BoxType& box, TPave* pave);
  float_t GetTextSizePixel(float_t textSizeNDC);

  template <typename T>
  TAxis* GetAxis(T* histPtr, int16_t i);
  std::string GetAxisStr(int16_t i);

  std::vector<int16_t> GenerateGradientColors(int32_t nColors, const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha = 1.);
  static void CollectCustomColors(TObject* obj, std::set<int32_t>& colors);
  static std::string ColorDefinitionCommand(const std::set<int32_t>& colors);
  static std::string GradientPaletteCommand(int32_t nColors, const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha);

  double_t mScale{1.};
  bool mOnScreen{};
  std::map<char, std::vector<std::vector<uint8_t>>> mSharedAxes;
  std::set<std::pair<uint8_t, char>> mCommonRanges;
  std::map<std::pair<uint8_t, char>, std::pair<double_t, double_t>> mFrameRanges;
  std::map<uint8_t, TH1*> mFrameHists;          // the frame histograms of the flat frames (owned by their pads)
  std::map<uint8_t, TH1*> mScaleHists;          // the frame histograms with a z axis, also in 3D views (owned by their pads)
  std::map<uint8_t, double_t> mDivisionWidths;  // per pad: the widest pad (fraction of the canvas) with a frame as wide as its frame
  std::set<uint8_t> mScalesInCells;             // pads whose colour scale a ZAxis() cell shows instead of the pad
  // on screen: the free user-defined line styles that stand in for styles 2-4 (their dash strings from gStyle), 0 if there is none
  std::array<Style_t, 3> mScreenDashStyles{};
  std::set<std::pair<const TObject*, size_t>> mReportedStoredRanges;  // input axes whose stored range a projection of this plot does not apply (reported once)
  // histograms drawn as graphs (by an alias only graphs have), with the graph they are drawn as: their statistics box and placeholders refer to the histogram
  std::unordered_map<const TObject*, std::unique_ptr<TH1>> mSourceHists;
  // definition of the drawn data (also for the histogram a graph is drawn from): for <name>, <source>, <projection> and <selection>
  std::unordered_map<const TObject*, std::shared_ptr<const Plot::Pad::Data>> mDataDefinitions;
  // axis titles of the input a projection was made from, keyed by the projection: <projection> and <selection> name the axes by them
  std::unordered_map<const TObject*, std::vector<std::string>> mProjectionAxisTitles;
  struct StyleSizes {
    Width_t lineWidth;
    int32_t hatchesLineWidth;
    float_t endErrorSize;
    Style_t gridStyle;
    std::vector<std::string> lineStyles;
  };
  static inline std::optional<StyleSizes> sUnscaledStyle;  // absolute sizes in gStyle before the scale of the last plot was applied
};
}  // end namespace SciRooPlot
#endif  // SRC_PLOTPAINTER_H_
