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
  colz,
  surf,
  cont,
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
    std::optional<int16_t> style;  // marker_style , line_style, fill_style, text_font
    std::optional<float_t> scale;  // marker_size , line_width, fill_opacity, text_size
  };

  Plot() = default;
  explicit Plot(const boost::property_tree::ptree& plotTree);
  Plot(const std::string& name, const std::string& figureGroupAndCategory, const std::optional<std::string>& plotTemplateName = std::nullopt);
  Pad& operator[](uint8_t padID) { return mPads[padID]; }
  Pad& GetPad(uint8_t padID) { return mPads[padID]; }
  Pad& GetPadDefaults() { return mPads[0]; }
  void operator+=(const Plot& plot);
  friend Plot operator+(const Plot& templatePlot, const Plot& plot);
  Plot(const Plot& otherPlot, const std::string& name, const std::string& figureGroup, const std::optional<std::string>& figureCategory = std::nullopt);
  Plot Clone() const;

  // accessors for user
  void SetFigureCategory(const std::string& figureCategory);
  void SetPlotTemplateName(const std::string& plotTemplateName);
  void SetDimensions(int32_t width, int32_t height, bool fixAspectRatio = false);
  void SetWidth(int32_t width);
  void SetHeight(int32_t height);
  void SetFixAspectRatio(bool fixAspectRatio = true);

  Plot& SetFill(int16_t color, std::optional<int16_t> style = std::nullopt, std::optional<float_t> opacity = std::nullopt);
  Plot& SetFillColor(int16_t color);
  Plot& SetFillStyle(int16_t style);
  Plot& SetFillOpacity(float_t opacity);
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
  const auto& GetFillOpacity() const { return mFill.scale; }

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
  Data& AddData(const std::string& name, const std::string& inputID, const std::optional<std::string>& label = std::nullopt);
  Data& AddData(const std::string& name, const Data& dataTemplate, const std::optional<std::string>& label = std::nullopt);
  Data& AddFunction(const std::string& function, const std::optional<std::string>& label = std::nullopt);

  Data& AddPoints(std::vector<double_t> x, std::vector<double_t> y, const std::optional<std::string>& label = std::nullopt);
  Data& AddPoints(std::vector<std::pair<double_t, double_t>> positions, const std::optional<std::string>& label = std::nullopt);

  Data& AddLine(std::pair<double_t, double_t> pos1, std::pair<double_t, double_t> pos2, const std::optional<std::string>& label = std::nullopt);

  Ratio& AddRatio(const std::string& numeratorName, const std::string& numeratorInputID,
                  const std::string& denominatorName, const std::string& denominatorInputID,
                  const std::optional<std::string>& label = std::nullopt);
  Ratio& AddRatio(const std::string& numeratorName, const Data& numeratorLayout, const std::string& denominatorName,
                  const std::string& denominatorInputID, const std::optional<std::string>& label = std::nullopt);
  Ratio& AddRatio(const std::string& numeratorName, const Data& numeratorLayout, const std::string& denominatorName,
                  const Data& denominatorLayout, const std::optional<std::string>& label = std::nullopt);
  Ratio& AddRatio(const std::string& numeratorName, const std::string& numeratorInputID, const std::string& denominatorName,
                  const Data& denominatorLayout, const std::optional<std::string>& label = std::nullopt);

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
  Pad& SetPalette(int32_t palette);
  Pad& SetDefaultTextSize(float_t size);
  Pad& SetDefaultTextColor(int16_t color);
  Pad& SetDefaultTextFont(int16_t font);
  Pad& SetDefaultMarkerSize(float_t size);
  Pad& SetDefaultMarkerColors(const std::vector<int16_t>& colors);
  Pad& SetDefaultMarkerColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = std::nullopt, std::optional<int32_t> nColors = std::nullopt);
  Pad& SetDefaultMarkerStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultLineWidth(float_t width);
  Pad& SetDefaultLineColors(const std::vector<int16_t>& colors);
  Pad& SetDefaultLineColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = std::nullopt, std::optional<int32_t> nColors = std::nullopt);
  Pad& SetDefaultLineStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultFillOpacity(float_t opacity);
  Pad& SetDefaultFillColors(const std::vector<int16_t>& colors);
  Pad& SetDefaultFillColors(const std::vector<std::tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, std::optional<float_t> alpha = std::nullopt, std::optional<int32_t> nColors = std::nullopt);
  Pad& SetDefaultFillStyles(const std::vector<int16_t>& styles);
  Pad& SetDefaultDrawingOptionGraph(drawing_options_t drawingOption);
  Pad& SetDefaultDrawingOptionHist(drawing_options_t drawingOption);
  Pad& SetDefaultDrawingOptionHist2d(drawing_options_t drawingOption);
  Pad& SetDefaultCandleBoxRange(float_t candleOption);
  Pad& SetDefaultCandleWhiskerRange(float_t candleOption);
  Pad& SetFill(int16_t color, std::optional<int16_t> style = std::nullopt, std::optional<float_t> opacity = std::nullopt);
  Pad& SetFillColor(int16_t color);
  Pad& SetFillStyle(int16_t style);
  Pad& SetFillOpacity(float_t opacity);
  Pad& SetTransparent();
  Pad& SetFrameFill(int16_t color, std::optional<int16_t> style = std::nullopt, std::optional<float_t> opacity = std::nullopt);
  Pad& SetFrameFillColor(int16_t color);
  Pad& SetFrameFillStyle(int16_t style);
  Pad& SetFrameFillOpacity(float_t opacity);
  Pad& SetFrameBorder(int16_t color, std::optional<int16_t> style = std::nullopt, std::optional<float_t> width = std::nullopt);
  Pad& SetFrameBorderColor(int16_t color);
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
  const auto& GetPalette() const { return mPalette; }
  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillStyle() const { return mFill.style; }
  const auto& GetFillOpacity() const { return mFill.scale; }
  const auto& GetFrameFillColor() const { return mFrameFill.color; }
  const auto& GetFrameFillStyle() const { return mFrameFill.style; }
  const auto& GetFrameFillOpacity() const { return mFrameFill.scale; }
  const auto& GetFrameBorderColor() const { return mFrameBorder.color; }
  const auto& GetFrameBorderStyle() const { return mFrameBorder.style; }
  const auto& GetFrameBorderWidth() const { return mFrameBorder.scale; }
  const auto& GetDefaultTextColor() const { return mText.color; }
  const auto& GetDefaultTextFont() const { return mText.style; }
  const auto& GetDefaultTextSize() const { return mText.scale; }
  const auto& GetDefaultMarkerSize() const { return mMarkerDefaults.scale; }
  const auto& GetDefaultMarkerStyles() const { return mMarkerDefaults.styles; }
  const auto& GetDefaultMarkerColors() const { return mMarkerDefaults.colors; }
  const auto& GetDefaultMarkerColorsGradient() const { return mMarkerDefaults.colorGradient; }
  const auto& GetDefaultLineWidth() const { return mLineDefaults.scale; }
  const auto& GetDefaultLineColors() const { return mLineDefaults.colors; }
  const auto& GetDefaultLineColorsGradient() const { return mLineDefaults.colorGradient; }
  const auto& GetDefaultLineStyles() const { return mLineDefaults.styles; }
  const auto& GetDefaultFillOpacity() const { return mFillDefaults.scale; }
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
  struct gradient_color_t {
    std::optional<std::vector<std::tuple<float_t, float_t, float_t, float_t>>> rgbEndpoints;
    std::optional<float_t> alpha;
    std::optional<int32_t> nColors;
  };
  struct view_defaults_t {
    std::optional<float_t> scale;
    std::optional<std::vector<int16_t>> styles;
    std::optional<std::vector<int16_t>> colors;
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

  std::optional<int32_t> mPalette;

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
  virtual Data& SetLegendLabel(const std::string& legendLabel);
  virtual Data& SetLegendID(uint8_t legendID);
  virtual Data& SetOptions(const std::string& options);
  virtual Data& SetOptions(drawing_options_t optionAlias);
  virtual Data& UnsetOptions();
  virtual Data& SetTextFormat(const std::string& textFormat);
  virtual Data& SetColor(int16_t color);
  virtual Data& SetMarker(int16_t color, int16_t style, float_t size);
  virtual Data& SetMarkerColor(int16_t color);
  virtual Data& SetMarkerStyle(int16_t style);
  virtual Data& SetMarkerSize(float_t size);
  virtual Data& SetLine(int16_t color, int16_t style, float_t width);
  virtual Data& SetLineColor(int16_t color);
  virtual Data& SetLineStyle(int16_t style);
  virtual Data& SetLineWidth(float_t width);
  virtual Data& SetFill(int16_t color, int16_t style, float_t opacity = 1.);
  virtual Data& SetFillColor(int16_t color);
  virtual Data& SetFillStyle(int16_t style);
  virtual Data& SetFillOpacity(float_t opacity);
  virtual Data& SetDefinesFrame();
  virtual Data& SetContours(const std::vector<double>& contours);
  virtual Data& SetContours(int32_t nContours);

  // data modifiers
  virtual Data& Normalize(bool multiplyByBinWidth = false);
  virtual Data& Scale(double_t scaleFactor);
  virtual Data& ScaleMinimum(double_t scaleFactor);
  virtual Data& ScaleMaximum(double_t scaleFactor);
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

  virtual Data& Project(std::vector<data_dim_t> dataDims, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  virtual Data& Project1D(data_dim_t x, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  virtual Data& Project2D(data_dim_t x, data_dim_t y, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});

  virtual Data& Scatter(std::string x, std::string y, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {});
  virtual Data& Scatter(std::string x, std::string y, std::string xErr, std::string yErr, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {});
  virtual Data& Scatter(std::string x, std::string y, std::string xErrLow, std::string xErrHigh, std::string yErrLow, std::string yErrHigh, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {});

  virtual Data& Profile(std::vector<data_dim_t> dataDims, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  virtual Data& Profile1D(data_dim_t x, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  virtual Data& Profile2D(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});

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
  const auto& GetMarkerStyle() const { return mMarker.style; }
  const auto& GetMarkerSize() const { return mMarker.scale; }
  const auto& GetLineColor() const { return mLine.color; }
  const auto& GetLineStyle() const { return mLine.style; }
  const auto& GetLineWidth() const { return mLine.scale; }
  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillStyle() const { return mFill.style; }
  const auto& GetFillOpacity() const { return mFill.scale; }
  const auto& GetDrawingOptions() const { return mDrawingOptions; }
  const auto& GetDrawingOptionAlias() const { return mDrawingOptionAlias; }
  const auto& GetTextFormat() const { return mTextFormat; }
  const auto& GetScaleFactor() const { return mModify.scaleFactor; }
  const auto& GetNormMode() const { return mModify.normMode; }
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

  struct proj_info_t {
    std::vector<uint8_t> dims;                                    // dimensions to project on (can be one or two)
    std::vector<std::tuple<uint8_t, double_t, double_t>> ranges;  // range restrictions on other dimensions
    std::optional<bool> isUserCoord{};                            // whether ranges are specified in user coordinates or as bins
    std::optional<bool> isProfile{};                              // whether this should be a profile instead of a projection
    std::string GetNameSuffix() const;
  };

  struct data_info_t {
    std::vector<data_dim_t> dataDims{};
    std::optional<std::string> filter{};
    std::optional<std::string> weight{};
    std::optional<int64_t> nEntries{};
    std::optional<bool> isProfileNoScatter{};
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
    std::optional<uint8_t> normMode;  // 0: sum over bin contents, 1: with bin width
    std::optional<double_t> scaleFactor;
  };
  struct legend_t {
    std::optional<std::string> label;
    std::optional<uint8_t> id;
  };

  struct dataRange_t {
    std::optional<double_t> min;
    std::optional<double_t> max;
  };

  std::optional<data_info_t> mDataInfo;
  std::optional<proj_info_t> mProjInfo;

  legend_t mLegend;
  layout_t mMarker;
  layout_t mLine;
  layout_t mFill;
  dataRange_t mRangeX;
  dataRange_t mRangeY;
  dataRange_t mScaleRange;
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
  Ratio& SetDivideNormalized(bool multiplyByBinWidth = false);

  Ratio& SetLayout(const Data& dataLayout) { return static_cast<decltype(*this)&>(Data::SetLayout(dataLayout)); }
  Ratio& ApplyLayout(const Data& dataLayout) { return static_cast<decltype(*this)&>(Data::ApplyLayout(dataLayout)); }
  Ratio& SetRangeX(double_t min, double_t max) { return static_cast<decltype(*this)&>(Data::SetRangeX(min, max)); }
  Ratio& SetMaxRangeX(double_t max) { return static_cast<decltype(*this)&>(Data::SetMaxRangeX(max)); }
  Ratio& SetMinRangeX(double_t min) { return static_cast<decltype(*this)&>(Data::SetMinRangeX(min)); }
  Ratio& UnsetRangeX() { return static_cast<decltype(*this)&>(Data::UnsetRangeX()); }
  Ratio& SetRangeY(double_t min, double_t max) { return static_cast<decltype(*this)&>(Data::SetRangeY(min, max)); }
  Ratio& SetMaxRangeY(double_t max) { return static_cast<decltype(*this)&>(Data::SetMaxRangeY(max)); }
  Ratio& SetMinRangeY(double_t min) { return static_cast<decltype(*this)&>(Data::SetMinRangeY(min)); }
  Ratio& UnsetRangeY() { return static_cast<decltype(*this)&>(Data::UnsetRangeY()); }
  Ratio& SetLegendLabel(const std::string& legendLabel) { return static_cast<decltype(*this)&>(Data::SetLegendLabel(legendLabel)); }
  Ratio& SetLegendID(uint8_t legendID) { return static_cast<decltype(*this)&>(Data::SetLegendID(legendID)); }
  Ratio& SetOptions(const std::string& options) { return static_cast<decltype(*this)&>(Data::SetOptions(options)); }
  Ratio& SetOptions(drawing_options_t optionAlias) { return static_cast<decltype(*this)&>(Data::SetOptions(optionAlias)); }
  Ratio& UnsetOptions() { return static_cast<decltype(*this)&>(Data::UnsetOptions()); }
  Ratio& SetTextFormat(const std::string& textFormat) { return static_cast<decltype(*this)&>(Data::SetTextFormat(textFormat)); }
  Ratio& SetColor(int16_t color) { return static_cast<decltype(*this)&>(Data::SetColor(color)); }
  Ratio& SetMarker(int16_t color, int16_t style, float_t size) { return static_cast<decltype(*this)&>(Data::SetMarker(color, style, size)); }
  Ratio& SetMarkerColor(int16_t color) { return static_cast<decltype(*this)&>(Data::SetMarkerColor(color)); }
  Ratio& SetMarkerStyle(int16_t style) { return static_cast<decltype(*this)&>(Data::SetMarkerStyle(style)); }
  Ratio& SetMarkerSize(float_t size) { return static_cast<decltype(*this)&>(Data::SetMarkerSize(size)); }
  Ratio& SetLine(int16_t color, int16_t style, float_t width) { return static_cast<decltype(*this)&>(Data::SetLine(color, style, width)); }
  Ratio& SetLineColor(int16_t color) { return static_cast<decltype(*this)&>(Data::SetLineColor(color)); }
  Ratio& SetLineStyle(int16_t style) { return static_cast<decltype(*this)&>(Data::SetLineStyle(style)); }
  Ratio& SetLineWidth(float_t width) { return static_cast<decltype(*this)&>(Data::SetLineWidth(width)); }
  Ratio& SetFill(int16_t color, int16_t style, float_t opacity = 1.) { return static_cast<decltype(*this)&>(Data::SetFill(color, style, opacity)); }
  Ratio& SetFillColor(int16_t color) { return static_cast<decltype(*this)&>(Data::SetFillColor(color)); }
  Ratio& SetFillStyle(int16_t style) { return static_cast<decltype(*this)&>(Data::SetFillStyle(style)); }
  Ratio& SetFillOpacity(float_t opacity) { return static_cast<decltype(*this)&>(Data::SetFillOpacity(opacity)); }
  Ratio& SetDefinesFrame() { return static_cast<decltype(*this)&>(Data::SetDefinesFrame()); }
  Ratio& SetContours(const std::vector<double>& contours) { return static_cast<decltype(*this)&>(Data::SetContours(contours)); }
  Ratio& SetContours(int32_t nContours) { return static_cast<decltype(*this)&>(Data::SetContours(nContours)); }

  // data modifiers
  Ratio& Normalize(bool multiplyByBinWidth = false) { return static_cast<decltype(*this)&>(Data::Normalize(multiplyByBinWidth)); }
  Ratio& Scale(double_t scaleFactor) { return static_cast<decltype(*this)&>(Data::Scale(scaleFactor)); }
  Ratio& ScaleMinimum(double_t scaleFactor) { return static_cast<decltype(*this)&>(Data::ScaleMinimum(scaleFactor)); }
  Ratio& ScaleMaximum(double_t scaleFactor) { return static_cast<decltype(*this)&>(Data::ScaleMaximum(scaleFactor)); }
  Ratio& Smooth(uint16_t nIterSmooth = 1) { return static_cast<decltype(*this)&>(Data::Smooth(nIterSmooth)); }

  Ratio& Project(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {}) { return static_cast<decltype(*this)&>(Data::Project(dims, ranges, isUserCoord)); }
  Ratio& ProjectX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {}) { return static_cast<decltype(*this)&>(Data::ProjectX(startY, endY, isUserCoord)); }
  Ratio& ProjectY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {}) { return static_cast<decltype(*this)&>(Data::ProjectY(startX, endX, isUserCoord)); }

  Ratio& ProjectDenom(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {});
  Ratio& ProjectXDenom(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});
  Ratio& ProjectYDenom(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});

  Ratio& Profile(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {}) { return static_cast<decltype(*this)&>(Data::Profile(dims, ranges, isUserCoord)); }
  Ratio& ProfileX(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {}) { return static_cast<decltype(*this)&>(Data::ProfileX(startY, endY, isUserCoord)); }
  Ratio& ProfileY(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {}) { return static_cast<decltype(*this)&>(Data::ProfileY(startX, endX, isUserCoord)); }

  Ratio& ProfileDenom(std::vector<uint8_t> dims, std::vector<std::tuple<uint8_t, double_t, double_t>> ranges = {}, std::optional<bool> isUserCoord = {});
  Ratio& ProfileXDenom(double_t startY = 0, double_t endY = -1, std::optional<bool> isUserCoord = {});
  Ratio& ProfileYDenom(double_t startX = 0, double_t endX = -1, std::optional<bool> isUserCoord = {});

  Ratio& Project(std::vector<data_dim_t> dataDims, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Project(dataDims, filter, weight, nEntries)); }
  Ratio& Project1D(data_dim_t x, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Project1D(x, filter, weight, nEntries)); }
  Ratio& Project2D(data_dim_t x, data_dim_t y, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Project2D(x, y, filter, weight, nEntries)); }

  Ratio& ProjectDenom(std::vector<data_dim_t> dataDims, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  Ratio& Project1DDenom(data_dim_t x, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  Ratio& Project2DDenom(data_dim_t x, data_dim_t y, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});

  Ratio& Scatter(std::string x, std::string y, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Scatter(x, y, filter, nEntries)); }
  Ratio& Scatter(std::string x, std::string y, std::string xErr, std::string yErr, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Scatter(x, y, xErr, yErr, filter, nEntries)); }
  Ratio& Scatter(std::string x, std::string y, std::string xErrLow, std::string xErrHigh, std::string yErrLow, std::string yErrHigh, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Scatter(x, y, xErrLow, xErrHigh, yErrLow, yErrHigh, filter, nEntries)); }

  Ratio& ScatterDenom(std::string x, std::string y, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {});
  Ratio& ScatterDenom(std::string x, std::string y, std::string xErr, std::string yErr, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {});
  Ratio& ScatterDenom(std::string x, std::string y, std::string xErrLow, std::string xErrHigh, std::string yErrLow, std::string yErrHigh, std::optional<std::string> filter = {}, std::optional<uint64_t> nEntries = {});

  Ratio& Profile(std::vector<data_dim_t> dataDims, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Profile(dataDims, profile, filter, weight, nEntries)); }
  Ratio& Profile1D(data_dim_t x, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Profile1D(x, profile, filter, weight, nEntries)); }
  Ratio& Profile2D(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {}) { return static_cast<decltype(*this)&>(Data::Profile2D(x, y, profile, filter, weight, nEntries)); }

  Ratio& ProfileDenom(std::vector<data_dim_t> dataDims, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  Ratio& Profile1DDenom(data_dim_t x, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});
  Ratio& Profile2DDenom(data_dim_t x, data_dim_t y, const std::string& profile, std::optional<std::string> filter = {}, std::optional<std::string> weight = {}, std::optional<uint64_t> nEntries = {});

 protected:
  friend class PlotManager;
  friend class PlotPainter;
  friend class Plot;

  virtual std::shared_ptr<Data> Clone() const { return std::make_shared<Ratio>(*this); }
  boost::property_tree::ptree GetPropertyTree() const;
  const auto& GetDenomInputID() const { return mDenomInputID; }
  const auto& GetDenomName() const { return mDenomName; }
  const auto& GetDivisionNormMode() const { return mDivisionNormMode; }
  const bool& GetIsCorrelated() const { return mIsCorrelated; }
  const auto& GetDenomDataInfo() const { return mDenomDataInfo; }
  const auto& GetDenomProjInfo() const { return mDenomProjInfo; }

 private:
  std::string mDenomName;
  std::string mDenomInputID;
  bool mIsCorrelated{};
  std::optional<bool> mDivisionNormMode;  // normalize both numerater and denominator to 0: sum over bin contents, 1: times bin widths
  std::optional<data_info_t> mDenomDataInfo;
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
  Axis& SetNumDivisions(int32_t numDivisions);
  Axis& SetMaxDigits(int32_t maxDigits);
  Axis& SetTickLength(float_t tickLength);
  Axis& SetTitleFont(int16_t font);
  Axis& SetLabelFont(int16_t font);
  Axis& SetTitleSize(float_t size);
  Axis& SetLabelSize(float_t size);
  Axis& SetTitleColor(int16_t color);
  Axis& SetLabelColor(int16_t color);
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
  const auto& GetTickLength() const { return mTickLength; }
  const auto& GetNumDivisions() const { return mNumDivisions; }
  const auto& GetMaxDigits() const { return mMaxDigits; }
  const auto& GetAxisColor() const { return mAxisColor; }
  const auto& GetTitle() const { return mTitle; }
  const auto& GetTitleFont() const { return mTitleProperties.font; }
  const auto& GetLabelFont() const { return mLabelProperties.font; }
  const auto& GetTitleSize() const { return mTitleProperties.size; }
  const auto& GetLabelSize() const { return mLabelProperties.size; }
  const auto& GetTitleColor() const { return mTitleProperties.color; }
  const auto& GetLabelColor() const { return mLabelProperties.color; }
  const auto& GetTitleOffset() const { return mTitleProperties.offset; }
  const auto& GetLabelOffset() const { return mLabelProperties.offset; }
  const auto& GetTitleCenter() const { return mTitleProperties.center; }
  const auto& GetLabelCenter() const { return mLabelProperties.center; }
  const auto& GetLog() const { return mIsLog; }
  const auto& GetGrid() const { return mIsGrid; }
  const auto& GetOppositeTicks() const { return mIsOppositeTicks; }
  const auto& GetNoExponent() const { return mIsNoExponent; }
  const auto& GetTimeFormat() const { return mTimeFormat; }
  const auto& GetTickOrientation() const { return mTickOrientation; }

 private:
  struct axisTextProperties_t {
    std::optional<int16_t> font;
    std::optional<float_t> size;
    std::optional<int16_t> color;
    std::optional<float_t> offset;
    std::optional<bool> center;
  };
  struct axisRange_t {
    std::optional<double_t> min;
    std::optional<double_t> max;
  };

  char mName{};
  axisRange_t mRange;
  std::optional<std::string> mTitle;
  std::optional<int32_t> mNumDivisions;
  std::optional<int32_t> mMaxDigits;
  std::optional<float_t> mTickLength;
  std::optional<int16_t> mAxisColor;

  std::optional<bool> mIsLog;
  std::optional<bool> mIsGrid;
  std::optional<bool> mIsOppositeTicks;
  std::optional<bool> mIsNoExponent;
  std::optional<std::string> mTimeFormat;
  std::optional<std::string> mTickOrientation;

  axisTextProperties_t mTitleProperties;
  axisTextProperties_t mLabelProperties;
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
  BoxType& SetUserCoordinates(bool isUserCoord = true);
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
  BoxType& SetMargin(float_t margin);
  BoxType& SetLineSpacing(float_t lineSpacing);

 protected:
  boost::property_tree::ptree GetPropertyTree() const;

  double_t GetXPosition() const { return (mPos.x) ? *mPos.x : 0.; }
  double_t GetYPosition() const { return (mPos.y) ? *mPos.y : 0.; }
  auto& GetBorderStyle() const { return mBorder.style; }
  auto& GetBorderWidth() const { return mBorder.scale; }
  auto& GetBorderColor() const { return mBorder.color; }
  auto& GetFillStyle() const { return mFill.style; }
  auto& GetFillOpacity() const { return mFill.scale; }
  auto& GetFillColor() const { return mFill.color; }
  auto& GetTextFont() const { return mText.style; }
  auto& GetTextSize() const { return mText.scale; }
  auto& GetTextColor() const { return mText.color; }
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
  LegendBox& SetDefaultLineColor(int16_t color);
  LegendBox& SetDefaultLineStyle(int16_t style);
  LegendBox& SetDefaultLineWidth(float_t width);
  LegendBox& SetDefaultMarkerColor(int16_t color);
  LegendBox& SetDefaultMarkerStyle(int16_t style);
  LegendBox& SetDefaultMarkerSize(float_t size);
  LegendBox& SetDefaultFillColor(int16_t color);
  LegendBox& SetDefaultFillStyle(int16_t style);
  LegendBox& SetDefaultFillOpacity(float_t opacity);
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
  const auto& GetDefaultMarkerStyle() const { return mMarkerDefault.style; }
  const auto& GetDefaultMarkerSize() const { return mMarkerDefault.scale; }
  const auto& GetDefaultLineColor() const { return mLineDefault.color; }
  const auto& GetDefaultLineStyle() const { return mLineDefault.style; }
  const auto& GetDefaultLineWidth() const { return mLineDefault.scale; }
  const auto& GetDefaultFillColor() const { return mFillDefault.color; }
  const auto& GetDefaultFillStyle() const { return mFillDefault.style; }
  const auto& GetDefaultFillOpacity() const { return mFillDefault.scale; }
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
  explicit LegendEntry(const std::optional<std::string>& label = std::nullopt, const std::optional<uint16_t>& refDataID = std::nullopt, const std::optional<std::string>& drawStyle = std::nullopt);
  explicit LegendEntry(const boost::property_tree::ptree& legendEntryTree);

  LegendEntry& SetLabel(const std::string& label);
  LegendEntry& SetRefData(uint16_t refDataID);
  LegendEntry& SetDrawStyle(const std::string& drawStyle);
  LegendEntry& SetColor(int16_t color);
  LegendEntry& SetMarkerColor(int16_t color);
  LegendEntry& SetMarkerStyle(int16_t style);
  LegendEntry& SetMarkerSize(float_t size);
  LegendEntry& SetLineColor(int16_t color);
  LegendEntry& SetLineStyle(int16_t style);
  LegendEntry& SetLineWidth(float_t width);
  LegendEntry& SetFillColor(int16_t color);
  LegendEntry& SetFillStyle(int16_t style);
  LegendEntry& SetFillOpacity(float_t opacity);
  LegendEntry& SetTextColor(int16_t color);
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
  const auto& GetMarkerStyle() const { return mMarker.style; }
  const auto& GetMarkerSize() const { return mMarker.scale; }

  const auto& GetLineColor() const { return mLine.color; }
  const auto& GetLineStyle() const { return mLine.style; }
  const auto& GetLineWidth() const { return mLine.scale; }

  const auto& GetFillColor() const { return mFill.color; }
  const auto& GetFillStyle() const { return mFill.style; }
  const auto& GetFillOpacity() const { return mFill.scale; }

  const auto& GetTextColor() const { return mText.color; }
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
