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

#include "SciRooPlot/Plot.h"

#include "SciRooPlot/Logger.h"
#include "SciRooPlot/PanelLayout.h"

#include <TAttText.h>
#include <TColor.h>
#include <TROOT.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <istream>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Validation.h"
#include "util/Optional.h"
#include "util/Paths.h"
#include "util/PropertyTree.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"

using boost::property_tree::ptree;
using std::nullopt;
using std::optional;
using std::pair;
using std::string;
using std::tuple;
using std::vector;

namespace SciRooPlot
{
using util::expand_path;
using util::get_from_tree;
using util::is_blank;
using util::join_strings;
using util::number_to_string;
using util::put_in_tree;
using util::read_from_tree;
using util::split_path;
using util::str_contains;
using util::str_starts_with;

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class color_t
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Colour from "#rrggbb" (hexadecimal digits, upper or lower case).
 */
//**************************************************************************************************
color_t::color_t(const string& hex) : mIndex(nullopt)
{
  const bool valid = (hex.size() == 7 && hex[0] == '#' && std::all_of(hex.begin() + 1, hex.end(), [](unsigned char c) { return std::isxdigit(c); }));
  if (!valid) logger::throw_invalid_argument("Colour '{}' is not valid: give a ROOT colour (e.g. kRed + 1), \"#rrggbb\" or {{r, g, b}} with components from 0 to 1.", hex);
  for (size_t i = 0; i < 3; ++i) {
    mRGB[i] = static_cast<uint8_t>(std::stoi(hex.substr(1 + 2 * i, 2), nullptr, 16));
  }
}

//**************************************************************************************************
/**
 * Colour from its red, green and blue components (from 0 to 1), stored with the 8 bits per component that ROOT uses.
 */
//**************************************************************************************************
color_t::color_t(double_t red, double_t green, double_t blue) : mIndex(nullopt)
{
  const std::array<double_t, 3> components{red, green, blue};
  for (size_t i = 0; i < 3; ++i) {
    if (!(components[i] >= 0. && components[i] <= 1.)) {
      logger::throw_invalid_argument("Colour {{{}, {}, {}}} is not valid: the components go from 0 to 1.", red, green, blue);
    }
    mRGB[i] = static_cast<uint8_t>(std::lround(components[i] * 255.));
  }
}

//**************************************************************************************************
/**
 * The colour as stored in the plot definitions: the index as number, an rgb colour as "#rrggbb".
 */
//**************************************************************************************************
string color_t::ToString() const
{
  if (mIndex) return std::to_string(*mIndex);
  return fmt::format("#{:02x}{:02x}{:02x}", mRGB[0], mRGB[1], mRGB[2]);
}

//**************************************************************************************************
/**
 * Index of the colour for ROOT: an rgb colour gets the index of an existing ROOT colour with the same components or of a new one.
 */
//**************************************************************************************************
int16_t color_t::ToIndex() const
{
  if (mIndex) {
    // ROOT draws an index it does not know (also one made by the definition program, which does not exist here) in black, without a word
    static std::unordered_set<int16_t> reported;
    if (!logger::muted() && !gROOT->GetColor(*mIndex) && reported.insert(*mIndex).second) {
      logger::warning("Colour index {} is not defined in ROOT: drawn black. Give the colour as \"#rrggbb\" or (r, g, b).", *mIndex);
    }
    return *mIndex;
  }
  return static_cast<int16_t>(TColor::GetColor(static_cast<Int_t>(mRGB[0]), static_cast<Int_t>(mRGB[1]), static_cast<Int_t>(mRGB[2])));
}

//**************************************************************************************************
/**
 * Red, green and blue from 0 to 1; for an index those of the ROOT colour.
 */
//**************************************************************************************************
std::array<float_t, 3> color_t::ToFractions() const
{
  if (mIndex) {
    const TColor* color = gROOT->GetColor(*mIndex);
    if (!color) logger::throw_invalid_argument("Colour index {} is not defined in ROOT.", *mIndex);
    return {color->GetRed(), color->GetGreen(), color->GetBlue()};
  }
  return {mRGB[0] / 255.f, mRGB[1] / 255.f, mRGB[2] / 255.f};
}

std::ostream& operator<<(std::ostream& stream, const color_t& color)
{
  return stream << color.ToString();
}

std::istream& operator>>(std::istream& stream, color_t& color)
{
  string token;
  if (!(stream >> token)) return stream;
  try {
    color = (token[0] == '#') ? color_t(token) : color_t(util::string_to_type<int16_t>(token));
  } catch (const std::exception&) {
    stream.setstate(std::ios::failbit);
  }
  return stream;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Plot
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Helper that rounds line widths.
 */
//**************************************************************************************************
float_t Plot::RoundWidth(float_t width, const char* what)
{
  const float_t rounded = std::round(width);
  if (rounded != width) {
    logger::warning("The {} {} is not an integer; ROOT stores line widths as short, using {}.", what, width, rounded);
  }
  return rounded;
}
bool Plot::CheckAlpha(float_t alpha, const string& what)
{
  if (alpha >= 0.f && alpha <= 1.f) return true;
  logger::warning("The {} ({}) is ignored: it is outside of 0 (transparent) to 1 (opaque).", what, alpha);
  return false;
}
string Plot::Shown(double value)
{
  // a float_t widened to double (1.2f becomes 1.2000000476837158) is written as the float that was given
  const auto given = static_cast<float_t>(value);
  return (static_cast<double>(given) == value) ? fmt::format("{}", given) : fmt::format("{}", value);
}
bool Plot::CheckFinite(double value, const string& what)
{
  if (std::isfinite(value)) return true;
  logger::warning("The {} ({}) is ignored: it is not a finite number.", what, Shown(value));
  return false;
}
bool Plot::CheckMarkerStyle(int16_t style, const string& what)
{
  // ROOT draws markers 1 to 49, from 50 on the same with thicker lines (width 2 for 50 to 67, 3 for 68 to 85 ...); absurd widths crash it
  constexpr int16_t maxStyle = 50 + 18 * 9 - 1;  // line width 10
  if (style >= 0 && style <= maxStyle) return true;
  logger::warning("The {} ({}) is ignored: ROOT has markers 1 to 49 and their thicker versions up to line width 10 (styles up to {}).", what, style, maxStyle);
  return false;
}
bool Plot::CheckNotNegative(double value, const string& what)
{
  if (!CheckFinite(value, what)) return false;
  if (value >= 0.) return true;
  logger::warning("The {} ({}) is ignored: it is negative.", what, Shown(value));
  return false;
}
bool Plot::CheckFraction(double value, const string& what)
{
  if (value >= 0. && value <= 1.) return true;
  logger::warning("The {} ({}) is ignored: it is outside of 0 to 1.", what, Shown(value));
  return false;
}
bool Plot::CheckCount(int64_t count, const string& what)
{
  if (count >= 1) return true;
  logger::warning("The {} ({}) is ignored: it is not a positive number.", what, count);
  return false;
}
bool Plot::CheckPixels(int64_t size, const string& what)
{
  // X11 (and ROOT's bitmaps after it) cannot make a window or image side beyond 32767 pixels: painting such a canvas crashes
  if (!CheckCount(size, what)) return false;
  if (size <= 32767) return true;
  logger::warning("The {} ({}) is ignored: it is larger than 32767 pixels, the largest side of a canvas.", what, size);
  return false;
}
bool Plot::CheckFont(int16_t font)
{
  if (font / 10 >= 1 && font / 10 <= 15 && font % 10 <= 3) return true;
  logger::warning("The text font ({}) is ignored: it is not a ROOT font (10 * font number from 1 to 15 + precision from 0 to 3).", font);
  return false;
}
void Plot::Quietly(bool quiet, const std::function<void()>& call)
{
  if (!quiet) return call();
  const bool wasMuted = std::exchange(logger::muted(), true);
  auto restore = util::make_scope_guard([wasMuted]() { logger::muted() = wasMuted; });
  call();
}
optional<float_t> Plot::RoundWidth(const optional<float_t>& width, const char* what)
{
  if (!width) return nullopt;
  return RoundWidth(*width, what);
}
void Plot::ReadWidthFromTree(const ptree& tree, optional<float_t>& target, const char* key)
{
  read_from_tree(tree, target, key);
  target = RoundWidth(target, key);
}

//**************************************************************************************************
/**
 * Read and write the keys of a layout.
 */
//**************************************************************************************************
void Plot::ReadLayout(const ptree& tree, layout_t& layout, const string& prefix, const char* style, const char* scale)
{
  read_from_tree(tree, layout.color, prefix + "color");
  read_from_tree(tree, layout.alpha, prefix + "alpha");
  read_from_tree(tree, layout.style, prefix + style);
  if (!scale) return;
  const string scaleKey = prefix + scale;
  if (std::string_view(scale) == "width") {
    ReadWidthFromTree(tree, layout.scale, scaleKey.c_str());
  } else {
    read_from_tree(tree, layout.scale, scaleKey);
  }
}
void Plot::PutLayout(ptree& tree, const layout_t& layout, const string& prefix, const char* style, const char* scale)
{
  put_in_tree(tree, layout.color, prefix + "color");
  put_in_tree(tree, layout.alpha, prefix + "alpha");
  put_in_tree(tree, layout.style, prefix + style);
  if (scale) put_in_tree(tree, layout.scale, prefix + scale);
}

//**************************************************************************************************
/**
 * Take over the values set in other.
 */
//**************************************************************************************************
void layout_t::Update(const layout_t& other)
{
  if (other.color) color = other.color;
  if (other.alpha) alpha = other.alpha;
  if (other.style) style = other.style;
  if (other.scale) scale = other.scale;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION of the setter groups (marker, line, fill, text, border)
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

template <class Derived>
Derived& MarkerSetters<Derived>::SetMarker(int16_t style, float_t size, color_t color, float_t alpha)
{
  bool valid = Plot::CheckFinite(size, "marker size");
  valid = Plot::CheckAlpha(alpha, "marker alpha") && valid;
  valid = Plot::CheckMarkerStyle(style, "marker style") && valid;
  if (valid) mMarker = {color, alpha, style, size};
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& MarkerSetters<Derived>::SetMarkerColor(color_t color)
{
  mMarker.color = color;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& MarkerSetters<Derived>::SetMarkerAlpha(float_t alpha)
{
  if (Plot::CheckAlpha(alpha, "marker alpha")) mMarker.alpha = alpha;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& MarkerSetters<Derived>::SetMarkerStyle(int16_t style)
{
  if (Plot::CheckMarkerStyle(style, "marker style")) mMarker.style = style;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& MarkerSetters<Derived>::SetMarkerSize(float_t size)
{
  if (Plot::CheckFinite(size, "marker size")) mMarker.scale = size;
  return static_cast<Derived&>(*this);
}
template <class Derived>
void MarkerSetters<Derived>::ReadMarker(const ptree& tree)
{
  Plot::ReadLayout(tree, mMarker, "marker_", "style", "size");
}
template <class Derived>
void MarkerSetters<Derived>::PutMarker(ptree& tree) const
{
  Plot::PutLayout(tree, mMarker, "marker_", "style", "size");
}

template <class Derived>
Derived& LineSetters<Derived>::SetLine(int16_t style, float_t width, color_t color, float_t alpha)
{
  bool valid = Plot::CheckFinite(width, "line width");
  valid = Plot::CheckAlpha(alpha, "line alpha") && valid;
  if (valid) mLine = {color, alpha, style, Plot::RoundWidth(width, "line width")};
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& LineSetters<Derived>::SetLineColor(color_t color)
{
  mLine.color = color;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& LineSetters<Derived>::SetLineAlpha(float_t alpha)
{
  if (Plot::CheckAlpha(alpha, "line alpha")) mLine.alpha = alpha;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& LineSetters<Derived>::SetLineStyle(int16_t style)
{
  mLine.style = style;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& LineSetters<Derived>::SetLineWidth(float_t width)
{
  if (Plot::CheckFinite(width, "line width")) mLine.scale = Plot::RoundWidth(width, "line width");
  return static_cast<Derived&>(*this);
}
template <class Derived>
void LineSetters<Derived>::ReadLine(const ptree& tree)
{
  Plot::ReadLayout(tree, mLine, "line_", "style", "width");
}
template <class Derived>
void LineSetters<Derived>::PutLine(ptree& tree) const
{
  Plot::PutLayout(tree, mLine, "line_", "style", "width");
}

template <class Derived>
Derived& FillSetters<Derived>::SetFill(int16_t style, color_t color, float_t alpha)
{
  if (Plot::CheckAlpha(alpha, "fill alpha")) mFill = {color, alpha, style, {}};
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& FillSetters<Derived>::SetFillColor(color_t color)
{
  mFill.color = color;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& FillSetters<Derived>::SetFillStyle(int16_t style)
{
  mFill.style = style;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& FillSetters<Derived>::SetFillAlpha(float_t alpha)
{
  if (Plot::CheckAlpha(alpha, "fill alpha")) mFill.alpha = alpha;
  return static_cast<Derived&>(*this);
}
template <class Derived>
void FillSetters<Derived>::ReadFill(const ptree& tree)
{
  Plot::ReadLayout(tree, mFill, "fill_", "style");
}
template <class Derived>
void FillSetters<Derived>::PutFill(ptree& tree) const
{
  Plot::PutLayout(tree, mFill, "fill_", "style");
}

template <class Derived>
Derived& TextSetters<Derived>::SetText(int16_t font, float_t size, color_t color, float_t alpha)
{
  bool valid = Plot::CheckFont(font);
  valid = Plot::CheckNotNegative(size, "text size") && valid;
  valid = Plot::CheckAlpha(alpha, "text alpha") && valid;
  if (valid) mText = {color, alpha, font, size};
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& TextSetters<Derived>::SetTextColor(color_t color)
{
  mText.color = color;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& TextSetters<Derived>::SetTextAlpha(float_t alpha)
{
  if (Plot::CheckAlpha(alpha, "text alpha")) mText.alpha = alpha;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& TextSetters<Derived>::SetTextFont(int16_t font)
{
  if (Plot::CheckFont(font)) mText.style = font;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& TextSetters<Derived>::SetTextSize(float_t size)
{
  if (Plot::CheckNotNegative(size, "text size")) mText.scale = size;
  return static_cast<Derived&>(*this);
}
template <class Derived>
void TextSetters<Derived>::ReadText(const ptree& tree)
{
  Plot::ReadLayout(tree, mText, "text_", "font", "size");
}
template <class Derived>
void TextSetters<Derived>::PutText(ptree& tree) const
{
  Plot::PutLayout(tree, mText, "text_", "font", "size");
}

template <class Derived>
Derived& BorderSetters<Derived>::SetBorder(int16_t style, float_t width, color_t color, float_t alpha)
{
  bool valid = Plot::CheckFinite(width, "border width");
  valid = Plot::CheckAlpha(alpha, "border alpha") && valid;
  if (valid) mBorder = {color, alpha, style, Plot::RoundWidth(width, "border width")};
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& BorderSetters<Derived>::SetBorderColor(color_t color)
{
  mBorder.color = color;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& BorderSetters<Derived>::SetBorderAlpha(float_t alpha)
{
  if (Plot::CheckAlpha(alpha, "border alpha")) mBorder.alpha = alpha;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& BorderSetters<Derived>::SetBorderStyle(int16_t style)
{
  mBorder.style = style;
  return static_cast<Derived&>(*this);
}
template <class Derived>
Derived& BorderSetters<Derived>::SetBorderWidth(float_t width)
{
  if (Plot::CheckFinite(width, "border width")) mBorder.scale = Plot::RoundWidth(width, "border width");
  return static_cast<Derived&>(*this);
}
template <class Derived>
void BorderSetters<Derived>::ReadBorder(const ptree& tree)
{
  Plot::ReadLayout(tree, mBorder, "border_", "style", "width");
}
template <class Derived>
void BorderSetters<Derived>::PutBorder(ptree& tree) const
{
  Plot::PutLayout(tree, mBorder, "border_", "style", "width");
}

//**************************************************************************************************
/**
 * Why a plot name or group cannot be used (nullopt: it can).
 */
//**************************************************************************************************
optional<string> Plot::NameError(const string& name, bool isGroup)
{
  if (!isGroup && name.empty()) return "Plot name must not be empty.";
  if (auto illegal = find_illegal_name_char(name, isGroup)) {
    return fmt::format("{} '{}' contains illegal character '{}'.", (isGroup) ? "Group" : "Plot name", name, *illegal);
  }
  return nullopt;
}

//**************************************************************************************************
/**
 * Helper to strip group of misplaced slashes.
 */
//**************************************************************************************************
string Plot::NormalizeGroup(const string& group)
{
  string normalized;
  normalized.reserve(group.size());
  for (char c : group) {
    if (c == '/' && (normalized.empty() || normalized.back() == '/')) continue;
    normalized += c;
  }
  while (!normalized.empty() && normalized.back() == '/') {
    normalized.pop_back();
  }
  if (normalized != group) {
    logger::warning("Group '{}' was changed to '{}' (leading, trailing and repeated slashes are removed).", group, normalized);
  }
  return normalized;
}

//**************************************************************************************************
/**
 * Enums stored by name.
 */
//**************************************************************************************************
template <typename T>
void Plot::ReadNamed(const ptree& tree, optional<T>& value, const char* key)
{
  if (auto name = tree.get_optional<string>(key)) {
    value = enum_from_name<T>(*name);
    if (!value) logger::warning("Unknown {} '{}' is ignored.", enum_names<T>::what, *name);
  }
}
template <typename T>
void Plot::PutNamed(ptree& tree, const optional<T>& value, const char* key)
{
  if (value) tree.put(key, enum_name(*value));
}

//**************************************************************************************************
/**
 * Constructor of a plot with name, group and optional base plot.
 */
//**************************************************************************************************
Plot::Plot(const string& name, const string& group, const optional<string>& basePlot) : Plot()
{
  if (auto error = NameError(name)) logger::throw_invalid_argument("{}", *error);
  if (auto error = NameError(group, true)) logger::throw_invalid_argument("{}", *error);
  mName = name;
  mGroup = NormalizeGroup(group);
  mBasePlot = basePlot;
  UpdateUniqueName();
}

//**************************************************************************************************
/**
 * Constructor of a base plot from a layout.
 */
//**************************************************************************************************
Plot::Plot(const string& name, const PanelLayout& layout) : Plot(name)
{
  layout.ApplyTo(*this);
}

//**************************************************************************************************
/**
 * Constructor from existing plot.
 */
//**************************************************************************************************
Plot::Plot(const Plot& other, const string& name, const optional<string>& group)
  : Plot(other)
{
  if (auto error = NameError(name)) logger::throw_invalid_argument("{}", *error);
  if (group) {
    if (auto error = NameError(*group, true)) logger::throw_invalid_argument("{}", *error);
    mGroup = NormalizeGroup(*group);
  }
  mName = name;
  UpdateUniqueName();
}

//**************************************************************************************************
/**
 * Constructor from property tree.
 */
//**************************************************************************************************
Plot::Plot(const ptree& plotTree)
{
  try {
    mName = plotTree.get<string>("name");
    mGroup = plotTree.get<string>("group");
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct plot from ptree: {}", e.what());
  }
  if (auto error = NameError(mName)) logger::throw_invalid_argument("{}", *error);
  if (auto error = NameError(mGroup, true)) logger::throw_invalid_argument("{}", *error);
  mGroup = NormalizeGroup(mGroup);
  read_from_tree(plotTree, mBasePlot, "base_plot");
  read_from_tree(plotTree, mPlotDimensions.width, "width");
  read_from_tree(plotTree, mPlotDimensions.height, "height");
  read_from_tree(plotTree, mPlotDimensions.fixAspectRatio, "fix_aspect_ratio");
  ReadFill(plotTree);

  // loop over all pads defined in property tree
  // the pads of a shared axis or common title
  auto readPads = [](const ptree& tree) { return ReadPadList(get_from_tree<string>(tree, "pads").value_or(""), "pads"); };
  auto readAxis = [](const ptree& tree, const string& key) {
    const auto axis = get_from_tree<char>(tree, "axis");
    if (!axis) logger::throw_invalid_argument("{} has no axis", key);
    return *axis;
  };
  for (const auto& [key, child] : plotTree) {
    if (str_starts_with(key, "PAD_")) {
      int32_t padID{};
      try {
        padID = util::string_to_type<int32_t>(key.substr(4));
      } catch (const std::exception&) {
        logger::throw_invalid_argument("Bad key {}: a pad is PAD_<number>", key);
      }
      mPads[CheckedPadID(padID)] = Pad(child);
    } else if (str_starts_with(key, "SHARED_AXIS_")) {
      mSharedAxes.push_back({readAxis(child, key), readPads(child), get_from_tree<bool>(child, "layout").value_or(false)});
    } else if (str_starts_with(key, "COMMON_TITLE_")) {
      mCommonTitles.emplace_back(readAxis(child, key), readPads(child));
    }
  }
  UpdateUniqueName();
}

//**************************************************************************************************
/**
 * Update the unique internal name of the plot based on the current name and group.
 */
//**************************************************************************************************
void Plot::UpdateUniqueName()
{
  mUniqueName = mName + ":" + mGroup;
}

//**************************************************************************************************
/**
 * A pad number read from a saved plot: a number above 255 would wrap around and replace another pad.
 */
//**************************************************************************************************
uint8_t Plot::CheckedPadID(int64_t padID)
{
  if (padID < 0 || padID > std::numeric_limits<uint8_t>::max()) logger::throw_invalid_argument("Pad number {} is out of range (0 to 255).", padID);
  return static_cast<uint8_t>(padID);
}

//**************************************************************************************************
/**
 * The pads of a list read from a saved plot (the value of the given key): pad numbers separated by spaces.
 */
//**************************************************************************************************
vector<uint8_t> Plot::ReadPadList(const string& list, const string& key)
{
  vector<uint8_t> pads;
  std::istringstream stream(list);
  for (string padID; stream >> padID;) {
    int64_t number{};
    try {
      number = util::string_to_type<int32_t>(padID);
    } catch (const std::exception&) {
      logger::throw_invalid_argument("Bad value '{}' of {}: pad numbers separated by spaces", list, key);
    }
    pads.push_back(CheckedPadID(number));
  }
  return pads;
}

//**************************************************************************************************
/**
 * A list of pads as saved: pad numbers separated by spaces.
 */
//**************************************************************************************************
string Plot::WritePadList(const vector<uint8_t>& pads)
{
  string list;
  for (uint8_t padID : pads) {
    list += (list.empty() ? "" : " ") + std::to_string(padID);
  }
  return list;
}

//**************************************************************************************************
/**
 * Get representation of plot as property tree.
 */
//**************************************************************************************************
ptree Plot::GetPropertyTree() const
{
  ptree plotTree;
  plotTree.put("name", mName);
  plotTree.put("group", mGroup);
  put_in_tree(plotTree, mBasePlot, "base_plot");
  put_in_tree(plotTree, mPlotDimensions.width, "width");
  put_in_tree(plotTree, mPlotDimensions.height, "height");
  put_in_tree(plotTree, mPlotDimensions.fixAspectRatio, "fix_aspect_ratio");
  PutFill(plotTree);

  for (const auto& [padID, pad] : mPads) {
    plotTree.put_child("PAD_" + std::to_string(padID), pad.GetPropertyTree());
  }
  for (size_t i = 0; i < mSharedAxes.size(); ++i) {
    ptree group;
    group.put("axis", mSharedAxes[i].axis);
    group.put("pads", WritePadList(mSharedAxes[i].pads));
    if (mSharedAxes[i].fromLayout) group.put("layout", true);
    plotTree.put_child("SHARED_AXIS_" + std::to_string(i + 1), group);
  }
  for (size_t i = 0; i < mCommonTitles.size(); ++i) {
    ptree group;
    group.put("axis", mCommonTitles[i].first);
    group.put("pads", WritePadList(mCommonTitles[i].second));
    plotTree.put_child("COMMON_TITLE_" + std::to_string(i + 1), group);
  }
  return plotTree;
}

//**************************************************************************************************
/**
 * Pads sharing the scale of an axis (see Plot.h).
 */
//**************************************************************************************************
auto Plot::ShareAxis(char axis, const vector<uint8_t>& pads) -> decltype(*this)
{
  if (axis != 'X' && axis != 'Y' && axis != 'Z') {
    logger::warning("Plot {}: ShareAxis needs the axis 'X', 'Y' or 'Z' (got '{}'{}).", Label(mName, mGroup), axis, (axis == 'C') ? "; the colour scale of a 3d histogram is shared with 'Z'" : "");
    return *this;
  }
  vector<uint8_t> group;
  for (uint8_t padID : pads) {
    if (padID == 0) {
      logger::warning("Plot {}: pad 0 cannot share the {} axis (it holds the defaults of all pads, the pads are counted from 1).", Label(mName, mGroup), axis);
    } else if (std::find(group.begin(), group.end(), padID) == group.end()) {
      group.push_back(padID);
    }
  }
  if (group.empty() && !pads.empty()) return *this;
  mSharedAxes.push_back({axis, group, false});
  return *this;
}

//**************************************************************************************************
/**
 * The groups of pads sharing an axis: the declarations in their order, a pad listed again leaves its earlier group
 * (a warning if both groups were declared with ShareAxis: the touching panels of the layout are meant to be regrouped;
 * none for a single pad, the way to take a pad out of its group).
 */
//**************************************************************************************************
std::map<char, vector<Plot::shared_group_t>> Plot::GetSharedAxes() const
{
  std::map<char, vector<std::pair<vector<uint8_t>, bool>>> declared;  // per axis: the groups, whether the layout declared them
  for (const auto& [axis, pads, fromLayout] : mSharedAxes) {
    auto& groups = declared[axis];
    if (pads.empty()) {
      groups.clear();
      continue;
    }
    for (uint8_t padID : pads) {
      for (auto& [members, layoutGroup] : groups) {
        auto member = std::find(members.begin(), members.end(), padID);
        if (member == members.end()) continue;
        if (!layoutGroup && !fromLayout && members.size() > 1 && pads.size() > 1) {
          logger::warning("Plot {}: pad {} is listed in two groups sharing the {} axis, the later one is used.", Label(mName, mGroup), padID, axis);
        }
        members.erase(member);
      }
    }
    groups.emplace_back(pads, fromLayout);
  }
  std::map<char, vector<shared_group_t>> result;
  for (const auto& [axis, groups] : declared) {
    for (const auto& [members, layoutGroup] : groups) {
      if (members.size() > 1) result[axis].push_back({members, layoutGroup});
    }
  }
  // the layout shows such an axis once (the labels between touching panels are hidden, one colour scale for all): a group taken
  // apart by ShareAxis leaves panels whose ranges can differ unnoticed
  for (const auto& [axis, pads, fromLayout] : mSharedAxes) {
    if (!fromLayout || pads.empty()) continue;
    auto groupOf = [&, axis = axis](uint8_t padID) -> const vector<uint8_t>* {
      for (const auto& group : result[axis]) {
        if (std::find(group.pads.begin(), group.pads.end(), padID) != group.pads.end()) return &group.pads;
      }
      return nullptr;
    };
    const auto* first = groupOf(pads.front());
    const bool together = first && std::all_of(pads.begin(), pads.end(), [&](uint8_t padID) { return groupOf(padID) == first; });
    if (together) continue;
    vector<string> padNames;
    for (uint8_t padID : pads)
      padNames.push_back(std::to_string(padID));
    logger::warning("Plot {}: the layout shows the {} axis of pads {} once, but ShareAxis no longer lets them share it: their ranges can differ unnoticed. Give them the same range, or use a layout with gaps.",
                    Label(mName, mGroup), axis, join_strings(padNames, ", "));
  }
  return result;
}

//**************************************************************************************************
/**
 * Count number of inputs needed to create the plot.
 */
//**************************************************************************************************
uint16_t Plot::GetDataCount() const
{
  uint16_t count{};
  for (const auto& [padID, pad] : mPads) {
    count += pad.GetDataCount();
  }
  return count;
}

void Plot::Print(const boost::property_tree::ptree& pt, const string& name)
{
  string message;
  if (!name.empty()) message = fmt::format("Settings of {}:", name);
  auto print_pt = [&](auto&& self, const auto& node, int indent) -> void {
    for (const auto& child : node) {
      const std::string pad(indent * 2, ' ');
      if (!message.empty()) message += "\n         ";
      if (!child.second.data().empty()) {
        message += fmt::format("{}- {}: {}", pad, child.first, child.second.data());
      } else {
        message += fmt::format("{}- {}:", pad, child.first);
        self(self, child.second, indent + 1);
      }
    }
  };
  print_pt(print_pt, pt, 0);
  if (!message.empty()) logger::print("{}", message);
}

string Plot::Label(const string& name, const string& group)
{
  const string coloured = fmt::format("{}{}{}", logger::begin_color(logger::Color::Green), name, logger::end_color());
  return (group.empty()) ? coloured : fmt::format("{} from group {}{}{}", coloured, logger::begin_color(logger::Color::Yellow), group, logger::end_color());
}

auto Plot::SetName(const string& name) -> decltype(*this)
{
  if (auto error = NameError(name)) {
    logger::warning("{} SetName is ignored.", *error);
    return *this;
  }
  mName = name;
  UpdateUniqueName();
  return *this;
}

auto Plot::SetGroup(const string& group) -> decltype(*this)
{
  if (auto error = NameError(group, true)) {
    logger::warning("{} SetGroup is ignored.", *error);
    return *this;
  }
  const string normalized = NormalizeGroup(group);
  if (normalized.empty()) {
    logger::warning("SetGroup with an empty group is ignored.");
    return *this;
  }
  mGroup = normalized;
  UpdateUniqueName();
  return *this;
}

auto Plot::AppendGroup(const string& subgroup) -> decltype(*this)
{
  if (subgroup.empty()) {
    logger::warning("AppendGroup with an empty subgroup is ignored.");
    return *this;
  }
  if (auto error = NameError(subgroup, true)) {
    logger::warning("{} AppendGroup is ignored.", *error);
    return *this;
  }
  const string normalizedSubgroup = NormalizeGroup(subgroup);
  if (normalizedSubgroup.empty()) {
    logger::warning("AppendGroup with an empty subgroup is ignored.");
    return *this;
  }
  mGroup = mGroup.empty() ? normalizedSubgroup : mGroup + "/" + normalizedSubgroup;
  UpdateUniqueName();
  return *this;
}

auto Plot::SetBasePlot(const string& basePlot) -> decltype(*this)
{
  mBasePlot = basePlot;
  return *this;
}

auto Plot::SetDimensions(int32_t width, int32_t height, bool fixAspectRatio) -> decltype(*this)
{
  SetWidth(width);
  SetHeight(height);
  mPlotDimensions.fixAspectRatio = fixAspectRatio;
  return *this;
}

auto Plot::SetWidth(int32_t width) -> decltype(*this)
{
  if (CheckPixels(width, "width of plot " + mName)) mPlotDimensions.width = width;
  return *this;
}

auto Plot::SetHeight(int32_t height) -> decltype(*this)
{
  if (CheckPixels(height, "height of plot " + mName)) mPlotDimensions.height = height;
  return *this;
}

auto Plot::SetFixAspectRatio(bool fixAspectRatio) -> decltype(*this)
{
  mPlotDimensions.fixAspectRatio = fixAspectRatio;
  return *this;
}

//**************************************************************************************************
/**
 * Set this plot transparent.
 */
//**************************************************************************************************
auto Plot::SetTransparent() -> decltype(*this)
{
  mFill.style = 0;
  return *this;
}

//**************************************************************************************************
/**
 * Apply all settings from plot on top of this plot.
 */
//**************************************************************************************************
void Plot::operator+=(const Plot& plot)
{
  if (this == &plot) logger::throw_invalid_argument("Cannot add plot {} to itself.", GetUniqueName());
  mName = plot.mName;
  mGroup = plot.mGroup;
  mBasePlot = plot.mBasePlot;

  if (plot.mPlotDimensions.width) mPlotDimensions.width = plot.mPlotDimensions.width;
  if (plot.mPlotDimensions.height) mPlotDimensions.height = plot.mPlotDimensions.height;
  if (plot.mPlotDimensions.fixAspectRatio) mPlotDimensions.fixAspectRatio = plot.mPlotDimensions.fixAspectRatio;

  mFill.Update(plot.mFill);

  // the data of an added pad come after the data the pad already has: references to the data of a named pad (<mean:1@2>,
  // a legend row for data of pad 2) move by the number of data that pad had before, whichever pad they are in
  std::map<uint8_t, uint16_t> dataOffsets;
  for (const auto& [padID, pad] : mPads) {
    if (!pad.mData.empty()) dataOffsets[padID] = static_cast<uint16_t>(pad.mData.size());
  }
  for (const auto& [padID, pad] : plot.mPads) {
    auto& thisPad = mPads[padID];  // initializes the pad in case it was not yet defined in this plot
    const size_t nLegendBoxes = thisPad.mLegendBoxes.size(), nTextBoxes = thisPad.mTextBoxes.size();
    thisPad += pad;
    if (dataOffsets.empty()) continue;
    for (size_t i = nLegendBoxes; i < thisPad.mLegendBoxes.size(); ++i)
      thisPad.mLegendBoxes[i]->ShiftDataIDsOfPads(dataOffsets);
    for (size_t i = nTextBoxes; i < thisPad.mTextBoxes.size(); ++i)
      thisPad.mTextBoxes[i]->ShiftDataIDsOfPads(dataOffsets);
  }
  mSharedAxes.insert(mSharedAxes.end(), plot.mSharedAxes.begin(), plot.mSharedAxes.end());  // later declarations win
  mCommonTitles.insert(mCommonTitles.end(), plot.mCommonTitles.begin(), plot.mCommonTitles.end());
  UpdateUniqueName();
}

//**************************************************************************************************
/**
 * Friend function to apply the plot settings on top of a base plot.
 */
//**************************************************************************************************
Plot operator+(const Plot& basePlot, const Plot& plot)
{
  if (&basePlot == &plot) logger::throw_invalid_argument("Cannot add plot {} to itself.", plot.GetUniqueName());
  Plot combinedPlot = basePlot;
  combinedPlot += plot;
  return combinedPlot;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Pad
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Trigger redrawing of axes after all data was drawn to the pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetRedrawAxes(bool redraw) -> decltype(*this)
{
  mRedrawAxes = redraw;
  return *this;
}

//**************************************************************************************************
/**
 * Set text size
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultTextSize(float_t size) -> decltype(*this)
{
  if (CheckNotNegative(size, "text size")) mText.scale = size;
  return *this;
}

//**************************************************************************************************
/**
 * Set text color
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultTextColor(color_t color) -> decltype(*this)
{
  mText.color = color;
  return *this;
}
auto Plot::Pad::SetDefaultTextAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "default text alpha")) return *this;
  mText.alpha = alpha;
  return *this;
}

//**************************************************************************************************
/**
 * Set text font for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultTextFont(int16_t font) -> decltype(*this)
{
  if (CheckFont(font)) mText.style = font;
  return *this;
}

//**************************************************************************************************
/**
 * Set default marker size for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultMarkerSize(float_t size) -> decltype(*this)
{
  if (CheckFinite(size, "marker size")) mMarkerDefaults.scale = size;
  return *this;
}

//**************************************************************************************************
/**
 * Set default line width for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultLineWidth(float_t width) -> decltype(*this)
{
  if (CheckFinite(width, "default line width")) mLineDefaults.scale = RoundWidth(width, "default line width");
  return *this;
}

//**************************************************************************************************
/**
 * Set default fill alpha for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultFillAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "default fill alpha")) return *this;
  mFillDefaults.alpha = alpha;
  return *this;
}
auto Plot::Pad::SetDefaultMarkerAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "default marker alpha")) return *this;
  mMarkerDefaults.alpha = alpha;
  return *this;
}
auto Plot::Pad::SetDefaultLineAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "default line alpha")) return *this;
  mLineDefaults.alpha = alpha;
  return *this;
}
auto Plot::Pad::SetDefaultAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "default alpha")) return *this;
  mMarkerDefaults.alpha = alpha;
  mLineDefaults.alpha = alpha;
  mFillDefaults.alpha = alpha;
  return *this;
}

//**************************************************************************************************
/**
 * The (r, g, b, position) stops of a gradient given as (colour, position): an index is replaced by the components of its ROOT colour.
 */
//**************************************************************************************************
vector<tuple<float_t, float_t, float_t, float_t>> Plot::Pad::GradientEndpoints(const vector<pair<color_t, float_t>>& gradient)
{
  vector<tuple<float_t, float_t, float_t, float_t>> endpoints;
  endpoints.reserve(gradient.size());
  for (const auto& [color, position] : gradient) {
    const auto [red, green, blue] = color.ToFractions();
    endpoints.emplace_back(red, green, blue, position);
  }
  return endpoints;
}

//**************************************************************************************************
/**
 * Report gradient stops that cannot give the intended colours: fewer than two stops or colours, positions outside [0, 1] or decreasing
 * (equal neighbours are a hard step), colour components outside [0, 1]. Run once when the gradient is set, which is then ignored (false).
 */
//**************************************************************************************************
bool Plot::Pad::CheckGradient(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<int32_t> nColors, const string& setter)
{
  bool valid = true;
  auto invalid = [&](auto&&... args) {
    logger::warning(args...);
    valid = false;
  };
  if (rgbEndpoints.size() < 2) invalid("The gradient of {} is ignored: it needs at least two stops, got {}.", setter, rgbEndpoints.size());
  if (nColors && *nColors < 1) invalid("The gradient of {} is ignored: the number of its colours must be positive, got {}.", setter, *nColors);
  auto inUnitRange = [](float_t value) { return value >= 0.f && value <= 1.f; };
  for (size_t i = 0; i < rgbEndpoints.size(); ++i) {
    const auto& [red, green, blue, position] = rgbEndpoints[i];
    if (!inUnitRange(position)) {
      invalid("The gradient of {} is ignored: stop {} is at position {}, outside of 0 to 1 (the colour scale only covers 0 to 1).", setter, i + 1, position);
    } else if (i > 0 && position < std::get<3>(rgbEndpoints[i - 1])) {
      invalid("The gradient of {} is ignored: stop {} is at position {}, before the previous one ({}), positions must not decrease.", setter, i + 1, position, std::get<3>(rgbEndpoints[i - 1]));
    }
    if (!inUnitRange(red) || !inUnitRange(green) || !inUnitRange(blue)) {
      invalid("The gradient of {} is ignored: stop {} has the colour ({}, {}, {}), components must be between 0 and 1.", setter, i + 1, red, green, blue);
    }
  }
  return valid;
}

//**************************************************************************************************
/**
 * Default colours of the pad: a colour list or a gradient, one replaces the other.
 */
//**************************************************************************************************
void Plot::Pad::view_defaults_t::SetColors(const vector<color_t>& colorList)
{
  colorGradient = {};
  colors = colorList;
}
void Plot::Pad::view_defaults_t::SetGradient(const gradient_color_t& gradient)
{
  colors = nullopt;
  alpha = nullopt;
  colorGradient = gradient;
}
void Plot::Pad::view_defaults_t::Update(const view_defaults_t& other)
{
  if (other.scale) scale = other.scale;
  if (other.alpha) alpha = other.alpha;
  if (other.styles) styles = other.styles;
  if (other.colors) SetColors(*other.colors);
  if (other.colorGradient.rgbEndpoints) {
    // a gradient comes with its own alpha and number of colours; an alpha set on the pad after its gradient is kept
    SetGradient(other.colorGradient);
    alpha = other.alpha;
  }
}
void Plot::Pad::view_defaults_t::ReadColors(const ptree& tree, const string& key)
{
  read_from_tree(tree, colors, key);
  colorGradient.Read(tree, key + "_gradient_");
}
void Plot::Pad::view_defaults_t::PutColors(ptree& tree, const string& key) const
{
  put_in_tree(tree, colors, key);
  colorGradient.Put(tree, key + "_gradient_");
}
void Plot::Pad::gradient_color_t::Read(const ptree& tree, const string& prefix)
{
  read_from_tree(tree, rgbEndpoints, prefix + "endpoints");
  read_from_tree(tree, alpha, prefix + "alpha");
  read_from_tree(tree, nColors, prefix + "num_colors");
}
void Plot::Pad::gradient_color_t::Put(ptree& tree, const string& prefix) const
{
  put_in_tree(tree, rgbEndpoints, prefix + "endpoints");
  put_in_tree(tree, alpha, prefix + "alpha");
  put_in_tree(tree, nColors, prefix + "num_colors");
}

//**************************************************************************************************
/**
 * Set default marker, line, fill or all colours.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultMarkerColors(const vector<color_t>& colors) -> decltype(*this)
{
  mMarkerDefaults.SetColors(colors);
  return *this;
}
auto Plot::Pad::SetDefaultMarkerColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  if (alpha && !Plot::CheckAlpha(*alpha, "alpha of SetDefaultMarkerColors")) return *this;
  if (!CheckGradient(rgbEndpoints, nColors, "SetDefaultMarkerColors")) return *this;
  mMarkerDefaults.SetGradient({rgbEndpoints, alpha, nColors});
  return *this;
}
auto Plot::Pad::SetDefaultMarkerColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultMarkerColors(GradientEndpoints(gradient), alpha, nColors);
}
auto Plot::Pad::SetDefaultLineColors(const vector<color_t>& colors) -> decltype(*this)
{
  mLineDefaults.SetColors(colors);
  return *this;
}
auto Plot::Pad::SetDefaultLineColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  if (alpha && !Plot::CheckAlpha(*alpha, "alpha of SetDefaultLineColors")) return *this;
  if (!CheckGradient(rgbEndpoints, nColors, "SetDefaultLineColors")) return *this;
  mLineDefaults.SetGradient({rgbEndpoints, alpha, nColors});
  return *this;
}
auto Plot::Pad::SetDefaultLineColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultLineColors(GradientEndpoints(gradient), alpha, nColors);
}
auto Plot::Pad::SetDefaultFillColors(const vector<color_t>& colors) -> decltype(*this)
{
  mFillDefaults.SetColors(colors);
  return *this;
}
auto Plot::Pad::SetDefaultFillColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  if (alpha && !Plot::CheckAlpha(*alpha, "alpha of SetDefaultFillColors")) return *this;
  if (!CheckGradient(rgbEndpoints, nColors, "SetDefaultFillColors")) return *this;
  mFillDefaults.SetGradient({rgbEndpoints, alpha, nColors});
  return *this;
}
auto Plot::Pad::SetDefaultFillColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultFillColors(GradientEndpoints(gradient), alpha, nColors);
}
auto Plot::Pad::SetDefaultColors(const vector<color_t>& colors) -> decltype(*this)
{
  for (auto* defaults : {&mMarkerDefaults, &mLineDefaults, &mFillDefaults})
    defaults->SetColors(colors);
  return *this;
}
auto Plot::Pad::SetDefaultColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  if (alpha && !Plot::CheckAlpha(*alpha, "alpha of SetDefaultColors")) return *this;
  if (!CheckGradient(rgbEndpoints, nColors, "SetDefaultColors")) return *this;
  for (auto* defaults : {&mMarkerDefaults, &mLineDefaults, &mFillDefaults})
    defaults->SetGradient({rgbEndpoints, alpha, nColors});
  return *this;
}
auto Plot::Pad::SetDefaultColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultColors(GradientEndpoints(gradient), alpha, nColors);
}

//**************************************************************************************************
/**
 * Set default marker styles.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultMarkerStyles(const vector<int16_t>& styles) -> decltype(*this)
{
  for (int16_t style : styles) {
    if (!CheckMarkerStyle(style, "default marker style")) return *this;
  }
  mMarkerDefaults.styles = styles;
  return *this;
}

//**************************************************************************************************
/**
 * Set default line styles.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultLineStyles(const vector<int16_t>& styles) -> decltype(*this)
{
  mLineDefaults.styles = styles;
  return *this;
}

//**************************************************************************************************
/**
 * Set default fill styles.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultFillStyles(const vector<int16_t>& styles) -> decltype(*this)
{
  mFillDefaults.styles = styles;
  return *this;
}

//**************************************************************************************************
/**
 * Set default box range option for 2d candle plot.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultCandleBoxRange(float_t range) -> decltype(*this)
{
  if (CheckFraction(range, "candle box range")) mCandleOptionDefaults.boxRange = range;
  return *this;
}

//**************************************************************************************************
/**
 * Set default whisker range option for 2d candle plot.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultCandleWhiskerRange(float_t range) -> decltype(*this)
{
  if (CheckFraction(range, "candle whisker range")) mCandleOptionDefaults.whiskerRange = range;
  return *this;
}

//**************************************************************************************************
/**
 * Coordinate system of the legends, texts, statistics boxes and images that set none (default: pad_coordinates).
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultBoxCoordinates(coordinates_t coordinates) -> decltype(*this)
{
  mBoxCoordinates = coordinates;
  return *this;
}

//**************************************************************************************************
/**
 * Coordinate system of the shapes that set none (default: user_coordinates).
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultShapeCoordinates(coordinates_t coordinates) -> decltype(*this)
{
  mShapeCoordinates = coordinates;
  return *this;
}

//**************************************************************************************************
/**
 * Make this pad transparent.
 */
//**************************************************************************************************
auto Plot::Pad::SetTransparent() -> decltype(*this)
{
  mFill.style = 0;
  return *this;
}

//**************************************************************************************************
/**
 * Set frame fill properties.
 */
//**************************************************************************************************
auto Plot::Pad::SetFrameFill(int16_t style, color_t color, float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "frame fill alpha")) return *this;
  mFrameFill.color = color;
  mFrameFill.style = style;
  mFrameFill.alpha = alpha;
  return *this;
}

auto Plot::Pad::SetFrameFillColor(color_t color) -> decltype(*this)
{
  mFrameFill.color = color;
  return *this;
}

auto Plot::Pad::SetFrameFillStyle(int16_t style) -> decltype(*this)
{
  mFrameFill.style = style;
  return *this;
}

auto Plot::Pad::SetFrameFillAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "frame fill alpha")) return *this;
  mFrameFill.alpha = alpha;
  return *this;
}

//**************************************************************************************************
/**
 * Set frame line properties.
 */
//**************************************************************************************************
auto Plot::Pad::SetFrameBorder(int16_t style, float_t width, color_t color, float_t alpha) -> decltype(*this)
{
  if (!CheckFinite(width, "frame border width") || !Plot::CheckAlpha(alpha, "frame border alpha")) return *this;
  mFrameBorder.color = color;
  mFrameBorder.alpha = alpha;
  mFrameBorder.style = style;
  mFrameBorder.scale = RoundWidth(width, "frame border width");
  return *this;
}

auto Plot::Pad::SetFrameBorderColor(color_t color) -> decltype(*this)
{
  mFrameBorder.color = color;
  return *this;
}

auto Plot::Pad::SetFrameBorderAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "frame border alpha")) return *this;
  mFrameBorder.alpha = alpha;
  return *this;
}

auto Plot::Pad::SetFrameBorderStyle(int16_t style) -> decltype(*this)
{
  mFrameBorder.style = style;
  return *this;
}

auto Plot::Pad::SetFrameBorderWidth(float_t width) -> decltype(*this)
{
  if (CheckFinite(width, "frame border width")) mFrameBorder.scale = RoundWidth(width, "frame border width");
  return *this;
}

//**************************************************************************************************
/**
 * Set the frame of this pad transparent.
 */
//**************************************************************************************************
auto Plot::Pad::SetFrameTransparent() -> decltype(*this)
{
  mFrameFill.style = 0;
  mFrameFill.color = 0;
  return *this;
}

//**************************************************************************************************
/**
 * Set colour palette for 2D plots.
 * This is a selection of available palettes:
 * - Standard: kBird, 1 (ROOT5 default), kDarkRainBow, kTemperatureMap, kLightTemperature, kRainBow
 * - Gray: kGreyScale, kGreyYellow, kPearl, kPigeon
 * - Green: kAvocado, kBlueGreenYellow, kAlpine, kGistEarth, kViridis
 * - RedBlue: kThermometer, kCandy, kBlackBody, kCherry, kRedBlue
 * - RedGreen: kRose, kWaterMelon
 * - Blue: kLake, kAtlantic
 * - BlueYellow: kBlueYellow, kStarryNight
 * - Brown: kCoffee, kFall, kSandyTerrain, kCopper
 * - Purple: kFuchsia
 * - Intense: kGreenPink, kOcean, kDarkBodyRadiator, kInvertedDarkBodyRadiator, kSunset,
 * kVisibleSpectrum
 */
//**************************************************************************************************
auto Plot::Pad::SetPalette(int32_t palette, optional<float_t> alpha) -> decltype(*this)
{
  if (alpha && !Plot::CheckAlpha(*alpha, "alpha of SetPalette")) return *this;
  if (palette > kCividis) {  // up to 0: ROOT's default palette, 1 to 50: of so many colours
    logger::warning("The palette ({}) is ignored: it is not a ROOT palette (the last one is kCividis, {}).", palette, static_cast<int>(kCividis));
    return *this;
  }
  mPalette.index = palette;
  mPalette.colorGradient = {{}, alpha, {}};  // a ROOT palette keeps only the alpha of the gradient settings
  return *this;
}
auto Plot::Pad::SetPalette(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  if (alpha && !Plot::CheckAlpha(*alpha, "alpha of SetPalette")) return *this;
  if (!CheckGradient(rgbEndpoints, nColors, "SetPalette")) return *this;
  mPalette.index = {};
  mPalette.colorGradient = {rgbEndpoints, alpha, nColors};
  return *this;
}
auto Plot::Pad::SetPalette(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetPalette(GradientEndpoints(gradient), alpha, nColors);
}

//**************************************************************************************************
/**
 * Set position of pad within plot. The variables are given in fractions of plot width resp. height (between 0 and 1).
 * The lower left corner is defined by (xlow, ylow) and the upper right corner by (xup, yup).
 */
//**************************************************************************************************
auto Plot::Pad::SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup) -> decltype(*this)
{
  bool valid = CheckFraction(xlow, "pad position xlow");
  valid = CheckFraction(ylow, "pad position ylow") && valid;
  valid = CheckFraction(xup, "pad position xup") && valid;
  valid = CheckFraction(yup, "pad position yup") && valid;
  if (valid && (xlow >= xup || ylow >= yup)) {
    logger::warning("The pad position ({}, {}) to ({}, {}) is ignored: the lower left corner must be below and left of the upper right one.", xlow, ylow, xup, yup);
    valid = false;
  }
  if (!valid) return *this;
  mPosition.xlow = xlow;
  mPosition.ylow = ylow;
  mPosition.xup = xup;
  mPosition.yup = yup;
  return *this;
}

//**************************************************************************************************
/**
 * Set margins of pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetMargins(float_t top, float_t bottom, float_t left, float_t right) -> decltype(*this)
{
  bool valid = CheckFraction(top, "top margin");
  valid = CheckFraction(bottom, "bottom margin") && valid;
  valid = CheckFraction(left, "left margin") && valid;
  valid = CheckFraction(right, "right margin") && valid;
  if (!valid) return *this;
  if (top + bottom >= 1.f || left + right >= 1.f) {
    logger::warning("The margins (top {}, bottom {}, left {}, right {}) are ignored: they leave no room for the frame.", top, bottom, left, right);
    return *this;
  }
  mMargins.top = top;
  mMargins.bottom = bottom;
  mMargins.left = left;
  mMargins.right = right;
  return *this;
}
auto Plot::Pad::SetTopMargin(float_t top) -> decltype(*this)
{
  if (CheckFraction(top, "top margin")) mMargins.top = top;
  return *this;
}
auto Plot::Pad::SetBottomMargin(float_t bottom) -> decltype(*this)
{
  if (CheckFraction(bottom, "bottom margin")) mMargins.bottom = bottom;
  return *this;
}
auto Plot::Pad::SetLeftMargin(float_t left) -> decltype(*this)
{
  if (CheckFraction(left, "left margin")) mMargins.left = left;
  return *this;
}
auto Plot::Pad::SetRightMargin(float_t right) -> decltype(*this)
{
  if (CheckFraction(right, "right margin")) mMargins.right = right;
  return *this;
}

//**************************************************************************************************
/**
 * Set viewing perspective on pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetView(double_t theta, double_t phi) -> decltype(*this)
{
  bool valid = CheckFinite(theta, "view angle theta");
  valid = CheckFinite(phi, "view angle phi") && valid;
  if (!valid) return *this;
  mView.theta = theta;
  mView.phi = phi;
  return *this;
}

//**************************************************************************************************
/**
 * Constructor from other pad.
 */
//**************************************************************************************************
Plot::Pad::Pad(const Pad& other)
  : FillSetters<Pad>(other), mPosition(other.mPosition), mMargins(other.mMargins), mView(other.mView),
    mFrameFill(other.mFrameFill), mFrameBorder(other.mFrameBorder), mText(other.mText),
    mMarkerDefaults(other.mMarkerDefaults), mLineDefaults(other.mLineDefaults), mFillDefaults(other.mFillDefaults),
    mCandleOptionDefaults(other.mCandleOptionDefaults), mBoxCoordinates(other.mBoxCoordinates), mShapeCoordinates(other.mShapeCoordinates),
    mPalette(other.mPalette), mRedrawAxes(other.mRedrawAxes), mIsEmptyCell(other.mIsEmptyCell), mColorScaleOf(other.mColorScaleOf),
    mLayoutValues(other.mLayoutValues), mHiddenNextTo(other.mHiddenNextTo), mAxes(other.mAxes)
{
  mData.reserve(other.mData.size());
  for (const auto& data : other.mData) {
    mData.push_back(data->Clone());
  }
  if (other.mRefFunc) mRefFunc = other.mRefFunc->Clone();
  if (other.mFrame) mFrame = other.mFrame->Clone();

  mLegendBoxes.reserve(other.mLegendBoxes.size());
  for (const auto& legendBox : other.mLegendBoxes) {
    mLegendBoxes.push_back(std::make_shared<LegendBox>(*legendBox));
  }
  mTextBoxes.reserve(other.mTextBoxes.size());
  for (const auto& textBox : other.mTextBoxes) {
    mTextBoxes.push_back(std::make_shared<TextBox>(*textBox));
  }
  mShapes.reserve(other.mShapes.size());
  for (const auto& shape : other.mShapes) {
    mShapes.push_back(std::make_shared<Shape>(*shape));
  }
  mImageBoxes.reserve(other.mImageBoxes.size());
  for (const auto& imageBox : other.mImageBoxes) {
    mImageBoxes.push_back(std::make_shared<ImageBox>(*imageBox));
  }
}

//**************************************************************************************************
/**
 * Assignment operator.
 */
//**************************************************************************************************
Plot::Pad& Plot::Pad::operator=(const Pad& other)
{
  if (this != &other) {
    Pad tmp(other);
    *this = std::move(tmp);
  }
  return *this;
}

//**************************************************************************************************
/**
 * Constructor from property tree.
 */
//**************************************************************************************************
Plot::Pad::Pad(const ptree& padTree)
{
  read_from_tree(padTree, mPosition.xlow, "position_xlow");
  read_from_tree(padTree, mPosition.ylow, "position_ylow");
  read_from_tree(padTree, mPosition.xup, "position_xup");
  read_from_tree(padTree, mPosition.yup, "position_yup");
  read_from_tree(padTree, mMargins.top, "margins_top");
  read_from_tree(padTree, mMargins.bottom, "margins_bottom");
  read_from_tree(padTree, mMargins.left, "margins_left");
  read_from_tree(padTree, mMargins.right, "margins_right");
  read_from_tree(padTree, mView.theta, "view_theta");
  read_from_tree(padTree, mView.phi, "view_phi");
  read_from_tree(padTree, mPalette.index, "palette_index");
  mPalette.colorGradient.Read(padTree, "palette_colors_gradient_");
  ReadFill(padTree);
  ReadLayout(padTree, mFrameFill, "frame_fill_", "style");
  ReadLayout(padTree, mFrameBorder, "frame_border_", "style", "width");
  ReadLayout(padTree, mText, "text_", "font", "size");
  read_from_tree(padTree, mMarkerDefaults.alpha, "default_marker_alpha");
  read_from_tree(padTree, mMarkerDefaults.scale, "default_marker_size");
  read_from_tree(padTree, mMarkerDefaults.styles, "default_marker_styles");
  mMarkerDefaults.ReadColors(padTree, "default_marker_colors");
  read_from_tree(padTree, mLineDefaults.alpha, "default_line_alpha");
  ReadWidthFromTree(padTree, mLineDefaults.scale, "default_line_width");
  read_from_tree(padTree, mLineDefaults.styles, "default_line_styles");
  mLineDefaults.ReadColors(padTree, "default_line_colors");
  read_from_tree(padTree, mFillDefaults.alpha, "default_fill_alpha");
  read_from_tree(padTree, mFillDefaults.styles, "default_fill_styles");
  mFillDefaults.ReadColors(padTree, "default_fill_colors");
  read_from_tree(padTree, mCandleOptionDefaults.boxRange, "default_candle_option_box_range");
  read_from_tree(padTree, mCandleOptionDefaults.whiskerRange, "default_candle_option_whisker_range");
  ReadNamed(padTree, mBoxCoordinates, "default_box_coordinates");
  ReadNamed(padTree, mShapeCoordinates, "default_shape_coordinates");
  read_from_tree(padTree, mRedrawAxes, "redraw_axes");
  mIsEmptyCell = get_from_tree<bool>(padTree, "empty_cell").value_or(false);
  read_from_tree(padTree, mColorScaleOf, "color_scale_of");
  if (auto hiddenTree = padTree.get_child_optional("hidden_next_to")) {
    for (const auto& [key, pads] : *hiddenTree) {
      if (key != "x" && key != "y") logger::throw_invalid_argument("Bad key hidden_next_to.{}: x or y", key);
      mHiddenNextTo[static_cast<char>(std::toupper(key[0]))] = ReadPadList(pads.data(), "hidden_next_to." + key);
    }
  }
  if (auto layoutTree = padTree.get_child_optional("layout_values")) {
    // the values a PanelLayout sets (another key is not read: the loading names it)
    static const std::set<string> layoutKeys{"title_size", "title_offset_x", "title_offset_y", "title_offset_z", "color_scale_offset", "color_scale_width"};
    for (const auto& [key, value] : *layoutTree) {
      if (layoutKeys.count(key)) mLayoutValues[key] = *get_from_tree<float_t>(*layoutTree, key);
    }
  }

  for (const auto& content : padTree) {
    // add data
    if (str_contains(content.first, "DATA")) {
      string type = content.second.get<string>("type");
      if (type == "data") {
        mData.push_back(std::make_shared<Data>(content.second));
      }
      if (type == "ratio") {
        mData.push_back(std::make_shared<Ratio>(content.second));
      }
    }
    if (str_contains(content.first, "REF_FUNC")) {
      mRefFunc = std::make_shared<Data>(content.second);
    }
    if (content.first == "FRAME") {
      mFrame = std::make_shared<Data>(content.second);
    }

    // add boxes
    if (str_contains(content.first, "LEGEND")) {
      mLegendBoxes.push_back(std::make_shared<LegendBox>(content.second));
    }
    if (str_contains(content.first, "TEXT")) {
      mTextBoxes.push_back(std::make_shared<TextBox>(content.second));
    }
    if (str_contains(content.first, "SHAPE")) {
      if (const string kind = content.second.get<string>("kind", ""); enum_from_name<Shape::kind_t>(kind)) {
        mShapes.push_back(std::make_shared<Shape>(content.second));
      } else {
        logger::warning("Unknown shape kind {} is ignored.", kind);
      }
    }
    if (str_contains(content.first, "IMAGE")) {
      mImageBoxes.push_back(std::make_shared<ImageBox>(content.second));
    }
    // add axes
    if (str_contains(content.first, "AXIS")) {
      const auto axis = get_from_tree<char>(content.second, "name");
      if (!axis) logger::throw_invalid_argument("{} has no name", content.first);
      mAxes[*axis] = Axis(content.second);
    }
  }
}

//**************************************************************************************************
/**
 * Get representation of pad as property tree.
 */
//**************************************************************************************************
ptree Plot::Pad::GetPropertyTree() const
{
  // convert properties of plot to ptree
  ptree padTree;
  put_in_tree(padTree, mPosition.xlow, "position_xlow");
  put_in_tree(padTree, mPosition.ylow, "position_ylow");
  put_in_tree(padTree, mPosition.xup, "position_xup");
  put_in_tree(padTree, mPosition.yup, "position_yup");
  put_in_tree(padTree, mMargins.top, "margins_top");
  put_in_tree(padTree, mMargins.bottom, "margins_bottom");
  put_in_tree(padTree, mMargins.left, "margins_left");
  put_in_tree(padTree, mMargins.right, "margins_right");
  put_in_tree(padTree, mView.theta, "view_theta");
  put_in_tree(padTree, mView.phi, "view_phi");
  put_in_tree(padTree, mPalette.index, "palette_index");
  mPalette.colorGradient.Put(padTree, "palette_colors_gradient_");
  PutFill(padTree);
  PutLayout(padTree, mFrameFill, "frame_fill_", "style");
  PutLayout(padTree, mFrameBorder, "frame_border_", "style", "width");
  PutLayout(padTree, mText, "text_", "font", "size");
  put_in_tree(padTree, mMarkerDefaults.alpha, "default_marker_alpha");
  put_in_tree(padTree, mMarkerDefaults.scale, "default_marker_size");
  put_in_tree(padTree, mMarkerDefaults.styles, "default_marker_styles");
  mMarkerDefaults.PutColors(padTree, "default_marker_colors");
  put_in_tree(padTree, mLineDefaults.alpha, "default_line_alpha");
  put_in_tree(padTree, mLineDefaults.scale, "default_line_width");
  put_in_tree(padTree, mLineDefaults.styles, "default_line_styles");
  mLineDefaults.PutColors(padTree, "default_line_colors");
  put_in_tree(padTree, mFillDefaults.alpha, "default_fill_alpha");
  put_in_tree(padTree, mFillDefaults.styles, "default_fill_styles");
  mFillDefaults.PutColors(padTree, "default_fill_colors");
  put_in_tree(padTree, mCandleOptionDefaults.boxRange, "default_candle_option_box_range");
  put_in_tree(padTree, mCandleOptionDefaults.whiskerRange, "default_candle_option_whisker_range");
  PutNamed(padTree, mBoxCoordinates, "default_box_coordinates");
  PutNamed(padTree, mShapeCoordinates, "default_shape_coordinates");
  put_in_tree(padTree, mRedrawAxes, "redraw_axes");
  if (mIsEmptyCell) padTree.put("empty_cell", true);
  put_in_tree(padTree, mColorScaleOf, "color_scale_of");
  for (const auto& [axisLabel, pads] : mHiddenNextTo) {
    padTree.put(string("hidden_next_to.") + static_cast<char>(std::tolower(axisLabel)), WritePadList(pads));
  }
  for (const auto& [key, value] : mLayoutValues) {
    padTree.put("layout_values." + key, value);
  }

  int32_t dataID = 1;
  for (const auto& data : mData) {
    padTree.put_child("DATA_" + std::to_string(dataID), data->GetPropertyTree());
    ++dataID;
  }
  if (mRefFunc) {
    padTree.put_child("REF_FUNC", mRefFunc->GetPropertyTree());
  }
  if (mFrame) {
    padTree.put_child("FRAME", mFrame->GetPropertyTree());
  }

  int32_t legendBoxID = 1;
  for (const auto& legendBox : mLegendBoxes) {
    padTree.put_child("LEGEND_" + std::to_string(legendBoxID), legendBox->GetPropertyTree());
    ++legendBoxID;
  }
  int32_t textBoxID = 1;
  for (const auto& textBox : mTextBoxes) {
    padTree.put_child("TEXT_" + std::to_string(textBoxID), textBox->GetPropertyTree());
    ++textBoxID;
  }
  int32_t shapeID = 1;
  for (const auto& shape : mShapes) {
    padTree.put_child("SHAPE_" + std::to_string(shapeID), shape->GetPropertyTree());
    ++shapeID;
  }
  int32_t imageBoxID = 1;
  for (const auto& imageBox : mImageBoxes) {
    padTree.put_child("IMAGE_" + std::to_string(imageBoxID), imageBox->GetPropertyTree());
    ++imageBoxID;
  }

  for (const auto& axis : {'X', 'Y', 'Z', 'C'}) {
    if (mAxes.find(axis) != mAxes.end()) {
      padTree.put_child(string("AXIS_") + axis, mAxes.at(axis).GetPropertyTree());
    }
  }

  return padTree;
}

// the reference function and the frame are handed out by reference (also to Python): a reference kept stays valid
void Plot::Pad::ReplaceData(std::shared_ptr<Data>& slot, const Data& data)
{
  if (slot) {
    *slot = data;
  } else {
    slot = std::make_shared<Data>(data);
  }
}

//**************************************************************************************************
/**
 * Apply all settings from pad on top of this pad.
 */
//**************************************************************************************************
void Plot::Pad::operator+=(const Pad& pad)
{
  if (this == &pad) logger::throw_invalid_argument("Cannot add a pad to itself.");
  // the position is one setting (SetPosition sets all four): a pad placed by the added plot is placed entirely by it, never with
  // corners from both (a plots file with only some of them would otherwise mix them); margins are merged one by one, since a plot
  // may change a single margin of its base plot (SetLeftMargin)
  if (pad.mPosition.xlow || pad.mPosition.ylow || pad.mPosition.xup || pad.mPosition.yup) mPosition = pad.mPosition;
  if (pad.mMargins.top) mMargins.top = pad.mMargins.top;
  if (pad.mMargins.bottom) mMargins.bottom = pad.mMargins.bottom;
  if (pad.mMargins.left) mMargins.left = pad.mMargins.left;
  if (pad.mMargins.right) mMargins.right = pad.mMargins.right;
  if (pad.mView.theta) mView.theta = pad.mView.theta;
  if (pad.mView.phi) mView.phi = pad.mView.phi;
  mFill.Update(pad.mFill);
  mFrameFill.Update(pad.mFrameFill);
  mFrameBorder.Update(pad.mFrameBorder);
  mText.Update(pad.mText);
  mMarkerDefaults.Update(pad.mMarkerDefaults);
  mLineDefaults.Update(pad.mLineDefaults);
  mFillDefaults.Update(pad.mFillDefaults);
  if (pad.mCandleOptionDefaults.boxRange) mCandleOptionDefaults.boxRange = pad.mCandleOptionDefaults.boxRange;
  if (pad.mCandleOptionDefaults.whiskerRange) mCandleOptionDefaults.whiskerRange = pad.mCandleOptionDefaults.whiskerRange;
  if (pad.mBoxCoordinates) mBoxCoordinates = pad.mBoxCoordinates;
  if (pad.mShapeCoordinates) mShapeCoordinates = pad.mShapeCoordinates;

  if (pad.mPalette.colorGradient.rgbEndpoints) {
    mPalette.colorGradient = pad.mPalette.colorGradient;  // a gradient comes with its own alpha and number of colours
    mPalette.index = nullopt;
  }
  if (pad.mPalette.index) {
    mPalette.index = pad.mPalette.index;
    mPalette.colorGradient = {{}, pad.mPalette.colorGradient.alpha, {}};
  }
  if (pad.mRedrawAxes) mRedrawAxes = pad.mRedrawAxes;
  if (pad.mIsEmptyCell) mIsEmptyCell = true;
  if (pad.mColorScaleOf) mColorScaleOf = pad.mColorScaleOf;
  for (const auto& [axisLabel, pads] : pad.mHiddenNextTo) {
    mHiddenNextTo[axisLabel] = pads;
  }
  for (const auto& [key, value] : pad.mLayoutValues) {
    mLayoutValues[key] = value;
  }
  if (pad.mRefFunc) ReplaceData(mRefFunc, *pad.mRefFunc);
  if (pad.mFrame) ReplaceData(mFrame, *pad.mFrame);
  for (auto& [axisLabel, axis] : pad.mAxes) {
    mAxes[axisLabel];  // default initialize in case this axis was not yet defined
    mAxes[axisLabel] += axis;
  }
  size_t legendOffset = mLegendBoxes.size();
  for (const auto& legendBox : pad.mLegendBoxes) {
    // the data of this pad come first: the rows of the added pad refer to its own data
    mLegendBoxes.push_back(std::make_shared<LegendBox>(*legendBox));
    mLegendBoxes.back()->ShiftDataIDs(static_cast<uint16_t>(mData.size()));
  }
  for (const auto& textBox : pad.mTextBoxes) {
    mTextBoxes.push_back(std::make_shared<TextBox>(*textBox));
    mTextBoxes.back()->ShiftDataIDs(static_cast<uint16_t>(mData.size()));
  }
  for (const auto& shape : pad.mShapes) {
    mShapes.push_back(std::make_shared<Shape>(*shape));
  }
  for (const auto& imageBox : pad.mImageBoxes) {
    mImageBoxes.push_back(std::make_shared<ImageBox>(*imageBox));
  }
  const auto dataOffset = static_cast<uint16_t>(mData.size());  // the data of this pad come first
  for (const auto& data : pad.mData) {
    auto clonedData = data->Clone();
    // the legend of the data refers to the boxes of the added pad, also the implicit legend 1; without boxes there it is the first one of this pad
    if (clonedData && (clonedData->GetLegendID() || !pad.mLegendBoxes.empty())) {
      size_t shiftedLegendID = clonedData->GetLegendID().value_or(1u) + legendOffset;
      if (shiftedLegendID > std::numeric_limits<uint8_t>::max()) {
        logger::throw_out_of_range("Too many legend boxes to merge into a single pad (legend ID {} would exceed the maximum of {}).", shiftedLegendID, std::numeric_limits<uint8_t>::max());
      }
      clonedData->SetLegend(static_cast<uint8_t>(shiftedLegendID));
    }
    if (clonedData && clonedData->mLegend.entry) {
      clonedData->mLegend.entry->ShiftPosition(dataOffset);
      clonedData->mLegend.entry->ShiftSymbolOf(dataOffset);
    }
    mData.push_back(clonedData);
  }
}

//**************************************************************************************************
/**
 * Access operator for pad axis.
 */
//**************************************************************************************************
Plot::Pad::Axis& Plot::Pad::operator[](const char axis)
{
  return GetAxis(axis);
}
//**************************************************************************************************
/**
 * Access operator for data in pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::operator()(uint16_t dataID)
{
  return GetData(dataID);
}
//**************************************************************************************************
/**
 * Access function for pad axis.
 */
//**************************************************************************************************
Plot::Pad::Axis& Plot::Pad::GetAxis(const char axis)
{
  const vector<char> allowedAxes = {'X', 'Y', 'Z', 'C'};
  if (std::find(allowedAxes.begin(), allowedAxes.end(), axis) == allowedAxes.end()) {
    logger::throw_invalid_argument("Axis '{}' does not exist: use 'X', 'Y', 'Z' or 'C' (the content of a 3d histogram).", axis);
  }
  if (mAxes.find(axis) == mAxes.end()) {
    mAxes[axis] = Axis(axis);
  }
  return mAxes[axis];
}

//**************************************************************************************************
/**
 * Access function for pad data.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::GetData(uint16_t dataID)
{
  if (dataID < 1 || dataID > mData.size()) {
    logger::throw_out_of_range("Data with ID {} is not defined: only data already added to the pad can be accessed.", dataID);
  }
  return *mData[dataID - 1];
}

//**************************************************************************************************
/**
 * Access function for pad legends.
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::GetLegend(uint8_t legendID)
{
  if (legendID < 1 || legendID > mLegendBoxes.size()) {
    logger::throw_out_of_range("Legend with ID {} is not defined: only legends already added to the pad can be accessed.", legendID);
  }
  return *mLegendBoxes[legendID - 1];
}

//**************************************************************************************************
/**
 * Access function for text boxes.
 */
//**************************************************************************************************
Plot::Pad::TextBox& Plot::Pad::GetText(uint8_t textID)
{
  if (textID < 1 || textID > mTextBoxes.size()) {
    logger::throw_out_of_range("Text with ID {} is not defined: only texts already added to the pad can be accessed.", textID);
  }
  return *mTextBoxes[textID - 1];
}
Plot::Pad::Shape& Plot::Pad::GetShape(uint8_t shapeID)
{
  if (shapeID < 1 || shapeID > mShapes.size()) {
    logger::throw_out_of_range("Shape with ID {} is not defined: only shapes already added to the pad can be accessed.", shapeID);
  }
  return *mShapes[shapeID - 1];
}

//**************************************************************************************************
/**
 * Access function for images.
 */
//**************************************************************************************************
Plot::Pad::ImageBox& Plot::Pad::GetImage(uint8_t imageID)
{
  if (imageID < 1 || imageID > mImageBoxes.size()) {
    logger::throw_out_of_range("Image with ID {} is not defined: only images already added to the pad can be accessed.", imageID);
  }
  return *mImageBoxes[imageID - 1];
}

//**************************************************************************************************
/**
 * Add data to this pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::AddData(const string& name, const string& dataSource, const optional<string>& label)
{
  return AddData(Data(name, dataSource, nullopt), label);
}

Plot::Pad::Data& Plot::Pad::AddData(const Data& data, const optional<string>& label)
{
  if (data.mType == "ratio") {
    logger::throw_invalid_argument("Cannot add ratio {} via AddData, use AddRatio instead.", data.mName);
  }
  if (data.mName.empty() || data.mDataSource.empty()) {
    logger::throw_invalid_argument("Data needs a name and a data source to be added (name: '{}', data source: '{}').", data.mName, data.mDataSource);
  }
  mData.push_back(std::make_shared<Data>(data));
  auto& added = *mData.back();
  added.mType = "data";
  if (label) added.mLegend.label = label;
  return added;
}

//**************************************************************************************************
/**
 * Add user-defined function to this pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::AddFunction(const string& function, const optional<string>& label)
{
  if (is_blank(function)) logger::throw_invalid_argument("Empty function expression.");
  mData.push_back(std::make_shared<Data>(function, kUserFunctions, label));
  return *mData.back();
}

//**************************************************************************************************
/**
 * Add user-defined points to this pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::AddPoints(const vector<double_t>& x, const vector<double_t>& y, const optional<string>& label)
{
  size_t nPoints = x.size();
  if (!nPoints || x.size() != y.size()) {
    logger::throw_invalid_argument("AddPoints needs as many y values as x values, at least one (got {} and {}).", x.size(), y.size());
  }
  string xStr;
  string yStr;
  for (size_t i = 0; i < nPoints; ++i) {
    xStr += number_to_string(x[i]) + ((i == nPoints - 1) ? "" : ",");
    yStr += number_to_string(y[i]) + ((i == nPoints - 1) ? "" : ",");
  }
  mData.push_back(std::make_shared<Data>(xStr + ";" + yStr, kUserGraphs, label));
  return *mData.back();
}
Plot::Pad::Data& Plot::Pad::AddPoints(const vector<pair<double_t, double_t>>& positions, const optional<string>& label)
{
  vector<double_t> x;
  vector<double_t> y;
  for (auto [xi, yi] : positions) {
    x.push_back(xi);
    y.push_back(yi);
  }
  return AddPoints(x, y, label);
}

//**************************************************************************************************
/**
 * Add shapes (not data) to this pad.
 */
//**************************************************************************************************
Plot::Pad::Shape& Plot::Pad::AddLine(pair<double_t, double_t> pos1, pair<double_t, double_t> pos2)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::line, {pos1.first, pos1.second, pos2.first, pos2.second})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddArrow(pair<double_t, double_t> pos1, pair<double_t, double_t> pos2)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::arrow, {pos1.first, pos1.second, pos2.first, pos2.second})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddVerticalLine(double_t x)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::vertical_line, {x})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddHorizontalLine(double_t y)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::horizontal_line, {y})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddRegion(pair<double_t, double_t> pos1, pair<double_t, double_t> pos2)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::region, {pos1.first, pos1.second, pos2.first, pos2.second})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddRegionX(double_t x1, double_t x2)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::region_x, {x1, x2})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddRegionY(double_t y1, double_t y2)
{
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::region_y, {y1, y2})));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddPolygon(const vector<pair<double_t, double_t>>& points)
{
  if (points.size() < 3) logger::warning("Polygon with {} points: it needs at least three, it will not be drawn.", points.size());
  vector<double_t> coordinates;
  for (const auto& [x, y] : points) {
    coordinates.push_back(x);
    coordinates.push_back(y);
  }
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::polygon, coordinates)));
  return *mShapes.back();
}
Plot::Pad::Shape& Plot::Pad::AddEllipse(pair<double_t, double_t> center, double_t radiusX, double_t radiusY)
{
  if (radiusX <= 0. || radiusY <= 0.) logger::warning("Ellipse with radii {} and {}: radii must be positive, it will not be drawn.", radiusX, radiusY);
  mShapes.push_back(std::shared_ptr<Shape>(new Shape(Shape::kind_t::ellipse, {center.first, center.second, radiusX, radiusY})));
  return *mShapes.back();
}

//**************************************************************************************************
/**
 * Add ratio to this pad.
 */
//**************************************************************************************************
Plot::Pad::Ratio& Plot::Pad::AddRatio(const string& numeratorName, const string& numeratorDataSource, const string& denominatorName, const string& denominatorDataSource, const optional<string>& label)
{
  return AddRatio(Ratio(numeratorName, numeratorDataSource, denominatorName, denominatorDataSource, nullopt), label);
}

Plot::Pad::Ratio& Plot::Pad::AddRatio(const Data& numerator, const Data& denominator, const optional<string>& label)
{
  return AddRatio(Ratio(numerator, denominator), label);
}

Plot::Pad::Ratio& Plot::Pad::AddRatio(const Ratio& ratio, const optional<string>& label)
{
  if (ratio.mName.empty() || ratio.mDataSource.empty() || ratio.mDenomName.empty() || ratio.mDenomDataSource.empty()) {
    logger::throw_invalid_argument("A ratio needs names and data sources for numerator and denominator (numerator: '{}' from '{}', denominator: '{}' from '{}').",
                                   ratio.mName, ratio.mDataSource, ratio.mDenomName, ratio.mDenomDataSource);
  }
  mData.push_back(std::make_shared<Ratio>(ratio));
  auto& added = *std::dynamic_pointer_cast<Ratio>(mData.back());
  if (label) added.mLegend.label = label;
  added.mModMode = Ratio::Mode::Res;  // further modifiers act on the ratio, independent of the selection the ratio object ended with
  return added;
}

//**************************************************************************************************
/**
 * Add reference function. An empty one means none: it hides the reference function of the base plot or of pad 0.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::SetRefFunc(const string& refFunc)
{
  ReplaceData(mRefFunc, Data(refFunc, kUserFunctions, nullopt));
  return *mRefFunc;
}

//**************************************************************************************************
/**
 * Set the input that defines the axis frame of this pad without being drawn (e.g. for labelled bins or a given binning).
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::SetFrame(const string& name, const string& dataSource)
{
  return SetFrame(Data(name, dataSource, nullopt));
}

Plot::Pad::Data& Plot::Pad::SetFrame(const Data& data)
{
  if (data.mType == "ratio") {
    logger::throw_invalid_argument("A ratio ({}) cannot be the frame of a pad.", data.mName);
  }
  if (data.mName.empty() || data.mDataSource.empty()) {
    logger::throw_invalid_argument("The frame needs a name and a data source (name: '{}', data source: '{}').", data.mName, data.mDataSource);
  }
  ReplaceData(mFrame, data);
  mFrame->mType = "data";
  mFrame->mLegend.label = nullopt;
  return *mFrame;
}

//**************************************************************************************************
/**
 * Add text box to this pad.
 */
//**************************************************************************************************
Plot::Pad::TextBox& Plot::Pad::AddText(double_t xPos, double_t yPos, const string& text)
{
  mTextBoxes.push_back(std::make_shared<TextBox>(xPos, yPos, text));
  return *mTextBoxes.back();
}
Plot::Pad::TextBox& Plot::Pad::AddText(double_t xPos, double_t yPos, const vector<string>& lines)
{
  mTextBoxes.push_back(std::make_shared<TextBox>(xPos, yPos, lines));
  return *mTextBoxes.back();
}
Plot::Pad::TextBox& Plot::Pad::AddText(double_t xPos, double_t yPos, std::initializer_list<string> lines)
{
  return AddText(xPos, yPos, vector<string>(lines));
}

//**************************************************************************************************
/**
 * Add text box to this pad automatically find a position for it.
 */
//**************************************************************************************************
Plot::Pad::TextBox& Plot::Pad::AddText(const string& text)
{
  mTextBoxes.push_back(std::make_shared<TextBox>(text));
  return *mTextBoxes.back();
}
Plot::Pad::TextBox& Plot::Pad::AddText(const vector<string>& lines)
{
  mTextBoxes.push_back(std::make_shared<TextBox>(lines));
  return *mTextBoxes.back();
}
Plot::Pad::TextBox& Plot::Pad::AddText(std::initializer_list<string> lines)
{
  return AddText(vector<string>(lines));
}

//**************************************************************************************************
/**
 * Add text box to this pad and automatically find a position for it (close to the given corner of the frame or the one where it fits best).
 */
//**************************************************************************************************
Plot::Pad::TextBox& Plot::Pad::AddText(box_placement_t placement, const string& text)
{
  return AddText(text).SetAutoPlacement(placement);
}
Plot::Pad::TextBox& Plot::Pad::AddText(box_placement_t placement, const vector<string>& lines)
{
  return AddText(lines).SetAutoPlacement(placement);
}
Plot::Pad::TextBox& Plot::Pad::AddText(box_placement_t placement, std::initializer_list<string> lines)
{
  return AddText(placement, vector<string>(lines));
}

//**************************************************************************************************
/**
 * Add an image (e.g. a logo) to this pad, at the given upper left corner or automatically placed (close to the given corner of the frame).
 */
//**************************************************************************************************
Plot::Pad::ImageBox& Plot::Pad::AddImage(double_t xPos, double_t yPos, const string& path)
{
  mImageBoxes.push_back(std::make_shared<ImageBox>(xPos, yPos, path));
  return *mImageBoxes.back();
}
Plot::Pad::ImageBox& Plot::Pad::AddImage(const string& path)
{
  mImageBoxes.push_back(std::make_shared<ImageBox>(path));
  return *mImageBoxes.back();
}
Plot::Pad::ImageBox& Plot::Pad::AddImage(box_placement_t placement, const string& path)
{
  return AddImage(path).SetAutoPlacement(placement);
}

//**************************************************************************************************
/**
 * Add legend box to this pad.
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::AddLegend(double_t xPos, double_t yPos, const std::optional<std::string>& title)
{
  mLegendBoxes.push_back(std::make_shared<LegendBox>(xPos, yPos, title));
  return *mLegendBoxes.back();
}

//**************************************************************************************************
/**
 * Add legend box to this pad and automatically find a position for it.
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::AddLegend(const std::optional<std::string>& title)
{
  mLegendBoxes.push_back(std::make_shared<LegendBox>(title));
  return *mLegendBoxes.back();
}

//**************************************************************************************************
/**
 * Add legend box to this pad and automatically find a position for it (close to the given corner of the frame or the one where it fits best).
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::AddLegend(box_placement_t placement, const std::optional<std::string>& title)
{
  return AddLegend(title).SetAutoPlacement(placement);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Data
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Default constructor for Data objects.
 */
//**************************************************************************************************
Plot::Pad::Data::Data(const string& name, const string& dataSource, const optional<string>& legendLabel)
  : Data()
{
  mType = "data";

  mLegend.label = legendLabel;
  mName = name;
  mDataSource = dataSource;  // can be restricted to a folder (dataSource:some/folder), which stays part of the data source
}

//**************************************************************************************************
/**
 * Use this data as template: copy with the given name (and optionally another data source).
 */
//**************************************************************************************************
auto Plot::Pad::Data::operator()(const string& name, const optional<string>& dataSource) const -> Data
{
  Data data = *this;
  data.mType = "data";
  data.mName = name;
  if (dataSource) data.mDataSource = *dataSource;
  return data;
}

//**************************************************************************************************
/**
 * Constructor for Data objects from property tree.
 */
//**************************************************************************************************
Plot::Pad::Data::Data(const ptree& dataTree) : Data()
{
  try {
    mType = dataTree.get<string>("type");
    mName = dataTree.get<string>("name");
    mDataSource = dataTree.get<string>("data_source");
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct data from ptree: {}", e.what());
  }

  if (auto var = get_from_tree<bool>(dataTree, "defines_frame")) mDefinesFrame = *var;
  if (auto var = get_from_tree<bool>(dataTree, "keep_input_style")) mKeepInputStyle = *var;
  read_from_tree(dataTree, mLegend.label, "legend_label");
  read_from_tree(dataTree, mLegend.id, "legend_id");
  if (const auto entryTree = dataTree.get_child_optional("legend_entry")) mLegend.entry = LegendEntry(*entryTree);
  read_from_tree(dataTree, mDrawOption, "draw_option");
  ReadNamed(dataTree, mDrawOptionAlias, "draw_option_alias");
  read_from_tree(dataTree, mTextFormat, "text_format");
  read_from_tree(dataTree, mTextAngle, "text_angle");
  read_from_tree(dataTree, mStats.format, "stats_format");
  read_from_tree(dataTree, mStats.title, "stats_title");
  if (const auto statsBoxTree = dataTree.get_child_optional("stats_box")) mStats.box = std::make_shared<const StatsBox>(*statsBoxTree);
  ReadMarker(dataTree);
  ReadLine(dataTree);
  ReadFill(dataTree);
  ReadModify(dataTree, mModify);
  read_from_tree(dataTree, mRangeX.min, "range_x_min");
  read_from_tree(dataTree, mRangeX.max, "range_x_max");
  read_from_tree(dataTree, mRangeY.min, "range_y_min");
  read_from_tree(dataTree, mRangeY.max, "range_y_max");
  read_from_tree(dataTree, mContours, "contours");
  read_from_tree(dataTree, mNContours, "num_contours");
  read_from_tree(dataTree, mNpx, "npx");
  read_from_tree(dataTree, mNpy, "npy");

  ReadDataInfo(dataTree, mDataInfo, "data_");
  ReadProjInfo(dataTree, mProjInfo, "proj_");
}

//**************************************************************************************************
/**
 * Read / write the selection of tree or table data and the projection of histograms (key prefixes: data_ and proj_, for the denominator
 * of a ratio denomData_ and denomProj_).
 */
//**************************************************************************************************
void Plot::Pad::Data::ReadDataInfo(const ptree& tree, data_info_t& dataInfo, const string& prefix)
{
  optional<vector<string>> vars;
  read_from_tree(tree, vars, prefix + "vars");
  if (!vars) return;
  dataInfo = data_info_t();
  optional<vector<double_t>> binning;
  optional<vector<int32_t>> sizes;
  read_from_tree(tree, binning, prefix + "binning");
  read_from_tree(tree, sizes, prefix + "binning_sizes");
  if (binning && sizes) {
    if (sizes->size() != vars->size()) {
      logger::throw_invalid_argument("{}binning_sizes has {} entries, expected {} (one per {}vars entry).", prefix, sizes->size(), vars->size(), prefix);
    }
    auto lastPos = binning->begin();
    for (size_t i = 0; i < vars->size(); ++i) {
      if ((*sizes)[i] < 1) {
        logger::throw_invalid_argument("{}binning_sizes entry {} must be at least 1.", prefix, i);
      }
      if (std::distance(lastPos, binning->end()) < static_cast<std::ptrdiff_t>((*sizes)[i])) {
        logger::throw_invalid_argument("{}binning does not contain enough entries for {}binning_sizes.", prefix, prefix);
      }
      auto nextPos = lastPos + (*sizes)[i];
      vector<double_t> edges(lastPos, nextPos);
      lastPos = nextPos;
      int32_t nBins = static_cast<int32_t>(edges.back());
      edges.pop_back();
      dataInfo.dataDims.push_back({vars->at(i), nBins, edges});
    }
  }
  read_from_tree(tree, dataInfo.filters, prefix + "filters");
  dataInfo.joins = ReadJoins(tree, prefix + "joins");
  read_from_tree(tree, dataInfo.weight, prefix + "weight");
  if (const auto entryMax = get_from_tree<uint32_t>(tree, prefix + "entry_max")) {
    dataInfo.entries.Set(get_from_tree<uint32_t>(tree, prefix + "entry_min"), *entryMax);
  }
  read_from_tree(tree, dataInfo.isProfileNoScatter, prefix + "is_profile_no_scatter");
  read_from_tree(tree, dataInfo.definitions.keys, prefix + "definitions_keys");
  read_from_tree(tree, dataInfo.definitions.values, prefix + "definitions_values");
}
void Plot::Pad::Data::PutDataInfo(ptree& tree, const data_info_t& dataInfo, const string& prefix)
{
  if (dataInfo.dataDims.empty()) return;
  vector<string> vars;
  vector<double_t> binning;
  vector<int32_t> sizes;
  for (const auto& dataDim : dataInfo.dataDims) {
    vars.push_back(dataDim.var);
    binning.insert(binning.end(), dataDim.edges.begin(), dataDim.edges.end());
    binning.push_back(static_cast<double_t>(dataDim.nBins));
    sizes.push_back(static_cast<int32_t>(dataDim.edges.size()) + 1);
  }
  put_in_tree(tree, optional<vector<string>>{vars}, prefix + "vars");
  put_in_tree(tree, optional<vector<double_t>>{binning}, prefix + "binning");
  put_in_tree(tree, optional<vector<int32_t>>{sizes}, prefix + "binning_sizes");
  put_in_tree(tree, dataInfo.filters, prefix + "filters");
  PutJoins(tree, dataInfo.joins, prefix + "joins");
  put_in_tree(tree, dataInfo.weight, prefix + "weight");
  put_in_tree(tree, dataInfo.entries.min, prefix + "entry_min");
  put_in_tree(tree, dataInfo.entries.max, prefix + "entry_max");
  put_in_tree(tree, dataInfo.isProfileNoScatter, prefix + "is_profile_no_scatter");
  put_in_tree(tree, dataInfo.definitions.keys, prefix + "definitions_keys");
  put_in_tree(tree, dataInfo.definitions.values, prefix + "definitions_values");
}
void Plot::Pad::Data::ReadProjInfo(const ptree& tree, optional<proj_info_t>& projInfo, const string& prefix)
{
  optional<vector<uint8_t>> dims;
  optional<vector<tuple<uint8_t, double_t, double_t>>> ranges;
  optional<bool> isUserCoord;
  optional<bool> isProfile;
  optional<uint8_t> averagedAxis;
  read_from_tree(tree, dims, prefix + "dims");
  read_from_tree(tree, ranges, prefix + "ranges");
  read_from_tree(tree, isUserCoord, prefix + "is_user_coordinates");
  read_from_tree(tree, isProfile, prefix + "is_profile");
  read_from_tree(tree, averagedAxis, prefix + "averaged_axis");
  if (!dims) return;
  if (!ranges) {
    logger::throw_invalid_argument("{}dims is set but {}ranges is missing.", prefix, prefix);
  }
  projInfo = {*dims, *ranges, isUserCoord, isProfile, averagedAxis};
}
void Plot::Pad::Data::PutProjInfo(ptree& tree, const optional<proj_info_t>& projInfo, const string& prefix)
{
  if (!projInfo) return;
  put_in_tree(tree, optional<vector<uint8_t>>{projInfo->dims}, prefix + "dims");
  put_in_tree(tree, optional<vector<tuple<uint8_t, double_t, double_t>>>{projInfo->ranges}, prefix + "ranges");
  put_in_tree(tree, projInfo->isUserCoord, prefix + "is_user_coordinates");
  put_in_tree(tree, projInfo->isProfile, prefix + "is_profile");
  put_in_tree(tree, projInfo->averagedAxis, prefix + "averaged_axis");
}

//**************************************************************************************************
/**
 * Read / write the data modifiers (optionally with a key prefix, used for ratio numerator and denominator).
 */
//**************************************************************************************************
void Plot::Pad::Data::ReadModify(const ptree& tree, modify_t& modify, const string& prefix)
{
  read_from_tree(tree, modify.showOverflowBins, prefix + "show_overflow_bins");
  read_from_tree(tree, modify.scaleFactor, prefix + "scale_factor");
  read_from_tree(tree, modify.scaleBinWidthNorm, prefix + "scale_bin_width_norm");
  read_from_tree(tree, modify.normMaximum, prefix + "norm_to_maximum");
  read_from_tree(tree, modify.normMin, prefix + "norm_min");
  read_from_tree(tree, modify.normMax, prefix + "norm_max");
  read_from_tree(tree, modify.normMaximumMin, prefix + "norm_to_maximum_min");
  read_from_tree(tree, modify.normMaximumMax, prefix + "norm_to_maximum_max");
  read_from_tree(tree, modify.divideBinWidth, prefix + "divide_bin_width");
  read_from_tree(tree, modify.rebinGroupX, prefix + "rebin_x");
  read_from_tree(tree, modify.rebinGroupY, prefix + "rebin_y");
  read_from_tree(tree, modify.rebinGroupZ, prefix + "rebin_z");
  read_from_tree(tree, modify.scaleAxisX, prefix + "scale_axis_x");
  read_from_tree(tree, modify.scaleAxisY, prefix + "scale_axis_y");
  read_from_tree(tree, modify.scaleAxisZ, prefix + "scale_axis_z");
  read_from_tree(tree, modify.cumulative, prefix + "cumulative");
  read_from_tree(tree, modify.nIterSmooth, prefix + "num_smooth_iterations");
}
void Plot::Pad::Data::PutModify(ptree& tree, const modify_t& modify, const string& prefix)
{
  put_in_tree(tree, modify.showOverflowBins, prefix + "show_overflow_bins");
  put_in_tree(tree, modify.scaleFactor, prefix + "scale_factor");
  put_in_tree(tree, modify.scaleBinWidthNorm, prefix + "scale_bin_width_norm");
  put_in_tree(tree, modify.normMaximum, prefix + "norm_to_maximum");
  put_in_tree(tree, modify.normMin, prefix + "norm_min");
  put_in_tree(tree, modify.normMax, prefix + "norm_max");
  put_in_tree(tree, modify.normMaximumMin, prefix + "norm_to_maximum_min");
  put_in_tree(tree, modify.normMaximumMax, prefix + "norm_to_maximum_max");
  put_in_tree(tree, modify.divideBinWidth, prefix + "divide_bin_width");
  put_in_tree(tree, modify.rebinGroupX, prefix + "rebin_x");
  put_in_tree(tree, modify.rebinGroupY, prefix + "rebin_y");
  put_in_tree(tree, modify.rebinGroupZ, prefix + "rebin_z");
  put_in_tree(tree, modify.scaleAxisX, prefix + "scale_axis_x");
  put_in_tree(tree, modify.scaleAxisY, prefix + "scale_axis_y");
  put_in_tree(tree, modify.scaleAxisZ, prefix + "scale_axis_z");
  put_in_tree(tree, modify.cumulative, prefix + "cumulative");
  put_in_tree(tree, modify.nIterSmooth, prefix + "num_smooth_iterations");
}

//**************************************************************************************************
/**
 * Generate property tree for this Data object.
 */
//**************************************************************************************************
ptree Plot::Pad::Data::GetPropertyTree() const
{
  ptree dataTree;
  dataTree.put("type", mType);
  dataTree.put("name", mName);
  dataTree.put("data_source", mDataSource);
  if (mDefinesFrame) dataTree.put("defines_frame", mDefinesFrame);
  if (mKeepInputStyle) dataTree.put("keep_input_style", *mKeepInputStyle);
  put_in_tree(dataTree, mLegend.label, "legend_label");
  put_in_tree(dataTree, mLegend.id, "legend_id");
  if (mLegend.entry) dataTree.put_child("legend_entry", mLegend.entry->GetPropertyTree());
  put_in_tree(dataTree, mDrawOption, "draw_option");
  PutNamed(dataTree, mDrawOptionAlias, "draw_option_alias");
  put_in_tree(dataTree, mTextFormat, "text_format");
  put_in_tree(dataTree, mTextAngle, "text_angle");
  put_in_tree(dataTree, mStats.format, "stats_format");
  put_in_tree(dataTree, mStats.title, "stats_title");
  if (mStats.box) dataTree.put_child("stats_box", mStats.box->GetPropertyTree());
  PutMarker(dataTree);
  PutLine(dataTree);
  PutFill(dataTree);
  PutModify(dataTree, mModify);
  put_in_tree(dataTree, mRangeX.min, "range_x_min");
  put_in_tree(dataTree, mRangeX.max, "range_x_max");
  put_in_tree(dataTree, mRangeY.min, "range_y_min");
  put_in_tree(dataTree, mRangeY.max, "range_y_max");
  put_in_tree(dataTree, mContours, "contours");
  put_in_tree(dataTree, mNContours, "num_contours");
  put_in_tree(dataTree, mNpx, "npx");
  put_in_tree(dataTree, mNpy, "npy");

  PutDataInfo(dataTree, mDataInfo, "data_");
  PutProjInfo(dataTree, mProjInfo, "proj_");
  return dataTree;
}

//**************************************************************************************************
/**
 * Reinterpret Data as Ratio.
 */
//**************************************************************************************************
Plot::Pad::Ratio& Plot::Pad::Data::AsRatio()
{
  auto* ptr = dynamic_cast<Plot::Pad::Ratio*>(this);
  if (!ptr) {
    logger::throw_logic_error("Data {} is no ratio.", mName);
  }
  return *ptr;
}

//**************************************************************************************************
/**
 * User accessors for Data properties.
 */
//**************************************************************************************************
auto Plot::Pad::Data::SetLayout(const Data& dataLayout) -> decltype(*this)
{
  // set all properties related to the appearance of the data exactly as specified in the layout
  mDrawOption = dataLayout.mDrawOption;
  mKeepInputStyle = dataLayout.mKeepInputStyle;
  mDrawOptionAlias = dataLayout.mDrawOptionAlias;
  mTextFormat = dataLayout.mTextFormat;
  mTextAngle = dataLayout.mTextAngle;
  // an entry handed out by GetLegendEntry (also to Python) stays valid: replaced in place, emptied instead of removed
  if (mLegend.entry) {
    *mLegend.entry = dataLayout.mLegend.entry.value_or(LegendEntry{});
  } else {
    mLegend.entry = dataLayout.mLegend.entry;
  }
  mRangeX.min = dataLayout.mRangeX.min;
  mRangeX.max = dataLayout.mRangeX.max;
  mRangeY.min = dataLayout.mRangeY.min;
  mRangeY.max = dataLayout.mRangeY.max;
  mMarker = dataLayout.mMarker;
  mLine = dataLayout.mLine;
  mFill = dataLayout.mFill;
  mContours = dataLayout.mContours;
  mNContours = dataLayout.mNContours;
  mNpx = dataLayout.mNpx;
  mNpy = dataLayout.mNpy;
  return *this;
}
auto Plot::Pad::Data::ApplyLayout(const Data& dataLayout) -> decltype(*this)
{
  // apply only the properties of the layout which are set
  if (dataLayout.mDrawOption || dataLayout.mDrawOptionAlias) {  // string and alias are one setting: the layout's replaces both
    mDrawOption = dataLayout.mDrawOption;
    mDrawOptionAlias = dataLayout.mDrawOptionAlias;
  }
  if (dataLayout.mKeepInputStyle) mKeepInputStyle = dataLayout.mKeepInputStyle;
  if (dataLayout.mTextFormat) mTextFormat = dataLayout.mTextFormat;
  if (dataLayout.mTextAngle) mTextAngle = dataLayout.mTextAngle;
  if (dataLayout.mLegend.entry) {  // merged like the styles: what the data's own entry sets and the layout does not is kept
    if (mLegend.entry) {
      *mLegend.entry += *dataLayout.mLegend.entry;
      // += leaves the place of the row out (AddDataRow sets it apart): the layout's place counts as well
      if (dataLayout.mLegend.entry->mPosition) {
        mLegend.entry->mPosition = dataLayout.mLegend.entry->mPosition;
        mLegend.entry->mPositionDataID = dataLayout.mLegend.entry->mPositionDataID;
      }
    } else {
      mLegend.entry = dataLayout.mLegend.entry;
    }
  }
  if (dataLayout.mRangeX.min) mRangeX.min = dataLayout.mRangeX.min;
  if (dataLayout.mRangeX.max) mRangeX.max = dataLayout.mRangeX.max;
  if (dataLayout.mRangeY.min) mRangeY.min = dataLayout.mRangeY.min;
  if (dataLayout.mRangeY.max) mRangeY.max = dataLayout.mRangeY.max;
  mMarker.Update(dataLayout.mMarker);
  mLine.Update(dataLayout.mLine);
  mFill.Update(dataLayout.mFill);
  if (dataLayout.mContours || dataLayout.mNContours) {  // levels and their number are one setting: the layout's replaces both
    mContours = dataLayout.mContours;
    mNContours = dataLayout.mNContours;
  }
  if (dataLayout.mNpx) mNpx = dataLayout.mNpx;
  if (dataLayout.mNpy) mNpy = dataLayout.mNpy;
  return *this;
}
auto Plot::Pad::Data::SetDataSource(const string& dataSource) -> decltype(*this)
{
  mDataSource = dataSource;
  return *this;
}
auto Plot::Pad::Data::SetLegendLabel(const string& legendLabel) -> decltype(*this)
{
  mLegend.label = legendLabel;
  return *this;
}
auto Plot::Pad::Data::SetLegend(uint8_t legendID) -> decltype(*this)
{
  if (legendID == 0) {
    logger::warning("SetLegend(0) is ignored: legends are numbered from 1.");
    return *this;
  }
  mLegend.id = legendID;
  return *this;
}
auto Plot::Pad::Data::SetLegendEntry(const LegendEntry& legendEntry) -> decltype(*this)
{
  // the data decides whether it has a row and what it says, the entry how the row looks
  mLegend.entry = legendEntry;
  if (mLegend.entry->mLabel) {
    logger::warning("The legend entry of {} cannot set its label, use SetLegendLabel.", mName);
    mLegend.entry->mLabel.reset();
  }
  if (mLegend.entry->mRefDataID || mLegend.entry->mRefPadID) {  // e.g. a copy of an entry of LegendBox::AddDataEntry
    logger::warning("The legend entry of {} cannot refer to other data.", mName);
    mLegend.entry->mRefDataID.reset();
    mLegend.entry->mRefPadID.reset();
  }
  return *this;
}
auto Plot::Pad::Data::GetLegendEntry() -> LegendEntry&
{
  if (!mLegend.entry) mLegend.entry.emplace();
  return *mLegend.entry;
}
optional<Plot::Pad::LegendEntry> Plot::Pad::Data::GetLegendEntryIfSet() const
{
  if (!mLegend.entry) return {};
  LegendEntry entry = *mLegend.entry;
  if (entry.mLabel || entry.mRefDataID || entry.mRefPadID) {  // set on the entry from GetLegendEntry
    logger::warning("The legend entry of {} is ignored: it cannot set its label (use SetLegendLabel) or refer to other data.", mName);
    entry.mLabel.reset();
    entry.mRefDataID.reset();
    entry.mRefPadID.reset();
  }
  return entry;
}
auto Plot::Pad::Data::SetDrawOption(const string& drawOption) -> decltype(*this)
{
  // the name of an alias given as raw option (SetDrawOption("col_clamped") for SetDrawOption(col_clamped)) goes to ROOT as it is: said,
  // unless the name is also an option ROOT knows (hist, colz, lego, ...)
  static const std::set<string> rootOptions{"box", "candle", "col", "colz", "cont", "hist", "iso", "lego", "surf", "text", "violin"};
  const size_t first = drawOption.find_first_not_of(" \t");
  const string name = (first == string::npos) ? "" : drawOption.substr(first, drawOption.find_last_not_of(" \t") - first + 1);
  if (enum_from_name<draw_option_t>(name) && !rootOptions.count(name)) {
    logger::warning("Drawing option '{}' of {} is the name of an alias, but is passed to ROOT as it is: write SetDrawOption({}) for the alias.", name, mName, name);
  }
  mDrawOption = drawOption;
  mDrawOptionAlias = {};
  return *this;
}
auto Plot::Pad::Data::SetDrawOption(draw_option_t optionAlias) -> decltype(*this)
{
  mDrawOptionAlias = optionAlias;
  mDrawOption = {};
  return *this;
}
auto Plot::Pad::Data::SetDrawOption() -> decltype(*this)
{
  mDrawOptionAlias = {};
  mDrawOption = {};
  return *this;
}
auto Plot::Pad::Data::SetTextFormat(const string& textFormat) -> decltype(*this)
{
  // printed with a % in front (as ROOT's text option does): a conversion of a floating point number, text may follow
  static const std::regex valid(R"(^[-+ #0]*\d*(\.\d*)?[eEfFgGaA]([^%]|%%)*$)");
  if (!std::regex_match(textFormat, valid)) {
    logger::warning("The text format '{}' of {} is ignored: it is not a conversion of a number without % (e.g. 4.2f or .3g).", textFormat, mName);
    return *this;
  }
  mTextFormat = textFormat;
  return *this;
}
auto Plot::Pad::Data::SetTextAngle(float_t angle) -> decltype(*this)
{
  if (CheckFinite(angle, "text angle of " + GetName())) mTextAngle = angle;
  return *this;
}
auto Plot::Pad::Data::SetShowStats(const string& format, const optional<string>& title) -> decltype(*this)
{
  // the first line of the box ('n', which would be the name of the histogram) is shown exactly when there is a title
  if (format.find_first_not_of("nemMrRuoiIsSkK") != string::npos) {
    logger::warning("The statistics box format '{}' of {} is ignored: it may only hold letters of TStyle::SetOptStat (nemMrRuoiIsSkK).", format, mName);
    return *this;
  }
  mStats.format = format;
  mStats.format->erase(std::remove(mStats.format->begin(), mStats.format->end(), 'n'), mStats.format->end());
  mStats.title = title;
  return *this;
}
auto Plot::Pad::Data::SetStatsBox(const StatsBox& statsBox) -> decltype(*this)
{
  mStats.box = std::make_shared<const StatsBox>(statsBox);
  return *this;
}
auto Plot::Pad::Data::SetRangeX(double_t min, double_t max) -> decltype(*this)
{
  bool valid = CheckFinite(min, "range minimum of " + GetName());
  valid = CheckFinite(max, "range maximum of " + GetName()) && valid;
  if (!valid) return *this;
  if (min >= max) {
    logger::warning("The x range [{}, {}] of {} is ignored: its minimum is not below its maximum.", min, max, mName);
    return *this;
  }
  mRangeX.min = min;
  mRangeX.max = max;
  return *this;
}
auto Plot::Pad::Data::SetMinRangeX(double_t min) -> decltype(*this)
{
  if (CheckFinite(min, "range minimum of " + GetName())) mRangeX.min = min;
  return *this;
}
auto Plot::Pad::Data::SetMaxRangeX(double_t max) -> decltype(*this)
{
  if (CheckFinite(max, "range maximum of " + GetName())) mRangeX.max = max;
  return *this;
}
auto Plot::Pad::Data::SetRangeX() -> decltype(*this)
{
  mRangeX.min = {};
  mRangeX.max = {};
  return *this;
}
auto Plot::Pad::Data::SetRangeY(double_t min, double_t max) -> decltype(*this)
{
  bool valid = CheckFinite(min, "range minimum of " + GetName());
  valid = CheckFinite(max, "range maximum of " + GetName()) && valid;
  if (!valid) return *this;
  if (min >= max) {
    logger::warning("The y range [{}, {}] of {} is ignored: its minimum is not below its maximum.", min, max, mName);
    return *this;
  }
  mRangeY.min = min;
  mRangeY.max = max;
  return *this;
}
auto Plot::Pad::Data::SetMinRangeY(double_t min) -> decltype(*this)
{
  if (CheckFinite(min, "range minimum of " + GetName())) mRangeY.min = min;
  return *this;
}
auto Plot::Pad::Data::SetMaxRangeY(double_t max) -> decltype(*this)
{
  if (CheckFinite(max, "range maximum of " + GetName())) mRangeY.max = max;
  return *this;
}
auto Plot::Pad::Data::SetRangeY() -> decltype(*this)
{
  mRangeY.min = {};
  mRangeY.max = {};
  return *this;
}
auto Plot::Pad::Data::SetShowOverflowBins(bool showOverflowBins) -> decltype(*this)
{
  mModify.showOverflowBins = showOverflowBins;
  return *this;
}
auto Plot::Pad::Data::SetColor(color_t color) -> decltype(*this)
{
  mMarker.color = color;
  mLine.color = color;
  mFill.color = color;
  return *this;
}
auto Plot::Pad::Data::SetAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, fmt::format("alpha of {}", mName))) return *this;
  mMarker.alpha = alpha;
  mLine.alpha = alpha;
  mFill.alpha = alpha;
  return *this;
}
auto Plot::Pad::Data::SetDefinesFrame(bool define) -> decltype(*this)
{
  mDefinesFrame = define;
  return *this;
}
auto Plot::Pad::Data::SetKeepInputStyle(bool keep) -> decltype(*this)
{
  mKeepInputStyle = keep;
  return *this;
}
auto Plot::Pad::Data::SetContours(const vector<double>& contours) -> decltype(*this)
{
  const bool finite = std::all_of(contours.begin(), contours.end(), [](double level) { return std::isfinite(level); });
  if (!finite) {
    logger::warning("The contour levels of {} are ignored: they are not all finite numbers.", mName);
    return *this;
  }
  if (std::adjacent_find(contours.begin(), contours.end(), std::greater_equal<double>()) != contours.end()) {
    logger::warning("The contour levels of {} are ignored: ROOT expects them in increasing order.", mName);
    return *this;
  }
  mContours = contours;
  mNContours.reset();  // levels and their number are one setting
  return *this;
}
auto Plot::Pad::Data::SetContours(int32_t nContours) -> decltype(*this)
{
  if (!CheckCount(nContours, "number of contour levels of " + GetName())) return *this;
  mNContours = nContours;
  mContours.reset();  // levels and their number are one setting
  return *this;
}
auto Plot::Pad::Data::SetNpx(int32_t nx, optional<int32_t> ny) -> decltype(*this)
{
  bool valid = CheckCount(nx, "number of x samples of " + GetName());
  valid = (!ny || CheckCount(*ny, "number of y samples of " + GetName())) && valid;
  if (!valid) return *this;
  mNpx = nx;
  mNpy = ny;
  return *this;
}
auto Plot::Pad::Data::Normalize(bool isDensity, optional<double_t> min, optional<double_t> max) -> decltype(*this)
{
  if (min && !CheckFinite(*min, "Normalize range minimum of " + GetName())) min.reset();
  if (max && !CheckFinite(*max, "Normalize range maximum of " + GetName())) max.reset();
  if (min && max && !(*min < *max)) {
    logger::warning("The range [{}, {}] of Normalize of {} is ignored: it is empty.", *min, *max, GetName());
    min.reset();
    max.reset();
  }
  mModify.scaleBinWidthNorm = isDensity;
  mModify.normMin = min;
  mModify.normMax = max;
  return *this;
}
auto Plot::Pad::Data::NormalizeToMaximum(optional<double_t> min, optional<double_t> max) -> decltype(*this)
{
  if (min && !CheckFinite(*min, "NormalizeToMaximum range minimum of " + GetName())) min.reset();
  if (max && !CheckFinite(*max, "NormalizeToMaximum range maximum of " + GetName())) max.reset();
  if (min && max && !(*min < *max)) {
    logger::warning("The range [{}, {}] of NormalizeToMaximum of {} is ignored: it is empty.", *min, *max, GetName());
    min.reset();
    max.reset();
  }
  mModify.normMaximum = true;
  mModify.normMaximumMin = min;
  mModify.normMaximumMax = max;
  return *this;
}
auto Plot::Pad::Data::Scale(double_t scaleFactor) -> decltype(*this)
{
  if (CheckFinite(scaleFactor, "scale factor of " + GetName())) mModify.scaleFactor = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::Smooth(uint16_t nIterSmooth) -> decltype(*this)
{
  if (nIterSmooth == 0) {
    logger::warning("Smooth(0) of {} is ignored: it does not smooth, give the number of iterations.", mName);
    return *this;
  }
  mModify.nIterSmooth = nIterSmooth;
  return *this;
}
auto Plot::Pad::Data::Cumulative(bool forward) -> decltype(*this)
{
  mModify.cumulative = forward;
  return *this;
}
auto Plot::Pad::Data::DivideBinWidth(bool divideBinWidth) -> decltype(*this)
{
  mModify.divideBinWidth = divideBinWidth;
  return *this;
}
auto Plot::Pad::Data::Density() -> decltype(*this)
{
  // divided by the bin widths, the data is a density: Normalize() then uses the bin-width-weighted integral
  mModify.divideBinWidth = true;
  mModify.scaleBinWidthNorm = false;
  return *this;
}
auto Plot::Pad::Data::RebinX(uint16_t nGroup) -> decltype(*this)
{
  if (CheckCount(nGroup, "number of bins to merge for " + GetName())) mModify.rebinGroupX = nGroup;
  return *this;
}
auto Plot::Pad::Data::RebinY(uint16_t nGroup) -> decltype(*this)
{
  if (CheckCount(nGroup, "number of bins to merge for " + GetName())) mModify.rebinGroupY = nGroup;
  return *this;
}
auto Plot::Pad::Data::RebinZ(uint16_t nGroup) -> decltype(*this)
{
  if (CheckCount(nGroup, "number of bins to merge for " + GetName())) mModify.rebinGroupZ = nGroup;
  return *this;
}
auto Plot::Pad::Data::RebinXY(uint16_t nGroupX, uint16_t nGroupY) -> decltype(*this)
{
  bool valid = CheckCount(nGroupX, "number of x bins to merge for " + GetName());
  valid = CheckCount(nGroupY, "number of y bins to merge for " + GetName()) && valid;
  if (!valid) return *this;
  mModify.rebinGroupX = nGroupX;
  mModify.rebinGroupY = nGroupY;
  return *this;
}
auto Plot::Pad::Data::RebinXYZ(uint16_t nGroupX, uint16_t nGroupY, uint16_t nGroupZ) -> decltype(*this)
{
  bool valid = CheckCount(nGroupX, "number of x bins to merge for " + GetName());
  valid = CheckCount(nGroupY, "number of y bins to merge for " + GetName()) && valid;
  valid = CheckCount(nGroupZ, "number of z bins to merge for " + GetName()) && valid;
  if (!valid) return *this;
  mModify.rebinGroupX = nGroupX;
  mModify.rebinGroupY = nGroupY;
  mModify.rebinGroupZ = nGroupZ;
  return *this;
}
auto Plot::Pad::Data::ScaleX(double_t scaleFactor) -> decltype(*this)
{
  if (CheckFinite(scaleFactor, "x scale factor of " + GetName())) mModify.scaleAxisX = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::ScaleY(double_t scaleFactor) -> decltype(*this)
{
  if (CheckFinite(scaleFactor, "y scale factor of " + GetName())) mModify.scaleAxisY = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::ScaleZ(double_t scaleFactor) -> decltype(*this)
{
  if (CheckFinite(scaleFactor, "z scale factor of " + GetName())) mModify.scaleAxisZ = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::Project(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  if (dims.empty()) logger::throw_invalid_argument("Project needs at least one axis to project onto.");
  mProjInfo = {dims, FiniteRanges(ranges), isUserCoord};
  return *this;
}
auto Plot::Pad::Data::ProjectX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{0}, FiniteRanges({{1, startY, endY}}), isUserCoord};
  return *this;
}
auto Plot::Pad::Data::ProjectY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{1}, FiniteRanges({{0, startX, endX}}), isUserCoord};
  return *this;
}
auto Plot::Pad::Data::Profile(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  if (dims.empty()) logger::throw_invalid_argument("Profile needs at least one axis to profile along.");
  mProjInfo = {dims, FiniteRanges(ranges), isUserCoord, true};
  return *this;
}
auto Plot::Pad::Data::Profile(uint8_t averagedAxis, const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  if (dims.empty()) logger::throw_invalid_argument("Profile needs at least one axis to profile along.");
  if (std::find(dims.begin(), dims.end(), averagedAxis) != dims.end()) logger::throw_invalid_argument("Profile cannot average axis {}, which it profiles along.", static_cast<int>(averagedAxis));
  mProjInfo = {dims, FiniteRanges(ranges), isUserCoord, true, averagedAxis};
  return *this;
}
vector<tuple<uint8_t, double_t, double_t>> Plot::Pad::Data::FiniteRanges(const vector<tuple<uint8_t, double_t, double_t>>& ranges)
{
  vector<tuple<uint8_t, double_t, double_t>> finite;
  for (const auto& [axis, min, max] : ranges) {
    if (std::isfinite(min) && std::isfinite(max)) {
      finite.emplace_back(axis, min, max);
    } else {
      logger::warning("The projection range [{}, {}] of axis {} is ignored: a limit is not a finite number.", min, max, static_cast<int>(axis));
    }
  }
  return finite;
}
auto Plot::Pad::Data::ProfileX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{0}, FiniteRanges({{1, startY, endY}}), isUserCoord, true};
  return *this;
}
auto Plot::Pad::Data::ProfileY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{1}, FiniteRanges({{0, startX, endX}}), isUserCoord, true};
  return *this;
}
auto Plot::Pad::Data::Project(std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Project(vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Data::Profile(std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Profile(vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Data::Profile(uint8_t averagedAxis, std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Profile(averagedAxis, vector<uint8_t>(dims), ranges, isUserCoord);
}
void Plot::Pad::Data::data_info_t::set(const data_info_t& other)
{
  const bool isProfile = other.isProfileNoScatter.value_or(false);
  if (other.dataDims.size() < (isProfile ? 2u : 1u)) {  // a profile: a binned axis and the profiled variable
    if (isProfile) {
      logger::throw_invalid_argument("Profile needs at least one binned axis besides the profiled variable.");
    } else {
      logger::throw_invalid_argument("Project needs at least one dimension.");
    }
  }
  for (const auto& dim : other.dataDims) {
    if (is_blank(dim.var)) logger::throw_invalid_argument("Empty expression for an axis of a projection, profile or scatter.");
  }
  if (other.weight && is_blank(*other.weight)) logger::throw_invalid_argument("Empty weight expression (leave the weight out instead).");
  // binned axes (all of a projection, all but the profiled one of a profile) need a positive number of bins or explicit edges
  const size_t nBinned = other.isProfileNoScatter.value_or(true) ? other.dataDims.size() - other.isProfileNoScatter.value_or(false) : 0;
  for (size_t i = 0; i < nBinned; ++i) {
    const auto& dim = other.dataDims[i];
    if (dim.nBins < 0 || (dim.nBins == 0 && dim.edges.size() <= 2)) {
      logger::throw_invalid_argument("Number of bins for {} must be positive (or give at least two different bin edges).", dim.var);
    }
    if (dim.nBins > 0 && dim.edges.size() != 2) {
      logger::throw_invalid_argument("A number of bins for {} takes a range of two values (got {}).", dim.var, dim.edges.size());
    }
    const bool finite = std::all_of(dim.edges.begin(), dim.edges.end(), [](double_t edge) { return std::isfinite(edge); });
    if (!finite) {
      logger::throw_invalid_argument("The bin {} of {} must be finite numbers.", dim.nBins ? "range" : "edges", dim.var);
    }
    if (dim.nBins > 0 && dim.edges[0] > dim.edges[1]) {
      logger::throw_invalid_argument("The range of {} is inverted ([{}, {}]).", dim.var, dim.edges[0], dim.edges[1]);
    }
    if (const auto edge = std::adjacent_find(dim.edges.begin(), dim.edges.end(), std::greater_equal<>()); !dim.nBins && edge != dim.edges.end()) {
      logger::throw_invalid_argument("The bin edges of {} must increase ({} is followed by {}).", dim.var, *edge, *(edge + 1));
    }
  }
  dataDims = other.dataDims;
  weight = other.weight;
  isProfileNoScatter = other.isProfileNoScatter;
}
auto Plot::Pad::Data::Project(std::initializer_list<data_dim_t> dataDims, optional<string> weight) -> decltype(*this)
{
  return Project(vector<data_dim_t>(dataDims), weight);
}
auto Plot::Pad::Data::Project(const vector<data_dim_t>& dataDims, optional<string> weight) -> decltype(*this)
{
  mDataInfo.set({dataDims, weight});
  return *this;
}
auto Plot::Pad::Data::Project1D(data_dim_t x, optional<string> weight) -> decltype(*this)
{
  mDataInfo.set({{x}, weight});
  return *this;
}
auto Plot::Pad::Data::Project2D(data_dim_t x, data_dim_t y, optional<string> weight) -> decltype(*this)
{
  mDataInfo.set({{x, y}, weight});
  return *this;
}
auto Plot::Pad::Data::Scatter(const string& x, const string& y) -> decltype(*this)
{
  mDataInfo.set({{{x}, {y}}, {}, false});
  return *this;
}
auto Plot::Pad::Data::Scatter(const string& x, const string& y, const string& xErr, const string& yErr) -> decltype(*this)
{
  mDataInfo.set({{{x}, {y}, {xErr}, {yErr}}, {}, false});
  return *this;
}
auto Plot::Pad::Data::Scatter(const string& x, const string& y, const string& xErrLow, const string& xErrHigh, const string& yErrLow, const string& yErrHigh) -> decltype(*this)
{
  mDataInfo.set({{{x}, {y}, {xErrLow}, {xErrHigh}, {yErrLow}, {yErrHigh}}, {}, false});
  return *this;
}
auto Plot::Pad::Data::Scatter(const string& x, const string& y, const string& z) -> decltype(*this)
{
  mDataInfo.set({{{x}, {y}, {z}}, {}, false});
  return *this;
}
auto Plot::Pad::Data::Profile(const vector<data_dim_t>& dataDims, const string& profile, optional<string> weight) -> decltype(*this)
{
  vector<data_dim_t> dims = dataDims;  // checked with the profiled variable last, as the other forms
  dims.push_back({profile, {}});
  mDataInfo.set({dims, weight, true});
  return *this;
}
auto Plot::Pad::Data::Profile1D(data_dim_t x, const string& profile, optional<string> weight) -> decltype(*this)
{
  mDataInfo.set({{x, {profile, {}}}, weight, true});
  return *this;
}
auto Plot::Pad::Data::Profile2D(data_dim_t x, data_dim_t y, const string& profile, optional<string> weight) -> decltype(*this)
{
  mDataInfo.set({{x, y, {profile, {}}}, weight, true});
  return *this;
}
auto Plot::Pad::Data::Define(const string& key, const string& value) -> decltype(*this)
{
  if (is_blank(key) || is_blank(value)) logger::throw_invalid_argument("Define needs a column name and an expression.");
  if (mDataInfo.definitions.keys && mDataInfo.definitions.values) {
    mDataInfo.definitions.keys->push_back(key);
    mDataInfo.definitions.values->push_back(value);
  } else {
    mDataInfo.definitions.keys = {key};
    mDataInfo.definitions.values = {value};
  }
  return *this;
}
auto Plot::Pad::Data::Filter(const std::string& filter) -> decltype(*this)
{
  if (is_blank(filter)) logger::throw_invalid_argument("Empty filter expression.");
  if (mDataInfo.filters) {
    mDataInfo.filters->push_back(filter);
  } else {
    mDataInfo.filters = {filter};
  }
  return *this;
}
// joins are stored in their string form
auto Plot::Pad::Data::ReadJoins(const ptree& tree, const string& label) -> optional<vector<data_info_t::join_t>>
{
  const auto strs = get_from_tree<vector<string>>(tree, label);
  if (!strs) return nullopt;
  vector<data_info_t::join_t> joins;
  for (const auto& str : *strs) {
    if (const auto nBars = std::count(str.begin(), str.end(), '|'); nBars != 3 && nBars != 4) {
      logger::throw_invalid_argument("Bad value '{}' of {}: tree|data source|keys|alias[|index column]", str, label);
    }
    joins.push_back(data_info_t::join_t::FromString(str));
  }
  return joins;
}
void Plot::Pad::Data::PutJoins(ptree& tree, const optional<vector<data_info_t::join_t>>& joins, const string& label)
{
  if (!joins) return;
  vector<string> strs;
  for (const auto& join : *joins) {
    strs.push_back(join.ToString());
  }
  put_in_tree(tree, optional<vector<string>>{strs}, label);
}
auto Plot::Pad::Data::Join(const join_tree_t& tree, const std::vector<std::string>& keys, const std::string& alias) -> decltype(*this)
{
  AddJoin(mDataInfo, mName, tree, keys, alias, "");
  return *this;
}
auto Plot::Pad::Data::JoinByIndex(const join_tree_t& tree, const std::string& indexColumn, const std::string& alias) -> decltype(*this)
{
  AddJoin(mDataInfo, mName, tree, {}, alias, indexColumn);
  return *this;
}
void Plot::Pad::Data::AddJoin(data_info_t& dataInfo, const std::string& name, const join_tree_t& tree, const std::vector<std::string>& keys, const std::string& alias,
                              const std::string& indexColumn)
{
  // names of columns and aliases are used in expressions
  auto isIdentifier = [](const std::string& str) {
    return !str.empty() && !std::isdigit(static_cast<unsigned char>(str[0])) && std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; });
  };
  if (tree.name.empty() || tree.name.front() == '/' || tree.name.back() == '/' || tree.name.find("//") != std::string::npos) {
    logger::throw_invalid_argument("Cannot join tree '{}' to {}: invalid tree name.", tree.name, name);
  }
  if (auto illegal = find_illegal_name_char(tree.name, true)) {
    logger::throw_invalid_argument("Cannot join tree '{}' to {}: the name contains illegal character '{}'.", tree.name, name, *illegal);
  }
  if (auto illegal = find_illegal_name_char(tree.dataSource.substr(0, tree.dataSource.find(':')))) {  // may be restricted to a folder: dataSource:some/folder
    logger::throw_invalid_argument("Cannot join tree {} to {}: data source '{}' contains illegal character '{}'.", tree.name, name, tree.dataSource, *illegal);
  }
  if (keys.size() > 2) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: at most two key columns are possible.", tree.name, name);
  }
  for (const auto& key : keys) {
    if (!isIdentifier(key)) logger::throw_invalid_argument("Cannot join tree {} to {}: '{}' is not a column name.", tree.name, name, key);
  }
  if (!indexColumn.empty() && !tree.dataSource.empty()) {
    logger::throw_invalid_argument("Cannot join tree {} from {} to {}: by an index column only a tree of the same data source can be joined.", tree.name, tree.dataSource, name);
  }
  if (!indexColumn.empty() && !isIdentifier(indexColumn)) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: '{}' is not a column name.", tree.name, name, indexColumn);
  }
  if (keys.size() == 2 && keys[0] == keys[1]) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: key column {} is given twice.", tree.name, name, keys[0]);
  }
  if (!alias.empty() && !isIdentifier(alias)) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: alias '{}' must consist of letters, digits and underscores only and must not start with a digit.", tree.name, name, alias);
  }
  data_info_t::join_t join{tree.name, tree.dataSource, keys, alias, indexColumn};
  if (!isIdentifier(join.GetAlias())) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: its columns cannot be named {}.column, please specify an alias.", tree.name, name, join.GetAlias());
  }
  // the columns of each tree must be distinguishable by their prefix (for the tree itself this is checked when joining, since its name can still change)
  auto suggestAlias = [&]() {
    if (!indexColumn.empty()) return fmt::format("JoinByIndex(\"{}\", \"{}\", \"{}\")", join.tree, indexColumn, "myAlias");
    return fmt::format("Join({}, {{{}}}, \"{}\")", (join.dataSource.empty()) ? "\"" + join.tree + "\"" : fmt::format("{{\"{}\", \"{}\"}}", join.tree, join.dataSource),
                       (keys.empty()) ? "" : "\"" + join_strings(keys, "\", \"") + "\"", "myAlias");
  };
  if (!dataInfo.joins) dataInfo.joins.emplace();
  for (const auto& other : *dataInfo.joins) {
    if (other == join) {
      logger::warning("The repeated Join of tree {} to {} is ignored: it is joined already.", other.GetDescription(), name);
      return;
    }
    if (other.GetAlias() == join.GetAlias()) {
      logger::throw_invalid_argument("Cannot join tree {} to {}: its columns would be called {}.column like those of the joined tree {}, please specify an alias, e.g. {}.", tree.name, name, join.GetAlias(), other.GetDescription(), suggestAlias());
    }
  }
  dataInfo.joins->push_back(join);
}
void Plot::Pad::Data::data_info_t::entry_range_t::Set(optional<uint32_t> entryMin, uint32_t entryMax)
{
  if (entryMin) {
    if (entryMax <= *entryMin) {
      logger::warning("The entry range [{}, {}) is ignored: it is empty.", *entryMin, entryMax);
      return;
    }
  } else if (!entryMax) {
    logger::warning("Entries(0) is ignored: it would select no entries at all.");
    return;
  }
  min = entryMin;
  max = entryMax;
}
auto Plot::Pad::Data::Entries(uint32_t nEntries) -> decltype(*this)
{
  mDataInfo.entries.Set(nullopt, nEntries);
  return *this;
}
auto Plot::Pad::Data::Entries(uint32_t entryMin, uint32_t entryMax) -> decltype(*this)
{
  mDataInfo.entries.Set(entryMin, entryMax);
  return *this;
}

//**************************************************************************************************
/**
 * Name suffix encoding the details of projections.
 */
//**************************************************************************************************
string Plot::Pad::Data::proj_info_t::GetNameSuffix() const
{
  string nameSuffix = (isProfile && *isProfile) ? "_Prof{" : "_Proj{";
  for (auto dim : dims) {
    nameSuffix += number_to_string(dim);
  }
  nameSuffix += "}";
  if (averagedAxis) nameSuffix += "_Mean{" + number_to_string(*averagedAxis) + "}";
  if (!ranges.empty()) {
    nameSuffix += "_Range";
    for (const auto& range : ranges) {
      nameSuffix += "{";
      nameSuffix += number_to_string(std::get<0>(range)) + ":";
      if (isUserCoord && *isUserCoord) {
        nameSuffix += number_to_string(std::get<1>(range)) + "," + number_to_string(std::get<2>(range));
      } else {
        nameSuffix += number_to_string(static_cast<int>(std::get<1>(range))) + "," + number_to_string(static_cast<int>(std::get<2>(range)));
      }
      nameSuffix += "}";
    }
  }
  if (isUserCoord && *isUserCoord) {
    nameSuffix += "_User";
  }
  return nameSuffix;
}

//**************************************************************************************************
/**
 * Trees joined to a tree.
 */
//**************************************************************************************************
string Plot::Pad::Data::data_info_t::join_t::GetAlias() const
{
  return (alias.empty()) ? split_path(tree).second : alias;
}
string Plot::Pad::Data::data_info_t::join_t::ToString() const
{
  return tree + "|" + dataSource + "|" + join_strings(keys, ",") + "|" + alias + ((indexColumn.empty()) ? "" : "|" + indexColumn);
}
auto Plot::Pad::Data::data_info_t::join_t::FromString(const string& str) -> join_t
{
  vector<string> fields(1);
  for (char c : str) {
    if (c == '|') {
      fields.emplace_back();
    } else {
      fields.back() += c;
    }
  }
  fields.resize(5);
  join_t join{fields[0], fields[1], {}, fields[3], fields[4]};
  if (!fields[2].empty()) join.keys = util::split_string(fields[2], ',');
  return join;
}
string Plot::Pad::Data::data_info_t::join_t::GetDescription() const
{
  string description = tree;
  if (!dataSource.empty()) description += " from " + dataSource;
  if (!alias.empty()) description += " as " + alias;
  if (!indexColumn.empty()) {
    description += " (row given by " + indexColumn + ")";
  } else {
    description += (keys.empty()) ? " (row by row)" : " (by " + join_strings(keys, ", ") + ")";
  }
  return description;
}

//**************************************************************************************************
/**
 * Name suffix encoding the details of tree projections.
 */
//**************************************************************************************************
string Plot::Pad::Data::data_info_t::GetNameSuffix() const
{
  if (dataDims.empty()) return "";
  // every request must get its own name: fields are tagged and separated by '@' (never part of C++ code outside of literals),
  // the parts of a dimension by ',' (these characters are escaped in the expressions); ';' is avoided since ROOT reads it as
  // the cycle of a key (Data.root), ':' and '/' since names are split there in data mode
  auto escape = [](const string& str, const string& separators) {
    string escaped;
    for (char c : str) {
      if (c == '\\' || separators.find(c) != string::npos) escaped += '\\';
      escaped += c;
    }
    return escaped;
  };
  // C++ code without whitespace that does not separate tokens ("eta > 0" -> "eta>0", but "x and y", "a - -b" stay):
  // expressions with the same tokens compute the same and get the same name; string and character literals are kept as they are
  // (';' between statements becomes '`', which C++ code has nowhere outside of literals, since ROOT cuts key names at ';')
  auto compact = [](const string& code) {
    auto isWord = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    auto isOperator = [](char c) { return std::string_view("+-*/%<>=!&|^~?:.#").find(c) != std::string_view::npos; };
    string compacted;
    char quote{};  // inside a string or character literal
    bool hadSpace{};
    for (size_t i = 0; i < code.size(); ++i) {
      const char c = code[i];
      if (quote) {
        compacted += c;
        if (c == '\\' && i + 1 < code.size()) {
          compacted += code[++i];
        } else if (c == quote) {
          quote = 0;
        }
        continue;
      }
      if (std::isspace(static_cast<unsigned char>(c))) {
        hadSpace = !compacted.empty();
        continue;
      }
      if (hadSpace) {
        const char previous = compacted.back();
        if ((isWord(previous) && isWord(c)) || (isOperator(previous) && isOperator(c))) compacted += ' ';
        hadSpace = false;
      }
      // a quote after a digit separates digits (1'000'000)
      if (c == '"' || (c == '\'' && (compacted.empty() || !std::isdigit(static_cast<unsigned char>(compacted.back()))))) quote = c;
      compacted += (c == ';') ? '`' : c;
    }
    return compacted;
  };
  vector<string> fields;
  for (const auto& dataDim : dataDims) {
    string field = "dim=" + escape(compact(dataDim.var), ",@") + "," + number_to_string(dataDim.nBins);
    for (const auto& edge : dataDim.edges) {
      field += "," + number_to_string(edge);
    }
    fields.push_back(field);
  }
  if (filters) {
    for (const auto& filter : *filters) {
      fields.push_back("filter=" + escape(compact(filter), "@"));
    }
  }
  if (definitions.keys && definitions.values) {
    for (size_t i = 0; i < definitions.keys->size() && i < definitions.values->size(); ++i) {
      fields.push_back("define=" + escape(compact(definitions.keys->at(i)), "=@") + "=" + escape(compact(definitions.values->at(i)), "@"));
    }
  }
  if (joins) {
    for (const auto& join : *joins) {
      fields.push_back("join=" + escape(join.ToString(), "@"));
    }
  }
  if (weight) fields.push_back("weight=" + escape(compact(*weight), "@"));
  if (entries.max) fields.push_back((entries.min) ? "entries=" + number_to_string(*entries.min) + "-" + number_to_string(*entries.max) : "first=" + number_to_string(*entries.max));
  if (isProfileNoScatter) fields.push_back(*isProfileNoScatter ? "profile" : "scatter");
  return "{" + join_strings(fields, "@") + "}";
}

//**************************************************************************************************
/**
 * Human readable description of tree projections (used in messages).
 */
//**************************************************************************************************
string Plot::Pad::Data::data_info_t::GetDescription() const
{
  string description = !isProfileNoScatter ? "projection" : (*isProfileNoScatter ? "profile" : "scatter");
  for (size_t i = 0; i < dataDims.size(); ++i) {
    description += ((i == 0) ? " of " : ", ") + dataDims[i].var;
  }
  if (definitions.keys && definitions.values) {
    for (size_t i = 0; i < definitions.keys->size() && i < definitions.values->size(); ++i) {
      description += "; Define " + definitions.keys->at(i) + " = " + definitions.values->at(i);
    }
  }
  if (filters) {
    for (const auto& filter : *filters) {
      description += "; Filter " + filter;
    }
  }
  if (joins) {
    for (const auto& join : *joins) {
      description += "; Join " + join.GetDescription();
    }
  }
  if (weight) description += "; weight " + *weight;
  if (entries.max) {
    description += (entries.min) ? "; entries " + std::to_string(*entries.min) + "-" + std::to_string(*entries.max) : "; first " + std::to_string(*entries.max) + " entries";
  }
  return description;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Ratio
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Constructor from numerator and denominator data: their input selections and modifiers (applied before the division)
 * are taken over, the ratio takes the appearance of the numerator.
 */
//**************************************************************************************************
Plot::Pad::Ratio::Ratio(const Data& numerator, const Data& denominator)
  : Ratio(numerator.mName, numerator.mDataSource, denominator.mName, denominator.mDataSource, {})
{
  for (const auto& [part, data] : {std::pair{"numerator", &numerator}, std::pair{"denominator", &denominator}}) {
    if (data->mType == "ratio") {
      logger::throw_invalid_argument("The {} of a ratio cannot be a ratio ({}).", part, data->mName);
    }
    if (data->mName.empty()) {
      logger::throw_invalid_argument("The {} of a ratio needs a name (data source: '{}').", part, data->mDataSource);
    }
  }
  if (numerator.mDataSource.empty()) {
    logger::throw_invalid_argument("The numerator of a ratio needs a data source ({}).", numerator.mName);
  }
  static_cast<Data&>(*this) = numerator;
  SetType("ratio");
  mNumModify = numerator.mModify;
  mModify = {};
  mDenomDataInfo = denominator.mDataInfo;
  mDenomProjInfo = denominator.mProjInfo;
  mDenomModify = denominator.mModify;
}

//**************************************************************************************************
/**
 * Constructor from names and data sources of numerator and denominator.
 */
//**************************************************************************************************
Plot::Pad::Ratio::Ratio(const string& name, const string& dataSource, const string& denomName,
                        const string& denomDataSource, const optional<string>& label)
  : Data(name, dataSource, label), mDenomName(denomName),
    mDenomDataSource(denomDataSource.empty() ? dataSource : denomDataSource), mCorrelated(false)
{
  SetType("ratio");
}

//**************************************************************************************************
/**
 * Constructor from property tree.
 */
//**************************************************************************************************
Plot::Pad::Ratio::Ratio(const ptree& dataTree) : Data(dataTree)
{
  try {
    mDenomName = dataTree.get<string>("denom_name");
    mDenomDataSource = dataTree.get<string>("denom_data_source");
    mCorrelated = get_from_tree<bool>(dataTree, "is_correlated").value_or(false);
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct ratio from ptree: {}", e.what());
  }

  ReadModify(dataTree, mNumModify, "numer_");
  ReadModify(dataTree, mDenomModify, "denom_");

  ReadDataInfo(dataTree, mDenomDataInfo, "denom_data_");
  ReadProjInfo(dataTree, mDenomProjInfo, "denom_proj_");
}

//**************************************************************************************************
/**
 * Convert ratio to property tree.
 */
//**************************************************************************************************
ptree Plot::Pad::Ratio::GetPropertyTree() const
{
  ptree dataTree = Data::GetPropertyTree();
  dataTree.put("denom_name", mDenomName);
  dataTree.put("denom_data_source", mDenomDataSource);
  dataTree.put("is_correlated", mCorrelated);
  PutModify(dataTree, mNumModify, "numer_");
  PutModify(dataTree, mDenomModify, "denom_");

  PutDataInfo(dataTree, mDenomDataInfo, "denom_data_");
  PutProjInfo(dataTree, mDenomProjInfo, "denom_proj_");
  return dataTree;
}

//**************************************************************************************************
/**
 * Numerator is a subset of the denominator: binomial errors.
 */
//**************************************************************************************************
auto Plot::Pad::Ratio::SetCorrelated(bool correlated) -> decltype(*this)
{
  mCorrelated = correlated;
  return *this;
}
//**************************************************************************************************
/**
 * Data modifiers that can be applied to either numerator or denominator.
 */
//**************************************************************************************************
auto Plot::Pad::Ratio::Numer() -> decltype(*this)
{
  mModMode = Mode::Num;
  return *this;
}
auto Plot::Pad::Ratio::Denom() -> decltype(*this)
{
  mModMode = Mode::Den;
  return *this;
}
//**************************************************************************************************
/**
 * Data selection (projections, profiles, scatter, define, filter, entries) always acts on the inputs.
 * In Result mode it is interpreted as Numer() with a warning, and the mode is switched accordingly.
 */
//**************************************************************************************************
template <typename Select>
auto Plot::Pad::Ratio::SelectInput(const char* call, Select select) -> decltype(*this)
{
  if (mModMode == Mode::Res) {
    logger::warning("Ratio {}: {}() selects input data, it cannot act on the ratio itself; interpreted as Numer().{}(). Write Numer(), Denom() or Both() explicitly.",
                    GetName(), call, call);
    mModMode = Mode::Num;
  }
  if (mModMode != Mode::Den) select();
  if (mModMode == Mode::Den || mModMode == Mode::Both) {
    // the selection of the denominator is made by Data on the swapped in denominator, as the modifiers (FORWARD_MODIFIER)
    SwapDenominator(true);
    auto swapBack = util::make_scope_guard([this]() { SwapDenominator(true); });
    Quietly(mModMode == Mode::Both, select);  // with Both() the arguments were checked for the numerator already
  }
  return *this;
}
void Plot::Pad::Ratio::SwapDenominator(bool selection)
{
  std::swap(mName, mDenomName);
  if (selection) {
    std::swap(mDataInfo, mDenomDataInfo);
    std::swap(mProjInfo, mDenomProjInfo);
  } else {
    std::swap(Modify(), mDenomModify);
  }
}
auto Plot::Pad::Ratio::Both() -> decltype(*this)
{
  mModMode = Mode::Both;
  return *this;
}
auto Plot::Pad::Ratio::Result() -> decltype(*this)
{
  mModMode = Mode::Res;
  return *this;
}
auto Plot::Pad::Ratio::Project(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("Project", [&]() { Data::Project(dims, ranges, isUserCoord); });
}
auto Plot::Pad::Ratio::ProjectX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("ProjectX", [&]() { Data::ProjectX(startY, endY, isUserCoord); });
}
auto Plot::Pad::Ratio::ProjectY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("ProjectY", [&]() { Data::ProjectY(startX, endX, isUserCoord); });
}
auto Plot::Pad::Ratio::Profile(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("Profile", [&]() { Data::Profile(dims, ranges, isUserCoord); });
}
auto Plot::Pad::Ratio::Profile(uint8_t averagedAxis, const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("Profile", [&]() { Data::Profile(averagedAxis, dims, ranges, isUserCoord); });
}
auto Plot::Pad::Ratio::ProfileX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("ProfileX", [&]() { Data::ProfileX(startY, endY, isUserCoord); });
}
auto Plot::Pad::Ratio::ProfileY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  return SelectInput("ProfileY", [&]() { Data::ProfileY(startX, endX, isUserCoord); });
}
auto Plot::Pad::Ratio::Project(std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Project(vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Ratio::Profile(std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Profile(vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Ratio::Profile(uint8_t averagedAxis, std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Profile(averagedAxis, vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Ratio::Project(std::initializer_list<data_dim_t> dataDims, optional<string> weight) -> decltype(*this)
{
  return Project(vector<data_dim_t>(dataDims), weight);
}
auto Plot::Pad::Ratio::Project(const vector<data_dim_t>& dataDims, optional<string> weight) -> decltype(*this)
{
  return SelectInput("Project", [&]() { Data::Project(dataDims, weight); });
}
auto Plot::Pad::Ratio::Project1D(data_dim_t x, optional<string> weight) -> decltype(*this)
{
  return SelectInput("Project1D", [&]() { Data::Project1D(x, weight); });
}
auto Plot::Pad::Ratio::Project2D(data_dim_t x, data_dim_t y, optional<string> weight) -> decltype(*this)
{
  return SelectInput("Project2D", [&]() { Data::Project2D(x, y, weight); });
}
auto Plot::Pad::Ratio::Scatter(const string& x, const string& y) -> decltype(*this)
{
  return SelectInput("Scatter", [&]() { Data::Scatter(x, y); });
}
auto Plot::Pad::Ratio::Scatter(const string& x, const string& y, const string& xErr, const string& yErr) -> decltype(*this)
{
  return SelectInput("Scatter", [&]() { Data::Scatter(x, y, xErr, yErr); });
}
auto Plot::Pad::Ratio::Scatter(const string& x, const string& y, const string& xErrLow, const string& xErrHigh, const string& yErrLow, const string& yErrHigh) -> decltype(*this)
{
  return SelectInput("Scatter", [&]() { Data::Scatter(x, y, xErrLow, xErrHigh, yErrLow, yErrHigh); });
}
auto Plot::Pad::Ratio::Profile(const vector<data_dim_t>& dataDims, const string& profile, optional<string> weight) -> decltype(*this)
{
  return SelectInput("Profile", [&]() { Data::Profile(dataDims, profile, weight); });
}
auto Plot::Pad::Ratio::Profile1D(data_dim_t x, const string& profile, optional<string> weight) -> decltype(*this)
{
  return SelectInput("Profile1D", [&]() { Data::Profile1D(x, profile, weight); });
}
auto Plot::Pad::Ratio::Profile2D(data_dim_t x, data_dim_t y, const string& profile, optional<string> weight) -> decltype(*this)
{
  return SelectInput("Profile2D", [&]() { Data::Profile2D(x, y, profile, weight); });
}
auto Plot::Pad::Ratio::Define(const std::string& key, const std::string& value) -> decltype(*this)
{
  return SelectInput("Define", [&]() { Data::Define(key, value); });
}
auto Plot::Pad::Ratio::Filter(const std::string& filter) -> decltype(*this)
{
  return SelectInput("Filter", [&]() { Data::Filter(filter); });
}
auto Plot::Pad::Ratio::Join(const join_tree_t& tree, const std::vector<std::string>& keys, const std::string& alias) -> decltype(*this)
{
  return SelectInput("Join", [&]() { Data::Join(tree, keys, alias); });
}
auto Plot::Pad::Ratio::JoinByIndex(const join_tree_t& tree, const std::string& indexColumn, const std::string& alias) -> decltype(*this)
{
  return SelectInput("JoinByIndex", [&]() { Data::JoinByIndex(tree, indexColumn, alias); });
}
auto Plot::Pad::Ratio::Entries(uint32_t nEntries) -> decltype(*this)
{
  return SelectInput("Entries", [&]() { Data::Entries(nEntries); });
}
auto Plot::Pad::Ratio::Entries(uint32_t entryMin, uint32_t entryMax) -> decltype(*this)
{
  return SelectInput("Entries", [&]() { Data::Entries(entryMin, entryMax); });
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Axis
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Constructor setting axis name (X, Y, Z, C).
 */
//**************************************************************************************************
Plot::Pad::Axis::Axis(const char axisName) : Axis()
{
  mName = axisName;
}

//**************************************************************************************************
/**
 * Constructor to define entry from file.
 */
//**************************************************************************************************
Plot::Pad::Axis::Axis(const ptree& axisTree) : Axis()
{
  try {
    const auto name = get_from_tree<char>(axisTree, "name");
    if (!name) logger::throw_invalid_argument("the axis has no name");
    mName = *name;
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct axis from ptree: {}", e.what());
  }

  read_from_tree(axisTree, mAxisFrame.title, "title");
  read_from_tree(axisTree, mRange.min, "range_min");
  read_from_tree(axisTree, mRange.max, "range_max");
  read_from_tree(axisTree, mRoom.belowMin, "room_below_min");
  read_from_tree(axisTree, mRoom.aboveMax, "room_above_max");
  read_from_tree(axisTree, mAxisFrame.style, "num_divisions");
  read_from_tree(axisTree, mAxisFrame.maxDigits, "max_digits");
  read_from_tree(axisTree, mAxisFrame.scale, "tick_length");
  read_from_tree(axisTree, mAxisFrame.color, "axis_color");
  read_from_tree(axisTree, mAxisFrame.alpha, "axis_alpha");
  ReadLayout(axisTree, mTitleLayout, "title_", "font", "size");
  read_from_tree(axisTree, mTitleLayout.offset, "title_offset");
  read_from_tree(axisTree, mTitleLayout.center, "title_center");
  ReadLayout(axisTree, mLabelLayout, "label_", "font", "size");
  read_from_tree(axisTree, mLabelLayout.offset, "label_offset");
  read_from_tree(axisTree, mLabelLayout.center, "label_center");
  read_from_tree(axisTree, mLabelLayout.angle, "label_angle");
  read_from_tree(axisTree, mAxisFrame.isLog, "is_log");
  read_from_tree(axisTree, mAxisFrame.isGrid, "is_grid");
  read_from_tree(axisTree, mAxisFrame.isOppositeTicks, "is_opposite_ticks");
  read_from_tree(axisTree, mAxisFrame.isNoExponent, "is_no_exponent");
  read_from_tree(axisTree, mAxisFrame.hideClippedLabels, "hide_clipped_labels");
  read_from_tree(axisTree, mAxisFrame.colorScaleOffset, "color_scale_offset");
  read_from_tree(axisTree, mAxisFrame.colorScaleWidth, "color_scale_width");
  read_from_tree(axisTree, mAxisFrame.tickOrientation, "tick_orientation");
  read_from_tree(axisTree, mAxisFrame.timeFormat, "time_format");
}

//**************************************************************************************************
/**
 * Convert axis object to property tree.
 */
//**************************************************************************************************
ptree Plot::Pad::Axis::GetPropertyTree() const
{
  ptree axisTree;
  axisTree.put("name", mName);
  put_in_tree(axisTree, mAxisFrame.title, "title");
  put_in_tree(axisTree, mRange.min, "range_min");
  put_in_tree(axisTree, mRange.max, "range_max");
  put_in_tree(axisTree, mRoom.belowMin, "room_below_min");
  put_in_tree(axisTree, mRoom.aboveMax, "room_above_max");
  put_in_tree(axisTree, mAxisFrame.style, "num_divisions");
  put_in_tree(axisTree, mAxisFrame.maxDigits, "max_digits");
  put_in_tree(axisTree, mAxisFrame.scale, "tick_length");
  put_in_tree(axisTree, mAxisFrame.color, "axis_color");
  put_in_tree(axisTree, mAxisFrame.alpha, "axis_alpha");
  PutLayout(axisTree, mTitleLayout, "title_", "font", "size");
  put_in_tree(axisTree, mTitleLayout.offset, "title_offset");
  put_in_tree(axisTree, mTitleLayout.center, "title_center");
  PutLayout(axisTree, mLabelLayout, "label_", "font", "size");
  put_in_tree(axisTree, mLabelLayout.offset, "label_offset");
  put_in_tree(axisTree, mLabelLayout.center, "label_center");
  put_in_tree(axisTree, mLabelLayout.angle, "label_angle");
  put_in_tree(axisTree, mAxisFrame.isLog, "is_log");
  put_in_tree(axisTree, mAxisFrame.isGrid, "is_grid");
  put_in_tree(axisTree, mAxisFrame.isOppositeTicks, "is_opposite_ticks");
  put_in_tree(axisTree, mAxisFrame.isNoExponent, "is_no_exponent");
  put_in_tree(axisTree, mAxisFrame.hideClippedLabels, "hide_clipped_labels");
  put_in_tree(axisTree, mAxisFrame.colorScaleOffset, "color_scale_offset");
  put_in_tree(axisTree, mAxisFrame.colorScaleWidth, "color_scale_width");
  put_in_tree(axisTree, mAxisFrame.tickOrientation, "tick_orientation");
  put_in_tree(axisTree, mAxisFrame.timeFormat, "time_format");

  return axisTree;
}

//**************************************************************************************************
/**
 * Apply all settings from axis on top of this axis.
 */
//**************************************************************************************************
void Plot::Pad::Axis::operator+=(const Axis& axis)
{
  mName = axis.mName;
  if (axis.mRange.min) mRange.min = axis.mRange.min;
  if (axis.mRange.max) mRange.max = axis.mRange.max;
  if (axis.mRoom.belowMin) mRoom.belowMin = axis.mRoom.belowMin;
  if (axis.mRoom.aboveMax) mRoom.aboveMax = axis.mRoom.aboveMax;
  if (axis.mAxisFrame.title) mAxisFrame.title = axis.mAxisFrame.title;
  if (axis.mAxisFrame.style) mAxisFrame.style = axis.mAxisFrame.style;
  if (axis.mAxisFrame.maxDigits) mAxisFrame.maxDigits = axis.mAxisFrame.maxDigits;
  if (axis.mAxisFrame.scale) mAxisFrame.scale = axis.mAxisFrame.scale;
  if (axis.mAxisFrame.color) mAxisFrame.color = axis.mAxisFrame.color;
  if (axis.mAxisFrame.alpha) mAxisFrame.alpha = axis.mAxisFrame.alpha;
  if (axis.mAxisFrame.isLog) mAxisFrame.isLog = axis.mAxisFrame.isLog;
  if (axis.mAxisFrame.isGrid) mAxisFrame.isGrid = axis.mAxisFrame.isGrid;
  if (axis.mAxisFrame.isOppositeTicks) mAxisFrame.isOppositeTicks = axis.mAxisFrame.isOppositeTicks;
  if (axis.mAxisFrame.isNoExponent) mAxisFrame.isNoExponent = axis.mAxisFrame.isNoExponent;
  if (axis.mAxisFrame.hideClippedLabels) mAxisFrame.hideClippedLabels = axis.mAxisFrame.hideClippedLabels;
  if (axis.mAxisFrame.colorScaleOffset) mAxisFrame.colorScaleOffset = axis.mAxisFrame.colorScaleOffset;
  if (axis.mAxisFrame.colorScaleWidth) mAxisFrame.colorScaleWidth = axis.mAxisFrame.colorScaleWidth;
  mTitleLayout.Update(axis.mTitleLayout);
  if (axis.mTitleLayout.offset) mTitleLayout.offset = axis.mTitleLayout.offset;
  if (axis.mTitleLayout.center) mTitleLayout.center = axis.mTitleLayout.center;
  mLabelLayout.Update(axis.mLabelLayout);
  if (axis.mLabelLayout.offset) mLabelLayout.offset = axis.mLabelLayout.offset;
  if (axis.mLabelLayout.center) mLabelLayout.center = axis.mLabelLayout.center;
  if (axis.mLabelLayout.angle) mLabelLayout.angle = axis.mLabelLayout.angle;
  if (axis.mAxisFrame.timeFormat) mAxisFrame.timeFormat = axis.mAxisFrame.timeFormat;
  if (axis.mAxisFrame.tickOrientation) mAxisFrame.tickOrientation = axis.mAxisFrame.tickOrientation;
}

//**************************************************************************************************
/**
 * User accessors to change axis layout.
 */
//**************************************************************************************************
auto Plot::Pad::Axis::SetTitle(const string& title) -> decltype(*this)
{
  mAxisFrame.title = title;
  return *this;
}
auto Plot::Pad::Axis::SetRange(double_t min, double_t max) -> decltype(*this)
{
  bool valid = CheckFinite(min, "range minimum");
  valid = CheckFinite(max, "range maximum") && valid;
  if (!valid) return *this;
  if (min >= max) {
    logger::warning("The range [{}, {}] of axis {} is ignored: its minimum is not below its maximum.", min, max, mName);
    return *this;
  }
  mRange = {min, max};
  return *this;
}
auto Plot::Pad::Axis::SetMaxRange(double_t max) -> decltype(*this)
{
  if (CheckFinite(max, "range maximum")) mRange.max = max;
  return *this;
}
auto Plot::Pad::Axis::SetMinRange(double_t min) -> decltype(*this)
{
  if (CheckFinite(min, "range minimum")) mRange.min = min;
  return *this;
}
auto Plot::Pad::Axis::SetRoomAboveMax(double_t frameFraction) -> decltype(*this)
{
  if (!(frameFraction >= 0. && frameFraction < 1.)) {
    logger::warning("The room above the maximum of axis {} ({}) is ignored: it is not a fraction of the frame in [0, 1).", mName, frameFraction);
    return *this;
  }
  mRoom.aboveMax = frameFraction;
  return *this;
}
auto Plot::Pad::Axis::SetRoomBelowMin(double_t frameFraction) -> decltype(*this)
{
  if (!(frameFraction >= 0. && frameFraction < 1.)) {
    logger::warning("The room below the minimum of axis {} ({}) is ignored: it is not a fraction of the frame in [0, 1).", mName, frameFraction);
    return *this;
  }
  mRoom.belowMin = frameFraction;
  return *this;
}
auto Plot::Pad::Axis::SetColor(color_t color) -> decltype(*this)
{
  mAxisFrame.color = color;
  mLabelLayout.color = color;
  mTitleLayout.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetAxisColor(color_t color) -> decltype(*this)
{
  mAxisFrame.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetAxisAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "axis alpha")) return *this;
  mAxisFrame.alpha = alpha;
  return *this;
}
auto Plot::Pad::Axis::SetNumDivisions(int16_t numDivisions) -> decltype(*this)
{
  mAxisFrame.style = numDivisions;
  return *this;
}
auto Plot::Pad::Axis::SetMaxDigits(int32_t maxDigits) -> decltype(*this)
{
  mAxisFrame.maxDigits = maxDigits;
  return *this;
}
auto Plot::Pad::Axis::SetTickLength(float_t tickLength) -> decltype(*this)
{
  if (CheckFinite(tickLength, "tick length")) mAxisFrame.scale = tickLength;
  return *this;
}
auto Plot::Pad::Axis::SetTitleFont(int16_t font) -> decltype(*this)
{
  if (CheckFont(font)) mTitleLayout.style = font;
  return *this;
}
auto Plot::Pad::Axis::SetLabelFont(int16_t font) -> decltype(*this)
{
  if (CheckFont(font)) mLabelLayout.style = font;
  return *this;
}
auto Plot::Pad::Axis::SetTitleSize(float_t size) -> decltype(*this)
{
  if (CheckNotNegative(size, "title size")) mTitleLayout.scale = size;
  return *this;
}
auto Plot::Pad::Axis::SetLabelSize(float_t size) -> decltype(*this)
{
  if (CheckNotNegative(size, "label size")) mLabelLayout.scale = size;
  return *this;
}
auto Plot::Pad::Axis::SetTitleColor(color_t color) -> decltype(*this)
{
  mTitleLayout.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetLabelColor(color_t color) -> decltype(*this)
{
  mLabelLayout.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetTitleAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "title alpha")) return *this;
  mTitleLayout.alpha = alpha;
  return *this;
}
auto Plot::Pad::Axis::SetLabelAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "label alpha")) return *this;
  mLabelLayout.alpha = alpha;
  return *this;
}
auto Plot::Pad::Axis::SetTitleOffset(float_t offset) -> decltype(*this)
{
  if (CheckFinite(offset, "title offset")) mTitleLayout.offset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetLabelOffset(float_t offset) -> decltype(*this)
{
  if (CheckFinite(offset, "label offset")) mLabelLayout.offset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetLabelAngle(float_t angle) -> decltype(*this)
{
  if (CheckFinite(angle, "label angle")) mLabelLayout.angle = angle;
  return *this;
}
auto Plot::Pad::Axis::SetTitleCenter(bool center) -> decltype(*this)
{
  mTitleLayout.center = center;
  return *this;
}
auto Plot::Pad::Axis::SetLabelCenter(bool center) -> decltype(*this)
{
  mLabelLayout.center = center;
  return *this;
}
auto Plot::Pad::Axis::SetLog(bool isLog) -> decltype(*this)
{
  mAxisFrame.isLog = isLog;
  return *this;
}
auto Plot::Pad::Axis::SetGrid(bool isGrid) -> decltype(*this)
{
  mAxisFrame.isGrid = isGrid;
  return *this;
}
auto Plot::Pad::Axis::SetOppositeTicks(bool isOppositeTicks) -> decltype(*this)
{
  mAxisFrame.isOppositeTicks = isOppositeTicks;
  return *this;
}
auto Plot::Pad::Axis::SetNoExponent(bool isNoExponent) -> decltype(*this)
{
  mAxisFrame.isNoExponent = isNoExponent;
  return *this;
}
auto Plot::Pad::Axis::SetHideClippedLabels(bool hide) -> decltype(*this)
{
  mAxisFrame.hideClippedLabels = hide;
  return *this;
}
auto Plot::Pad::Axis::SetColorScaleOffset(float_t offset) -> decltype(*this)
{
  if (CheckFinite(offset, "colour scale offset")) mAxisFrame.colorScaleOffset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetColorScaleWidth(float_t width) -> decltype(*this)
{
  if (CheckNotNegative(width, "colour scale width")) mAxisFrame.colorScaleWidth = width;
  return *this;
}
auto Plot::Pad::Axis::SetTimeFormat(const string& timeFormat) -> decltype(*this)
{
  mAxisFrame.timeFormat = timeFormat;
  return *this;
}
auto Plot::Pad::Axis::SetTickOrientation(const string& tickOrientation) -> decltype(*this)
{
  if (tickOrientation != "+" && tickOrientation != "-" && tickOrientation != "+-" && tickOrientation != "-+") {
    logger::warning("The tick orientation '{}' of axis {} is ignored: it is not + (positive side), - (negative side) or +- (both).", tickOrientation, mName);
    return *this;
  }
  mAxisFrame.tickOrientation = tickOrientation;
  return *this;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Box
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Constructor of Box
 */
//**************************************************************************************************
template <typename BoxType>
Plot::Pad::Box<BoxType>::Box(double_t xPos, double_t yPos) : Box()
{
  bool valid = CheckFinite(xPos, "box position x");
  valid = CheckFinite(yPos, "box position y") && valid;
  if (!valid) {
    SetAutoPlacement();
    return;
  }
  mPos.x = xPos;
  mPos.y = yPos;
}

//**************************************************************************************************
/**
 * Construct box from property tree.
 */
//**************************************************************************************************
template <typename BoxType>
Plot::Pad::Box<BoxType>::Box(const ptree& boxTree) : Box()
{
  read_from_tree(boxTree, mPos.x, "x");
  read_from_tree(boxTree, mPos.y, "y");
  read_from_tree(boxTree, mPos.w, "w");
  read_from_tree(boxTree, mPos.h, "h");
  ReadNamed(boxTree, mPos.coordinates, "coordinate_system");
  ReadNamed(boxTree, mPos.placement, "placement");
  this->ReadBorder(boxTree);
  this->ReadFill(boxTree);
  this->ReadText(boxTree);
  read_from_tree(boxTree, mMargin, "margin");
  read_from_tree(boxTree, mLineSpacing, "line_spacing");
}

//**************************************************************************************************
/**
 * Get property tree representation of box.
 */
//**************************************************************************************************
template <typename BoxType>
ptree Plot::Pad::Box<BoxType>::GetPropertyTree() const
{
  ptree boxTree;
  put_in_tree(boxTree, mPos.x, "x");
  put_in_tree(boxTree, mPos.y, "y");
  put_in_tree(boxTree, mPos.w, "w");
  put_in_tree(boxTree, mPos.h, "h");
  PutNamed(boxTree, mPos.coordinates, "coordinate_system");
  PutNamed(boxTree, mPos.placement, "placement");
  this->PutBorder(boxTree);
  this->PutFill(boxTree);
  this->PutText(boxTree);
  put_in_tree(boxTree, mMargin, "margin");
  put_in_tree(boxTree, mLineSpacing, "line_spacing");

  return boxTree;
}

//**************************************************************************************************
/**
 * User accessors to change box properties.
 */
//**************************************************************************************************
template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetPosition(double_t x, double_t y)
{
  bool valid = CheckFinite(x, "box position x");
  valid = CheckFinite(y, "box position y") && valid;
  if (!valid) return *GetThis();
  mPos.x = x;
  mPos.y = y;
  mPos.placement = nullopt;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetSize(double_t width, double_t height)
{
  bool valid = CheckNotNegative(width, "box width");
  valid = CheckNotNegative(height, "box height") && valid;
  if (!valid) return *GetThis();
  mPos.w = width;
  mPos.h = height;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetPadCoordinates()
{
  mPos.coordinates = pad_coordinates;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetFrameCoordinates()
{
  mPos.coordinates = frame_coordinates;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetUserCoordinates()
{
  mPos.coordinates = user_coordinates;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetAutoPlacement(box_placement_t placement)
{
  mPos.x = nullopt;
  mPos.y = nullopt;
  mPos.placement = placement;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTransparent()
{
  this->mFill.style = 0;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetNoBox()
{
  this->mFill.style = 0;
  this->mBorder.scale = 0.f;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetMargin(float_t margin)
{
  if (CheckNotNegative(margin, "box margin")) mMargin = margin;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetLineSpacing(float_t lineSpacing)
{
  if (CheckNotNegative(lineSpacing, "line spacing")) mLineSpacing = lineSpacing;
  return *GetThis();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class ImageBox
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * ImageBox constructors. A relative path is taken relative to the current directory, so that it can be stored with the project;
 * a path with environment variables is kept as it is (they are expanded when the plot is drawn).
 */
//**************************************************************************************************
Plot::Pad::ImageBox::ImageBox(double_t xPos, double_t yPos, const string& path) : ImageBox(path)
{
  SetPosition(xPos, yPos);
}
Plot::Pad::ImageBox::ImageBox(const string& path) : mPath(path), mPlacement(best_corner)
{
  const auto expandedPath = expand_path(path);
  if (expandedPath && std::filesystem::path(*expandedPath).is_relative()) mPath = std::filesystem::absolute(*expandedPath).lexically_normal().string();
}
Plot::Pad::ImageBox::ImageBox(const ptree& imageBoxTree)
{
  try {
    mPath = imageBoxTree.get<string>("path");
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct image box from ptree: {}", e.what());
  }
  read_from_tree(imageBoxTree, mX, "x");
  read_from_tree(imageBoxTree, mY, "y");
  read_from_tree(imageBoxTree, mWidth, "w");
  read_from_tree(imageBoxTree, mHeight, "h");
  ReadNamed(imageBoxTree, mCoordinates, "coordinate_system");
  ReadNamed(imageBoxTree, mPlacement, "placement");
}

//**************************************************************************************************
/**
 * Get property tree representation of ImageBox.
 */
//**************************************************************************************************
ptree Plot::Pad::ImageBox::GetPropertyTree() const
{
  ptree boxTree;
  boxTree.put("path", mPath);
  put_in_tree(boxTree, mX, "x");
  put_in_tree(boxTree, mY, "y");
  put_in_tree(boxTree, mWidth, "w");
  put_in_tree(boxTree, mHeight, "h");
  PutNamed(boxTree, mCoordinates, "coordinate_system");
  PutNamed(boxTree, mPlacement, "placement");
  return boxTree;
}

//**************************************************************************************************
/**
 * User accessors to change the position and size of the image.
 */
//**************************************************************************************************
auto Plot::Pad::ImageBox::SetPosition(double_t x, double_t y) -> decltype(*this)
{
  bool valid = CheckFinite(x, "image position x");
  valid = CheckFinite(y, "image position y") && valid;
  if (!valid) return *this;
  mX = x;
  mY = y;
  mPlacement = nullopt;
  return *this;
}
auto Plot::Pad::ImageBox::SetWidth(double_t width) -> decltype(*this)
{
  if (CheckNotNegative(width, "image width")) mWidth = width;
  return *this;
}
auto Plot::Pad::ImageBox::SetHeight(double_t height) -> decltype(*this)
{
  if (CheckNotNegative(height, "image height")) mHeight = height;
  return *this;
}
auto Plot::Pad::ImageBox::SetPadCoordinates() -> decltype(*this)
{
  mCoordinates = pad_coordinates;
  return *this;
}
auto Plot::Pad::ImageBox::SetFrameCoordinates() -> decltype(*this)
{
  mCoordinates = frame_coordinates;
  return *this;
}
auto Plot::Pad::ImageBox::SetUserCoordinates() -> decltype(*this)
{
  mCoordinates = user_coordinates;
  return *this;
}
auto Plot::Pad::ImageBox::SetAutoPlacement(box_placement_t placement) -> decltype(*this)
{
  mX = nullopt;
  mY = nullopt;
  mPlacement = placement;
  return *this;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class TextBox
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * TextBox constructor.
 */
//**************************************************************************************************
Plot::Pad::TextBox::TextBox(double_t xPos, double_t yPos, const string& text)
  : Box(xPos, yPos)
{
  SetText(text);
}
Plot::Pad::TextBox::TextBox(double_t xPos, double_t yPos, const vector<string>& lines)
  : Box(xPos, yPos)
{
  SetText(lines);
}

//**************************************************************************************************
/**
 * TextBox constructor with automatic placing.
 */
//**************************************************************************************************
Plot::Pad::TextBox::TextBox(const string& text) : Box()
{
  SetText(text);
  SetAutoPlacement();
}
Plot::Pad::TextBox::TextBox(const vector<string>& lines) : Box()
{
  SetText(lines);
  SetAutoPlacement();
}

//**************************************************************************************************
/**
 * TextBox constructor from property tree.
 */
//**************************************************************************************************
Plot::Pad::TextBox::TextBox(const ptree& textBoxTree) : Box(textBoxTree)
{
  try {
    SetText(textBoxTree.get<string>("text"));
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct text box from ptree: {}", e.what());
  }
  read_from_tree(textBoxTree, mTextAlign, "text_align");
}

//**************************************************************************************************
/**
 * Get property tree representation of TextBox.
 */
//**************************************************************************************************
ptree Plot::Pad::TextBox::GetPropertyTree() const
{
  ptree boxTree = Box::GetPropertyTree();
  boxTree.put("text", mText);
  put_in_tree(boxTree, mTextAlign, "text_align");
  return boxTree;
}

//**************************************************************************************************
/**
 * Set text displayed in TextBox.
 */
//**************************************************************************************************
auto Plot::Pad::TextBox::SetText(const string& text) -> decltype(*this)
{
  mText = text;
  return *this;
}
auto Plot::Pad::TextBox::SetText(const vector<string>& lines) -> decltype(*this)
{
  mText.clear();
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i) mText += "\n";
    mText += lines[i];
  }
  return *this;
}
auto Plot::Pad::TextBox::SetText(std::initializer_list<string> lines) -> decltype(*this)
{
  return SetText(vector<string>(lines));
}

//**************************************************************************************************
/**
 * The lines of the text.
 */
//**************************************************************************************************
vector<string> Plot::Pad::TextBox::GetLines() const
{
  vector<string> lines;
  size_t last{};
  for (size_t pos{}; (pos = mText.find('\n', last)) != string::npos; last = pos + 1)
    lines.push_back(mText.substr(last, pos - last));
  lines.push_back(mText.substr(last));
  return lines;
}
bool Plot::Pad::TextBox::RefersToOtherPads() const
{
  // the placeholder grammar of the painter: a format in brackets (which may contain < or >) before or after :n@p
  static const std::regex otherPadRegex("<[a-z]+(?:\\[[^\\]]*\\])?:\\d+@\\d+(?:\\[[^\\]]*\\])?>");
  return mText.find('@') != string::npos && std::regex_search(mText, otherPadRegex);
}
void Plot::Pad::TextBox::ShiftDataIDs(uint16_t offset)
{
  // the data of the pad this text is added to come first: <mean:1> becomes <mean:(1 + offset)>, references to other pads are kept
  if (offset == 0 || mText.find('<') == string::npos) return;
  static const std::regex refRegex("(<[a-z]+(?:\\[[^\\]]*\\])?:)(\\d{1,5})((?:\\[[^\\]]*\\])?)>");
  string shifted;
  auto last = mText.cbegin();
  for (auto match = std::sregex_iterator(mText.begin(), mText.end(), refRegex); match != std::sregex_iterator(); ++match) {
    shifted.append(last, (*match)[0].first);
    shifted += (*match)[1].str() + std::to_string(std::stoul((*match)[2].str()) + offset) + (*match)[3].str() + ">";
    last = (*match)[0].second;
  }
  shifted.append(last, mText.cend());
  mText = std::move(shifted);
}
void Plot::Pad::TextBox::ShiftDataIDsOfPads(const std::map<uint8_t, uint16_t>& offsets)
{
  // <mean:1@2> names the first data of pad 2: when plots are added, the data of pad 2 move up by the data pad 2 had before
  if (mText.find('@') == string::npos) return;
  static const std::regex refRegex("(<[a-z]+(?:\\[[^\\]]*\\])?:)(\\d{1,5})@(\\d{1,3})((?:\\[[^\\]]*\\])?)>");
  string shifted;
  auto last = mText.cbegin();
  for (auto match = std::sregex_iterator(mText.begin(), mText.end(), refRegex); match != std::sregex_iterator(); ++match) {
    const auto padID = std::stoul((*match)[3].str());
    const auto offset = (padID <= std::numeric_limits<uint8_t>::max() && offsets.count(static_cast<uint8_t>(padID))) ? offsets.at(static_cast<uint8_t>(padID)) : 0;
    shifted.append(last, (*match)[0].first);
    shifted += (*match)[1].str() + std::to_string(std::stoul((*match)[2].str()) + offset) + "@" + (*match)[3].str() + (*match)[4].str() + ">";
    last = (*match)[0].second;
  }
  shifted.append(last, mText.cend());
  mText = std::move(shifted);
}
auto Plot::Pad::TextBox::SetTextAlign(int16_t align) -> decltype(*this)
{
  if (align < 0 || align / 10 > 3 || align % 10 > 3) {
    logger::warning("The text alignment ({}) is ignored: its tens must be 1-3 (left, centre, right), its units 1-3 (bottom, middle, top).", align);
    return *this;
  }
  int16_t hAlign = (align / 10) * 10;
  int16_t vAlign = align % 10;
  if (!hAlign) hAlign = kHAlignLeft;
  if (!vAlign) vAlign = kVAlignCenter;
  mTextAlign = hAlign + vAlign;
  return *this;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class StatsBox
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * StatsBox constructor: placed automatically, as without settings.
 */
//**************************************************************************************************
Plot::Pad::StatsBox::StatsBox() : Box()
{
  SetAutoPlacement();
}

//**************************************************************************************************
/**
 * StatsBox constructor from property tree.
 */
//**************************************************************************************************
Plot::Pad::StatsBox::StatsBox(const ptree& statsBoxTree) : Box(statsBoxTree)
{
}

//**************************************************************************************************
/**
 * Get property tree representation of StatsBox.
 */
//**************************************************************************************************
ptree Plot::Pad::StatsBox::GetPropertyTree() const
{
  return Box::GetPropertyTree();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class LegendBox
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * LegendBox constructor.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendBox(double_t xPos, double_t yPos, const std::optional<std::string>& title)
  : Box(xPos, yPos), mTitle{title}, mNumColumns{}
{
}

//**************************************************************************************************
/**
 * LegendBox constructor with automatic placing.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendBox(const std::optional<std::string>& title) : Box(), mTitle{title}, mNumColumns{}
{
  SetAutoPlacement();
}

//**************************************************************************************************
/**
 * LegendBox constructor from property tree.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendBox(const ptree& legendBoxTree) : Box(legendBoxTree)
{
  read_from_tree(legendBoxTree, mTitle, "title");
  if (const auto numColumns = get_from_tree<uint8_t>(legendBoxTree, "num_columns")) {
    SetNumColumns(*numColumns);
  }
  read_from_tree(legendBoxTree, mColumnSpacing, "column_spacing");
  read_from_tree(legendBoxTree, mOrderTopDown, "order_top_down");
  read_from_tree(legendBoxTree, mSymbolElementsDefault, "default_symbol_elements");
  ReadLayout(legendBoxTree, mMarkerDefault, "default_marker_", "style", "size");
  ReadLayout(legendBoxTree, mLineDefault, "default_line_", "style", "width");
  ReadLayout(legendBoxTree, mFillDefault, "default_fill_", "style");
  read_from_tree(legendBoxTree, mSymbolColScale, "symbol_col_scale");

  for (const auto& content : legendBoxTree) {
    if (content.first == "ENTRY") mLegendEntriesUser.emplace_back(content.second);
  }
}

//**************************************************************************************************
/**
 * Get property tree of LegendBox.
 */
//**************************************************************************************************
ptree Plot::Pad::LegendBox::GetPropertyTree() const
{
  ptree legendBoxTree = Box::GetPropertyTree();
  put_in_tree(legendBoxTree, mTitle, "title");
  put_in_tree(legendBoxTree, mNumColumns, "num_columns");
  put_in_tree(legendBoxTree, mColumnSpacing, "column_spacing");
  put_in_tree(legendBoxTree, mOrderTopDown, "order_top_down");
  put_in_tree(legendBoxTree, mSymbolElementsDefault, "default_symbol_elements");
  PutLayout(legendBoxTree, mMarkerDefault, "default_marker_", "style", "size");
  PutLayout(legendBoxTree, mLineDefault, "default_line_", "style", "width");
  PutLayout(legendBoxTree, mFillDefault, "default_fill_", "style");
  put_in_tree(legendBoxTree, mSymbolColScale, "symbol_col_scale");

  for (const auto& legendEntry : mLegendEntriesUser) {
    legendBoxTree.add_child("ENTRY", legendEntry.GetPropertyTree());  // repeated key: the order of the rows is kept
  }

  return legendBoxTree;
}

//**************************************************************************************************
/**
 * User accessors for LegendBox properties.
 */
//**************************************************************************************************
Plot::Pad::LegendEntry& Plot::Pad::LegendBox::AddEntry(const string& label)
{
  return mLegendEntriesUser.emplace_back(LegendEntry(label));
}

Plot::Pad::LegendEntry& Plot::Pad::LegendBox::AddDataEntry(uint8_t padID, uint16_t dataID)
{
  LegendEntry& entry = mLegendEntriesUser.emplace_back(LegendEntry(nullopt, dataID));
  entry.mRefPadID = padID;
  return entry;
}

bool Plot::Pad::LegendBox::HasForeignRows() const
{
  return std::any_of(mLegendEntriesUser.begin(), mLegendEntriesUser.end(), [](const LegendEntry& entry) { return entry.mRefPadID.has_value(); });
}

auto Plot::Pad::LegendBox::SetTitle(const string& title) -> decltype(*this)
{
  mTitle = title;
  return *this;
}

auto Plot::Pad::LegendBox::SetNumColumns(uint8_t numColumns) -> decltype(*this)
{
  if (CheckCount(numColumns, "number of legend columns")) mNumColumns = numColumns;
  return *this;
}

auto Plot::Pad::LegendBox::SetColumnSpacing(float_t spacing) -> decltype(*this)
{
  if (CheckNotNegative(spacing, "legend column spacing")) mColumnSpacing = spacing;
  return *this;
}

auto Plot::Pad::LegendBox::SetOrderTopDown(bool topDown) -> decltype(*this)
{
  mOrderTopDown = topDown;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultSymbolElements(const string& symbolElements) -> decltype(*this)
{
  LegendEntry::CheckSymbolElements(symbolElements);
  mSymbolElementsDefault = symbolElements;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultColor(color_t color) -> decltype(*this)
{
  mLineDefault.color = color;
  mMarkerDefault.color = color;
  mFillDefault.color = color;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "legend default alpha")) return *this;
  mLineDefault.alpha = alpha;
  mMarkerDefault.alpha = alpha;
  mFillDefault.alpha = alpha;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultLineColor(color_t color) -> decltype(*this)
{
  mLineDefault.color = color;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultLineAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "legend default line alpha")) return *this;
  mLineDefault.alpha = alpha;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultLineStyle(int16_t style) -> decltype(*this)
{
  mLineDefault.style = style;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultLineWidth(float_t width) -> decltype(*this)
{
  if (CheckFinite(width, "legend default line width")) mLineDefault.scale = RoundWidth(width, "legend default line width");
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultMarkerColor(color_t color) -> decltype(*this)
{
  mMarkerDefault.color = color;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultMarkerAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "legend default marker alpha")) return *this;
  mMarkerDefault.alpha = alpha;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultMarkerStyle(int16_t style) -> decltype(*this)
{
  if (!CheckMarkerStyle(style, "legend default marker style")) return *this;
  mMarkerDefault.style = style;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultMarkerSize(float_t size) -> decltype(*this)
{
  if (CheckFinite(size, "marker size")) mMarkerDefault.scale = size;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultFillColor(color_t color) -> decltype(*this)
{
  mFillDefault.color = color;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultFillAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "legend default fill alpha")) return *this;
  mFillDefault.alpha = alpha;
  return *this;
}

auto Plot::Pad::LegendBox::SetDefaultFillStyle(int16_t style) -> decltype(*this)
{
  mFillDefault.style = style;
  return *this;
}

auto Plot::Pad::LegendBox::SetSymbolColScale(float_t scale) -> decltype(*this)
{
  if (CheckNotNegative(scale, "legend symbol column scale")) mSymbolColScale = scale;
  return *this;
}

//**************************************************************************************************
/**
 * Add the row of a data item (dataID: its number in the pad), with the appearance the user set for it.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::AddDataRow(const string& label, uint16_t dataID, const optional<LegendEntry>& appearance)
{
  LegendEntry& entry = mLegendEntries.emplace_back(LegendEntry(label, dataID));
  if (appearance) {
    entry += *appearance;
    entry.mPosition = appearance->mPosition;  // a place of its own among the rows (by default the place of the data)
    entry.mPositionDataID = appearance->mPositionDataID;
  }
}

//**************************************************************************************************
/**
 * Shift the data the rows refer to, when the pad is added to a pad whose data come first.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::ShiftDataIDs(uint16_t offset)
{
  for (auto& entry : mLegendEntriesUser) {
    if (!entry.mRefPadID) {  // rows for the data of another pad keep their numbers
      if (entry.mRefDataID) *entry.mRefDataID += offset;
      entry.ShiftSymbolOf(offset);
    }
    entry.ShiftPosition(offset);
  }
}
void Plot::Pad::LegendBox::ShiftDataIDsOfPads(const std::map<uint8_t, uint16_t>& offsets)
{
  for (auto& entry : mLegendEntriesUser) {
    if (!entry.mRefPadID) continue;
    const auto offset = offsets.find(*entry.mRefPadID);
    if (offset == offsets.end()) continue;
    if (entry.mRefDataID) *entry.mRefDataID += offset->second;
    entry.ShiftSymbolOf(offset->second);
  }
}

//**************************************************************************************************
/**
 * Put the rows added with AddEntry between the rows of the data (nData: number of data in the pad).
 * The rows of the data are in the order of the data unless placed with their legend entry; a row is at the top, before or after the place
 * of a data item (whether or not it has a row in this legend), a row added by the user by default at the end. Rows at the same place:
 * the rows of the data first (in the order of the data), then the rows added by the user in the order in which they were added.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::MergeLegendEntries(uint16_t nData)
{
  constexpr int32_t atEnd = std::numeric_limits<int32_t>::max();
  vector<std::pair<std::pair<int32_t, int32_t>, LegendEntry>> rows;  // (data the row is at, before/at/after it), row
  auto placeOf = [nData](const LegendEntry& entry, std::pair<int32_t, int32_t> unplaced) -> std::pair<int32_t, int32_t> {
    if (!entry.mPosition) return unplaced;
    if (entry.mPosition == LegendEntry::position_t::top) return {0, 0};
    if (entry.mPositionDataID < 1 || entry.mPositionDataID > nData) {
      logger::warning("Legend entry '{}' is placed relative to data {}, but the pad has {} data.", entry.mLabel.value_or(""), entry.mPositionDataID, nData);
    }
    return {entry.mPositionDataID, (entry.mPosition == LegendEntry::position_t::before) ? 0 : 2};
  };
  for (const auto& entry : mLegendEntries) {
    rows.push_back({placeOf(entry, {*entry.mRefDataID, 1}), entry});
  }
  for (const auto& entry : mLegendEntriesUser) {
    rows.push_back({placeOf(entry, {atEnd, 0}), entry});
  }
  std::stable_sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  mLegendEntries.clear();
  for (auto& [place, entry] : rows) {
    mLegendEntries.push_back(std::move(entry));
  }
}

//**************************************************************************************************
/**
 * Construct LegendEntry.
 */
//**************************************************************************************************
Plot::Pad::LegendEntry::LegendEntry(const optional<string>& label, const optional<uint16_t>& refDataID, const optional<string>& symbolElements)
{
  mLabel = label;
  mRefDataID = refDataID;
  mSymbolElements = symbolElements;
}

//**************************************************************************************************
/**
 * Construct LegendEntry from property tree.
 */
//**************************************************************************************************
Plot::Pad::LegendEntry::LegendEntry(const ptree& legendEntryTree)
{
  read_from_tree(legendEntryTree, mLabel, "label");
  read_from_tree(legendEntryTree, mRefDataID, "ref_data_id");
  read_from_tree(legendEntryTree, mRefPadID, "ref_pad_id");
  read_from_tree(legendEntryTree, mSymbolElements, "symbol_elements");
  if (const auto symbolOf = get_from_tree<string>(legendEntryTree, "symbol_of")) {
    std::istringstream stream(*symbolOf);
    for (string dataID; stream >> dataID;) {
      try {
        mSymbolOf.push_back(util::string_to_type<uint16_t>(dataID));
      } catch (const std::exception&) {
        logger::throw_invalid_argument("Bad value '{}' of symbol_of: data numbers separated by spaces", *symbolOf);
      }
    }
  }
  ReadNamed(legendEntryTree, mPosition, "position");
  if (mPosition && *mPosition != position_t::top) {
    // before or after the data numbered by position_data (from 1)
    const auto dataID = get_from_tree<uint16_t>(legendEntryTree, "position_data").value_or(0);
    if (dataID) {
      mPositionDataID = dataID;
    } else {
      mPosition = nullopt;
    }
  }
  ReadFill(legendEntryTree);
  ReadLine(legendEntryTree);
  ReadMarker(legendEntryTree);
  ReadText(legendEntryTree);
}

//**************************************************************************************************
/**
 * Get property tree of LegendEntry.
 */
//**************************************************************************************************
ptree Plot::Pad::LegendEntry::GetPropertyTree() const
{
  ptree legendEntryTree;
  put_in_tree(legendEntryTree, mLabel, "label");
  put_in_tree(legendEntryTree, mRefDataID, "ref_data_id");
  put_in_tree(legendEntryTree, mRefPadID, "ref_pad_id");
  put_in_tree(legendEntryTree, mSymbolElements, "symbol_elements");
  if (!mSymbolOf.empty()) {
    string symbolOf;
    for (uint16_t dataID : mSymbolOf)
      symbolOf += (symbolOf.empty() ? "" : " ") + std::to_string(dataID);
    legendEntryTree.put("symbol_of", symbolOf);
  }
  if (mPosition) {
    PutNamed(legendEntryTree, mPosition, "position");
    if (mPosition != position_t::top) legendEntryTree.put("position_data", mPositionDataID);
  }
  PutFill(legendEntryTree);
  PutLine(legendEntryTree);
  PutMarker(legendEntryTree);
  PutText(legendEntryTree);
  return legendEntryTree;
}

//**************************************************************************************************
/**
 * Apply user defined properties to existing LegendEntry
 */
//**************************************************************************************************
void Plot::Pad::LegendEntry::operator+=(
  const Plot::Pad::LegendEntry& legendEntry)
{
  if (legendEntry.mSymbolElements) this->mSymbolElements = legendEntry.mSymbolElements;
  if (!legendEntry.mSymbolOf.empty()) this->mSymbolOf = legendEntry.mSymbolOf;
  if (legendEntry.mLabel) this->mLabel = legendEntry.mLabel;
  if (legendEntry.mRefDataID) this->mRefDataID = legendEntry.mRefDataID;
  if (legendEntry.mRefPadID) this->mRefPadID = legendEntry.mRefPadID;
  mMarker.Update(legendEntry.mMarker);
  mLine.Update(legendEntry.mLine);
  mFill.Update(legendEntry.mFill);
  mText.Update(legendEntry.mText);
}

//**************************************************************************************************
/**
 * Shift the data the position refers to.
 */
//**************************************************************************************************
void Plot::Pad::LegendEntry::ShiftPosition(uint16_t offset)
{
  if (mPosition && mPosition != position_t::top) mPositionDataID += offset;
}
// the data whose symbols are added are in the pad of the row's data
void Plot::Pad::LegendEntry::ShiftSymbolOf(uint16_t offset)
{
  for (auto& dataID : mSymbolOf)
    dataID += offset;
}

//**************************************************************************************************
/**
 * User accessors to change legend entry properties.
 */
//**************************************************************************************************
auto Plot::Pad::LegendEntry::SetLabel(const string& label) -> decltype(*this)
{
  mLabel = label;
  return *this;
}

auto Plot::Pad::LegendEntry::SetRefData(uint16_t refDataID) -> decltype(*this)
{
  if (refDataID == 0) {
    logger::warning("SetRefData(0) is ignored: data are numbered from 1.");
    return *this;
  }
  mRefDataID = refDataID;
  return *this;
}
auto Plot::Pad::LegendEntry::AddSymbolOf(uint16_t dataID) -> decltype(*this)
{
  if (dataID == 0) {
    logger::warning("AddSymbolOf(0) is ignored: data are numbered from 1.");
    return *this;
  }
  mSymbolOf.push_back(dataID);
  return *this;
}
auto Plot::Pad::LegendEntry::AddSymbolOf(const vector<uint16_t>& dataIDs) -> decltype(*this)
{
  for (uint16_t dataID : dataIDs)
    AddSymbolOf(dataID);
  return *this;
}

// the letters of the symbol of a legend row (TLegendEntry): L line, P marker, F fill, E error bars
void Plot::Pad::LegendEntry::CheckSymbolElements(const string& symbolElements)
{
  if (symbolElements.find_first_not_of("LPFElpfe") != string::npos) {
    logger::warning("Legend symbol '{}' contains letters other than L (line), P (marker), F (fill) and E (error bars).", symbolElements);
  }
}
auto Plot::Pad::LegendEntry::SetSymbolElements(const string& symbolElements) -> decltype(*this)
{
  CheckSymbolElements(symbolElements);
  mSymbolElements = symbolElements;
  return *this;
}

auto Plot::Pad::LegendEntry::AtTop() -> decltype(*this)
{
  mPosition = position_t::top;
  mPositionDataID = 0;
  return *this;
}

auto Plot::Pad::LegendEntry::BeforeData(uint16_t dataID) -> decltype(*this)
{
  if (dataID == 0) {
    logger::warning("BeforeData(0) is ignored: data are numbered from 1.");
    return *this;
  }
  mPosition = position_t::before;
  mPositionDataID = dataID;
  return *this;
}

auto Plot::Pad::LegendEntry::AfterData(uint16_t dataID) -> decltype(*this)
{
  if (dataID == 0) {
    logger::warning("AfterData(0) is ignored: data are numbered from 1.");
    return *this;
  }
  mPosition = position_t::after;
  mPositionDataID = dataID;
  return *this;
}

auto Plot::Pad::LegendEntry::SetColor(color_t color) -> decltype(*this)
{
  mMarker.color = color;
  mLine.color = color;
  mFill.color = color;
  return *this;
}

auto Plot::Pad::LegendEntry::SetAlpha(float_t alpha) -> decltype(*this)
{
  if (!Plot::CheckAlpha(alpha, "legend entry alpha")) return *this;
  mMarker.alpha = alpha;
  mLine.alpha = alpha;
  mFill.alpha = alpha;
  return *this;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Shape
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

Plot::Pad::Shape::Shape(kind_t kind, const vector<double_t>& coordinates) : mKind(kind), mCoordinates(coordinates)
{
  const bool finite = std::all_of(coordinates.begin(), coordinates.end(), [](double_t value) { return std::isfinite(value); });
  if (!finite) {
    logger::warning("A {} with a position that is not a finite number will not be drawn.", enum_name(kind));
  }
}

string Plot::Pad::Shape::KindLabel(kind_t kind)
{
  if (kind == kind_t::region_x) return "region X";
  if (kind == kind_t::region_y) return "region Y";
  string label = enum_name(kind);
  std::replace(label.begin(), label.end(), '_', ' ');
  return label;
}

uint32_t Plot::Pad::Shape::EncodeDrawnID(const drawn_id_t& id)
{
  // bit 24 marks a shape (other objects keep 0); kind in bits 16-23, coordinate system in bits 14-15, number in bits 0-13
  return (1u << 24) | (static_cast<uint32_t>(id.kind) << 16) | (static_cast<uint32_t>(id.coordinates) << 14) | (id.number & 0x3FFFu);
}

optional<Plot::Pad::Shape::drawn_id_t> Plot::Pad::Shape::DecodeDrawnID(uint32_t uniqueID)
{
  if ((uniqueID >> 24) != 1u || ((uniqueID >> 16) & 0xFFu) > static_cast<uint32_t>(kind_t::polygon) || ((uniqueID >> 14) & 3u) > static_cast<uint32_t>(user_coordinates)) return nullopt;
  return drawn_id_t{static_cast<kind_t>((uniqueID >> 16) & 0xFFu), static_cast<uint16_t>(uniqueID & 0x3FFFu), static_cast<coordinates_t>((uniqueID >> 14) & 3u)};
}

Plot::Pad::Shape::Shape(const ptree& shapeTree)
{
  if (auto kind = enum_from_name<kind_t>(shapeTree.get<string>("kind", ""))) mKind = *kind;
  optional<vector<double_t>> coordinates;
  read_from_tree(shapeTree, coordinates, "coordinates");
  if (coordinates) mCoordinates = *coordinates;
  ReadNamed(shapeTree, mCoordinateSystem, "coordinate_system");
  ReadNamed(shapeTree, mLayer, "layer");
  read_from_tree(shapeTree, mArrowHead, "arrow_head");
  read_from_tree(shapeTree, mArrowSize, "arrow_size");
  optional<double_t> phiMin;
  optional<double_t> phiMax;
  read_from_tree(shapeTree, phiMin, "arc_min");
  read_from_tree(shapeTree, phiMax, "arc_max");
  if (phiMin && phiMax) mArc = {*phiMin, *phiMax};
  read_from_tree(shapeTree, mRotation, "rotation");
  ReadLine(shapeTree);
  ReadFill(shapeTree);
}

ptree Plot::Pad::Shape::GetPropertyTree() const
{
  ptree shapeTree;
  shapeTree.put("kind", enum_name(mKind));
  put_in_tree(shapeTree, optional<vector<double_t>>(mCoordinates), "coordinates");
  PutNamed(shapeTree, mCoordinateSystem, "coordinate_system");
  PutNamed(shapeTree, mLayer, "layer");
  put_in_tree(shapeTree, mArrowHead, "arrow_head");
  put_in_tree(shapeTree, mArrowSize, "arrow_size");
  if (mArc) {
    shapeTree.put("arc_min", mArc->first);
    shapeTree.put("arc_max", mArc->second);
  }
  put_in_tree(shapeTree, mRotation, "rotation");
  PutLine(shapeTree);
  PutFill(shapeTree);
  return shapeTree;
}

auto Plot::Pad::Shape::SetPadCoordinates() -> decltype(*this)
{
  mCoordinateSystem = pad_coordinates;
  return *this;
}
auto Plot::Pad::Shape::SetFrameCoordinates() -> decltype(*this)
{
  mCoordinateSystem = frame_coordinates;
  return *this;
}
auto Plot::Pad::Shape::SetUserCoordinates() -> decltype(*this)
{
  mCoordinateSystem = user_coordinates;
  return *this;
}
auto Plot::Pad::Shape::SetBehindData() -> decltype(*this)
{
  mLayer = layer_t::behind_data;
  return *this;
}
auto Plot::Pad::Shape::SetOnTop() -> decltype(*this)
{
  mLayer = layer_t::on_top;
  return *this;
}
auto Plot::Pad::Shape::SetArrowHead(const string& style) -> decltype(*this)
{
  if (mKind != kind_t::arrow) logger::warning("SetArrowHead has an effect only on arrows (Pad::AddArrow), not on a {}.", enum_name(mKind));
  mArrowHead = style;
  return *this;
}
auto Plot::Pad::Shape::SetArrowSize(float_t size) -> decltype(*this)
{
  if (mKind != kind_t::arrow) logger::warning("SetArrowSize has an effect only on arrows (Pad::AddArrow), not on a {}.", enum_name(mKind));
  if (CheckNotNegative(size, "arrow size")) mArrowSize = size;
  return *this;
}
auto Plot::Pad::Shape::SetArc(double_t phiMin, double_t phiMax) -> decltype(*this)
{
  if (mKind != kind_t::ellipse) logger::warning("SetArc has an effect only on ellipses (Pad::AddEllipse), not on a {}.", enum_name(mKind));
  bool valid = CheckFinite(phiMin, "start angle of the arc");
  valid = CheckFinite(phiMax, "end angle of the arc") && valid;
  if (valid) mArc = {phiMin, phiMax};
  return *this;
}
auto Plot::Pad::Shape::SetRotation(double_t theta) -> decltype(*this)
{
  if (mKind != kind_t::ellipse) logger::warning("SetRotation has an effect only on ellipses (Pad::AddEllipse), not on a {}.", enum_name(mKind));
  if (CheckFinite(theta, "rotation angle")) mRotation = theta;
  return *this;
}

//**************************************************************************************************
/**
 * Explicitly instantiate required box and setter-group templates.
 */
//**************************************************************************************************
template class Plot::Pad::Box<Plot::Pad::TextBox>;
template class Plot::Pad::Box<Plot::Pad::StatsBox>;
template class Plot::Pad::Box<Plot::Pad::LegendBox>;
template class MarkerSetters<Plot::Pad::Data>;
template class MarkerSetters<Plot::Pad::LegendEntry>;
template class LineSetters<Plot::Pad::Data>;
template class LineSetters<Plot::Pad::LegendEntry>;
template class LineSetters<Plot::Pad::Shape>;
template class FillSetters<Plot>;
template class FillSetters<Plot::Pad>;
template class FillSetters<Plot::Pad::Data>;
template class FillSetters<Plot::Pad::LegendEntry>;
template class FillSetters<Plot::Pad::TextBox>;
template class FillSetters<Plot::Pad::StatsBox>;
template class FillSetters<Plot::Pad::LegendBox>;
template class FillSetters<Plot::Pad::Shape>;
template class TextSetters<Plot::Pad::LegendEntry>;
template class TextSetters<Plot::Pad::TextBox>;
template class TextSetters<Plot::Pad::StatsBox>;
template class TextSetters<Plot::Pad::LegendBox>;
template class BorderSetters<Plot::Pad::TextBox>;
template class BorderSetters<Plot::Pad::StatsBox>;
template class BorderSetters<Plot::Pad::LegendBox>;

}  // end namespace SciRooPlot
