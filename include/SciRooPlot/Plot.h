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

#ifndef INCLUDE_SCIROOPLOT_PLOT_H_
#define INCLUDE_SCIROOPLOT_PLOT_H_

// make root style property enums available to user
#include <Rtypes.h>
#include <TAttFill.h>
#include <TAttLine.h>
#include <TAttMarker.h>
#include <TAttText.h>
#include <TCandle.h>
#include <TColor.h>
#include <TF1.h>

#include <boost/property_tree/ptree.hpp>

#include <array>
#include <cstddef>
#include <deque>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace SciRooPlot
{
// aliases for the drawing options of ROOT: the base word names the appearance, suffixes add one feature each
// (1d: _xerr also draws the x errors, _endcaps draws end caps on the error bars, _smooth smooths, _nomarkers omits the markers;
//  2d: _col colours the lines by the contents, _fill fills by the contents, a trailing z also draws the colour axis)
// an alias that a data type cannot draw is reported and the data is drawn with the default of ROOT
#define DRAW_OPTIONS       \
  OPT(points, 0)           \
  OPT(points_xerr)         \
  OPT(points_endcaps)      \
  OPT(points_xerr_endcaps) \
  OPT(points_line)         \
  OPT(points_text)         \
  OPT(points_arrows)       \
  OPT(points_col)          \
  OPT(points_colz)         \
  OPT(line)                \
  OPT(curve)               \
  OPT(curve_clipped)       \
  OPT(hist)                \
  OPT(hist_open)           \
  OPT(hist_clipped)        \
  OPT(hist_open_clipped)   \
  OPT(bars)                \
  OPT(hbars)               \
  OPT(area)                \
  OPT(area_smooth)         \
  OPT(band)                \
  OPT(band_smooth)         \
  OPT(boxes)               \
  OPT(boxes_nomarkers)     \
  OPT(brackets)            \
  OPT(text)                \
  OPT(col)                 \
  OPT(colz)                \
  OPT(colz_text)           \
  OPT(col_clamped)         \
  OPT(colz_clamped)        \
  OPT(box)                 \
  OPT(iso)                 \
  OPT(lego)                \
  OPT(lego_col)            \
  OPT(lego_colz)           \
  OPT(lego_noborders)      \
  OPT(surf)                \
  OPT(surf_col)            \
  OPT(surf_colz)           \
  OPT(surf_fill)           \
  OPT(surf_fillz)          \
  OPT(surf_contours)       \
  OPT(surf_shaded)         \
  OPT(col_polar)           \
  OPT(colz_polar)          \
  OPT(lego_polar)          \
  OPT(lego_col_polar)      \
  OPT(surf_polar)          \
  OPT(surf_col_polar)      \
  OPT(triangles)           \
  OPT(triangles_wire)      \
  OPT(triangles_col)       \
  OPT(triangles_colz)      \
  OPT(triangles_fill)      \
  OPT(triangles_fillz)     \
  OPT(cont)                \
  OPT(cont_col)            \
  OPT(cont_colz)           \
  OPT(cont_fill)           \
  OPT(cont_fillz)          \
  OPT(cont_smooth)         \
  OPT(cont_smoothz)        \
  OPT(candle)              \
  OPT(candle_minmax)       \
  OPT(candle_mean)         \
  OPT(candle_notched)      \
  OPT(candle_points)       \
  OPT(candle_scatter)      \
  OPT(candle_meanline)     \
  OPT(violin)              \
  OPT(violin_minmax)
enum draw_option_t : uint8_t {
#define OPT(name, ...) name,
  DRAW_OPTIONS
#undef OPT
};
// the aliases are stored by name
inline std::string draw_option_name(draw_option_t option)
{
  switch (option) {
#define OPT(name, ...) \
  case name:           \
    return #name;
    DRAW_OPTIONS
#undef OPT
  }
  return "";
}
inline std::optional<draw_option_t> draw_option_from_name(const std::string& name)
{
#define OPT(optName, ...) \
  if (name == #optName) return optName;
  DRAW_OPTIONS
#undef OPT
  return std::nullopt;
}

// automatic placement of a legend or text: as close as possible to the corner of the frame where it fits best or to the given corner
#define BOX_PLACEMENTS    \
  PLACEMENT(best_corner)  \
  PLACEMENT(top_left)     \
  PLACEMENT(top_right)    \
  PLACEMENT(bottom_right) \
  PLACEMENT(bottom_left)
enum box_placement_t : uint8_t {
#define PLACEMENT(name) name,
  BOX_PLACEMENTS
#undef PLACEMENT
};
// the placements are stored by name
inline std::string box_placement_name(box_placement_t placement)
{
  switch (placement) {
#define PLACEMENT(name) \
  case name:            \
    return #name;
    BOX_PLACEMENTS
#undef PLACEMENT
  }
  return "";
}
inline std::optional<box_placement_t> box_placement_from_name(const std::string& name)
{
#define PLACEMENT(placementName) \
  if (name == #placementName) return placementName;
  BOX_PLACEMENTS
#undef PLACEMENT
  return std::nullopt;
}

class PanelLayout;

//**************************************************************************************************
/**
 * A colour: an index of ROOT (kRed + 1, 632) or rgb components, given as "#rrggbb" or {r, g, b} (each from 0 to 1).
 * ROOT draws with indices only: an rgb colour gets one when the plot is drawn (TColor::GetColor finds an existing colour of the same rgb),
 * since the index of a newly created colour depends on the process and cannot be stored with the plot definition. Transparency is set with
 * the alpha setters, as for indices.
 */
//**************************************************************************************************
class color_t
{
 public:
  color_t() = default;                       // kWhite
  color_t(int16_t index) : mIndex(index) {}  // NOLINT(runtime/explicit): a ROOT colour index is a colour
  color_t(const std::string& hex);           // NOLINT(runtime/explicit): "#rrggbb"
  // a string literal (not const char*: SetColor(0) would then be ambiguous between the index and a null pointer)
  template <std::size_t N>
  color_t(const char (&hex)[N]);  // NOLINT(runtime/explicit)
  color_t(double_t red, double_t green, double_t blue);
  bool operator==(const color_t& other) const { return mIndex == other.mIndex && mRGB == other.mRGB; }
  bool operator!=(const color_t& other) const { return !(*this == other); }

  std::optional<int16_t> GetIndex() const { return mIndex; }
  std::string ToString() const;                // the index as number or "#rrggbb"
  int16_t ToIndex() const;                     // index for ROOT (an rgb colour is looked up or created)
  std::array<float_t, 3> ToFractions() const;  // rgb from 0 to 1 (of the ROOT colour for an index)

 private:
  std::optional<int16_t> mIndex{0};
  std::array<uint8_t, 3> mRGB{};
};
template <std::size_t N>
color_t::color_t(const char (&hex)[N]) : color_t(std::string(hex))
{
}
std::ostream& operator<<(std::ostream& stream, const color_t& color);
std::istream& operator>>(std::istream& stream, color_t& color);

// colour, transparency, style and size of one attribute of a drawn object
struct layout_t {
  std::optional<color_t> color;        // marker_color , line_color, fill_color, text_color
  std::optional<float_t> alpha;        // marker_alpha , line_alpha, fill_alpha, text_alpha
  std::optional<int16_t> style;        // marker_style , line_style, fill_style, text_font
  std::optional<float_t> scale;        // marker_size , line_width, fill_?????, text_size
  void Update(const layout_t& other);  // takes over the values set in other
};

//**************************************************************************************************
/**
 * Setters of the marker, line, fill, text and border attributes, shared by the classes that have them (Derived: the class returned for chaining).
 */
//**************************************************************************************************
template <class Derived>
class MarkerSetters
{
 public:
  Derived& SetMarker(int16_t style, float_t size, color_t color, float_t alpha = 1.f);
  Derived& SetMarkerColor(color_t color);
  Derived& SetMarkerAlpha(float_t alpha);
  Derived& SetMarkerStyle(int16_t style);
  Derived& SetMarkerSize(float_t size);

 protected:
  const auto& GetMarkerColor() const { return mMarker.color; }
  const auto& GetMarkerAlpha() const { return mMarker.alpha; }
  const auto& GetMarkerStyle() const { return mMarker.style; }
  const auto& GetMarkerSize() const { return mMarker.scale; }
  void ReadMarker(const boost::property_tree::ptree& tree);
  void PutMarker(boost::property_tree::ptree& tree) const;
  layout_t mMarker;
};

template <class Derived>
class LineSetters
{
 public:
  Derived& SetLine(int16_t style, float_t width, color_t color, float_t alpha = 1.f);
  Derived& SetLineColor(color_t color);
  Derived& SetLineAlpha(float_t alpha);
  Derived& SetLineStyle(int16_t style);
  Derived& SetLineWidth(float_t width);

 protected:
  const auto& GetLineColor() const { return mLine.color; }
  const auto& GetLineAlpha() const { return mLine.alpha; }
  const auto& GetLineStyle() const { return mLine.style; }
  const auto& GetLineWidth() const { return mLine.scale; }
  void ReadLine(const boost::property_tree::ptree& tree);
  void PutLine(boost::property_tree::ptree& tree) const;
  layout_t mLine;
};

template <class Derived>
class FillSetters
{
 public:
  Derived& SetFill(int16_t style, color_t color, float_t alpha = 1.f);
  Derived& SetFillColor(color_t color);
  Derived& SetFillStyle(int16_t style);
  Derived& SetFillAlpha(float_t alpha);

 protected:
  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillAlpha() const { return mFill.alpha; }
  const auto& GetFillStyle() const { return mFill.style; }
  void ReadFill(const boost::property_tree::ptree& tree);
  void PutFill(boost::property_tree::ptree& tree) const;
  layout_t mFill;
};

template <class Derived>
class TextSetters
{
 public:
  Derived& SetText(int16_t font, float_t size, color_t color, float_t alpha = 1.f);
  Derived& SetTextColor(color_t color);
  Derived& SetTextAlpha(float_t alpha);
  Derived& SetTextFont(int16_t font);
  Derived& SetTextSize(float_t size);

 protected:
  const auto& GetTextColor() const { return mText.color; }
  const auto& GetTextAlpha() const { return mText.alpha; }
  const auto& GetTextFont() const { return mText.style; }
  const auto& GetTextSize() const { return mText.scale; }
  void ReadText(const boost::property_tree::ptree& tree);
  void PutText(boost::property_tree::ptree& tree) const;
  layout_t mText;
};

template <class Derived>
class BorderSetters
{
 public:
  Derived& SetBorder(int16_t style, float_t width, color_t color, float_t alpha = 1.f);
  Derived& SetBorderColor(color_t color);
  Derived& SetBorderAlpha(float_t alpha);
  Derived& SetBorderStyle(int16_t style);
  Derived& SetBorderWidth(float_t width);

 protected:
  const auto& GetBorderColor() const { return mBorder.color; }
  const auto& GetBorderAlpha() const { return mBorder.alpha; }
  const auto& GetBorderStyle() const { return mBorder.style; }
  const auto& GetBorderWidth() const { return mBorder.scale; }
  void ReadBorder(const boost::property_tree::ptree& tree);
  void PutBorder(boost::property_tree::ptree& tree) const;
  layout_t mBorder;
};

//**************************************************************************************************
/**
 * Class for internal representation of a plot.
 */
//**************************************************************************************************
class Plot : public FillSetters<Plot>
{
 public:
  class Pad;

  Plot() = default;
  explicit Plot(const boost::property_tree::ptree& plotTree);
  explicit Plot(const std::string& name, const std::string& group = "", const std::optional<std::string>& basePlot = {});
  Plot(const std::string& name, const PanelLayout& layout);  // a base plot with the panels and lengths of the layout and the look of the shipped plots (see PanelLayout.h)
  Pad& operator[](uint8_t padID) { return mPads[padID]; }
  Pad& GetPad(uint8_t padID) { return mPads[padID]; }
  void operator+=(const Plot& plot);
  friend Plot operator+(const Plot& basePlot, const Plot& plot);
  Plot(const Plot& otherPlot, const std::string& name, const std::optional<std::string>& group = {});
  void Print() const { Plot::Print(GetPropertyTree(), "Plot"); }

  // accessors for user
  Plot& SetName(const std::string& name);
  Plot& SetGroup(const std::string& group);
  Plot& AppendGroup(const std::string& subgroup);
  Plot& SetBasePlot(const std::string& name);
  Plot& SetDimensions(int32_t width, int32_t height, bool fixAspectRatio = false);
  Plot& SetWidth(int32_t width);
  Plot& SetHeight(int32_t height);
  Plot& SetFixAspectRatio(bool fixAspectRatio = true);
  Plot& SetTransparent();
  // The pads share the scale of the axis ('X', 'Y' or 'Z'): its range, log scale and time format. A setting of one of them made in a pad
  // of the group or in pad 0 holds for the whole group; without a range, the group gets the union of the automatic ranges of its pads.
  // Touching panels of a layout share their axes already (x per column, y per row). A pad listed again leaves its earlier group,
  // a single pad leaves its group, an empty list undoes all groups of the axis (also those of the layout).
  // 3D views (lego, surfaces) share only z, polar drawings nothing: they are left out of the other groups.
  Plot& ShareAxis(char axis, const std::vector<uint8_t>& pads);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class PanelLayout;

  // accessors for internal use by manager and painter
  const auto& GetName() const { return mName; }
  const auto& GetGroup() const { return mGroup; }
  const auto& GetBasePlotName() const { return mBasePlot; }
  const auto& GetUniqueName() const { return mUniqueName; }
  boost::property_tree::ptree GetPropertyTree() const;
  auto& GetPads() { return mPads; }

  const auto& GetHeight() const { return mPlotDimensions.height; }
  const auto& GetWidth() const { return mPlotDimensions.width; }
  const auto& IsFixAspectRatio() const { return mPlotDimensions.fixAspectRatio; }
  uint16_t GetDataCount() const;
  // the groups of pads sharing an axis, as the layout and ShareAxis declared them (groups of at least two pads)
  struct shared_group_t {
    std::vector<uint8_t> pads;
    bool fromLayout{};
  };
  std::map<char, std::vector<shared_group_t>> GetSharedAxes() const;
  // rows (X) and columns (Y) of touching panels whose axis title is drawn once for all of them
  const auto& GetCommonTitles() const { return mCommonTitles; }

  static void Print(const boost::property_tree::ptree& pt, const std::string& name);

 private:
  struct dimension_t {
    std::optional<int32_t> width;
    std::optional<int32_t> height;
    std::optional<bool> fixAspectRatio;
  };
  void UpdateUniqueName();

  template <class>
  friend class LineSetters;
  template <class>
  friend class BorderSetters;

  // helpers (also used by the nested classes and the setter groups)
  static std::string NormalizeGroup(const std::string& group);
  static float_t RoundWidth(float_t width, const char* what);
  static std::optional<float_t> RoundWidth(const std::optional<float_t>& width, const char* what);
  static void ReadWidthFromTree(const boost::property_tree::ptree& tree, std::optional<float_t>& target, const char* key);
  static void ReadDrawOption(const boost::property_tree::ptree& tree, std::optional<draw_option_t>& option, const char* key);
  static void PutDrawOption(boost::property_tree::ptree& tree, const std::optional<draw_option_t>& option, const char* key);

  std::string mName;
  std::string mGroup;
  std::string mUniqueName;
  std::optional<std::string> mBasePlot;
  dimension_t mPlotDimensions;
  std::map<uint8_t, Pad> mPads;
  struct shared_axis_t {
    char axis{};
    std::vector<uint8_t> pads;  // empty: undoes the groups of the axis declared before
    bool fromLayout{};          // declared by the layout (touching panels) or the base plot, not with ShareAxis
  };
  std::vector<shared_axis_t> mSharedAxes;                            // in the order of the declarations: a later one wins
  std::vector<std::pair<char, std::vector<uint8_t>>> mCommonTitles;  // declared by the layout
};

//**************************************************************************************************
/**
 * Representation of a pad.
 */
//**************************************************************************************************
class Plot::Pad : public FillSetters<Pad>
{
 public:
  class Data;
  class Ratio;
  class Axis;
  template <class BoxType>
  class Box;
  class TextBox;
  class LegendBox;
  class LegendEntry;
  class StatsBox;
  class Shape;
  class ImageBox;

  Pad() = default;
  explicit Pad(const boost::property_tree::ptree& padTree);
  Pad(const Pad& other);
  Pad(Pad&&) = default;
  Pad& operator=(const Pad& other);
  Pad& operator=(Pad&&) = default;
  void operator+=(const Pad& pad);
  Axis& operator[](const char axis);
  Data& operator()(uint16_t dataID);
  void Print() const { Plot::Print(GetPropertyTree(), "Pad"); }

  // User accessors:
  Data& AddData(const std::string& name, const std::string& dataSource, const std::optional<std::string>& label = {});
  Data& AddData(const Data& data, const std::optional<std::string>& label = {});
  Data& AddFunction(const std::string& function, const std::optional<std::string>& label = {});

  Data& AddPoints(const std::vector<double_t>& x, const std::vector<double_t>& y, const std::optional<std::string>& label = {});
  Data& AddPoints(const std::vector<std::pair<double_t, double_t>>& positions, const std::optional<std::string>& label = {});

  Ratio& AddRatio(const Data& numerator, const Data& denominator, const std::optional<std::string>& label = {});
  Ratio& AddRatio(const Ratio& ratio, const std::optional<std::string>& label = {});
  Ratio& AddRatio(const std::string& numeratorName, const std::string& numeratorDataSource,
                  const std::string& denominatorName, const std::string& denominatorDataSource,
                  const std::optional<std::string>& label = {});

  TextBox& AddText(double_t xPos, double_t yPos, const std::string& text);
  TextBox& AddText(double_t xPos, double_t yPos, const std::vector<std::string>& lines);
  TextBox& AddText(const std::string& text);
  TextBox& AddText(const std::vector<std::string>& lines);
  TextBox& AddText(box_placement_t placement, const std::string& text);
  TextBox& AddText(box_placement_t placement, const std::vector<std::string>& lines);
  // (for braced lists of lines, which would otherwise be ambiguous with std::string's iterator constructor)
  TextBox& AddText(double_t xPos, double_t yPos, std::initializer_list<std::string> lines);
  TextBox& AddText(std::initializer_list<std::string> lines);
  TextBox& AddText(box_placement_t placement, std::initializer_list<std::string> lines);
  LegendBox& AddLegend(double_t xPos, double_t yPos, const std::optional<std::string>& title = {});
  LegendBox& AddLegend(const std::optional<std::string>& title = {});
  LegendBox& AddLegend(box_placement_t placement, const std::optional<std::string>& title = {});
  // image file (png, jpg, ...) drawn at the given upper left corner or placed automatically; by default 0.2 of the pad width wide
  ImageBox& AddImage(double_t xPos, double_t yPos, const std::string& path);
  ImageBox& AddImage(const std::string& path);
  ImageBox& AddImage(box_placement_t placement, const std::string& path);

  // shapes: drawn in the pad but not data (never in the axis ranges, not numbered with the data, no legend row); positions in axis values
  Shape& AddLine(std::pair<double_t, double_t> pos1, std::pair<double_t, double_t> pos2);    // not cut at the frame
  Shape& AddArrow(std::pair<double_t, double_t> pos1, std::pair<double_t, double_t> pos2);   // from pos1 to pos2, not cut at the frame
  Shape& AddVerticalLine(double_t x);                                                        // over the height of the frame
  Shape& AddHorizontalLine(double_t y);                                                      // over the width of the frame
  Shape& AddRegion(std::pair<double_t, double_t> pos1, std::pair<double_t, double_t> pos2);  // rectangle between two corners, cut at the frame
  Shape& AddRegionX(double_t x1, double_t x2);                                               // over the height of the frame
  Shape& AddRegionY(double_t y1, double_t y2);                                               // over the width of the frame
  Shape& AddEllipse(std::pair<double_t, double_t> center, double_t radiusX, double_t radiusY);
  Shape& AddPolygon(const std::vector<std::pair<double_t, double_t>>& points);  // closed automatically, cut at the frame

  Axis& GetAxis(const char axis);
  Data& GetData(uint16_t dataID);
  LegendBox& GetLegend(uint8_t legendID);
  TextBox& GetText(uint8_t textID);
  Shape& GetShape(uint8_t shapeID);
  ImageBox& GetImage(uint8_t imageID);

  Data& SetRefFunc(const std::string& refFunc);                            // "": no reference function, also not the one of the base plot or of pad 0
  Data& SetFrame(const std::string& name, const std::string& dataSource);  // input that only defines the axis frame (not drawn, not numbered)
  Data& SetFrame(const Data& data);

  Pad& SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup);
  Pad& SetMargins(float_t top, float_t bottom, float_t left, float_t right);
  Pad& SetTopMargin(float_t top);
  Pad& SetBottomMargin(float_t bottom);
  Pad& SetRightMargin(float_t right);
  Pad& SetLeftMargin(float_t left);
  Pad& SetView(double_t theta, double_t phi);  // 3D drawings (lego, surf, triangles, 3D points), degrees: theta above the x-y plane, phi around the z axis (ROOT: 30, 30)
  Pad& SetPalette(int32_t palette);
  // gradients: (r, g, b, position) or (colour, position), positions from 0 to 1
  Pad& SetPalette(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetPalette(const std::vector<std::pair<color_t, float_t>>& gradient, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultTextSize(float_t size);
  Pad& SetDefaultTextColor(color_t color);
  Pad& SetDefaultTextAlpha(float_t alpha);
  Pad& SetDefaultTextFont(int16_t font);
  Pad& SetDefaultAlpha(float_t alpha);
  Pad& SetDefaultColors(const std::vector<color_t>& colors);
  Pad& SetDefaultColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultColors(const std::vector<std::pair<color_t, float_t>>& gradient, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultMarkerAlpha(float_t alpha);
  Pad& SetDefaultMarkerSize(float_t size);
  Pad& SetDefaultMarkerColors(const std::vector<color_t>& colors);
  Pad& SetDefaultMarkerColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultMarkerColors(const std::vector<std::pair<color_t, float_t>>& gradient, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultMarkerStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultLineAlpha(float_t alpha);
  Pad& SetDefaultLineWidth(float_t width);
  Pad& SetDefaultLineColors(const std::vector<color_t>& colors);
  Pad& SetDefaultLineColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultLineColors(const std::vector<std::pair<color_t, float_t>>& gradient, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultLineStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultFillAlpha(float_t alpha);
  Pad& SetDefaultFillColors(const std::vector<color_t>& colors);
  Pad& SetDefaultFillColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultFillColors(const std::vector<std::pair<color_t, float_t>>& gradient, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultFillStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultCandleBoxRange(float_t range);
  Pad& SetDefaultCandleWhiskerRange(float_t range);
  Pad& SetTransparent();
  Pad& SetFrameFill(int16_t style, color_t color, float_t alpha = 1.f);
  Pad& SetFrameFillColor(color_t color);
  Pad& SetFrameFillStyle(int16_t style);
  Pad& SetFrameFillAlpha(float_t alpha);
  Pad& SetFrameBorder(int16_t style, float_t width, color_t color, float_t alpha = 1.f);
  Pad& SetFrameBorderColor(color_t color);
  Pad& SetFrameBorderAlpha(float_t alpha);
  Pad& SetFrameBorderStyle(int16_t style);
  Pad& SetFrameBorderWidth(float_t width);
  Pad& SetFrameTransparent();
  Pad& SetRedrawAxes(bool redraw = true);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;
  friend class PanelLayout;

  boost::property_tree::ptree GetPropertyTree() const;

  auto& GetData() { return mData; }
  auto& GetLegendBoxes() { return mLegendBoxes; }
  auto& GetTextBoxes() { return mTextBoxes; }
  auto& GetShapes() { return mShapes; }
  auto& GetImageBoxes() { return mImageBoxes; }
  static std::vector<std::tuple<float_t, float_t, float_t, float_t>> GradientEndpoints(const std::vector<std::pair<color_t, float_t>>& gradient);
  static void CheckGradient(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, const std::string& setter);
  uint16_t GetDataCount() const { return mData.size(); }

  const auto& GetAxes() const { return mAxes; }
  const auto& GetXLow() const { return mPosition.xlow; }
  const auto& GetYLow() const { return mPosition.ylow; }
  const auto& GetXUp() const { return mPosition.xup; }
  const auto& GetYUp() const { return mPosition.yup; }
  const auto& GetMarginTop() const { return mMargins.top; }
  const auto& GetMarginBottom() const { return mMargins.bottom; }
  const auto& GetMarginLeft() const { return mMargins.left; }
  const auto& GetMarginRight() const { return mMargins.right; }
  const auto& GetViewTheta() const { return mView.theta; }
  const auto& GetViewPhi() const { return mView.phi; }
  const auto& GetPalette() const { return mPalette.index; }
  const auto& GetPaletteGradient() const { return mPalette.colorGradient; }
  const auto& GetFrameFillColor() const { return mFrameFill.color; }
  const auto& GetFrameFillStyle() const { return mFrameFill.style; }
  const auto& GetFrameFillAlpha() const { return mFrameFill.alpha; }
  const auto& GetFrameBorderColor() const { return mFrameBorder.color; }
  const auto& GetFrameBorderAlpha() const { return mFrameBorder.alpha; }
  const auto& GetFrameBorderStyle() const { return mFrameBorder.style; }
  const auto& GetFrameBorderWidth() const { return mFrameBorder.scale; }
  const auto& GetDefaultTextColor() const { return mText.color; }
  const auto& GetDefaultTextAlpha() const { return mText.alpha; }
  const auto& GetDefaultTextFont() const { return mText.style; }
  const auto& GetDefaultTextSize() const { return mText.scale; }
  const auto& GetDefaultMarkerSize() const { return mMarkerDefaults.scale; }
  const auto& GetDefaultMarkerAlpha() const { return mMarkerDefaults.alpha; }
  const auto& GetDefaultMarkerStyles() const { return mMarkerDefaults.styles; }
  const auto& GetDefaultMarkerColors() const { return mMarkerDefaults.colors; }
  const auto& GetDefaultMarkerColorsGradient() const { return mMarkerDefaults.colorGradient; }
  const auto& GetDefaultLineWidth() const { return mLineDefaults.scale; }
  const auto& GetDefaultLineAlpha() const { return mLineDefaults.alpha; }
  const auto& GetDefaultLineColors() const { return mLineDefaults.colors; }
  const auto& GetDefaultLineColorsGradient() const { return mLineDefaults.colorGradient; }
  const auto& GetDefaultLineStyles() const { return mLineDefaults.styles; }
  const auto& GetDefaultFillAlpha() const { return mFillDefaults.alpha; }
  const auto& GetDefaultFillColors() const { return mFillDefaults.colors; }
  const auto& GetDefaultFillColorsGradient() const { return mFillDefaults.colorGradient; }
  const auto& GetDefaultFillStyles() const { return mFillDefaults.styles; }
  const auto& GetDefaultCandleBoxRange() const { return mCandleOptionDefaults.boxRange; }
  const auto& GetDefaultCandleWhiskerRange() const { return mCandleOptionDefaults.whiskerRange; }
  const auto& GetRedrawAxes() const { return mRedrawAxes; }
  bool GetIsEmptyCell() const { return mIsEmptyCell; }
  const auto& GetColorScaleOf() const { return mColorScaleOf; }
  const auto& GetRefFunc() const { return mRefFunc; }
  const auto& GetFrame() const { return mFrame; }

 private:
  struct pad_position_t {
    std::optional<double_t> xlow;
    std::optional<double_t> ylow;
    std::optional<double_t> xup;
    std::optional<double_t> yup;
  };
  struct pad_margin_t {
    std::optional<float_t> top;
    std::optional<float_t> bottom;
    std::optional<float_t> left;
    std::optional<float_t> right;
  };
  struct pad_view_t {
    std::optional<double_t> theta;
    std::optional<double_t> phi;
  };
  struct gradient_color_t {
    std::optional<std::vector<std::tuple<float_t, float_t, float_t, float_t>>> rgbEndpoints;
    std::optional<float_t> alpha;
    std::optional<int32_t> nColors;
  };
  struct view_defaults_t {
    std::optional<float_t> scale;
    std::optional<float_t> alpha;
    std::optional<std::vector<int16_t>> styles;
    std::optional<std::vector<color_t>> colors;
    gradient_color_t colorGradient;
  };
  struct palette_defaults_t {
    std::optional<int32_t> index;
    gradient_color_t colorGradient;
  };
  struct candle_defaults_t {
    std::optional<double_t> boxRange;
    std::optional<double_t> whiskerRange;
  };

  // properties
  pad_position_t mPosition;
  pad_margin_t mMargins;
  pad_view_t mView;
  layout_t mFrameFill;
  layout_t mFrameBorder;
  layout_t mText;

  // user defined default data representation
  view_defaults_t mMarkerDefaults;
  view_defaults_t mLineDefaults;
  view_defaults_t mFillDefaults;
  candle_defaults_t mCandleOptionDefaults;
  palette_defaults_t mPalette;
  std::optional<bool> mRedrawAxes;
  bool mIsEmptyCell{};                   // an Empty() cell of a PanelLayout: room for a legend or a text, nothing to warn about when left blank
  std::optional<uint8_t> mColorScaleOf;  // a ZAxis() cell of a PanelLayout: shows the colour scale of the z group of this pad

  std::map<char, Axis> mAxes;
  std::vector<std::shared_ptr<Data>> mData;
  std::shared_ptr<Data> mRefFunc;
  std::shared_ptr<Data> mFrame;

  std::vector<std::shared_ptr<LegendBox>> mLegendBoxes;
  std::vector<std::shared_ptr<TextBox>> mTextBoxes;
  std::vector<std::shared_ptr<Shape>> mShapes;
  std::vector<std::shared_ptr<ImageBox>> mImageBoxes;
};

//**************************************************************************************************
/**
 * Representation of a legend row: the row of a data item (styled with Data::SetLegendEntry) or a row added with LegendBox::AddEntry.
 */
//**************************************************************************************************
class Plot::Pad::LegendEntry : public MarkerSetters<LegendEntry>, public LineSetters<LegendEntry>, public FillSetters<LegendEntry>, public TextSetters<LegendEntry>
{
 public:
  LegendEntry() = default;
  explicit LegendEntry(const boost::property_tree::ptree& legendEntryTree);
  void Print() const { Plot::Print(GetPropertyTree(), "LegendEntry"); }

  LegendEntry& SetLabel(const std::string& label);
  LegendEntry& SetRefData(uint16_t refDataID);
  LegendEntry& SetSymbolElements(const std::string& symbolElements);
  // position of a row added with LegendBox::AddEntry (by default after all other rows), relative to the rows of the data of the pad,
  // which are in the order of the data (dataID as in plot[padID](dataID)), whether or not that data has a row in this legend
  LegendEntry& AtTop();
  LegendEntry& BeforeData(uint16_t dataID);
  LegendEntry& AfterData(uint16_t dataID);
  LegendEntry& SetColor(color_t color);
  LegendEntry& SetAlpha(float_t alpha);

 protected:
  friend class PlotPainter;
  friend class LegendBox;
  friend class Data;
  enum class position_t : uint8_t { top,
                                    before,
                                    after };
  // the rows a LegendBox makes itself: an extra row with its label, the row of a data item
  explicit LegendEntry(const std::optional<std::string>& label, const std::optional<uint16_t>& refDataID = {}, const std::optional<std::string>& symbolElements = {});
  void operator+=(const LegendEntry& legendEntry);

  boost::property_tree::ptree GetPropertyTree() const;
  const auto& GetRefDataID() const { return mRefDataID; }
  const auto& GetRefPadID() const { return mRefPadID; }
  const auto& GetLabel() const { return mLabel; }
  const auto& GetSymbolElements() const { return mSymbolElements; }

 private:
  std::optional<std::string> mLabel;
  std::optional<uint16_t> mRefDataID;
  std::optional<uint8_t> mRefPadID;  // the data of the row is drawn in another pad
  std::optional<std::string> mSymbolElements;
  std::optional<position_t> mPosition;
  uint16_t mPositionDataID{};  // the data the position is relative to (for before and after)
};

//**************************************************************************************************
/**
 * Representation of drawable data.
 */
//**************************************************************************************************
class Plot::Pad::Data : public MarkerSetters<Data>, public LineSetters<Data>, public FillSetters<Data>
{
 public:
  Data() = default;
  Data(const std::string& name, const std::string& dataSource, const std::optional<std::string>& label = {});
  explicit Data(const boost::property_tree::ptree& dataTree);

  virtual ~Data() = default;
  Data(const Data& other) = default;
  Data(Data&&) = default;
  Data& operator=(const Data& other) = default;
  Data& operator=(Data&& other) = default;
  virtual std::shared_ptr<Data> Clone() const { return std::make_shared<Data>(*this); }
  void Print() const { Plot::Print(GetPropertyTree(), "Data"); }

  // use this data as template: copy with the given name (and optionally another data source); called on a Ratio, the copy is data
  // with the ratio's appearance (no denominator: add it with AddData, or use it as numerator or denominator of AddRatio)
  Data operator()(const std::string& name, const std::optional<std::string>& dataSource = {}) const;

  Ratio& AsRatio();
  const std::string& GetDataSource() const { return mDataSource; }

  Data& SetDataSource(const std::string& dataSource);
  Data& SetLayout(const Data& dataLayout);
  Data& ApplyLayout(const Data& dataLayout);
  Data& SetRangeX(double_t min, double_t max);
  Data& SetRangeX();  // no range restriction in x
  Data& SetMaxRangeX(double_t max);
  Data& SetMinRangeX(double_t min);
  Data& SetRangeY(double_t min, double_t max);
  Data& SetRangeY();  // no range restriction in y
  Data& SetMaxRangeY(double_t max);
  Data& SetMinRangeY(double_t min);
  Data& SetShowOverflowBins(bool showOverflowBins = true);
  Data& SetLegendLabel(const std::string& legendLabel);
  Data& SetLegend(uint8_t legendID);
  Data& SetLegendEntry(const LegendEntry& legendEntry);
  Data& SetDrawOption(const std::string& drawOption);
  Data& SetDrawOption(draw_option_t optionAlias);
  Data& SetDrawOption();
  Data& SetTextFormat(const std::string& textFormat);
  Data& SetTextAngle(float_t angle);  // degrees, of the values written by text and points_text (ROOT: 0 to 90)
  // statistics box of a histogram: the letters of TStyle::SetOptStat (without 'n'), the title (if given) is its first line
  Data& SetShowStats(const std::string& format = "emr", const std::optional<std::string>& title = {});
  Data& SetStatsBox(const StatsBox& statsBox);
  Data& SetColor(color_t color);
  Data& SetAlpha(float_t alpha);
  Data& SetDefinesFrame();                    // this data (drawn as usual) defines the axis frame; the last one in a pad wins
  Data& SetKeepInputStyle(bool keep = true);  // line, marker and fill stored with the input win over the defaults of the pad
  Data& SetContours(const std::vector<double>& contours);
  Data& SetContours(int32_t nContours);
  Data& SetNpx(int32_t nx, std::optional<int32_t> ny = {});

  // data modifiers
  // the optional range in x restricts the integral or the maximum to the bins reaching into it (points within it for graphs)
  Data& Normalize(bool isDensity = false, std::optional<double_t> min = {}, std::optional<double_t> max = {});
  Data& NormalizeToMaximum(std::optional<double_t> min = {}, std::optional<double_t> max = {});
  Data& Scale(double_t scaleFactor);
  Data& DivideBinWidth(bool divideBinWidth = true);
  Data& Density();
  Data& RebinX(uint16_t nGroup);
  Data& RebinY(uint16_t nGroup);
  Data& RebinZ(uint16_t nGroup);
  Data& RebinXY(uint16_t nGroupX, uint16_t nGroupY);
  Data& RebinXYZ(uint16_t nGroupX, uint16_t nGroupY, uint16_t nGroupZ);
  Data& ScaleX(double_t scaleFactor);
  Data& ScaleY(double_t scaleFactor);
  Data& ScaleZ(double_t scaleFactor);
  Data& Smooth(uint16_t nIterSmooth = 1);
  Data& Cumulative(bool forward = true);

  virtual Data& Project(const std::vector<uint8_t>& dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {});
  virtual Data& ProjectX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});  // for 2d histos
  virtual Data& ProjectY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});  // for 2d histos

  virtual Data& Profile(const std::vector<uint8_t>& dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {});  // for 2d & 3d histos
  virtual Data& ProfileX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});                                                                   // for 2d histos
  virtual Data& ProfileY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});                                                                   // for 2d histos
  // (for braced lists of axes like Project({2}, {{0, 10, 50}}), which would otherwise be ambiguous with the tree overloads)
  Data& Project(std::initializer_list<uint8_t> dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {});
  Data& Profile(std::initializer_list<uint8_t> dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {});

  struct data_dim_t {
    data_dim_t() = default;
    // template instead of plain const char* so a literal 0 will not crash at runtime
    // implicit, so a plain variable can be passed: Project1D("pt")
    template <typename Str, typename = std::enable_if_t<std::is_same_v<Str, const char*> || std::is_same_v<Str, char*>>>
    data_dim_t(Str _var, int32_t _nBins = 100, const std::vector<double_t>& _range = {0, 0}) : var(_var), edges(_range), nBins(_nBins)  // NOLINT(runtime/explicit)
    {
    }
    data_dim_t(const std::string& _var, int32_t _nBins = 100, const std::vector<double_t>& _range = {0, 0}) : var(_var), edges(_range), nBins(_nBins) {}  // NOLINT(runtime/explicit)
    data_dim_t(const std::string& _var, const std::vector<double_t>& _edges) : var(_var), edges(_edges) {}
    std::string var{};
    std::vector<double_t> edges{};
    int32_t nBins{0};
  };

  virtual Data& Project(const std::vector<data_dim_t>& dataDims, std::optional<std::string> weight = {});
  // (for braced lists of columns like Project({"x", "y"}), which would otherwise be ambiguous with the histogram overload)
  Data& Project(std::initializer_list<data_dim_t> dataDims, std::optional<std::string> weight = {});
  virtual Data& Project1D(data_dim_t x, std::optional<std::string> weight = {});
  virtual Data& Project2D(data_dim_t x, data_dim_t y, std::optional<std::string> weight = {});

  virtual Data& Scatter(const std::string& x, const std::string& y);
  virtual Data& Scatter(const std::string& x, const std::string& y, const std::string& xErr, const std::string& yErr);
  virtual Data& Scatter(const std::string& x, const std::string& y, const std::string& xErrLow, const std::string& xErrHigh, const std::string& yErrLow, const std::string& yErrHigh);
  // points in 3d (x, y, z): a 2d graph (TGraph2D), e.g. drawn as points_colz or triangles; scalar columns only
  Data& Scatter(const std::string& x, const std::string& y, const std::string& z);

  virtual Data& Profile(const std::vector<data_dim_t>& dataDims, const std::string& profile, std::optional<std::string> weight = {});
  virtual Data& Profile1D(data_dim_t x, const std::string& profile, std::optional<std::string> weight = {});
  virtual Data& Profile2D(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> weight = {});

  virtual Data& Define(const std::string& key, const std::string& value);
  virtual Data& Filter(const std::string& filter);
  // tree to join: "name" (from the data source of the data) or {"name", "dataSource"}
  struct join_tree_t {
    // template instead of plain const char* so a literal 0 will not crash at runtime
    // implicit, so a plain name can be passed: Join("scores")
    template <typename Str, typename = std::enable_if_t<std::is_same_v<Str, const char*> || std::is_same_v<Str, char*>>>
    join_tree_t(Str _name) : name(_name)  // NOLINT(runtime/explicit)
    {
    }
    join_tree_t(const std::string& _name) : name(_name) {}  // NOLINT(runtime/explicit)
    join_tree_t(const std::string& _name, const std::string& _dataSource) : name(_name), dataSource(_dataSource) {}
    std::string name;
    std::string dataSource;
  };
  virtual Data& Join(const join_tree_t& tree, const std::vector<std::string>& keys = {}, const std::string& alias = "");
  virtual Data& Entries(uint32_t nEntries);
  virtual Data& Entries(uint32_t entryMin, uint32_t entryMax);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;
  friend class Pad;
  friend class Ratio;

  virtual boost::property_tree::ptree GetPropertyTree() const;
  void SetType(const std::string& type) { mType = type; }

  const auto& GetType() const { return mType; }
  const auto& GetName() const { return mName; }
  const auto& GetLegendLabel() const { return mLegend.label; }
  const auto& GetLegendID() const { return mLegend.id; }
  const auto& GetLegendEntry() const { return mLegend.entry; }
  const auto& GetDrawOption() const { return mDrawOption; }
  const auto& GetDrawOptionAlias() const { return mDrawOptionAlias; }
  const auto& GetTextFormat() const { return mTextFormat; }
  const auto& GetTextAngle() const { return mTextAngle; }
  const auto& GetStatsFormat() const { return mStats.format; }
  const auto& GetStatsTitle() const { return mStats.title; }
  const auto& GetStatsBox() const { return mStats.box; }
  const auto& GetScaleFactor() const { return mModify.scaleFactor; }
  const auto& GetScaleBinWidthNorm() const { return mModify.scaleBinWidthNorm; }
  const auto& GetNormMaximum() const { return mModify.normMaximum; }
  const auto& GetNormMin() const { return mModify.normMin; }
  const auto& GetNormMax() const { return mModify.normMax; }
  const auto& GetNormMaximumMin() const { return mModify.normMaximumMin; }
  const auto& GetNormMaximumMax() const { return mModify.normMaximumMax; }
  const auto& GetMinRangeX() const { return mRangeX.min; }
  const auto& GetMaxRangeX() const { return mRangeX.max; }
  const auto& GetMinRangeY() const { return mRangeY.min; }
  const auto& GetMaxRangeY() const { return mRangeY.max; }
  const bool& GetDefinesFrame() const { return mDefinesFrame; }
  const bool& GetKeepInputStyle() const { return mKeepInputStyle; }
  const auto& GetContours() const { return mContours; }
  const auto& GetNContours() const { return mNContours; }
  const auto& GetNpx() const { return mNpx; }
  const auto& GetNpy() const { return mNpy; }
  const auto& GetDataInfo() const { return mDataInfo; }
  const auto& GetProjInfo() const { return mProjInfo; }
  const auto& GetNiterSmooth() const { return mModify.nIterSmooth; }
  const auto& GetRebinGroupX() const { return mModify.rebinGroupX; }
  const auto& GetRebinGroupY() const { return mModify.rebinGroupY; }
  const auto& GetRebinGroupZ() const { return mModify.rebinGroupZ; }
  const auto& GetScaleAxisX() const { return mModify.scaleAxisX; }
  const auto& GetScaleAxisY() const { return mModify.scaleAxisY; }
  const auto& GetScaleAxisZ() const { return mModify.scaleAxisZ; }
  const auto& GetDivideBinWidth() const { return mModify.divideBinWidth; }
  const auto& GetShowOverflowBins() const { return mModify.showOverflowBins; }
  const auto& GetCumulative() const { return mModify.cumulative; }

  struct modify_t {
    std::optional<bool> scaleBinWidthNorm;
    std::optional<bool> normMaximum;
    std::optional<double_t> normMin;  // range in x for Normalize
    std::optional<double_t> normMax;
    std::optional<double_t> normMaximumMin;  // range in x for NormalizeToMaximum
    std::optional<double_t> normMaximumMax;
    std::optional<double_t> scaleFactor;
    std::optional<bool> divideBinWidth;
    std::optional<uint16_t> rebinGroupX;
    std::optional<uint16_t> rebinGroupY;
    std::optional<uint16_t> rebinGroupZ;
    std::optional<double_t> scaleAxisX;
    std::optional<double_t> scaleAxisY;
    std::optional<double_t> scaleAxisZ;
    std::optional<bool> showOverflowBins;
    std::optional<bool> cumulative;
    std::optional<uint16_t> nIterSmooth;
  };
  modify_t& Modify() { return mModify; }
  const modify_t& Modify() const { return mModify; }
  static void ReadModify(const boost::property_tree::ptree& tree, modify_t& modify, const std::string& prefix = "");
  static void PutModify(boost::property_tree::ptree& tree, const modify_t& modify, const std::string& prefix = "");

  struct proj_info_t {
    std::vector<uint8_t> dims;                                    // dimensions to project on (can be one or two)
    std::vector<std::tuple<uint8_t, double_t, double_t>> ranges;  // range restrictions on other dimensions
    std::optional<bool> isUserCoord{};                            // whether ranges are specified in user coordinates or as bins
    std::optional<bool> isProfile{};                              // whether this should be a profile instead of a projection
    std::string GetNameSuffix() const;
  };

  struct data_info_t {
    std::vector<data_dim_t> dataDims{};
    std::optional<std::string> weight{};
    std::optional<bool> isProfileNoScatter{};
    struct definitions_t {
      std::optional<std::vector<std::string>> keys{};
      std::optional<std::vector<std::string>> values{};
    };
    definitions_t definitions;
    std::optional<std::vector<std::string>> filters{};
    // further trees whose columns are added to the rows of the tree
    struct join_t {
      std::string tree;                 // name of the joined tree (with the same meaning of a path as in the name of the data)
      std::string dataSource;           // empty: the data source of the tree
      std::vector<std::string> keys{};  // columns with equal values in matching rows (empty: rows are matched by their number)
      std::string alias;                // empty: last part of the tree name
      std::string GetAlias() const;     // prefix of the columns of the joined tree
      std::string ToString() const;     // stored form: tree|dataSource|key1,key2|alias
      static join_t FromString(const std::string& str);
      std::string GetDescription() const;
      bool operator==(const join_t& other) const { return ToString() == other.ToString(); }
    };
    std::optional<std::vector<join_t>> joins{};
    struct entry_range_t {
      std::optional<uint32_t> min{};
      std::optional<uint32_t> max{};
      void Set(std::optional<uint32_t> entryMin, uint32_t entryMax);  // the rules for Entries(), shared by data and ratio denominator
    };
    entry_range_t entries;
    void set(const data_info_t& other);  // adopts the request (dims, weight, kind); throws when no dimensions are given
    bool singleProc() const
    {
      // disable multi processing in case scatter (order might be relevant) or range is requested
      return !isProfileNoScatter.value_or(true) || entries.max.has_value();
    }
    std::string GetNameSuffix() const;
    std::string GetDescription() const;  // human readable version of the name suffix
  };
  static std::optional<std::vector<data_info_t::join_t>> ReadJoins(const boost::property_tree::ptree& tree, const std::string& label);
  static void PutJoins(boost::property_tree::ptree& tree, const std::optional<std::vector<data_info_t::join_t>>& joins, const std::string& label);
  static void AddJoin(data_info_t& dataInfo, const std::string& name, const join_tree_t& tree, const std::vector<std::string>& keys, const std::string& alias);

 private:
  bool mDefinesFrame{};
  bool mKeepInputStyle{};

  std::string mType;  // for introspection: "data" or "ratio"
  std::string mName;
  std::string mDataSource;

  std::optional<std::string> mDrawOption;
  std::optional<draw_option_t> mDrawOptionAlias;
  std::optional<std::string> mTextFormat;
  std::optional<float_t> mTextAngle;

  struct stats_t {
    std::optional<std::string> format;
    std::optional<std::string> title;
    std::shared_ptr<const StatsBox> box;  // placement and appearance of the box (not changed once set, so copies of the data can share it)
  };
  stats_t mStats;

  struct legend_t {
    std::optional<std::string> label;
    std::optional<uint8_t> id;
    std::optional<LegendEntry> entry;  // appearance of the row of the data
  };

  struct data_range_t {
    std::optional<double_t> min;
    std::optional<double_t> max;
  };

  data_info_t mDataInfo;
  std::optional<proj_info_t> mProjInfo;

  legend_t mLegend;
  data_range_t mRangeX;
  data_range_t mRangeY;
  modify_t mModify;

  std::optional<std::vector<double_t>> mContours;
  std::optional<int32_t> mNContours;
  std::optional<int32_t> mNpx;  // points at which a function is evaluated to draw it (along y for 2d functions: mNpy)
  std::optional<int32_t> mNpy;
};

//**************************************************************************************************
/**
 * Representation of a Ratio.
 */
//**************************************************************************************************
class Plot::Pad::Ratio : public Plot::Pad::Data
{
 public:
  Ratio(const Data& numerator, const Data& denominator);  // inputs from numerator and denominator, appearance from numerator
  Ratio(const std::string& numeratorName, const std::string& numeratorDataSource, const std::string& denominatorName,
        const std::string& denominatorDataSource, const std::optional<std::string>& label = {});
  explicit Ratio(const boost::property_tree::ptree& dataTree);

  virtual ~Ratio() = default;
  Ratio(const Ratio& other) = default;
  Ratio(Ratio&&) = default;
  Ratio& operator=(const Ratio& other) = default;
  Ratio& operator=(Ratio&& other) = default;
  std::shared_ptr<Data> Clone() const override { return std::make_shared<Ratio>(*this); }
  void Print() const { Plot::Print(GetPropertyTree(), "Ratio"); }

  Ratio& SetCorrelated(bool correlated = true);

#define FORWARD_TO_DATA(METHOD)                \
  template <typename... Args>                  \
  Ratio& METHOD(Args&&... args)                \
  {                                            \
    Data::METHOD(std::forward<Args>(args)...); \
    return *this;                              \
  }
  FORWARD_TO_DATA(SetDataSource)
  FORWARD_TO_DATA(SetLayout)
  FORWARD_TO_DATA(ApplyLayout)
  FORWARD_TO_DATA(SetRangeX)
  FORWARD_TO_DATA(SetMaxRangeX)
  FORWARD_TO_DATA(SetMinRangeX)
  FORWARD_TO_DATA(SetRangeY)
  FORWARD_TO_DATA(SetMaxRangeY)
  FORWARD_TO_DATA(SetMinRangeY)
  FORWARD_TO_DATA(SetShowOverflowBins)
  FORWARD_TO_DATA(SetLegendLabel)
  FORWARD_TO_DATA(SetLegend)
  FORWARD_TO_DATA(SetLegendEntry)
  FORWARD_TO_DATA(SetDrawOption)
  FORWARD_TO_DATA(SetTextFormat)
  FORWARD_TO_DATA(SetTextAngle)
  FORWARD_TO_DATA(SetShowStats)
  FORWARD_TO_DATA(SetStatsBox)
  FORWARD_TO_DATA(SetColor)
  FORWARD_TO_DATA(SetAlpha)
  FORWARD_TO_DATA(SetMarker)
  FORWARD_TO_DATA(SetMarkerColor)
  FORWARD_TO_DATA(SetMarkerAlpha)
  FORWARD_TO_DATA(SetMarkerStyle)
  FORWARD_TO_DATA(SetMarkerSize)
  FORWARD_TO_DATA(SetLine)
  FORWARD_TO_DATA(SetLineColor)
  FORWARD_TO_DATA(SetLineAlpha)
  FORWARD_TO_DATA(SetLineStyle)
  FORWARD_TO_DATA(SetLineWidth)
  FORWARD_TO_DATA(SetFill)
  FORWARD_TO_DATA(SetFillColor)
  FORWARD_TO_DATA(SetFillAlpha)
  FORWARD_TO_DATA(SetFillStyle)
  FORWARD_TO_DATA(SetDefinesFrame)
  FORWARD_TO_DATA(SetKeepInputStyle)
  FORWARD_TO_DATA(SetContours)
  FORWARD_TO_DATA(SetNpx)

#undef FORWARD_TO_DATA

  // data modifiers: act on the ratio itself (default), or on numerator and/or denominator before the division
#define FORWARD_MODIFIER(METHOD)                                                \
  template <typename... Args>                                                   \
  Ratio& METHOD(Args... args)                                                   \
  {                                                                             \
    auto applyTo = [&](modify_t& target) {                                      \
      std::swap(Modify(), target);                                              \
      Data::METHOD(args...);                                                    \
      std::swap(Modify(), target);                                              \
    };                                                                          \
    if (mModMode == Mode::Res) Data::METHOD(args...);                           \
    if (mModMode == Mode::Num || mModMode == Mode::Both) applyTo(mNumModify);   \
    if (mModMode == Mode::Den || mModMode == Mode::Both) applyTo(mDenomModify); \
    return *this;                                                               \
  }
  FORWARD_MODIFIER(Normalize)
  FORWARD_MODIFIER(NormalizeToMaximum)
  FORWARD_MODIFIER(Scale)
  FORWARD_MODIFIER(DivideBinWidth)
  FORWARD_MODIFIER(Density)
  FORWARD_MODIFIER(RebinX)
  FORWARD_MODIFIER(RebinY)
  FORWARD_MODIFIER(RebinZ)
  FORWARD_MODIFIER(RebinXY)
  FORWARD_MODIFIER(RebinXYZ)
  FORWARD_MODIFIER(ScaleX)
  FORWARD_MODIFIER(ScaleY)
  FORWARD_MODIFIER(ScaleZ)
  FORWARD_MODIFIER(Smooth)
  FORWARD_MODIFIER(Cumulative)
#undef FORWARD_MODIFIER

  Ratio& Numer();   // following modifiers act on the numerator, before the division
  Ratio& Denom();   // following modifiers act on the denominator, before the division
  Ratio& Both();    // following modifiers act on numerator and denominator, before the division
  Ratio& Result();  // following modifiers act on the ratio itself (default)

  Ratio& Project(const std::vector<uint8_t>& dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {}) override;
  Ratio& ProjectX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {}) override;
  Ratio& ProjectY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {}) override;

  Ratio& Profile(const std::vector<uint8_t>& dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {}) override;
  Ratio& ProfileX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {}) override;
  Ratio& ProfileY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {}) override;
  Ratio& Project(std::initializer_list<uint8_t> dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {});
  Ratio& Profile(std::initializer_list<uint8_t> dims, const std::vector<std::tuple<uint8_t, double_t, double_t>>& ranges = {}, std::optional<bool> isUserCoord = {});

  Ratio& Project(const std::vector<data_dim_t>& dataDims, std::optional<std::string> weight = {}) override;
  Ratio& Project(std::initializer_list<data_dim_t> dataDims, std::optional<std::string> weight = {});
  Ratio& Project1D(data_dim_t x, std::optional<std::string> weight = {}) override;
  Ratio& Project2D(data_dim_t x, data_dim_t y, std::optional<std::string> weight = {}) override;

  Ratio& Scatter(const std::string& x, const std::string& y) override;
  Ratio& Scatter(const std::string& x, const std::string& y, const std::string& xErr, const std::string& yErr) override;
  Ratio& Scatter(const std::string& x, const std::string& y, const std::string& xErrLow, const std::string& xErrHigh, const std::string& yErrLow, const std::string& yErrHigh) override;

  Ratio& Profile(const std::vector<data_dim_t>& dataDims, const std::string& profile, std::optional<std::string> weight = {}) override;
  Ratio& Profile1D(data_dim_t x, const std::string& profile, std::optional<std::string> weight = {}) override;
  Ratio& Profile2D(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> weight = {}) override;

  Ratio& Define(const std::string& key, const std::string& value) override;
  Ratio& Filter(const std::string& filter) override;
  Ratio& Join(const join_tree_t& tree, const std::vector<std::string>& keys = {}, const std::string& alias = "") override;
  Ratio& Entries(uint32_t nEntries) override;
  Ratio& Entries(uint32_t entryMin, uint32_t entryMax) override;

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;
  friend class Pad;

  boost::property_tree::ptree GetPropertyTree() const override;
  const auto& GetDenomDataSource() const { return mDenomDataSource; }
  const auto& GetDenomName() const { return mDenomName; }
  const bool& GetCorrelated() const { return mCorrelated; }
  const auto& GetDenomDataInfo() const { return mDenomDataInfo; }
  const auto& GetDenomProjInfo() const { return mDenomProjInfo; }
  const auto& GetNumModify() const { return mNumModify; }
  const auto& GetDenomModify() const { return mDenomModify; }

 private:
  enum class Mode { Res,
                    Num,
                    Den,
                    Both };
  Mode mModMode = Mode::Res;
  void ResolveSelectionMode(const char* call);
  modify_t mNumModify;
  modify_t mDenomModify;
  std::string mDenomName;
  std::string mDenomDataSource;
  bool mCorrelated{};
  data_info_t mDenomDataInfo;
  std::optional<proj_info_t> mDenomProjInfo;
};

//**************************************************************************************************
/**
 * Representation of an axis.
 */
//**************************************************************************************************
class Plot::Pad::Axis
{
 public:
  Axis() = default;
  void Print() const { Plot::Print(GetPropertyTree(), "Axis"); }

  Axis& SetTitle(const std::string& title);
  Axis& SetRange(double_t min, double_t max);
  Axis& SetMaxRange(double_t max);
  Axis& SetMinRange(double_t min);
  Axis& SetRoomAboveMax(double_t frameFraction);  // fraction of the frame kept free above everything drawn in the pad (0.3: the data end 30 % below the top)
  Axis& SetRoomBelowMin(double_t frameFraction);  // fraction of the frame kept free below everything drawn in the pad
  Axis& SetColor(color_t color);
  Axis& SetAxisColor(color_t color);
  Axis& SetAxisAlpha(float_t alpha);
  Axis& SetNumDivisions(int16_t numDivisions);
  Axis& SetMaxDigits(int32_t maxDigits);
  Axis& SetTickLength(float_t tickLength);
  Axis& SetTitleFont(int16_t font);
  Axis& SetLabelFont(int16_t font);
  Axis& SetTitleSize(float_t size);
  Axis& SetLabelSize(float_t size);
  Axis& SetTitleColor(color_t color);
  Axis& SetLabelColor(color_t color);
  Axis& SetTitleAlpha(float_t alpha);
  Axis& SetLabelAlpha(float_t alpha);
  Axis& SetTitleOffset(float_t offset);
  Axis& SetLabelOffset(float_t offset);
  Axis& SetLabelAngle(float_t angle);  // degrees, counter-clockwise: alphanumeric bin labels of an x axis (ROOT tilts no other labels)
  Axis& SetTitleCenter(bool center = true);
  Axis& SetLabelCenter(bool center = true);
  Axis& SetLog(bool isLog = true);
  Axis& SetGrid(bool isGrid = true);
  Axis& SetOppositeTicks(bool isOppositeTicks = true);
  Axis& SetNoExponent(bool isNoExponent = true);
  Axis& SetHideClippedLabels(bool hide = true);  // drop a label whose text would reach beyond the pad (e.g. at the edge shared with a touching pad); on by default in the base plots, false keeps the labels
  Axis& SetColorScaleOffset(float_t offset);     // Z axis: distance of the colour scale from the frame, fraction of the pad width (ROOT: 0.005)
  Axis& SetColorScaleWidth(float_t width);       // Z axis: width of the colour scale, fraction of the pad width (ROOT: 0.045)
  Axis& SetTimeFormat(const std::string& timeFormat);
  Axis& SetTickOrientation(const std::string& tickOrientation);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  explicit Axis(const char axisName);
  explicit Axis(const boost::property_tree::ptree& axisTree);

  boost::property_tree::ptree GetPropertyTree() const;
  void operator+=(const Axis& axis);

  const auto& GetMinRange() const { return mRange.min; }
  const auto& GetMaxRange() const { return mRange.max; }
  const auto& GetRoomAboveMax() const { return mRoom.aboveMax; }
  const auto& GetRoomBelowMin() const { return mRoom.belowMin; }
  const auto& GetTickLength() const { return mAxisFrame.scale; }
  const auto& GetNumDivisions() const { return mAxisFrame.style; }
  const auto& GetMaxDigits() const { return mAxisFrame.maxDigits; }
  const auto& GetAxisColor() const { return mAxisFrame.color; }
  const auto& GetAxisAlpha() const { return mAxisFrame.alpha; }
  const auto& GetTitle() const { return mAxisFrame.title; }
  const auto& GetTitleFont() const { return mTitleLayout.style; }
  const auto& GetLabelFont() const { return mLabelLayout.style; }
  const auto& GetTitleSize() const { return mTitleLayout.scale; }
  const auto& GetLabelSize() const { return mLabelLayout.scale; }
  const auto& GetTitleColor() const { return mTitleLayout.color; }
  const auto& GetLabelColor() const { return mLabelLayout.color; }
  const auto& GetTitleAlpha() const { return mTitleLayout.alpha; }
  const auto& GetLabelAlpha() const { return mLabelLayout.alpha; }
  const auto& GetTitleOffset() const { return mTitleLayout.offset; }
  const auto& GetLabelOffset() const { return mLabelLayout.offset; }
  const auto& GetLabelAngle() const { return mLabelLayout.angle; }
  const auto& GetTitleCenter() const { return mTitleLayout.center; }
  const auto& GetLabelCenter() const { return mLabelLayout.center; }
  const auto& GetLog() const { return mAxisFrame.isLog; }
  const auto& GetGrid() const { return mAxisFrame.isGrid; }
  const auto& GetOppositeTicks() const { return mAxisFrame.isOppositeTicks; }
  const auto& GetNoExponent() const { return mAxisFrame.isNoExponent; }
  const auto& GetHideClippedLabels() const { return mAxisFrame.hideClippedLabels; }
  const auto& GetColorScaleOffset() const { return mAxisFrame.colorScaleOffset; }
  const auto& GetColorScaleWidth() const { return mAxisFrame.colorScaleWidth; }
  const auto& GetTimeFormat() const { return mAxisFrame.timeFormat; }
  const auto& GetTickOrientation() const { return mAxisFrame.tickOrientation; }

 private:
  struct axis_layout_t : layout_t {
    std::optional<float_t> offset;
    std::optional<bool> center;
    std::optional<float_t> angle;
  };
  struct axis_range_t {
    std::optional<double_t> min;
    std::optional<double_t> max;
  };
  struct axis_room_t {
    std::optional<double_t> belowMin;
    std::optional<double_t> aboveMax;
  };
  struct axis_frame_t : layout_t {
    std::optional<std::string> title;
    std::optional<int32_t> maxDigits;
    std::optional<bool> isLog;
    std::optional<bool> isGrid;
    std::optional<bool> isOppositeTicks;
    std::optional<bool> isNoExponent;
    std::optional<bool> hideClippedLabels;
    std::optional<float_t> colorScaleOffset;
    std::optional<float_t> colorScaleWidth;
    std::optional<std::string> timeFormat;
    std::optional<std::string> tickOrientation;
  };

  char mName{};
  axis_frame_t mAxisFrame;
  axis_layout_t mTitleLayout;
  axis_layout_t mLabelLayout;
  axis_range_t mRange;
  axis_room_t mRoom;
};

//**************************************************************************************************
/**
 * Representation of a box.
 */
//**************************************************************************************************
template <class BoxType>
class Plot::Pad::Box : public BorderSetters<BoxType>, public TextSetters<BoxType>, public FillSetters<BoxType>
{
 public:
  virtual ~Box() = default;
  Box(const Box& other) = default;
  Box(Box&&) = default;
  Box& operator=(const Box& other) = default;
  Box& operator=(Box&& other) = default;
  void Print() const { Plot::Print(GetPropertyTree(), "Box"); }

  BoxType& SetPosition(double_t x, double_t y);
  BoxType& SetSize(double_t width, double_t height);
  BoxType& SetUserCoordinates(bool isUserCoord = true);
  BoxType& SetAutoPlacement(box_placement_t placement = best_corner);
  BoxType& SetTransparent();
  BoxType& SetNoBox();
  BoxType& SetMargin(float_t margin);
  BoxType& SetLineSpacing(float_t lineSpacing);

 protected:
  boost::property_tree::ptree GetPropertyTree() const;

  double_t GetXPosition() const { return (mPos.x) ? *mPos.x : 0.; }
  double_t GetYPosition() const { return (mPos.y) ? *mPos.y : 0.; }
  auto& GetWidth() const { return mPos.w; }
  auto& GetHeight() const { return mPos.h; }
  auto& GetPlacement() const { return mPos.placement; }
  auto& GetMargin() const { return mMargin; }
  auto& GetLineSpacing() const { return mLineSpacing; }

  bool IsUserCoordinates() const { return (mPos.isUserCoord) ? *mPos.isUserCoord : false; }
  bool IsAutoPlacement() const { return mPos.placement.has_value(); }

 private:
  // allow construction of Box base class only in context actually useful boxes
  friend BoxType;
  Box() = default;
  Box(double_t xPos, double_t yPos);
  explicit Box(const boost::property_tree::ptree& boxTree);

  auto GetThis() { return static_cast<BoxType*>(this); }

  struct position_t {
    std::optional<double_t> x;
    std::optional<double_t> y;
    std::optional<double_t> w;
    std::optional<double_t> h;
    std::optional<bool> isUserCoord;
    std::optional<box_placement_t> placement;  // automatic placement (instead of the position given by x and y)
  };

  std::optional<float_t> mMargin;
  std::optional<float_t> mLineSpacing;
  position_t mPos;
};

//**************************************************************************************************
/**
 * Representation of a text box.
 */
//**************************************************************************************************
class Plot::Pad::TextBox : public Plot::Pad::Box<TextBox>
{
 public:
  explicit TextBox(const std::string& text);
  explicit TextBox(const std::vector<std::string>& lines);
  TextBox(double_t xPos, double_t yPos, const std::string& text);
  TextBox(double_t xPos, double_t yPos, const std::vector<std::string>& lines);
  explicit TextBox(const boost::property_tree::ptree& textBoxTree);

  virtual ~TextBox() = default;
  TextBox(const TextBox& other) = default;
  TextBox(TextBox&&) = default;
  TextBox& operator=(const TextBox& other) = default;
  TextBox& operator=(TextBox&& other) = default;
  void Print() const { Plot::Print(GetPropertyTree(), "TextBox"); }

  using TextSetters<TextBox>::SetText;
  TextBox& SetText(const std::string& text);  // lines separated by newlines
  TextBox& SetText(const std::vector<std::string>& lines);
  TextBox& SetText(std::initializer_list<std::string> lines);
  TextBox& SetTextAlign(int16_t align);  // 11 to 33 (default 12) for the lines in the box; a single line at (x, y) is aligned at that point (21: centred above it)

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  boost::property_tree::ptree GetPropertyTree() const;
  const std::string& GetText() const { return mText; }
  std::vector<std::string> GetLines() const;
  const auto& GetTextAlign() const { return mTextAlign; }

 private:
  std::string mText;  // lines separated by newlines
  std::optional<int16_t> mTextAlign;
};

//**************************************************************************************************
/**
 * Representation of an image (e.g. a logo) drawn in a pad. Its position is the upper left corner (in pad or user coordinates),
 * as for texts; a size not given follows from the other one and the aspect ratio of the image.
 */
//**************************************************************************************************
class Plot::Pad::ImageBox
{
 public:
  ImageBox(double_t xPos, double_t yPos, const std::string& path);
  explicit ImageBox(const std::string& path);
  explicit ImageBox(const boost::property_tree::ptree& imageBoxTree);
  void Print() const { Plot::Print(GetPropertyTree(), "ImageBox"); }

  ImageBox& SetPosition(double_t x, double_t y);
  ImageBox& SetWidth(double_t width);    // fraction of the pad width
  ImageBox& SetHeight(double_t height);  // fraction of the pad height
  ImageBox& SetUserCoordinates(bool isUserCoord = true);
  ImageBox& SetAutoPlacement(box_placement_t placement = best_corner);

 protected:
  friend class PlotPainter;
  friend class Plot;

  boost::property_tree::ptree GetPropertyTree() const;
  const std::string& GetPath() const { return mPath; }
  double_t GetXPosition() const { return mX.value_or(0.); }
  double_t GetYPosition() const { return mY.value_or(0.); }
  const auto& GetWidth() const { return mWidth; }
  const auto& GetHeight() const { return mHeight; }
  const auto& GetPlacement() const { return mPlacement; }
  bool IsUserCoordinates() const { return mIsUserCoord.value_or(false); }
  bool IsAutoPlacement() const { return mPlacement.has_value(); }

 private:
  std::string mPath;  // as given (environment variables are expanded when the plot is drawn), a relative path made absolute
  std::optional<double_t> mX;
  std::optional<double_t> mY;
  std::optional<double_t> mWidth;
  std::optional<double_t> mHeight;
  std::optional<bool> mIsUserCoord;
  std::optional<box_placement_t> mPlacement;
};

//**************************************************************************************************
/**
 * Placement and appearance of the statistics box of a data item (Data::SetStatsBox), whose content is set with Data::SetShowStats.
 */
//**************************************************************************************************
class Plot::Pad::StatsBox : public Plot::Pad::Box<StatsBox>
{
 public:
  StatsBox();
  explicit StatsBox(const boost::property_tree::ptree& statsBoxTree);
  void Print() const { Plot::Print(GetPropertyTree(), "StatsBox"); }

 protected:
  friend class PlotPainter;
  friend class Data;

  boost::property_tree::ptree GetPropertyTree() const;
};

//**************************************************************************************************
/**
 * A shape drawn in a pad that is not data: a line, an arrow, a region, an ellipse or a polygon (Pad::AddLine, AddArrow, AddRegion, ...).
 * It never enters the axis ranges, is not numbered with the data, takes no default colours and has no legend row.
 */
//**************************************************************************************************
class Plot::Pad::Shape : public LineSetters<Shape>, public FillSetters<Shape>
{
 public:
  explicit Shape(const boost::property_tree::ptree& shapeTree);
  void Print() const { Plot::Print(GetPropertyTree(), "Shape"); }

  Shape& SetUserCoordinates(bool isUserCoord = true);  // false: positions as fractions of the pad (0 to 1) instead of axis values
  Shape& SetBehindData();                              // drawn after the frame and before the data (default: after the data)
  Shape& SetOnTop();                                   // drawn after the legends and texts (default: before them, so that they avoid it)
  Shape& SetArrowHead(const std::string& style);       // the option of TArrow: "|>" (default), ">", "<|>", "<>", "-|>-", ...
  Shape& SetArrowSize(float_t size);                   // length of the arrow head as fraction of the pad height (default 0.03)
  Shape& SetArc(double_t phiMin, double_t phiMax);     // part of an ellipse, in degrees (default: 0 to 360)
  Shape& SetRotation(double_t theta);                  // rotation of an ellipse, in degrees

 protected:
  friend class PlotPainter;
  friend class PlotManager;
  friend class Pad;

  enum class kind_t : uint8_t { line,
                                arrow,
                                vertical_line,
                                horizontal_line,
                                region,
                                region_x,
                                region_y,
                                ellipse,
                                polygon };
  enum class layer_t : uint8_t { behind_data,
                                 after_data,
                                 on_top };
  Shape(kind_t kind, const std::vector<double_t>& coordinates);
  boost::property_tree::ptree GetPropertyTree() const;
  static const char* KindName(kind_t kind);   // as stored: "vertical_line"
  static std::string KindLabel(kind_t kind);  // for messages: "vertical line", "region X"
  // the drawn object of a shape carries it as TObject::SetUniqueID, so that a double-click in show mode names it like the user added it:
  // its kind, its number among the shapes of this kind in the pad (the nth AddArrow), and whether it is in fractions of the pad
  struct drawn_id_t {
    kind_t kind;
    uint16_t number;
    bool isUserCoord;
  };
  static uint32_t EncodeDrawnID(const drawn_id_t& id);
  static std::optional<drawn_id_t> DecodeDrawnID(uint32_t uniqueID);

  auto GetKind() const { return mKind; }
  const auto& GetCoordinates() const { return mCoordinates; }
  bool IsUserCoordinates() const { return mIsUserCoord.value_or(true); }
  layer_t GetLayer() const { return mLayer.value_or(layer_t::after_data); }
  const auto& GetArrowHead() const { return mArrowHead; }
  const auto& GetArrowSize() const { return mArrowSize; }
  const auto& GetArc() const { return mArc; }
  const auto& GetRotation() const { return mRotation; }

 private:
  kind_t mKind{kind_t::line};
  std::vector<double_t> mCoordinates;  // line, arrow, region: x1, y1, x2, y2; vertical or horizontal line: x or y; region in x or y: x1, x2 or y1, y2; ellipse: x, y, rx, ry; polygon: x1, y1, x2, y2, ...
  std::optional<bool> mIsUserCoord;
  std::optional<layer_t> mLayer;
  std::optional<std::string> mArrowHead;
  std::optional<float_t> mArrowSize;
  std::optional<std::pair<double_t, double_t>> mArc;
  std::optional<double_t> mRotation;
};

//**************************************************************************************************
/**
 * Representation of a legend box.
 */
//**************************************************************************************************
class Plot::Pad::LegendBox : public Plot::Pad::Box<LegendBox>
{
 public:
  explicit LegendBox(const std::optional<std::string>& title = {});
  LegendBox(double_t xPos, double_t yPos, const std::optional<std::string>& title = {});
  explicit LegendBox(const boost::property_tree::ptree& legendBoxTree);

  virtual ~LegendBox() = default;
  LegendBox(const LegendBox& other) = default;
  LegendBox(LegendBox&&) = default;
  LegendBox& operator=(const LegendBox& other) = default;
  LegendBox& operator=(LegendBox&& other) = default;
  void Print() const { Plot::Print(GetPropertyTree(), "LegendBox"); }

  LegendEntry& AddEntry(const std::string& label = "");  // a row of its own (with an empty label: an empty line)
  // a row for data drawn in another pad (dataID as in plot[padID](dataID)), with its symbol, style and legend label
  LegendEntry& AddDataEntry(uint8_t padID, uint16_t dataID);
  LegendBox& SetTitle(const std::string& title);
  LegendBox& SetNumColumns(uint8_t numColumns);
  LegendBox& SetColumnSpacing(float_t spacing);  // space after the widest label of a column, in units of the text height (default 1)
  LegendBox& SetDefaultSymbolElements(const std::string& symbolElements);
  LegendBox& SetDefaultColor(color_t color);
  LegendBox& SetDefaultAlpha(float_t alpha);
  LegendBox& SetDefaultLineColor(color_t color);
  LegendBox& SetDefaultLineAlpha(float_t alpha);
  LegendBox& SetDefaultLineStyle(int16_t style);
  LegendBox& SetDefaultLineWidth(float_t width);
  LegendBox& SetDefaultMarkerColor(color_t color);
  LegendBox& SetDefaultMarkerAlpha(float_t alpha);
  LegendBox& SetDefaultMarkerStyle(int16_t style);
  LegendBox& SetDefaultMarkerSize(float_t size);
  LegendBox& SetDefaultFillColor(color_t color);
  LegendBox& SetDefaultFillAlpha(float_t alpha);
  LegendBox& SetDefaultFillStyle(int16_t style);
  LegendBox& SetSymbolColScale(float_t scale);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  boost::property_tree::ptree GetPropertyTree() const;
  const std::optional<uint8_t>& GetNumColumns() const { return mNumColumns; }
  const std::optional<float_t>& GetColumnSpacing() const { return mColumnSpacing; }
  const std::optional<std::string>& GetTitle() const { return mTitle; }

  void AddDataRow(const std::string& label, uint16_t dataID, const std::optional<LegendEntry>& appearance);
  bool HasForeignRows() const;  // rows for data of other pads: the legend is made once all pads are drawn
  void ShiftDataIDs(uint16_t offset);

  const auto& GetEntries() const { return mLegendEntries; }
  const auto& GetDefaultSymbolElements() const { return mSymbolElementsDefault; }
  const auto& GetDefaultMarkerColor() const { return mMarkerDefault.color; }
  const auto& GetDefaultMarkerAlpha() const { return mMarkerDefault.alpha; }
  const auto& GetDefaultMarkerStyle() const { return mMarkerDefault.style; }
  const auto& GetDefaultMarkerSize() const { return mMarkerDefault.scale; }
  const auto& GetDefaultLineColor() const { return mLineDefault.color; }
  const auto& GetDefaultLineAlpha() const { return mLineDefault.alpha; }
  const auto& GetDefaultLineStyle() const { return mLineDefault.style; }
  const auto& GetDefaultLineWidth() const { return mLineDefault.scale; }
  const auto& GetDefaultFillColor() const { return mFillDefault.color; }
  const auto& GetDefaultFillAlpha() const { return mFillDefault.alpha; }
  const auto& GetDefaultFillStyle() const { return mFillDefault.style; }
  const auto& GetSymbolColScale() const { return mSymbolColScale; }

 private:
  std::optional<std::string> mTitle;
  std::optional<uint8_t> mNumColumns;
  std::optional<float_t> mColumnSpacing;
  std::vector<LegendEntry> mLegendEntries;     // the rows as drawn, generated when the plot is drawn
  std::deque<LegendEntry> mLegendEntriesUser;  // the rows added with AddEntry, in the order of the calls (a deque keeps references valid)

  layout_t mLineDefault;
  layout_t mMarkerDefault;
  layout_t mFillDefault;
  std::optional<std::string> mSymbolElementsDefault;
  std::optional<float_t> mSymbolColScale;

  void MergeLegendEntries(uint16_t nData);
};

}  // end namespace SciRooPlot
#endif  // INCLUDE_SCIROOPLOT_PLOT_H_
