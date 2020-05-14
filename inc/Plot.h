// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see docs/Credits
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
  Plot(ptree &plotTree);
  Plot(string name, string figureGroup, string plotTemplateName = "");
  Pad& operator[](uint8_t padID) { return mPads[padID]; }
  void operator+=(const Plot& plot);
  friend Plot operator+(const Plot& templatePlot, const Plot& plot);

  Plot(Plot& plotTemplate, string name, string plotGroup) {*this = plotTemplate; this->mName = name; this->mFigureGroup = plotGroup;}

  // accessors for user
  inline void SetFigureCategory(string figureCategory){mFigureCategory = figureCategory;}
  inline void SetPlotTemplateName(string plotTemplateName){mPlotTemplateName = plotTemplateName;}
  
  inline void SetDimensions(int32_t width, int32_t height, bool fixAspectRatio = false){ mPlotDimensions = {width, height, fixAspectRatio};}
  inline void SetWidth(int32_t width){ mPlotDimensions.width = width;}
  inline void SetHeight(int32_t height){ mPlotDimensions.height = height;}
  inline void SetFixAspectRatio(bool fixAspectRatio = true){ mPlotDimensions.fixAspectRatio = fixAspectRatio;}

  auto SetFill(int16_t color, int16_t style = 1001)  ->decltype(*this);
  auto SetTransparent()  ->decltype(*this);

  
protected:
  friend class PlotManager;
  friend class PlotPainter;
  
  inline void SetFigureGroup(string figureGroup){mFigureGroup = figureGroup;}

  // accessors for internal use by manager and painter
  const string& GetName(){return mName;}
  const string& GetFigureGroup(){return mFigureGroup;}
  const string& GetFigureCategory(){return mFigureCategory;}
  const optional<string>& GetPlotTemplateName(){return mPlotTemplateName;}
  string GetUniqueName(){return mName + gNameGroupSeparator + mFigureGroup + ((mFigureCategory != "") ? ":" + mFigureCategory : "");}
  int GetNumRequiredPads(){return mPads.size();} //TODO: this could return a set of padIDs that need to be present?
  ptree GetPropetyTree();
  
  map<uint8_t, Pad>& GetPads() {return mPads;}
  
  const optional<int32_t>& GetHeight(){return mPlotDimensions.height;}
  const optional<int32_t>& GetWidth(){return mPlotDimensions.width;}
  const optional<bool>& IsFixAspectRatio(){return mPlotDimensions.fixAspectRatio;}

  const optional<int16_t>& GetFillColor(){return mFill.color;}
  const optional<int16_t>& GetFillStyle(){return mFill.style;}

  
private:
  typedef struct{
    optional<int32_t> width;
    optional<int32_t> height;
    optional<bool> fixAspectRatio;
  } dimension_t;
  typedef struct{
    optional<int16_t> color;
    optional<int16_t> style;
  } plot_fill_t;

  
  string mName;
  string mFigureGroup;
  string mFigureCategory;
  optional<string> mPlotTemplateName;
  dimension_t mPlotDimensions;

  plot_fill_t mFill;

  //map<string, vector<vector<uint8_t>>> mLinkedAxes;// [padID, axis] -> {[padID, axis]}
  
  // per thing here one has font, color, etc -> structure textLayout
  // -> how to reduce this info??
  /*
  vector<int> mDefaultColors;
  vector<int> mDefaultMarkers;
  vector<int> mDefaultMarkersFull;
  vector<int> mDefaultMarkersOpen;
  vector<int> mDefaultLineStyles;
  string m2dStyle;
  position_t mTimestampPosition;
  bool mDrawTimestamp;
  */

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

  typedef struct{
    string name;
    string inputIdentifier;
  } input_t;


  Pad() = default;
  Pad(ptree &padTree);
  Axis& operator[](string axis);
  void operator+=(const Pad& pad);
  
  // User accessors:
  void AddFrame(input_t data);
  Data& AddData(input_t input, string lable = "");
  Ratio& AddRatio(input_t numerator, input_t denominator, string lable = "");

  void AddText(double xPos, double yPos, string text = "", bool userCoord = false, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegend(double xPos, double yPos, string title = "", bool userCoordinates = false, int nColumns = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegend(string title = "", int nColumns = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);



  inline void SetPadOptions(string options){mOptions = options;}

  auto SetTitle(string title)  ->decltype(*this);
  auto SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup) ->decltype(*this);
  auto SetMargins(float_t top, float_t bottom, float_t left, float_t right) ->decltype(*this);
  auto SetPalette(int32_t palette)  ->decltype(*this);

  auto SetDefaultTextSize(float_t size)  ->decltype(*this);
  auto SetDefaultTextColor(int16_t color)  ->decltype(*this);
  auto SetDefaultTextFont(int16_t font)  ->decltype(*this);

  auto SetDefaultMarkerSize(float_t size)  ->decltype(*this);
  auto SetDefaultLineWidth(float_t width)  ->decltype(*this);

  auto SetFill(int16_t color, int16_t style = 1001)  ->decltype(*this);
  auto SetTransparent()  ->decltype(*this);

  auto SetFillFrame(int16_t color, int16_t style = 1001)  ->decltype(*this);
  auto SetLineFrame(int16_t color, int16_t style = kSolid, float_t width = 1.f)  ->decltype(*this);
  auto SetTransparentFrame()  ->decltype(*this);

  auto SetRedrawAxes(bool redraw = true)  ->decltype(*this);
  auto SetRefFunc(string refFunc) ->decltype(*this) {mRefFunc = refFunc; return *this;}

  //int GetDefaultColor(int colorIndex) {return mDefaultColors[colorIndex % mDefaultColors.size()];}
  //void SetTextFont(int font){(font > 0 && font < 16) ? mTextFont = font * 10 + 3 : mTextFont = 43;}

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

  const optional<float_t>& GetDefaultMarkerSize(){return mMarkerSize;}
  const optional<float_t>& GetDefaultLineWidth(){return mLineWidth;}

  
  const optional<bool>& GetRedrawAxes(){return mRedrawAxes;}
  const optional<string>& GetRefFunc() {return mRefFunc;}

  
private:
  typedef struct{
    optional<double_t> xlow;
    optional<double_t> ylow;
    optional<double_t> xup;
    optional<double_t> yup;
  } pad_position_t;
  typedef struct{
    optional<float_t> top;
    optional<float_t> bottom;
    optional<float_t> left;
    optional<float_t> right;
  } pad_margin_t;
  typedef struct{
    optional<int16_t> color;
    optional<int16_t> style;
  } pad_fill_t;
  typedef struct{
    optional<int16_t> fillColor;
    optional<int16_t> fillStyle;
    optional<int16_t> lineColor;
    optional<int16_t> lineStyle;
    optional<float_t> lineWidth;
  } frame_t;
  typedef struct{
    optional<float_t> size;
    optional<int16_t> font;
    optional<int16_t> color;
  } text_t;

  
  // properties
  optional<string> mTitle;
  optional<string> mOptions;
  pad_position_t mPosition;
  pad_margin_t mMargins;
  pad_fill_t mFill;
  frame_t mFrame;
  text_t mText;
  optional<float_t> mMarkerSize;
  optional<float_t> mLineWidth;

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

  // default constructor for user
  Data(string name, string inputIdentifier, string lable = "");
  Data(ptree &dataTree);
  // copy constructor
  Data(const Data& otherPlot) = default;

  // user accessors
  virtual auto SetRangeX(double_t min, double_t max) -> decltype(*this);
  virtual auto SetMaxRangeX(double_t max) -> decltype(*this);
  virtual auto SetMinRangeX(double_t min) -> decltype(*this);
  virtual auto SetRangeY(double_t min, double_t max) -> decltype(*this);
  virtual auto SetMaxRangeY(double_t max) -> decltype(*this);
  virtual auto SetMinRangeY(double_t min) -> decltype(*this);
  virtual auto SetLegendLable(string legendLable) -> decltype(*this);
  virtual auto SetOptions(string opions) -> decltype(*this);
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
  virtual auto SetFill(int16_t color, int16_t style) -> decltype(*this);
  virtual auto SetFillColor(int16_t color) -> decltype(*this);
  virtual auto SetFillStyle(int16_t style) -> decltype(*this);
  virtual auto SetFillOpacity(float_t opacity) -> decltype(*this);
  virtual auto SetOptions(drawing_options_t optionAlias) -> decltype(*this);
    
protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual ptree GetPropertyTree();

  void SetType(string type){mType = type;}

  string GetUniqueName(){return mName + gNameGroupSeparator + mInputIdentifier;}

  const string& GetType(){return mType;}
  const string& GetName() {return mName;}
  const string& GetInputIdentifier(){return mInputIdentifier;}
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

private:
  Data() = default;
  
  string mType; // for introspection: "data" or "ratio"
  string mName;
  string mInputIdentifier;
  
  optional<string> mLegendLable;
  optional<string> mDrawingOptions;
  optional<drawing_options_t> mDrawingOptionAlias;
  optional<double_t> mScaleFactor;

  typedef struct{
    optional<int16_t> color;
    optional<int16_t> style;
    optional<float_t> scale;  // marker size , line width, fill opacity
  } dataLayout_t;
  
  typedef struct{
    optional<double_t> min;
    optional<double_t> max;
  } dataRange_t;

  
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
  Ratio(string name, string inputIdentifier, string denomName, string denomInputIdentifier, string lable);
  Ratio(ptree &dataTree);

  virtual ~Ratio() = default;
  Ratio(const Ratio& otherRatio) = default;

  auto SetDivideMethod(string divideMethod) -> decltype(*this);

  // return correct type for the data accessors
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
  virtual auto SetLegendLable(string legendLable) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetLegendLable(legendLable));}
  virtual auto SetOptions(string opions) -> decltype(*this)
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
  virtual auto SetFill(int16_t color, int16_t style) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFill(color, style));}
  virtual auto SetFillColor(int16_t color) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFillColor(color));}
  virtual auto SetFillStyle(int16_t style) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFillStyle(style));}
  virtual auto SetFillOpacity(float_t opacity) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetFillOpacity(opacity));}
  virtual auto SetOptions(drawing_options_t optionAlias) -> decltype(*this)
  {return dynamic_cast<decltype(*this)&>(Data::SetOptions(optionAlias));}

protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropertyTree();
  string GetDenomIdentifier(){return mDenomInputIdentifier;}
  string GetDenomName(){return mDenomName;}
  string GetUniqueNameDenom(){return mDenomName + gNameGroupSeparator + mDenomInputIdentifier;}

  
private:
  string mDenomName;
  string mDenomInputIdentifier;
  string mDivideMethod;  // "tspline3", "binomial", etc.
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

  Axis(string axisName);
  Axis(ptree &axisTree);
  
  void operator+=(const Axis& axis);


  auto SetTitle(string title) ->decltype(*this) {mTitle = title; return *this;}
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


  
private:
  
  typedef struct{
    optional<int16_t> font;
    optional<float_t> size;
    optional<int16_t> color;
    optional<float_t> offset;
    optional<bool> center;
  } axisTextProperties_t;
  
  typedef struct{
    optional<double_t> min;
    optional<double_t> max;
  } axisRange_t;

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
  Box(const Box& otherBox) = default;
  Box(Box& otherBox) = default;
  
  Box(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor)
  : mType("none"), mUserCoordinates(userCoordinates), mAutoPlacement(autoPlacement), mX(x), mY(y), mBorderStyle(borderStyle), mBorderSize(borderSize), mBorderColor(borderColor)
  {
    // constructor code
  }
  double GetXPosition(){return mX;}
  double GetYPosition(){return mY;}
  int GetBorderStyle(){return mBorderStyle;}
  int GetBorderSize(){return mBorderSize;}
  int GetBorderColor(){return mBorderColor;}
  bool IsUserCoordinates(){return mUserCoordinates;}
  bool IsAutoPlacement(){return mAutoPlacement;}
  
  const string& GetType(){return mType;}
  
  virtual ptree GetPropertyTree(){
    ptree boxTree;
    boxTree.put("type", mType);
    boxTree.put("userCoordinates", mUserCoordinates);
    boxTree.put("autoPlacement", mAutoPlacement);
    boxTree.put("x", mX);
    boxTree.put("y", mY);
    boxTree.put("borderStyle", mBorderStyle);
    boxTree.put("borderSize", mBorderSize);
    boxTree.put("borderColor", mBorderColor);
    return boxTree;
  };
  
  // constructor to define entry from file
  Box(ptree &boxTree){
    try{
      mType = boxTree.get<string>("type");
      mUserCoordinates = boxTree.get<bool>("userCoordinates");
      mAutoPlacement = boxTree.get<bool>("autoPlacement");
      mX = boxTree.get<double>("x");
      mY = boxTree.get<double>("y");
      mBorderStyle = boxTree.get<int>("borderStyle");
      mBorderSize = boxTree.get<int>("borderSize");
      mBorderColor = boxTree.get<int>("borderColor");
    }catch(...){
      ERROR("Could not construct box from ptree.");
    }
    
  }
  
  
protected:
  void SetType(string type){mType = type;}
private:
  string mType; // for introspection
  bool mUserCoordinates;
  bool mAutoPlacement;
  double mX;
  double mY;
  int mBorderStyle;
  int mBorderSize;
  int mBorderColor;
};


//****************************************************************************************
/**
 * Representation of a text box.
 */
//****************************************************************************************
class Plot::Pad::TextBox : public Plot::Pad::Box {
public:
  TextBox(const TextBox& otherTextBox) = default;
  TextBox(TextBox& otherTextBox) = default;
  virtual ~TextBox() = default;
  
  TextBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, string text)
  : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mText(text)
  {
    SetType("text");
    // constructor code
  }
  string GetText(){return mText;}
  void SetText(string text);
  //void SetDelimiter(string delimiter){mDelimiter = delimiter;}
  
  ptree GetPropertyTree(){
    ptree boxTree = Box::GetPropertyTree();
    boxTree.put("text", mText);
    return boxTree;
  };
  
  // constructor to define entry from file
  TextBox(ptree &boxTree) : Box(boxTree)
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
  LegendBox(LegendBox& otherLegendBox) = default;
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
  LegendBox(ptree &boxTree) : Box(boxTree)
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
