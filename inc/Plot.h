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
#include "HelperFunctions.h"

using namespace PlottingFramework;
namespace PlottingFramework
{

//**************************************************************************************************
/**
 * Class for internal representation of a plot.
 */
//**************************************************************************************************
class Plot
{
 public:
  class Pad;

  Plot() = default;
  Plot(const ptree& plotTree);
  Plot(const string& name, const string& figureGroup, const string& plotTemplateName = "");
  Pad& operator[](uint8_t padID)
  {
    return mPads[padID];
  }
  Pad& GetPad(uint8_t padID)
  {
    return mPads[padID];
  }
  void operator+=(const Plot& plot);
  friend Plot operator+(const Plot& templatePlot, const Plot& plot);
  Plot(const Plot& otherPlot, const string& name, const string& plotGroup);
  Plot Clone() const;

  // accessors for user
  inline void SetFigureCategory(const string& figureCategory)
  {
    mFigureCategory = figureCategory;
  }
  inline void SetPlotTemplateName(const string& plotTemplateName)
  {
    mPlotTemplateName = plotTemplateName;
  }

  inline void SetDimensions(int32_t width, int32_t height, bool fixAspectRatio = false)
  {
    mPlotDimensions = { width, height, fixAspectRatio };
  }
  inline void SetWidth(int32_t width)
  {
    mPlotDimensions.width = width;
  }
  inline void SetHeight(int32_t height)
  {
    mPlotDimensions.height = height;
  }
  inline void SetFixAspectRatio(bool fixAspectRatio = true)
  {
    mPlotDimensions.fixAspectRatio = fixAspectRatio;
  }

  auto SetFill(int16_t color, int16_t style = 1001) -> decltype(*this);
  auto SetTransparent() -> decltype(*this);

 protected:
  friend class PlotManager;
  friend class PlotPainter;

  inline void SetFigureGroup(const string& figureGroup)
  {
    mFigureGroup = figureGroup;
  }

  // accessors for internal use by manager and painter
  const string& GetName()
  {
    return mName;
  }
  const string& GetFigureGroup()
  {
    return mFigureGroup;
  }
  const string& GetFigureCategory()
  {
    return mFigureCategory;
  }
  const optional<string>& GetPlotTemplateName()
  {
    return mPlotTemplateName;
  }
  string GetUniqueName()
  {
    return mName + gNameGroupSeparator + mFigureGroup
           + ((mFigureCategory != "") ? ":" + mFigureCategory : "");
  }
  ptree GetPropetyTree();

  map<uint8_t, Pad>& GetPads()
  {
    return mPads;
  }

  const optional<int32_t>& GetHeight()
  {
    return mPlotDimensions.height;
  }
  const optional<int32_t>& GetWidth()
  {
    return mPlotDimensions.width;
  }
  const optional<bool>& IsFixAspectRatio()
  {
    return mPlotDimensions.fixAspectRatio;
  }

  const optional<int16_t>& GetFillColor()
  {
    return mFill.color;
  }
  const optional<int16_t>& GetFillStyle()
  {
    return mFill.style;
  }

 private:
  struct dimension_t
  {
    optional<int32_t> width;
    optional<int32_t> height;
    optional<bool> fixAspectRatio;
  };
  struct plot_fill_t
  {
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

//**************************************************************************************************
/**
 * Representation of a pad.
 */
//**************************************************************************************************
class Plot::Pad
{
 public:
  class Data;
  class Ratio;
  class Axis;
  template <class BoxType>
  class Box;
  class TextBox;
  class LegendBox;

  struct input_t
  {
    string name;
    string inputIdentifier;
  };

  Pad() = default;
  Pad(const ptree& padTree);
  Axis& operator[](const string& axis);
  void operator+=(const Pad& pad);

  // User accessors:
  Data& AddData(const input_t& input, const string& lable = "");
  Ratio& AddRatio(const input_t& numerator, const input_t& denominator, const string& lable = "");

  TextBox& AddText(double_t xPos, double_t yPos, const string& text);
  TextBox& AddText(const string& text);
  LegendBox& AddLegend(double_t xPos, double_t yPos);
  LegendBox& AddLegend();

  Axis& GetAxis(const string& axis);
  Data& GetData(uint8_t dataID);
  LegendBox& GetLegend(uint8_t legendID);
  TextBox& GetText(uint8_t textID);

  auto SetTitle(const string& title) -> decltype(*this);
  auto SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup) -> decltype(*this);
  auto SetMargins(float_t top, float_t bottom, float_t left, float_t right) -> decltype(*this);
  auto SetPalette(int32_t palette) -> decltype(*this);

  auto SetDefaultTextSize(float_t size) -> decltype(*this);
  auto SetDefaultTextColor(int16_t color) -> decltype(*this);
  auto SetDefaultTextFont(int16_t font) -> decltype(*this);

  auto SetDefaultMarkerSize(float_t size) -> decltype(*this);
  auto SetDefaultMarkerColors(const vector<int16_t>& colors) -> decltype(*this);
  auto SetDefaultMarkerStyles(const vector<int16_t>& styles) -> decltype(*this);

  auto SetDefaultLineWidth(float_t width) -> decltype(*this);
  auto SetDefaultLineColors(const vector<int16_t>& colors) -> decltype(*this);
  auto SetDefaultLineStyles(const vector<int16_t>& styles) -> decltype(*this);

  auto SetDefaultFillOpacity(float_t opacity) -> decltype(*this);
  auto SetDefaultFillColors(const vector<int16_t>& colors) -> decltype(*this);
  auto SetDefaultFillStyles(const vector<int16_t>& styles) -> decltype(*this);

  auto SetDefaultDrawingOptionGraph(drawing_options_t drawingOption) -> decltype(*this);
  auto SetDefaultDrawingOptionHist(drawing_options_t drawingOption) -> decltype(*this);
  auto SetDefaultDrawingOptionHist2d(drawing_options_t drawingOption) -> decltype(*this);

  auto SetFill(int16_t color, int16_t style = 1001) -> decltype(*this);
  auto SetTransparent() -> decltype(*this);

  auto SetFillFrame(int16_t color, int16_t style = 1001) -> decltype(*this);
  auto SetLineFrame(int16_t color, int16_t style = kSolid, float_t width = 1.f) -> decltype(*this);
  auto SetTransparentFrame() -> decltype(*this);

  auto SetRedrawAxes(bool redraw = true) -> decltype(*this);
  auto SetRefFunc(const string& refFunc) -> decltype(*this)
  {
    mRefFunc = refFunc;
    return *this;
  }

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropetyTree();

  vector<shared_ptr<Data>>& GetData()
  {
    return mData;
  }
  vector<shared_ptr<LegendBox>>& GetLegendBoxes()
  {
    return mLegendBoxes;
  }
  vector<shared_ptr<TextBox>>& GetTextBoxes()
  {
    return mTextBoxes;
  }

  const map<string, Axis>& GetAxes()
  {
    return mAxes;
  }

  const optional<string>& GetTitle()
  {
    return mTitle;
  }

  const optional<double_t>& GetXLow()
  {
    return mPosition.xlow;
  }
  const optional<double_t>& GetYLow()
  {
    return mPosition.ylow;
  }
  const optional<double_t>& GetXUp()
  {
    return mPosition.xup;
  }
  const optional<double_t>& GetYUp()
  {
    return mPosition.yup;
  }

  const optional<float_t>& GetMarginTop()
  {
    return mMargins.top;
  }
  const optional<float_t>& GetMarginBottom()
  {
    return mMargins.bottom;
  }
  const optional<float_t>& GetMarginLeft()
  {
    return mMargins.left;
  }
  const optional<float_t>& GetMarginRight()
  {
    return mMargins.right;
  }

  const optional<int32_t>& GetPalette()
  {
    return mPalette;
  }

  const optional<int16_t>& GetFillColor()
  {
    return mFill.color;
  }
  const optional<int16_t>& GetFillStyle()
  {
    return mFill.style;
  }

  const optional<int16_t>& GetFillColorFrame()
  {
    return mFrame.fillColor;
  }
  const optional<int16_t>& GetFillStyleFrame()
  {
    return mFrame.fillStyle;
  }
  const optional<int16_t>& GetLineColorFrame()
  {
    return mFrame.lineColor;
  }
  const optional<int16_t>& GetLineStyleFrame()
  {
    return mFrame.lineStyle;
  }
  const optional<float_t>& GetLineWidthFrame()
  {
    return mFrame.lineWidth;
  }

  const optional<int16_t>& GetDefaultTextColor()
  {
    return mText.color;
  }
  const optional<int16_t>& GetDefaultTextFont()
  {
    return mText.font;
  }
  const optional<float_t>& GetDefaultTextSize()
  {
    return mText.size;
  }

  const optional<float_t>& GetDefaultMarkerSize()
  {
    return mMarkerDefaults.scale;
  }
  const optional<float_t>& GetDefaultLineWidth()
  {
    return mLineDefaults.scale;
  }
  const optional<float_t>& GetDefaultFillOpacity()
  {
    return mFillDefaults.scale;
  }

  const optional<vector<int16_t>>& GetDefaultMarkerColors()
  {
    return mMarkerDefaults.colors;
  }
  const optional<vector<int16_t>>& GetDefaultLineColors()
  {
    return mLineDefaults.colors;
  }
  const optional<vector<int16_t>>& GetDefaultFillColors()
  {
    return mFillDefaults.colors;
  }
  const optional<vector<int16_t>>& GetDefaultMarkerStyles()
  {
    return mMarkerDefaults.styles;
  }
  const optional<vector<int16_t>>& GetDefaultLineStyles()
  {
    return mLineDefaults.styles;
  }
  const optional<vector<int16_t>>& GetDefaultFillStyles()
  {
    return mFillDefaults.styles;
  }

  const optional<drawing_options_t>& GetDefaultDrawingOptionGraph()
  {
    return mDrawingOptionDefaults.graph;
  }
  const optional<drawing_options_t>& GetDefaultDrawingOptionHist()
  {
    return mDrawingOptionDefaults.hist;
  }
  const optional<drawing_options_t>& GetDefaultDrawingOptionHist2d()
  {
    return mDrawingOptionDefaults.hist2d;
  }

  const optional<bool>& GetRedrawAxes()
  {
    return mRedrawAxes;
  }
  const optional<string>& GetRefFunc()
  {
    return mRefFunc;
  }

 private:
  struct pad_position_t
  {
    optional<double_t> xlow;
    optional<double_t> ylow;
    optional<double_t> xup;
    optional<double_t> yup;
  };
  struct pad_margin_t
  {
    optional<float_t> top;
    optional<float_t> bottom;
    optional<float_t> left;
    optional<float_t> right;
  };
  struct pad_fill_t
  {
    optional<int16_t> color;
    optional<int16_t> style;
  };
  struct frame_t
  {
    optional<int16_t> fillColor;
    optional<int16_t> fillStyle;
    optional<int16_t> lineColor;
    optional<int16_t> lineStyle;
    optional<float_t> lineWidth;
  };
  struct text_t
  {
    optional<float_t> size;
    optional<int16_t> font;
    optional<int16_t> color;
  };
  struct view_defaults_t
  {
    optional<float_t> scale;
    optional<vector<int16_t>> styles;
    optional<vector<int16_t>> colors;
  };
  struct data_defaults_t
  {
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
  vector<shared_ptr<Data>> mData;

  vector<shared_ptr<LegendBox>> mLegendBoxes;
  vector<shared_ptr<TextBox>> mTextBoxes;
};

//**************************************************************************************************
/**
 * Representation of drawable data.
 */
//**************************************************************************************************
class Plot::Pad::Data
{
 public:
  Data() = default;
  Data(const string& name, const string& inputIdentifier, const string& lable = "");
  Data(const ptree& dataTree);

  virtual ~Data() = default;
  Data(const Data& other) = default;
  Data(Data&&) = default;
  Data& operator=(const Data& other) = default;
  Data& operator=(Data&& other) = default;

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
  virtual auto SetNormalize(bool useWidth = false) -> decltype(*this);
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

  auto SetInputID(const string& inputIdentifier) -> decltype(*this)
  {
    mInputIdentifier = inputIdentifier;
    return *this;
  }
  const string& GetInputID()
  {
    return mInputIdentifier;
  }

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual std::shared_ptr<Data> Clone() const
  {
    return std::make_shared<Data>(*this);
  }

  virtual ptree GetPropertyTree();

  void SetType(const string& type)
  {
    mType = type;
  }

  string GetUniqueName()
  {
    return mName + gNameGroupSeparator + mInputIdentifier;
  }

  const string& GetType()
  {
    return mType;
  }
  const string& GetName()
  {
    return mName;
  }
  const optional<string>& GetLegendLable()
  {
    return mLegendLable;
  }

  const optional<int16_t>& GetMarkerColor()
  {
    return mMarker.color;
  }
  const optional<int16_t>& GetMarkerStyle()
  {
    return mMarker.style;
  }
  const optional<float_t>& GetMarkerSize()
  {
    return mMarker.scale;
  }

  const optional<int16_t>& GetLineColor()
  {
    return mLine.color;
  }
  const optional<int16_t>& GetLineStyle()
  {
    return mLine.style;
  }
  const optional<float_t>& GetLineWidth()
  {
    return mLine.scale;
  }

  const optional<int16_t>& GetFillColor()
  {
    return mFill.color;
  }
  const optional<int16_t>& GetFillStyle()
  {
    return mFill.style;
  }
  const optional<float_t>& GetFillOpacity()
  {
    return mFill.scale;
  }

  const optional<string>& GetDrawingOptions()
  {
    return mDrawingOptions;
  }
  const optional<drawing_options_t>& GetDrawingOptionAlias()
  {
    return mDrawingOptionAlias;
  }
  const optional<double_t>& GetScaleFactor()
  {
    return mModify.scale_factor;
  }
  const optional<uint8_t>& GetNormMode()
  {
    return mModify.norm_mode;
  }

  const optional<double_t>& GetMinRangeX()
  {
    return mRangeX.min;
  }
  const optional<double_t>& GetMaxRangeX()
  {
    return mRangeX.max;
  }
  const optional<double_t>& GetMinRangeY()
  {
    return mRangeY.min;
  }
  const optional<double_t>& GetMaxRangeY()
  {
    return mRangeY.max;
  }

  const bool& GetDefinesFrame()
  {
    return mDefinesFrame;
  }

 private:
  bool mDefinesFrame;

  string mType; // for introspection: "data" or "ratio"
  string mName;
  string mInputIdentifier;

  optional<string> mLegendLable;
  optional<string> mDrawingOptions;
  optional<drawing_options_t> mDrawingOptionAlias;

  struct modify_t
  {
    optional<uint8_t>
      norm_mode; // 0: normalize by summing of bin contents, 1: additionally multiply with bin width
    optional<double_t> scale_factor;
  };

  struct dataLayout_t
  {
    optional<int16_t> color;
    optional<int16_t> style;
    optional<float_t> scale; // marker size , line width, fill opacity
  };

  struct dataRange_t
  {
    optional<double_t> min;
    optional<double_t> max;
  };

  dataLayout_t mMarker;
  dataLayout_t mLine;
  dataLayout_t mFill;
  dataRange_t mRangeX;
  dataRange_t mRangeY;
  modify_t mModify;
};

//**************************************************************************************************
/**
 * Representation of a Ratio.
 */
//**************************************************************************************************
class Plot::Pad::Ratio : public Plot::Pad::Data
{
 public:
  Ratio(const string& name, const string& inputIdentifier, const string& denomName,
        const string& denomInputIdentifier, const string& lable);
  Ratio(const ptree& dataTree);

  virtual ~Ratio() = default;
  Ratio(const Ratio& other) = default;
  Ratio(Ratio&&) = default;
  Ratio& operator=(const Ratio& other) = default;
  Ratio& operator=(Ratio&& other) = default;

  auto SetIsCorrelated(bool isCorrelated = true) -> decltype(*this);

  // return correct type for the data accessors
  virtual auto SetLayout(const Data& dataLayout) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetLayout(dataLayout));
  }
  virtual auto SetRangeX(double_t min, double_t max) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetRangeX(min, max));
  }
  virtual auto SetMaxRangeX(double_t max) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMaxRangeX(max));
  }
  virtual auto SetMinRangeX(double_t min) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMinRangeX(min));
  }
  virtual auto SetRangeY(double_t min, double_t max) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetRangeY(min, max));
  }
  virtual auto SetMaxRangeY(double_t max) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMaxRangeY(max));
  }
  virtual auto SetMinRangeY(double_t min) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMinRangeY(min));
  }
  virtual auto SetLegendLable(const string& legendLable) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetLegendLable(legendLable));
  }
  virtual auto SetOptions(const string& opions) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetOptions(opions));
  }
  virtual auto SetNormalize(bool useWidth = false) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetNormalize(useWidth));
  }
  virtual auto SetScale(double_t scale) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetScaleFactor(scale));
  }
  virtual auto SetColor(int16_t color) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetColor(color));
  }
  virtual auto SetMarker(int16_t color, int16_t style, float_t size) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMarker(color, style, size));
  }
  virtual auto SetMarkerColor(int16_t color) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMarkerColor(color));
  }
  virtual auto SetMarkerStyle(int16_t style) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMarkerStyle(style));
  }
  virtual auto SetMarkerSize(float_t size) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetMarkerSize(size));
  }
  virtual auto SetLine(int16_t color, int16_t style, float_t width) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetLine(color, style, width));
  }
  virtual auto SetLineColor(int16_t color) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetLineColor(color));
  }
  virtual auto SetLineStyle(int16_t style) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetLineStyle(style));
  }
  virtual auto SetLineWidth(float_t width) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetLineWidth(width));
  }
  virtual auto SetFill(int16_t color, int16_t style, float_t opacity = 1.) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetFill(color, style, opacity));
  }
  virtual auto SetFillColor(int16_t color) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetFillColor(color));
  }
  virtual auto SetFillStyle(int16_t style) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetFillStyle(style));
  }
  virtual auto SetFillOpacity(float_t opacity) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetFillOpacity(opacity));
  }
  virtual auto SetOptions(drawing_options_t optionAlias) -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetOptions(optionAlias));
  }
  virtual auto SetDefinesFrame() -> decltype(*this)
  {
    return dynamic_cast<decltype(*this)&>(Data::SetDefinesFrame());
  }

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual std::shared_ptr<Data> Clone() const
  {
    return std::make_shared<Ratio>(*this);
  }

  ptree GetPropertyTree();
  string GetDenomIdentifier()
  {
    return mDenomInputIdentifier;
  }
  string GetDenomName()
  {
    return mDenomName;
  }
  string GetUniqueNameDenom()
  {
    return mDenomName + gNameGroupSeparator + mDenomInputIdentifier;
  }

  const bool& GetIsCorrelated()
  {
    return mIsCorrelated;
  }

 private:
  string mDenomName;
  string mDenomInputIdentifier;
  bool mIsCorrelated;
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

  Axis& SetTitle(const string& title)
  {
    mTitle = title;
    return *this;
  }
  Axis& SetRange(double_t min, double_t max)
  {
    mRange = { min, max };
    return *this;
  }
  Axis& SetMaxRange(double_t max)
  {
    mRange.max = max;
    return *this;
  }
  Axis& SetMinRange(double_t min)
  {
    mRange.min = min;
    return *this;
  }
  Axis& SetColor(int16_t color)
  {
    mAxisColor = color;
    mLableProperties.color = color;
    mTitleProperties.color = color;
    return *this;
  }
  Axis& SetAxisColor(int16_t color)
  {
    mAxisColor = color;
    return *this;
  }
  Axis& SetNumDivisions(int32_t numDivisions)
  {
    mNumDivisions = numDivisions;
    return *this;
  }
  Axis& SetMaxDigits(int32_t maxDigtis)
  {
    mMaxDigits = maxDigtis;
    return *this;
  }
  Axis& SetTickLength(float_t tickLength)
  {
    mTickLength = tickLength;
    return *this;
  }
  Axis& SetTitleFont(int16_t font)
  {
    mTitleProperties.font = font;
    return *this;
  }
  Axis& SetLableFont(int16_t font)
  {
    mLableProperties.font = font;
    return *this;
  }
  Axis& SetTitleSize(float_t size)
  {
    mTitleProperties.size = size;
    return *this;
  }
  Axis& SetLableSize(float_t size)
  {
    mLableProperties.size = size;
    return *this;
  }
  Axis& SetTitleColor(int16_t color)
  {
    mTitleProperties.color = color;
    return *this;
  }
  Axis& SetLableColor(int16_t color)
  {
    mLableProperties.color = color;
    return *this;
  }
  Axis& SetTitleOffset(float_t offset)
  {
    mTitleProperties.offset = offset;
    return *this;
  }
  Axis& SetLableOffset(float_t offset)
  {
    mLableProperties.offset = offset;
    return *this;
  }
  Axis& SetTitleCenter(bool center = true)
  {
    mTitleProperties.center = center;
    return *this;
  }
  Axis& SetLableCenter(bool center = true)
  {
    mLableProperties.center = center;
    return *this;
  }
  Axis& SetLog(bool isLog = true)
  {
    mIsLog = isLog;
    return *this;
  }
  Axis& SetGrid(bool isGrid = true)
  {
    mIsGrid = isGrid;
    return *this;
  }
  Axis& SetOppositeTicks(bool isOppositeTicks = true)
  {
    mIsOppositeTicks = isOppositeTicks;
    return *this;
  }
  Axis& SetTimeFormat(const string& timeFormat)
  {
    mTimeFormat = timeFormat;
    return *this;
  }
  Axis& SetTickOrientation(const string& tickOrientation)
  {
    mTickOrientation = tickOrientation;
    return *this;
  }

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  Axis(const string& axisName);
  Axis(const ptree& axisTree);

  ptree GetPropertyTree();
  void operator+=(const Axis& axis);

  const optional<double_t>& GetMinRange()
  {
    return mRange.min;
  }
  const optional<double_t>& GetMaxRange()
  {
    return mRange.max;
  }
  const optional<float_t>& GetTickLength()
  {
    return mTickLength;
  }
  const optional<int32_t>& GetNumDivisions()
  {
    return mNumDivisions;
  }
  const optional<int32_t>& GetMaxDigits()
  {
    return mMaxDigits;
  }
  const optional<int16_t>& GetAxisColor()
  {
    return mAxisColor;
  }
  const optional<string>& GetTitle()
  {
    return mTitle;
  }
  const optional<int16_t>& GetTitleFont()
  {
    return mTitleProperties.font;
  }
  const optional<int16_t>& GetLableFont()
  {
    return mLableProperties.font;
  }
  const optional<float_t>& GetTitleSize()
  {
    return mTitleProperties.size;
  }
  const optional<float_t>& GetLableSize()
  {
    return mLableProperties.size;
  }
  const optional<int16_t>& GetTitleColor()
  {
    return mTitleProperties.color;
  }
  const optional<int16_t>& GetLableColor()
  {
    return mLableProperties.color;
  }
  const optional<float_t>& GetTitleOffset()
  {
    return mTitleProperties.offset;
  }
  const optional<float_t>& GetLableOffset()
  {
    return mLableProperties.offset;
  }
  const optional<bool>& GetTitleCenter()
  {
    return mTitleProperties.center;
  }
  const optional<bool>& GetLableCenter()
  {
    return mLableProperties.center;
  }
  const optional<bool>& GetLog()
  {
    return mIsLog;
  }
  const optional<bool>& GetGrid()
  {
    return mIsGrid;
  }
  const optional<bool>& GetOppositeTicks()
  {
    return mIsOppositeTicks;
  }
  const optional<string>& GetTimeFormat()
  {
    return mTimeFormat;
  }
  const optional<string>& GetTickOrientation()
  {
    return mTickOrientation;
  }

 private:
  struct axisTextProperties_t
  {
    optional<int16_t> font;
    optional<float_t> size;
    optional<int16_t> color;
    optional<float_t> offset;
    optional<bool> center;
  };
  struct axisRange_t
  {
    optional<double_t> min;
    optional<double_t> max;
  };

  string mName;
  axisRange_t mRange;
  optional<string> mTitle;
  optional<int32_t> mNumDivisions;
  optional<int32_t> mMaxDigits;
  optional<float_t> mTickLength;
  optional<int16_t> mAxisColor;

  optional<bool> mIsLog;
  optional<bool> mIsGrid;
  optional<bool> mIsOppositeTicks;
  optional<string> mTimeFormat;
  optional<string> mTickOrientation;

  axisTextProperties_t mTitleProperties;
  axisTextProperties_t mLableProperties;
};

//**************************************************************************************************
/**
 * Representation of a box.
 */
//**************************************************************************************************
template <class BoxType>
class Plot::Pad::Box
{
 public:
  virtual ~Box() = default;
  Box(const Box& other) = default;
  Box(Box&&) = default;
  Box& operator=(const Box& other) = default;
  Box& operator=(Box&& other) = default;

  BoxType& SetPosition(double_t x, double_t y);
  BoxType& SetUserCoordinates(bool userCoordinates = true);
  BoxType& SetAutoPlacement();
  BoxType& SetBorder(int16_t color, int16_t style, float_t width);
  BoxType& SetBorderColor(int16_t color);
  BoxType& SetBorderStyle(int16_t style);
  BoxType& SetBorderWidth(float_t width);
  BoxType& SetText(int16_t color, int16_t font, float_t size);
  BoxType& SetTextColor(int16_t color);
  BoxType& SetTextFont(int16_t font);
  BoxType& SetTextSize(float_t size);
  BoxType& SetFill(int16_t color, int16_t style, float_t opacity);
  BoxType& SetFillColor(int16_t color);
  BoxType& SetFillStyle(int16_t style);
  BoxType& SetFillOpacity(float_t opacity);
  BoxType& SetTransparent();
  BoxType& SetNoBox();

 protected:
  ptree GetPropertyTree();

  double_t GetXPosition()
  {
    return (mPos.x) ? *mPos.x : 0.;
  }
  double_t GetYPosition()
  {
    return (mPos.y) ? *mPos.y : 0.;
  }
  optional<int16_t>& GetBorderStyle()
  {
    return mBorder.style;
  }
  optional<float_t>& GetBorderWidth()
  {
    return mBorder.scale;
  }
  optional<int16_t>& GetBorderColor()
  {
    return mBorder.color;
  }
  optional<int16_t>& GetFillStyle()
  {
    return mFill.style;
  }
  optional<float_t>& GetFillOpacity()
  {
    return mFill.scale;
  }
  optional<int16_t>& GetFillColor()
  {
    return mFill.color;
  }
  optional<int16_t>& GetTextFont()
  {
    return mText.style;
  }
  optional<float_t>& GetTextSize()
  {
    return mText.scale;
  }
  optional<int16_t>& GetTextColor()
  {
    return mText.color;
  }

  bool IsUserCoordinates()
  {
    return (mPos.isUserCoord) ? *mPos.isUserCoord : false;
  }
  bool IsAutoPlacement()
  {
    return (!mPos.x || !mPos.y);
  }

 private:
  // allow construction of Box base class only in context actually useful boxes
  friend BoxType;
  Box() = default;
  Box(double_t xPos, double_t yPos);
  Box(const ptree& boxTree);

  auto GetThis()
  {
    return static_cast<BoxType*>(this);
  }

  struct layout_t
  {
    optional<int16_t> color;
    optional<int16_t> style;
    optional<float_t> scale; // marker size , line width, fill opacity
  };
  struct position_t
  {
    optional<double_t> x;
    optional<double_t> y;
    optional<bool> isUserCoord;
  };

  position_t mPos;
  layout_t mText;
  layout_t mBorder;
  layout_t mFill;
};

//**************************************************************************************************
/**
 * Representation of a text box.
 */
//**************************************************************************************************
class Plot::Pad::TextBox : public Plot::Pad::Box<TextBox>
{
 public:
  TextBox(const string& text);
  TextBox(double_t xPos, double_t yPos, const string& text);
  TextBox(const ptree& textBoxTree);

  virtual ~TextBox() = default;
  TextBox(const TextBox& other) = default;
  TextBox(TextBox&&) = default;
  TextBox& operator=(const TextBox& other) = default;
  TextBox& operator=(TextBox&& other) = default;

  TextBox& SetText(const string& text);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropertyTree();
  const string& GetText()
  {
    return mText;
  }

 private:
  string mText;
};

//**************************************************************************************************
/**
 * Representation of a legend box.
 */
//**************************************************************************************************
class Plot::Pad::LegendBox : public Plot::Pad::Box<LegendBox>
{
 public:
  LegendBox();
  LegendBox(double_t xPos, double_t yPos);
  LegendBox(const ptree& legendBoxTree);

  virtual ~LegendBox() = default;
  LegendBox(const LegendBox& other) = default;
  LegendBox(LegendBox&&) = default;
  LegendBox& operator=(const LegendBox& other) = default;
  LegendBox& operator=(LegendBox&& other) = default;

  LegendBox& SetTitle(const string& title);
  LegendBox& SetNumColumns(uint8_t numColumns);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  ptree GetPropertyTree();
  const optional<uint8_t>& GetNumColumns()
  {
    return mNumColumns;
  }
  const optional<string>& GetTitle()
  {
    return mTitle;
  }

 private:
  optional<string> mTitle;
  optional<uint8_t> mNumColumns;

  struct legendEntry_t
  {
    optional<string> refDataName;
    optional<string> lable;
    optional<string> options;
  };
  // FIXME: every entry needs marker/line/fill/text attributes ...
};

} // end namespace PlottingFramework
#endif /* Plot_h */
