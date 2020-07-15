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

using namespace PlottingFramework;
namespace PlottingFramework {

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// IMPLEMENTATION class Plot
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

//****************************************************************************************
/**
 * Default constructor.
 */
//****************************************************************************************
Plot::Plot(const string& name, const string& figureGroup, const string& plotTemplateName)
: Plot()
{
  if(figureGroup.find(".") != string::npos) ERROR("Figure Group must not contain '.'!");
  mName = name;
  mFigureGroup = figureGroup;
  if(plotTemplateName != "") mPlotTemplateName = plotTemplateName;
  
  // in case category was specified via figureGroup:my/category/tree
  auto subPathPos = figureGroup.find(":");
  if(subPathPos != string::npos)
  {
    mFigureGroup = figureGroup.substr(0, subPathPos);
    mFigureCategory = figureGroup.substr(subPathPos+1);
  }
}

//****************************************************************************************
/**
 * Constructor from property tree.
 */
//****************************************************************************************
Plot::Plot(const ptree& plotTree)
{
  try{
    mName = plotTree.get<string>("name");
    mFigureGroup = plotTree.get<string>("figureGroup");
    mFigureCategory = plotTree.get<string>("figureCategory");
  }catch(...){
    ERROR("Could not construct data from ptree.");
  }

  if(auto var = plotTree.get_optional<string>("plotTemplateName")) mPlotTemplateName = *var;
  if(auto var = plotTree.get_optional<int32_t>("width")) mPlotDimensions.width = *var;
  if(auto var = plotTree.get_optional<int32_t>("height")) mPlotDimensions.height = *var;
  if(auto var = plotTree.get_optional<bool>("fixAspectRatio")) mPlotDimensions.fixAspectRatio = *var;
  if(auto var = plotTree.get_optional<int32_t>("fill_color")) mFill.color = *var;
  if(auto var = plotTree.get_optional<int32_t>("fill_style")) mFill.style = *var;

  // loop over all pads defined in property tree
  for(auto& pad : plotTree)
  {
    if(pad.first.find("PAD") != string::npos)
    {
      uint8_t padID = std::stoi(pad.first.substr(pad.first.find("_")+1));
      mPads[padID] = std::move(Pad(pad.second));
    }
  }
}

//****************************************************************************************
/**
 * Get representation of plot as property tree.
 */
//****************************************************************************************
ptree Plot::GetPropetyTree()
{
  ptree plotTree;
  plotTree.put("name", mName);
  plotTree.put("figureGroup", mFigureGroup);
  plotTree.put("figureCategory", mFigureCategory);

  if(mPlotTemplateName) plotTree.put("plotTemplateName", *mPlotTemplateName);
  if(mPlotDimensions.width) plotTree.put("width", *mPlotDimensions.width);
  if(mPlotDimensions.height) plotTree.put("height", *mPlotDimensions.height);
  if(mPlotDimensions.fixAspectRatio) plotTree.put("fixAspectRatio", *mPlotDimensions.fixAspectRatio);
  if(mFill.color) plotTree.put("fill_color", *mFill.color);
  if(mFill.style) plotTree.put("fill_style", *mFill.style);

  for(auto& [padID, pad] : mPads)
  {
    plotTree.put_child("PAD_" + std::to_string(padID), pad.GetPropetyTree());
  }
  return plotTree;
}


//****************************************************************************************
/**
 * Set fill for this plot.
 */
//****************************************************************************************
auto Plot::SetFill(int16_t color, int16_t style)  ->decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  return *this;
}

//****************************************************************************************
/**
 * Set this plot transparent.
 */
//****************************************************************************************
auto Plot::SetTransparent()  ->decltype(*this)
{
  mFill.style = 4000;
  return *this;
}

//****************************************************************************************
/**
 * Apply all settings from plot on top of this plot.
 */
//****************************************************************************************
void Plot::operator+=(const Plot& plot)
{
  mName = plot.mName;
  mFigureGroup = plot.mFigureGroup;
  mFigureCategory = plot.mFigureCategory;
  mPlotTemplateName = plot.mPlotTemplateName;
  
  if(plot.mPlotDimensions.width) mPlotDimensions.width = plot.mPlotDimensions.width;
  if(plot.mPlotDimensions.height) mPlotDimensions.height = plot.mPlotDimensions.height;
  if(plot.mPlotDimensions.fixAspectRatio) mPlotDimensions.fixAspectRatio = plot.mPlotDimensions.fixAspectRatio;

  if(plot.mFill.color) mFill.color = plot.mFill.color;
  if(plot.mFill.style) mFill.style = plot.mFill.style;

  for(auto& [padID, pad] : plot.mPads)
  {
    mPads[padID]; // initializes the pad in case it was not yet defined in this plot
    mPads[padID] += pad;
  }
}

//****************************************************************************************
/**
 * Friend function to get the combination of template and actual plot.
 */
//****************************************************************************************
Plot operator+(const Plot& templatePlot, const Plot& plot)
{
  Plot combinedPlot = templatePlot;
  combinedPlot += plot;
  return combinedPlot;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// IMPLEMENTATION class Pad
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

//****************************************************************************************
/**
 * Trigger redrawing of axes after all data was drawn to the pad.
 */
//****************************************************************************************
auto Plot::Pad::SetRedrawAxes(bool redraw)  ->decltype(*this)
{
  mRedrawAxes = redraw;
  return *this;
}

//****************************************************************************************
/**
 * Set text size
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultTextSize(float_t size)  ->decltype(*this)
{
  mText.size = size;
  return *this;
}

//****************************************************************************************
/**
 * Set text color
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultTextColor(int16_t color)  ->decltype(*this)
{
  mText.color = color;
  return *this;
}

//****************************************************************************************
/**
 * Set text font for this pad.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultTextFont(int16_t font)  ->decltype(*this)
{
  mText.font = font;
  return *this;
}

//****************************************************************************************
/**
 * Set default marker size for this pad.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultMarkerSize(float_t size)  ->decltype(*this)
{
  mMarkerDefaults.scale = size;
  return *this;
}

//****************************************************************************************
/**
 * Set default line width for this pad.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultLineWidth(float_t width)  ->decltype(*this)
{
  mLineDefaults.scale = width;
  return *this;
}

//****************************************************************************************
/**
 * Set default fill opacity for this pad.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultFillOpacity(float_t opacity)  ->decltype(*this)
{
  mFillDefaults.scale = opacity;
  return *this;
}

//****************************************************************************************
/**
 * Set default marker colors.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultMarkerColors(const vector<int16_t>& colors)  ->decltype(*this)
{
  mMarkerDefaults.colors = colors;
  return *this;
}

//****************************************************************************************
/**
 * Set default line colors.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultLineColors(const vector<int16_t>& colors)  ->decltype(*this)
{
  mLineDefaults.colors = colors;
  return *this;
}

//****************************************************************************************
/**
 * Set default fill colors.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultFillColors(const vector<int16_t>& colors)  ->decltype(*this)
{
  mFillDefaults.colors = colors;
  return *this;
}

//****************************************************************************************
/**
 * Set default marker styles.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultMarkerStyles(const vector<int16_t>& styles)  ->decltype(*this)
{
  mMarkerDefaults.styles = styles;
  return *this;
}

//****************************************************************************************
/**
 * Set default line styles.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultLineStyles(const vector<int16_t>& styles)  ->decltype(*this)
{
  mLineDefaults.styles = styles;
  return *this;
}

//****************************************************************************************
/**
 * Set default fill styles.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultFillStyles(const vector<int16_t>& styles)  ->decltype(*this)
{
  mFillDefaults.styles = styles;
  return *this;
}

//****************************************************************************************
/**
 * Set default drawing option for graphs.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultDrawingOptionGraph(drawing_options_t drawingOption)  ->decltype(*this)
{
  mDrawingOptionDefaults.graph = drawingOption;
  return *this;
}

//****************************************************************************************
/**
 * Set default drawing option for histograms.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultDrawingOptionHist(drawing_options_t drawingOption)  ->decltype(*this)
{
  mDrawingOptionDefaults.hist = drawingOption;
  return *this;
}

//****************************************************************************************
/**
 * Set default drawing option for 2d histograms.
 */
//****************************************************************************************
auto Plot::Pad::SetDefaultDrawingOptionHist2d(drawing_options_t drawingOption)  ->decltype(*this)
{
  mDrawingOptionDefaults.hist2d = drawingOption;
  return *this;
}

//****************************************************************************************
/**
 * Set fill for this pad.
 */
//****************************************************************************************
auto Plot::Pad::SetFill(int16_t color, int16_t style)  ->decltype(*this)
{
  mFill.color = color;
  mFill.style = style;
  return *this;
}

//****************************************************************************************
/**
 * Make this pad transparent.
 */
//****************************************************************************************
auto Plot::Pad::SetTransparent()  ->decltype(*this)
{
  mFill.style = 4000;
  return *this;
}

//****************************************************************************************
/**
 * Set frame fill properties.
 */
//****************************************************************************************
auto Plot::Pad::SetFillFrame(int16_t color, int16_t style)  ->decltype(*this)
{
  mFrame.fillColor = color;
  mFrame.fillStyle = style;
  return *this;
}

//****************************************************************************************
/**
 * Set frame line properties.
 */
//****************************************************************************************
auto Plot::Pad::SetLineFrame(int16_t color, int16_t style, float_t width)  ->decltype(*this)
{
  mFrame.lineColor = color;
  mFrame.lineStyle = style;
  mFrame.lineWidth = width;
  return *this;
}

//****************************************************************************************
/**
 * Set this pad transparent.
 */
//****************************************************************************************
auto Plot::Pad::SetTransparentFrame()  ->decltype(*this)
{
  mFrame.fillStyle = 0;
  return *this;
}

//****************************************************************************************
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
 * - Intense: kGreenPink, kOcean, kDarkBodyRadiator, kInvertedDarkBodyRadiator, kSunset, kVisibleSpectrum
 */
//****************************************************************************************
auto Plot::Pad::SetPalette(int32_t palette)  ->decltype(*this)
{
  mPalette = palette;
  return *this;
}

//****************************************************************************************
/**
 * Set title of pad
 */
//****************************************************************************************
auto Plot::Pad::SetTitle(const string& title)  ->decltype(*this)
{
  mTitle = title;
  return *this;
}

//****************************************************************************************
/**
 * Set position of pad within plot. The variables are given in fractions of plot width resp. height (between 0 and 1).
 * The lower left corner is defined by (xlow, ylow) and the upper right corner by (xup, yup).
 */
//****************************************************************************************
auto Plot::Pad::SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup) ->decltype(*this)
{
  mPosition.xlow = xlow;
  mPosition.ylow = ylow;
  mPosition.xup = xup;
  mPosition.yup = yup;
  return *this;
}

//****************************************************************************************
/**
 * Set margins of pad.
 */
//****************************************************************************************
auto Plot::Pad::SetMargins(float_t top, float_t bottom, float_t left, float_t right) ->decltype(*this)
{
  mMargins.top = top;
  mMargins.bottom = bottom;
  mMargins.left = left;
  mMargins.right = right;
  return *this;
}

//****************************************************************************************
/**
 * Constructor from property tree.
 */
//****************************************************************************************
Plot::Pad::Pad(const ptree& padTree)
{
  if(auto var = padTree.get_optional<string>("title")) mTitle = *var;
  if(auto var = padTree.get_optional<string>("options")) mOptions = *var;
  if(auto var = padTree.get_optional<double_t>("position_xlow")) mPosition.xlow = *var;
  if(auto var = padTree.get_optional<double_t>("position_ylow")) mPosition.ylow = *var;
  if(auto var = padTree.get_optional<double_t>("position_xup")) mPosition.xup = *var;
  if(auto var = padTree.get_optional<double_t>("position_yup")) mPosition.yup = *var;
  if(auto var = padTree.get_optional<float_t>("margins_top")) mMargins.top = *var;
  if(auto var = padTree.get_optional<float_t>("margins_bottom")) mMargins.bottom = *var;
  if(auto var = padTree.get_optional<float_t>("margins_left")) mMargins.left = *var;
  if(auto var = padTree.get_optional<float_t>("margins_right")) mMargins.right = *var;
  if(auto var = padTree.get_optional<int32_t>("palette")) mPalette = *var;
  if(auto var = padTree.get_optional<int32_t>("fill_color")) mFill.color = *var;
  if(auto var = padTree.get_optional<int32_t>("fill_style")) mFill.style = *var;
  if(auto var = padTree.get_optional<int32_t>("frame_fill_color")) mFrame.fillColor = *var;
  if(auto var = padTree.get_optional<int32_t>("frame_fill_style")) mFrame.fillStyle = *var;
  if(auto var = padTree.get_optional<int32_t>("frame_line_color")) mFrame.lineColor = *var;
  if(auto var = padTree.get_optional<int32_t>("frame_line_style")) mFrame.lineStyle = *var;
  if(auto var = padTree.get_optional<float_t>("frame_line_width")) mFrame.lineWidth = *var;
  if(auto var = padTree.get_optional<int32_t>("text_color")) mText.color = *var;
  if(auto var = padTree.get_optional<int32_t>("text_font")) mText.font = *var;
  if(auto var = padTree.get_optional<float_t>("text_size")) mText.size = *var;
  if(auto var = padTree.get_optional<float_t>("default_marker_size")) mMarkerDefaults.scale = *var;
  if(auto var = padTree.get_optional<float_t>("default_line_width")) mLineDefaults.scale = *var;
  if(auto var = padTree.get_optional<float_t>("default_fill_opacity")) mFillDefaults.scale = *var;
  
  if(auto var = padTree.get_optional<string>("default_marker_colors")) mMarkerDefaults.colors = StringToVector(*var);
  if(auto var = padTree.get_optional<string>("default_line_colors")) mLineDefaults.colors = StringToVector(*var);
  if(auto var = padTree.get_optional<string>("default_fill_colors")) mFillDefaults.colors = StringToVector(*var);
  if(auto var = padTree.get_optional<string>("default_marker_styles")) mMarkerDefaults.styles = StringToVector(*var);
  if(auto var = padTree.get_optional<string>("default_line_styles")) mLineDefaults.styles = StringToVector(*var);
  if(auto var = padTree.get_optional<string>("default_fill_styles")) mFillDefaults.styles = StringToVector(*var);

  if(auto var = padTree.get_optional<uint8_t>("default_drawing_option_graph")) mDrawingOptionDefaults.graph = (drawing_options_t)*var;
  if(auto var = padTree.get_optional<uint8_t>("default_drawing_option_hist")) mDrawingOptionDefaults.hist = (drawing_options_t)*var;
  if(auto var = padTree.get_optional<uint8_t>("default_drawing_option_hist2d")) mDrawingOptionDefaults.hist2d = (drawing_options_t)*var;


  if(auto var = padTree.get_optional<bool>("redraw_axes")) mRedrawAxes = *var;
  if(auto var = padTree.get_optional<string>("ref_func")) mRefFunc = *var;

  for(auto& content : padTree)
  {
    // add data
    if(content.first.find("DATA") != string::npos)
    {
      string type = content.second.get<string>("type");
      if(type == "data")
      {
        mData.push_back(std::make_shared<Data>(content.second));
      }
      if(type == "ratio")
      {
        mData.push_back(std::make_shared<Ratio>(content.second));
      }
    }
    
    // add boxes
    if(content.first.find("BOX") != string::npos)
    {
      string type = content.second.get<string>("type");
      if(type == "legend")
      {
        mBoxes.push_back(std::make_shared<LegendBox>(content.second));
      }
      if(type == "text")
      {
        mBoxes.push_back(std::make_shared<TextBox>(content.second));
      }
    }
    
    // add axes
    if(content.first.find("AXIS") != string::npos)
    {
      string axis = content.second.get<string>("name");
      mAxes[axis] = std::move(Axis(content.second));
    }
  }
}

//****************************************************************************************
/**
 * Access operator for pad axis.
 */
//****************************************************************************************

Plot::Pad::Axis& Plot::Pad::operator[](string axis)
{
  std::transform(axis.begin(), axis.end(),axis.begin(), ::toupper);
  vector<string> allowedAxes = {"X", "Y", "Z"};
  if(std::find(allowedAxes.begin(), allowedAxes.end(), axis) == allowedAxes.end())
  {
    ERROR("Axis \"{}\" is not allowed! Falling back to \"X\".", axis);
    axis = "X";
  }
  if(mAxes.find(axis) == mAxes.end())
  {
    mAxes[axis] = Axis(axis);
  }
  return mAxes[axis];
}

//****************************************************************************************
/**
 * Get representation of pad as property tree.
 */
//****************************************************************************************
ptree Plot::Pad::GetPropetyTree()
{
  // convert properties of plot to ptree
  ptree padTree;
  if(mTitle) padTree.put("title", *mTitle);
  if(mOptions) padTree.put("options", *mOptions);
  if(mPosition.xlow) padTree.put("position_xlow", *mPosition.xlow);
  if(mPosition.ylow) padTree.put("position_ylow", *mPosition.ylow);
  if(mPosition.xup) padTree.put("position_xup", *mPosition.xup);
  if(mPosition.yup) padTree.put("position_yup", *mPosition.yup);
  if(mMargins.top) padTree.put("margins_top", *mMargins.top);
  if(mMargins.bottom) padTree.put("margins_bottom", *mMargins.bottom);
  if(mMargins.left) padTree.put("margins_left", *mMargins.left);
  if(mMargins.right) padTree.put("margins_right", *mMargins.right);
  if(mPalette) padTree.put("palette", *mPalette);
  if(mFill.color) padTree.put("fill_color", *mFill.color);
  if(mFill.style) padTree.put("fill_style", *mFill.style);
  if(mFrame.fillColor) padTree.put("frame_fill_color", *mFrame.fillColor);
  if(mFrame.fillStyle) padTree.put("frame_fill_style", *mFrame.fillStyle);
  if(mFrame.lineColor) padTree.put("frame_line_color", *mFrame.lineColor);
  if(mFrame.lineStyle) padTree.put("frame_line_style", *mFrame.lineStyle);
  if(mFrame.lineWidth) padTree.put("frame_line_width", *mFrame.lineWidth);
  if(mText.color) padTree.put("text_color", *mText.color);
  if(mText.size) padTree.put("text_size", *mText.size);
  if(mText.font) padTree.put("text_font", *mText.font);
  if(mMarkerDefaults.scale) padTree.put("default_marker_size", *mMarkerDefaults.scale);
  if(mLineDefaults.scale) padTree.put("default_line_width", *mLineDefaults.scale);
  if(mFillDefaults.scale) padTree.put("default_fill_opacity", *mFillDefaults.scale);
  
  if(mMarkerDefaults.colors) padTree.put("default_marker_colors", VectorToString(*mMarkerDefaults.colors));
  if(mLineDefaults.colors) padTree.put("default_line_colors", VectorToString(*mLineDefaults.colors));
  if(mFillDefaults.colors) padTree.put("default_fill_colors", VectorToString(*mFillDefaults.colors));
  if(mMarkerDefaults.styles) padTree.put("default_marker_styles", VectorToString(*mMarkerDefaults.styles));
  if(mLineDefaults.styles) padTree.put("default_line_styles", VectorToString(*mLineDefaults.styles));
  if(mFillDefaults.styles) padTree.put("default_fill_styles", VectorToString(*mFillDefaults.styles));

  if(mDrawingOptionDefaults.graph) padTree.put("default_drawing_option_graph", *mDrawingOptionDefaults.graph);
  if(mDrawingOptionDefaults.hist) padTree.put("default_drawing_option_hist", *mDrawingOptionDefaults.hist);
  if(mDrawingOptionDefaults.hist2d) padTree.put("default_drawing_option_hist2d", *mDrawingOptionDefaults.hist2d);

  

  if(mRedrawAxes) padTree.put("redraw_axes", *mRedrawAxes);
  if(mRefFunc) padTree.put("ref_func", *mRefFunc);

  int dataID = 1;
  for(auto& data : mData)
  {
    padTree.put_child("DATA_" + std::to_string(dataID), data->GetPropertyTree());
    ++dataID;
  }

  int boxID = 1;
  for(auto& box : mBoxes)
  {
    padTree.put_child("BOX_" + std::to_string(boxID), box->GetPropertyTree());
    ++boxID;
  }
    
  for(auto& axis : {"X", "Y", "Z"})
  {
    if(mAxes.find(axis) != mAxes.end())
    {
      padTree.put_child(string("AXIS_") + axis, mAxes[axis].GetPropertyTree());
    }
  }

  return padTree;
}

//****************************************************************************************
/**
 * Apply all settings from pad on top of this pad.
 */
//****************************************************************************************
void Plot::Pad::operator+=(const Pad& pad)
{
  if(pad.mTitle) mTitle = pad.mTitle;
  if(pad.mOptions) mOptions = pad.mOptions;


  if(pad.mPosition.xlow) mPosition.xlow = pad.mPosition.xlow;
  if(pad.mPosition.ylow) mPosition.ylow = pad.mPosition.ylow;
  if(pad.mPosition.xup) mPosition.xup = pad.mPosition.xup;
  if(pad.mPosition.yup) mPosition.yup = pad.mPosition.yup;

  if(pad.mMargins.top) mMargins.top = pad.mMargins.top;
  if(pad.mMargins.bottom) mMargins.bottom = pad.mMargins.bottom;
  if(pad.mMargins.left) mMargins.left = pad.mMargins.left;
  if(pad.mMargins.right) mMargins.right = pad.mMargins.right;

  if(pad.mFill.color) mFill.color = pad.mFill.color;
  if(pad.mFill.style) mFill.style = pad.mFill.style;

  if(pad.mFrame.fillColor) mFrame.fillColor = pad.mFrame.fillColor;
  if(pad.mFrame.fillStyle) mFrame.fillStyle = pad.mFrame.fillStyle;
  if(pad.mFrame.lineColor) mFrame.lineColor = pad.mFrame.lineColor;
  if(pad.mFrame.lineStyle) mFrame.lineStyle = pad.mFrame.lineStyle;
  if(pad.mFrame.lineWidth) mFrame.lineWidth = pad.mFrame.lineWidth;

  if(pad.mText.size) mText.size = pad.mText.size;
  if(pad.mText.font) mText.font = pad.mText.font;
  if(pad.mText.color) mText.color = pad.mText.color;

  if(pad.mMarkerDefaults.scale) mMarkerDefaults.scale = pad.mMarkerDefaults.scale;
  if(pad.mLineDefaults.scale) mLineDefaults.scale = pad.mLineDefaults.scale;
  if(pad.mFillDefaults.scale) mFillDefaults.scale = pad.mFillDefaults.scale;
  
  if(pad.mMarkerDefaults.colors) mMarkerDefaults.colors = pad.mMarkerDefaults.colors;
  if(pad.mLineDefaults.colors) mLineDefaults.colors = pad.mLineDefaults.colors;
  if(pad.mFillDefaults.colors) mFillDefaults.colors = pad.mFillDefaults.colors;
  if(pad.mMarkerDefaults.styles) mMarkerDefaults.styles = pad.mMarkerDefaults.styles;
  if(pad.mLineDefaults.styles) mLineDefaults.styles = pad.mLineDefaults.styles;
  if(pad.mFillDefaults.styles) mFillDefaults.styles = pad.mFillDefaults.styles;

  if(pad.mDrawingOptionDefaults.graph) mDrawingOptionDefaults.graph = pad.mDrawingOptionDefaults.graph;
  if(pad.mDrawingOptionDefaults.hist) mDrawingOptionDefaults.hist = pad.mDrawingOptionDefaults.hist;
  if(pad.mDrawingOptionDefaults.hist2d) mDrawingOptionDefaults.hist2d = pad.mDrawingOptionDefaults.hist2d;

  if(pad.mPalette) mPalette = pad.mPalette;

  if(pad.mRedrawAxes) mRedrawAxes = pad.mRedrawAxes;
  if(pad.mRefFunc) mRefFunc = pad.mRefFunc;

  for(auto& [axisLable, axis] : pad.mAxes)
  {
    mAxes[axisLable]; // default initiialize in case this axis was not yet defined
    mAxes[axisLable] += axis;
  }
  mData = pad.mData; // this does not copy the data itself, which might become a problem at some point
  mBoxes = pad.mBoxes;
}

//****************************************************************************************
/**
 * Add data to this pad.
 */
//****************************************************************************************
Plot::Pad::Data& Plot::Pad::AddData(const input_t& data, const string& lable)
{
  mData.push_back(std::make_shared<Data>(data.name, data.inputIdentifier, lable));
  return *mData[mData.size()-1];
}

//****************************************************************************************
/**
 * Add ratio to this pad.
 */
//****************************************************************************************
Plot::Pad::Ratio& Plot::Pad::AddRatio(const input_t& numerator, const input_t& denominator, const string& lable)
{
  mData.push_back(std::make_shared<Ratio>(numerator.name, numerator.inputIdentifier, denominator.name, denominator.inputIdentifier, lable));
  return *std::dynamic_pointer_cast<Ratio>(mData[mData.size()-1]);
}

//****************************************************************************************
/**
 * Add text box to this pad.
 */
//****************************************************************************************
void Plot::Pad::AddText(double xPos, double yPos, const string& text, bool userCoordinates, int borderStyle, int borderSize, int borderColor)
{
  mBoxes.push_back(std::make_shared<TextBox>(userCoordinates, false, xPos, yPos, borderStyle, borderSize, borderColor, text));
}

//****************************************************************************************
/**
 * Add legend box to this pad.
 */
//****************************************************************************************
void Plot::Pad::AddLegend(double xPos, double yPos, const string& title, bool userCoordinates, int nColumns, int borderStyle, int borderSize, int borderColor)
{
  // TODO: add userCoordinates to arguments
  mBoxes.push_back(std::make_shared<LegendBox>(userCoordinates, false, xPos, yPos, borderStyle, borderSize, borderColor, title, nColumns));
}

//****************************************************************************************
/**
 * Add legend box to this pad and auto-place it.
 */
//****************************************************************************************
void Plot::Pad::AddLegend(const string& title, int nColumns, int borderStyle, int borderSize, int borderColor)
{
  // TODO: add userCoordinates to arguments
  mBoxes.push_back(std::make_shared<LegendBox>(false, true, 0, 0, borderStyle, borderSize, borderColor, title, nColumns));
}

//****************************************************************************************
/**
 * Helper function to convert vector of integers to string for saving in ptree.
 */
//****************************************************************************************
string Plot::Pad::VectorToString(vector<int16_t> numbers)
{
  string numberString;
  for(auto& number : numbers)
  {
    numberString += std::to_string(number);
    if(&number != &numbers.back()) numberString += ",";
  }
  return numberString;
}

//****************************************************************************************
/**
 * Helper function to convert string from ptree to vector of integers.
 */
//****************************************************************************************
vector<int16_t> Plot::Pad::StringToVector(string numberString)
{
  // savety in case user put some blank spaces between numbers
  std::remove_if(numberString.begin(), numberString.end(), ::isspace);
  vector<int16_t> numbers;
  string curNumStr;
  std::istringstream stream(numberString);
  while(std::getline(stream, curNumStr, ',')) {
    numbers.push_back(std::stoi(curNumStr));
  }
  return numbers;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// IMPLEMENTATION class Data
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

//****************************************************************************************
/**
 * Default constructor for Data objects.
 */
//****************************************************************************************
Plot::Pad::Data::Data(const string& name, const string& inputIdentifier, const string& legendLable)
  : Data()
  {
    mType = "data";
    
    if(legendLable != "") mLegendLable = legendLable;
    // in case input was specified further via inputIdentifier:some/path/in/file
    auto subPathPos = inputIdentifier.find(":");
    if(subPathPos != string::npos)
    {
      // prepend path to plot name
      mName = inputIdentifier.substr(subPathPos+1) + "/" + name;
      mInputIdentifier = inputIdentifier.substr(0, subPathPos);
    }
    else{
      mName = name;
      mInputIdentifier = inputIdentifier;
    }
  }

//****************************************************************************************
/**
 * Constructor for Data objects from property tree.
 */
//****************************************************************************************
Plot::Pad::Data::Data(const ptree& dataTree) : Data()
{
  try{
    mType = dataTree.get<string>("type");
    mName = dataTree.get<string>("name");
    mInputIdentifier = dataTree.get<string>("inputIdentifier");
  }catch(...){
    ERROR("Could not construct data from ptree."); // FIXME: what to do if this happens??
  }
  if(auto var = dataTree.get_optional<string>("legend_lable")) mLegendLable = *var;
  if(auto var = dataTree.get_optional<string>("drawingOptions")) mDrawingOptions = *var;
  if(auto var = dataTree.get_optional<uint8_t>("drawingOptionAlias")) mDrawingOptionAlias = (drawing_options_t)*var;
  if(auto var = dataTree.get_optional<int16_t>("marker_color")) mMarker.color = *var;
  if(auto var = dataTree.get_optional<int16_t>("marker_style")) mMarker.style = *var;
  if(auto var = dataTree.get_optional<float_t>("marker_size")) mMarker.scale = *var;
  if(auto var = dataTree.get_optional<int16_t>("line_color")) mLine.color = *var;
  if(auto var = dataTree.get_optional<int16_t>("line_style")) mLine.style = *var;
  if(auto var = dataTree.get_optional<float_t>("line_width")) mLine.scale = *var;
  if(auto var = dataTree.get_optional<int16_t>("fill_color")) mFill.color = *var;
  if(auto var = dataTree.get_optional<int16_t>("fill_style")) mFill.style = *var;
  if(auto var = dataTree.get_optional<float_t>("fill_opacity")) mFill.scale = *var;
  if(auto var = dataTree.get_optional<double_t>("scale_factor")) mScaleFactor = *var;
  if(auto var = dataTree.get_optional<double_t>("rangeX_min")) mRangeX.min = *var;
  if(auto var = dataTree.get_optional<double_t>("rangeX_max")) mRangeX.max = *var;
  if(auto var = dataTree.get_optional<double_t>("rangeY_min")) mRangeY.min = *var;
  if(auto var = dataTree.get_optional<double_t>("rangeY_max")) mRangeY.max = *var;

  if(auto var = dataTree.get_optional<bool>("defines_frame")) mDefinesFrame = *var;
}

//****************************************************************************************
/**
 * Generate property tree for this Data object.
 */
//****************************************************************************************
ptree Plot::Pad::Data::GetPropertyTree(){
  ptree dataTree;
  dataTree.put("type", mType);
  dataTree.put("name", mName);
  dataTree.put("inputIdentifier", mInputIdentifier);
  if(mLegendLable)    dataTree.put("legend_lable", *mLegendLable);
  if(mMarker.color)   dataTree.put("marker_color", *mMarker.color);
  if(mMarker.style)   dataTree.put("marker_style", *mMarker.style);
  if(mMarker.scale)   dataTree.put("marker_size", *mMarker.scale);
  if(mLine.color)     dataTree.put("line_color", *mLine.color);
  if(mLine.style)     dataTree.put("line_style", *mLine.style);
  if(mLine.scale)     dataTree.put("line_width", *mLine.scale);
  if(mFill.color)     dataTree.put("fill_color", *mFill.color);
  if(mFill.style)     dataTree.put("fill_style", *mFill.style);
  if(mFill.scale)     dataTree.put("fill_opacity", *mFill.scale);
  if(mDrawingOptions) dataTree.put("drawingOptions", *mDrawingOptions);
  if(mDrawingOptionAlias) dataTree.put("drawingOptionAlias", *mDrawingOptionAlias);
  if(mScaleFactor) dataTree.put("scale_factor", *mScaleFactor);
  if(mRangeX.min) dataTree.put("rangeX_min", *mRangeX.min);
  if(mRangeX.max) dataTree.put("rangeX_max", *mRangeX.max);
  if(mRangeY.min) dataTree.put("rangeY_min", *mRangeY.min);
  if(mRangeY.max) dataTree.put("rangeY_max", *mRangeY.max);

  if(mDefinesFrame) dataTree.put("defines_frame", mDefinesFrame);

  return dataTree;
}

//****************************************************************************************
/**
 * User accessors.
 */
//****************************************************************************************
auto Plot::Pad::Data::SetLegendLable(const string& legendLable) -> decltype(*this)
{
  mLegendLable = legendLable;
  return *this;
}
auto Plot::Pad::Data::SetOptions(const string& opions) -> decltype(*this)
{
  mDrawingOptions = opions;
  return *this;
}
auto Plot::Pad::Data::SetScaleFactor(double_t scale) -> decltype(*this)
{
  mScaleFactor = scale;
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
  if(opacity < 0.f || opacity > 1.f)
  {
    WARNING("Illegal value for opacity! It has to be between 0 (transparent) and 1 (fully opaque).");
  }
  else
  {
    mFill.scale = opacity;
  }
  return *this;
}
auto Plot::Pad::Data::SetOptions(drawing_options_t optionAlias) -> decltype(*this)
{
  mDrawingOptionAlias = optionAlias;
  return *this;
}
auto Plot::Pad::Data::SetDefinesFrame() -> decltype(*this)
{
  mDefinesFrame = true;
  return *this;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// IMPLEMENTATION class Ratio
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

//****************************************************************************************
/**
 * Default constructor.
 */
//****************************************************************************************
Plot::Pad::Ratio::Ratio(const string& name, const string& inputIdentifier, const string& denomName, const string& denomInputIdentifier, const string& lable)
: Data(name, inputIdentifier, lable), mDenomName(denomName), mDenomInputIdentifier(denomInputIdentifier), mIsCorrelated(false)
{
  SetType("ratio");
}

//****************************************************************************************
/**
 * Constructor from property tree.
 */
//****************************************************************************************
Plot::Pad::Ratio::Ratio(const ptree& dataTree) : Data(dataTree)
{
  try{
    mDenomName = dataTree.get<string>("denomName");
    mDenomInputIdentifier = dataTree.get<string>("denomInputID");
    mIsCorrelated = dataTree.get<bool>("isCorrelated");
  }catch(...){
    ERROR("Could not construct ratio from ptree.");
  }
}

//****************************************************************************************
/**
 * Convert ratio to property tree.
 */
//****************************************************************************************
ptree Plot::Pad::Ratio::GetPropertyTree()
{
  ptree dataTree = Data::GetPropertyTree();
  dataTree.put("denomName", mDenomName);
  dataTree.put("denomInputID", mDenomInputIdentifier);
  dataTree.put("isCorrelated", mIsCorrelated);
  return dataTree;
}

//****************************************************************************************
/**
 * Specify how the division should be done (spline, biniomial, etc...).
 */
//****************************************************************************************
auto Plot::Pad::Ratio::SetIsCorrelated(bool isCorrelated) -> decltype(*this)
{
  mIsCorrelated = isCorrelated;
  return *this;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// IMPLEMENTATION class Axis
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

//****************************************************************************************
/**
 * Constructor setting axis name (X,Y,Z).
 */
//****************************************************************************************
Plot::Pad::Axis::Axis(const string& axisName) : Axis()
{
  mName = axisName;
}

//****************************************************************************************
/**
 * Constructor to define entry from file.
 */
//****************************************************************************************
Plot::Pad::Axis::Axis(const ptree& axisTree) : Axis()
{
  try{
    mName = axisTree.get<string>("name");
  }catch(...){
    ERROR("Could not construct axis from ptree.");
  }
  if(auto var = axisTree.get_optional<string>("title")) mTitle = *var;
  if(auto var = axisTree.get_optional<double_t>("range_min")) mRange.min = *var;
  if(auto var = axisTree.get_optional<double_t>("range_max")) mRange.max = *var;
  if(auto var = axisTree.get_optional<int32_t>("numDivisions")) mNumDivisions = *var;
  if(auto var = axisTree.get_optional<int32_t>("maxDigits")) mMaxDigits = *var;
  if(auto var = axisTree.get_optional<float_t>("tickLength")) mTickLength = *var;
  if(auto var = axisTree.get_optional<int16_t>("axisColor")) mAxisColor = *var;
  if(auto var = axisTree.get_optional<int16_t>("title_font")) mTitleProperties.font = *var;
  if(auto var = axisTree.get_optional<float_t>("title_size")) mTitleProperties.size = *var;
  if(auto var = axisTree.get_optional<int16_t>("title_color")) mTitleProperties.color = *var;
  if(auto var = axisTree.get_optional<float_t>("title_offset")) mTitleProperties.offset = *var;
  if(auto var = axisTree.get_optional<bool>("title_center")) mTitleProperties.center = *var;
  if(auto var = axisTree.get_optional<int16_t>("lable_font")) mLableProperties.font = *var;
  if(auto var = axisTree.get_optional<float_t>("lable_size")) mLableProperties.size = *var;
  if(auto var = axisTree.get_optional<int16_t>("lable_color")) mLableProperties.color = *var;
  if(auto var = axisTree.get_optional<float_t>("lable_offset")) mLableProperties.offset = *var;
  if(auto var = axisTree.get_optional<bool>("lable_center")) mLableProperties.center = *var;
  if(auto var = axisTree.get_optional<bool>("isLog")) mIsLog = *var;
  if(auto var = axisTree.get_optional<bool>("isGrid")) mIsGrid = *var;
  if(auto var = axisTree.get_optional<bool>("isOppositeTicks")) mIsOppositeTicks = *var;
  if(auto var = axisTree.get_optional<string>("time_format")) mTimeFormat = *var;
  if(auto var = axisTree.get_optional<string>("tick_orientation")) mTickOrientation = *var;
}

//****************************************************************************************
/**
 * Convert axis object to property tree.
 */
//****************************************************************************************
ptree Plot::Pad::Axis::GetPropertyTree(){
  ptree axisTree;
  axisTree.put("name", mName);
  if(mTitle) axisTree.put("title", *mTitle);
  if(mRange.min) axisTree.put("range_min", *mRange.min);
  if(mRange.max) axisTree.put("range_max", *mRange.max);
  if(mNumDivisions) axisTree.put("numDivisions", *mNumDivisions);
  if(mMaxDigits) axisTree.put("maxDigits", *mMaxDigits);
  if(mTickLength) axisTree.put("tickLength", *mTickLength);
  if(mAxisColor) axisTree.put("axisColor", *mAxisColor);
  if(mTitleProperties.font) axisTree.put("title_font", *mTitleProperties.font);
  if(mTitleProperties.size) axisTree.put("title_size", *mTitleProperties.size);
  if(mTitleProperties.color) axisTree.put("title_color", *mTitleProperties.color);
  if(mTitleProperties.offset) axisTree.put("title_offset", *mTitleProperties.offset);
  if(mTitleProperties.center) axisTree.put("title_center", *mTitleProperties.center);
  if(mLableProperties.font) axisTree.put("lable_font", *mLableProperties.font);
  if(mLableProperties.size) axisTree.put("lable_size", *mLableProperties.size);
  if(mLableProperties.color) axisTree.put("lable_color", *mLableProperties.color);
  if(mLableProperties.offset) axisTree.put("lable_offset", *mLableProperties.offset);
  if(mLableProperties.center) axisTree.put("lable_center", *mLableProperties.center);
  if(mIsLog) axisTree.put("isLog", *mIsLog);
  if(mIsGrid) axisTree.put("isGrid", *mIsGrid);
  if(mIsOppositeTicks) axisTree.put("isOppositeTicks", *mIsOppositeTicks);
  if(mTimeFormat) axisTree.put("time_format", *mTimeFormat);
  if(mTickOrientation) axisTree.put("tick_orientation", *mTickOrientation);

  return axisTree;
}

//****************************************************************************************
/**
 * Apply all settings from axis on top of this axis.
 */
//****************************************************************************************
void Plot::Pad::Axis::Axis::operator+=(const Axis& axis)
{
  mName = axis.mName;

  if(axis.mRange.min) mRange.min = axis.mRange.min;
  if(axis.mRange.max) mRange.max = axis.mRange.max;

  if(axis.mTitle) mTitle = axis.mTitle;
  if(axis.mNumDivisions) mNumDivisions = axis.mNumDivisions;

  if(axis.mMaxDigits) mMaxDigits = axis.mMaxDigits;
  if(axis.mTickLength) mTickLength = axis.mTickLength;
  if(axis.mAxisColor) mAxisColor = axis.mAxisColor;

  if(axis.mIsLog) mIsLog = axis.mIsLog;
  if(axis.mIsGrid) mIsGrid = axis.mIsGrid;
  if(axis.mIsOppositeTicks) mIsOppositeTicks = axis.mIsOppositeTicks;

  if(axis.mTitleProperties.font) mTitleProperties.font = axis.mTitleProperties.font;
  if(axis.mTitleProperties.size) mTitleProperties.size = axis.mTitleProperties.size;
  if(axis.mTitleProperties.color) mTitleProperties.color = axis.mTitleProperties.color;
  if(axis.mTitleProperties.offset) mTitleProperties.offset = axis.mTitleProperties.offset;
  if(axis.mTitleProperties.center) mTitleProperties.center = axis.mTitleProperties.center;

  if(axis.mLableProperties.font) mLableProperties.font = axis.mLableProperties.font;
  if(axis.mLableProperties.size) mLableProperties.size = axis.mLableProperties.size;
  if(axis.mLableProperties.color) mLableProperties.color = axis.mLableProperties.color;
  if(axis.mLableProperties.offset) mLableProperties.offset = axis.mLableProperties.offset;
  if(axis.mLableProperties.center) mLableProperties.center = axis.mLableProperties.center;

  if(axis.mTimeFormat) mTimeFormat = axis.mTimeFormat;
  if(axis.mTickOrientation) mTickOrientation = axis.mTickOrientation;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// IMPLEMENTATION class Box
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

//****************************************************************************************
/**
 * Constructor of Box
 */
//****************************************************************************************
Plot::Pad::Box::Box(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor)
: Box()
{
  mType = "none";
  mUserCoordinates = userCoordinates;
  mAutoPlacement = autoPlacement;
  mX = x;
  mY = y;
}


//****************************************************************************************
/**
 * Construct box from property tree.
 */
//****************************************************************************************
Plot::Pad::Box::Box(const ptree& boxTree)
{
  try{
    mType = boxTree.get<string>("type");
    mUserCoordinates = boxTree.get<bool>("userCoordinates");
    mAutoPlacement = boxTree.get<bool>("autoPlacement");
    mX = boxTree.get<double>("x");
    mY = boxTree.get<double>("y");
  }catch(...){
    ERROR("Could not construct box from ptree.");
  }
  
  if(auto var = boxTree.get_optional<int16_t>("border_style")) mBorder.style = *var;
  if(auto var = boxTree.get_optional<float_t>("border_width")) mBorder.scale = *var;
  if(auto var = boxTree.get_optional<int16_t>("border_color")) mBorder.color = *var;
  if(auto var = boxTree.get_optional<int16_t>("fill_style")) mFill.style = *var;
  if(auto var = boxTree.get_optional<float_t>("fill_opacity")) mFill.scale = *var;
  if(auto var = boxTree.get_optional<int16_t>("fill_color")) mFill.color = *var;
  if(auto var = boxTree.get_optional<int16_t>("text_style")) mText.style = *var;
  if(auto var = boxTree.get_optional<float_t>("text_size")) mText.scale = *var;
  if(auto var = boxTree.get_optional<int16_t>("text_color")) mText.color = *var;
}

//****************************************************************************************
/**
 * Get property tree representation of box.
 */
//****************************************************************************************
ptree Plot::Pad::Box::GetPropertyTree()
{
  ptree boxTree;
  boxTree.put("type", mType);
  boxTree.put("userCoordinates", mUserCoordinates);
  boxTree.put("autoPlacement", mAutoPlacement);
  boxTree.put("x", mX);
  boxTree.put("y", mY);
  if(mBorder.style) boxTree.put("border_style", *mBorder.style);
  if(mBorder.scale) boxTree.put("border_width", *mBorder.scale);
  if(mBorder.color) boxTree.put("border_color", *mBorder.color);
  if(mFill.style) boxTree.put("fill_style", *mFill.style);
  if(mFill.scale) boxTree.put("fill_opacity", *mFill.scale);
  if(mFill.color) boxTree.put("fill_color", *mFill.color);
  if(mText.style) boxTree.put("text_style", *mText.style);
  if(mText.scale) boxTree.put("text_size", *mText.scale);
  if(mText.color) boxTree.put("text_color", *mText.color);
  return boxTree;
};


} // end namespace PlottingFramework
