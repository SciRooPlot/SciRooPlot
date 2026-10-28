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
#include <initializer_list>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Validation.h"
#include "util/Optional.h"
#include "util/PropertyTree.h"
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
using util::get_from_tree;
using util::number_to_string;
using util::put_in_tree;
using util::read_from_tree;
using util::set_if;
using util::str_contains;

namespace
{
string join_strings(const vector<string>& items, const string& separator)
{
  string joined;
  for (size_t i = 0; i < items.size(); ++i) {
    joined += ((i) ? separator : "") + items[i];
  }
  return joined;
}
}  // namespace

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
  if (mIndex) return *mIndex;
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
 * Drawing option aliases are stored by name.
 */
//**************************************************************************************************
void Plot::ReadDrawOption(const ptree& tree, optional<draw_option_t>& option, const char* key)
{
  if (auto name = tree.get_optional<string>(key)) {
    option = draw_option_from_name(*name);
    if (!option) logger::warning("Unknown drawing option {} is ignored.", *name);
  }
}
void Plot::PutDrawOption(ptree& tree, const optional<draw_option_t>& option, const char* key)
{
  if (option) tree.put(key, draw_option_name(*option));
}

//**************************************************************************************************
/**
 * Default constructor.
 */
//**************************************************************************************************
Plot::Plot(const string& name, const string& group, const optional<string>& basePlot) : Plot()
{
  if (name.empty()) {
    logger::throw_invalid_argument("Plot name must not be empty.");
  }
  if (auto illegal = find_illegal_name_char(name)) {
    logger::throw_invalid_argument("Plot name '{}' contains illegal character '{}'.", name, *illegal);
  }
  if (auto illegal = find_illegal_name_char(group, true)) {
    logger::throw_invalid_argument("Group '{}' contains illegal character '{}'.", group, *illegal);
  }
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
Plot::Plot(const Plot& otherPlot, const string& name, const optional<string>& group)
  : Plot(otherPlot)
{
  if (name.empty()) {
    logger::throw_invalid_argument("Plot name must not be empty.");
  }
  if (auto illegal = find_illegal_name_char(name)) {
    logger::throw_invalid_argument("Plot name '{}' contains illegal character '{}'.", name, *illegal);
  }
  if (group) {
    if (auto illegal = find_illegal_name_char(*group, true)) {
      logger::throw_invalid_argument("Group '{}' contains illegal character '{}'.", *group, *illegal);
    }
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
  if (mName.empty()) {
    logger::throw_invalid_argument("Plot name must not be empty.");
  }
  if (auto illegal = find_illegal_name_char(mName)) {
    logger::throw_invalid_argument("Plot name '{}' contains illegal character '{}'.", mName, *illegal);
  }
  if (auto illegal = find_illegal_name_char(mGroup, true)) {
    logger::throw_invalid_argument("Group '{}' contains illegal character '{}'.", mGroup, *illegal);
  }
  mGroup = NormalizeGroup(mGroup);
  read_from_tree(plotTree, mBasePlot, "base_plot");
  read_from_tree(plotTree, mPlotDimensions.width, "width");
  read_from_tree(plotTree, mPlotDimensions.height, "height");
  read_from_tree(plotTree, mPlotDimensions.fixAspectRatio, "fix_aspect_ratio");
  read_from_tree(plotTree, mFill.color, "fill_color");
  read_from_tree(plotTree, mFill.style, "fill_style");
  read_from_tree(plotTree, mFill.alpha, "fill_alpha");

  // loop over all pads defined in property tree
  for (const auto& pad : plotTree) {
    if (str_contains(pad.first, "PAD")) {
      uint8_t padID = std::stoi(pad.first.substr(pad.first.find("_") + 1));
      mPads[padID] = Pad(pad.second);
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
  put_in_tree(plotTree, mFill.color, "fill_color");
  put_in_tree(plotTree, mFill.style, "fill_style");
  put_in_tree(plotTree, mFill.alpha, "fill_alpha");

  for (const auto& [padID, pad] : mPads) {
    plotTree.put_child("PAD_" + std::to_string(padID), pad.GetPropertyTree());
  }
  return plotTree;
}

//**************************************************************************************************
/**
 * Count number of inputs needed to create the plot.
 */
//**************************************************************************************************
uint8_t Plot::GetDataCount() const
{
  uint8_t count{};
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

auto Plot::SetName(const string& name) -> decltype(*this)
{
  if (name.empty()) {
    logger::error("Cannot set empty name.");
    return *this;
  }
  if (auto illegal = find_illegal_name_char(name, false)) {
    logger::error("Plot name '{}' contains illegal character '{}'.", name, *illegal);
    return *this;
  }
  mName = name;
  UpdateUniqueName();
  return *this;
}

auto Plot::SetGroup(const string& group) -> decltype(*this)
{
  if (auto illegal = find_illegal_name_char(group, true)) {
    logger::error("Group '{}' contains illegal character '{}'.", group, *illegal);
    return *this;
  }
  const string normalized = NormalizeGroup(group);
  if (normalized.empty()) {
    logger::error("Cannot set empty group.");
    return *this;
  }
  mGroup = normalized;
  UpdateUniqueName();
  return *this;
}

auto Plot::AppendGroup(const string& subgroup) -> decltype(*this)
{
  if (subgroup.empty()) {
    logger::error("Cannot append empty subgroup.");
    return *this;
  }
  if (auto illegal = find_illegal_name_char(subgroup, true)) {
    logger::error("Subgroup '{}' contains illegal character '{}'.", subgroup, *illegal);
    return *this;
  }
  const string normalizedSubgroup = NormalizeGroup(subgroup);
  if (normalizedSubgroup.empty()) {
    logger::error("Cannot append empty subgroup.");
    return *this;
  }
  mGroup = mGroup.empty() ? normalizedSubgroup : mGroup + "/" + normalizedSubgroup;
  UpdateUniqueName();
  return *this;
}

auto Plot::SetBasePlot(const string& name) -> decltype(*this)
{
  mBasePlot = name;
  return *this;
}

auto Plot::SetDimensions(int32_t width, int32_t height, bool fixAspectRatio) -> decltype(*this)
{
  mPlotDimensions = {width, height, fixAspectRatio};
  return *this;
}

auto Plot::SetWidth(int32_t width) -> decltype(*this)
{
  mPlotDimensions.width = width;
  return *this;
}

auto Plot::SetHeight(int32_t height) -> decltype(*this)
{
  mPlotDimensions.height = height;
  return *this;
}

auto Plot::SetFixAspectRatio(bool fixAspectRatio) -> decltype(*this)
{
  mPlotDimensions.fixAspectRatio = fixAspectRatio;
  return *this;
}

//**************************************************************************************************
/**
 * Set fill for this plot.
 */
//**************************************************************************************************
auto Plot::SetFill(color_t color, optional<int16_t> style, optional<float_t> alpha) -> decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  mFill.alpha = alpha;
  return *this;
}

auto Plot::SetFillColor(color_t color) -> decltype(*this)
{
  mFill.color = color;
  return *this;
}

auto Plot::SetFillStyle(int16_t style) -> decltype(*this)
{
  mFill.style = style;
  return *this;
}

auto Plot::SetFillAlpha(float_t alpha) -> decltype(*this)
{
  mFill.alpha = alpha;
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
  mFill.color = 0;
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

  if (plot.mFill.color) mFill.color = plot.mFill.color;
  if (plot.mFill.style) mFill.style = plot.mFill.style;
  if (plot.mFill.alpha) mFill.alpha = plot.mFill.alpha;

  for (const auto& [padID, pad] : plot.mPads) {
    mPads[padID];  // initializes the pad in case it was not yet defined in this plot
    mPads[padID] += pad;
  }
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
  mText.scale = size;
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
  mText.style = font;
  return *this;
}

//**************************************************************************************************
/**
 * Set default marker size for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultMarkerSize(float_t size) -> decltype(*this)
{
  mMarkerDefaults.scale = size;
  return *this;
}

//**************************************************************************************************
/**
 * Set default line width for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultLineWidth(float_t width) -> decltype(*this)
{
  mLineDefaults.scale = RoundWidth(width, "default line width");
  return *this;
}

//**************************************************************************************************
/**
 * Set default fill alpha for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultFillAlpha(float_t alpha) -> decltype(*this)
{
  mFillDefaults.alpha = alpha;
  return *this;
}
auto Plot::Pad::SetDefaultMarkerAlpha(float_t alpha) -> decltype(*this)
{
  mMarkerDefaults.alpha = alpha;
  return *this;
}
auto Plot::Pad::SetDefaultLineAlpha(float_t alpha) -> decltype(*this)
{
  mLineDefaults.alpha = alpha;
  return *this;
}
auto Plot::Pad::SetDefaultAlpha(float_t alpha) -> decltype(*this)
{
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
 * Set default marker colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultMarkerColors(const vector<color_t>& colors) -> decltype(*this)
{
  mMarkerDefaults.colorGradient = {};
  mMarkerDefaults.colors = colors;
  return *this;
}

auto Plot::Pad::SetDefaultMarkerColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  mMarkerDefaults.colors = nullopt;
  mMarkerDefaults.alpha = nullopt;
  mMarkerDefaults.colorGradient = {rgbEndpoints, alpha, nColors};
  return *this;
}
auto Plot::Pad::SetDefaultMarkerColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultMarkerColors(GradientEndpoints(gradient), alpha, nColors);
}

//**************************************************************************************************
/**
 * Set default line colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultLineColors(const vector<color_t>& colors) -> decltype(*this)
{
  mLineDefaults.colorGradient = {};
  mLineDefaults.colors = colors;
  return *this;
}

auto Plot::Pad::SetDefaultLineColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  mLineDefaults.colors = nullopt;
  mLineDefaults.alpha = nullopt;
  mLineDefaults.colorGradient = {rgbEndpoints, alpha, nColors};
  return *this;
}
auto Plot::Pad::SetDefaultLineColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultLineColors(GradientEndpoints(gradient), alpha, nColors);
}

//**************************************************************************************************
/**
 * Set default fill colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultFillColors(const vector<color_t>& colors) -> decltype(*this)
{
  mFillDefaults.colorGradient = {};
  mFillDefaults.colors = colors;
  return *this;
}

auto Plot::Pad::SetDefaultFillColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  mFillDefaults.colors = nullopt;
  mFillDefaults.alpha = nullopt;
  mFillDefaults.colorGradient = {rgbEndpoints, alpha, nColors};
  return *this;
}
auto Plot::Pad::SetDefaultFillColors(const vector<pair<color_t, float_t>>& gradient, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  return SetDefaultFillColors(GradientEndpoints(gradient), alpha, nColors);
}

//**************************************************************************************************
/**
 * Set default colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultColors(const vector<color_t>& colors) -> decltype(*this)
{
  mMarkerDefaults.colorGradient = {};
  mMarkerDefaults.colors = colors;
  mLineDefaults.colorGradient = {};
  mLineDefaults.colors = colors;
  mFillDefaults.colorGradient = {};
  mFillDefaults.colors = colors;
  return *this;
}

auto Plot::Pad::SetDefaultColors(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
  mMarkerDefaults.colors = nullopt;
  mMarkerDefaults.alpha = nullopt;
  mMarkerDefaults.colorGradient = {rgbEndpoints, alpha, nColors};
  mLineDefaults.colors = nullopt;
  mLineDefaults.alpha = nullopt;
  mLineDefaults.colorGradient = {rgbEndpoints, alpha, nColors};
  mFillDefaults.colors = nullopt;
  mFillDefaults.alpha = nullopt;
  mFillDefaults.colorGradient = {rgbEndpoints, alpha, nColors};
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
auto Plot::Pad::SetDefaultCandleBoxRange(float_t candleOption) -> decltype(*this)
{
  mCandleOptionDefaults.boxRange = candleOption;
  return *this;
}

//**************************************************************************************************
/**
 * Set default whisker range option for 2d candle plot.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultCandleWhiskerRange(float_t candleOption) -> decltype(*this)
{
  mCandleOptionDefaults.whiskerRange = candleOption;
  return *this;
}

//**************************************************************************************************
/**
 * Set fill for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetFill(color_t color, optional<int16_t> style, optional<float_t> alpha) -> decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  mFill.alpha = alpha;
  return *this;
}

auto Plot::Pad::SetFillColor(color_t color) -> decltype(*this)
{
  mFill.color = color;
  return *this;
}

auto Plot::Pad::SetFillStyle(int16_t style) -> decltype(*this)
{
  mFill.style = style;
  return *this;
}

auto Plot::Pad::SetFillAlpha(float_t alpha) -> decltype(*this)
{
  mFill.alpha = alpha;
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
  mFill.color = 0;
  return *this;
}

//**************************************************************************************************
/**
 * Set frame fill properties.
 */
//**************************************************************************************************
auto Plot::Pad::SetFrameFill(color_t color, optional<int16_t> style, optional<float_t> alpha) -> decltype(*this)
{
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
  mFrameFill.alpha = alpha;
  return *this;
}

//**************************************************************************************************
/**
 * Set frame line properties.
 */
//**************************************************************************************************
auto Plot::Pad::SetFrameBorder(color_t color, optional<int16_t> style, optional<float_t> width, optional<float_t> alpha) -> decltype(*this)
{
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
  mFrameBorder.scale = RoundWidth(width, "frame border width");
  return *this;
}

//**************************************************************************************************
/**
 * Set this pad transparent.
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
 * Set color palette for 2D plots.
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
auto Plot::Pad::SetPalette(int32_t palette) -> decltype(*this)
{
  mPalette.index = palette;
  mPalette.colorGradient = {};
  return *this;
}
auto Plot::Pad::SetPalette(const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, optional<float_t> alpha, optional<int32_t> nColors) -> decltype(*this)
{
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
  mMargins.top = top;
  mMargins.bottom = bottom;
  mMargins.left = left;
  mMargins.right = right;
  return *this;
}
auto Plot::Pad::SetTopMargin(float_t top) -> decltype(*this)
{
  mMargins.top = top;
  return *this;
}
auto Plot::Pad::SetBottomMargin(float_t bottom) -> decltype(*this)
{
  mMargins.bottom = bottom;
  return *this;
}
auto Plot::Pad::SetLeftMargin(float_t left) -> decltype(*this)
{
  mMargins.left = left;
  return *this;
}
auto Plot::Pad::SetRightMargin(float_t right) -> decltype(*this)
{
  mMargins.right = right;
  return *this;
}

//**************************************************************************************************
/**
 * Set viewing perspective on pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetView(double_t theta, double_t phi) -> decltype(*this)
{
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
  : mPosition(other.mPosition), mMargins(other.mMargins), mView(other.mView),
    mFill(other.mFill), mFrameFill(other.mFrameFill), mFrameBorder(other.mFrameBorder), mText(other.mText),
    mMarkerDefaults(other.mMarkerDefaults), mLineDefaults(other.mLineDefaults), mFillDefaults(other.mFillDefaults),
    mCandleOptionDefaults(other.mCandleOptionDefaults),
    mPalette(other.mPalette), mRedrawAxes(other.mRedrawAxes), mAxes(other.mAxes)
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
  read_from_tree(padTree, mPalette.colorGradient.rgbEndpoints, "palette_colors_gradient_endpoints");
  read_from_tree(padTree, mPalette.colorGradient.alpha, "palette_colors_gradient_alpha");
  read_from_tree(padTree, mPalette.colorGradient.nColors, "palette_colors_gradient_nColors");
  read_from_tree(padTree, mFill.color, "fill_color");
  read_from_tree(padTree, mFill.style, "fill_style");
  read_from_tree(padTree, mFill.alpha, "fill_alpha");
  read_from_tree(padTree, mFrameFill.color, "frame_fill_color");
  read_from_tree(padTree, mFrameFill.style, "frame_fill_style");
  read_from_tree(padTree, mFrameFill.alpha, "frame_fill_alpha");
  read_from_tree(padTree, mFrameBorder.color, "frame_border_color");
  read_from_tree(padTree, mFrameBorder.alpha, "frame_border_alpha");
  read_from_tree(padTree, mFrameBorder.style, "frame_border_style");
  ReadWidthFromTree(padTree, mFrameBorder.scale, "frame_border_width");
  read_from_tree(padTree, mText.style, "text_font");
  read_from_tree(padTree, mText.color, "text_color");
  read_from_tree(padTree, mText.alpha, "text_alpha");
  read_from_tree(padTree, mText.scale, "text_size");
  read_from_tree(padTree, mMarkerDefaults.alpha, "default_marker_alpha");
  read_from_tree(padTree, mMarkerDefaults.scale, "default_marker_size");
  read_from_tree(padTree, mMarkerDefaults.styles, "default_marker_styles");
  read_from_tree(padTree, mMarkerDefaults.colors, "default_marker_colors");
  read_from_tree(padTree, mMarkerDefaults.colorGradient.rgbEndpoints, "default_marker_colors_gradient_endpoints");
  read_from_tree(padTree, mMarkerDefaults.colorGradient.alpha, "default_marker_colors_gradient_alpha");
  read_from_tree(padTree, mMarkerDefaults.colorGradient.nColors, "default_marker_colors_gradient_nColors");
  read_from_tree(padTree, mLineDefaults.alpha, "default_line_alpha");
  ReadWidthFromTree(padTree, mLineDefaults.scale, "default_line_width");
  read_from_tree(padTree, mLineDefaults.styles, "default_line_styles");
  read_from_tree(padTree, mLineDefaults.colors, "default_line_colors");
  read_from_tree(padTree, mLineDefaults.colorGradient.rgbEndpoints, "default_line_colors_gradient_endpoints");
  read_from_tree(padTree, mLineDefaults.colorGradient.alpha, "default_line_colors_gradient_alpha");
  read_from_tree(padTree, mLineDefaults.colorGradient.nColors, "default_line_colors_gradient_nColors");
  read_from_tree(padTree, mFillDefaults.alpha, "default_fill_alpha");
  read_from_tree(padTree, mFillDefaults.styles, "default_fill_styles");
  read_from_tree(padTree, mFillDefaults.colors, "default_fill_colors");
  read_from_tree(padTree, mFillDefaults.colorGradient.rgbEndpoints, "default_fill_colors_gradient_endpoints");
  read_from_tree(padTree, mFillDefaults.colorGradient.alpha, "default_fill_colors_gradient_alpha");
  read_from_tree(padTree, mFillDefaults.colorGradient.nColors, "default_fill_colors_gradient_nColors");
  read_from_tree(padTree, mCandleOptionDefaults.boxRange, "default_candle_option_boxrange");
  read_from_tree(padTree, mCandleOptionDefaults.whiskerRange, "default_candle_option_whiskerrange");
  read_from_tree(padTree, mRedrawAxes, "redraw_axes");

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
    // add axes
    if (str_contains(content.first, "AXIS")) {
      char axis = content.second.get<char>("name");
      mAxes[axis] = Axis(content.second);
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
  put_in_tree(padTree, mPalette.colorGradient.rgbEndpoints, "palette_colors_gradient_endpoints");
  put_in_tree(padTree, mPalette.colorGradient.alpha, "palette_colors_gradient_alpha");
  put_in_tree(padTree, mPalette.colorGradient.nColors, "palette_colors_gradient_nColors");
  put_in_tree(padTree, mFill.color, "fill_color");
  put_in_tree(padTree, mFill.style, "fill_style");
  put_in_tree(padTree, mFill.alpha, "fill_alpha");
  put_in_tree(padTree, mFrameFill.color, "frame_fill_color");
  put_in_tree(padTree, mFrameFill.style, "frame_fill_style");
  put_in_tree(padTree, mFrameFill.alpha, "frame_fill_alpha");
  put_in_tree(padTree, mFrameBorder.color, "frame_border_color");
  put_in_tree(padTree, mFrameBorder.alpha, "frame_border_alpha");
  put_in_tree(padTree, mFrameBorder.style, "frame_border_style");
  put_in_tree(padTree, mFrameBorder.scale, "frame_border_width");
  put_in_tree(padTree, mText.style, "text_font");
  put_in_tree(padTree, mText.color, "text_color");
  put_in_tree(padTree, mText.alpha, "text_alpha");
  put_in_tree(padTree, mText.scale, "text_size");
  put_in_tree(padTree, mMarkerDefaults.alpha, "default_marker_alpha");
  put_in_tree(padTree, mMarkerDefaults.scale, "default_marker_size");
  put_in_tree(padTree, mMarkerDefaults.styles, "default_marker_styles");
  put_in_tree(padTree, mMarkerDefaults.colors, "default_marker_colors");
  put_in_tree(padTree, mMarkerDefaults.colorGradient.rgbEndpoints, "default_marker_colors_gradient_endpoints");
  put_in_tree(padTree, mMarkerDefaults.colorGradient.alpha, "default_marker_colors_gradient_alpha");
  put_in_tree(padTree, mMarkerDefaults.colorGradient.nColors, "default_marker_colors_gradient_nColors");
  put_in_tree(padTree, mLineDefaults.alpha, "default_line_alpha");
  put_in_tree(padTree, mLineDefaults.scale, "default_line_width");
  put_in_tree(padTree, mLineDefaults.styles, "default_line_styles");
  put_in_tree(padTree, mLineDefaults.colors, "default_line_colors");
  put_in_tree(padTree, mLineDefaults.colorGradient.rgbEndpoints, "default_line_colors_gradient_endpoints");
  put_in_tree(padTree, mLineDefaults.colorGradient.alpha, "default_line_colors_gradient_alpha");
  put_in_tree(padTree, mLineDefaults.colorGradient.nColors, "default_line_colors_gradient_nColors");
  put_in_tree(padTree, mFillDefaults.alpha, "default_fill_alpha");
  put_in_tree(padTree, mFillDefaults.styles, "default_fill_styles");
  put_in_tree(padTree, mFillDefaults.colors, "default_fill_colors");
  put_in_tree(padTree, mFillDefaults.colorGradient.rgbEndpoints, "default_fill_colors_gradient_endpoints");
  put_in_tree(padTree, mFillDefaults.colorGradient.alpha, "default_fill_colors_gradient_alpha");
  put_in_tree(padTree, mFillDefaults.colorGradient.nColors, "default_fill_colors_gradient_nColors");
  put_in_tree(padTree, mCandleOptionDefaults.boxRange, "default_candle_option_boxrange");
  put_in_tree(padTree, mCandleOptionDefaults.whiskerRange, "default_candle_option_whiskerrange");
  put_in_tree(padTree, mRedrawAxes, "redraw_axes");

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

  for (const auto& axis : {'X', 'Y', 'Z'}) {
    if (mAxes.find(axis) != mAxes.end()) {
      padTree.put_child(string("AXIS_") + axis, mAxes.at(axis).GetPropertyTree());
    }
  }

  return padTree;
}

//**************************************************************************************************
/**
 * Apply all settings from pad on top of this pad.
 */
//**************************************************************************************************
void Plot::Pad::operator+=(const Pad& pad)
{
  if (this == &pad) logger::throw_invalid_argument("Cannot add a pad to itself.");
  if (pad.mPosition.xlow) mPosition.xlow = pad.mPosition.xlow;
  if (pad.mPosition.ylow) mPosition.ylow = pad.mPosition.ylow;
  if (pad.mPosition.xup) mPosition.xup = pad.mPosition.xup;
  if (pad.mPosition.yup) mPosition.yup = pad.mPosition.yup;
  if (pad.mMargins.top) mMargins.top = pad.mMargins.top;
  if (pad.mMargins.bottom) mMargins.bottom = pad.mMargins.bottom;
  if (pad.mMargins.left) mMargins.left = pad.mMargins.left;
  if (pad.mMargins.right) mMargins.right = pad.mMargins.right;
  if (pad.mView.theta) mView.theta = pad.mView.theta;
  if (pad.mView.phi) mView.phi = pad.mView.phi;
  if (pad.mFill.color) mFill.color = pad.mFill.color;
  if (pad.mFill.style) mFill.style = pad.mFill.style;
  if (pad.mFill.alpha) mFill.alpha = pad.mFill.alpha;
  if (pad.mFrameFill.color) mFrameFill.color = pad.mFrameFill.color;
  if (pad.mFrameFill.style) mFrameFill.style = pad.mFrameFill.style;
  if (pad.mFrameFill.alpha) mFrameFill.alpha = pad.mFrameFill.alpha;
  if (pad.mFrameBorder.color) mFrameBorder.color = pad.mFrameBorder.color;
  if (pad.mFrameBorder.alpha) mFrameBorder.alpha = pad.mFrameBorder.alpha;
  if (pad.mFrameBorder.style) mFrameBorder.style = pad.mFrameBorder.style;
  if (pad.mFrameBorder.scale) mFrameBorder.scale = pad.mFrameBorder.scale;
  if (pad.mText.scale) mText.scale = pad.mText.scale;
  if (pad.mText.style) mText.style = pad.mText.style;
  if (pad.mText.color) mText.color = pad.mText.color;
  if (pad.mText.alpha) mText.alpha = pad.mText.alpha;
  if (pad.mMarkerDefaults.alpha) mMarkerDefaults.alpha = pad.mMarkerDefaults.alpha;
  if (pad.mMarkerDefaults.scale) mMarkerDefaults.scale = pad.mMarkerDefaults.scale;
  if (pad.mMarkerDefaults.styles) mMarkerDefaults.styles = pad.mMarkerDefaults.styles;
  if (pad.mMarkerDefaults.colors) {
    mMarkerDefaults.colors = pad.mMarkerDefaults.colors;
    mMarkerDefaults.colorGradient = {};
  }
  if (pad.mMarkerDefaults.colorGradient.rgbEndpoints) {
    // a gradient replaces the colour list and comes with its own alpha and number of colours; an alpha set on the pad after its gradient is kept
    mMarkerDefaults.colorGradient = pad.mMarkerDefaults.colorGradient;
    mMarkerDefaults.colors = nullopt;
    mMarkerDefaults.alpha = pad.mMarkerDefaults.alpha;
  }
  if (pad.mLineDefaults.alpha) mLineDefaults.alpha = pad.mLineDefaults.alpha;
  if (pad.mLineDefaults.scale) mLineDefaults.scale = pad.mLineDefaults.scale;
  if (pad.mLineDefaults.styles) mLineDefaults.styles = pad.mLineDefaults.styles;
  if (pad.mLineDefaults.colors) {
    mLineDefaults.colors = pad.mLineDefaults.colors;
    mLineDefaults.colorGradient = {};
  }
  if (pad.mLineDefaults.colorGradient.rgbEndpoints) {
    // a gradient replaces the colour list and comes with its own alpha and number of colours; an alpha set on the pad after its gradient is kept
    mLineDefaults.colorGradient = pad.mLineDefaults.colorGradient;
    mLineDefaults.colors = nullopt;
    mLineDefaults.alpha = pad.mLineDefaults.alpha;
  }
  if (pad.mFillDefaults.alpha) mFillDefaults.alpha = pad.mFillDefaults.alpha;
  if (pad.mFillDefaults.styles) mFillDefaults.styles = pad.mFillDefaults.styles;
  if (pad.mFillDefaults.colors) {
    mFillDefaults.colors = pad.mFillDefaults.colors;
    mFillDefaults.colorGradient = {};
  }
  if (pad.mFillDefaults.colorGradient.rgbEndpoints) {
    // a gradient replaces the colour list and comes with its own alpha and number of colours; an alpha set on the pad after its gradient is kept
    mFillDefaults.colorGradient = pad.mFillDefaults.colorGradient;
    mFillDefaults.colors = nullopt;
    mFillDefaults.alpha = pad.mFillDefaults.alpha;
  }
  if (pad.mCandleOptionDefaults.boxRange) mCandleOptionDefaults.boxRange = pad.mCandleOptionDefaults.boxRange;
  if (pad.mCandleOptionDefaults.whiskerRange) mCandleOptionDefaults.whiskerRange = pad.mCandleOptionDefaults.whiskerRange;

  if (pad.mPalette.colorGradient.rgbEndpoints) {
    mPalette.colorGradient.rgbEndpoints = pad.mPalette.colorGradient.rgbEndpoints;
    if (pad.mPalette.colorGradient.alpha) mPalette.colorGradient.alpha = pad.mPalette.colorGradient.alpha;
    if (pad.mPalette.colorGradient.nColors) mPalette.colorGradient.nColors = pad.mPalette.colorGradient.nColors;
    mPalette.index = nullopt;
  }
  if (pad.mPalette.index) {
    mPalette.index = pad.mPalette.index;
    mPalette.colorGradient = {};
  }
  if (pad.mRedrawAxes) mRedrawAxes = pad.mRedrawAxes;
  if (pad.mRefFunc) mRefFunc = pad.mRefFunc->Clone();
  if (pad.mFrame) mFrame = pad.mFrame->Clone();
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
  }
  for (const auto& data : pad.mData) {
    auto clonedData = data->Clone();
    if (clonedData && clonedData->GetLegendID()) {
      size_t shiftedLegendID = *clonedData->GetLegendID() + legendOffset;
      if (shiftedLegendID > std::numeric_limits<uint8_t>::max()) {
        logger::throw_out_of_range("Too many legend boxes to merge into a single pad (legend ID {} would exceed the maximum of {}).", shiftedLegendID, std::numeric_limits<uint8_t>::max());
      }
      clonedData->SetLegend(static_cast<uint8_t>(shiftedLegendID));
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
Plot::Pad::Data& Plot::Pad::operator()(uint8_t dataID)
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
  const vector<char> allowedAxes = {'X', 'Y', 'Z'};
  if (std::find(allowedAxes.begin(), allowedAxes.end(), axis) == allowedAxes.end()) {
    logger::throw_invalid_argument("Axis '{}' does not exist! Please use 'X', 'Y' or 'Z'.", axis);
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
Plot::Pad::Data& Plot::Pad::GetData(uint8_t dataID)
{
  if (dataID < 1 || dataID > mData.size()) {
    logger::throw_out_of_range("Data with ID '{}' is not defined! You can access only data that was already added to the pad.", dataID);
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
    logger::throw_out_of_range("Legend with ID {} is not defined! You can access only legends that have already been added to the pad.", legendID);
  }
  return *mLegendBoxes[legendID - 1];
}

//**************************************************************************************************
/**
 * Access function for pad axis.
 */
//**************************************************************************************************
Plot::Pad::TextBox& Plot::Pad::GetText(uint8_t textID)
{
  if (textID < 1 || textID > mTextBoxes.size()) {
    logger::throw_out_of_range("Text with ID {} is not defined! You can access only texts that have already been added to the pad.", textID);
  }
  return *mTextBoxes[textID - 1];
}

//**************************************************************************************************
/**
 * Add data to this pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::AddData(const string& name, const string& dataSource, const optional<string>& label)
{
  mData.push_back(std::make_shared<Data>(name, dataSource, label));
  return *mData.back();
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
  mData.push_back(std::make_shared<Data>(function, "USER_FUNCTIONS", label));
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
    logger::throw_invalid_argument("Illegal arguments for adding points.");
  }
  string xStr;
  string yStr;
  for (size_t i = 0; i < nPoints; ++i) {
    xStr += number_to_string(x[i]) + ((i == nPoints - 1) ? "" : ",");
    yStr += number_to_string(y[i]) + ((i == nPoints - 1) ? "" : ",");
  }
  mData.push_back(std::make_shared<Data>(xStr + ";" + yStr, "USER_GRAPHS", label));
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
 * Add user-defined line to this pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::AddLine(pair<double_t, double_t> pos1, pair<double_t, double_t> pos2, const optional<string>& label)
{
  auto& data = AddPoints({pos1.first, pos2.first}, {pos1.second, pos2.second}, label);
  data.SetDrawOption(line);
  return data;
}

//**************************************************************************************************
/**
 * Add ratio to this pad.
 */
//**************************************************************************************************
Plot::Pad::Ratio& Plot::Pad::AddRatio(const string& numeratorName, const string& numeratorDataSource, const string& denominatorName, const string& denominatorDataSource, const optional<string>& label)
{
  mData.push_back(std::make_shared<Ratio>(numeratorName, numeratorDataSource,
                                          denominatorName, denominatorDataSource, label));
  return *std::dynamic_pointer_cast<Ratio>(mData.back());
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
 * Add reference function.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::SetRefFunc(const string& refFunc)
{
  mRefFunc = std::make_shared<Data>(refFunc, "USER_FUNCTIONS", nullopt);
  return *mRefFunc;
}

//**************************************************************************************************
/**
 * Set the input that defines the axis frame of this pad without being drawn (e.g. for labelled bins or a given binning).
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::SetFrame(const string& name, const string& dataSource)
{
  mFrame = std::make_shared<Data>(name, dataSource, nullopt);
  return *mFrame;
}

Plot::Pad::Data& Plot::Pad::SetFrame(const Data& data)
{
  if (data.mType == "ratio") {
    logger::throw_invalid_argument("A ratio ({}) cannot be the frame of a pad.", data.mName);
  }
  if (data.mName.empty() || data.mDataSource.empty()) {
    logger::throw_invalid_argument("The frame needs a name and a data source (name: '{}', data source: '{}').", data.mName, data.mDataSource);
  }
  mFrame = std::make_shared<Data>(data);
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
    mDataSource = dataTree.get<string>("dataSource");
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct data from ptree: {}", e.what());
  }

  if (auto var = dataTree.get_optional<bool>("defines_frame")) mDefinesFrame = *var;
  if (auto var = dataTree.get_optional<bool>("keep_input_style")) mKeepInputStyle = *var;
  read_from_tree(dataTree, mLegend.label, "legend_label");
  read_from_tree(dataTree, mLegend.id, "legend_id");
  if (const auto entryTree = dataTree.get_child_optional("legend_entry")) mLegend.entry = LegendEntry(*entryTree);
  read_from_tree(dataTree, mDrawOption, "draw_option");
  ReadDrawOption(dataTree, mDrawOptionAlias, "draw_option_alias");
  read_from_tree(dataTree, mTextFormat, "text_format");
  read_from_tree(dataTree, mStats.format, "stats_format");
  read_from_tree(dataTree, mStats.title, "stats_title");
  if (const auto statsBoxTree = dataTree.get_child_optional("stats_box")) mStats.box = std::make_shared<const StatsBox>(*statsBoxTree);
  read_from_tree(dataTree, mMarker.color, "marker_color");
  read_from_tree(dataTree, mMarker.alpha, "marker_alpha");
  read_from_tree(dataTree, mMarker.style, "marker_style");
  read_from_tree(dataTree, mMarker.scale, "marker_size");
  read_from_tree(dataTree, mLine.color, "line_color");
  read_from_tree(dataTree, mLine.alpha, "line_alpha");
  read_from_tree(dataTree, mLine.style, "line_style");
  ReadWidthFromTree(dataTree, mLine.scale, "line_width");
  read_from_tree(dataTree, mFill.color, "fill_color");
  read_from_tree(dataTree, mFill.alpha, "fill_alpha");
  read_from_tree(dataTree, mFill.style, "fill_style");
  ReadModify(dataTree, mModify);
  read_from_tree(dataTree, mRangeX.min, "rangeX_min");
  read_from_tree(dataTree, mRangeX.max, "rangeX_max");
  read_from_tree(dataTree, mRangeY.min, "rangeY_min");
  read_from_tree(dataTree, mRangeY.max, "rangeY_max");
  read_from_tree(dataTree, mScaleRange.min, "scale_range_min");
  read_from_tree(dataTree, mScaleRange.max, "scale_range_max");
  read_from_tree(dataTree, mContours, "contours");
  read_from_tree(dataTree, mNContours, "number_of_contours");
  read_from_tree(dataTree, mNpx, "npx");
  read_from_tree(dataTree, mNpy, "npy");

  // extract data info
  {
    optional<vector<string>> vars;
    read_from_tree(dataTree, vars, "data_vars");
    if (vars) {
      mDataInfo = data_info_t();
      vector<data_dim_t> dataDims{};

      optional<vector<double_t>> binning;
      optional<vector<int32_t>> sizes;
      read_from_tree(dataTree, binning, "data_binning");
      read_from_tree(dataTree, sizes, "data_binning_sizes");

      if (binning && sizes) {
        if (sizes->size() != vars->size()) {
          logger::throw_invalid_argument("data_binning_sizes has {} entries, expected {} (one per data_vars entry).", sizes->size(), vars->size());
        }
        auto lastPos = binning->begin();
        for (size_t i = 0; i < vars->size(); ++i) {
          if ((*sizes)[i] < 1) {
            logger::throw_invalid_argument("data_binning_sizes entry {} must be at least 1.", i);
          }
          if (std::distance(lastPos, binning->end()) < static_cast<std::ptrdiff_t>((*sizes)[i])) {
            logger::throw_invalid_argument("data_binning does not contain enough entries for data_binning_sizes.");
          }
          auto nextPos = lastPos + (*sizes)[i];
          vector<double_t> edges(lastPos, nextPos);
          lastPos = nextPos;
          int32_t nBins = static_cast<int32_t>(edges.back());
          edges.pop_back();
          dataDims.push_back({vars->at(i), nBins, edges});
        }
      }
      mDataInfo.dataDims = dataDims;
      read_from_tree(dataTree, mDataInfo.filters, "data_filters");
      mDataInfo.joins = ReadJoins(dataTree, "data_joins");
      read_from_tree(dataTree, mDataInfo.weight, "data_weight");
      const auto entryMin = get_from_tree<uint32_t>(dataTree, "data_entryMin");
      if (const auto entryMax = get_from_tree<uint32_t>(dataTree, "data_entryMax")) {
        (entryMin) ? Entries(*entryMin, *entryMax) : Entries(*entryMax);
      }
      read_from_tree(dataTree, mDataInfo.isProfileNoScatter, "data_isProfileNoScatter");
      read_from_tree(dataTree, mDataInfo.definitions.keys, "data_definitions_keys");
      read_from_tree(dataTree, mDataInfo.definitions.values, "data_definitions_values");
    }
  }
  // extract proj info
  {
    optional<vector<uint8_t>> dims;
    optional<vector<tuple<uint8_t, double_t, double_t>>> ranges;
    optional<bool> isUserCoord;
    optional<bool> isProfile;
    read_from_tree(dataTree, dims, "proj_dims");
    read_from_tree(dataTree, ranges, "proj_ranges");
    read_from_tree(dataTree, isUserCoord, "proj_isUserCoord");
    read_from_tree(dataTree, isProfile, "proj_isProfile");
    if (dims) {
      if (!ranges) {
        logger::throw_invalid_argument("proj_dims is set but proj_ranges is missing.");
      }
      mProjInfo = {*dims, *ranges, isUserCoord, isProfile};
    }
  }
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
  read_from_tree(tree, modify.rebinGroupX, prefix + "rebinX");
  read_from_tree(tree, modify.rebinGroupY, prefix + "rebinY");
  read_from_tree(tree, modify.rebinGroupZ, prefix + "rebinZ");
  read_from_tree(tree, modify.scaleAxisX, prefix + "scale_axis_x");
  read_from_tree(tree, modify.scaleAxisY, prefix + "scale_axis_y");
  read_from_tree(tree, modify.scaleAxisZ, prefix + "scale_axis_z");
  read_from_tree(tree, modify.cumulative, prefix + "cumulative");
  read_from_tree(tree, modify.nIterSmooth, prefix + "nIter_smooth");
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
  put_in_tree(tree, modify.rebinGroupX, prefix + "rebinX");
  put_in_tree(tree, modify.rebinGroupY, prefix + "rebinY");
  put_in_tree(tree, modify.rebinGroupZ, prefix + "rebinZ");
  put_in_tree(tree, modify.scaleAxisX, prefix + "scale_axis_x");
  put_in_tree(tree, modify.scaleAxisY, prefix + "scale_axis_y");
  put_in_tree(tree, modify.scaleAxisZ, prefix + "scale_axis_z");
  put_in_tree(tree, modify.cumulative, prefix + "cumulative");
  put_in_tree(tree, modify.nIterSmooth, prefix + "nIter_smooth");
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
  dataTree.put("dataSource", mDataSource);
  if (mDefinesFrame) dataTree.put("defines_frame", mDefinesFrame);
  if (mKeepInputStyle) dataTree.put("keep_input_style", mKeepInputStyle);
  put_in_tree(dataTree, mLegend.label, "legend_label");
  put_in_tree(dataTree, mLegend.id, "legend_id");
  if (mLegend.entry) dataTree.put_child("legend_entry", mLegend.entry->GetPropertyTree());
  put_in_tree(dataTree, mDrawOption, "draw_option");
  PutDrawOption(dataTree, mDrawOptionAlias, "draw_option_alias");
  put_in_tree(dataTree, mTextFormat, "text_format");
  put_in_tree(dataTree, mStats.format, "stats_format");
  put_in_tree(dataTree, mStats.title, "stats_title");
  if (mStats.box) dataTree.put_child("stats_box", mStats.box->GetPropertyTree());
  put_in_tree(dataTree, mMarker.color, "marker_color");
  put_in_tree(dataTree, mMarker.alpha, "marker_alpha");
  put_in_tree(dataTree, mMarker.style, "marker_style");
  put_in_tree(dataTree, mMarker.scale, "marker_size");
  put_in_tree(dataTree, mLine.color, "line_color");
  put_in_tree(dataTree, mLine.alpha, "line_alpha");
  put_in_tree(dataTree, mLine.style, "line_style");
  put_in_tree(dataTree, mLine.scale, "line_width");
  put_in_tree(dataTree, mFill.color, "fill_color");
  put_in_tree(dataTree, mFill.alpha, "fill_alpha");
  put_in_tree(dataTree, mFill.style, "fill_style");
  PutModify(dataTree, mModify);
  put_in_tree(dataTree, mRangeX.min, "rangeX_min");
  put_in_tree(dataTree, mRangeX.max, "rangeX_max");
  put_in_tree(dataTree, mRangeY.min, "rangeY_min");
  put_in_tree(dataTree, mRangeY.max, "rangeY_max");
  put_in_tree(dataTree, mScaleRange.min, "scale_range_min");
  put_in_tree(dataTree, mScaleRange.max, "scale_range_max");
  put_in_tree(dataTree, mContours, "contours");
  put_in_tree(dataTree, mNContours, "number_of_contours");
  put_in_tree(dataTree, mNpx, "npx");
  put_in_tree(dataTree, mNpy, "npy");

  if (mDataInfo.dataDims.size()) {
    vector<string> vars;
    vector<double_t> binning;
    vector<int32_t> sizes;
    for (const auto& dataDim : mDataInfo.dataDims) {
      vars.push_back(dataDim.var);
      binning.insert(binning.end(), dataDim.edges.begin(), dataDim.edges.end());
      binning.push_back(static_cast<double_t>(dataDim.nBins));
      sizes.push_back(static_cast<int32_t>(dataDim.edges.size()) + 1);
    }
    put_in_tree(dataTree, optional<vector<string>>{vars}, "data_vars");
    put_in_tree(dataTree, optional<vector<double_t>>{binning}, "data_binning");
    put_in_tree(dataTree, optional<vector<int32_t>>{sizes}, "data_binning_sizes");
    put_in_tree(dataTree, mDataInfo.filters, "data_filters");
    PutJoins(dataTree, mDataInfo.joins, "data_joins");
    put_in_tree(dataTree, mDataInfo.weight, "data_weight");
    put_in_tree(dataTree, mDataInfo.entries.min, "data_entryMin");
    put_in_tree(dataTree, mDataInfo.entries.max, "data_entryMax");
    put_in_tree(dataTree, mDataInfo.isProfileNoScatter, "data_isProfileNoScatter");
    put_in_tree(dataTree, mDataInfo.definitions.keys, "data_definitions_keys");
    put_in_tree(dataTree, mDataInfo.definitions.values, "data_definitions_values");
  }
  if (mProjInfo) {
    put_in_tree(dataTree, optional<vector<uint8_t>>{mProjInfo->dims}, "proj_dims");
    put_in_tree(dataTree, optional<vector<tuple<uint8_t, double_t, double_t>>>{mProjInfo->ranges}, "proj_ranges");
    put_in_tree(dataTree, mProjInfo->isUserCoord, "proj_isUserCoord");
    put_in_tree(dataTree, mProjInfo->isProfile, "proj_isProfile");
  }
  return dataTree;
}

//**************************************************************************************************
/**
 * Reinterprete Data as Ratio.
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
  mLegend.entry = dataLayout.mLegend.entry;
  mRangeX.min = dataLayout.mRangeX.min;
  mRangeX.max = dataLayout.mRangeX.max;
  mRangeY.min = dataLayout.mRangeY.min;
  mRangeY.max = dataLayout.mRangeY.max;
  mScaleRange.min = dataLayout.mScaleRange.min;
  mScaleRange.max = dataLayout.mScaleRange.max;
  mMarker.color = dataLayout.mMarker.color;
  mMarker.alpha = dataLayout.mMarker.alpha;
  mMarker.style = dataLayout.mMarker.style;
  mMarker.scale = dataLayout.mMarker.scale;
  mLine.color = dataLayout.mLine.color;
  mLine.alpha = dataLayout.mLine.alpha;
  mLine.style = dataLayout.mLine.style;
  mLine.scale = dataLayout.mLine.scale;
  mFill.color = dataLayout.mFill.color;
  mFill.alpha = dataLayout.mFill.alpha;
  mFill.style = dataLayout.mFill.style;
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
  if (dataLayout.mKeepInputStyle) mKeepInputStyle = true;
  set_if(dataLayout.mTextFormat, mTextFormat);
  set_if(dataLayout.mLegend.entry, mLegend.entry);
  set_if(dataLayout.mRangeX.min, mRangeX.min);
  set_if(dataLayout.mRangeX.max, mRangeX.max);
  set_if(dataLayout.mRangeY.min, mRangeY.min);
  set_if(dataLayout.mRangeY.max, mRangeY.max);
  set_if(dataLayout.mScaleRange.min, mScaleRange.min);
  set_if(dataLayout.mScaleRange.max, mScaleRange.max);
  set_if(dataLayout.mMarker.color, mMarker.color);
  set_if(dataLayout.mMarker.alpha, mMarker.alpha);
  set_if(dataLayout.mMarker.style, mMarker.style);
  set_if(dataLayout.mMarker.scale, mMarker.scale);
  set_if(dataLayout.mLine.color, mLine.color);
  set_if(dataLayout.mLine.alpha, mLine.alpha);
  set_if(dataLayout.mLine.style, mLine.style);
  set_if(dataLayout.mLine.scale, mLine.scale);
  set_if(dataLayout.mFill.color, mFill.color);
  set_if(dataLayout.mFill.alpha, mFill.alpha);
  set_if(dataLayout.mFill.style, mFill.style);
  set_if(dataLayout.mContours, mContours);
  set_if(dataLayout.mNContours, mNContours);
  set_if(dataLayout.mNpx, mNpx);
  set_if(dataLayout.mNpy, mNpy);
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
  if (mLegend.entry->mRefDataID) {
    logger::warning("The legend entry of {} cannot refer to other data.", mName);
    mLegend.entry->mRefDataID.reset();
  }
  if (mLegend.entry->mPosition) {
    logger::warning("The legend entry of {} cannot be placed, it is at the place of the data.", mName);
    mLegend.entry->mPosition.reset();
  }
  return *this;
}
auto Plot::Pad::Data::SetDrawOption(const string& drawOption) -> decltype(*this)
{
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
  mTextFormat = textFormat;
  return *this;
}
auto Plot::Pad::Data::SetShowStats(const string& format, const optional<string>& title) -> decltype(*this)
{
  // the first line of the box ('n', which would be the name of the histogram) is shown exactly when there is a title
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
  if (min >= max) logger::warning("Range ({}, {}) of {} in x has min >= max.", min, max, mName);
  mRangeX.min = min;
  mRangeX.max = max;
  return *this;
}
auto Plot::Pad::Data::SetMinRangeX(double_t min) -> decltype(*this)
{
  mRangeX.min = min;
  return *this;
}
auto Plot::Pad::Data::SetMaxRangeX(double_t max) -> decltype(*this)
{
  mRangeX.max = max;
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
  if (min >= max) logger::warning("Range ({}, {}) of {} in y has min >= max.", min, max, mName);
  mRangeY.min = min;
  mRangeY.max = max;
  return *this;
}
auto Plot::Pad::Data::SetMinRangeY(double_t min) -> decltype(*this)
{
  mRangeY.min = min;
  return *this;
}
auto Plot::Pad::Data::SetMaxRangeY(double_t max) -> decltype(*this)
{
  mRangeY.max = max;
  return *this;
}
auto Plot::Pad::Data::SetRangeY() -> decltype(*this)
{
  mRangeY.min = {};
  mRangeY.max = {};
  return *this;
}
auto Plot::Pad::Data::SetScaleMinimum(double_t scaleFactor) -> decltype(*this)
{
  mScaleRange.min = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::SetScaleMaximum(double_t scaleFactor) -> decltype(*this)
{
  mScaleRange.max = scaleFactor;
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
  mMarker.alpha = alpha;
  mLine.alpha = alpha;
  mFill.alpha = alpha;
  return *this;
}
auto Plot::Pad::Data::SetMarker(color_t color, int16_t style, float_t size, optional<float_t> alpha) -> decltype(*this)
{
  mMarker.color = color;
  mMarker.alpha = alpha;
  mMarker.style = style;
  mMarker.scale = size;
  return *this;
}
auto Plot::Pad::Data::SetMarkerColor(color_t color) -> decltype(*this)
{
  mMarker.color = color;
  return *this;
}
auto Plot::Pad::Data::SetMarkerAlpha(float_t alpha) -> decltype(*this)
{
  mMarker.alpha = alpha;
  return *this;
}
auto Plot::Pad::Data::SetMarkerStyle(int16_t style) -> decltype(*this)
{
  mMarker.style = style;
  return *this;
}
auto Plot::Pad::Data::SetMarkerSize(float_t size) -> decltype(*this)
{
  mMarker.scale = size;
  return *this;
}
auto Plot::Pad::Data::SetLine(color_t color, int16_t style, float_t width, optional<float_t> alpha) -> decltype(*this)
{
  mLine.color = color;
  mLine.alpha = alpha;
  mLine.style = style;
  mLine.scale = RoundWidth(width, "line width");
  return *this;
}
auto Plot::Pad::Data::SetLineColor(color_t color) -> decltype(*this)
{
  mLine.color = color;
  return *this;
}
auto Plot::Pad::Data::SetLineAlpha(float_t alpha) -> decltype(*this)
{
  mLine.alpha = alpha;
  return *this;
}
auto Plot::Pad::Data::SetLineStyle(int16_t style) -> decltype(*this)
{
  mLine.style = style;
  return *this;
}
auto Plot::Pad::Data::SetLineWidth(float_t width) -> decltype(*this)
{
  mLine.scale = RoundWidth(width, "line width");
  return *this;
}
auto Plot::Pad::Data::SetFill(color_t color, int16_t style, optional<float_t> alpha) -> decltype(*this)
{
  mFill.color = color;
  mFill.alpha = alpha;
  mFill.style = style;
  return *this;
}
auto Plot::Pad::Data::SetFillColor(color_t color) -> decltype(*this)
{
  mFill.color = color;
  return *this;
}
auto Plot::Pad::Data::SetFillStyle(int16_t style) -> decltype(*this)
{
  mFill.style = style;
  return *this;
}
auto Plot::Pad::Data::SetFillAlpha(float_t alpha) -> decltype(*this)
{
  mFill.alpha = alpha;
  return *this;
}
auto Plot::Pad::Data::SetDefinesFrame() -> decltype(*this)
{
  mDefinesFrame = true;
  return *this;
}
auto Plot::Pad::Data::SetKeepInputStyle(bool keep) -> decltype(*this)
{
  mKeepInputStyle = keep;
  return *this;
}
auto Plot::Pad::Data::SetContours(const vector<double>& contours) -> decltype(*this)
{
  mContours = contours;
  return *this;
}
auto Plot::Pad::Data::SetContours(int32_t nContours) -> decltype(*this)
{
  mNContours = nContours;
  return *this;
}
auto Plot::Pad::Data::SetNpx(int32_t nx, optional<int32_t> ny) -> decltype(*this)
{
  mNpx = nx;
  mNpy = ny;
  return *this;
}
auto Plot::Pad::Data::Normalize(bool isDensity, optional<double_t> min, optional<double_t> max) -> decltype(*this)
{
  if (min && max && !(*min < *max)) {
    logger::warning("Range [{}, {}] of Normalize for {} is empty, ignoring the range.", *min, *max, GetName());
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
  if (min && max && !(*min < *max)) {
    logger::warning("Range [{}, {}] of NormalizeToMaximum for {} is empty, ignoring the range.", *min, *max, GetName());
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
  mModify.scaleFactor = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::Smooth(uint16_t nIterSmooth) -> decltype(*this)
{
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
  mModify.rebinGroupX = nGroup;
  return *this;
}
auto Plot::Pad::Data::RebinY(uint16_t nGroup) -> decltype(*this)
{
  mModify.rebinGroupY = nGroup;
  return *this;
}
auto Plot::Pad::Data::RebinZ(uint16_t nGroup) -> decltype(*this)
{
  mModify.rebinGroupZ = nGroup;
  return *this;
}
auto Plot::Pad::Data::RebinXY(uint16_t nGroupX, uint16_t nGroupY) -> decltype(*this)
{
  mModify.rebinGroupX = nGroupX;
  mModify.rebinGroupY = nGroupY;
  return *this;
}
auto Plot::Pad::Data::RebinXYZ(uint16_t nGroupX, uint16_t nGroupY, uint16_t nGroupZ) -> decltype(*this)
{
  mModify.rebinGroupX = nGroupX;
  mModify.rebinGroupY = nGroupY;
  mModify.rebinGroupZ = nGroupZ;
  return *this;
}
auto Plot::Pad::Data::ScaleX(double_t scaleFactor) -> decltype(*this)
{
  mModify.scaleAxisX = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::ScaleY(double_t scaleFactor) -> decltype(*this)
{
  mModify.scaleAxisY = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::ScaleZ(double_t scaleFactor) -> decltype(*this)
{
  mModify.scaleAxisZ = scaleFactor;
  return *this;
}
auto Plot::Pad::Data::Project(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  if (dims.empty()) logger::throw_invalid_argument("Project needs at least one axis to project onto.");
  mProjInfo = {dims, ranges, isUserCoord};
  return *this;
}
auto Plot::Pad::Data::ProjectX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{0}, {{1, startY, endY}}, isUserCoord};
  return *this;
}
auto Plot::Pad::Data::ProjectY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{1}, {{0, startX, endX}}, isUserCoord};
  return *this;
}
auto Plot::Pad::Data::Profile(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  if (dims.empty()) logger::throw_invalid_argument("Profile needs at least one axis to profile along.");
  mProjInfo = {dims, ranges, isUserCoord, true};
  return *this;
}
auto Plot::Pad::Data::ProfileX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{0}, {{1, startY, endY}}, isUserCoord, true};
  return *this;
}
auto Plot::Pad::Data::ProfileY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  mProjInfo = {{1}, {{0, startX, endX}}, isUserCoord, true};
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
void Plot::Pad::Data::data_info_t::set(const data_info_t& other)
{
  if (other.dataDims.empty()) {
    if (other.isProfileNoScatter.value_or(false)) {
      logger::throw_invalid_argument("Profile needs at least one binned axis besides the profiled variable.");
    } else {
      logger::throw_invalid_argument("Project needs at least one dimension.");
    }
  }
  // binned axes (all of a projection, all but the profiled one of a profile) need a positive number of bins or explicit edges
  const size_t nBinned = other.isProfileNoScatter.value_or(true) ? other.dataDims.size() - other.isProfileNoScatter.value_or(false) : 0;
  for (size_t i = 0; i < nBinned; ++i) {
    const auto& dim = other.dataDims[i];
    if (dim.nBins < 0 || (dim.nBins == 0 && dim.edges.size() <= 2)) {
      logger::throw_invalid_argument("Number of bins for {} must be positive (or give the bin edges).", dim.var);
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
auto Plot::Pad::Data::Profile(const vector<data_dim_t>& dataDims, const string& profile, optional<string> weight) -> decltype(*this)
{
  mDataInfo.set({dataDims, weight, true});
  mDataInfo.dataDims.push_back({profile, {}});
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
  AddJoin(mDataInfo, mName, tree, keys, alias);
  return *this;
}
void Plot::Pad::Data::AddJoin(data_info_t& dataInfo, const std::string& name, const join_tree_t& tree, const std::vector<std::string>& keys, const std::string& alias)
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
  if (keys.size() == 2 && keys[0] == keys[1]) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: key column {} is given twice.", tree.name, name, keys[0]);
  }
  if (!alias.empty() && !isIdentifier(alias)) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: alias '{}' must consist of letters, digits and underscores only and must not start with a digit.", tree.name, name, alias);
  }
  data_info_t::join_t join{tree.name, tree.dataSource, keys, alias};
  if (!isIdentifier(join.GetAlias())) {
    logger::throw_invalid_argument("Cannot join tree {} to {}: its columns cannot be named {}.column, please specify an alias.", tree.name, name, join.GetAlias());
  }
  // the columns of each tree must be distinguishable by their prefix (for the tree itself this is checked when joining, since its name can still change)
  auto suggestAlias = [&]() {
    return fmt::format("Join({}, {{{}}}, \"{}\")", (join.dataSource.empty()) ? "\"" + join.tree + "\"" : fmt::format("{{\"{}\", \"{}\"}}", join.tree, join.dataSource),
                       (keys.empty()) ? "" : "\"" + join_strings(keys, "\", \"") + "\"", "myAlias");
  };
  if (!dataInfo.joins) dataInfo.joins.emplace();
  for (const auto& other : *dataInfo.joins) {
    if (other == join) {
      logger::warning("Tree {} is already joined to {}; ignoring the repeated Join.", other.GetDescription(), name);
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
      logger::error("Entry range [{}, {}) is empty; ignoring.", *entryMin, entryMax);
      return;
    }
  } else if (!entryMax) {
    logger::error("Entries(0) would select no entries at all; ignoring.");
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
  return (alias.empty()) ? tree.substr(tree.find_last_of('/') + 1) : alias;
}
string Plot::Pad::Data::data_info_t::join_t::ToString() const
{
  return tree + "|" + dataSource + "|" + join_strings(keys, ",") + "|" + alias;
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
  fields.resize(4);
  join_t join{fields[0], fields[1], {}, fields[3]};
  if (!fields[2].empty()) join.keys = util::split_string(fields[2], ',');
  return join;
}
string Plot::Pad::Data::data_info_t::join_t::GetDescription() const
{
  string description = tree;
  if (!dataSource.empty()) description += " from " + dataSource;
  if (!alias.empty()) description += " as " + alias;
  description += (keys.empty()) ? " (row by row)" : " (by " + join_strings(keys, ", ") + ")";
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
  // (';' between statements becomes ',' since ROOT cuts key names at ';')
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
      compacted += (c == ';') ? ',' : c;
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
    if (data->mName.empty() || data->mDataSource.empty()) {
      logger::throw_invalid_argument("The {} of a ratio needs a name and a data source (name: '{}', data source: '{}').", part, data->mName, data->mDataSource);
    }
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
    mDenomDataSource(denomDataSource), mIsCorrelated(false)
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
    mDenomName = dataTree.get<string>("denomName");
    mDenomDataSource = dataTree.get<string>("denomDataSource");
    mIsCorrelated = dataTree.get<bool>("isCorrelated");
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct ratio from ptree: {}", e.what());
  }

  ReadModify(dataTree, mNumModify, "numer_");
  ReadModify(dataTree, mDenomModify, "denom_");

  // extract data info
  {
    optional<vector<string>> vars;
    read_from_tree(dataTree, vars, "denomData_vars");
    if (vars) {
      mDenomDataInfo = data_info_t();
      vector<data_dim_t> dataDims{};

      optional<vector<double_t>> binning;
      optional<vector<int32_t>> sizes;
      read_from_tree(dataTree, binning, "denomData_binning");
      read_from_tree(dataTree, sizes, "denomData_binning_sizes");

      if (binning && sizes) {
        if (sizes->size() != vars->size()) {
          logger::throw_invalid_argument("denomData_binning_sizes has {} entries, expected {} (one per denomData_vars entry).", sizes->size(), vars->size());
        }
        auto lastPos = binning->begin();
        for (size_t i = 0; i < vars->size(); ++i) {
          if ((*sizes)[i] < 1) {
            logger::throw_invalid_argument("denomData_binning_sizes entry {} must be at least 1.", i);
          }
          if (std::distance(lastPos, binning->end()) < static_cast<std::ptrdiff_t>((*sizes)[i])) {
            logger::throw_invalid_argument("denomData_binning does not contain enough entries for denomData_binning_sizes.");
          }
          auto nextPos = lastPos + (*sizes)[i];
          vector<double_t> edges(lastPos, nextPos);
          lastPos = nextPos;
          int32_t nBins = static_cast<int32_t>(edges.back());
          edges.pop_back();
          dataDims.push_back({vars->at(i), nBins, edges});
        }
      }
      mDenomDataInfo.dataDims = dataDims;
      read_from_tree(dataTree, mDenomDataInfo.filters, "denomData_filters");
      mDenomDataInfo.joins = ReadJoins(dataTree, "denomData_joins");
      read_from_tree(dataTree, mDenomDataInfo.weight, "denomData_weight");
      const auto entryMin = get_from_tree<uint32_t>(dataTree, "denomData_entryMin");
      if (const auto entryMax = get_from_tree<uint32_t>(dataTree, "denomData_entryMax")) {
        (entryMin) ? Denom().Entries(*entryMin, *entryMax) : Denom().Entries(*entryMax);
        Numer();
      }
      read_from_tree(dataTree, mDenomDataInfo.isProfileNoScatter, "denomData_isProfileNoScatter");
      read_from_tree(dataTree, mDenomDataInfo.definitions.keys, "denomData_definitions_keys");
      read_from_tree(dataTree, mDenomDataInfo.definitions.values, "denomData_definitions_values");
    }
  }
  // extract proj info
  {
    optional<vector<uint8_t>> dims;
    optional<vector<tuple<uint8_t, double_t, double_t>>> ranges;
    optional<bool> isUserCoord;
    optional<bool> isProfile;
    read_from_tree(dataTree, dims, "denomProj_dims");
    read_from_tree(dataTree, ranges, "denomProj_ranges");
    read_from_tree(dataTree, isUserCoord, "denomProj_isUserCoord");
    read_from_tree(dataTree, isProfile, "denomProj_isProfile");
    if (dims) {
      if (!ranges) {
        logger::throw_invalid_argument("denomProj_dims is set but denomProj_ranges is missing.");
      }
      mDenomProjInfo = {*dims, *ranges, isUserCoord, isProfile};
    }
  }
}

//**************************************************************************************************
/**
 * Convert ratio to property tree.
 */
//**************************************************************************************************
ptree Plot::Pad::Ratio::GetPropertyTree() const
{
  ptree dataTree = Data::GetPropertyTree();
  dataTree.put("denomName", mDenomName);
  dataTree.put("denomDataSource", mDenomDataSource);
  dataTree.put("isCorrelated", mIsCorrelated);
  PutModify(dataTree, mNumModify, "numer_");
  PutModify(dataTree, mDenomModify, "denom_");

  if (mDenomDataInfo.dataDims.size()) {
    vector<string> vars;
    vector<double_t> binning;
    vector<int32_t> sizes;
    for (const auto& dataDim : mDenomDataInfo.dataDims) {
      vars.push_back(dataDim.var);
      binning.insert(binning.end(), dataDim.edges.begin(), dataDim.edges.end());
      binning.push_back(static_cast<double_t>(dataDim.nBins));
      sizes.push_back(static_cast<int32_t>(dataDim.edges.size()) + 1);
    }
    put_in_tree(dataTree, optional<vector<string>>{vars}, "denomData_vars");
    put_in_tree(dataTree, optional<vector<double_t>>{binning}, "denomData_binning");
    put_in_tree(dataTree, optional<vector<int32_t>>{sizes}, "denomData_binning_sizes");
    put_in_tree(dataTree, mDenomDataInfo.filters, "denomData_filters");
    PutJoins(dataTree, mDenomDataInfo.joins, "denomData_joins");
    put_in_tree(dataTree, mDenomDataInfo.weight, "denomData_weight");
    put_in_tree(dataTree, mDenomDataInfo.entries.min, "denomData_entryMin");
    put_in_tree(dataTree, mDenomDataInfo.entries.max, "denomData_entryMax");
    put_in_tree(dataTree, mDenomDataInfo.isProfileNoScatter, "denomData_isProfileNoScatter");
    put_in_tree(dataTree, mDenomDataInfo.definitions.keys, "denomData_definitions_keys");
    put_in_tree(dataTree, mDenomDataInfo.definitions.values, "denomData_definitions_values");
  }
  if (mDenomProjInfo) {
    put_in_tree(dataTree, optional<vector<uint8_t>>{mDenomProjInfo->dims}, "denomProj_dims");
    put_in_tree(dataTree, optional<vector<tuple<uint8_t, double_t, double_t>>>{mDenomProjInfo->ranges}, "denomProj_ranges");
    put_in_tree(dataTree, mDenomProjInfo->isUserCoord, "denomProj_isUserCoord");
    put_in_tree(dataTree, mDenomProjInfo->isProfile, "denomProj_isProfile");
  }
  return dataTree;
}

//**************************************************************************************************
/**
 * Specify how the division should be done (spline, binomial, etc...).
 */
//**************************************************************************************************
auto Plot::Pad::Ratio::SetIsCorrelated(bool isCorrelated) -> decltype(*this)
{
  mIsCorrelated = isCorrelated;
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
void Plot::Pad::Ratio::ResolveSelectionMode(const char* call)
{
  if (mModMode != Mode::Res) return;
  logger::warning("Ratio {}: {}() selects input data, it cannot act on the ratio itself; interpreted as Numer().{}(). Write Numer(), Denom() or Both() explicitly.",
                  GetName(), call, call);
  mModMode = Mode::Num;
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
  if (dims.empty()) logger::throw_invalid_argument("Project needs at least one axis to project onto.");
  ResolveSelectionMode("Project");
  if (mModMode != Mode::Den) {
    Data::Project(dims, ranges, isUserCoord);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomProjInfo = {dims, ranges, isUserCoord};
  return *this;
}
auto Plot::Pad::Ratio::ProjectX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  ResolveSelectionMode("ProjectX");
  if (mModMode != Mode::Den) {
    Data::ProjectX(startY, endY, isUserCoord);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomProjInfo = {{0}, {{1, startY, endY}}, isUserCoord};
  return *this;
}
auto Plot::Pad::Ratio::ProjectY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  ResolveSelectionMode("ProjectY");
  if (mModMode != Mode::Den) {
    Data::ProjectY(startX, endX, isUserCoord);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomProjInfo = {{1}, {{0, startX, endX}}, isUserCoord};
  return *this;
}
auto Plot::Pad::Ratio::Profile(const vector<uint8_t>& dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  if (dims.empty()) logger::throw_invalid_argument("Profile needs at least one axis to profile along.");
  ResolveSelectionMode("Profile");
  if (mModMode != Mode::Den) {
    Data::Profile(dims, ranges, isUserCoord);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomProjInfo = {dims, ranges, isUserCoord, true};
  return *this;
}
auto Plot::Pad::Ratio::ProfileX(double_t startY, double_t endY, optional<bool> isUserCoord) -> decltype(*this)
{
  ResolveSelectionMode("ProfileX");
  if (mModMode != Mode::Den) {
    Data::ProfileX(startY, endY, isUserCoord);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomProjInfo = {{0}, {{1, startY, endY}}, isUserCoord, true};
  return *this;
}
auto Plot::Pad::Ratio::ProfileY(double_t startX, double_t endX, optional<bool> isUserCoord) -> decltype(*this)
{
  ResolveSelectionMode("ProfileY");
  if (mModMode != Mode::Den) {
    Data::ProfileY(startX, endX, isUserCoord);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomProjInfo = {{1}, {{0, startX, endX}}, isUserCoord, true};
  return *this;
}
auto Plot::Pad::Ratio::Project(std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Project(vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Ratio::Profile(std::initializer_list<uint8_t> dims, const vector<tuple<uint8_t, double_t, double_t>>& ranges, optional<bool> isUserCoord) -> decltype(*this)
{
  return Profile(vector<uint8_t>(dims), ranges, isUserCoord);
}
auto Plot::Pad::Ratio::Project(std::initializer_list<data_dim_t> dataDims, optional<string> weight) -> decltype(*this)
{
  return Project(vector<data_dim_t>(dataDims), weight);
}
auto Plot::Pad::Ratio::Project(const vector<data_dim_t>& dataDims, optional<string> weight) -> decltype(*this)
{
  ResolveSelectionMode("Project");
  if (mModMode != Mode::Den) {
    Data::Project(dataDims, weight);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({dataDims, weight});
  return *this;
}
auto Plot::Pad::Ratio::Project1D(data_dim_t x, optional<string> weight) -> decltype(*this)
{
  ResolveSelectionMode("Project1D");
  if (mModMode != Mode::Den) {
    Data::Project1D(x, weight);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{x}, weight});
  return *this;
}
auto Plot::Pad::Ratio::Project2D(data_dim_t x, data_dim_t y, optional<string> weight) -> decltype(*this)
{
  ResolveSelectionMode("Project2D");
  if (mModMode != Mode::Den) {
    Data::Project2D(x, y, weight);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{x, y}, weight});
  return *this;
}
auto Plot::Pad::Ratio::Scatter(const string& x, const string& y) -> decltype(*this)
{
  ResolveSelectionMode("Scatter");
  if (mModMode != Mode::Den) {
    Data::Scatter(x, y);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{{x}, {y}}, {}, false});
  return *this;
}
auto Plot::Pad::Ratio::Scatter(const string& x, const string& y, const string& xErr, const string& yErr) -> decltype(*this)
{
  ResolveSelectionMode("Scatter");
  if (mModMode != Mode::Den) {
    Data::Scatter(x, y, xErr, yErr);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{{x}, {y}, {xErr}, {yErr}}, {}, false});
  return *this;
}
auto Plot::Pad::Ratio::Scatter(const string& x, const string& y, const string& xErrLow, const string& xErrHigh, const string& yErrLow, const string& yErrHigh) -> decltype(*this)
{
  ResolveSelectionMode("Scatter");
  if (mModMode != Mode::Den) {
    Data::Scatter(x, y, xErrLow, xErrHigh, yErrLow, yErrHigh);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{{x}, {y}, {xErrLow}, {xErrHigh}, {yErrLow}, {yErrHigh}}, {}, false});
  return *this;
}
auto Plot::Pad::Ratio::Profile(const vector<data_dim_t>& dataDims, const string& profile, optional<string> weight) -> decltype(*this)
{
  ResolveSelectionMode("Profile");
  if (mModMode != Mode::Den) {
    Data::Profile(dataDims, profile, weight);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({dataDims, weight, true});
  mDenomDataInfo.dataDims.push_back({profile, {}});
  return *this;
}
auto Plot::Pad::Ratio::Profile1D(data_dim_t x, const string& profile, optional<string> weight) -> decltype(*this)
{
  ResolveSelectionMode("Profile1D");
  if (mModMode != Mode::Den) {
    Data::Profile1D(x, profile, weight);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{x, {profile, {}}}, weight, true});
  return *this;
}
auto Plot::Pad::Ratio::Profile2D(data_dim_t x, data_dim_t y, const string& profile, optional<string> weight) -> decltype(*this)
{
  ResolveSelectionMode("Profile2D");
  if (mModMode != Mode::Den) {
    Data::Profile2D(x, y, profile, weight);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.set({{x, y, {profile, {}}}, weight, true});
  return *this;
}
auto Plot::Pad::Ratio::Define(const std::string& key, const std::string& value) -> decltype(*this)
{
  ResolveSelectionMode("Define");
  if (mModMode != Mode::Den) {
    Data::Define(key, value);
    if (mModMode != Mode::Both) return *this;
  }
  if (mDenomDataInfo.definitions.keys && mDenomDataInfo.definitions.values) {
    mDenomDataInfo.definitions.keys->push_back(key);
    mDenomDataInfo.definitions.values->push_back(value);
  } else {
    mDenomDataInfo.definitions.keys = {key};
    mDenomDataInfo.definitions.values = {value};
  }
  return *this;
}
auto Plot::Pad::Ratio::Filter(const std::string& filter) -> decltype(*this)
{
  ResolveSelectionMode("Filter");
  if (mModMode != Mode::Den) {
    Data::Filter(filter);
    if (mModMode != Mode::Both) return *this;
  }
  if (mDenomDataInfo.filters) {
    mDenomDataInfo.filters->push_back(filter);
  } else {
    mDenomDataInfo.filters = {filter};
  }
  return *this;
}
auto Plot::Pad::Ratio::Join(const join_tree_t& tree, const std::vector<std::string>& keys, const std::string& alias) -> decltype(*this)
{
  ResolveSelectionMode("Join");
  if (mModMode != Mode::Den) {
    Data::Join(tree, keys, alias);
    if (mModMode != Mode::Both) return *this;
  }
  AddJoin(mDenomDataInfo, mDenomName, tree, keys, alias);
  return *this;
}
auto Plot::Pad::Ratio::Entries(uint32_t nEntries) -> decltype(*this)
{
  ResolveSelectionMode("Entries");
  if (mModMode != Mode::Den) {
    Data::Entries(nEntries);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.entries.Set(nullopt, nEntries);
  return *this;
}
auto Plot::Pad::Ratio::Entries(uint32_t entryMin, uint32_t entryMax) -> decltype(*this)
{
  ResolveSelectionMode("Entries");
  if (mModMode != Mode::Den) {
    Data::Entries(entryMin, entryMax);
    if (mModMode != Mode::Both) return *this;
  }
  mDenomDataInfo.entries.Set(entryMin, entryMax);
  return *this;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Axis
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Constructor setting axis name (X,Y,Z).
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
    mName = axisTree.get<char>("name");
  } catch (const std::exception& e) {
    logger::throw_invalid_argument("Could not construct axis from ptree: {}", e.what());
  }

  read_from_tree(axisTree, mAxisFrame.title, "title");
  read_from_tree(axisTree, mRange.min, "range_min");
  read_from_tree(axisTree, mRange.max, "range_max");
  read_from_tree(axisTree, mAxisFrame.style, "num_divisions");
  read_from_tree(axisTree, mAxisFrame.maxDigits, "max_digits");
  read_from_tree(axisTree, mAxisFrame.scale, "tick_length");
  read_from_tree(axisTree, mAxisFrame.color, "axis_color");
  read_from_tree(axisTree, mAxisFrame.alpha, "axis_alpha");
  read_from_tree(axisTree, mTitleLayout.style, "title_font");
  read_from_tree(axisTree, mTitleLayout.scale, "title_size");
  read_from_tree(axisTree, mTitleLayout.color, "title_color");
  read_from_tree(axisTree, mTitleLayout.alpha, "title_alpha");
  read_from_tree(axisTree, mTitleLayout.offset, "title_offset");
  read_from_tree(axisTree, mTitleLayout.center, "title_center");
  read_from_tree(axisTree, mLabelLayout.style, "label_font");
  read_from_tree(axisTree, mLabelLayout.scale, "label_size");
  read_from_tree(axisTree, mLabelLayout.color, "label_color");
  read_from_tree(axisTree, mLabelLayout.alpha, "label_alpha");
  read_from_tree(axisTree, mLabelLayout.offset, "label_offset");
  read_from_tree(axisTree, mLabelLayout.center, "label_center");
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
  put_in_tree(axisTree, mAxisFrame.style, "num_divisions");
  put_in_tree(axisTree, mAxisFrame.maxDigits, "max_digits");
  put_in_tree(axisTree, mAxisFrame.scale, "tick_length");
  put_in_tree(axisTree, mAxisFrame.color, "axis_color");
  put_in_tree(axisTree, mAxisFrame.alpha, "axis_alpha");
  put_in_tree(axisTree, mTitleLayout.style, "title_font");
  put_in_tree(axisTree, mTitleLayout.scale, "title_size");
  put_in_tree(axisTree, mTitleLayout.color, "title_color");
  put_in_tree(axisTree, mTitleLayout.alpha, "title_alpha");
  put_in_tree(axisTree, mTitleLayout.offset, "title_offset");
  put_in_tree(axisTree, mTitleLayout.center, "title_center");
  put_in_tree(axisTree, mLabelLayout.style, "label_font");
  put_in_tree(axisTree, mLabelLayout.scale, "label_size");
  put_in_tree(axisTree, mLabelLayout.color, "label_color");
  put_in_tree(axisTree, mLabelLayout.alpha, "label_alpha");
  put_in_tree(axisTree, mLabelLayout.offset, "label_offset");
  put_in_tree(axisTree, mLabelLayout.center, "label_center");
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
  if (axis.mTitleLayout.style) mTitleLayout.style = axis.mTitleLayout.style;
  if (axis.mTitleLayout.scale) mTitleLayout.scale = axis.mTitleLayout.scale;
  if (axis.mTitleLayout.color) mTitleLayout.color = axis.mTitleLayout.color;
  if (axis.mTitleLayout.alpha) mTitleLayout.alpha = axis.mTitleLayout.alpha;
  if (axis.mTitleLayout.offset) mTitleLayout.offset = axis.mTitleLayout.offset;
  if (axis.mTitleLayout.center) mTitleLayout.center = axis.mTitleLayout.center;
  if (axis.mLabelLayout.style) mLabelLayout.style = axis.mLabelLayout.style;
  if (axis.mLabelLayout.scale) mLabelLayout.scale = axis.mLabelLayout.scale;
  if (axis.mLabelLayout.color) mLabelLayout.color = axis.mLabelLayout.color;
  if (axis.mLabelLayout.alpha) mLabelLayout.alpha = axis.mLabelLayout.alpha;
  if (axis.mLabelLayout.offset) mLabelLayout.offset = axis.mLabelLayout.offset;
  if (axis.mLabelLayout.center) mLabelLayout.center = axis.mLabelLayout.center;
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
  if (min >= max) logger::warning("Range ({}, {}) of axis {} has min >= max.", min, max, mName);
  mRange = {min, max};
  return *this;
}
auto Plot::Pad::Axis::SetMaxRange(double_t max) -> decltype(*this)
{
  mRange.max = max;
  return *this;
}
auto Plot::Pad::Axis::SetMinRange(double_t min) -> decltype(*this)
{
  mRange.min = min;
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
  mAxisFrame.scale = tickLength;
  return *this;
}
auto Plot::Pad::Axis::SetTitleFont(int16_t font) -> decltype(*this)
{
  mTitleLayout.style = font;
  return *this;
}
auto Plot::Pad::Axis::SetLabelFont(int16_t font) -> decltype(*this)
{
  mLabelLayout.style = font;
  return *this;
}
auto Plot::Pad::Axis::SetTitleSize(float_t size) -> decltype(*this)
{
  mTitleLayout.scale = size;
  return *this;
}
auto Plot::Pad::Axis::SetLabelSize(float_t size) -> decltype(*this)
{
  mLabelLayout.scale = size;
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
  mTitleLayout.alpha = alpha;
  return *this;
}
auto Plot::Pad::Axis::SetLabelAlpha(float_t alpha) -> decltype(*this)
{
  mLabelLayout.alpha = alpha;
  return *this;
}
auto Plot::Pad::Axis::SetTitleOffset(float_t offset) -> decltype(*this)
{
  mTitleLayout.offset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetLabelOffset(float_t offset) -> decltype(*this)
{
  mLabelLayout.offset = offset;
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
  mAxisFrame.colorScaleOffset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetColorScaleWidth(float_t width) -> decltype(*this)
{
  mAxisFrame.colorScaleWidth = width;
  return *this;
}
auto Plot::Pad::Axis::SetTimeFormat(const string& timeFormat) -> decltype(*this)
{
  mAxisFrame.timeFormat = timeFormat;
  return *this;
}
auto Plot::Pad::Axis::SetTickOrientation(const string& tickOrientation) -> decltype(*this)
{
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
  read_from_tree(boxTree, mPos.isUserCoord, "is_user_coordinates");
  if (auto name = boxTree.get_optional<string>("placement")) {
    mPos.placement = box_placement_from_name(*name);
    if (!mPos.placement) logger::warning("Unknown box placement {} is ignored.", *name);
  }
  read_from_tree(boxTree, mBorder.style, "border_style");
  read_from_tree(boxTree, mBorder.color, "border_color");
  read_from_tree(boxTree, mBorder.alpha, "border_alpha");
  ReadWidthFromTree(boxTree, mBorder.scale, "border_width");
  read_from_tree(boxTree, mFill.style, "fill_style");
  read_from_tree(boxTree, mFill.color, "fill_color");
  read_from_tree(boxTree, mFill.alpha, "fill_alpha");
  read_from_tree(boxTree, mText.style, "text_font");
  read_from_tree(boxTree, mText.color, "text_color");
  read_from_tree(boxTree, mText.alpha, "text_alpha");
  read_from_tree(boxTree, mText.scale, "text_size");
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
  put_in_tree(boxTree, mPos.isUserCoord, "is_user_coordinates");
  if (mPos.placement) boxTree.put("placement", box_placement_name(*mPos.placement));
  put_in_tree(boxTree, mBorder.style, "border_style");
  put_in_tree(boxTree, mBorder.color, "border_color");
  put_in_tree(boxTree, mBorder.alpha, "border_alpha");
  put_in_tree(boxTree, mBorder.scale, "border_width");
  put_in_tree(boxTree, mFill.style, "fill_style");
  put_in_tree(boxTree, mFill.color, "fill_color");
  put_in_tree(boxTree, mFill.alpha, "fill_alpha");
  put_in_tree(boxTree, mText.style, "text_font");
  put_in_tree(boxTree, mText.color, "text_color");
  put_in_tree(boxTree, mText.alpha, "text_alpha");
  put_in_tree(boxTree, mText.scale, "text_size");
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
  mPos.x = x;
  mPos.y = y;
  mPos.placement = nullopt;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetSize(double_t width, double_t height)
{
  mPos.w = width;
  mPos.h = height;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetUserCoordinates(bool isUserCoord)
{
  mPos.isUserCoord = isUserCoord;
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
BoxType& Plot::Pad::Box<BoxType>::SetBorder(color_t color, int16_t style, float_t width, optional<float_t> alpha)
{
  mBorder.color = color;
  mBorder.style = style;
  mBorder.scale = RoundWidth(width, "box border width");
  mBorder.alpha = alpha;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetBorderColor(color_t color)
{
  mBorder.color = color;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetBorderAlpha(float_t alpha)
{
  mBorder.alpha = alpha;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetBorderStyle(int16_t style)
{
  mBorder.style = style;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetBorderWidth(float_t width)
{
  mBorder.scale = RoundWidth(width, "box border width");
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetText(color_t color, int16_t font, float_t size, optional<float_t> alpha)
{
  mText.color = color;
  mText.alpha = alpha;
  mText.style = font;
  mText.scale = size;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTextColor(color_t color)
{
  mText.color = color;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTextAlpha(float_t alpha)
{
  mText.alpha = alpha;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTextFont(int16_t font)
{
  mText.style = font;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTextSize(float_t size)
{
  mText.scale = size;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetFill(color_t color, int16_t style, optional<float_t> alpha)
{
  mFill.color = color;
  mFill.style = style;
  mFill.alpha = alpha;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetFillColor(color_t color)
{
  mFill.color = color;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetFillStyle(int16_t style)
{
  mFill.style = style;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetFillAlpha(float_t alpha)
{
  mFill.alpha = alpha;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTransparent()
{
  mFill.style = 0;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetNoBox()
{
  mFill.style = 0;
  mBorder.scale = 0.f;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetMargin(float_t margin)
{
  mMargin = margin;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetLineSpacing(float_t lineSpacing)
{
  mLineSpacing = lineSpacing;
  return *GetThis();
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
Plot::Pad::TextBox& Plot::Pad::TextBox::SetText(const string& text)
{
  mText = text;
  return *this;
}
Plot::Pad::TextBox& Plot::Pad::TextBox::SetText(const vector<string>& lines)
{
  mText.clear();
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i) mText += "\n";
    mText += lines[i];
  }
  return *this;
}
Plot::Pad::TextBox& Plot::Pad::TextBox::SetText(std::initializer_list<string> lines)
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
Plot::Pad::TextBox& Plot::Pad::TextBox::SetTextAlign(int16_t align)
{
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
  read_from_tree(legendBoxTree, mSymbolElementsDefault, "default_symbol_elements");
  read_from_tree(legendBoxTree, mMarkerDefault.color, "default_marker_color");
  read_from_tree(legendBoxTree, mMarkerDefault.alpha, "default_marker_alpha");
  read_from_tree(legendBoxTree, mMarkerDefault.style, "default_marker_style");
  read_from_tree(legendBoxTree, mMarkerDefault.scale, "default_marker_size");
  read_from_tree(legendBoxTree, mLineDefault.color, "default_line_color");
  read_from_tree(legendBoxTree, mLineDefault.alpha, "default_line_alpha");
  read_from_tree(legendBoxTree, mLineDefault.style, "default_line_style");
  ReadWidthFromTree(legendBoxTree, mLineDefault.scale, "default_line_width");
  read_from_tree(legendBoxTree, mFillDefault.color, "default_fill_color");
  read_from_tree(legendBoxTree, mFillDefault.alpha, "default_fill_alpha");
  read_from_tree(legendBoxTree, mFillDefault.style, "default_fill_style");
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
  put_in_tree(legendBoxTree, mSymbolElementsDefault, "default_symbol_elements");
  put_in_tree(legendBoxTree, mMarkerDefault.color, "default_marker_color");
  put_in_tree(legendBoxTree, mMarkerDefault.alpha, "default_marker_alpha");
  put_in_tree(legendBoxTree, mMarkerDefault.style, "default_marker_style");
  put_in_tree(legendBoxTree, mMarkerDefault.scale, "default_marker_size");
  put_in_tree(legendBoxTree, mLineDefault.color, "default_line_color");
  put_in_tree(legendBoxTree, mLineDefault.alpha, "default_line_alpha");
  put_in_tree(legendBoxTree, mLineDefault.style, "default_line_style");
  put_in_tree(legendBoxTree, mLineDefault.scale, "default_line_width");
  put_in_tree(legendBoxTree, mFillDefault.color, "default_fill_color");
  put_in_tree(legendBoxTree, mFillDefault.alpha, "default_fill_alpha");
  put_in_tree(legendBoxTree, mFillDefault.style, "default_fill_style");
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
  return mLegendEntriesUser.emplace_back(label);
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetTitle(const string& title)
{
  mTitle = title;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetNumColumns(uint8_t numColumns)
{
  if (numColumns) {
    mNumColumns = numColumns;
  } else {
    logger::warning("Legend box cannot have 0 columns.");
  }
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultSymbolElements(const string& symbolElements)
{
  mSymbolElementsDefault = symbolElements;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultColor(color_t color)
{
  mLineDefault.color = color;
  mMarkerDefault.color = color;
  mFillDefault.color = color;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultAlpha(float_t alpha)
{
  mLineDefault.alpha = alpha;
  mMarkerDefault.alpha = alpha;
  mFillDefault.alpha = alpha;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineColor(color_t color)
{
  mLineDefault.color = color;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineAlpha(float_t alpha)
{
  mLineDefault.alpha = alpha;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineStyle(int16_t style)
{
  mLineDefault.style = style;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineWidth(float_t width)
{
  mLineDefault.scale = RoundWidth(width, "legend default line width");
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultMarkerColor(color_t color)
{
  mMarkerDefault.color = color;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultMarkerAlpha(float_t alpha)
{
  mMarkerDefault.alpha = alpha;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultMarkerStyle(int16_t style)
{
  mMarkerDefault.style = style;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultMarkerSize(float_t size)
{
  mMarkerDefault.scale = size;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultFillColor(color_t color)
{
  mFillDefault.color = color;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultFillAlpha(float_t alpha)
{
  mFillDefault.alpha = alpha;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultFillStyle(int16_t style)
{
  mFillDefault.style = style;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetSymbolColScale(float_t scale)
{
  mSymbolColScale = scale;
  return *this;
}

//**************************************************************************************************
/**
 * Add the row of a data item (dataID: its number in the pad), with the appearance the user set for it.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::AddDataEntry(const string& label, uint16_t dataID, const optional<LegendEntry>& appearance)
{
  LegendEntry& entry = mLegendEntries.emplace_back(label, dataID);
  if (appearance) entry += *appearance;
}

//**************************************************************************************************
/**
 * Shift the data the rows refer to, when the pad is added to a pad whose data come first.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::ShiftDataIDs(uint16_t offset)
{
  for (auto& entry : mLegendEntriesUser) {
    if (entry.mRefDataID) *entry.mRefDataID += offset;
    if (entry.mPosition && entry.mPosition != LegendEntry::position_t::top) entry.mPositionDataID += offset;
  }
}

//**************************************************************************************************
/**
 * Put the rows added with AddEntry between the rows of the data (nData: number of data in the pad).
 * The rows of the data are in the order of the data; a row added by the user is at the top, before or after the place of a data item
 * (whether or not it has a row in this legend) or, by default, at the end. Rows at the same place keep the order in which they were added.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::MergeLegendEntries(uint16_t nData)
{
  constexpr int32_t atEnd = std::numeric_limits<int32_t>::max();
  vector<std::pair<std::pair<int32_t, int32_t>, LegendEntry>> rows;  // (data the row is at, before/at/after it), row
  for (const auto& entry : mLegendEntries) {
    rows.push_back({{*entry.mRefDataID, 1}, entry});
  }
  for (const auto& entry : mLegendEntriesUser) {
    std::pair<int32_t, int32_t> place{atEnd, 0};
    if (entry.mPosition == LegendEntry::position_t::top) {
      place = {0, 0};
    } else if (entry.mPosition) {
      if (entry.mPositionDataID < 1 || entry.mPositionDataID > nData) {
        logger::warning("Legend entry '{}' is placed relative to data {}, but the pad has {} data.", entry.mLabel.value_or(""), entry.mPositionDataID, nData);
      }
      place = {entry.mPositionDataID, (entry.mPosition == LegendEntry::position_t::before) ? 0 : 2};
    }
    rows.push_back({place, entry});
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
  read_from_tree(legendEntryTree, mSymbolElements, "symbol_elements");
  if (const auto position = get_from_tree<string>(legendEntryTree, "position")) {
    if (*position == "top") {
      AtTop();
    } else if (const auto dataID = get_from_tree<uint16_t>(legendEntryTree, "position_data")) {
      if (*position == "before") {
        BeforeData(*dataID);
      } else {
        AfterData(*dataID);
      }
    }
  }
  read_from_tree(legendEntryTree, mFill.color, "fill_color");
  read_from_tree(legendEntryTree, mFill.alpha, "fill_alpha");
  read_from_tree(legendEntryTree, mFill.style, "fill_style");
  read_from_tree(legendEntryTree, mLine.color, "line_color");
  read_from_tree(legendEntryTree, mLine.alpha, "line_alpha");
  read_from_tree(legendEntryTree, mLine.style, "line_style");
  ReadWidthFromTree(legendEntryTree, mLine.scale, "line_width");
  read_from_tree(legendEntryTree, mMarker.color, "marker_color");
  read_from_tree(legendEntryTree, mMarker.alpha, "marker_alpha");
  read_from_tree(legendEntryTree, mMarker.style, "marker_style");
  read_from_tree(legendEntryTree, mMarker.scale, "marker_size");
  read_from_tree(legendEntryTree, mText.color, "text_color");
  read_from_tree(legendEntryTree, mText.alpha, "text_alpha");
  read_from_tree(legendEntryTree, mText.style, "text_font");
  read_from_tree(legendEntryTree, mText.scale, "text_size");
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
  put_in_tree(legendEntryTree, mSymbolElements, "symbol_elements");
  if (mPosition) {
    legendEntryTree.put("position", (mPosition == position_t::top) ? "top" : ((mPosition == position_t::before) ? "before" : "after"));
    if (mPosition != position_t::top) legendEntryTree.put("position_data", mPositionDataID);
  }
  put_in_tree(legendEntryTree, mFill.color, "fill_color");
  put_in_tree(legendEntryTree, mFill.style, "fill_style");
  put_in_tree(legendEntryTree, mFill.alpha, "fill_alpha");
  put_in_tree(legendEntryTree, mLine.color, "line_color");
  put_in_tree(legendEntryTree, mLine.alpha, "line_alpha");
  put_in_tree(legendEntryTree, mLine.style, "line_style");
  put_in_tree(legendEntryTree, mLine.scale, "line_width");
  put_in_tree(legendEntryTree, mMarker.color, "marker_color");
  put_in_tree(legendEntryTree, mMarker.alpha, "marker_alpha");
  put_in_tree(legendEntryTree, mMarker.style, "marker_style");
  put_in_tree(legendEntryTree, mMarker.scale, "marker_size");
  put_in_tree(legendEntryTree, mText.color, "text_color");
  put_in_tree(legendEntryTree, mText.alpha, "text_alpha");
  put_in_tree(legendEntryTree, mText.style, "text_font");
  put_in_tree(legendEntryTree, mText.scale, "text_size");
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
  if (legendEntry.mLabel) this->mLabel = legendEntry.mLabel;
  if (legendEntry.mRefDataID) this->mRefDataID = legendEntry.mRefDataID;
  if (legendEntry.mMarker.color) this->mMarker.color = legendEntry.mMarker.color;
  if (legendEntry.mMarker.alpha) this->mMarker.alpha = legendEntry.mMarker.alpha;
  if (legendEntry.mMarker.scale) this->mMarker.scale = legendEntry.mMarker.scale;
  if (legendEntry.mMarker.style) this->mMarker.style = legendEntry.mMarker.style;
  if (legendEntry.mLine.color) this->mLine.color = legendEntry.mLine.color;
  if (legendEntry.mLine.alpha) this->mLine.alpha = legendEntry.mLine.alpha;
  if (legendEntry.mLine.scale) this->mLine.scale = legendEntry.mLine.scale;
  if (legendEntry.mLine.style) this->mLine.style = legendEntry.mLine.style;
  if (legendEntry.mFill.color) this->mFill.color = legendEntry.mFill.color;
  if (legendEntry.mFill.alpha) this->mFill.alpha = legendEntry.mFill.alpha;
  if (legendEntry.mFill.style) this->mFill.style = legendEntry.mFill.style;
  if (legendEntry.mText.color) this->mText.color = legendEntry.mText.color;
  if (legendEntry.mText.alpha) this->mText.alpha = legendEntry.mText.alpha;
  if (legendEntry.mText.scale) this->mText.scale = legendEntry.mText.scale;
  if (legendEntry.mText.style) this->mText.style = legendEntry.mText.style;
}

//**************************************************************************************************
/**
 * User accessors to change legend entry properties.
 */
//**************************************************************************************************

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetLabel(const string& label)
{
  mLabel = label;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetRefData(uint16_t refDataID)
{
  mRefDataID = refDataID;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetSymbolElements(const string& symbolElements)
{
  mSymbolElements = symbolElements;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::AtTop()
{
  mPosition = position_t::top;
  mPositionDataID = 0;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::BeforeData(uint16_t dataID)
{
  mPosition = position_t::before;
  mPositionDataID = dataID;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::AfterData(uint16_t dataID)
{
  mPosition = position_t::after;
  mPositionDataID = dataID;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetColor(color_t color)
{
  mMarker.color = color;
  mLine.color = color;
  mFill.color = color;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetAlpha(float_t alpha)
{
  mMarker.alpha = alpha;
  mLine.alpha = alpha;
  mFill.alpha = alpha;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetMarker(color_t color, int16_t style, float_t size, optional<float_t> alpha)
{
  mMarker.color = color;
  mMarker.alpha = alpha;
  mMarker.style = style;
  mMarker.scale = size;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetMarkerColor(color_t color)
{
  mMarker.color = color;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetMarkerAlpha(float_t alpha)
{
  mMarker.alpha = alpha;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetMarkerStyle(int16_t style)
{
  mMarker.style = style;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetMarkerSize(float_t size)
{
  mMarker.scale = size;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetLine(color_t color, int16_t style, float_t width, optional<float_t> alpha)
{
  mLine.color = color;
  mLine.alpha = alpha;
  mLine.style = style;
  mLine.scale = RoundWidth(width, "legend entry line width");
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetLineColor(color_t color)
{
  mLine.color = color;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetLineAlpha(float_t alpha)
{
  mLine.alpha = alpha;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetLineStyle(int16_t style)
{
  mLine.style = style;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetLineWidth(float_t width)
{
  mLine.scale = RoundWidth(width, "legend entry line width");
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetFill(color_t color, int16_t style, optional<float_t> alpha)
{
  mFill.color = color;
  mFill.alpha = alpha;
  mFill.style = style;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetFillColor(color_t color)
{
  mFill.color = color;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetFillAlpha(float_t alpha)
{
  mFill.alpha = alpha;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetFillStyle(int16_t style)
{
  mFill.style = style;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetTextColor(color_t color)
{
  mText.color = color;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetTextAlpha(float_t alpha)
{
  mText.alpha = alpha;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetTextFont(int16_t font)
{
  mText.style = font;
  return *this;
}

Plot::Pad::LegendEntry& Plot::Pad::LegendEntry::SetTextSize(float_t size)
{
  mText.scale = size;
  return *this;
}

//**************************************************************************************************
// explicitly instantiate required box templates
template class Plot::Pad::Box<Plot::Pad::TextBox>;
template class Plot::Pad::Box<Plot::Pad::StatsBox>;
template class Plot::Pad::Box<Plot::Pad::LegendBox>;
//**************************************************************************************************
}  // end namespace SciRooPlot
