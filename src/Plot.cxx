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

#include "Plot.h"
#include "HelperFunctions.h"
#include "Logging.h"

namespace PlottingFramework
{

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Plot
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Default constructor.
 */
//**************************************************************************************************
Plot::Plot(const string& name, const string& figureGroup, const string& plotTemplateName) : Plot()
{
  if (str_contains(figureGroup, ".")) {
    ERROR("Figure Group must not contain '.'!");
    std::exit(EXIT_FAILURE);
  }
  mName = name;
  mFigureGroup = figureGroup;
  if (plotTemplateName != "") mPlotTemplateName = plotTemplateName;

  // in case category was specified via figureGroup:my/category/tree
  auto subPathPos = figureGroup.find(":");
  if (subPathPos != string::npos) {
    mFigureGroup = figureGroup.substr(0, subPathPos);
    mFigureCategory = figureGroup.substr(subPathPos + 1);
  }
}

//**************************************************************************************************
/**
 * Constructor from existing plot.
 */
//**************************************************************************************************
Plot::Plot(const Plot& otherPlot, const string& name, const string& plotGroup)
{
  *this = otherPlot.Clone();
  this->mName = name;
  this->mFigureGroup = plotGroup;
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
    mFigureGroup = plotTree.get<string>("figureGroup");
    mFigureCategory = plotTree.get<string>("figureCategory");
  } catch (...) {
    ERROR("Could not construct data from ptree.");
  }
  read_from_tree(plotTree, mPlotTemplateName, "plot_template_name");
  read_from_tree(plotTree, mPlotDimensions.width, "width");
  read_from_tree(plotTree, mPlotDimensions.height, "height");
  read_from_tree(plotTree, mPlotDimensions.fixAspectRatio, "fix_aspect_ratio");
  read_from_tree(plotTree, mFill.color, "fill_color");
  read_from_tree(plotTree, mFill.style, "fill_style");

  // loop over all pads defined in property tree
  for (auto& pad : plotTree) {
    if (str_contains(pad.first, "PAD")) {
      uint8_t padID = std::stoi(pad.first.substr(pad.first.find("_") + 1));
      mPads[padID] = Pad(pad.second);
    }
  }
}

//**************************************************************************************************
/**
 * Get representation of plot as property tree.
 */
//**************************************************************************************************
ptree Plot::GetPropertyTree()
{
  ptree plotTree;
  plotTree.put("name", mName);
  plotTree.put("figureGroup", mFigureGroup);
  plotTree.put("figureCategory", mFigureCategory);
  put_in_tree(plotTree, mPlotTemplateName, "plot_template_name");
  put_in_tree(plotTree, mPlotDimensions.width, "width");
  put_in_tree(plotTree, mPlotDimensions.height, "height");
  put_in_tree(plotTree, mPlotDimensions.fixAspectRatio, "fix_aspect_ratio");
  put_in_tree(plotTree, mFill.color, "fill_color");
  put_in_tree(plotTree, mFill.style, "fill_style");

  for (auto& [padID, pad] : mPads) {
    plotTree.put_child("PAD_" + std::to_string(padID), pad.GetPropertyTree());
  }
  return plotTree;
}

//**************************************************************************************************
/**
 * Make a deep copy of the Plot where also data and boxes are copied.
 */
//**************************************************************************************************
Plot Plot::Clone() const
{
  Plot newPlot(*this);
  for (auto& [padID, pad] : newPlot.GetPads()) {
    std::transform(newPlot[padID].GetData().begin(), newPlot[padID].GetData().end(),
                   newPlot[padID].GetData().begin(),
                   [](auto& data_ptr) { return data_ptr->Clone(); });

    std::transform(newPlot[padID].GetLegendBoxes().begin(), newPlot[padID].GetLegendBoxes().end(),
                   newPlot[padID].GetLegendBoxes().begin(), [](auto& legend_ptr) {
                     return std::make_shared<Plot::Pad::LegendBox>(*legend_ptr);
                   });

    std::transform(newPlot[padID].GetTextBoxes().begin(), newPlot[padID].GetTextBoxes().end(),
                   newPlot[padID].GetTextBoxes().begin(),
                   [](auto& text_ptr) { return std::make_shared<Plot::Pad::TextBox>(*text_ptr); });
  }
  return newPlot;
}

//**************************************************************************************************
/**
 * Set fill for this plot.
 */
//**************************************************************************************************
auto Plot::SetFill(int16_t color, int16_t style) -> decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  return *this;
}

//**************************************************************************************************
/**
 * Set this plot transparent.
 */
//**************************************************************************************************
auto Plot::SetTransparent() -> decltype(*this)
{
  mFill.style = 4000;
  return *this;
}

//**************************************************************************************************
/**
 * Apply all settings from plot on top of this plot.
 */
//**************************************************************************************************
void Plot::operator+=(const Plot& plot)
{
  mName = plot.mName;
  mFigureGroup = plot.mFigureGroup;
  mFigureCategory = plot.mFigureCategory;
  mPlotTemplateName = plot.mPlotTemplateName;

  if (plot.mPlotDimensions.width) mPlotDimensions.width = plot.mPlotDimensions.width;
  if (plot.mPlotDimensions.height) mPlotDimensions.height = plot.mPlotDimensions.height;
  if (plot.mPlotDimensions.fixAspectRatio)
    mPlotDimensions.fixAspectRatio = plot.mPlotDimensions.fixAspectRatio;

  if (plot.mFill.color) mFill.color = plot.mFill.color;
  if (plot.mFill.style) mFill.style = plot.mFill.style;

  for (auto& [padID, pad] : plot.mPads) {
    mPads[padID]; // initializes the pad in case it was not yet defined in this plot
    mPads[padID] += pad;
  }
}

//**************************************************************************************************
/**
 * Friend function to get the combination of template and actual plot.
 */
//**************************************************************************************************
Plot operator+(const Plot& templatePlot, const Plot& plot)
{
  Plot combinedPlot = templatePlot;
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
 * Set reference function..
 */
//**************************************************************************************************
auto Plot::Pad::SetRefFunc(const string& refFunc) -> decltype(*this)
{
  mRefFunc = refFunc;
  return *this;
}

//**************************************************************************************************
/**
 * Set text size
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultTextSize(float_t size) -> decltype(*this)
{
  mText.size = size;
  return *this;
}

//**************************************************************************************************
/**
 * Set text color
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultTextColor(int16_t color) -> decltype(*this)
{
  mText.color = color;
  return *this;
}

//**************************************************************************************************
/**
 * Set text font for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultTextFont(int16_t font) -> decltype(*this)
{
  mText.font = font;
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
  mLineDefaults.scale = width;
  return *this;
}

//**************************************************************************************************
/**
 * Set default fill opacity for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultFillOpacity(float_t opacity) -> decltype(*this)
{
  mFillDefaults.scale = opacity;
  return *this;
}

//**************************************************************************************************
/**
 * Set default marker colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultMarkerColors(const vector<int16_t>& colors) -> decltype(*this)
{
  mMarkerDefaults.colors = colors;
  return *this;
}

//**************************************************************************************************
/**
 * Set default line colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultLineColors(const vector<int16_t>& colors) -> decltype(*this)
{
  mLineDefaults.colors = colors;
  return *this;
}

//**************************************************************************************************
/**
 * Set default fill colors.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultFillColors(const vector<int16_t>& colors) -> decltype(*this)
{
  mFillDefaults.colors = colors;
  return *this;
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
 * Set default drawing option for graphs.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultDrawingOptionGraph(drawing_options_t drawingOption) -> decltype(*this)
{
  mDrawingOptionDefaults.graph = drawingOption;
  return *this;
}

//**************************************************************************************************
/**
 * Set default drawing option for histograms.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultDrawingOptionHist(drawing_options_t drawingOption) -> decltype(*this)
{
  mDrawingOptionDefaults.hist = drawingOption;
  return *this;
}

//**************************************************************************************************
/**
 * Set default drawing option for 2d histograms.
 */
//**************************************************************************************************
auto Plot::Pad::SetDefaultDrawingOptionHist2d(drawing_options_t drawingOption) -> decltype(*this)
{
  mDrawingOptionDefaults.hist2d = drawingOption;
  return *this;
}

//**************************************************************************************************
/**
 * Set fill for this pad.
 */
//**************************************************************************************************
auto Plot::Pad::SetFill(int16_t color, int16_t style) -> decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  return *this;
}

//**************************************************************************************************
/**
 * Make this pad transparent.
 */
//**************************************************************************************************
auto Plot::Pad::SetTransparent() -> decltype(*this)
{
  mFill.style = 4000;
  return *this;
}

//**************************************************************************************************
/**
 * Set frame fill properties.
 */
//**************************************************************************************************
auto Plot::Pad::SetFillFrame(int16_t color, int16_t style) -> decltype(*this)
{
  mFrame.fillColor = color;
  mFrame.fillStyle = style;
  return *this;
}

//**************************************************************************************************
/**
 * Set frame line properties.
 */
//**************************************************************************************************
auto Plot::Pad::SetLineFrame(int16_t color, int16_t style, float_t width) -> decltype(*this)
{
  mFrame.lineColor = color;
  mFrame.lineStyle = style;
  mFrame.lineWidth = width;
  return *this;
}

//**************************************************************************************************
/**
 * Set this pad transparent.
 */
//**************************************************************************************************
auto Plot::Pad::SetTransparentFrame() -> decltype(*this)
{
  mFrame.fillStyle = 0;
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
  mPalette = palette;
  return *this;
}

//**************************************************************************************************
/**
 * Set title of pad
 */
//**************************************************************************************************
auto Plot::Pad::SetTitle(const string& title) -> decltype(*this)
{
  mTitle = title;
  return *this;
}

//**************************************************************************************************
/**
 * Set position of pad within plot. The variables are given in fractions of plot width resp. height
 * (between 0 and 1). The lower left corner is defined by (xlow, ylow) and the upper right corner by
 * (xup, yup).
 */
//**************************************************************************************************
auto Plot::Pad::SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup)
  -> decltype(*this)
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
auto Plot::Pad::SetMargins(float_t top, float_t bottom, float_t left, float_t right)
  -> decltype(*this)
{
  mMargins.top = top;
  mMargins.bottom = bottom;
  mMargins.left = left;
  mMargins.right = right;
  return *this;
}

//**************************************************************************************************
/**
 * Constructor from property tree.
 */
//**************************************************************************************************
Plot::Pad::Pad(const ptree& padTree)
{
  read_from_tree(padTree, mTitle, "title");
  read_from_tree(padTree, mOptions, "options");
  read_from_tree(padTree, mPosition.xlow, "position_xlow");
  read_from_tree(padTree, mPosition.ylow, "position_ylow");
  read_from_tree(padTree, mPosition.xup, "position_xup");
  read_from_tree(padTree, mPosition.yup, "position_yup");
  read_from_tree(padTree, mMargins.top, "margins_top");
  read_from_tree(padTree, mMargins.bottom, "margins_bottom");
  read_from_tree(padTree, mMargins.left, "margins_left");
  read_from_tree(padTree, mMargins.right, "margins_right");
  read_from_tree(padTree, mPalette, "palette");
  read_from_tree(padTree, mFill.color, "fill_color");
  read_from_tree(padTree, mFill.style, "fill_style");
  read_from_tree(padTree, mFrame.fillColor, "frame_fill_color");
  read_from_tree(padTree, mFrame.fillStyle, "frame_fill_style");
  read_from_tree(padTree, mFrame.lineColor, "frame_line_color");
  read_from_tree(padTree, mFrame.lineStyle, "frame_line_style");
  read_from_tree(padTree, mFrame.lineWidth, "frame_line_width");
  read_from_tree(padTree, mText.font, "text_font");
  read_from_tree(padTree, mText.color, "text_color");
  read_from_tree(padTree, mText.size, "text_size");
  read_from_tree(padTree, mMarkerDefaults.scale, "default_marker_size");
  read_from_tree(padTree, mLineDefaults.scale, "default_line_width");
  read_from_tree(padTree, mFillDefaults.scale, "default_fill_opacity");
  read_from_tree(padTree, mMarkerDefaults.colors, "default_marker_colors");
  read_from_tree(padTree, mLineDefaults.colors, "default_line_colors");
  read_from_tree(padTree, mFillDefaults.colors, "default_fill_colors");
  read_from_tree(padTree, mMarkerDefaults.styles, "default_marker_styles");
  read_from_tree(padTree, mLineDefaults.styles, "default_line_styles");
  read_from_tree(padTree, mFillDefaults.styles, "default_fill_styles");
  read_from_tree(padTree, mDrawingOptionDefaults.graph, "default_drawing_option_graph");
  read_from_tree(padTree, mDrawingOptionDefaults.hist, "default_drawing_option_hist");
  read_from_tree(padTree, mDrawingOptionDefaults.hist2d, "default_drawing_option_hist2d");
  read_from_tree(padTree, mRedrawAxes, "redraw_axes");
  read_from_tree(padTree, mRefFunc, "ref_func");

  for (auto& content : padTree) {
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
  put_in_tree(padTree, mTitle, "title");
  put_in_tree(padTree, mOptions, "options");
  put_in_tree(padTree, mPosition.xlow, "position_xlow");
  put_in_tree(padTree, mPosition.ylow, "position_ylow");
  put_in_tree(padTree, mPosition.xup, "position_xup");
  put_in_tree(padTree, mPosition.yup, "position_yup");
  put_in_tree(padTree, mMargins.top, "margins_top");
  put_in_tree(padTree, mMargins.bottom, "margins_bottom");
  put_in_tree(padTree, mMargins.left, "margins_left");
  put_in_tree(padTree, mMargins.right, "margins_right");
  put_in_tree(padTree, mPalette, "palette");
  put_in_tree(padTree, mFill.color, "fill_color");
  put_in_tree(padTree, mFill.style, "fill_style");
  put_in_tree(padTree, mFrame.fillColor, "frame_fill_color");
  put_in_tree(padTree, mFrame.fillStyle, "frame_fill_style");
  put_in_tree(padTree, mFrame.lineColor, "frame_line_color");
  put_in_tree(padTree, mFrame.lineStyle, "frame_line_style");
  put_in_tree(padTree, mFrame.lineWidth, "frame_line_width");
  put_in_tree(padTree, mText.font, "text_font");
  put_in_tree(padTree, mText.color, "text_color");
  put_in_tree(padTree, mText.size, "text_size");
  put_in_tree(padTree, mMarkerDefaults.scale, "default_marker_size");
  put_in_tree(padTree, mLineDefaults.scale, "default_line_width");
  put_in_tree(padTree, mFillDefaults.scale, "default_fill_opacity");
  put_in_tree(padTree, mMarkerDefaults.colors, "default_marker_colors");
  put_in_tree(padTree, mLineDefaults.colors, "default_line_colors");
  put_in_tree(padTree, mFillDefaults.colors, "default_fill_colors");
  put_in_tree(padTree, mMarkerDefaults.styles, "default_marker_styles");
  put_in_tree(padTree, mLineDefaults.styles, "default_line_styles");
  put_in_tree(padTree, mFillDefaults.styles, "default_fill_styles");
  put_in_tree(padTree, mDrawingOptionDefaults.graph, "default_drawing_option_graph");
  put_in_tree(padTree, mDrawingOptionDefaults.hist, "default_drawing_option_hist");
  put_in_tree(padTree, mDrawingOptionDefaults.hist2d, "default_drawing_option_hist2d");
  put_in_tree(padTree, mRedrawAxes, "redraw_axes");
  put_in_tree(padTree, mRefFunc, "ref_func");

  int dataID = 1;
  for (auto& data : mData) {
    padTree.put_child("DATA_" + std::to_string(dataID), data->GetPropertyTree());
    ++dataID;
  }

  int legendBoxID = 1;
  for (auto& legendBox : mLegendBoxes) {
    padTree.put_child("LEGEND_" + std::to_string(legendBoxID), legendBox->GetPropertyTree());
    ++legendBoxID;
  }
  int textBoxID = 1;
  for (auto& textBox : mTextBoxes) {
    padTree.put_child("TEXT_" + std::to_string(textBoxID), textBox->GetPropertyTree());
    ++textBoxID;
  }

  for (auto& axis : {'X', 'Y', 'Z'}) {
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
  if (pad.mTitle) mTitle = pad.mTitle;
  if (pad.mOptions) mOptions = pad.mOptions;
  if (pad.mPosition.xlow) mPosition.xlow = pad.mPosition.xlow;
  if (pad.mPosition.ylow) mPosition.ylow = pad.mPosition.ylow;
  if (pad.mPosition.xup) mPosition.xup = pad.mPosition.xup;
  if (pad.mPosition.yup) mPosition.yup = pad.mPosition.yup;
  if (pad.mMargins.top) mMargins.top = pad.mMargins.top;
  if (pad.mMargins.bottom) mMargins.bottom = pad.mMargins.bottom;
  if (pad.mMargins.left) mMargins.left = pad.mMargins.left;
  if (pad.mMargins.right) mMargins.right = pad.mMargins.right;
  if (pad.mFill.color) mFill.color = pad.mFill.color;
  if (pad.mFill.style) mFill.style = pad.mFill.style;
  if (pad.mFrame.fillColor) mFrame.fillColor = pad.mFrame.fillColor;
  if (pad.mFrame.fillStyle) mFrame.fillStyle = pad.mFrame.fillStyle;
  if (pad.mFrame.lineColor) mFrame.lineColor = pad.mFrame.lineColor;
  if (pad.mFrame.lineStyle) mFrame.lineStyle = pad.mFrame.lineStyle;
  if (pad.mFrame.lineWidth) mFrame.lineWidth = pad.mFrame.lineWidth;
  if (pad.mText.size) mText.size = pad.mText.size;
  if (pad.mText.font) mText.font = pad.mText.font;
  if (pad.mText.color) mText.color = pad.mText.color;
  if (pad.mMarkerDefaults.scale) mMarkerDefaults.scale = pad.mMarkerDefaults.scale;
  if (pad.mLineDefaults.scale) mLineDefaults.scale = pad.mLineDefaults.scale;
  if (pad.mFillDefaults.scale) mFillDefaults.scale = pad.mFillDefaults.scale;
  if (pad.mMarkerDefaults.colors) mMarkerDefaults.colors = pad.mMarkerDefaults.colors;
  if (pad.mLineDefaults.colors) mLineDefaults.colors = pad.mLineDefaults.colors;
  if (pad.mFillDefaults.colors) mFillDefaults.colors = pad.mFillDefaults.colors;
  if (pad.mMarkerDefaults.styles) mMarkerDefaults.styles = pad.mMarkerDefaults.styles;
  if (pad.mLineDefaults.styles) mLineDefaults.styles = pad.mLineDefaults.styles;
  if (pad.mFillDefaults.styles) mFillDefaults.styles = pad.mFillDefaults.styles;
  if (pad.mDrawingOptionDefaults.graph) mDrawingOptionDefaults.graph = pad.mDrawingOptionDefaults.graph;
  if (pad.mDrawingOptionDefaults.hist) mDrawingOptionDefaults.hist = pad.mDrawingOptionDefaults.hist;
  if (pad.mDrawingOptionDefaults.hist2d) mDrawingOptionDefaults.hist2d = pad.mDrawingOptionDefaults.hist2d;
  if (pad.mPalette) mPalette = pad.mPalette;
  if (pad.mRedrawAxes) mRedrawAxes = pad.mRedrawAxes;
  if (pad.mRefFunc) mRefFunc = pad.mRefFunc;
  for (auto& [axisLable, axis] : pad.mAxes) {
    mAxes[axisLable]; // default initiialize in case this axis was not yet defined
    mAxes[axisLable] += axis;
  }
  mData = pad.mData; // this does not copy the data (!!)
  mLegendBoxes = pad.mLegendBoxes;
  mTextBoxes = pad.mTextBoxes;
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
 * Access function for pad axis.
 */
//**************************************************************************************************
Plot::Pad::Axis& Plot::Pad::GetAxis(const char axis)
{
  const vector<char> allowedAxes = {'X', 'Y', 'Z'};
  if (std::find(allowedAxes.begin(), allowedAxes.end(), axis) == allowedAxes.end()) {
    ERROR("Axis \"{}\" is not allowed! Please use 'X', 'Y' or 'Z'.", axis);
    std::exit(EXIT_FAILURE);
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
    ERROR("Data with ID {} is not defined! You can access only data that was already added to the pad.", dataID);
    std::exit(EXIT_FAILURE);
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
    ERROR("Legend with ID {} is not defined! You can access only legends that have already been added to the pad.", legendID);
    std::exit(EXIT_FAILURE);
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
    ERROR("Text with ID {} is not defined! You can access only texts that have already been added to the pad.", textID);
    std::exit(EXIT_FAILURE);
  }
  return *mTextBoxes[textID - 1];
}

//**************************************************************************************************
/**
 * Add data to this pad.
 */
//**************************************************************************************************
Plot::Pad::Data& Plot::Pad::AddData(const string& name, const string& inputIdentifier, const string& lable)
{
  mData.push_back(std::make_shared<Data>(name, inputIdentifier, lable));
  return *mData.back();
}

Plot::Pad::Data& Plot::Pad::AddData(const string& name, const Data& data, const string& lable)
{
  mData.push_back(std::make_shared<Data>(name, data.GetInputID(), lable));
  mData.back()->SetLayout(data);
  return *mData.back();
}
/*
Plot::Pad::Data& Plot::Pad::AddData(const Data& data, const string& lable)
{
  mData.push_back(std::make_shared<Data>(data.GetName(), data.GetInputID(), lable));
  mData.back()->SetLayout(data);
  return *mData.back();
}

Plot::Pad::Data& Plot::Pad::AddData(const Data& data)
{
  mData.push_back(std::make_shared<Data>(data));
  return *mData.back();
}
*/

//**************************************************************************************************
/**
 * Add ratio to this pad.
 */
//**************************************************************************************************
Plot::Pad::Ratio& Plot::Pad::AddRatio(const string& numeratorName, const string& numeratorInputIdentifier, const string& denominatorName, const string& denominatorInputIdentifier, const string& lable)
{
  mData.push_back(std::make_shared<Ratio>(numeratorName, numeratorInputIdentifier,
                                          denominatorName, denominatorInputIdentifier, lable));
  return *std::dynamic_pointer_cast<Ratio>(mData.back());
}

Plot::Pad::Ratio& Plot::Pad::AddRatio(const string& numeratorName, const Data& data, const string& denominatorName, const string& denominatorInputIdentifier, const string& lable)
{
  mData.push_back(std::make_shared<Ratio>(numeratorName, data.GetInputID(),
                                          denominatorName, denominatorInputIdentifier, lable));
  mData.back()->SetLayout(data);
  return *std::dynamic_pointer_cast<Ratio>(mData.back());
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

//**************************************************************************************************
/**
 * Add legend box to this pad.
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::AddLegend(double_t xPos, double_t yPos)
{
  mLegendBoxes.push_back(std::make_shared<LegendBox>(xPos, yPos));
  return *mLegendBoxes.back();
}

//**************************************************************************************************
/**
 * Add legend box to this pad and automatically find a position for it.
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::AddLegend()
{
  mLegendBoxes.push_back(std::make_shared<LegendBox>());
  return *mLegendBoxes.back();
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
Plot::Pad::Data::Data(const string& name, const string& inputIdentifier, const string& legendLable)
  : Data()
{
  mType = "data";

  if (legendLable != "") mLegend.lable = legendLable;
  // in case input was specified further via inputIdentifier:some/path/in/file
  auto subPathPos = inputIdentifier.find(":");
  if (subPathPos != string::npos) {
    // prepend path to plot name
    mName = inputIdentifier.substr(subPathPos + 1) + "/" + name;
    mInputIdentifier = inputIdentifier.substr(0, subPathPos);
  } else {
    mName = name;
    mInputIdentifier = inputIdentifier;
  }
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
    mInputIdentifier = dataTree.get<string>("inputIdentifier");
  } catch (...) {
    ERROR("Could not construct data from ptree.");
    std::exit(EXIT_FAILURE);
  }
  if (auto var = dataTree.get_optional<bool>("defines_frame")) mDefinesFrame = *var;
  read_from_tree(dataTree, mLegend.lable, "legend_lable");
  read_from_tree(dataTree, mLegend.identifier, "legend_id");
  read_from_tree(dataTree, mDrawingOptions, "drawing_options");
  read_from_tree(dataTree, mDrawingOptionAlias, "drawing_option_alias");
  read_from_tree(dataTree, mTextFormat, "text_format");
  read_from_tree(dataTree, mMarker.color, "marker_color");
  read_from_tree(dataTree, mMarker.style, "marker_style");
  read_from_tree(dataTree, mMarker.scale, "marker_size");
  read_from_tree(dataTree, mLine.color, "line_color");
  read_from_tree(dataTree, mLine.style, "line_style");
  read_from_tree(dataTree, mLine.scale, "line_width");
  read_from_tree(dataTree, mFill.color, "fill_color");
  read_from_tree(dataTree, mFill.style, "fill_style");
  read_from_tree(dataTree, mFill.scale, "fill_opacity");
  read_from_tree(dataTree, mModify.scale_factor, "scale_factor");
  read_from_tree(dataTree, mModify.norm_mode, "norm_mode");
  read_from_tree(dataTree, mRangeX.min, "rangeX_min");
  read_from_tree(dataTree, mRangeX.max, "rangeX_max");
  read_from_tree(dataTree, mRangeY.min, "rangeY_min");
  read_from_tree(dataTree, mRangeY.max, "rangeY_max");
  read_from_tree(dataTree, mContours, "contours");
  read_from_tree(dataTree, mNContours, "number_of_contours");
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
  dataTree.put("inputIdentifier", mInputIdentifier);
  if (mDefinesFrame) dataTree.put("defines_frame", mDefinesFrame);
  put_in_tree(dataTree, mLegend.lable, "legend_lable");
  put_in_tree(dataTree, mLegend.identifier, "legend_id");
  put_in_tree(dataTree, mDrawingOptions, "drawing_options");
  put_in_tree(dataTree, mDrawingOptionAlias, "drawing_option_alias");
  put_in_tree(dataTree, mTextFormat, "text_format");
  put_in_tree(dataTree, mMarker.color, "marker_color");
  put_in_tree(dataTree, mMarker.style, "marker_style");
  put_in_tree(dataTree, mMarker.scale, "marker_size");
  put_in_tree(dataTree, mLine.color, "line_color");
  put_in_tree(dataTree, mLine.style, "line_style");
  put_in_tree(dataTree, mLine.scale, "line_width");
  put_in_tree(dataTree, mFill.color, "fill_color");
  put_in_tree(dataTree, mFill.style, "fill_style");
  put_in_tree(dataTree, mFill.scale, "fill_opacity");
  put_in_tree(dataTree, mModify.scale_factor, "scale_factor");
  put_in_tree(dataTree, mModify.norm_mode, "norm_mode");
  put_in_tree(dataTree, mRangeX.min, "rangeX_min");
  put_in_tree(dataTree, mRangeX.max, "rangeX_max");
  put_in_tree(dataTree, mRangeY.min, "rangeY_min");
  put_in_tree(dataTree, mRangeY.max, "rangeY_max");
  put_in_tree(dataTree, mContours, "contours");
  put_in_tree(dataTree, mNContours, "number_of_contours");
  return dataTree;
}

//**************************************************************************************************
/**
 * User accessors for Data properties.
 */
//**************************************************************************************************
auto Plot::Pad::Data::SetLayout(const Data& dataLayout) -> decltype(*this)
{
  // apply all properties related to the appearence of the data
  mDrawingOptions = dataLayout.mDrawingOptions;
  mDrawingOptionAlias = dataLayout.mDrawingOptionAlias;
  mTextFormat = dataLayout.mTextFormat;
  mRangeX.min = dataLayout.mRangeX.min;
  mRangeX.max = dataLayout.mRangeX.max;
  mRangeY.min = dataLayout.mRangeY.min;
  mRangeY.max = dataLayout.mRangeY.max;
  mMarker.color = dataLayout.mMarker.color;
  mMarker.style = dataLayout.mMarker.style;
  mMarker.scale = dataLayout.mMarker.scale;
  mLine.color = dataLayout.mLine.color;
  mLine.style = dataLayout.mLine.style;
  mLine.scale = dataLayout.mLine.scale;
  mFill.color = dataLayout.mFill.color;
  mFill.style = dataLayout.mFill.style;
  mFill.scale = dataLayout.mFill.scale;
  mContours = dataLayout.mContours;
  mNContours = dataLayout.mNContours;
  return *this;
}
auto Plot::Pad::Data::SetInputID(const string& inputIdentifier) -> decltype(*this)
{
  mInputIdentifier = inputIdentifier;
  return *this;
}
auto Plot::Pad::Data::SetLegendLable(const string& legendLable) -> decltype(*this)
{
  mLegend.lable = legendLable;
  return *this;
}
auto Plot::Pad::Data::SetLegendID(uint8_t legendID) -> decltype(*this)
{
  mLegend.identifier = legendID;
  return *this;
}
auto Plot::Pad::Data::SetOptions(const string& opions) -> decltype(*this)
{
  mDrawingOptions = opions;
  return *this;
}
auto Plot::Pad::Data::SetOptions(drawing_options_t optionAlias) -> decltype(*this)
{
  mDrawingOptionAlias = optionAlias;
  return *this;
}
auto Plot::Pad::Data::UnsetOptions() -> decltype(*this)
{
  mDrawingOptionAlias = {};
  mDrawingOptions = {};
  return *this;
}
auto Plot::Pad::Data::SetTextFormat(const string& textFormat) -> decltype(*this)
{
  mTextFormat = textFormat;
  return *this;
}
auto Plot::Pad::Data::SetScaleFactor(double_t scale) -> decltype(*this)
{
  mModify.scale_factor = scale;
  return *this;
}
auto Plot::Pad::Data::SetNormalize(bool useWidth) -> decltype(*this)
{
  mModify.norm_mode = useWidth;
  return *this;
}
auto Plot::Pad::Data::SetRangeX(double_t min, double_t max) -> decltype(*this)
{
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
auto Plot::Pad::Data::UnsetRangeX() -> decltype(*this)
{
  mRangeX.min = {};
  mRangeX.max = {};
  return *this;
}
auto Plot::Pad::Data::SetRangeY(double_t min, double_t max) -> decltype(*this)
{
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
auto Plot::Pad::Data::UnsetRangeY() -> decltype(*this)
{
  mRangeY.min = {};
  mRangeY.max = {};
  return *this;
}
auto Plot::Pad::Data::SetColor(int16_t color) -> decltype(*this)
{
  mMarker.color = color;
  mLine.color = color;
  mFill.color = color;
  return *this;
}
auto Plot::Pad::Data::SetMarker(int16_t color, int16_t style, float_t size) -> decltype(*this)
{
  mMarker.color = color;
  mMarker.style = style;
  mMarker.scale = size;
  return *this;
}
auto Plot::Pad::Data::SetMarkerColor(int16_t color) -> decltype(*this)
{
  mMarker.color = color;
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
auto Plot::Pad::Data::SetLine(int16_t color, int16_t style, float_t width) -> decltype(*this)
{
  mLine.color = color;
  mLine.style = style;
  mLine.scale = width;
  return *this;
}
auto Plot::Pad::Data::SetLineColor(int16_t color) -> decltype(*this)
{
  mLine.color = color;
  return *this;
}
auto Plot::Pad::Data::SetLineStyle(int16_t style) -> decltype(*this)
{
  mLine.style = style;
  return *this;
}
auto Plot::Pad::Data::SetLineWidth(float_t width) -> decltype(*this)
{
  mLine.scale = width;
  return *this;
}
auto Plot::Pad::Data::SetFill(int16_t color, int16_t style, float_t opacity) -> decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  SetFillOpacity(opacity);
  return *this;
}
auto Plot::Pad::Data::SetFillColor(int16_t color) -> decltype(*this)
{
  mFill.color = color;
  return *this;
}
auto Plot::Pad::Data::SetFillStyle(int16_t style) -> decltype(*this)
{
  mFill.style = style;
  return *this;
}
auto Plot::Pad::Data::SetFillOpacity(float_t opacity) -> decltype(*this)
{
  if (opacity < 0.f || opacity > 1.f) {
    WARNING(
      "Illegal value for opacity! It has to be between 0 (transparent) and 1 (fully opaque).");
  } else {
    mFill.scale = opacity;
  }
  return *this;
}
auto Plot::Pad::Data::SetDefinesFrame() -> decltype(*this)
{
  mDefinesFrame = true;
  return *this;
}
auto Plot::Pad::Data::SetContours(const vector<double>& contours) -> decltype(*this)
{
  mContours = contours;
  return *this;
}
auto Plot::Pad::Data::SetContours(const int32_t nContours) -> decltype(*this)
{
  mNContours = nContours;
  return *this;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// IMPLEMENTATION class Ratio
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

//**************************************************************************************************
/**
 * Default constructor.
 */
//**************************************************************************************************
Plot::Pad::Ratio::Ratio(const string& name, const string& inputIdentifier, const string& denomName,
                        const string& denomInputIdentifier, const string& lable)
  : Data(name, inputIdentifier, lable), mDenomName(denomName),
    mDenomInputIdentifier(denomInputIdentifier), mIsCorrelated(false)
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
    mDenomInputIdentifier = dataTree.get<string>("denomInputID");
    mIsCorrelated = dataTree.get<bool>("isCorrelated");
  } catch (...) {
    ERROR("Could not construct ratio from ptree.");
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
  dataTree.put("denomInputID", mDenomInputIdentifier);
  dataTree.put("isCorrelated", mIsCorrelated);
  return dataTree;
}

//**************************************************************************************************
/**
 * Specify how the division should be done (spline, biniomial, etc...).
 */
//**************************************************************************************************
auto Plot::Pad::Ratio::SetIsCorrelated(bool isCorrelated) -> decltype(*this)
{
  mIsCorrelated = isCorrelated;
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
  } catch (...) {
    ERROR("Could not construct axis from ptree.");
  }
  read_from_tree(axisTree, mTitle, "title");
  read_from_tree(axisTree, mRange.min, "range_min");
  read_from_tree(axisTree, mRange.max, "range_max");
  read_from_tree(axisTree, mNumDivisions, "num_divisions");
  read_from_tree(axisTree, mMaxDigits, "max_digits");
  read_from_tree(axisTree, mTickLength, "tick_length");
  read_from_tree(axisTree, mAxisColor, "axis_color");
  read_from_tree(axisTree, mTitleProperties.font, "title_font");
  read_from_tree(axisTree, mTitleProperties.size, "title_size");
  read_from_tree(axisTree, mTitleProperties.color, "title_color");
  read_from_tree(axisTree, mTitleProperties.offset, "title_offset");
  read_from_tree(axisTree, mTitleProperties.center, "title_center");
  read_from_tree(axisTree, mLableProperties.font, "lable_font");
  read_from_tree(axisTree, mLableProperties.size, "lable_size");
  read_from_tree(axisTree, mLableProperties.color, "lable_color");
  read_from_tree(axisTree, mLableProperties.offset, "lable_offset");
  read_from_tree(axisTree, mLableProperties.center, "lable_center");
  read_from_tree(axisTree, mIsLog, "is_log");
  read_from_tree(axisTree, mIsGrid, "is_grid");
  read_from_tree(axisTree, mIsOppositeTicks, "is_opposite_ticks");
  read_from_tree(axisTree, mTickOrientation, "tick_orientation");
  read_from_tree(axisTree, mTimeFormat, "time_format");
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
  put_in_tree(axisTree, mTitle, "title");
  put_in_tree(axisTree, mRange.min, "range_min");
  put_in_tree(axisTree, mRange.max, "range_max");
  put_in_tree(axisTree, mNumDivisions, "num_divisions");
  put_in_tree(axisTree, mMaxDigits, "max_digits");
  put_in_tree(axisTree, mTickLength, "tick_length");
  put_in_tree(axisTree, mAxisColor, "axis_color");
  put_in_tree(axisTree, mTitleProperties.font, "title_font");
  put_in_tree(axisTree, mTitleProperties.size, "title_size");
  put_in_tree(axisTree, mTitleProperties.color, "title_color");
  put_in_tree(axisTree, mTitleProperties.offset, "title_offset");
  put_in_tree(axisTree, mTitleProperties.center, "title_center");
  put_in_tree(axisTree, mLableProperties.font, "lable_font");
  put_in_tree(axisTree, mLableProperties.size, "lable_size");
  put_in_tree(axisTree, mLableProperties.color, "lable_color");
  put_in_tree(axisTree, mLableProperties.offset, "lable_offset");
  put_in_tree(axisTree, mLableProperties.center, "lable_center");
  put_in_tree(axisTree, mIsLog, "is_log");
  put_in_tree(axisTree, mIsGrid, "is_grid");
  put_in_tree(axisTree, mIsOppositeTicks, "is_opposite_ticks");
  put_in_tree(axisTree, mTickOrientation, "tick_orientation");
  put_in_tree(axisTree, mTimeFormat, "time_format");

  return axisTree;
}

//**************************************************************************************************
/**
 * Apply all settings from axis on top of this axis.
 */
//**************************************************************************************************
void Plot::Pad::Axis::Axis::operator+=(const Axis& axis)
{
  mName = axis.mName;
  if (axis.mRange.min) mRange.min = axis.mRange.min;
  if (axis.mRange.max) mRange.max = axis.mRange.max;
  if (axis.mTitle) mTitle = axis.mTitle;
  if (axis.mNumDivisions) mNumDivisions = axis.mNumDivisions;
  if (axis.mMaxDigits) mMaxDigits = axis.mMaxDigits;
  if (axis.mTickLength) mTickLength = axis.mTickLength;
  if (axis.mAxisColor) mAxisColor = axis.mAxisColor;
  if (axis.mIsLog) mIsLog = axis.mIsLog;
  if (axis.mIsGrid) mIsGrid = axis.mIsGrid;
  if (axis.mIsOppositeTicks) mIsOppositeTicks = axis.mIsOppositeTicks;
  if (axis.mTitleProperties.font) mTitleProperties.font = axis.mTitleProperties.font;
  if (axis.mTitleProperties.size) mTitleProperties.size = axis.mTitleProperties.size;
  if (axis.mTitleProperties.color) mTitleProperties.color = axis.mTitleProperties.color;
  if (axis.mTitleProperties.offset) mTitleProperties.offset = axis.mTitleProperties.offset;
  if (axis.mTitleProperties.center) mTitleProperties.center = axis.mTitleProperties.center;
  if (axis.mLableProperties.font) mLableProperties.font = axis.mLableProperties.font;
  if (axis.mLableProperties.size) mLableProperties.size = axis.mLableProperties.size;
  if (axis.mLableProperties.color) mLableProperties.color = axis.mLableProperties.color;
  if (axis.mLableProperties.offset) mLableProperties.offset = axis.mLableProperties.offset;
  if (axis.mLableProperties.center) mLableProperties.center = axis.mLableProperties.center;
  if (axis.mTimeFormat) mTimeFormat = axis.mTimeFormat;
  if (axis.mTickOrientation) mTickOrientation = axis.mTickOrientation;
}

//**************************************************************************************************
/**
 * User accessors to change axis properties.
 */
//**************************************************************************************************
auto Plot::Pad::Axis::SetTitle(const string& title) -> decltype(*this)
{
  mTitle = title;
  return *this;
}
auto Plot::Pad::Axis::SetRange(double_t min, double_t max) -> decltype(*this)
{
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
auto Plot::Pad::Axis::SetColor(int16_t color) -> decltype(*this)
{
  mAxisColor = color;
  mLableProperties.color = color;
  mTitleProperties.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetAxisColor(int16_t color) -> decltype(*this)
{
  mAxisColor = color;
  return *this;
}
auto Plot::Pad::Axis::SetNumDivisions(int32_t numDivisions) -> decltype(*this)
{
  mNumDivisions = numDivisions;
  return *this;
}
auto Plot::Pad::Axis::SetMaxDigits(int32_t maxDigtis) -> decltype(*this)
{
  mMaxDigits = maxDigtis;
  return *this;
}
auto Plot::Pad::Axis::SetTickLength(float_t tickLength) -> decltype(*this)
{
  mTickLength = tickLength;
  return *this;
}
auto Plot::Pad::Axis::SetTitleFont(int16_t font) -> decltype(*this)
{
  mTitleProperties.font = font;
  return *this;
}
auto Plot::Pad::Axis::SetLableFont(int16_t font) -> decltype(*this)
{
  mLableProperties.font = font;
  return *this;
}
auto Plot::Pad::Axis::SetTitleSize(float_t size) -> decltype(*this)
{
  mTitleProperties.size = size;
  return *this;
}
auto Plot::Pad::Axis::SetLableSize(float_t size) -> decltype(*this)
{
  mLableProperties.size = size;
  return *this;
}
auto Plot::Pad::Axis::SetTitleColor(int16_t color) -> decltype(*this)
{
  mTitleProperties.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetLableColor(int16_t color) -> decltype(*this)
{
  mLableProperties.color = color;
  return *this;
}
auto Plot::Pad::Axis::SetTitleOffset(float_t offset) -> decltype(*this)
{
  mTitleProperties.offset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetLableOffset(float_t offset) -> decltype(*this)
{
  mLableProperties.offset = offset;
  return *this;
}
auto Plot::Pad::Axis::SetTitleCenter(bool center) -> decltype(*this)
{
  mTitleProperties.center = center;
  return *this;
}
auto Plot::Pad::Axis::SetLableCenter(bool center) -> decltype(*this)
{
  mLableProperties.center = center;
  return *this;
}
auto Plot::Pad::Axis::SetLog(bool isLog) -> decltype(*this)
{
  mIsLog = isLog;
  return *this;
}
auto Plot::Pad::Axis::SetGrid(bool isGrid) -> decltype(*this)
{
  mIsGrid = isGrid;
  return *this;
}
auto Plot::Pad::Axis::SetOppositeTicks(bool isOppositeTicks) -> decltype(*this)
{
  mIsOppositeTicks = isOppositeTicks;
  return *this;
}
auto Plot::Pad::Axis::SetTimeFormat(const string& timeFormat) -> decltype(*this)
{
  mTimeFormat = timeFormat;
  return *this;
}
auto Plot::Pad::Axis::SetTickOrientation(const string& tickOrientation) -> decltype(*this)
{
  mTickOrientation = tickOrientation;
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
  read_from_tree(boxTree, mPos.isUserCoord, "is_user_coordinates");
  read_from_tree(boxTree, mBorder.style, "border_style");
  read_from_tree(boxTree, mBorder.color, "border_color");
  read_from_tree(boxTree, mBorder.scale, "border_width");
  read_from_tree(boxTree, mFill.style, "fill_style");
  read_from_tree(boxTree, mFill.color, "fill_color");
  read_from_tree(boxTree, mFill.scale, "fill_opacity");
  read_from_tree(boxTree, mText.style, "text_style");
  read_from_tree(boxTree, mText.color, "text_color");
  read_from_tree(boxTree, mText.scale, "text_size");
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
  put_in_tree(boxTree, mPos.isUserCoord, "is_user_coordinates");
  put_in_tree(boxTree, mBorder.style, "border_style");
  put_in_tree(boxTree, mBorder.color, "border_color");
  put_in_tree(boxTree, mBorder.scale, "border_width");
  put_in_tree(boxTree, mFill.style, "fill_style");
  put_in_tree(boxTree, mFill.color, "fill_color");
  put_in_tree(boxTree, mFill.scale, "fill_opacity");
  put_in_tree(boxTree, mText.style, "text_style");
  put_in_tree(boxTree, mText.color, "text_color");
  put_in_tree(boxTree, mText.scale, "text_size");

  return boxTree;
};

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
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetUserCoordinates(bool userCoordinates)
{
  mPos.isUserCoord = userCoordinates;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetAutoPlacement()
{
  mPos.x = std::nullopt;
  mPos.y = std::nullopt;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetBorder(int16_t color, int16_t style, float_t width)
{
  mBorder.color = color;
  mBorder.style = style;
  mBorder.scale = width;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetBorderColor(int16_t color)
{
  mBorder.color = color;
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
  mBorder.scale = width;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetText(int16_t color, int16_t font, float_t size)
{
  mText.color = color;
  mText.style = font;
  mText.scale = size;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetTextColor(int16_t color)
{
  mText.color = color;
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
BoxType& Plot::Pad::Box<BoxType>::SetFill(int16_t color, int16_t style, float_t opacity)
{
  mFill.color = color;
  mFill.style = style;
  mFill.scale = opacity;
  return *GetThis();
}

template <typename BoxType>
BoxType& Plot::Pad::Box<BoxType>::SetFillColor(int16_t color)
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
BoxType& Plot::Pad::Box<BoxType>::SetFillOpacity(float_t opacity)
{
  mFill.scale = opacity;
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
  : Box(xPos, yPos), mText{text}
{
}

//**************************************************************************************************
/**
 * TextBox constructor with automatic placing.
 */
//**************************************************************************************************
Plot::Pad::TextBox::TextBox(const string& text) : Box(), mText{text}
{
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
    mText = textBoxTree.get<string>("text");
  } catch (...) {
    ERROR("Could not construct textbox from ptree.");
  }
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
  return boxTree;
};

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
Plot::Pad::LegendBox::LegendBox(double_t xPos, double_t yPos)
  : Box(xPos, yPos), mTitle{}, mNumColumns{}
{
}

//**************************************************************************************************
/**
 * LegendBox constructor with automatic placing.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendBox() : Box(), mTitle{}, mNumColumns{}
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
  read_from_tree(legendBoxTree, mNumColumns, "num_columns");
  read_from_tree(legendBoxTree, mDrawStyleDefault, "default_draw_style");
  read_from_tree(legendBoxTree, mMarkerDefault.color, "default_marker_color");
  read_from_tree(legendBoxTree, mMarkerDefault.style, "default_marker_style");
  read_from_tree(legendBoxTree, mMarkerDefault.scale, "default_marker_size");
  read_from_tree(legendBoxTree, mLineDefault.color, "default_line_color");
  read_from_tree(legendBoxTree, mLineDefault.style, "default_line_style");
  read_from_tree(legendBoxTree, mLineDefault.scale, "default_line_width");
  read_from_tree(legendBoxTree, mFillDefault.color, "default_fill_color");
  read_from_tree(legendBoxTree, mFillDefault.style, "default_fill_style");
  read_from_tree(legendBoxTree, mFillDefault.scale, "default_fill_opacity");

  for (auto& content : legendBoxTree) {
    if (str_contains(content.first, "ENTRY")) {
      uint8_t legendEntryID = std::stoi(content.first.substr(content.first.find("_") + 1));
      mLegendEntriesUser[legendEntryID] = LegendEntry(content.second);
    }
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
  put_in_tree(legendBoxTree, mDrawStyleDefault, "default_draw_style");
  put_in_tree(legendBoxTree, mMarkerDefault.color, "default_marker_color");
  put_in_tree(legendBoxTree, mMarkerDefault.style, "default_marker_style");
  put_in_tree(legendBoxTree, mMarkerDefault.scale, "default_marker_size");
  put_in_tree(legendBoxTree, mLineDefault.color, "default_line_color");
  put_in_tree(legendBoxTree, mLineDefault.style, "default_line_style");
  put_in_tree(legendBoxTree, mLineDefault.scale, "default_line_width");
  put_in_tree(legendBoxTree, mFillDefault.color, "default_fill_color");
  put_in_tree(legendBoxTree, mFillDefault.style, "default_fill_style");
  put_in_tree(legendBoxTree, mFillDefault.scale, "default_fill_opacity");

  for (auto& [legendEntryID, legendEntry] : mLegendEntriesUser) {
    legendBoxTree.put_child("ENTRY_" + std::to_string(legendEntryID),
                            legendEntry.GetPropertyTree());
  }

  return legendBoxTree;
};

//**************************************************************************************************
/**
 * User accessors for LegendBox properties.
 */
//**************************************************************************************************
Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetTitle(const string& title)
{
  mTitle = title;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetNumColumns(uint8_t numColumns)
{
  mNumColumns = numColumns;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultDrawStyle(string drawStyle)
{
  mDrawStyleDefault = drawStyle;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineColor(int16_t color)
{
  mLineDefault.color = color;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineStyle(int16_t style)
{
  mLineDefault.style = style;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultLineWidth(float_t width)
{
  mLineDefault.scale = width;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultMarkerColor(int16_t color)
{
  mMarkerDefault.color = color;
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

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultFillColor(int16_t color)
{
  mFillDefault.color = color;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultFillStyle(int16_t style)
{
  mFillDefault.style = style;
  return *this;
}

Plot::Pad::LegendBox& Plot::Pad::LegendBox::SetDefaultFillOpacity(float_t opacity)
{
  mFillDefault.scale = opacity;
  return *this;
}

//**************************************************************************************************
/**
 * Add entry to LegendBox.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::AddEntry(const string& lable, const string& refDataName)
{
  mLegendEntries.push_back(LegendEntry(lable, refDataName));
  return mLegendEntries.back();
}

//**************************************************************************************************
/**
 * Apply legend entry settings from user to automatically generated legend.
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::MergeLegendEntries()
{

  for (auto iter = mLegendEntriesUser.begin(); iter != mLegendEntriesUser.end(); ++iter) {
    uint8_t legendIndex = iter->first;
    if (legendIndex == mLegendEntries.size() + 1) {
      mLegendEntries.push_back(iter->second);
    } else if (legendIndex > 0 && legendIndex < mLegendEntries.size()) {
      mLegendEntries[legendIndex - 1] += iter->second;
    } else {
      ERROR("Invalid index ({}) specified for legend entry!", legendIndex);
    }
  }
}

//**************************************************************************************************
/**
 * Construct LegendEntry.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendEntry::LegendEntry(const optional<string>& lable, const optional<string>& refDataName, const optional<string>& drawStyle)
{
  mLable = lable;
  mRefDataName = refDataName;
  mDrawStyle = drawStyle;
}

//**************************************************************************************************
/**
 * Construct LegendEntry from property tree.
 */
//**************************************************************************************************
Plot::Pad::LegendBox::LegendEntry::LegendEntry(const ptree& legendEntryTree)
{
  read_from_tree(legendEntryTree, mLable, "lable");
  read_from_tree(legendEntryTree, mRefDataName, "ref_data_name");
  read_from_tree(legendEntryTree, mDrawStyle, "draw_style");
  read_from_tree(legendEntryTree, mFill.color, "fill_color");
  read_from_tree(legendEntryTree, mFill.style, "fill_style");
  read_from_tree(legendEntryTree, mFill.scale, "fill_opacity");
  read_from_tree(legendEntryTree, mLine.color, "line_color");
  read_from_tree(legendEntryTree, mLine.style, "line_style");
  read_from_tree(legendEntryTree, mLine.scale, "line_width");
  read_from_tree(legendEntryTree, mMarker.color, "marker_color");
  read_from_tree(legendEntryTree, mMarker.style, "marker_style");
  read_from_tree(legendEntryTree, mMarker.scale, "marker_width");
  read_from_tree(legendEntryTree, mText.color, "text_color");
  read_from_tree(legendEntryTree, mText.style, "text_font");
  read_from_tree(legendEntryTree, mText.scale, "text_size");
}

//**************************************************************************************************
/**
 * Get property tree of LegendEntry.
 */
//**************************************************************************************************
ptree Plot::Pad::LegendBox::LegendEntry::GetPropertyTree() const
{
  ptree legendEntryTree;
  put_in_tree(legendEntryTree, mLable, "lable");
  put_in_tree(legendEntryTree, mRefDataName, "ref_data_name");
  put_in_tree(legendEntryTree, mDrawStyle, "draw_style");
  put_in_tree(legendEntryTree, mFill.color, "fill_color");
  put_in_tree(legendEntryTree, mFill.style, "fill_style");
  put_in_tree(legendEntryTree, mFill.scale, "fill_opacity");
  put_in_tree(legendEntryTree, mLine.color, "line_color");
  put_in_tree(legendEntryTree, mLine.style, "line_style");
  put_in_tree(legendEntryTree, mLine.scale, "line_width");
  put_in_tree(legendEntryTree, mMarker.color, "marker_color");
  put_in_tree(legendEntryTree, mMarker.style, "marker_style");
  put_in_tree(legendEntryTree, mMarker.scale, "marker_width");
  put_in_tree(legendEntryTree, mText.color, "text_color");
  put_in_tree(legendEntryTree, mText.style, "text_font");
  put_in_tree(legendEntryTree, mText.scale, "text_size");
  return legendEntryTree;
};

//**************************************************************************************************
/**
 * Apply user defined properties to existing LegendEntry
 */
//**************************************************************************************************
void Plot::Pad::LegendBox::LegendEntry::operator+=(
  const Plot::Pad::LegendBox::LegendEntry& legendEntry)
{
  this->mDrawStyle = legendEntry.mDrawStyle;
  this->mMarker.color = legendEntry.mMarker.color;
  this->mMarker.scale = legendEntry.mMarker.scale;
  this->mMarker.style = legendEntry.mMarker.style;
  this->mLine.color = legendEntry.mLine.color;
  this->mLine.scale = legendEntry.mLine.scale;
  this->mLine.style = legendEntry.mLine.style;
  this->mFill.color = legendEntry.mFill.color;
  this->mFill.scale = legendEntry.mFill.scale;
  this->mFill.style = legendEntry.mFill.style;
  this->mText.color = legendEntry.mText.color;
  this->mText.scale = legendEntry.mText.scale;
  this->mText.style = legendEntry.mText.style;
}

//**************************************************************************************************
/**
 * User accessors to change legend entry properties.
 */
//**************************************************************************************************

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetLable(const string& lable)
{
  mLable = lable;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetRefData(const string& name, const string& inputIdentifier)
{
  mRefDataName = name + gNameGroupSeparator + inputIdentifier;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetDrawStyle(const string& drawStyle)
{
  mDrawStyle = drawStyle;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetMarkerColor(int16_t color)
{
  mMarker.color = color;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetMarkerStyle(int16_t style)
{
  mMarker.style = style;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetMarkerSize(float_t size)
{
  mMarker.scale = size;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetLineColor(int16_t color)
{
  mLine.color = color;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetLineStyle(int16_t style)
{
  mLine.style = style;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetLineWidth(float_t width)
{
  mLine.scale = width;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetFillColor(int16_t color)
{
  mFill.color = color;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetFillStyle(int16_t style)
{
  mFill.style = style;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetFillOpacity(
  float_t opacity)
{
  mFill.scale = opacity;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetTextColor(int16_t color)
{
  mText.color = color;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetTextFont(int16_t font)
{
  mText.style = font;
  return *this;
}

Plot::Pad::LegendBox::LegendEntry& Plot::Pad::LegendBox::LegendEntry::SetTextSize(float_t size)
{
  mText.scale = size;
  return *this;
}

//**************************************************************************************************
// explicitly instanciate required box templates
template class Plot::Pad::Box<Plot::Pad::TextBox>;
template class Plot::Pad::Box<Plot::Pad::LegendBox>;
//**************************************************************************************************
} // end namespace PlottingFramework
