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
 * The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef INCLUDE_PLOT_H_
#define INCLUDE_PLOT_H_

// make root style property enums available to user
#include "TAttMarker.h"
#include "TAttLine.h"
#include "TAttFill.h"
#include "TCandle.h"
#include "Rtypes.h"
#include "TColor.h"
#include "TF1.h"

#include <boost/property_tree/ptree.hpp>
#include <tuple>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace SciRooPlot
{

enum drawing_options_t : uint8_t {
  // 1d options
  points = 0,
  points_xerr,
  points_endcaps,
  points_line,
  line,
  curve,
  band,
  band_smooth,
  hist,
  hist_no_borders,
  fit,
  bar,
  area,
  area_curve,
  area_line,
  boxes,
  boxes_only,
  stars,
  text,
  brackets,
  hbar,
  hbar1,
  hbar2,
  hbar3,
  hbar4,
  // 2d options
  box,
  box1,
  colz,
  legoz,
  lego,
  lego_no_borders,
  surf,
  surf1,
  surf1z,
  surf2,
  surf2z,
  surf3,
  surf3z,
  surf4,
  surf7,
  surf7z,
  cont,
  contz,
  cont1z,
  cont4z,
  candle1,
  candle2,
  candle3,
  candle4,
  candle5,
  candle6,
  candle7,
};

//**************************************************************************************************
/**
 * Class for internal representation of a plot.
 */
//**************************************************************************************************
class Plot
{
 public:
  class Pad;
  struct layout_t {
    std::optional<int16_t> color;  // marker_color , line_color, fill_color, text_color
    std::optional<float_t> alpha;  // marker_alpha , line_alpha, fill_alpha, text_alpha
    std::optional<int16_t> style;  // marker_style , line_style, fill_style, text_font
    std::optional<float_t> scale;  // marker_size , line_width, fill_?????, text_size
  };

  Plot() = default;
  explicit Plot(const boost::property_tree::ptree& plotTree);
  Plot(const std::string& name, const std::string& figureGroupAndCategory, const std::optional<std::string>& plotTemplateName = {});
  Pad& operator[](uint8_t padID) { return mPads[padID]; }
  Pad& GetPad(uint8_t padID) { return mPads[padID]; }
  Pad& GetPadDefaults() { return mPads[0]; }
  void operator+=(const Plot& plot);
  friend Plot operator+(const Plot& templatePlot, const Plot& plot);
  Plot(const Plot& otherPlot, const std::string& name, const std::string& figureGroup, const std::optional<std::string>& figureCategory = {});
  Plot Clone() const;

  // accessors for user
  void SetFigureCategory(const std::string& figureCategory);
  void SetPlotTemplateName(const std::string& plotTemplateName);
  void SetDimensions(int32_t width, int32_t height, bool fixAspectRatio = false);
  void SetWidth(int32_t width);
  void SetHeight(int32_t height);
  void SetFixAspectRatio(bool fixAspectRatio = true);
  void SetPaintColorWheel(bool paintColorWheel = true);

  Plot& SetFill(int16_t color, std::optional<int16_t> style = {}, std::optional<float_t> alpha = {});
  Plot& SetFillColor(int16_t color);
  Plot& SetFillStyle(int16_t style);
  Plot& SetFillAlpha(float_t alpha);
  Plot& SetTransparent();

 protected:
  friend class PlotManager;
  friend class PlotPainter;

  void SetFigureGroup(const std::string& figureGroup);

  // accessors for internal use by manager and painter
  const auto& GetName() const { return mName; }
  const auto& GetFigureGroup() const { return mFigureGroup; }
  const auto& GetFigureCategory() const { return mFigureCategory; }
  const auto& GetPlotTemplateName() const { return mPlotTemplateName; }
  const auto& GetUniqueName() const { return mUniqueName; }
  boost::property_tree::ptree GetPropertyTree() const;
  auto& GetPads() { return mPads; }

  const auto& GetHeight() const { return mPlotDimensions.height; }
  const auto& GetWidth() const { return mPlotDimensions.width; }
  const auto& IsFixAspectRatio() const { return mPlotDimensions.fixAspectRatio; }
  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillStyle() const { return mFill.style; }
  const auto& GetFillAlpha() const { return mFill.alpha; }
  const auto& GetPaintColorWheel() { return mPaintColorWheel; }

  uint8_t GetDataCount() const;

 private:
  struct dimension_t {
    std::optional<int32_t> width;
    std::optional<int32_t> height;
    std::optional<bool> fixAspectRatio;
  };
  void UpdateUniqueName();

  std::string mName;
  std::string mFigureGroup;
  std::optional<std::string> mFigureCategory;
  std::string mUniqueName;
  std::optional<std::string> mPlotTemplateName;
  dimension_t mPlotDimensions;
  layout_t mFill;
  std::map<uint8_t, Pad> mPads;
  std::optional<bool> mPaintColorWheel;
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

  Pad() = default;
  explicit Pad(const boost::property_tree::ptree& padTree);
  Axis& operator[](const char axis);
  Data& operator()(uint8_t dataID);
  void operator+=(const Pad& pad);

  // User accessors:
  Data& AddData(const std::string& name, const std::string& inputID, const std::optional<std::string>& label = {});
  Data& AddData(const std::string& name, const Data& dataTemplate, const std::optional<std::string>& label = {});
  Data& AddFunction(const std::string& function, const std::optional<std::string>& label = {});

  Data& AddPoints(std::vector<double_t> x, std::vector<double_t> y, const std::optional<std::string>& label = {});
  Data& AddPoints(std::vector<std::pair<double_t, double_t>> positions, const std::optional<std::string>& label = {});

  Data& AddLine(std::pair<double_t, double_t> pos1, std::pair<double_t, double_t> pos2, const std::optional<std::string>& label = {});

  Ratio& AddRatio(const std::string& numeratorName, const std::string& numeratorInputID,
                  const std::string& denominatorName, const std::string& denominatorInputID,
                  const std::optional<std::string>& label = {});
  Ratio& AddRatio(const std::string& numeratorName, const Data& numeratorLayout, const std::string& denominatorName,
                  const std::string& denominatorInputID, const std::optional<std::string>& label = {});
  Ratio& AddRatio(const std::string& numeratorName, const Data& numeratorLayout, const std::string& denominatorName,
                  const Data& denominatorLayout, const std::optional<std::string>& label = {});
  Ratio& AddRatio(const std::string& numeratorName, const std::string& numeratorInputID, const std::string& denominatorName,
                  const Data& denominatorLayout, const std::optional<std::string>& label = {});

  TextBox& AddText(double_t xPos, double_t yPos, const std::string& text);
  TextBox& AddText(const std::string& text);
  LegendBox& AddLegend(double_t xPos, double_t yPos, const std::optional<std::string>& title = {});
  LegendBox& AddLegend(const std::optional<std::string>& title = {});

  Axis& GetAxis(const char axis);
  Data& GetData(uint8_t dataID);
  LegendBox& GetLegend(uint8_t legendID);
  TextBox& GetText(uint8_t textID);

  Data& SetRefFunc(const std::string& refFunc);

  Pad& SetPosition(double_t xlow, double_t ylow, double_t xup, double_t yup);
  Pad& SetMargins(float_t top, float_t bottom, float_t left, float_t right);
  Pad& SetView(double_t theta, double_t phi);
  Pad& SetPalette(int32_t palette);
  Pad& SetPalette(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultTextSize(float_t size);
  Pad& SetDefaultTextColor(int16_t color);
  Pad& SetDefaultTextAlpha(float_t alpha);
  Pad& SetDefaultTextFont(int16_t font);
  Pad& SetDefaultMarkerAlpha(float_t alpha);
  Pad& SetDefaultMarkerSize(float_t size);
  Pad& SetDefaultMarkerColors(const std::vector<int16_t>& colors);
  Pad& SetDefaultMarkerColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultMarkerStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultLineAlpha(float_t alpha);
  Pad& SetDefaultLineWidth(float_t width);
  Pad& SetDefaultLineColors(const std::vector<int16_t>& colors);
  Pad& SetDefaultLineColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultLineStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultFillAlpha(float_t alpha);
  Pad& SetDefaultFillColors(const std::vector<int16_t>& colors);
  Pad& SetDefaultFillColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = {}, std::optional<int32_t> nColors = {});
  Pad& SetDefaultFillStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultDrawingOptionGraph(drawing_options_t drawingOption);
  Pad& SetDefaultDrawingOptionHist(drawing_options_t drawingOption);
  Pad& SetDefaultDrawingOptionHist2d(drawing_options_t drawingOption);
  Pad& SetDefaultCandleBoxRange(float_t candleOption);
  Pad& SetDefaultCandleWhiskerRange(float_t candleOption);
  Pad& SetFill(int16_t color, std::optional<int16_t> style = {}, std::optional<float_t> alpha = {});
  Pad& SetFillColor(int16_t color);
  Pad& SetFillStyle(int16_t style);
  Pad& SetFillAlpha(float_t alpha);
  Pad& SetTransparent();
  Pad& SetFrameFill(int16_t color, std::optional<int16_t> style = {}, std::optional<float_t> alpha = {});
  Pad& SetFrameFillColor(int16_t color);
  Pad& SetFrameFillStyle(int16_t style);
  Pad& SetFrameFillAlpha(float_t alpha);
  Pad& SetFrameBorder(int16_t color, std::optional<int16_t> style = {}, std::optional<float_t> width = {}, std::optional<float_t> alpha = {});
  Pad& SetFrameBorderColor(int16_t color);
  Pad& SetFrameBorderAlpha(float_t alpha);
  Pad& SetFrameBorderStyle(int16_t style);
  Pad& SetFrameBorderWidth(float_t width);
  Pad& SetFrameTransparent();
  Pad& SetRedrawAxes(bool redraw = true);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  boost::property_tree::ptree GetPropertyTree() const;

  auto& GetData() { return mData; }
  auto& GetLegendBoxes() { return mLegendBoxes; }
  auto& GetTextBoxes() { return mTextBoxes; }
  uint8_t GetDataCount() const { return mData.size(); }

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
  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillStyle() const { return mFill.style; }
  const auto& GetFillAlpha() const { return mFill.alpha; }
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
  const auto& GetDefaultDrawingOptionGraph() const { return mDrawingOptionDefaults.graph; }
  const auto& GetDefaultDrawingOptionHist() const { return mDrawingOptionDefaults.hist; }
  const auto& GetDefaultDrawingOptionHist2d() const { return mDrawingOptionDefaults.hist2d; }
  const auto& GetDefaultCandleBoxRange() const { return mCandleOptionDefaults.boxRange; }
  const auto& GetDefaultCandleWhiskerRange() const { return mCandleOptionDefaults.whiskerRange; }
  const auto& GetRedrawAxes() const { return mRedrawAxes; }
  const auto& GetRefFunc() const { return mRefFunc; }

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
    std::optional<std::vector<int16_t>> colors;
    gradient_color_t colorGradient;
  };
  struct palette_defaults_t {
    std::optional<int32_t> index;
    gradient_color_t colorGradient;
  };
  struct data_defaults_t {
    std::optional<drawing_options_t> graph;
    std::optional<drawing_options_t> hist;
    std::optional<drawing_options_t> hist2d;
  };
  struct candle_defaults_t {
    std::optional<double_t> boxRange;
    std::optional<double_t> whiskerRange;
  };

  // properties
  std::optional<std::string> mOptions;
  pad_position_t mPosition;
  pad_margin_t mMargins;
  pad_view_t mView;
  layout_t mFill;
  layout_t mFrameFill;
  layout_t mFrameBorder;
  layout_t mText;

  // user defined default data representation
  view_defaults_t mMarkerDefaults;
  view_defaults_t mLineDefaults;
  view_defaults_t mFillDefaults;
  data_defaults_t mDrawingOptionDefaults;
  candle_defaults_t mCandleOptionDefaults;
  palette_defaults_t mPalette;
  std::optional<bool> mRedrawAxes;

  std::map<char, Axis> mAxes;
  std::vector<std::shared_ptr<Data>> mData;
  std::shared_ptr<Data> mRefFunc;

  std::vector<std::shared_ptr<LegendBox>> mLegendBoxes;
  std::vector<std::shared_ptr<TextBox>> mTextBoxes;
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
  Data(const std::string& name, const std::string& inputID, const std::optional<std::string>& label);
  explicit Data(const boost::property_tree::ptree& dataTree);

  virtual ~Data() = default;
  Data(const Data& other) = default;
  Data(Data&&) = default;
  Data& operator=(const Data& other) = default;
  Data& operator=(Data&& other) = default;

  Ratio& AsRatio();
  Data& SetInputID(const std::string& inputID);
  const std::string& GetInputID() const { return mInputID; }

  virtual Data& SetLayout(const Data& dataLayout);
  virtual Data& ApplyLayout(const Data& dataLayout);
  virtual Data& SetRangeX(double_t min, double_t max);
  virtual Data& SetMaxRangeX(double_t max);
  virtual Data& SetMinRangeX(double_t min);
  virtual Data& UnsetRangeX();
  virtual Data& SetRangeY(double_t min, double_t max);
  virtual Data& SetMaxRangeY(double_t max);
  virtual Data& SetMinRangeY(double_t min);
  virtual Data& UnsetRangeY();
  virtual Data& SetScaleMinimum(double_t scaleFactor);
  virtual Data& SetScaleMaximum(double_t scaleFactor);
  virtual Data& SetShowOverflowBins(bool showOverflowBins = true);
  virtual Data& SetLegendLabel(const std::string& legendLabel);
  virtual Data& SetLegendID(uint8_t legendID);
  virtual Data& SetOptions(const std::string& options);
  virtual Data& SetOptions(drawing_options_t optionAlias);
  virtual Data& UnsetOptions();
  virtual Data& SetTextFormat(const std::string& textFormat);
  virtual Data& SetColor(int16_t color);
  virtual Data& SetAlpha(float_t alpha);
  virtual Data& SetMarker(int16_t color, int16_t style, float_t size, std::optional<float_t> alpha = {});
  virtual Data& SetMarkerColor(int16_t color);
  virtual Data& SetMarkerAlpha(float_t alpha);
  virtual Data& SetMarkerStyle(int16_t style);
  virtual Data& SetMarkerSize(float_t size);
  virtual Data& SetLine(int16_t color, int16_t style, float_t width, std::optional<float_t> alpha = {});
  virtual Data& SetLineColor(int16_t color);
  virtual Data& SetLineAlpha(float_t alpha);
  virtual Data& SetLineStyle(int16_t style);
  virtual Data& SetLineWidth(float_t width);
  virtual Data& SetFill(int16_t color, int16_t style, std::optional<float_t> alpha = {});
  virtual Data& SetFillColor(int16_t color);
  virtual Data& SetFillStyle(int16_t style);
  virtual Data& SetFillAlpha(float_t alpha);
  virtual Data& SetDefinesFrame();
  virtual Data& SetContours(const std::vector<double>& contours);
  virtual Data& SetContours(int32_t nContours);

  // data modifiers
  virtual Data& Normalize(bool scaleBinWidth = false);
  virtual Data& Scale(double_t scaleFactor);
  virtual Data& DivideBinWidth(bool divideBinWidth = true);
  virtual Data& RebinX(uint16_t nGroup);
  virtual Data& RebinY(uint16_t nGroup);
  virtual Data& RebinXY(uint16_t nGroupX, uint16_t nGroupY);
  virtual Data& Smooth(uint16_t nIterSmooth = 1);

  virtual Data& Project(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {});
  virtual Data& ProjectX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});  // for 2d histos
  virtual Data& ProjectY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});  // for 2d histos

  virtual Data& Profile(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {});  // for 2d & 3d histos
  virtual Data& ProfileX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});                                                     // for 2d histos
  virtual Data& ProfileY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});                                                     // for 2d histos

  struct data_dim_t {
    data_dim_t(const char* _var, int32_t _nBins = 100, const std::vector<double_t>& _range = {0, 0}) : var(_var), nBins(_nBins), edges(_range) {}
    data_dim_t(const std::string& _var, int32_t _nBins = 100, const std::vector<double_t>& _range = {0, 0}) : var(_var), nBins(_nBins), edges(_range) {}
    data_dim_t(const std::string& _var, const std::vector<double_t>& _edges) : var(_var), edges(_edges) {}
    std::string var{};
    std::vector<double_t> edges{};
    int32_t nBins{0};
  };

  virtual Data& Project(std::vector<data_dim_t> dataDims, std::optional<std::string> weight = {});
  virtual Data& Project1D(data_dim_t x, std::optional<std::string> weight = {});
  virtual Data& Project2D(data_dim_t x, data_dim_t y, std::optional<std::string> weight = {});

  virtual Data& Scatter(const std::string& x, const std::string& y);
  virtual Data& Scatter(const std::string& x, const std::string& y, const std::string& xErr, const std::string& yErr);
  virtual Data& Scatter(const std::string& x, const std::string& y, const std::string& xErrLow, const std::string& xErrHigh, const std::string& yErrLow, const std::string& yErrHigh);

  virtual Data& Profile(std::vector<data_dim_t> dataDims, const std::string& profile, std::optional<std::string> weight = {});
  virtual Data& Profile1D(data_dim_t x, const std::string& profile, std::optional<std::string> weight = {});
  virtual Data& Profile2D(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> weight = {});

  virtual Data& Define(const std::string& key, const std::string& value);
  virtual Data& Filter(const std::string& filter);
  virtual Data& Entries(uint64_t nEntries);
  virtual Data& Entries(uint64_t entryMin, uint64_t entryMax);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual std::shared_ptr<Data> Clone() const { return std::make_shared<Data>(*this); }
  virtual boost::property_tree::ptree GetPropertyTree() const;
  void SetType(const std::string& type) { mType = type; }

  const auto& GetType() const { return mType; }
  const auto& GetName() const { return mName; }
  const auto& GetLegendLabel() const { return mLegend.label; }
  const auto& GetLegendID() const { return mLegend.id; }
  const auto& GetMarkerColor() const { return mMarker.color; }
  const auto& GetMarkerAlpha() const { return mMarker.alpha; }
  const auto& GetMarkerStyle() const { return mMarker.style; }
  const auto& GetMarkerSize() const { return mMarker.scale; }
  const auto& GetLineColor() const { return mLine.color; }
  const auto& GetLineAlpha() const { return mLine.alpha; }
  const auto& GetLineStyle() const { return mLine.style; }
  const auto& GetLineWidth() const { return mLine.scale; }
  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillAlpha() const { return mFill.alpha; }
  const auto& GetFillStyle() const { return mFill.style; }
  const auto& GetDrawingOptions() const { return mDrawingOptions; }
  const auto& GetDrawingOptionAlias() const { return mDrawingOptionAlias; }
  const auto& GetTextFormat() const { return mTextFormat; }
  const auto& GetScaleFactor() const { return mModify.scaleFactor; }
  const auto& GetScaleBinWidthNorm() const { return mModify.scaleBinWidthNorm; }
  const auto& GetMinRangeX() const { return mRangeX.min; }
  const auto& GetMaxRangeX() const { return mRangeX.max; }
  const auto& GetMinRangeY() const { return mRangeY.min; }
  const auto& GetMaxRangeY() const { return mRangeY.max; }
  const auto& GetScaleMinimum() const { return mScaleRange.min; }
  const auto& GetScaleMaximum() const { return mScaleRange.max; }
  const bool& GetDefinesFrame() const { return mDefinesFrame; }
  const auto& GetContours() const { return mContours; }
  const auto& GetNContours() const { return mNContours; }
  const auto& GetDataInfo() const { return mDataInfo; }
  const auto& GetProjInfo() const { return mProjInfo; }
  const auto& GetNiterSmooth() const { return mNiterSmooth; }
  const auto& GetRebinGroupX() const { return mModify.rebinGroupX; }
  const auto& GetRebinGroupY() const { return mModify.rebinGroupY; }
  const auto& GetDivideBinWidth() const { return mModify.divideBinWidth; }
  const auto& GetShowOverflowBins() const { return mModify.showOverflowBins; }

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
    struct entry_range_t {
      std::optional<int64_t> min{};
      std::optional<int64_t> max{};
    };
    entry_range_t entries;
    void set(const data_info_t& other)
    {
      dataDims = other.dataDims;
      weight = other.weight;
      isProfileNoScatter = other.isProfileNoScatter;
    }
    bool singleProc()
    {
      // disable multi processing in case scatter (order might be relevant) or range is requested
      return !isProfileNoScatter.value_or(true) || entries.max.value_or(false);
    }
    std::string GetNameSuffix() const;
  };

 private:
  bool mDefinesFrame{};

  std::string mType;  // for introspection: "data" or "ratio"
  std::string mName;
  std::string mInputID;

  std::optional<std::string> mDrawingOptions;
  std::optional<drawing_options_t> mDrawingOptionAlias;
  std::optional<std::string> mTextFormat;

  struct modify_t {
    std::optional<bool> scaleBinWidthNorm;
    std::optional<double_t> scaleFactor;
    std::optional<bool> divideBinWidth;
    std::optional<uint16_t> rebinGroupX;
    std::optional<uint16_t> rebinGroupY;
    std::optional<bool> showOverflowBins;
  };
  struct legend_t {
    std::optional<std::string> label;
    std::optional<uint8_t> id;
  };

  struct data_range_t {
    std::optional<double_t> min;
    std::optional<double_t> max;
  };

  data_info_t mDataInfo;
  std::optional<proj_info_t> mProjInfo;

  legend_t mLegend;
  layout_t mMarker;
  layout_t mLine;
  layout_t mFill;
  data_range_t mRangeX;
  data_range_t mRangeY;
  data_range_t mScaleRange;
  modify_t mModify;

  std::optional<std::vector<double_t>> mContours;
  std::optional<int32_t> mNContours;
  std::optional<uint16_t> mNiterSmooth;
};

//**************************************************************************************************
/**
 * Representation of a Ratio.
 */
//**************************************************************************************************
class Plot::Pad::Ratio : public Plot::Pad::Data
{
 public:
  Ratio(const std::string& name, const std::string& inputID, const std::string& denomName,
        const std::string& denomInputID, const std::optional<std::string>& label);
  explicit Ratio(const boost::property_tree::ptree& dataTree);

  virtual ~Ratio() = default;
  Ratio(const Ratio& other) = default;
  Ratio(Ratio&&) = default;
  Ratio& operator=(const Ratio& other) = default;
  Ratio& operator=(Ratio&& other) = default;

  Ratio& SetIsCorrelated(bool isCorrelated = true);
  Ratio& SetDivideNormalized(bool scaleBinWidth = false);

#define FORWARD_TO_DATA(METHOD)                \
  template <typename... Args>                  \
  Ratio& METHOD(Args&&... args)                \
  {                                            \
    Data::METHOD(std::forward<Args>(args)...); \
    return *this;                              \
  }
  FORWARD_TO_DATA(SetLayout)
  FORWARD_TO_DATA(ApplyLayout)
  FORWARD_TO_DATA(SetRangeX)
  FORWARD_TO_DATA(SetMaxRangeX)
  FORWARD_TO_DATA(SetMinRangeX)
  FORWARD_TO_DATA(UnsetRangeX)
  FORWARD_TO_DATA(SetRangeY)
  FORWARD_TO_DATA(SetMaxRangeY)
  FORWARD_TO_DATA(SetMinRangeY)
  FORWARD_TO_DATA(UnsetRangeY)
  FORWARD_TO_DATA(SetScaleMinimum)
  FORWARD_TO_DATA(SetScaleMaximum)
  FORWARD_TO_DATA(SetShowOverflowBins)
  FORWARD_TO_DATA(SetLegendLabel)
  FORWARD_TO_DATA(SetLegendID)
  FORWARD_TO_DATA(SetOptions)
  FORWARD_TO_DATA(UnsetOptions)
  FORWARD_TO_DATA(SetTextFormat)
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
  FORWARD_TO_DATA(SetContours)

  // data modifiers
  FORWARD_TO_DATA(Normalize)
  FORWARD_TO_DATA(Scale)
  FORWARD_TO_DATA(DivideBinWidth)
  FORWARD_TO_DATA(RebinX)
  FORWARD_TO_DATA(RebinY)
  FORWARD_TO_DATA(RebinXY)
  FORWARD_TO_DATA(Smooth)

  Ratio& Numer();  // switch to numerator for follwing modifiers (default)
  Ratio& Denom();  // switch to denominator for follwing modifiers

  Ratio& Project(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {});
  Ratio& ProjectX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});
  Ratio& ProjectY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});

  Ratio& Profile(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {});
  Ratio& ProfileX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});
  Ratio& ProfileY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});

  Ratio& Project(std::vector<data_dim_t> dataDims, std::optional<std::string> weight = {});
  Ratio& Project1D(data_dim_t x, std::optional<std::string> weight = {});
  Ratio& Project2D(data_dim_t x, data_dim_t y, std::optional<std::string> weight = {});

  Ratio& Scatter(const std::string& x, const std::string& y);
  Ratio& Scatter(const std::string& x, const std::string& y, const std::string& xErr, const std::string& yErr);
  Ratio& Scatter(const std::string& x, const std::string& y, const std::string& xErrLow, const std::string& xErrHigh, const std::string& yErrLow, const std::string& yErrHigh);

  Ratio& Profile(std::vector<data_dim_t> dataDims, const std::string& profile, std::optional<std::string> weight = {});
  Ratio& Profile1D(data_dim_t x, const std::string& profile, std::optional<std::string> weight = {});
  Ratio& Profile2D(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> weight = {});

  Ratio& Define(const std::string& key, const std::string& value);
  Ratio& Filter(const std::string& filter);
  Ratio& Entries(uint64_t nEntries);
  Ratio& Entries(uint64_t entryMin, uint64_t entryMax);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual std::shared_ptr<Data> Clone() const { return std::make_shared<Ratio>(*this); }
  boost::property_tree::ptree GetPropertyTree() const;
  const auto& GetDenomInputID() const { return mDenomInputID; }
  const auto& GetDenomName() const { return mDenomName; }
  const auto& GetScaleBinWidthDivision() const { return mScaleBinWidth; }
  const bool& GetIsCorrelated() const { return mIsCorrelated; }
  const auto& GetDenomDataInfo() const { return mDenomDataInfo; }
  const auto& GetDenomProjInfo() const { return mDenomProjInfo; }

 private:
  enum class Mode { Num,
                    Den };
  Mode mModMode = Mode::Num;
  std::string mDenomName;
  std::string mDenomInputID;
  bool mIsCorrelated{};
  std::optional<bool> mScaleBinWidth;  // normalize both numerater and denominator with bin widths
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

  Axis& SetTitle(const std::string& title);
  Axis& SetRange(double_t min, double_t max);
  Axis& SetMaxRange(double_t max);
  Axis& SetMinRange(double_t min);
  Axis& SetColor(int16_t color);
  Axis& SetAxisColor(int16_t color);
  Axis& SetAxisAlpha(float_t alpha);
  Axis& SetNumDivisions(int32_t numDivisions);
  Axis& SetMaxDigits(int32_t maxDigits);
  Axis& SetTickLength(float_t tickLength);
  Axis& SetTitleFont(int16_t font);
  Axis& SetLabelFont(int16_t font);
  Axis& SetTitleSize(float_t size);
  Axis& SetLabelSize(float_t size);
  Axis& SetTitleColor(int16_t color);
  Axis& SetLabelColor(int16_t color);
  Axis& SetTitleAlpha(float_t alpha);
  Axis& SetLabelAlpha(float_t alpha);
  Axis& SetTitleOffset(float_t offset);
  Axis& SetLabelOffset(float_t offset);
  Axis& SetTitleCenter(bool center = true);
  Axis& SetLabelCenter(bool center = true);
  Axis& SetLog(bool isLog = true);
  Axis& SetGrid(bool isGrid = true);
  Axis& SetOppositeTicks(bool isOppositeTicks = true);
  Axis& SetNoExponent(bool isNoExponent = true);
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
  const auto& GetTitleCenter() const { return mTitleLayout.center; }
  const auto& GetLabelCenter() const { return mLabelLayout.center; }
  const auto& GetLog() const { return mAxisFrame.isLog; }
  const auto& GetGrid() const { return mAxisFrame.isGrid; }
  const auto& GetOppositeTicks() const { return mAxisFrame.isOppositeTicks; }
  const auto& GetNoExponent() const { return mAxisFrame.isNoExponent; }
  const auto& GetTimeFormat() const { return mAxisFrame.timeFormat; }
  const auto& GetTickOrientation() const { return mAxisFrame.tickOrientation; }

 private:
  struct axis_layout_t : layout_t {
    std::optional<float_t> offset;
    std::optional<bool> center;
  };
  struct axis_range_t {
    std::optional<double_t> min;
    std::optional<double_t> max;
  };
  struct axis_frame_t : layout_t {
    std::optional<std::string> title;
    std::optional<int32_t> maxDigits;
    std::optional<bool> isLog;
    std::optional<bool> isGrid;
    std::optional<bool> isOppositeTicks;
    std::optional<bool> isNoExponent;
    std::optional<std::string> timeFormat;
    std::optional<std::string> tickOrientation;
  };

  char mName{};
  axis_frame_t mAxisFrame;
  axis_layout_t mTitleLayout;
  axis_layout_t mLabelLayout;
  axis_range_t mRange;
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
  BoxType& SetSize(double_t width, double_t height);
  BoxType& SetUserCoordinates(bool isUserCoord = true);
  BoxType& SetAutoPlacement();
  BoxType& SetBorder(int16_t color, int16_t style, float_t width, std::optional<float_t> alpha = {});
  BoxType& SetBorderColor(int16_t color);
  BoxType& SetBorderAlpha(float_t alpha);
  BoxType& SetBorderStyle(int16_t style);
  BoxType& SetBorderWidth(float_t width);
  BoxType& SetText(int16_t color, int16_t font, float_t size, std::optional<float_t> alpha = {});
  BoxType& SetTextColor(int16_t color);
  BoxType& SetTextAlpha(float_t alpha);
  BoxType& SetTextFont(int16_t font);
  BoxType& SetTextSize(float_t size);
  BoxType& SetFill(int16_t color, int16_t style, float_t alpha);
  BoxType& SetFillColor(int16_t color);
  BoxType& SetFillStyle(int16_t style);
  BoxType& SetFillAlpha(float_t alpha);
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
  auto& GetBorderStyle() const { return mBorder.style; }
  auto& GetBorderWidth() const { return mBorder.scale; }
  auto& GetBorderColor() const { return mBorder.color; }
  auto& GetBorderAlpha() const { return mBorder.alpha; }
  auto& GetFillStyle() const { return mFill.style; }
  auto& GetFillAlpha() const { return mFill.alpha; }
  auto& GetFillColor() const { return mFill.color; }
  auto& GetTextFont() const { return mText.style; }
  auto& GetTextSize() const { return mText.scale; }
  auto& GetTextColor() const { return mText.color; }
  auto& GetTextAlpha() const { return mText.alpha; }
  auto& GetMargin() const { return mMargin; }
  auto& GetLineSpacing() const { return mLineSpacing; }

  bool IsUserCoordinates() const { return (mPos.isUserCoord) ? *mPos.isUserCoord : false; }
  bool IsAutoPlacement() const { return (!mPos.x || !mPos.y); }

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
  };

  std::optional<float_t> mMargin;
  std::optional<float_t> mLineSpacing;
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
  explicit TextBox(const std::string& text);
  TextBox(double_t xPos, double_t yPos, const std::string& text);
  explicit TextBox(const boost::property_tree::ptree& textBoxTree);

  virtual ~TextBox() = default;
  TextBox(const TextBox& other) = default;
  TextBox(TextBox&&) = default;
  TextBox& operator=(const TextBox& other) = default;
  TextBox& operator=(TextBox&& other) = default;

  TextBox& SetText(const std::string& text);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  boost::property_tree::ptree GetPropertyTree() const;
  const std::string& GetText() const { return mText; }

 private:
  std::string mText;
};

//**************************************************************************************************
/**
 * Representation of a legend box.
 */
//**************************************************************************************************
class Plot::Pad::LegendBox : public Plot::Pad::Box<LegendBox>
{
 public:
  class LegendEntry;
  explicit LegendBox(const std::optional<std::string>& title = {});
  LegendBox(double_t xPos, double_t yPos, const std::optional<std::string>& title = {});
  explicit LegendBox(const boost::property_tree::ptree& legendBoxTree);

  virtual ~LegendBox() = default;
  LegendBox(const LegendBox& other) = default;
  LegendBox(LegendBox&&) = default;
  LegendBox& operator=(const LegendBox& other) = default;
  LegendBox& operator=(LegendBox&& other) = default;

  LegendEntry& GetEntry(uint8_t entryID) { return mLegendEntriesUser[entryID]; }
  LegendBox& SetTitle(const std::string& title);
  LegendBox& SetNumColumns(uint8_t numColumns);
  LegendBox& SetDefaultDrawStyle(std::string drawStyle);
  LegendBox& SetDefaultColor(int16_t color);
  LegendBox& SetDefaultAlpha(float_t alpha);
  LegendBox& SetDefaultLineColor(int16_t color);
  LegendBox& SetDefaultLineAlpha(float_t alpha);
  LegendBox& SetDefaultLineStyle(int16_t style);
  LegendBox& SetDefaultLineWidth(float_t width);
  LegendBox& SetDefaultMarkerColor(int16_t color);
  LegendBox& SetDefaultMarkerAlpha(float_t alpha);
  LegendBox& SetDefaultMarkerStyle(int16_t style);
  LegendBox& SetDefaultMarkerSize(float_t size);
  LegendBox& SetDefaultFillColor(int16_t color);
  LegendBox& SetDefaultFillAlpha(float_t alpha);
  LegendBox& SetDefaultFillStyle(int16_t style);
  LegendBox& SetSymbolColScale(float_t scale);

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  boost::property_tree::ptree GetPropertyTree() const;
  const std::optional<uint8_t>& GetNumColumns() const { return mNumColumns; }
  const std::optional<std::string>& GetTitle() const { return mTitle; }

  LegendEntry& AddEntry(const std::string& label, uint16_t refDataID);

  const auto& GetEntries() const { return mLegendEntries; }
  const auto& GetDefaultDrawStyle() const { return mDrawStyleDefault; }
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
  std::vector<LegendEntry> mLegendEntries;            // this is transient and will be generated automatically
  std::map<uint8_t, LegendEntry> mLegendEntriesUser;  // this is persistent and must be saved

  layout_t mLineDefault;
  layout_t mMarkerDefault;
  layout_t mFillDefault;
  std::optional<std::string> mDrawStyleDefault;
  std::optional<float_t> mSymbolColScale;

  void MergeLegendEntries();
};

//**************************************************************************************************
/**
 * Representation of a legend box entry.
 */
//**************************************************************************************************
class Plot::Pad::LegendBox::LegendEntry
{
 public:
  explicit LegendEntry(const std::optional<std::string>& label = {}, const std::optional<uint16_t>& refDataID = {}, const std::optional<std::string>& drawStyle = {});
  explicit LegendEntry(const boost::property_tree::ptree& legendEntryTree);

  LegendEntry& SetLabel(const std::string& label);
  LegendEntry& SetRefData(uint16_t refDataID);
  LegendEntry& SetDrawStyle(const std::string& drawStyle);
  LegendEntry& SetColor(int16_t color);
  LegendEntry& SetAlpha(float_t alpha);
  LegendEntry& SetMarkerColor(int16_t color);
  LegendEntry& SetMarkerAlpha(float_t alpha);
  LegendEntry& SetMarkerStyle(int16_t style);
  LegendEntry& SetMarkerSize(float_t size);
  LegendEntry& SetLineColor(int16_t color);
  LegendEntry& SetLineAlpha(float_t alpha);
  LegendEntry& SetLineStyle(int16_t style);
  LegendEntry& SetLineWidth(float_t width);
  LegendEntry& SetFillColor(int16_t color);
  LegendEntry& SetFillAlpha(float_t alpha);
  LegendEntry& SetFillStyle(int16_t style);
  LegendEntry& SetTextColor(int16_t color);
  LegendEntry& SetTextAlpha(float_t alpha);
  LegendEntry& SetTextFont(int16_t font);
  LegendEntry& SetTextSize(float_t size);

 protected:
  friend class PlotPainter;
  friend class LegendBox;
  void operator+=(const LegendEntry& legendEntry);

  boost::property_tree::ptree GetPropertyTree() const;
  const auto& GetRefDataID() const { return mRefDataID; }
  const auto& GetLabel() const { return mLabel; }
  const auto& GetDrawStyle() const { return mDrawStyle; }

  const auto& GetMarkerColor() const { return mMarker.color; }
  const auto& GetMarkerAlpha() const { return mMarker.alpha; }
  const auto& GetMarkerStyle() const { return mMarker.style; }
  const auto& GetMarkerSize() const { return mMarker.scale; }

  const auto& GetLineColor() const { return mLine.color; }
  const auto& GetLineAlpha() const { return mLine.alpha; }
  const auto& GetLineStyle() const { return mLine.style; }
  const auto& GetLineWidth() const { return mLine.scale; }

  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillAlpha() const { return mFill.alpha; }
  const auto& GetFillStyle() const { return mFill.style; }

  const auto& GetTextColor() const { return mText.color; }
  const auto& GetTextAlpha() const { return mText.alpha; }
  const auto& GetTextFont() const { return mText.style; }
  const auto& GetTextSize() const { return mText.scale; }

 private:
  std::optional<std::string> mLabel;
  std::optional<uint16_t> mRefDataID;
  std::optional<std::string> mDrawStyle;
  layout_t mFill;
  layout_t mMarker;
  layout_t mLine;
  layout_t mText;
};

}  // end namespace SciRooPlot
#endif  // INCLUDE_PLOT_H_
