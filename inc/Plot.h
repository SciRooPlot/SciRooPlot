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

#ifndef Plot_h
#define Plot_h

#include "PlottingFramework.h"

using namespace PlottingFramework;
namespace PlottingFramework {

//****************************************************************************************
/**
 * Class for internal representation of a plot.
 */
//****************************************************************************************
class Plot
{
public:
  class Pad;

  Plot() = default;
  Plot(const ptree& plotTree);
  Plot(const string& name, const string& figureGroup, const string& plotTemplateName = "");
  Pad& operator[](uint8_t padID) { return mPads[padID]; }
  void operator+=(const Plot& plot);
  friend Plot operator+(const Plot& templatePlot, const Plot& plot);

  Plot(const Plot& plotTemplate, const string& name, const string& plotGroup) {*this = plotTemplate; this->mName = name; this->mFigureGroup = plotGroup;}

  // accessors for user
  inline void SetFigureCategory(const string& figureCategory){mFigureCategory = figureCategory;}
  inline void SetPlotTemplateName(const string& plotTemplateName){mPlotTemplateName = plotTemplateName;}
  
  inline void SetDimensions(int32_t width, int32_t height, bool fixAspectRatio = false){ mPlotDimensions = {width, height, fixAspectRatio};}
  inline void SetWidth(int32_t width){ mPlotDimensions.width = width;}
  inline void SetHeight(int32_t height){ mPlotDimensions.height = height;}
  inline void SetFixAspectRatio(bool fixAspectRatio = true){ mPlotDimensions.fixAspectRatio = fixAspectRatio;}

  auto SetFill(int16_t color, int16_t style = 1001)  ->decltype(*this);
  auto SetTransparent()  ->decltype(*this);

protected:
  friend class PlotManager;
  friend class PlotPainter;
  
  inline void SetFigureGroup(const string& figureGroup){mFigureGroup = figureGroup;}

  // accessors for internal use by manager and painter
  const string& GetName(){return mName;}
  const string& GetFigureGroup(){return mFigureGroup;}
  const string& GetFigureCategory(){return mFigureCategory;}
  const optional<string>& GetPlotTemplateName(){return mPlotTemplateName;}
  string GetUniqueName(){return mName + gNameGroupSeparator + mFigureGroup + ((mFigureCategory != "") ? ":" + mFigureCategory : "");}
  ptree GetPropetyTree();
  
  map<uint8_t, Pad>& GetPads() {return mPads;}
  
  const optional<int32_t>& GetHeight(){return mPlotDimensions.height;}
  const optional<int32_t>& GetWidth(){return mPlotDimensions.width;}
  const optional<bool>& IsFixAspectRatio(){return mPlotDimensions.fixAspectRatio;}

  const optional<int16_t>& GetFillColor(){return mFill.color;}
  const optional<int16_t>& GetFillStyle(){return mFill.style;}

private:
  struct dimension_t{
    optional<int32_t> width;
    optional<int32_t> height;
    optional<bool> fixAspectRatio;
  };
  struct plot_fill_t{
    optional<int16_t> color;
    optional<int16_t> style;
  };

  string mName;
  string mFigureGroup;
  string mFigureCategory;
  optional<string> mPlotTemplateName;
  dimension_t mPlotDimensions;

  plot_fill_t mFill;

  map<uint8_t, Pad> mPads;
};

//****************************************************************************************
/**
 * Representation of a pad.
 */
//****************************************************************************************
class Plot::Pad
{
public:
  class Data;
  class Ratio;
  class Axis;
  class Box;
  class TextBox;
  class LegendBox;

  struct input_t{
    string name;
    string inputIdentifier;
  };

  Pad() = default;
  Pad(const ptree &padTree);
  Axis& operator[](string axis);
  void operator+=(const Pad& pad);
  
  // User accessors:
  Data& AddData(const input_t& input, const string& lable = "");
  Ratio& AddRatio(const input_t& numerator, const input_t& denominator, const string& lable = "");

  
  // FIXME: These accessors will be simplified and properties like borderStyle etc can be set with extra functions directly on the boxes (as it is for Pad and Data properties)
  void AddText(double xPos, double yPos, const string& text = "", bool userCoord = false, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegend(double xPos, double yPos, const string& title = "", bool userCoordinates = false, int nColumns = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegend(const string& title = "", int nColumns = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);

  inline void SetPadOptions(const string& options){mOptions = options;}

  auto SetTitle(const string& title)  ->decltype(*this);
  auto SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup) ->decltype(*this);
  auto SetMargins(float_t top, float_t bottom, float_t left, float_t right) ->decltype(*this);
  auto SetPalette(int32_t palette)  ->decltype(*this);

  auto SetDefaultTextSize(float_t size)  ->decltype(*this);
  auto SetDefaultTextColor(int16_t color)  ->decltype(*this);
  auto SetDefaultTextFont(int16_t font)  ->decltype(*this);

  auto SetDefaultMarkerSize(float_t size)  ->decltype(*this);
  auto SetDefaultMarkerColors(const vector<int16_t>& colors)  ->decltype(*this);
  auto SetDefaultMarkerStyles(const vector<int16_t>& styles)  ->decltype(*this);

  auto SetDefaultLineWidth(float_t width)  ->decltype(*this);
  auto SetDefaultLineColors(const vector<int16_t>& colors)  ->decltype(*this);
  auto SetDefaultLineStyles(const vector<int16_t>& styles)  ->decltype(*this);

  auto SetDefaultFillOpacity(float_t opacity)  ->decltype(*this);
  auto SetDefaultFillColors(const vector<int16_t>& colors)  ->decltype(*this);
  auto SetDefaultFillStyles(const vector<int16_t>& styles)  ->decltype(*this);
  
  auto SetDefaultDrawingOptionGraph(drawing_options_t drawingOption)  ->decltype(*this);
  auto SetDefaultDrawingOptionHist(drawing_options_t drawingOption)  ->decltype(*this);
  auto SetDefaultDrawingOptionHist2d(drawing_options_t drawingOption)  ->decltype(*this);

  auto SetFill(int16_t color, int16_t style = 1001)  ->decltype(*this);
  auto SetTransparent()  ->decltype(*this);

  auto SetFillFrame(int16_t color, int16_t style = 1001)  ->decltype(*this);
  auto SetLineFrame(int16_t color, int16_t style = kSolid, float_t width = 1.f)  ->decltype(*this);
  auto SetTransparentFrame()  ->decltype(*this);

  auto SetRedrawAxes(bool redraw = true)  ->decltype(*this);
  auto SetRefFunc(const string& refFunc) ->decltype(*this) {mRefFunc = refFunc; return *this;}

protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropetyTree();

  vector<shared_ptr<Data>>& GetData(){return mData;}
  vector<shared_ptr<Box>>& GetBoxes(){return mBoxes;}

  const map<string, Axis>& GetAxes(){return mAxes;}

  const optional<string>& GetTitle(){return mTitle;}
  const optional<string>& GetOptions(){return mOptions;}

  const optional<double_t>& GetXLow(){return mPosition.xlow;}
  const optional<double_t>& GetYLow(){return mPosition.ylow;}
  const optional<double_t>& GetXUp(){return mPosition.xup;}
  const optional<double_t>& GetYUp(){return mPosition.yup;}

  const optional<float_t>& GetMarginTop(){return mMargins.top;}
  const optional<float_t>& GetMarginBottom(){return mMargins.bottom;}
  const optional<float_t>& GetMarginLeft(){return mMargins.left;}
  const optional<float_t>& GetMarginRight(){return mMargins.right;}

  const optional<int32_t>& GetPalette(){return mPalette;}

  const optional<int16_t>& GetFillColor(){return mFill.color;}
  const optional<int16_t>& GetFillStyle(){return mFill.style;}

  const optional<int16_t>& GetFillColorFrame(){return mFrame.fillColor;}
  const optional<int16_t>& GetFillStyleFrame(){return mFrame.fillStyle;}
  const optional<int16_t>& GetLineColorFrame(){return mFrame.lineColor;}
  const optional<int16_t>& GetLineStyleFrame(){return mFrame.lineStyle;}
  const optional<float_t>& GetLineWidthFrame(){return mFrame.lineWidth;}

  const optional<int16_t>& GetDefaultTextColor(){return mText.color;}
  const optional<int16_t>& GetDefaultTextFont(){return mText.font;}
  const optional<float_t>& GetDefaultTextSize(){return mText.size;}

  const optional<float_t>& GetDefaultMarkerSize(){return mMarkerDefaults.scale;}
  const optional<float_t>& GetDefaultLineWidth(){return mLineDefaults.scale;}
  const optional<float_t>& GetDefaultFillOpacity(){return mFillDefaults.scale;}

  const optional<vector<int16_t>>& GetDefaultMarkerColors() {return mMarkerDefaults.colors;}
  const optional<vector<int16_t>>& GetDefaultLineColors() {return mLineDefaults.colors;}
  const optional<vector<int16_t>>& GetDefaultFillColors() {return mFillDefaults.colors;}
  const optional<vector<int16_t>>& GetDefaultMarkerStyles() {return mMarkerDefaults.styles;}
  const optional<vector<int16_t>>& GetDefaultLineStyles() {return mLineDefaults.styles;}
  const optional<vector<int16_t>>& GetDefaultFillStyles() {return mFillDefaults.styles;}

  const optional<drawing_options_t>& GetDefaultDrawingOptionGraph() {return mDrawingOptionDefaults.graph;}
  const optional<drawing_options_t>& GetDefaultDrawingOptionHist() {return mDrawingOptionDefaults.hist;}
  const optional<drawing_options_t>& GetDefaultDrawingOptionHist2d() {return mDrawingOptionDefaults.hist2d;}

  const optional<bool>& GetRedrawAxes(){return mRedrawAxes;}
  const optional<string>& GetRefFunc() {return mRefFunc;}

private:
  string VectorToString(vector<int16_t> numbers);
  vector<int16_t> StringToVector(string numberString);

  struct pad_position_t{
    optional<double_t> xlow;
    optional<double_t> ylow;
    optional<double_t> xup;
    optional<double_t> yup;
  };
  struct pad_margin_t{
    optional<float_t> top;
    optional<float_t> bottom;
    optional<float_t> left;
    optional<float_t> right;
  };
  struct pad_fill_t{
    optional<int16_t> color;
    optional<int16_t> style;
  };
  struct frame_t{
    optional<int16_t> fillColor;
    optional<int16_t> fillStyle;
    optional<int16_t> lineColor;
    optional<int16_t> lineStyle;
    optional<float_t> lineWidth;
  };
  struct text_t{
    optional<float_t> size;
    optional<int16_t> font;
    optional<int16_t> color;
  };
  struct view_defaults_t{
    optional<float_t> scale;
    optional<vector<int16_t>> styles;
    optional<vector<int16_t>> colors;
  };
  struct data_defaults_t{
    optional<drawing_options_t> graph;
    optional<drawing_options_t> hist;
    optional<drawing_options_t> hist2d;
  };
  
  // properties
  optional<string> mTitle;
  optional<string> mOptions;
  pad_position_t mPosition;
  pad_margin_t mMargins;
  pad_fill_t mFill;
  frame_t mFrame;
  text_t mText;
  
  // user defined default data representation
  view_defaults_t mMarkerDefaults;
  view_defaults_t mLineDefaults;
  view_defaults_t mFillDefaults;
  data_defaults_t mDrawingOptionDefaults;

  optional<int32_t> mPalette;

  optional<bool> mRedrawAxes;
  optional<string> mRefFunc;

  map<string, Axis> mAxes;
  vector<shared_ptr<Data>>  mData;
  vector<shared_ptr<Box>>   mBoxes;
};

//****************************************************************************************
/**
 * Representation of drawable data.
 */
//****************************************************************************************
class Plot::Pad::Data
{
public:
  Data() = default;

  // default constructor for user
  Data(const string& name, const string& inputIdentifier, const string& lable = "");
  Data(const ptree &dataTree);
  // copy constructor
  Data(const Data& otherData) = default;

  // user accessors
  virtual auto SetLayout(const Data& dataLayout) -> decltype(*this);
  virtual auto SetRangeX(double_t min, double_t max) -> decltype(*this);
  virtual auto SetMaxRangeX(double_t max) -> decltype(*this);
  virtual auto SetMinRangeX(double_t min) -> decltype(*this);
  virtual auto SetRangeY(double_t min, double_t max) -> decltype(*this);
  virtual auto SetMaxRangeY(double_t max) -> decltype(*this);
  virtual auto SetMinRangeY(double_t min) -> decltype(*this);
  virtual auto SetLegendLable(const string& legendLable) -> decltype(*this);
  virtual auto SetOptions(const string& opions) -> decltype(*this);
  virtual auto SetScaleFactor(double_t scale) -> decltype(*this);
  virtual auto SetColor(int16_t color) -> decltype(*this);
  virtual auto SetMarker(int16_t color, int16_t style, float_t size) -> decltype(*this);
  virtual auto SetMarkerColor(int16_t color) -> decltype(*this);
  virtual auto SetMarkerStyle(int16_t style) -> decltype(*this);
  virtual auto SetMarkerSize(float_t size) -> decltype(*this);
  virtual auto SetLine(int16_t color, int16_t style, float_t width) -> decltype(*this);
  virtual auto SetLineColor(int16_t color) -> decltype(*this);
  virtual auto SetLineStyle(int16_t style) -> decltype(*this);
  virtual auto SetLineWidth(float_t width) -> decltype(*this);
  virtual auto SetFill(int16_t color, int16_t style, float_t opacity = 1.) -> decltype(*this);
  virtual auto SetFillColor(int16_t color) -> decltype(*this);
  virtual auto SetFillStyle(int16_t style) -> decltype(*this);
  virtual auto SetFillOpacity(float_t opacity) -> decltype(*this);
  virtual auto SetOptions(drawing_options_t optionAlias) -> decltype(*this);

  virtual auto SetDefinesFrame() -> decltype(*this);

  auto SetInputID(const string& inputIdentifier) -> decltype(*this) {mInputIdentifier = inputIdentifier; return *this;}
  const string& GetInputID(){return mInputIdentifier;}

protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual ptree GetPropertyTree();

  void SetType(const string& type){mType = type;}

  string GetUniqueName(){return mName + gNameGroupSeparator + mInputIdentifier;}

  const string& GetType(){return mType;}
  const string& GetName() {return mName;}
  const optional<string>& GetLegendLable(){return mLegendLable;}
    
  const optional<int16_t>& GetMarkerColor(){return mMarker.color;}
  const optional<int16_t>& GetMarkerStyle(){return mMarker.style;}
  const optional<float_t>& GetMarkerSize(){return mMarker.scale;}

  const optional<int16_t>& GetLineColor(){return mLine.color;}
  const optional<int16_t>& GetLineStyle(){return mLine.style;}
  const optional<float_t>& GetLineWidth(){return mLine.scale;}

  const optional<int16_t>& GetFillColor(){return mFill.color;}
  const optional<int16_t>& GetFillStyle(){return mFill.style;}
  const optional<float_t>& GetFillOpacity(){return mFill.scale;}

  const optional<string>& GetDrawingOptions(){return mDrawingOptions;}
  const optional<drawing_options_t>& GetDrawingOptionAlias(){return mDrawingOptionAlias;}
  const optional<double_t>& GetScaleFactor(){return mScaleFactor;}
  
  const optional<double_t>& GetMinRangeX(){return mRangeX.min;}
  const optional<double_t>& GetMaxRangeX(){return mRangeX.max;}
  const optional<double_t>& GetMinRangeY(){return mRangeY.min;}
  const optional<double_t>& GetMaxRangeY(){return mRangeY.max;}

  const bool& GetDefinesFrame(){return mDefinesFrame;}

private:

  bool mDefinesFrame;

  string mType; // for introspection: "data" or "ratio"
  string mName;
  string mInputIdentifier;
  
  optional<string> mLegendLable;
  optional<string> mDrawingOptions;
  optional<drawing_options_t> mDrawingOptionAlias;
  optional<double_t> mScaleFactor;

  struct dataLayout_t{
    optional<int16_t> color;
    optional<int16_t> style;
    optional<float_t> scale;  // marker size , line width, fill opacity
  };
  
  struct dataRange_t{
    optional<double_t> min;
    optional<double_t> max;
  };

  dataLayout_t mMarker;
  dataLayout_t mLine;
  dataLayout_t mFill;
  dataRange_t mRangeX;
  dataRange_t mRangeY;
};

//****************************************************************************************
/**
 * Representation of a Ratio.
 */
//****************************************************************************************
class Plot::Pad::Ratio : public Plot::Pad::Data
{
public:
  Ratio(const string& name, const string& inputIdentifier, const string& denomName, const string& denomInputIdentifier, const string& lable);
  Ratio(const ptree &dataTree);

  virtual ~Ratio() = default;
  Ratio(const Ratio& otherRatio) = default;

  auto SetIsCorrelated(bool isCorrelated = true) -> decltype(*this);

  // return correct type for the data accessors
  virtual auto SetLayout(const Data& dataLayout) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLayout(dataLayout));}
  virtual auto SetRangeX(double_t min, double_t max) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetRangeX(min, max));}
  virtual auto SetMaxRangeX(double_t max) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMaxRangeX(max));}
  virtual auto SetMinRangeX(double_t min) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMinRangeX(min));}
  virtual auto SetRangeY(double_t min, double_t max) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetRangeY(min, max));}
  virtual auto SetMaxRangeY(double_t max) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMaxRangeY(max));}
  virtual auto SetMinRangeY(double_t min) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMinRangeY(min));}
  virtual auto SetLegendLable(const string& legendLable) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLegendLable(legendLable));}
  virtual auto SetOptions(const string& opions) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetOptions(opions));}
  virtual auto SetScale(double_t scale) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetScaleFactor(scale));}
  virtual auto SetColor(int16_t color) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetColor(color));}
  virtual auto SetMarker(int16_t color, int16_t style, float_t size) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMarker(color, style, size));}
  virtual auto SetMarkerColor(int16_t color) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMarkerColor(color));}
  virtual auto SetMarkerStyle(int16_t style) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMarkerStyle(style));}
  virtual auto SetMarkerSize(float_t size) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetMarkerSize(size));}
  virtual auto SetLine(int16_t color, int16_t style, float_t width) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLine(color, style, width));}
  virtual auto SetLineColor(int16_t color) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLineColor(color));}
  virtual auto SetLineStyle(int16_t style) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLineStyle(style));}
  virtual auto SetLineWidth(float_t width) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLineWidth(width));}
  virtual auto SetFill(int16_t color, int16_t style, float_t opacity = 1.) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFill(color, style, opacity));}
  virtual auto SetFillColor(int16_t color) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFillColor(color));}
  virtual auto SetFillStyle(int16_t style) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFillStyle(style));}
  virtual auto SetFillOpacity(float_t opacity) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFillOpacity(opacity));}
  virtual auto SetOptions(drawing_options_t optionAlias) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetOptions(optionAlias));}
  virtual auto SetDefinesFrame() -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetDefinesFrame());}

protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropertyTree();
  string GetDenomIdentifier(){return mDenomInputIdentifier;}
  string GetDenomName(){return mDenomName;}
  string GetUniqueNameDenom(){return mDenomName + gNameGroupSeparator + mDenomInputIdentifier;}

  const bool& GetIsCorrelated() {return mIsCorrelated;}

private:
  string mDenomName;
  string mDenomInputIdentifier;
  bool mIsCorrelated;
};

//****************************************************************************************
/**
 * Representation of an axis.
 */
//****************************************************************************************
class Plot::Pad::Axis {
public:
  
  Axis() =  default;
  Axis(const Axis& otherAxis) = default;

  Axis(const string& axisName);
  Axis(const ptree &axisTree);
  
  void operator+=(const Axis& axis);


  auto SetTitle(const string& title) ->decltype(*this) {mTitle = title; return *this;}
  auto SetRange(double_t min, double_t max) ->decltype(*this) {mRange = {min, max}; return *this;}
  auto SetMaxRange(double_t max) ->decltype(*this) {mRange.max = max; return *this;}
  auto SetMinRange(double_t min) ->decltype(*this) {mRange.min = min; return *this;}
  auto SetColor(int16_t color) ->decltype(*this) {mAxisColor = color; mLableProperties.color = color; mTitleProperties.color = color; return *this;}
  auto SetAxisColor(int16_t color) ->decltype(*this) {mAxisColor = color; return *this;}
  auto SetNumDivisions(int32_t numDivisions) ->decltype(*this) {mNumDivisions = numDivisions; return *this;}
  auto SetMaxDigits(int32_t maxDigtis) ->decltype(*this) {mMaxDigits = maxDigtis; return *this;}

  auto SetTickLength(float_t tickLength) ->decltype(*this) {mTickLength = tickLength; return *this;}

  auto SetTitleFont(int16_t font) ->decltype(*this) {mTitleProperties.font = font; return *this;}
  auto SetLableFont(int16_t font) ->decltype(*this) {mLableProperties.font = font; return *this;}

  auto SetTitleSize(float_t size) ->decltype(*this) {mTitleProperties.size = size; return *this;}
  auto SetLableSize(float_t size) ->decltype(*this) {mLableProperties.size = size; return *this;}

  auto SetTitleColor(int16_t color) ->decltype(*this) {mTitleProperties.color = color; return *this;}
  auto SetLableColor(int16_t color) ->decltype(*this) {mLableProperties.color = color; return *this;}

  auto SetTitleOffset(float_t offset) ->decltype(*this) {mTitleProperties.offset = offset; return *this;}
  auto SetLableOffset(float_t offset) ->decltype(*this) {mLableProperties.offset = offset; return *this;}

  auto SetTitleCenter(bool center = true) ->decltype(*this) {mTitleProperties.center = center; return *this;}
  auto SetLableCenter(bool center = true) ->decltype(*this) {mLableProperties.center = center; return *this;}

  auto SetLog(bool isLog = true) ->decltype(*this) {mIsLog = isLog; return *this;}
  auto SetGrid(bool isGrid = true) ->decltype(*this) {mIsGrid = isGrid; return *this;}
  auto SetOppositeTicks(bool isOppositeTicks = true) ->decltype(*this) {mIsOppositeTicks = isOppositeTicks; return *this;}
  auto SetTimeFormat(const string& timeFormat) ->decltype(*this) {mTimeFormat = timeFormat; return *this;}
  auto SetTickOrientation(const string& tickOrientation) ->decltype(*this) {mTickOrientation = tickOrientation; return *this;}
  
protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropertyTree();
  
  optional<double_t> GetMinRange() {return mRange.min;}
  optional<double_t> GetMaxRange() {return mRange.max;}

  const optional<float_t>& GetTickLength() {return mTickLength;}
  const optional<int32_t>& GetNumDivisions() {return mNumDivisions;}
  const optional<int32_t>& GetMaxDigits() {return mMaxDigits;}

  const optional<int16_t>& GetAxisColor()   {return mAxisColor;}
  const optional<string>& GetTitle() {return mTitle;}

  const optional<int16_t>& GetTitleFont() {return mTitleProperties.font;}
  const optional<int16_t>& GetLableFont() {return mLableProperties.font;}

  const optional<float_t>& GetTitleSize() {return mTitleProperties.size;}
  const optional<float_t>& GetLableSize() {return mLableProperties.size;}

  const optional<int16_t>& GetTitleColor() {return mTitleProperties.color;}
  const optional<int16_t>& GetLableColor() {return mLableProperties.color;}

  const optional<float_t>& GetTitleOffset() {return mTitleProperties.offset;}
  const optional<float_t>& GetLableOffset() {return mLableProperties.offset;}

  const optional<bool>& GetTitleCenter() {return mTitleProperties.center;}
  const optional<bool>& GetLableCenter() {return mLableProperties.center;}

  const optional<bool>& GetLog() {return mIsLog;}
  const optional<bool>& GetGrid() {return mIsGrid;}
  const optional<bool>& GetOppositeTicks() {return mIsOppositeTicks;}

  const optional<string>& GetTimeFormat() {return mTimeFormat;}
  const optional<string>& GetTickOrientation() {return mTickOrientation;}

private:
  
  struct axisTextProperties_t{
    optional<int16_t> font;
    optional<float_t> size;
    optional<int16_t> color;
    optional<float_t> offset;
    optional<bool> center;
  };
  struct axisRange_t{
    optional<double_t> min;
    optional<double_t> max;
  };

  string mName;
  axisRange_t mRange;
  optional<string>  mTitle;
  optional<int32_t> mNumDivisions;
  optional<int32_t> mMaxDigits;
  optional<float_t> mTickLength;
  optional<int16_t> mAxisColor;

  optional<bool> mIsLog;
  optional<bool> mIsGrid;
  optional<bool> mIsOppositeTicks;
  optional<string>  mTimeFormat;
  optional<string>  mTickOrientation;

  axisTextProperties_t mTitleProperties;
  axisTextProperties_t mLableProperties;
};

//****************************************************************************************
/**
 * Representation of a box.
 */
//****************************************************************************************
class Plot::Pad::Box {
public:

  Box() =  default;
  Box(const Box& otherBox) = default;
  Box(const ptree &boxTree);
  Box(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor);
  
protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual ptree GetPropertyTree();
  void SetType(const string& type){mType = type;}
  const string& GetType(){return mType;}

  double_t GetXPosition(){return mX;}
  double_t GetYPosition(){return mY;}
  optional<int16_t>& GetBorderStyle(){return mBorder.style;}
  optional<float_t>& GetBorderWidth(){return mBorder.scale;}
  optional<int16_t>& GetBorderColor(){return mBorder.color;}

  optional<int16_t>& GetFillStyle(){return mFill.style;}
  optional<float_t>& GetFillOpacity(){return mFill.scale;}
  optional<int16_t>& GetFillColor(){return mFill.color;}

  optional<int16_t>& GetTextFont(){return mText.style;}
  optional<float_t>& GetTextSize(){return mText.scale;}
  optional<int16_t>& GetTextColor(){return mText.color;}

  bool IsUserCoordinates(){return mUserCoordinates;}
  bool IsAutoPlacement(){return mAutoPlacement;}
  
private:
  
  struct layout_t{
    optional<int16_t> color;
    optional<int16_t> style;
    optional<float_t> scale;  // marker size , line width, fill opacity
  };

  string mType; // for introspection
  layout_t mText;
  layout_t mBorder;
  layout_t mFill;

  bool mUserCoordinates;
  bool mAutoPlacement;
  double_t mX;
  double_t mY;
};

//****************************************************************************************
/**
 * Representation of a text box.
 */
//****************************************************************************************
class Plot::Pad::TextBox : public Plot::Pad::Box {
public:
  TextBox(const TextBox& otherTextBox) = default;
  virtual ~TextBox() = default;
  
  TextBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, const string& text)
  : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mText(text)
  {
    SetType("text");
    // constructor code
  }
  string GetText(){return mText;}
  void SetText(const string& text) {mText = text;}
  //void SetDelimiter(string delimiter){mDelimiter = delimiter;}
  
  ptree GetPropertyTree(){
    ptree boxTree = Box::GetPropertyTree();
    boxTree.put("text", mText);
    return boxTree;
  };
  
  // constructor to define entry from file
  TextBox(const ptree &boxTree) : Box(boxTree)
  {
    try{
      mText = boxTree.get<string>("text");
    }catch(...){
      ERROR("Could not construct textbox from ptree.");
    }
  }
  
private:
  int GetNumLines();
  //string mDelimiter;
  string mText;
};


//****************************************************************************************
/**
 * Representation of a legend box.
 */
//****************************************************************************************
class Plot::Pad::LegendBox : public Plot::Pad::Box {
public:
  LegendBox(const LegendBox& otherLegendBox) = default;
  virtual ~LegendBox() = default;
  
  LegendBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, string title, int nColumns)
  : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mTitle(title), mNumColumns(nColumns)
  {
    SetType("legend");
    // constructor code
  }
  string& GetTitle(){return mTitle;}
  int GetNumColumns(){return mNumColumns;}
  
  ptree GetPropertyTree(){
    ptree boxTree = Box::GetPropertyTree();
    boxTree.put("title", mTitle);
    boxTree.put("numColumns", mNumColumns);
    return boxTree;
  };
  
  // constructor to define entry from file
  LegendBox(const ptree &boxTree) : Box(boxTree)
  {
    try{
      mTitle = boxTree.get<string>("title");
      mNumColumns = boxTree.get<int>("numColumns");
    }catch(...){
      ERROR("Could not construct legendbox from ptree.");
    }
  }
  
private:
  int GetNumEntries();
  string mTitle;
  int mNumColumns;
};

} // end namespace PlottingFramework
#endif /* Plot_h */
