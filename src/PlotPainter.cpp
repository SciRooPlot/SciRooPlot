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

#include "PlotPainter.h"

#include "SciRooPlot/Logging.h"

#include <TApplication.h>
#include <TBox.h>
#include <TCanvas.h>
#include <TColorWheel.h>
#include <TEfficiency.h>
#include <TExec.h>
#include <TF1.h>
#include <TF2.h>
#include <TF3.h>
#include <TFrame.h>
#include <TGWindow.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TGraph2DAsymmErrors.h>
#include <TGraph2DErrors.h>
#include <TGraphAsymmErrors.h>
#include <TGraphBentErrors.h>
#include <TGraphErrors.h>
#include <TGraphSmooth.h>
#include <TH1.h>
#include <TH1D.h>
#include <TH2.h>
#include <TH2D.h>
#include <TH3.h>
#include <TH3D.h>
#include <THStack.h>
#include <THashList.h>
#include <THn.h>
#include <THnSparse.h>
#include <TIterator.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TLine.h>
#include <TMultiGraph.h>
#include <TObjArray.h>
#include <TObject.h>
#include <TObjectTable.h>
#include <TPave.h>
#include <TPaveText.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TProfile3D.h>
#include <TROOT.h>
#include <TRootCanvas.h>
#include <TSpline.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <TTimeStamp.h>
#include <TView.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "util/Optional.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"
#include "util/TypeTraits.h"

using std::array;
using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::variant;
using std::vector;

namespace SciRooPlot
{
using util::get_first;
using util::get_first_or;
using util::is_func;
using util::is_func_1d;
using util::is_func_2d;
using util::is_func_3d;
using util::is_graph_1d;
using util::is_graph_2d;
using util::is_hist;
using util::is_hist_1d;
using util::is_hist_2d;
using util::is_hist_3d;
using util::is_one_of_v;
using util::make_scope_guard;
using util::pick;
using util::str_contains;

//**************************************************************************************************
/**
 * Helper to copy the attributes of a function.
 */
//**************************************************************************************************
template <typename FuncT>
void CopyFuncAttributes(FuncT* from, FuncT* to)
{
  from->TAttLine::Copy(*to);
  from->TAttFill::Copy(*to);
  from->TAttMarker::Copy(*to);
  to->SetTitle(from->GetTitle());
  to->SetNpx(from->GetNpx());
  to->SetBit(kCanDelete);
}

//**************************************************************************************************
/**
 * Function to generate the plot.
 */
//**************************************************************************************************
unique_ptr<TCanvas> PlotPainter::GeneratePlot(Plot& plot, const unordered_map<string, unordered_map<string, unique_ptr<TObject>>>& dataBuffer)
{
  bool fail = false;

  // ROOT reads some absolute sizes from gStyle only at paint time (see end of this function): undo the scale of a previous plot before drawing
  if (sUnscaledStyle) {
    gStyle->SetLineWidth(sUnscaledStyle->lineWidth);
    gStyle->SetHatchesLineWidth(sUnscaledStyle->hatchesLineWidth);
    gStyle->SetEndErrorSize(sUnscaledStyle->endErrorSize);
    for (int32_t style = 1; style <= static_cast<int32_t>(sUnscaledStyle->lineStyles.size()); ++style) {
      gStyle->SetLineStyleString(style, sUnscaledStyle->lineStyles[style - 1].data());
    }
    sUnscaledStyle.reset();
  }

  double_t canvasWidth = std::round(mScale * plot.GetWidth().value_or(gStyle->GetCanvasDefW()));
  double_t canvasHeight = std::round(mScale * plot.GetHeight().value_or(gStyle->GetCanvasDefH()));
  if (canvasWidth <= 0 || canvasHeight <= 0) {
    ERROR("Plot {} has invalid dimensions {}x{}.", plot.GetName(), canvasWidth, canvasHeight);
    return nullptr;
  }
  // generate canvas with 'invisible' dummy size to avoid annoying popup window
  unique_ptr<TCanvas> canvas_ptr{new TCanvas("SRP_empty_scratch_canvas", plot.GetUniqueName().data(), 1., 1.)};
  // set actual name after ctor to avoid potential deletion of canvas with same name in root session
  canvas_ptr->SetName(plot.GetUniqueName().data());
  if (gROOT->IsBatch()) {
    canvas_ptr->SetCanvasSize(canvasWidth, canvasHeight);
  } else {
    auto canvasImp = static_cast<TRootCanvas*>(canvas_ptr->GetCanvasImp());
    canvasImp->UnmapWindow();
    canvas_ptr->SetCanvasSize(canvasWidth, canvasHeight);
    canvas_ptr->SetWindowPosition(gStyle->GetCanvasDefX(), gStyle->GetCanvasDefY());
    // define window size such that canvas size is correct
    canvasImp->Resize(canvasWidth, canvasHeight);
    canvasImp->FitCanvas();
    canvasImp->Resize(canvasWidth + (canvasWidth - canvas_ptr->GetWw()), canvasHeight + (canvasHeight - canvas_ptr->GetWh()));
    canvasImp->FitCanvas();
  }

  canvas_ptr->SetMargin(0., 0., 0., 0.);

  // apply user settings for plot
  if (plot.GetFillColor()) canvas_ptr->SetFillColor(*plot.GetFillColor());
  if (plot.GetFillStyle()) canvas_ptr->SetFillStyle(*plot.GetFillStyle());
  if (plot.GetFillAlpha()) canvas_ptr->SetFillColor(TColor::GetColorTransparent(canvas_ptr->GetFillColor(), *plot.GetFillAlpha()));

  if (plot.IsFixAspectRatio()) canvas_ptr->SetFixedAspectRatio(*plot.IsFixAspectRatio());

  if (plot.GetPaintColorWheel() && *plot.GetPaintColorWheel()) {
    auto fillColor = canvas_ptr->GetFillColor();
    auto fillStyle = canvas_ptr->GetFillStyle();
    auto wheel = new TColorWheel();
    wheel->SetCanvas(canvas_ptr.get());
    wheel->Draw();
    canvas_ptr->SetFillStyle(fillStyle);
    canvas_ptr->SetFillColor(fillColor);
    canvas_ptr->Update();
    return canvas_ptr;
  }

  auto& padDefaults = plot[0];
  for (const auto& [padID, dummy] : plot.GetPads()) {
    if (padID == 0) continue;  // pad 0 is used only to define the defaults
    auto& pad = plot[padID];   // needed because processData lambda cannot capture variable from structured binding ('dummy')

    // Pad placing
    array<double_t, 4> padPos = {0., 0., 1., 1.};

    auto xLow = pad.GetXLow();
    auto yLow = pad.GetYLow();
    auto xUp = pad.GetXUp();
    auto yUp = pad.GetYUp();

    const bool valid =
      xLow && yLow && xUp && yUp &&
      *xLow >= 0 && *xLow <= 1 &&
      *xUp >= 0 && *xUp <= 1 &&
      *yLow >= 0 && *yLow <= 1 &&
      *yUp >= 0 && *yUp <= 1 &&
      *xLow <= *xUp &&
      *yLow <= *yUp;

    if (!valid) {
      // a plot with only one pad and no position for it is drawn over the whole plot by design
      const bool isSinglePad = std::count_if(plot.GetPads().begin(), plot.GetPads().end(), [](const auto& p) { return p.first != 0; }) == 1;
      if (!isSinglePad || xLow || yLow || xUp || yUp) {
        WARNING("Position of pad {} was not defined properly! Drawing it over whole plot.", padID);
      }
    } else {
      padPos = {*xLow, *yLow, *xUp, *yUp};
    }

    // get the settings for this pad
    auto textFont = get_first(pad.GetDefaultTextFont(), padDefaults.GetDefaultTextFont());
    auto textSize = get_first(pad.GetDefaultTextSize(), padDefaults.GetDefaultTextSize());
    auto textColor = get_first(pad.GetDefaultTextColor(), padDefaults.GetDefaultTextColor());
    auto textAlpha = get_first(pad.GetDefaultTextAlpha(), padDefaults.GetDefaultTextAlpha());

    canvas_ptr->cd();
    string padName = "Pad_" + std::to_string(padID);

    TPad* pad_ptr = new TPad(padName.data(), "", padPos[0], padPos[1], padPos[2], padPos[3]);

    if (auto marginTop = get_first(pad.GetMarginTop(), padDefaults.GetMarginTop())) pad_ptr->SetTopMargin(*marginTop);
    if (auto marginBottom = get_first(pad.GetMarginBottom(), padDefaults.GetMarginBottom())) pad_ptr->SetBottomMargin(*marginBottom);
    if (auto marginLeft = get_first(pad.GetMarginLeft(), padDefaults.GetMarginLeft())) pad_ptr->SetLeftMargin(*marginLeft);
    if (auto marginRight = get_first(pad.GetMarginRight(), padDefaults.GetMarginRight())) pad_ptr->SetRightMargin(*marginRight);
    if (auto padFillColor = get_first(pad.GetFillColor(), padDefaults.GetFillColor())) pad_ptr->SetFillColor(*padFillColor);
    if (auto padFillStyle = get_first(pad.GetFillStyle(), padDefaults.GetFillStyle())) pad_ptr->SetFillStyle(*padFillStyle);
    if (auto padFillAlpha = get_first(pad.GetFillAlpha(), padDefaults.GetFillAlpha())) pad_ptr->SetFillColor(TColor::GetColorTransparent(pad_ptr->GetFillColor(), *padFillAlpha));
    if (auto frameFillColor = get_first(pad.GetFrameFillColor(), padDefaults.GetFrameFillColor())) pad_ptr->SetFrameFillColor(*frameFillColor);
    if (auto frameFillStyle = get_first(pad.GetFrameFillStyle(), padDefaults.GetFrameFillStyle())) pad_ptr->SetFrameFillStyle(*frameFillStyle);
    if (auto frameFillAlpha = get_first(pad.GetFrameFillAlpha(), padDefaults.GetFrameFillAlpha())) pad_ptr->SetFrameFillColor(TColor::GetColorTransparent(pad_ptr->GetFrameFillColor(), *frameFillAlpha));
    if (auto frameBorderColor = get_first(pad.GetFrameBorderColor(), padDefaults.GetFrameBorderColor())) pad_ptr->SetFrameLineColor(*frameBorderColor);
    if (auto frameBorderAlpha = get_first(pad.GetFrameBorderAlpha(), padDefaults.GetFrameBorderAlpha())) pad_ptr->SetFrameLineColor(TColor::GetColorTransparent(pad_ptr->GetFrameLineColor(), *frameBorderAlpha));
    if (auto frameBorderStyle = get_first(pad.GetFrameBorderStyle(), padDefaults.GetFrameBorderStyle())) pad_ptr->SetFrameLineStyle(*frameBorderStyle);
    if (auto frameBorderWidth = get_first(pad.GetFrameBorderWidth(), padDefaults.GetFrameBorderWidth())) pad_ptr->SetFrameLineWidth(*frameBorderWidth);
    if (auto candleBoxRange = get_first(pad.GetDefaultCandleBoxRange(), padDefaults.GetDefaultCandleBoxRange())) TCandle::SetBoxRange(*candleBoxRange);
    if (auto candleWhiskerRange = get_first(pad.GetDefaultCandleWhiskerRange(), padDefaults.GetDefaultCandleWhiskerRange())) TCandle::SetWhiskerRange(*candleWhiskerRange);

    if (pad.GetDefaultMarkerColorsGradient().rgbEndpoints) {
      const auto& gradient = pad.GetDefaultMarkerColorsGradient();
      int32_t nColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [](auto data) { return !data->GetMarkerColor(); }));
      pad.SetDefaultMarkerColors(GenerateGradientColors(get_first_or(nColors, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    } else if (padDefaults.GetDefaultMarkerColorsGradient().rgbEndpoints) {
      auto& gradient = padDefaults.GetDefaultMarkerColorsGradient();
      int32_t nColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [](auto data) { return !data->GetMarkerColor(); }));
      padDefaults.SetDefaultMarkerColors(GenerateGradientColors(get_first_or(nColors, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    }
    if (pad.GetDefaultLineColorsGradient().rgbEndpoints) {
      const auto& gradient = pad.GetDefaultLineColorsGradient();
      int32_t nColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [](auto data) { return !data->GetLineColor(); }));
      pad.SetDefaultLineColors(GenerateGradientColors(get_first_or(nColors, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    } else if (padDefaults.GetDefaultLineColorsGradient().rgbEndpoints) {
      const auto& gradient = padDefaults.GetDefaultLineColorsGradient();
      int32_t nColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [](auto data) { return !data->GetLineColor(); }));
      padDefaults.SetDefaultLineColors(GenerateGradientColors(get_first_or(nColors, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    }
    if (pad.GetDefaultFillColorsGradient().rgbEndpoints) {
      const auto& gradient = pad.GetDefaultFillColorsGradient();
      int32_t nColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [](auto data) { return !data->GetFillColor(); }));
      pad.SetDefaultFillColors(GenerateGradientColors(get_first_or(nColors, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    } else if (padDefaults.GetDefaultFillColorsGradient().rgbEndpoints) {
      const auto& gradient = padDefaults.GetDefaultFillColorsGradient();
      int32_t nColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [](auto data) { return !data->GetFillColor(); }));
      padDefaults.SetDefaultFillColors(GenerateGradientColors(get_first_or(nColors, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    }
    if (pad.GetPaletteGradient().rgbEndpoints) {
      const auto& gradient = pad.GetPaletteGradient();
      GenerateGradientColors(get_first_or(255, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha), true);
    } else if (padDefaults.GetPaletteGradient().rgbEndpoints) {
      const auto& gradient = padDefaults.GetPaletteGradient();
      GenerateGradientColors(get_first_or(255, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha), true);
    } else if (auto palette = get_first(pad.GetPalette(), padDefaults.GetPalette())) {
      gStyle->SetPalette(*palette);
    } else {
      // reset to default to avoid side effects in other plots
      gStyle->SetPalette(kBird);
    }
    pad_ptr->SetNumber(padID);
    pad_ptr->Draw();
    pad_ptr->cd();
    bool hasRefFunc = false;
    auto drawData = pad.GetData();

    if (drawData.empty()) {
      if (pad.GetLegendBoxes().empty() && pad.GetTextBoxes().empty()) {
        WARNING("Nothing to be drawn in pad {}.", padID);
        continue;
      }
    } else {
      // find data that should define the axis frame (selects the most recently added data that wants to define frame)
      auto framePos = std::find_if(drawData.rbegin(), drawData.rend(), [](const auto& curData) { return curData->GetDefinesFrame(); });
      size_t frameDataID = (framePos != drawData.rend()) ? static_cast<size_t>((framePos.base() - 1) - drawData.begin()) : 0u;
      // make a copy of data that will serve as axis frame and put it in front of data vector
      if (drawData[frameDataID]->GetType() == "ratio") {
        drawData.insert(drawData.begin(), std::make_shared<Plot::Pad::Ratio>(*std::dynamic_pointer_cast<Plot::Pad::Ratio>(drawData[frameDataID])));
      } else {
        drawData.insert(drawData.begin(), std::make_shared<Plot::Pad::Data>(*drawData[frameDataID]));
      }
      drawData[0]->SetLegendLabel("");  // axis frame should not appear in legend

      // put reference function in data vector right after the axis histogram
      auto refFunc = (pad.GetRefFunc()) ? pad.GetRefFunc() : padDefaults.GetRefFunc();
      if (refFunc) {
        drawData.insert(drawData.begin() + 1, refFunc);
        hasRefFunc = true;
      }
    }

    TH1* axisHist_ptr{nullptr};
    string drawingOptions;
    uint16_t dataIndex{};
    array<uint16_t, 6> defaultSettingIndices = {0};
    const int userErrorLevel = gErrorIgnoreLevel;
    auto errLevelGuard = make_scope_guard([userErrorLevel]() { gErrorIgnoreLevel = userErrorLevel; });
    for (const auto& data : drawData) {
      if (fail) break;
      if (data->GetDrawingOptions()) drawingOptions += *data->GetDrawingOptions();
      // obtain a copy of the current data
      // retrieve the actual pointer to the data
      auto processData = [&, padID = padID](auto&& data_ptr) {
        using data_type = std::decay_t<decltype(data_ptr)>;
        gErrorIgnoreLevel = dataIndex ? userErrorLevel : kFatal;
        optional<drawing_options_t> defaultDrawingOption = data->GetDrawingOptionAlias();

        if (!data->GetDrawingOptions()) {
          // MEMO: avoid code duplication here by implementing this in more clever way
          if constexpr (is_hist_2d<data_type>()) {
            if (!defaultDrawingOption) {
              if (pad.GetDefaultDrawingOptionHist2d())
                defaultDrawingOption = pad.GetDefaultDrawingOptionHist2d();
              else if (padDefaults.GetDefaultDrawingOptionHist2d())
                defaultDrawingOption = padDefaults.GetDefaultDrawingOptionHist2d();
            }

            if (defaultDrawingOption) {
              if (defaultDrawingOptions_Hist2d.find(*defaultDrawingOption) != defaultDrawingOptions_Hist2d.end()) {
                drawingOptions += defaultDrawingOptions_Hist2d.at(*defaultDrawingOption);
              } else if (dataIndex != 0) {
                WARNING("Default drawing option not defined for 2d histogram ({}).", data_ptr->GetName());
              }
            }
          } else if constexpr (is_hist_1d<data_type>()) {
            if (!defaultDrawingOption)
              defaultDrawingOption = (pad.GetDefaultDrawingOptionHist())
                                       ? pad.GetDefaultDrawingOptionHist()
                                     : (padDefaults.GetDefaultDrawingOptionHist())
                                       ? padDefaults.GetDefaultDrawingOptionHist()
                                       : nullopt;

            if (defaultDrawingOption) {
              if (defaultDrawingOptions_Hist.find(*defaultDrawingOption) != defaultDrawingOptions_Hist.end()) {
                drawingOptions += defaultDrawingOptions_Hist.at(*defaultDrawingOption);
              } else if (dataIndex != 0) {
                WARNING("Default drawing option not defined for 1d histogram ({}).", data_ptr->GetName());
              }
            }
          } else if constexpr (is_graph_1d<data_type>()) {
            if (!defaultDrawingOption)
              defaultDrawingOption = (pad.GetDefaultDrawingOptionGraph())
                                       ? pad.GetDefaultDrawingOptionGraph()
                                     : (padDefaults.GetDefaultDrawingOptionGraph())
                                       ? padDefaults.GetDefaultDrawingOptionGraph()
                                       : nullopt;

            if (defaultDrawingOption) {
              if (defaultDrawingOptions_Graph.find(*defaultDrawingOption) != defaultDrawingOptions_Graph.end()) {
                drawingOptions += defaultDrawingOptions_Graph.at(*defaultDrawingOption);
              } else if (dataIndex != 0) {
                WARNING("Default drawing option not defined for graph ({}).", data_ptr->GetName());
              }
            }
          }
        }

        auto warn = [&](auto&&... args) {
          if (dataIndex != 0) WARNING(std::forward<decltype(args)>(args)...);
        };

        // apply the data modifiers stored in mods to ptr (plain data, ratio inputs and ratio result)
        auto applyModifiers = [&](auto& ptr, const Plot::Pad::Data& mods) {
          using ptr_type = std::decay_t<decltype(ptr)>;
          auto warnUnsupported = [&](bool requested, const char* what) {
            if (requested) warn("{} is not supported for {} ({}), ignoring.", what, ptr->GetName(), ptr->ClassName());
          };

          auto scaleAxis = [&](int16_t axisIndex, const optional<double_t>& factor) {
            if (!factor) return;
            if constexpr (is_hist_1d<ptr_type>()) {
              if (axisIndex == 1) return;  // y is bin content here - equivalent to Scale()
            } else if constexpr (is_hist_2d<ptr_type>()) {
              if (axisIndex == 2) return;  // z is bin content here - equivalent to Scale()
            } else if constexpr (is_graph_1d<ptr_type>()) {
              if (axisIndex == 1) return;  // same operation as Scale() for a graph
            } else if constexpr (is_graph_2d<ptr_type>()) {
              if (axisIndex == 2) return;  // z is the value Scale() targets for a 2d graph
            } else if constexpr (is_func<ptr_type>()) {
              return;
            }
            string axisLetter = GetAxisStr(axisIndex);
            if (*factor <= 0.) {
              warn("Scale factor for {} axis of {} must be positive, ignoring.", axisLetter, ptr->GetName());
              return;
            }
            if constexpr (is_hist_1d<ptr_type>()) {
              if (axisIndex == 2) {
                warn("Cannot scale z axis of 1d histogram {} (no such axis).", ptr->GetName());
                return;
              }
              ScaleAxis(ptr, axisIndex, *factor);
            } else if constexpr (is_hist_2d<ptr_type>()) {
              ScaleAxis(ptr, axisIndex, *factor);
            } else if constexpr (is_hist_3d<ptr_type>()) {
              ScaleAxis(ptr, axisIndex, *factor);
            } else if constexpr (is_graph_1d<ptr_type>()) {
              if (axisIndex == 2) {
                warn("Cannot scale z axis of graph {} (graphs have no z-axis).", ptr->GetName());
                return;
              }
              ScaleGraphAxis(ptr, axisIndex, *factor);
            } else if constexpr (is_graph_2d<ptr_type>()) {
              ScaleGraphAxis(ptr, axisIndex, *factor);
            }
          };
          scaleAxis(0, mods.GetScaleAxisX());
          scaleAxis(1, mods.GetScaleAxisY());
          scaleAxis(2, mods.GetScaleAxisZ());

          if constexpr (is_hist<ptr_type>()) {
            if (!ptr->GetSumw2N()) ptr->Sumw2();
            if constexpr (is_hist_2d<ptr_type>()) {
              if (mods.GetRebinGroupX() && mods.GetRebinGroupY()) {
                ptr->Rebin2D(*mods.GetRebinGroupX(), *mods.GetRebinGroupY());
              } else if (mods.GetRebinGroupX()) {
                ptr->RebinX(*mods.GetRebinGroupX());
              } else if (mods.GetRebinGroupY()) {
                ptr->RebinY(*mods.GetRebinGroupY());
              }
              warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
            } else if constexpr (is_hist_3d<ptr_type>()) {
              if (mods.GetRebinGroupX()) {
                ptr->RebinX(*mods.GetRebinGroupX());
              }
              if (mods.GetRebinGroupY()) {
                ptr->RebinY(*mods.GetRebinGroupY());
              }
              if (mods.GetRebinGroupZ()) {
                ptr->RebinZ(*mods.GetRebinGroupZ());
              }
            } else {
              if (mods.GetRebinGroupX()) {
                ptr->RebinX(*mods.GetRebinGroupX());
              }
              warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
              warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
            }
            bool isDensity = mods.GetScaleBinWidthNorm() && *mods.GetScaleBinWidthNorm();
            if (mods.GetDivideBinWidth() && *mods.GetDivideBinWidth()) {
              ptr->Scale(1., "width");
              isDensity = true;
            }
            if (mods.GetNiterSmooth()) {
              if constexpr (is_one_of_v<ptr_type, TProfile*, TProfile2D*>()) {
                warn("Smooth is not supported for profile histogram {} (would corrupt its per-bin entry counts), ignoring.", ptr->GetName());
              } else {
                ptr->Smooth(*mods.GetNiterSmooth());
              }
            }

            // normalize to the integral before Cumulative, then normalized distribution results in CDF ending at 1
            if (mods.GetScaleBinWidthNorm()) {
              string scaleMode = (isDensity) ? "width" : "";
              double_t integral = ptr->Integral(scaleMode.data());
              if (integral == 0.) {
                warn("Cannot normalize histogram because integral is zero.");
              } else {
                ptr->Scale(1. / integral);
              }
            }

            if constexpr (is_hist_1d<ptr_type>()) {
              if (mods.GetCumulative()) {
                if constexpr (is_one_of_v<ptr_type, TProfile*>()) {
                  warn("Cumulative is not supported for profile histogram {} (bin content would become invalid), ignoring.", ptr->GetName());
                } else {
                  if (isDensity) {
                    // the running sum of a density has to be weighted with the bin widths to yield its integral
                    for (int32_t i = 1; i <= ptr->GetNbinsX(); ++i) {
                      double_t width = ptr->GetBinWidth(i);
                      ptr->SetBinContent(i, ptr->GetBinContent(i) * width);
                      ptr->SetBinError(i, ptr->GetBinError(i) * width);
                    }
                  }
                  TH1* cumulativeHist = ptr->GetCumulative(*mods.GetCumulative());
                  cumulativeHist->SetDirectory(nullptr);
                  cumulativeHist->SetBit(kCanDelete);
                  delete ptr;
                  ptr = cumulativeHist;
                }
              }
            } else {
              warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
            }
            // remaining factors act on the final shape (NormalizeToMaximum after Cumulative -> CDF ending at 1)
            optional<double_t> scaleFactor;
            if (mods.GetNormMaximum() && *mods.GetNormMaximum()) {
              if (double_t maximum = ptr->GetMaximum(); maximum == 0.) {
                warn("Cannot normalize {} to maximum because it is zero.", ptr->GetName());
              } else {
                scaleFactor = 1. / maximum;
              }
            }
            if (auto factor = mods.GetScaleFactor()) {
              if (*factor <= 0.) {
                warn("Scale factor for {} must be positive, ignoring.", ptr->GetName());
              } else {
                scaleFactor = (scaleFactor) ? (*scaleFactor) * (*factor) : (*factor);
              }
            }
            if constexpr (is_hist_1d<ptr_type>()) {
              if (auto axisScale = mods.GetScaleAxisY()) {
                if (*axisScale <= 0.) {
                  warn("Scale factor for y axis of {} must be positive, ignoring.", ptr->GetName());
                } else {
                  scaleFactor = (scaleFactor) ? (*scaleFactor) * (*axisScale) : (*axisScale);
                }
              }
            } else if constexpr (is_hist_2d<ptr_type>()) {
              if (auto axisScale = mods.GetScaleAxisZ()) {
                if (*axisScale <= 0.) {
                  warn("Scale factor for z axis of {} must be positive, ignoring.", ptr->GetName());
                } else {
                  scaleFactor = (scaleFactor) ? (*scaleFactor) * (*axisScale) : (*axisScale);
                }
              }
            }
            if (scaleFactor) ptr->Scale(*scaleFactor);
          } else if constexpr (is_graph_1d<ptr_type>()) {
            warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
            warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
            warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
            warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
            warnUnsupported(mods.GetShowOverflowBins().has_value(), "ShowOverflowBins");
            warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
            if (mods.GetNiterSmooth()) {
              if (ptr->GetN() < 4) {
                warn("Smooth needs at least 4 points, {} has {}; ignoring.", ptr->GetName(), ptr->GetN());
              } else {
                ptr->Sort();
                TGraphSmooth smoother;
                for (uint16_t iter = 0; iter < *mods.GetNiterSmooth(); ++iter) {
                  TGraph* smoothGraph = smoother.SmoothSuper(ptr);
                  for (int32_t i = 0; i < ptr->GetN(); ++i) {
                    ptr->GetY()[i] = smoothGraph->GetY()[i];
                  }
                }
              }
            }
            optional<double_t> scaleFactor;
            string scaleMode{};
            if (mods.GetScaleBinWidthNorm()) {
              double_t integral = ptr->Integral();
              if (integral == 0.) {
                warn("Cannot normalize graph because integral is zero.");
              } else {
                scaleFactor = 1. / integral;
              }
              if (*mods.GetScaleBinWidthNorm()) {
                warn("Cannot normalize graph by width.");
              }
            }
            if (mods.GetNormMaximum() && *mods.GetNormMaximum()) {
              if (ptr->GetN() == 0) {
                warn("Cannot normalize graph {} to maximum because it has no points.", ptr->GetName());
              } else if (double_t maxY = TMath::MaxElement(ptr->GetN(), ptr->GetY()); maxY == 0.) {
                warn("Cannot normalize graph {} to maximum because it is zero.", ptr->GetName());
              } else {
                scaleFactor = 1. / maxY;
              }
            }
            if (auto factor = mods.GetScaleFactor()) {
              if (*factor <= 0.) {
                warn("Scale factor for {} must be positive, ignoring.", ptr->GetName());
              } else {
                scaleFactor = (scaleFactor) ? (*scaleFactor) * (*factor) : (*factor);
              }
            }
            if (auto axisScale = mods.GetScaleAxisY()) {
              if (*axisScale <= 0.) {
                warn("Scale factor for y axis of {} must be positive, ignoring.", ptr->GetName());
              } else {
                scaleFactor = (scaleFactor) ? (*scaleFactor) * (*axisScale) : (*axisScale);
              }
            }
            if (scaleFactor) ScaleGraphAxis(ptr, 1, *scaleFactor);
          } else if constexpr (is_graph_2d<ptr_type>()) {
            warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
            warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
            warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
            warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
            warnUnsupported(mods.GetNiterSmooth().has_value(), "Smooth");
            warnUnsupported(mods.GetShowOverflowBins().has_value(), "ShowOverflowBins");
            warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
            warnUnsupported(mods.GetScaleBinWidthNorm().has_value(), "Normalize");
            optional<double_t> scaleFactor;
            if (mods.GetNormMaximum() && *mods.GetNormMaximum()) {
              if (ptr->GetN() == 0) {
                warn("Cannot normalize graph {} to maximum because it has no points.", ptr->GetName());
              } else if (double_t maxZ = TMath::MaxElement(ptr->GetN(), ptr->GetZ()); maxZ == 0.) {
                warn("Cannot normalize graph {} to maximum because it is zero.", ptr->GetName());
              } else {
                scaleFactor = 1. / maxZ;
              }
            }
            if (auto factor = mods.GetScaleFactor()) {
              if (*factor <= 0.) {
                warn("Scale factor for {} must be positive, ignoring.", ptr->GetName());
              } else {
                scaleFactor = (scaleFactor) ? (*scaleFactor) * (*factor) : (*factor);
              }
            }
            if (auto axisScale = mods.GetScaleAxisZ()) {
              if (*axisScale <= 0.) {
                warn("Scale factor for z axis of {} must be positive, ignoring.", ptr->GetName());
              } else {
                scaleFactor = (scaleFactor) ? (*scaleFactor) * (*axisScale) : (*axisScale);
              }
            }
            if (scaleFactor) ScaleGraphAxis(ptr, 2, *scaleFactor);
          } else if constexpr (is_func<ptr_type>()) {
            warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
            warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
            warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
            warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
            warnUnsupported(mods.GetNiterSmooth().has_value(), "Smooth");
            warnUnsupported(mods.GetNormMaximum() && *mods.GetNormMaximum(), "NormalizeToMaximum");
            warnUnsupported(mods.GetScaleBinWidthNorm().has_value(), "Normalize");
            warnUnsupported(mods.GetShowOverflowBins().has_value(), "ShowOverflowBins");
            warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
            if constexpr (is_func_1d<ptr_type>()) {
              warnUnsupported(mods.GetScaleAxisZ().has_value(), "ScaleZ");
            }
            double_t domainFactorX = 1.;
            if (auto axisScale = mods.GetScaleAxisX()) {
              if (*axisScale <= 0.) {
                warn("Scale factor for x axis of {} must be positive, ignoring.", ptr->GetName());
              } else {
                domainFactorX = *axisScale;
              }
            }
            double_t domainFactorY = 1.;
            if constexpr (is_func_2d<ptr_type>() || is_func_3d<ptr_type>()) {
              if (auto axisScale = mods.GetScaleAxisY()) {
                if (*axisScale <= 0.) {
                  warn("Scale factor for y axis of {} must be positive, ignoring.", ptr->GetName());
                } else {
                  domainFactorY = *axisScale;
                }
              }
            }
            double_t domainFactorZ = 1.;
            if constexpr (is_func_3d<ptr_type>()) {
              if (auto axisScale = mods.GetScaleAxisZ()) {
                if (*axisScale <= 0.) {
                  warn("Scale factor for z axis of {} must be positive, ignoring.", ptr->GetName());
                } else {
                  domainFactorZ = *axisScale;
                }
              }
            }
            optional<double_t> contentFactor;
            if (auto factor = mods.GetScaleFactor()) {
              if (*factor <= 0.) {
                warn("Scale factor for {} must be positive, ignoring.", ptr->GetName());
              } else {
                contentFactor = *factor;
              }
            }
            if constexpr (is_func_1d<ptr_type>()) {
              if (auto axisScale = mods.GetScaleAxisY()) {
                if (*axisScale <= 0.) {
                  warn("Scale factor for y axis of {} must be positive, ignoring.", ptr->GetName());
                } else {
                  contentFactor = (contentFactor) ? (*contentFactor) * (*axisScale) : (*axisScale);
                }
              }
            } else if constexpr (is_func_2d<ptr_type>()) {
              if (auto axisScale = mods.GetScaleAxisZ()) {
                if (*axisScale <= 0.) {
                  warn("Scale factor for z axis of {} must be positive, ignoring.", ptr->GetName());
                } else {
                  contentFactor = (contentFactor) ? (*contentFactor) * (*axisScale) : (*axisScale);
                }
              }
            }
            if (domainFactorX != 1. || domainFactorY != 1. || domainFactorZ != 1. || contentFactor) {
              if constexpr (is_func_1d<ptr_type>()) {
                ptr = ScaleFunc(ptr, domainFactorX, contentFactor.value_or(1.));
              } else if constexpr (is_func_2d<ptr_type>()) {
                ptr = ScaleFunc(ptr, domainFactorX, domainFactorY, contentFactor.value_or(1.));
              } else if constexpr (is_func_3d<ptr_type>()) {
                ptr = ScaleFunc(ptr, domainFactorX, domainFactorY, domainFactorZ, contentFactor.value_or(1.));
              }
            }
          }
        };

        if (data->GetType() == "ratio") {
          auto data_as_ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
          bool binomialErrors = data_as_ratio->GetIsCorrelated();
          // modifiers requested via Numer() / Denom() act on the inputs before the division
          auto operandMods = [&](const auto& operandModify) {
            Plot::Pad::Data mods = *data;
            mods.Modify() = operandModify;
            return mods;
          };
          applyModifiers(data_ptr, operandMods(data_as_ratio->GetNumModify()));
          auto processDenominator = [&](auto&& denom_data_ptr) {
            using denom_data_type = std::decay_t<decltype(denom_data_ptr)>;
            applyModifiers(denom_data_ptr, operandMods(data_as_ratio->GetDenomModify()));
            if constexpr (is_hist<data_type>()) {
              if constexpr (is_func<denom_data_type>()) {
                if (!Divide(data_ptr, denom_data_ptr, binomialErrors)) fail = true;
              } else if constexpr (is_hist<denom_data_type>()) {
                if (!Divide(data_ptr, denom_data_ptr, binomialErrors)) fail = true;
                if constexpr (is_hist_2d<data_type>()) {
                  data_ptr->GetZaxis()->SetTitle("ratio");
                } else if constexpr (is_hist_1d<data_type>()) {
                  data_ptr->GetYaxis()->SetTitle("ratio");
                }
              } else if constexpr (is_hist_1d<data_type>() && is_graph_1d<denom_data_type>()) {
                if (!Divide(data_ptr, denom_data_ptr, binomialErrors)) fail = true;
              }
            } else if constexpr (is_graph_1d<data_type>()) {
              if constexpr (is_graph_1d<denom_data_type>() || is_hist_1d<denom_data_type>() || is_func_1d<denom_data_type>()) {
                if (!Divide(data_ptr, denom_data_ptr, binomialErrors)) fail = true;
                data_ptr->GetHistogram()->GetYaxis()->SetTitle("ratio");
              }
            } else {
              ERROR("Unsupported division of {} and {}.", data_ptr->ClassName(), denom_data_ptr->ClassName());
            }
            delete denom_data_ptr;
          };

          // retrieve the actual pointer to the denominator data
          optional<data_ptr_t> rawDenomData;
          try {
            rawDenomData = GetDataClone(dataBuffer.at(data_as_ratio->GetDenomDataSource()).at(data_as_ratio->GetDenomName() + data_as_ratio->GetDenomDataInfo().GetNameSuffix()).get(), data_as_ratio->GetDenomProjInfo());
          } catch (std::out_of_range&) {
            rawDenomData = nullopt;
          }

          if (rawDenomData) {
            std::visit(processDenominator, *rawDenomData);
          } else {
            fail = true;
          }

          // modifiers on the ratio itself are applied as requested, but most of them are rarely meaningful there
          constexpr auto hint = "use Numer() / Denom() to apply it to the inputs before the division";
          if (data->GetRebinGroupX() || data->GetRebinGroupY() || data->GetRebinGroupZ()) {
            warn("Rebinning ratio {} sums ratio values; {}.", data_ptr->GetName(), hint);
          }
          if (data->GetCumulative()) {
            warn("Cumulative of ratio {} sums ratio values; {}.", data_ptr->GetName(), hint);
          }
          if (data->GetDivideBinWidth() && *data->GetDivideBinWidth()) {
            warn("Dividing ratio {} by bin width is rarely meaningful; {}.", data_ptr->GetName(), hint);
          }
          if (data->GetScaleBinWidthNorm()) {
            warn("Normalizing ratio {} to its integral is rarely meaningful; {}.", data_ptr->GetName(), hint);
          }
        }  // end ratio code

        applyModifiers(data_ptr, *data);

        // NaN or infinite values (e.g. from a division by zero) spoil the axis ranges and ROOT then draws nothing
        if constexpr (is_one_of_v<data_type, TH1*, TH2*, TH3*>()) {
          int32_t nInvalid{};
          for (int32_t bin = 0; bin < data_ptr->GetNcells(); ++bin) {
            if (!std::isfinite(data_ptr->GetBinContent(bin)) || !std::isfinite(data_ptr->GetBinError(bin))) {
              data_ptr->SetBinContent(bin, 0.);
              data_ptr->SetBinError(bin, 0.);
              ++nInvalid;
            }
          }
          if (nInvalid) warn("{} has {} bin{} with NaN or infinite values, which are drawn as zero.", data_ptr->GetName(), nInvalid, (nInvalid == 1) ? "" : "s");
        } else if constexpr (is_graph_1d<data_type>()) {
          int32_t nInvalid{};
          for (int32_t i = data_ptr->GetN() - 1; i >= 0; --i) {
            if (!std::isfinite(data_ptr->GetPointX(i)) || !std::isfinite(data_ptr->GetPointY(i))) {
              data_ptr->RemovePoint(i);
              ++nInvalid;
            }
          }
          if (nInvalid) warn("{} has {} point{} with NaN or infinite values, which are not drawn.", data_ptr->GetName(), nInvalid, (nInvalid == 1) ? "" : "s");
        }

        // first data is only used to define the axes
        if (dataIndex == 0) {
          // smallest positive value of the data, needed for a log scale (infinity if there is none);
          // for graphs taken from the points, since their axis frame is an empty histogram (and drawing it below deletes the graph)
          optional<double_t> graphPositiveMinimum;
          if constexpr (is_graph_1d<data_type>() || is_graph_2d<data_type>()) {
            graphPositiveMinimum = std::numeric_limits<double_t>::infinity();
            for (int32_t i = 0; i < data_ptr->GetN(); ++i) {
              double_t value{};
              if constexpr (is_graph_1d<data_type>()) {
                value = data_ptr->GetPointY(i);
              } else {
                value = data_ptr->GetZ()[i];
              }
              if (value > 0.) graphPositiveMinimum = std::min(*graphPositiveMinimum, value);
            }
          }
          data_ptr->Draw(drawingOptions.data());
          if constexpr (is_hist<data_type>()) {
            axisHist_ptr = data_ptr;
          } else {
            axisHist_ptr = static_cast<TH1*>(data_ptr->GetHistogram()->Clone());
            axisHist_ptr->SetDirectory(nullptr);
            axisHist_ptr->SetBit(kCanDelete);
          }
          string drawOptAxis = "AXIS";
          pad_ptr->Update();
          if (pad_ptr->GetView() || is_hist_2d<data_type>()) {
            drawOptAxis = "";
          }
          axisHist_ptr->Draw((drawingOptions + drawOptAxis).data());
          axisHist_ptr->Draw((drawingOptions + "SAME AXIG").data());
          bool isTHN = axisHist_ptr->InheritsFrom(TH2::Class()) || axisHist_ptr->InheritsFrom(TH3::Class());

          // a user range beyond the axes of the frame histogram needs a wider frame, since ROOT shows at most the axis of a histogram:
          // the frame is replaced by a histogram with extended axes (changing the axis alone would break the bin storage of the histogram)
          if (!pad_ptr->GetView()) {
            for (auto axisLabel : {'X', 'Y'}) {
              if (axisLabel == 'Y' && !isTHN) continue;
              optional<double_t> userMin;
              optional<double_t> userMax;
              for (Plot::Pad& curPad : {std::ref(padDefaults), std::ref(plot.GetPads()[padID])}) {
                if (curPad.GetAxes().find(axisLabel) == curPad.GetAxes().end()) continue;
                if (curPad[axisLabel].GetMinRange()) userMin = curPad[axisLabel].GetMinRange();
                if (curPad[axisLabel].GetMaxRange()) userMax = curPad[axisLabel].GetMaxRange();
              }
              const TAxis* axis = (axisLabel == 'X') ? axisHist_ptr->GetXaxis() : axisHist_ptr->GetYaxis();
              constexpr double_t relTol = 1e-12;
              if (userMin && !(*userMin < axis->GetXmin() && !TMath::AreEqualRel(*userMin, axis->GetXmin(), relTol))) userMin.reset();
              if (userMax && !(*userMax > axis->GetXmax() && !TMath::AreEqualRel(*userMax, axis->GetXmax(), relTol))) userMax.reset();
              if (!userMin && !userMax) continue;
              TH1* extended = ExtendFrameAxis(axisHist_ptr, axisLabel, userMin, userMax);
              // the added empty bins must not change the automatic range of the dependent axis: keep the one of the original frame
              // (for 1d the range ROOT determined when drawing it, for 2d the extremes of the contents, which define the colour scale below)
              if (axisHist_ptr->GetMinimumStored() == -1111 && axisHist_ptr->GetMaximumStored() == -1111) {
                if (isTHN) {
                  extended->SetMinimum(axisHist_ptr->GetMinimum());
                  extended->SetMaximum(axisHist_ptr->GetMaximum());
                } else {
                  extended->SetMinimum(pad_ptr->GetLogy() ? TMath::Power(10., pad_ptr->GetUymin()) : pad_ptr->GetUymin());
                  extended->SetMaximum(pad_ptr->GetLogy() ? TMath::Power(10., pad_ptr->GetUymax()) : pad_ptr->GetUymax());
                }
              }
              TList* primitives = pad_ptr->GetListOfPrimitives();
              while (primitives->Remove(axisHist_ptr)) {
              }
              delete axisHist_ptr;
              axisHist_ptr = extended;
              axisHist_ptr->Draw(drawingOptions.data());
              axisHist_ptr->Draw((drawingOptions + drawOptAxis).data());
              axisHist_ptr->Draw((drawingOptions + "SAME AXIG").data());
            }
          }
          axisHist_ptr->SetName(string("axis_hist_pad_" + std::to_string(padID)).data());
          axisHist_ptr->SetStats(false);
          axisHist_ptr->SetTitle("");
          axisHist_ptr->SetBit(TH1::kNoTitle);

          // apply axis settings
          for (auto axisLabel : {'X', 'Y', 'Z'}) {
            TAxis* axis_ptr = nullptr;
            if (axisLabel == 'X')
              axis_ptr = axisHist_ptr->GetXaxis();
            else if (axisLabel == 'Y')
              axis_ptr = axisHist_ptr->GetYaxis();
            else if (axisLabel == 'Z')
              axis_ptr = axisHist_ptr->GetZaxis();
            if (!axis_ptr) continue;

            auto textFontTitle = textFont;
            auto textSizeTitle = textSize;
            auto textColorTitle = textColor;
            auto textAlphaTitle = textAlpha;
            auto textFontLabel = textFont;
            auto textSizeLabel = textSize;
            auto textColorLabel = textColor;
            auto textAlphaLabel = textAlpha;

            optional<double_t> userRangeMin;
            optional<double_t> userRangeMax;

            // first apply default pad values and then settings for this specific pad
            for (Plot::Pad& curPad : {std::ref(padDefaults), std::ref(plot.GetPads()[padID])}) {
              if (curPad.GetAxes().find(axisLabel) != curPad.GetAxes().end()) {
                const auto& axisLayout = curPad[axisLabel];
                if (axisLayout.GetTitle()) axis_ptr->SetTitle((*axisLayout.GetTitle()).data());

                if (axisLayout.GetTitleFont()) textFontTitle = axisLayout.GetTitleFont();
                if (axisLayout.GetLabelFont()) textFontLabel = axisLayout.GetLabelFont();

                if (axisLayout.GetTitleColor()) textColorTitle = axisLayout.GetTitleColor();
                if (axisLayout.GetLabelColor()) textColorLabel = axisLayout.GetLabelColor();

                if (axisLayout.GetTitleAlpha()) textAlphaTitle = axisLayout.GetTitleAlpha();
                if (axisLayout.GetLabelAlpha()) textAlphaLabel = axisLayout.GetLabelAlpha();

                if (axisLayout.GetTitleSize()) textSizeTitle = axisLayout.GetTitleSize();
                if (axisLayout.GetLabelSize()) textSizeLabel = axisLayout.GetLabelSize();

                if (axisLayout.GetTitleCenter()) axis_ptr->CenterTitle(*axisLayout.GetTitleCenter());
                if (axisLayout.GetLabelCenter()) axis_ptr->CenterLabels(*axisLayout.GetLabelCenter());

                if (axisLayout.GetAxisColor()) axis_ptr->SetAxisColor(*axisLayout.GetAxisColor());
                if (axisLayout.GetAxisAlpha()) axis_ptr->SetAxisColor(TColor::GetColorTransparent(axis_ptr->GetAxisColor(), *axisLayout.GetAxisAlpha()));

                if (axisLayout.GetTitleOffset()) axis_ptr->SetTitleOffset(*axisLayout.GetTitleOffset());
                if (axisLayout.GetLabelOffset()) axis_ptr->SetLabelOffset(*axisLayout.GetLabelOffset());

                if (axisLayout.GetTickLength()) axis_ptr->SetTickLength(*axisLayout.GetTickLength());
                if (axisLayout.GetMaxDigits()) axis_ptr->SetMaxDigits(*axisLayout.GetMaxDigits());

                if (axisLayout.GetNumDivisions()) axis_ptr->SetNdivisions(*axisLayout.GetNumDivisions());

                if (axisLayout.GetOppositeTicks()) {
                  if (axisLabel == 'X') {
                    pad_ptr->SetTickx(*axisLayout.GetOppositeTicks());
                  } else if (axisLabel == 'Y') {
                    pad_ptr->SetTicky(*axisLayout.GetOppositeTicks());
                  }
                }
                if (axisLayout.GetNoExponent()) {
                  axis_ptr->SetNoExponent(*axisLayout.GetNoExponent());
                }
                if (axisLayout.GetTimeFormat()) {
                  axis_ptr->SetTimeDisplay(1);
                  axis_ptr->SetTimeFormat((*axisLayout.GetTimeFormat()).data());
                }
                if (axisLayout.GetTickOrientation()) {
                  axis_ptr->SetTicks((*axisLayout.GetTickOrientation()).data());
                }

                // the ranges are applied below, once the default and pad-specific settings are merged
                if (axisLayout.GetMinRange()) userRangeMin = axisLayout.GetMinRange();
                if (axisLayout.GetMaxRange()) userRangeMax = axisLayout.GetMaxRange();

                if (axisLayout.GetLog()) {
                  if (axisLabel == 'X') {
                    pad_ptr->SetLogx(*axisLayout.GetLog());
                  } else if (axisLabel == 'Y') {
                    pad_ptr->SetLogy(*axisLayout.GetLog());
                  } else if (axisLabel == 'Z') {
                    pad_ptr->SetLogz(*axisLayout.GetLog());
                  }
                }
                if (axisLayout.GetGrid()) {
                  if (axisLabel == 'X') {
                    pad_ptr->SetGridx(*axisLayout.GetGrid());
                  } else if (axisLabel == 'Y') {
                    pad_ptr->SetGridy(*axisLayout.GetGrid());
                  }
                }
              }
            }

            // ranges are only touched if the user set one: ROOT's bin edges are not exactly reproducible (e.g. 4.000000000000001 instead of 4),
            // so feeding the drawn range back in through SetRangeUser would add the overflow bin
            if (userRangeMin || userRangeMax) {
              const bool isBinnedAxis = (axisLabel == 'X') || (isTHN && axisLabel == 'Y') || (axisHist_ptr->InheritsFrom(TH3::Class()) && axisLabel == 'Z');
              if (isBinnedAxis) {
                // bin selection as in TAxis::SetRangeUser, but a side the user did not set keeps its current bin
                int32_t first = axis_ptr->GetFirst();
                int32_t last = axis_ptr->GetLast();
                if (userRangeMin) {
                  first = axis_ptr->FindFixBin(*userRangeMin);
                  if (axis_ptr->GetBinUpEdge(first) <= *userRangeMin) ++first;
                }
                if (userRangeMax) {
                  last = axis_ptr->FindFixBin(*userRangeMax);
                  if (axis_ptr->GetBinLowEdge(last) >= *userRangeMax) --last;
                }
                axis_ptr->SetRange(first, last);
              } else {
                // the dependent axis (y of 1d, z of 2d) has no bins: its range is the minimum and maximum of the histogram
                if (userRangeMin) axisHist_ptr->SetMinimum(*userRangeMin);
                if (userRangeMax) axisHist_ptr->SetMaximum(*userRangeMax);
              }
            }

            // a log scale needs a positive range: ROOT otherwise complains and draws nothing (or the data outside of the frame)
            if (!pad_ptr->GetView()) {
              const bool isDependentAxis = (axisLabel == ((isTHN) ? 'Z' : 'Y'));
              const bool isLog = (axisLabel == 'X') ? pad_ptr->GetLogx() : ((axisLabel == 'Y') ? pad_ptr->GetLogy() : pad_ptr->GetLogz());
              auto disableLog = [&]() {
                if (axisLabel == 'X')
                  pad_ptr->SetLogx(false);
                else if (axisLabel == 'Y')
                  pad_ptr->SetLogy(false);
                else
                  pad_ptr->SetLogz(false);
              };
              if (isLog && isDependentAxis) {
                // the smallest positive value is taken from the data itself: GetMinimum(0.) would return the stored minimum (which every graph frame has)
                double_t positiveMinimum = graphPositiveMinimum.value_or(std::numeric_limits<double_t>::infinity());
                for (int32_t bin = 0; !graphPositiveMinimum && bin < axisHist_ptr->GetNcells(); ++bin) {
                  const double_t content = axisHist_ptr->GetBinContent(bin);
                  if (!axisHist_ptr->IsBinUnderflow(bin) && !axisHist_ptr->IsBinOverflow(bin) && content > 0.) positiveMinimum = std::min(positiveMinimum, content);
                }
                if (std::isinf(positiveMinimum)) {
                  WARNING("Log scale of {} axis in pad {} ignored: the data has no positive values.", axisLabel, padID);
                  disableLog();
                } else if ((axisHist_ptr->GetMinimumStored() != -1111) ? (axisHist_ptr->GetMinimumStored() <= 0.) : (axisHist_ptr->GetMinimum() < 0.)) {
                  // ROOT cannot draw a log axis from a stored minimum at or below zero or from negative contents (zero contents it skips itself)
                  axisHist_ptr->SetMinimum(0.5 * positiveMinimum);
                }
              } else if (isLog && !isDependentAxis) {
                const int32_t firstBin = std::max(axis_ptr->GetFirst(), 1);
                const int32_t lastBin = std::min(axis_ptr->GetLast(), axis_ptr->GetNbins());
                if (axis_ptr->GetBinLowEdge(firstBin) < 0.) {  // ROOT itself copes with a range starting at zero
                  if (axis_ptr->GetBinUpEdge(lastBin) <= 0.) {
                    WARNING("Log scale of {} axis in pad {} ignored: the axis range has no positive values.", axisLabel, padID);
                    disableLog();
                  } else {
                    int32_t bin = firstBin;
                    while (axis_ptr->GetBinLowEdge(bin) <= 0.) {
                      ++bin;
                    }
                    // start at the first positive bin edge (ROOT would otherwise draw the bins with negative edges left of the frame)
                    const double_t newMin = (bin <= lastBin) ? axis_ptr->GetBinLowEdge(bin) : 1e-3 * axis_ptr->GetBinUpEdge(lastBin);
                    axis_ptr->SetRangeUser(newMin, axis_ptr->GetBinUpEdge(lastBin));
                  }
                }
              }
            }

            if (textFontTitle) axis_ptr->SetTitleFont(*textFontTitle);
            if (textSizeTitle) axis_ptr->SetTitleSize(*textSizeTitle);
            if (textColorTitle) axis_ptr->SetTitleColor(*textColorTitle);
            if (textAlphaTitle) axis_ptr->SetTitleColor(TColor::GetColorTransparent(axis_ptr->GetTitleColor(), *textAlphaTitle));
            if (textFontLabel) axis_ptr->SetLabelFont(*textFontLabel);
            if (textSizeLabel) axis_ptr->SetLabelSize(*textSizeLabel);
            if (textColorLabel) axis_ptr->SetLabelColor(*textColorLabel);
            if (textAlphaLabel) axis_ptr->SetLabelColor(TColor::GetColorTransparent(axis_ptr->GetLabelColor(), *textAlphaLabel));
            // ROOT places a y title with offset 0 automatically next to the widest label, but the gap it leaves is the title size
            // read as a fraction of the pad width (see TGaxis::PaintAxis), i.e. it grows for pads that are wider than high.
            // With a pixel font the gap is the title height in every pad, so relative fonts are converted for the automatic placement.
            if (axisLabel == 'Y' && axis_ptr->GetTitleOffset() == 0.f && axis_ptr->GetTitleFont() % 10 <= 2 && axis_ptr->GetTitleSize() > 0.f && axis_ptr->GetTitleSize() < 1.f) {
              const double_t padWidthPixel = pad_ptr->GetWw() * pad_ptr->GetAbsWNDC();
              const double_t padHeightPixel = pad_ptr->GetWh() * pad_ptr->GetAbsHNDC();
              axis_ptr->SetTitleFont(axis_ptr->GetTitleFont() / 10 * 10 + 3);
              axis_ptr->SetTitleSize(static_cast<float_t>(axis_ptr->GetTitleSize() * std::min(padWidthPixel, padHeightPixel)));
            }
          }

          if (auto minScale = data->GetScaleMinimum()) {
            axisHist_ptr->SetMinimum((*minScale) * axisHist_ptr->GetMinimum());
          }
          if (auto maxScale = data->GetScaleMaximum()) {
            axisHist_ptr->SetMaximum((*maxScale) * axisHist_ptr->GetMaximum());
          }

          if (isTHN) {
            // reset the axis histogram which owns the z axis, while keeping default range defined by the data
            double_t min = axisHist_ptr->GetMinimum();
            double_t max = axisHist_ptr->GetMaximum();
            axisHist_ptr->Reset("ICE");  // reset integral, contents and errors
            axisHist_ptr->SetMinimum(min);
            axisHist_ptr->SetMaximum(max);
            axisHist_ptr->SetMarkerSize(0);
            axisHist_ptr->SetLineWidth(0);
            axisHist_ptr->SetFillStyle(0);
          }
          pad_ptr->Update();
        } else if (!data->GetDontDraw()) {
          // do not draw the Z axis a second time
          std::replace(drawingOptions.begin(), drawingOptions.end(), 'Z', ' ');

          // define data appearance
          if (auto markerColor = get_first(data->GetMarkerColor(),
                                           pick(defaultSettingIndices[0], pad.GetDefaultMarkerColors()),
                                           pick(defaultSettingIndices[0], padDefaults.GetDefaultMarkerColors()))) {
            if (!data->GetMarkerColor()) defaultSettingIndices[0]++;
            data_ptr->SetMarkerColor(*markerColor);
          }
          if (auto markerAlpha = get_first(data->GetMarkerAlpha(),
                                           pad.GetDefaultMarkerAlpha(),
                                           padDefaults.GetDefaultMarkerAlpha())) {
            data_ptr->SetMarkerColor(TColor::GetColorTransparent(data_ptr->GetMarkerColor(), *markerAlpha));
          }
          if (auto markerStyle = get_first(data->GetMarkerStyle(),
                                           pick(defaultSettingIndices[1], pad.GetDefaultMarkerStyles()),
                                           pick(defaultSettingIndices[1], padDefaults.GetDefaultMarkerStyles()))) {
            if (!data->GetMarkerStyle()) defaultSettingIndices[1]++;
            data_ptr->SetMarkerStyle(*markerStyle);
          }
          if (auto markerSize = get_first(data->GetMarkerSize(),
                                          pad.GetDefaultMarkerSize(),
                                          padDefaults.GetDefaultMarkerSize())) {
            data_ptr->SetMarkerSize(*markerSize);
          }
          if (auto lineColor = get_first(data->GetLineColor(),
                                         pick(defaultSettingIndices[2], pad.GetDefaultLineColors()),
                                         pick(defaultSettingIndices[2], padDefaults.GetDefaultLineColors()))) {
            if (!data->GetLineColor()) defaultSettingIndices[2]++;
            data_ptr->SetLineColor(*lineColor);
          }
          if (auto lineAlpha = get_first(data->GetLineAlpha(),
                                         pad.GetDefaultLineAlpha(),
                                         padDefaults.GetDefaultLineAlpha())) {
            data_ptr->SetLineColor(TColor::GetColorTransparent(data_ptr->GetLineColor(), *lineAlpha));
          }
          if (auto lineStyle = get_first(data->GetLineStyle(),
                                         pick(defaultSettingIndices[3], pad.GetDefaultLineStyles()),
                                         pick(defaultSettingIndices[3], padDefaults.GetDefaultLineStyles()))) {
            if (!data->GetLineStyle()) defaultSettingIndices[3]++;
            data_ptr->SetLineStyle(*lineStyle);
          }
          if (auto lineWidth = get_first(data->GetLineWidth(),
                                         pad.GetDefaultLineWidth(),
                                         padDefaults.GetDefaultLineWidth())) {
            data_ptr->SetLineWidth(*lineWidth);
          }
          if (auto fillColor = get_first(data->GetFillColor(),
                                         pick(defaultSettingIndices[4], pad.GetDefaultFillColors()),
                                         pick(defaultSettingIndices[4], padDefaults.GetDefaultFillColors()))) {
            if (!data->GetFillColor()) defaultSettingIndices[4]++;
            data_ptr->SetFillColor(*fillColor);
          }
          if (auto fillAlpha = get_first(data->GetFillAlpha(),
                                         pad.GetDefaultFillAlpha(),
                                         padDefaults.GetDefaultFillAlpha())) {
            data_ptr->SetFillColor(TColor::GetColorTransparent(data_ptr->GetFillColor(), *fillAlpha));
          }
          if (auto fillStyle = get_first(data->GetFillStyle(),
                                         pick(defaultSettingIndices[5], pad.GetDefaultFillStyles()),
                                         pick(defaultSettingIndices[5], padDefaults.GetDefaultFillStyles()))) {
            if (!data->GetFillStyle()) defaultSettingIndices[5]++;
            data_ptr->SetFillStyle(*fillStyle);
          }

          // now define data ranges
          if (axisHist_ptr->GetMinimum()) {
            // TODO: check if this still works for bar histos
            data_ptr->SetMinimum(axisHist_ptr->GetMinimum());  // important for correct display of bar diagrams
          }
          // data_ptr->SetMaximum(axisHist_ptr->GetMaximum());

          double_t xmin = 0, xmax = 0, ymin = 0, ymax = 0;
          pad_ptr->GetRangeAxis(xmin, ymin, xmax, ymax);
          if (pad_ptr->GetLogx()) {
            xmin = TMath::Power(10, xmin);
            xmax = TMath::Power(10, xmax);
          }
          if (pad_ptr->GetLogy()) {
            ymin = TMath::Power(10, ymin);
            ymax = TMath::Power(10, ymax);
          }

          double_t rangeMinX = (data->GetMinRangeX()) ? *data->GetMinRangeX()
                                                      : xmin;
          double_t rangeMaxX = (data->GetMaxRangeX()) ? *data->GetMaxRangeX()
                                                      : xmax;

          double_t rangeMinY = (data->GetMinRangeY()) ? *data->GetMinRangeY()
                                                      : ymin;
          double_t rangeMaxY = (data->GetMaxRangeY()) ? *data->GetMaxRangeY()
                                                      : ymax;

          // for 3d view ignore individual data ranges and always let it coincide with the axes
          if (auto view = pad_ptr->GetView()) {
            double_t minArr[3];
            double_t maxArr[3];
            view->GetRange(minArr, maxArr);
            rangeMinX = minArr[0];
            rangeMaxX = maxArr[0];
            rangeMinY = minArr[1];
            rangeMaxY = maxArr[1];
            data_ptr->SetMinimum(axisHist_ptr->GetMinimum());
            data_ptr->SetMaximum(axisHist_ptr->GetMaximum());
            if constexpr (is_hist_3d<data_type>()) {
              data_ptr->GetZaxis()->SetRangeUser(minArr[2], maxArr[2]);
            }
          }

          if constexpr (is_func_2d<data_type>()) {
            data_ptr->SetRange(rangeMinX, rangeMinY, rangeMaxX, rangeMaxY);
          } else if constexpr (is_func_1d<data_type>()) {
            data_ptr->SetRange(rangeMinX, rangeMaxX);
          } else if constexpr (is_graph_1d<data_type>()) {
            SetGraphRange(static_cast<TGraph*>(data_ptr), data->GetMinRangeX(), data->GetMaxRangeX());
          } else {
            data_ptr->GetXaxis()->SetRangeUser(rangeMinX, rangeMaxX);
          }
          if constexpr (is_hist_2d<data_type>() || is_hist_3d<data_type>()) {
            data_ptr->GetYaxis()->SetRangeUser(rangeMinY, rangeMaxY);
            if (const auto& contours = data->GetContours()) {
              data_ptr->SetContour(static_cast<int32_t>(contours->size()), contours->data());
              if (axisHist_ptr->GetContour() < static_cast<int32_t>(contours->size())) axisHist_ptr->SetContour(static_cast<int32_t>(contours->size()), contours->data());
            } else if (const auto& nContours = data->GetNContours()) {
              data_ptr->SetContour(*nContours);
              if (axisHist_ptr->GetContour() < nContours) axisHist_ptr->SetContour(*nContours);
            }
          }
          if (data->GetTextFormat()) gStyle->SetPaintTextFormat((*data->GetTextFormat()).data());

          // disallow moving around the points of a graph in interactive mode
          if constexpr (is_graph_1d<data_type>()) {
            data_ptr->SetEditable(false);
          }
          if constexpr (is_hist<data_type>()) {
            data_ptr->SetStats(false);
            if (!(data->GetShowOverflowBins() && *data->GetShowOverflowBins())) {
              data_ptr->ClearUnderflowAndOverflow();
            }
          }
          data_ptr->SetName((std::to_string(dataIndex - hasRefFunc) + ":" + data_ptr->GetName()).data());
          data_ptr->Draw(drawingOptions.data());

          // in case a label was specified for the data, add it to corresponding legend
          const auto& legendBoxVector = pad.GetLegendBoxes();
          if (legendBoxVector.size() && data->GetLegendLabel() && !data->GetLegendLabel()->empty()) {
            // by default place legend entries in first legend
            uint8_t legendID{1u};
            // explicit user choice overrides this
            if (data->GetLegendID()) legendID = *data->GetLegendID();

            if (legendID > 0u && legendID <= legendBoxVector.size()) {
              legendBoxVector[legendID - 1]->AddEntry(*data->GetLegendLabel(), (dataIndex - hasRefFunc));
            } else {
              WARNING("Invalid legend label ({}) specified for data {} in {}.", legendID, data->GetName(), data->GetDataSource());
            }
          }
          pad_ptr->Update();  // adds something to the list of primitives
        } else {
          delete data_ptr;
        }
        ++dataIndex;
        drawingOptions = "SAME ";  // next data should be drawn to same pad
      };

      optional<data_ptr_t> rawData;
      try {
        rawData = GetDataClone(dataBuffer.at(data->GetDataSource()).at(data->GetName() + data->GetDataInfo().GetNameSuffix()).get(), data->GetProjInfo());
      } catch (std::out_of_range&) {
        rawData = nullopt;
      }

      if (rawData) {
        std::visit(processData, *rawData);
      } else {
        fail = true;
      }
    }  // end data code

    if (fail) {
      return nullptr;
    }

    bool redrawAxes = (pad.GetRedrawAxes())
                        ? *pad.GetRedrawAxes()
                        : ((padDefaults.GetRedrawAxes()) ? *padDefaults.GetRedrawAxes() : false);
    if (redrawAxes && axisHist_ptr && !pad_ptr->GetView()) {
      // re-draw frame
      TLine line;
      pad_ptr->GetFrame()->Copy(line);
      double_t lm = pad_ptr->GetLeftMargin();
      double_t rm = 1. - pad_ptr->GetRightMargin();
      double_t tm = 1. - pad_ptr->GetTopMargin();
      double_t bm = pad_ptr->GetBottomMargin();
      line.DrawLineNDC(rm, bm, rm, tm);
      line.DrawLineNDC(lm, bm, lm, tm);
      line.DrawLineNDC(lm, tm, rm, tm);
      line.DrawLineNDC(lm, bm, rm, bm);
      // re-draw axes
      axisHist_ptr->Draw("SAME AXIS");
    }

    // now place legends, text-boxes and shapes
    uint8_t legendIndex{1u};
    for (const auto& box : pad.GetLegendBoxes()) {
      string legendName = "LegendBox_" + std::to_string(legendIndex);
      box->MergeLegendEntries();  // apply individual user settings on top of automatic entries
      // apply default text properties of pad to the box
      if (!box->GetTextFont() && textFont) box->SetTextFont(*textFont);
      if (!box->GetTextSize() && textSize) box->SetTextSize(*textSize);
      if (!box->GetTextColor() && textColor) box->SetTextColor(*textColor);
      if (!box->GetTextAlpha() && textAlpha) box->SetTextAlpha(*textAlpha);
      TPave* legend = GenerateBox(box, pad_ptr);
      if (legend) {
        legend->SetName(legendName.data());
        legend->Draw("SAME");
        ++legendIndex;
      } else {
        WARNING("Legend {} was not added since it is empty.", legendIndex);
      }
    }
    uint8_t textIndex{1u};
    for (const auto& box : pad.GetTextBoxes()) {
      string textName = "TextBox_" + std::to_string(textIndex);
      // apply default text properties of pad to the box
      if (!box->GetTextFont() && textFont) box->SetTextFont(*textFont);
      if (!box->GetTextSize() && textSize) box->SetTextSize(*textSize);
      if (!box->GetTextColor() && textColor) box->SetTextColor(*textColor);
      if (!box->GetTextAlpha() && textAlpha) box->SetTextAlpha(*textAlpha);
      TPave* text = GenerateBox(box, pad_ptr);
      if (text) {
        text->SetName(textName.data());
        text->Draw("SAME");
        ++textIndex;
      } else {
        WARNING("Text {} was not added since it is empty.", textIndex);
      }
    }

    if (pad.GetViewTheta()) {
      pad_ptr->SetTheta(*pad.GetViewTheta());
    }
    if (pad.GetViewPhi()) {
      pad_ptr->SetPhi(*pad.GetViewPhi());
    }
    pad_ptr->Modified();
    pad_ptr->Update();
  }

  CheckFontSizes(canvas_ptr->GetListOfPrimitives());
  if (mScale != 1.) {
    std::set<TObject*> done;
    ApplyScale(canvas_ptr.get(), done);
    // axes, hatched fill areas, the ends of error bars and the dashes of line styles are painted with the absolute sizes defined in gStyle
    sUnscaledStyle = StyleSizes{gStyle->GetLineWidth(), gStyle->GetHatchesLineWidth(), gStyle->GetEndErrorSize(), {}};
    gStyle->SetLineWidth(ScaleLineWidth(gStyle->GetLineWidth()));
    gStyle->SetHatchesLineWidth(ScaleLineWidth(static_cast<Width_t>(gStyle->GetHatchesLineWidth())));
    gStyle->SetEndErrorSize(gStyle->GetEndErrorSize() * mScale);
    // line styles are lists of dash and gap lengths (see TStyle::SetLineStyleString); styles 2-4 are fixed in the X11 backend and only scale in files
    for (int32_t style = 1; style < 30; ++style) {
      const string dashes = gStyle->GetLineStyleString(style);
      sUnscaledStyle->lineStyles.push_back(dashes);
      string scaledDashes;
      std::istringstream stream(dashes);
      for (int32_t length{}; stream >> length;) {
        scaledDashes += std::to_string(std::lround(length * mScale)) + " ";
      }
      if (!scaledDashes.empty()) gStyle->SetLineStyleString(style, scaledDashes.data());
    }
  }
  canvas_ptr->cd();
  canvas_ptr->Modified();
  canvas_ptr->Update();
  return canvas_ptr;
}

//**************************************************************************************************
/**
 * Scales a line width by the scale factor. ROOT line widths are integers, and graphs encode an
 * exclusion zone as 100 x zone size + line width (see TGraphPainter); only the line width is scaled.
 */
//**************************************************************************************************
Width_t PlotPainter::ScaleLineWidth(Width_t width) const
{
  const int32_t sign = (width < 0) ? -1 : 1;
  const int32_t exclusionZone = std::abs(width) / 100 * 100;
  const int32_t lineWidth = std::abs(width) % 100;
  if (lineWidth == 0) return width;
  const int32_t scaled = std::clamp(static_cast<int32_t>(std::lround(lineWidth * mScale)), 1, 99);
  return static_cast<Width_t>(sign * (exclusionZone + scaled));
}

//**************************************************************************************************
/**
 * Scales all absolute sizes of a drawn object (and the objects it contains) by the scale factor:
 * line widths, marker sizes, border sizes and text sizes of pixel fonts (precision 3).
 * The draw option of the object is needed for ROOT features that use these sizes for other purposes.
 * All other sizes are defined relative to the pad and therefore already scale with the canvas.
 */
//**************************************************************************************************
void PlotPainter::ApplyScale(TObject* obj, std::set<TObject*>& done, const std::string& drawOption)
{
  if (!obj || !done.insert(obj).second) return;
  auto applyToList = [&](TCollection* list) {
    if (!list) return;
    TIter next(list);
    while (TObject* entry = next()) {
      ApplyScale(entry, done, next.GetOption());
    }
  };
  // ROOT moves filled histograms away from the frame by half of the frame line width (integer division) and then also draws their outer
  // vertical edges (see TGraphPainter::PaintGrapHist): a frame line of width 1 has to stay 1, otherwise these edges would appear
  auto scaleFrameLineWidth = [&](Width_t width) { return (width <= 1) ? width : ScaleLineWidth(width); };
  if (auto frame = dynamic_cast<TFrame*>(obj)) {
    frame->SetLineWidth(scaleFrameLineWidth(frame->GetLineWidth()));
  } else if (auto line = dynamic_cast<TAttLine*>(obj)) {
    line->SetLineWidth(ScaleLineWidth(line->GetLineWidth()));
  }
  // histograms drawn with option TEXT use their marker size as relative text size of the bin contents (see THistPainter::PaintText)
  const bool isHistText = dynamic_cast<TH1*>(obj) && TString(drawOption).Contains("TEXT", TString::kIgnoreCase);
  auto marker = dynamic_cast<TAttMarker*>(obj);
  if (marker && !isHistText) marker->SetMarkerSize(marker->GetMarkerSize() * mScale);
  auto text = dynamic_cast<TAttText*>(obj);
  if (text && text->GetTextFont() % 10 == 3) text->SetTextSize(text->GetTextSize() * mScale);
  if (auto pave = dynamic_cast<TPave*>(obj)) pave->SetBorderSize(static_cast<Int_t>(std::lround(pave->GetBorderSize() * mScale)));
  if (auto pad = dynamic_cast<TPad*>(obj)) {
    pad->SetFrameLineWidth(scaleFrameLineWidth(pad->GetFrameLineWidth()));
    pad->SetBorderSize(static_cast<Short_t>(std::lround(pad->GetBorderSize() * mScale)));
    applyToList(pad->GetListOfPrimitives());
  }
  if (auto hist = dynamic_cast<TH1*>(obj)) {
    for (TAxis* axis : {hist->GetXaxis(), hist->GetYaxis(), hist->GetZaxis()}) {
      if (axis->GetLabelFont() % 10 == 3) axis->SetLabelSize(axis->GetLabelSize() * mScale);
      if (axis->GetTitleFont() % 10 == 3) axis->SetTitleSize(axis->GetTitleSize() * mScale);
    }
    applyToList(hist->GetListOfFunctions());
  }
  if (auto graph = dynamic_cast<TGraph*>(obj)) applyToList(graph->GetListOfFunctions());
  if (auto multiGraph = dynamic_cast<TMultiGraph*>(obj)) applyToList(multiGraph->GetListOfGraphs());
  if (auto stack = dynamic_cast<THStack*>(obj)) applyToList(stack->GetHists());
  if (auto legend = dynamic_cast<TLegend*>(obj)) applyToList(legend->GetListOfPrimitives());
  if (auto paveText = dynamic_cast<TPaveText*>(obj)) applyToList(paveText->GetListOfLines());
}

//**************************************************************************************************
/**
 * Returns a copy of the frame histogram with the given axis extended to newMin and / or newMax (bin contents and errors copied).
 * The lengths of the other axes and the attributes are kept; the result replaces the frame in the pad and has to be drawn by the caller.
 */
//**************************************************************************************************
TH1* PlotPainter::ExtendFrameAxis(const TH1* frame, char axisLabel, optional<double_t> newMin, optional<double_t> newMax)
{
  const int32_t dim = frame->GetDimension();
  const int32_t extendedDim = (axisLabel == 'X') ? 0 : 1;
  const int32_t shift = (newMin) ? 1 : 0;  // bins move up by one if a bin is added below
  const array<const TAxis*, 3> axes = {frame->GetXaxis(), frame->GetYaxis(), frame->GetZaxis()};
  array<vector<double_t>, 3> edges;
  for (int32_t i = 0; i < dim; ++i) {
    if (i == extendedDim && newMin) edges[i].push_back(*newMin);
    for (int32_t bin = 1; bin <= axes[i]->GetNbins() + 1; ++bin)
      edges[i].push_back(axes[i]->GetBinLowEdge(bin));
    if (i == extendedDim && newMax) edges[i].push_back(*newMax);
  }
  TH1* extended{nullptr};
  if (dim == 1) {
    extended = new TH1D(frame->GetName(), frame->GetTitle(), edges[0].size() - 1, edges[0].data());
  } else if (dim == 2) {
    extended = new TH2D(frame->GetName(), frame->GetTitle(), edges[0].size() - 1, edges[0].data(), edges[1].size() - 1, edges[1].data());
  } else {
    extended = new TH3D(frame->GetName(), frame->GetTitle(), edges[0].size() - 1, edges[0].data(), edges[1].size() - 1, edges[1].data(), edges[2].size() - 1, edges[2].data());
  }
  extended->SetDirectory(nullptr);
  extended->SetBit(kCanDelete);
  const int32_t shiftX = (extendedDim == 0) ? shift : 0;
  const int32_t shiftY = (extendedDim == 1) ? shift : 0;
  for (int32_t binZ = 1; binZ <= axes[2]->GetNbins(); ++binZ) {
    for (int32_t binY = 1; binY <= axes[1]->GetNbins(); ++binY) {
      for (int32_t binX = 1; binX <= axes[0]->GetNbins(); ++binX) {
        const int32_t bin = frame->GetBin(binX, binY, binZ);
        const int32_t newBin = extended->GetBin(binX + shiftX, binY + shiftY, binZ);
        extended->SetBinContent(newBin, frame->GetBinContent(bin));
        extended->SetBinError(newBin, frame->GetBinError(bin));
      }
    }
  }
  extended->SetEntries(frame->GetEntries());
  extended->SetMinimum(frame->GetMinimumStored());
  extended->SetMaximum(frame->GetMaximumStored());
  frame->TAttLine::Copy(*extended);
  frame->TAttFill::Copy(*extended);
  frame->TAttMarker::Copy(*extended);
  // the axes are copied completely (attributes, labels, time format, bits, a selected range) and the extended one gets its new binning
  const array<TAxis*, 3> newAxes = {extended->GetXaxis(), extended->GetYaxis(), extended->GetZaxis()};
  for (int32_t i = 0; i < 3; ++i) {
    const bool hadRange = axes[i]->TestBit(TAxis::kAxisRange);
    axes[i]->Copy(*newAxes[i]);
    newAxes[i]->SetParent(extended);
    if (i != extendedDim) continue;
    newAxes[i]->Set(edges[i].size() - 1, edges[i].data());
    if (hadRange) {
      newAxes[i]->SetRange(axes[i]->GetFirst() + shift, axes[i]->GetLast() + shift);
    } else {
      newAxes[i]->SetRange(0, 0);  // full new axis
    }
    if (shift && newAxes[i]->GetLabels()) {
      TIter next(newAxes[i]->GetLabels());
      while (TObject* label = next())
        label->SetUniqueID(label->GetUniqueID() + shift);
    }
  }
  return extended;
}

//**************************************************************************************************
/**
 * Function to warn user if incompatible text font and size were used.
 */
//**************************************************************************************************
bool PlotPainter::CheckFontSizes(TList* list)
{
  auto checkCompat = [](int16_t textFont, float_t textSize, TObject* obj) {
    if (textSize) {
      if (textFont % 10 <= 2) {
        if (textSize >= 1.f) {
          WARNING("Though text font {} implies a relative text size it is set to {} for {}.", textFont, textSize, obj->GetName());
          return false;
        }
      } else {
        if (textSize <= 1.f) {
          WARNING("Though text font {} implies a text size in pixel it is set to {} for {}.", textFont, textSize, obj->GetName());
          return false;
        }
      }
    }
    return true;
  };

  if (!list) return false;
  TIter next(list);
  TObject* obj = nullptr;
  bool valid = true;
  while ((obj = next())) {
    if (auto pad = dynamic_cast<TPad*>(obj)) {
      valid = valid && CheckFontSizes(pad->GetListOfPrimitives());
    }
    if (auto leg = dynamic_cast<TLegend*>(obj)) {
      valid = valid && CheckFontSizes(leg->GetListOfPrimitives());
    }
    if (auto txt = dynamic_cast<TAttText*>(obj)) {
      valid = valid && checkCompat(txt->GetTextFont(), txt->GetTextSize(), obj);
    }
    if (auto h = dynamic_cast<TH1*>(obj)) {
      TString name = h->GetName();
      if (name.BeginsWith("axis_hist")) {
        if (auto ax = h->GetXaxis()) {
          valid = valid && checkCompat(ax->GetLabelFont(), ax->GetLabelSize(), ax);
          valid = valid && checkCompat(ax->GetTitleFont(), ax->GetTitleSize(), ax);
        }
        if (auto ax = h->GetYaxis()) {
          valid = valid && checkCompat(ax->GetLabelFont(), ax->GetLabelSize(), ax);
          valid = valid && checkCompat(ax->GetTitleFont(), ax->GetTitleSize(), ax);
        }
        if (auto ax = h->GetZaxis()) {
          valid = valid && checkCompat(ax->GetLabelFont(), ax->GetLabelSize(), ax);
          valid = valid && checkCompat(ax->GetTitleFont(), ax->GetTitleSize(), ax);
        }
      }
    }
    if (!valid) break;
  }
  return valid;
}

//**************************************************************************************************
/**
 * Function to generate a legend or text box.
 */
//**************************************************************************************************
TPave* PlotPainter::GenerateBox(variant<shared_ptr<Plot::Pad::LegendBox>, shared_ptr<Plot::Pad::TextBox>> boxVariant, TPad* pad)
{
  TPave* returnBox{nullptr};

  auto processBox = [&](auto&& box) {
    using BoxType = std::decay_t<decltype(*box)>;
    constexpr bool isLegend = std::is_same_v<BoxType, Plot::Pad::LegendBox>;

    auto textColor{box->GetTextColor()};
    auto textAlpha{box->GetTextAlpha()};
    auto textFont{box->GetTextFont()};
    auto textSize{box->GetTextSize()};

    auto borderColor{box->GetBorderColor()};
    auto borderAlpha{box->GetBorderAlpha()};
    auto borderStyle{box->GetBorderStyle()};
    auto borderWidth{box->GetBorderWidth()};

    auto fillColor{box->GetFillColor()};
    auto fillStyle{box->GetFillStyle()};
    auto fillAlpha{box->GetFillAlpha()};

    vector<string> lines;
    if constexpr (isLegend) {
      std::for_each(box->GetEntries().begin(), box->GetEntries().end(),
                    [&lines](const auto& entry) {
                      (entry.GetLabel()) ? lines.push_back(*entry.GetLabel()) : lines.push_back("");
                    });
    } else {
      // split text string to vector
      string delimiter{" // "};
      string text{box->GetText()};
      size_t pos{};
      size_t last{};
      while ((pos = text.find(delimiter, last)) != string::npos) {
        lines.push_back(text.substr(last, pos - last));
        last = pos + delimiter.length();
      }
      lines.push_back(text.substr(last));
    }

    float_t text_size = textSize.value_or(24.f);
    int16_t text_font = textFont.value_or(43);
    uint8_t nColumns{1u};
    if constexpr (isLegend) {
      if (box->GetNumColumns()) nColumns = *box->GetNumColumns();
    }
    uint16_t nLines = lines.size();
    if (nLines < 1) return;

    int32_t padWidthPixel = pad->XtoPixel(pad->GetX2());
    int32_t padHeightPixel = pad->YtoPixel(pad->GetY1());

    // determine max width and height of legend entries
    uint8_t iColumn{};
    double_t contentWidthPixel{};
    double_t titleWidthPixel{};
    vector<uint32_t> contentWidthPixelPerColumn(nColumns, 1);

    double_t lineHeightPixel{text_size};
    if (text_font % 10 <= 2) lineHeightPixel = GetTextSizePixel(text_size);

    uint8_t lineID{};
    for (auto& line : lines) {
      auto line_text_font = text_font;
      auto line_text_size = text_size;
      if constexpr (isLegend) {
        const auto& entry = box->GetEntries()[lineID];
        if (entry.GetTextFont()) line_text_font = *entry.GetTextFont();
        if (entry.GetTextSize()) line_text_size = *entry.GetTextSize();
        if (entry.GetRefDataID()) {
          TObject* data_ptr = nullptr;
          TIter next(pad->GetListOfPrimitives());
          while ((data_ptr = next())) {
            if (TString(data_ptr->GetName()).BeginsWith(std::to_string(*entry.GetRefDataID()).data())) {
              break;
            }
          }
          if (!data_ptr) {
            WARNING("Object belonging to legend entry {} not found.", line);
          } else {
            ReplacePlaceholders(line, static_cast<TNamed*>(data_ptr));
          }
        }
      }

      // determine width and height of line to find max width and height (per column)
      TLatex textLine(0, 0, line.data());
      textLine.SetTextFont(line_text_font);
      textLine.SetTextSize(line_text_size);
      auto [width, height] = GetTextDimensions(textLine, pad);
      if (height > lineHeightPixel) lineHeightPixel = height;
      if (width > contentWidthPixelPerColumn[iColumn]) contentWidthPixelPerColumn[iColumn] = width;
      ++iColumn;
      iColumn %= nColumns;
      ++lineID;
    }
    for (const auto& length : contentWidthPixelPerColumn) {
      contentWidthPixel += length;
    }
    uint32_t symbolColWidthPixel{0};
    if constexpr (isLegend) {
      string markerDummyString = "-+-";  // defines width of marker
      TLatex markerDummy(0, 0, markerDummyString.data());
      markerDummy.SetTextFont(text_font);
      markerDummy.SetTextSize(text_size);
      auto [w, h] = GetTextDimensions(markerDummy, pad);
      symbolColWidthPixel = box->GetSymbolColScale().value_or(1.) * w;

      if (const auto& title = box->GetTitle()) {
        TLatex textLine(0, 0, (*title).data());
        textLine.SetTextFont(text_font);
        textLine.SetTextSize(text_size);
        auto [width, height] = GetTextDimensions(textLine, pad);
        titleWidthPixel = width;
        if (height > lineHeightPixel) lineHeightPixel = height;
      }
    }

    double_t contentWidthNDC = (double_t)contentWidthPixel / padWidthPixel;
    double_t lineHeightNDC = (double_t)lineHeightPixel / padHeightPixel;
    double_t symbolColWidthNDC = (double_t)symbolColWidthPixel / padWidthPixel;
    double_t titleWidthNDC = (double_t)titleWidthPixel / padWidthPixel;
    double_t borderWidthNDC = mScale * box->GetBorderWidth().value_or(0.) / padWidthPixel;
    // double_t borderHeightNDC = box->GetBorderWidth().value_or(0.) / padHeightPixel;

    double_t totalWidthNDC{};
    double_t totalHeightNDC{};

    float_t marginNDC = box->GetMargin().value_or(0.01);
    double_t lineSpacing = box->GetLineSpacing().value_or(0.3);
    double_t totalMarginWidthNDC = (0.5 * borderWidthNDC + marginNDC + symbolColWidthNDC);

    if constexpr (isLegend) {
      totalWidthNDC = borderWidthNDC + 2 * marginNDC + symbolColWidthNDC + contentWidthNDC;
      totalHeightNDC = (1 + lineSpacing) * lineHeightNDC * std::ceil(static_cast<double_t>(nLines) / nColumns);
      if (box->GetTitle()) {
        if (titleWidthNDC > totalWidthNDC) {
          totalWidthNDC = borderWidthNDC + titleWidthNDC + 0.1 * totalMarginWidthNDC;  // root places header at margin/10
        }
        totalHeightNDC += (1 + lineSpacing) * lineHeightNDC;
      }
    } else {
      totalWidthNDC = borderWidthNDC + 2 * marginNDC + contentWidthNDC;
      totalHeightNDC = (1 + lineSpacing) * nLines * lineHeightNDC;
    }
    if (box->GetWidth()) {
      totalWidthNDC = *box->GetWidth();
    }
    if (box->GetHeight()) {
      totalHeightNDC = *box->GetHeight();
    }

    double_t relMarginWidth = totalMarginWidthNDC / totalWidthNDC;
    if (isLegend && nColumns > 1 && !box->GetWidth()) {
      // TLegend splits its width into equal columns and reserves the margin (symbol space) in each of them
      const double_t maxContentWidthNDC = static_cast<double_t>(*std::max_element(contentWidthPixelPerColumn.begin(), contentWidthPixelPerColumn.end())) / padWidthPixel;
      const double_t columnWidthNDC = totalMarginWidthNDC + maxContentWidthNDC + marginNDC;
      totalWidthNDC = std::max(borderWidthNDC + nColumns * columnWidthNDC, totalWidthNDC);
      relMarginWidth = totalMarginWidthNDC / (totalWidthNDC / nColumns);
    }

    double_t upperLeftX{box->GetXPosition()};
    double_t upperLeftY{box->GetYPosition()};

    if (box->IsAutoPlacement()) {
      pad->cd();
      pad->Update();
      double_t lowerLeftX{};
      double_t lowerLeftY{};
      bool foundPosition = false;

      // area available for the box: the frame without the ticks of the axes and a small gap (in NDC)
      constexpr double_t kDistanceToFrame{1.4};  // in units of the tick length
      TH1* axisHist{};
      for (auto* obj : *pad->GetListOfPrimitives()) {
        if (obj->InheritsFrom(TH1::Class()) && TString(obj->GetName()).BeginsWith("axis_hist")) {
          axisHist = static_cast<TH1*>(obj);
          break;
        }
      }
      const double_t tickLengthX = (axisHist) ? axisHist->GetXaxis()->GetTickLength() : gStyle->GetTickLength("X");  // ticks on the x axis (vertical)
      const double_t tickLengthY = (axisHist) ? axisHist->GetYaxis()->GetTickLength() : gStyle->GetTickLength("Y");  // ticks on the y axis (horizontal)
      const double_t frameX1 = (pad->GetUxmin() - pad->GetX1()) / (pad->GetX2() - pad->GetX1());
      const double_t frameX2 = (pad->GetUxmax() - pad->GetX1()) / (pad->GetX2() - pad->GetX1());
      const double_t frameY1 = (pad->GetUymin() - pad->GetY1()) / (pad->GetY2() - pad->GetY1());
      const double_t frameY2 = (pad->GetUymax() - pad->GetY1()) / (pad->GetY2() - pad->GetY1());
      const double_t distanceX = kDistanceToFrame * tickLengthY * (frameX2 - frameX1);
      const double_t distanceY = kDistanceToFrame * tickLengthX * (frameY2 - frameY1);
      const std::array<double_t, 4> freeArea{frameX1 + distanceX, frameY1 + distanceY, frameX2 - distanceX, frameY2 - distanceY};

      // the border is drawn centered on the edges of the box, so half of it sticks out on each side
      const double_t borderX = mScale * box->GetBorderWidth().value_or(0.) / padWidthPixel;
      const double_t borderY = mScale * box->GetBorderWidth().value_or(0.) / padHeightPixel;
      const double_t outerWidthNDC = totalWidthNDC + borderX;
      const double_t outerHeightNDC = totalHeightNDC + borderY;

      // find box position that does not collide with any of the drawn objects
      const double_t freeWidthNDC = freeArea[2] - freeArea[0];
      const double_t freeHeightNDC = freeArea[3] - freeArea[1];
      const bool fitsIntoFrame = (outerWidthNDC <= freeWidthNDC && outerHeightNDC <= freeHeightNDC);
      if (fitsIntoFrame) {
        foundPosition = FindFreeSpace(pad, freeArea, outerWidthNDC, outerHeightNDC, *box->GetPlacement(), lowerLeftX, lowerLeftY);
      }
      if (foundPosition) {
        upperLeftX = lowerLeftX + 0.5 * borderX;
        upperLeftY = lowerLeftY + 0.5 * borderY + totalHeightNDC;
      } else {
        if (fitsIntoFrame) {
          WARNING("Could not find enough space to place the {} properly.", (isLegend) ? "legend" : "text");
        } else {
          WARNING("The {} ({:.2f} x {:.2f}) is larger than the space inside the frame ({:.2f} x {:.2f}, in units of the pad size): use fewer lines, more columns or a smaller text size.",
                  (isLegend) ? "legend" : "text", outerWidthNDC, outerHeightNDC, freeWidthNDC, freeHeightNDC);
        }
        // just place it in the top left corner of the frame
        upperLeftX = freeArea[0] + 0.5 * borderX;
        upperLeftY = freeArea[3] - 0.5 * borderY;
      }
    } else if (box->IsUserCoordinates()) {
      // convert user coordinates to NDC
      pad->Update();
      upperLeftX = (upperLeftX - pad->GetX1()) / (pad->GetX2() - pad->GetX1());
      upperLeftY = (upperLeftY - pad->GetY1()) / (pad->GetY2() - pad->GetY1());
    }

    if constexpr (isLegend) {
      TLegend* legend = new TLegend(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "", "NDC NB");
      legend->SetMargin(relMarginWidth);
      legend->SetTextAlign(kHAlignLeft + kVAlignCenter);
      legend->SetNColumns(nColumns);
      legend->SetTextFont(text_font);
      legend->SetTextSize(text_size);
      if (textColor) legend->SetTextColor(*textColor);
      if (textAlpha) legend->SetTextColor(TColor::GetColorTransparent(legend->GetTextColor(), *textAlpha));
      if (textSize) legend->SetTextSize(*textSize);
      if (textFont) legend->SetTextFont(*textFont);

      if (auto title = box->GetTitle()) {
        string center = (nColumns > 1) ? "c" : "";
        legend->SetHeader((*title).data(), center.data());
      }

      int32_t i = 0;
      for (const auto& entry : box->GetEntries()) {
        string label = lines[i];

        // user-defined draw style for single entry or all entries
        string drawStyle = entry.GetDrawStyle() ? *entry.GetDrawStyle() : ((box->GetDefaultDrawStyle()) ? *box->GetDefaultDrawStyle() : "");

        TLegendEntry* curEntry = nullptr;
        TAttMarker markerAttr;
        TAttLine lineAttr;
        TAttFill fillAttr;
        if (entry.GetRefDataID()) {
          TObject* data_ptr = nullptr;
          TIter next(pad->GetListOfPrimitives());
          while ((data_ptr = next())) {
            if (TString(data_ptr->GetName()).BeginsWith(std::to_string(*entry.GetRefDataID()).data())) {
              break;
            }
          }
          if (!data_ptr) {
            // advance i so lines[] stays aligned with box->GetEntries() for subsequent iterations
            ++i;
            continue;
          }

          if (drawStyle.empty()) {
            drawStyle = "EP";

            string drawingOption = data_ptr->GetDrawOption();
            std::for_each(drawingOption.begin(), drawingOption.end(),
                          [](char& c) { c = ::toupper(c); });

            if ((data_ptr->InheritsFrom(TF1::Class())) || str_contains(drawingOption, "C") || str_contains(drawingOption, "L") || str_contains(drawingOption, "HIST")) {
              drawStyle = "L";
            } else if (data_ptr->InheritsFrom(TH1::Class()) && (str_contains(drawingOption, "HIST") || str_contains(drawingOption, "B")) && static_cast<TH1*>(data_ptr)->GetFillStyle() != 0) {
              drawStyle = "F";
            }
          }
          curEntry = legend->AddEntry(data_ptr, label.data(), drawStyle.data());
          curEntry->SetObject(static_cast<TObject*>(nullptr));
          if (auto ptr = dynamic_cast<TAttMarker*>(data_ptr)) ptr->Copy(markerAttr);
          if (auto ptr = dynamic_cast<TAttLine*>(data_ptr)) ptr->Copy(lineAttr);
          if (auto ptr = dynamic_cast<TAttFill*>(data_ptr)) ptr->Copy(fillAttr);
        } else {
          curEntry = legend->AddEntry(static_cast<TObject*>(nullptr), label.data(), drawStyle.data());
        }

        // override data attributes with default values if requested
        curEntry->SetMarkerColor(markerAttr.GetMarkerColor());
        if (box->GetDefaultMarkerColor()) {
          curEntry->SetMarkerColor(*box->GetDefaultMarkerColor());
        }
        if (box->GetDefaultMarkerAlpha()) {
          curEntry->SetMarkerColor(TColor::GetColorTransparent(curEntry->GetMarkerColor(), *box->GetDefaultMarkerAlpha()));
        }
        curEntry->SetMarkerStyle(markerAttr.GetMarkerStyle());
        if (box->GetDefaultMarkerStyle()) {
          curEntry->SetMarkerStyle(*box->GetDefaultMarkerStyle());
        }
        curEntry->SetMarkerSize(markerAttr.GetMarkerSize());
        if (box->GetDefaultMarkerSize()) {
          curEntry->SetMarkerSize(*box->GetDefaultMarkerSize());
        }
        curEntry->SetLineColor(lineAttr.GetLineColor());
        if (box->GetDefaultLineColor()) {
          curEntry->SetLineColor(*box->GetDefaultLineColor());
        }
        if (box->GetDefaultLineAlpha()) {
          curEntry->SetLineColor(TColor::GetColorTransparent(curEntry->GetLineColor(), *box->GetDefaultLineAlpha()));
        }
        curEntry->SetLineStyle(lineAttr.GetLineStyle());
        if (box->GetDefaultLineStyle()) {
          curEntry->SetLineStyle(*box->GetDefaultLineStyle());
        }
        curEntry->SetLineWidth(lineAttr.GetLineWidth());
        if (box->GetDefaultLineWidth()) {
          curEntry->SetLineWidth(*box->GetDefaultLineWidth());
        }
        curEntry->SetFillColor(fillAttr.GetFillColor());
        if (box->GetDefaultFillColor()) {
          curEntry->SetFillColor(*box->GetDefaultFillColor());
        }
        if (box->GetDefaultFillAlpha()) {
          curEntry->SetFillColor(TColor::GetColorTransparent(curEntry->GetFillColor(), *box->GetDefaultFillAlpha()));
        }
        curEntry->SetFillStyle(fillAttr.GetFillStyle());
        if (box->GetDefaultFillStyle()) {
          curEntry->SetFillStyle(*box->GetDefaultFillStyle());
        }

        if (entry.GetMarkerColor()) curEntry->SetMarkerColor(*entry.GetMarkerColor());
        if (entry.GetMarkerAlpha()) curEntry->SetMarkerColor(TColor::GetColorTransparent(curEntry->GetMarkerColor(), *entry.GetMarkerAlpha()));
        if (entry.GetMarkerStyle()) curEntry->SetMarkerStyle(*entry.GetMarkerStyle());
        if (entry.GetMarkerSize()) curEntry->SetMarkerSize(*entry.GetMarkerSize());

        if (entry.GetLineColor()) curEntry->SetLineColor(*entry.GetLineColor());
        if (entry.GetLineAlpha()) curEntry->SetLineColor(TColor::GetColorTransparent(curEntry->GetLineColor(), *entry.GetLineAlpha()));
        if (entry.GetLineStyle()) curEntry->SetLineStyle(*entry.GetLineStyle());
        if (entry.GetLineWidth()) curEntry->SetLineWidth(*entry.GetLineWidth());

        if (entry.GetFillColor()) curEntry->SetFillColor(*entry.GetFillColor());
        if (entry.GetFillAlpha()) curEntry->SetFillColor(TColor::GetColorTransparent(curEntry->GetFillColor(), *entry.GetFillAlpha()));
        if (entry.GetFillStyle()) curEntry->SetFillStyle(*entry.GetFillStyle());

        if (entry.GetTextColor()) curEntry->SetTextColor(*entry.GetTextColor());
        if (entry.GetTextAlpha()) curEntry->SetTextColor(TColor::GetColorTransparent(curEntry->GetTextColor(), *entry.GetTextAlpha()));
        if (entry.GetTextFont()) curEntry->SetTextFont(*entry.GetTextFont());
        if (entry.GetTextSize()) curEntry->SetTextSize(*entry.GetTextSize());
        ++i;
      }
      returnBox = legend;
    } else {
      TPaveText* paveText = new TPaveText(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "NDC NB");
      paveText->SetMargin(relMarginWidth);
      paveText->SetTextAlign(box->GetTextAlign().value_or(kHAlignLeft + kVAlignCenter));
      paveText->SetBorderSize(1);
      paveText->SetTextFont(text_font);
      paveText->SetTextSize(text_size);
      if (textColor) paveText->SetTextColor(*textColor);
      if (textAlpha) paveText->SetTextColor(TColor::GetColorTransparent(paveText->GetTextColor(), *textAlpha));

      for (const auto& line : lines) {
        TText* text = paveText->AddText(line.data());
        text->SetTextFont(text_font);
        text->SetTextSize(text_size);
      }
      returnBox = paveText;
    }

    if (returnBox) {
      returnBox->SetLineWidth(0);
      returnBox->SetFillStyle(0);
      if (borderStyle) returnBox->SetLineStyle(*borderStyle);
      if (borderColor) returnBox->SetLineColor(*borderColor);
      if (borderAlpha) returnBox->SetLineColor(TColor::GetColorTransparent(returnBox->GetLineColor(), *borderAlpha));
      if (borderWidth) returnBox->SetLineWidth(*borderWidth);
      if (fillStyle) returnBox->SetFillStyle(*fillStyle);
      if (fillColor) returnBox->SetFillColor(*fillColor);
      if (fillAlpha) returnBox->SetFillColor(TColor::GetColorTransparent(returnBox->GetFillColor(), *fillAlpha));
    }
  };
  std::visit(processBox, boxVariant);
  return returnBox;
}

//**************************************************************************************************
/**
 * Find a position for a box of the given size (in NDC) within the free area (x1, y1, x2, y2 in NDC) that does not overlap with anything drawn.
 * Replaces TPad::PlaceBox, which considers all bins of a histogram (also those outside of the visible range),
 * does not clip to the pad (absurd values like huge error bars then take forever and block the whole pad)
 * and samples graphs only coarsely.
 * The box is placed at the free position closest to the given corner of the frame
 * or (for best_corner) closest to the corner where it fits best.
 * Returns the lower left corner of the box in NDC.
 */
//**************************************************************************************************
bool PlotPainter::FindFreeSpace(TPad* pad, const std::array<double_t, 4>& freeArea, double_t width, double_t height, box_placement_t placement, double_t& lowerLeftX, double_t& lowerLeftY)
{
  const double_t cellSize = 10. * mScale;  // grid cell size in pixels (as in TPad::PlaceBox), scaled so that boxes are placed the same way at any scale
  const int32_t nx = static_cast<int32_t>(pad->GetWw() / cellSize);
  const int32_t ny = static_cast<int32_t>(pad->GetWh() / cellSize);
  if (nx <= 0 || ny <= 0) return false;
  vector<uint8_t> blocked(static_cast<size_t>(nx) * ny, 0u);  // cells occupied by drawn objects

  // conversion from pad coordinates to (fractional) grid cells
  const double_t cellsPerX = nx / (pad->GetX2() - pad->GetX1());
  const double_t cellsPerY = ny / (pad->GetY2() - pad->GetY1());
  auto cellX = [&](double_t x) { return (x - pad->GetX1()) * cellsPerX; };
  auto cellY = [&](double_t y) { return (y - pad->GetY1()) * cellsPerY; };
  // conversion from user coordinates to pad coordinates (on a log axis non-positive values are moved to the pad edge)
  auto padX = [&](double_t x) { return (!pad->GetLogx()) ? x : ((x > 0.) ? std::log10(x) : pad->GetX1()); };
  auto padY = [&](double_t y) { return (!pad->GetLogy()) ? y : ((y > 0.) ? std::log10(y) : pad->GetY1()); };

  auto setRect = [&](double_t x1, double_t y1, double_t x2, double_t y2, uint8_t value) {  // in cells, inclusive
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) return;
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x2 < 0. || y2 < 0. || x1 >= nx || y1 >= ny) return;  // completely outside of the grid
    // clamp to the grid before casting (values far outside of the pad do not fit into an integer)
    const int32_t i1 = static_cast<int32_t>(std::floor(std::max(x1, 0.)));
    const int32_t i2 = static_cast<int32_t>(std::floor(std::min(x2, nx - 1.)));
    const int32_t j1 = static_cast<int32_t>(std::floor(std::max(y1, 0.)));
    const int32_t j2 = static_cast<int32_t>(std::floor(std::min(y2, ny - 1.)));
    for (int32_t i = i1; i <= i2; ++i) {
      for (int32_t j = j1; j <= j2; ++j) {
        blocked[i + static_cast<size_t>(j) * nx] = value;
      }
    }
  };
  auto markPoint = [&](double_t x, double_t y) { setRect(cellX(x), cellY(y), cellX(x), cellY(y), 1u); };
  auto markLine = [&](double_t x1, double_t y1, double_t x2, double_t y2) {  // in pad coordinates
    double_t cx1 = cellX(x1), cy1 = cellY(y1), cx2 = cellX(x2), cy2 = cellY(y2);
    if (!std::isfinite(cx1) || !std::isfinite(cy1) || !std::isfinite(cx2) || !std::isfinite(cy2)) return;
    // clip the line to the grid (Liang-Barsky) so lines far outside of the pad cost nothing
    double_t t0 = 0., t1 = 1.;
    const double_t dx = cx2 - cx1, dy = cy2 - cy1;
    for (auto [p, q] : {std::pair{-dx, cx1}, std::pair{dx, nx - cx1}, std::pair{-dy, cy1}, std::pair{dy, ny - cy1}}) {
      if (p == 0.) {
        if (q < 0.) return;
      } else {
        const double_t t = q / p;
        if (p < 0.) {
          t0 = std::max(t0, t);
        } else {
          t1 = std::min(t1, t);
        }
      }
    }
    if (t0 > t1) return;
    const int32_t nSteps = static_cast<int32_t>(std::ceil(2. * std::max(std::abs(dx), std::abs(dy)) * (t1 - t0))) + 1;
    for (int32_t step = 0; step <= nSteps; ++step) {
      const double_t t = t0 + (t1 - t0) * step / nSteps;
      setRect(cx1 + t * dx, cy1 + t * dy, cx1 + t * dx, cy1 + t * dy, 1u);
    }
  };

  auto markHist = [&](TH1* hist) {
    if (hist->GetDimension() != 1) return;
    TString option = hist->GetDrawOption();
    option.ToLower();
    const bool hasErrors = !option.Contains("hist") && option.Contains("e");
    const bool hasErrorArea = hasErrors && (option.Contains("e2") || option.Contains("e3") || option.Contains("e4") || option.Contains("e5"));
    const bool showEmpty = option.Contains("e0");
    TAxis* axis = hist->GetXaxis();
    for (int32_t bin = axis->GetFirst(); bin <= axis->GetLast(); ++bin) {  // only the visible range
      const double_t content = hist->GetBinContent(bin);
      if (hasErrors && !showEmpty && content == 0.) continue;  // not drawn by ROOT
      const double_t xLow = padX(axis->GetBinLowEdge(bin));
      const double_t xUp = padX(axis->GetBinUpEdge(bin));
      const double_t xCenter = padX(axis->GetBinCenter(bin));
      const double_t y = padY(content);
      markLine(xLow, y, xUp, y);
      markPoint(xCenter, y);
      if (hasErrorArea) {
        setRect(cellX(xLow), cellY(padY(content - hist->GetBinErrorLow(bin))), cellX(xUp), cellY(padY(content + hist->GetBinErrorUp(bin))), 1u);
      } else if (hasErrors) {
        markLine(xCenter, padY(content - hist->GetBinErrorLow(bin)), xCenter, padY(content + hist->GetBinErrorUp(bin)));
      } else if (bin < axis->GetLast()) {
        markLine(xUp, y, xUp, padY(hist->GetBinContent(bin + 1)));  // step to the next bin
      }
    }
  };
  auto markGraph = [&](TGraph* graph) {
    TString option = graph->GetDrawOption();
    option.ToLower();
    const bool hasErrorBoxes = option.Contains("2") || option.Contains("5");
    const bool hasErrorBand = option.Contains("3") || option.Contains("4");
    const int32_t n = graph->GetN();
    for (int32_t i = 0; i < n; ++i) {
      const double_t xUser = graph->GetPointX(i), yUser = graph->GetPointY(i);
      const double_t x = padX(xUser), y = padY(yUser);
      markPoint(x, y);
      if (i > 0) markLine(padX(graph->GetPointX(i - 1)), padY(graph->GetPointY(i - 1)), x, y);
      const double_t exLow = graph->GetErrorXlow(i), exHigh = graph->GetErrorXhigh(i);
      const double_t eyLow = graph->GetErrorYlow(i), eyHigh = graph->GetErrorYhigh(i);
      if (hasErrorBoxes) {
        setRect(cellX(padX(xUser - exLow)), cellY(padY(yUser - eyLow)), cellX(padX(xUser + exHigh)), cellY(padY(yUser + eyHigh)), 1u);
        continue;
      }
      if (exLow > 0. || exHigh > 0.) markLine(padX(xUser - exLow), y, padX(xUser + exHigh), y);
      if (eyLow > 0. || eyHigh > 0.) markLine(x, padY(yUser - eyLow), x, padY(yUser + eyHigh));
      if (hasErrorBand && i > 0) {
        // area between the lower and upper edges of the band from the previous point to this one (column by column)
        const double_t xPrev = padX(graph->GetPointX(i - 1));
        const double_t lowPrev = padY(graph->GetPointY(i - 1) - graph->GetErrorYlow(i - 1)), highPrev = padY(graph->GetPointY(i - 1) + graph->GetErrorYhigh(i - 1));
        const double_t low = padY(yUser - eyLow), high = padY(yUser + eyHigh);
        const double_t c1 = std::max(std::min(cellX(xPrev), cellX(x)), 0.), c2 = std::min(std::max(cellX(xPrev), cellX(x)), static_cast<double_t>(nx));
        for (double_t c = std::floor(c1); c <= c2; c += 1.) {
          const double_t t = (cellX(x) == cellX(xPrev)) ? 0. : (c - cellX(xPrev)) / (cellX(x) - cellX(xPrev));
          if (t < 0. || t > 1.) continue;
          setRect(c, cellY(lowPrev + t * (low - lowPrev)), c, cellY(highPrev + t * (high - highPrev)), 1u);
        }
      }
    }
  };
  auto markFunc = [&](TF1* func) {
    if (func->GetNdim() != 1) return;
    double_t xMin{}, xMax{};
    func->GetRange(xMin, xMax);
    const int32_t nPoints = std::max(func->GetNpx(), 2);
    double_t xPrev{}, yPrev{};
    for (int32_t i = 0; i < nPoints; ++i) {
      const double_t xUser = xMin + (xMax - xMin) * i / (nPoints - 1);
      const double_t x = padX(xUser);
      const double_t y = padY(func->Eval(xUser));
      if (i > 0) markLine(xPrev, yPrev, x, y);
      xPrev = x;
      yPrev = y;
    }
  };

  for (auto* obj : *pad->GetListOfPrimitives()) {
    if (obj->InheritsFrom(TFrame::Class())) {
      // only the area within the frame is considered anyway
      continue;
    } else if (auto* pave = dynamic_cast<TPave*>(obj)) {
      // legends and texts placed before (coordinates in NDC)
      setRect(pave->GetX1NDC() * nx, pave->GetY1NDC() * ny, pave->GetX2NDC() * nx, pave->GetY2NDC() * ny, 1u);
    } else if (auto* box = dynamic_cast<TBox*>(obj)) {
      setRect(cellX(box->GetX1()), cellY(box->GetY1()), cellX(box->GetX2()), cellY(box->GetY2()), 1u);
    } else if (auto* hist = dynamic_cast<TH1*>(obj)) {
      if (!TString(hist->GetName()).BeginsWith("axis_hist") && !TString(hist->GetName()).Contains("hframe")) markHist(hist);
    } else if (auto* graph = dynamic_cast<TGraph*>(obj)) {
      markGraph(graph);
    } else if (auto* multiGraph = dynamic_cast<TMultiGraph*>(obj)) {
      for (auto* subGraph : *multiGraph->GetListOfGraphs()) {
        markGraph(static_cast<TGraph*>(subGraph));
      }
    } else if (auto* stack = dynamic_cast<THStack*>(obj)) {
      for (auto* subHist : *stack->GetHists()) {
        markHist(static_cast<TH1*>(subHist));
      }
    } else if (auto* func = dynamic_cast<TF1*>(obj)) {
      markFunc(func);
    }
  }

  // summed area table to check a whole rectangle at once
  vector<int32_t> sum(static_cast<size_t>(nx + 1) * (ny + 1), 0);
  for (int32_t i = 0; i < nx; ++i) {
    for (int32_t j = 0; j < ny; ++j) {
      sum[(i + 1) + static_cast<size_t>(j + 1) * (nx + 1)] = blocked[i + static_cast<size_t>(j) * nx] + sum[i + static_cast<size_t>(j + 1) * (nx + 1)] + sum[(i + 1) + static_cast<size_t>(j) * (nx + 1)] - sum[i + static_cast<size_t>(j) * (nx + 1)];
    }
  }
  // whether a box at this position (lower left corner in NDC) overlaps with any cell occupied by drawn objects
  auto isBlocked = [&](double_t x, double_t y) {
    const int32_t i1 = std::clamp(static_cast<int32_t>(std::floor(x * nx)), 0, nx);
    const int32_t i2 = std::clamp(static_cast<int32_t>(std::ceil((x + width) * nx)), 0, nx);
    const int32_t j1 = std::clamp(static_cast<int32_t>(std::floor(y * ny)), 0, ny);
    const int32_t j2 = std::clamp(static_cast<int32_t>(std::ceil((y + height) * ny)), 0, ny);
    auto at = [&](int32_t a, int32_t b) { return sum[a + static_cast<size_t>(b) * (nx + 1)]; };
    return (at(i2, j2) - at(i1, j2) - at(i2, j1) + at(i1, j1)) != 0;
  };
  // candidate positions: both edges of the free area and all cell boundaries in between
  if (freeArea[2] - width < freeArea[0] || freeArea[3] - height < freeArea[1]) return false;
  auto candidates = [](double_t min, double_t max, int32_t nCells) {
    vector<double_t> positions{min};
    for (int32_t k = static_cast<int32_t>(std::floor(min * nCells)) + 1; k < max * nCells; ++k) {
      positions.push_back(static_cast<double_t>(k) / nCells);
    }
    if (max > min) positions.push_back(max);
    return positions;
  };
  const auto xPositions = candidates(freeArea[0], freeArea[2] - width, nx);
  const auto yPositions = candidates(freeArea[1], freeArea[3] - height, ny);

  // distance (in pixels) of the box at this position to the given corner of the free area
  auto distanceToCorner = [&](box_placement_t boxCorner, double_t x, double_t y) {
    const bool isLeft = (boxCorner == bottom_left || boxCorner == top_left);
    const bool isBottom = (boxCorner == bottom_left || boxCorner == bottom_right);
    const double_t dx = ((isLeft) ? x - freeArea[0] : freeArea[2] - (x + width)) * pad->GetWw();
    const double_t dy = ((isBottom) ? y - freeArea[1] : freeArea[3] - (y + height)) * pad->GetWh();
    return dx * dx + dy * dy;
  };
  // for best_corner the one the box gets closest to is used (in case of a tie in this order of preference)
  const vector<box_placement_t> corners = (placement == best_corner) ? vector<box_placement_t>{top_left, top_right, bottom_right, bottom_left} : vector<box_placement_t>{placement};
  bool found = false;
  double_t minDistance{};
  size_t minRank{};  // position of the corner in the order of preference
  for (const double_t x : xPositions) {
    for (const double_t y : yPositions) {
      if (isBlocked(x, y)) continue;
      for (size_t rank = 0; rank < corners.size(); ++rank) {
        const double_t distance = distanceToCorner(corners[rank], x, y);
        if (found && (distance > minDistance || (distance == minDistance && rank >= minRank))) continue;
        found = true;
        minDistance = distance;
        minRank = rank;
        lowerLeftX = x;
        lowerLeftY = y;
      }
    }
  }
  return found;
}

//**************************************************************************************************
/**
 * Functions to retrieve a copy or projection of the stored data properly casted it to its actual ROOT type.
 */
//**************************************************************************************************
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj, const optional<Plot::Pad::Data::proj_info_t>& projInfo)
{
  if (obj) {
    bool addDirStatus = TH1::AddDirectoryStatus();
    TH1::AddDirectory(false);
    auto addDirGuard = make_scope_guard([addDirStatus]() { TH1::AddDirectory(addDirStatus); });

    if (projInfo) {
      string name = obj->GetName();
      name += projInfo->GetNameSuffix();
      auto returnPointer = GetProjection(obj, *projInfo);
      if (returnPointer &&
          std::visit([](auto&& ptr) { return ptr != nullptr; }, *returnPointer)) {
        std::visit([&name](auto&& ptr) { ptr->SetName(name.data()); ptr->SetBit(kCanDelete); }, *returnPointer);
        return returnPointer;
      } else {
        ERROR("Projection failed for {}.", obj->GetName());
      }
    } else {
      // TProfile2D is TH2, TH2 is TH1, TH3 is TH1, TProfile is TH1, TF3 is TF2, TF2 is TF1
      if (auto returnPointer = GetDataClone<TProfile2D, TH2, TH3, TProfile, TH1, TGraph2D, TGraph, TF3, TF2, TF1, TEfficiency>(obj)) {
        std::visit([](auto&& ptr) { ptr->SetBit(kCanDelete); }, *returnPointer);
        return returnPointer;
      } else {
        ERROR("Input data {} is of unsupported type {}.", obj->GetName(), obj->ClassName());
      }
    }
  }
  return nullopt;
}

template <typename T>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if (obj && obj->InheritsFrom(T::Class())) {
    if constexpr (std::is_same_v<T, TEfficiency>) {
      auto teff = static_cast<TEfficiency*>(obj);
      int dim = teff->GetDimension();
      if (dim == 1) {
        return static_cast<TGraph*>(teff->CreateGraph());
      } else if (dim == 2) {
        return static_cast<TH2*>(teff->CreateHistogram());
      }
    } else {
      return static_cast<T*>(obj->Clone());
    }
  }
  return nullopt;
}

template <typename T, typename Next, typename... Rest>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if (auto returnPointer = GetDataClone<T>(obj)) return returnPointer;
  return GetDataClone<Next, Rest...>(obj);
}

optional<data_ptr_t> PlotPainter::GetProjection(TObject* obj, Plot::Pad::Data::proj_info_t projInfo)
{
  const bool isProfile = projInfo.isProfile && *projInfo.isProfile;
  // only 1d and 2d histograms are valid outputs! (could be extended to 3d if there is a way to plot this)
  if (projInfo.dims.size() == 0 || projInfo.dims.size() > 2) {
    ERROR("Invalid number of dimensions specified for projection of histogram {}.", obj->GetName());
    return nullopt;
  }
  int32_t nDims{};
  if (obj->InheritsFrom(THnBase::Class())) {
    nDims = static_cast<THnBase*>(obj)->GetNdimensions();
  } else if (obj->InheritsFrom(TH1::Class())) {
    nDims = static_cast<TH1*>(obj)->GetDimension();
  } else {
    ERROR("Cannot do {} for type {} ({}).", (isProfile) ? "profiles" : "projections", obj->ClassName(), obj->GetName());
    return nullopt;
  }
  for (auto dim : projInfo.dims) {
    if (dim >= nDims) {
      ERROR("Cannot project {} onto dimension {}: it only has {} dimension{}.", obj->GetName(), dim, nDims, (nDims == 1) ? "" : "s");
      return nullopt;
    }
  }
  if (projInfo.dims.size() == 2 && projInfo.dims[0] == projInfo.dims[1]) {
    ERROR("Cannot project {} onto the same dimension twice.", obj->GetName());
    return nullopt;
  }
  // store original axis ranges so they can be reset by scope guard
  struct saved_range_t {
    TAxis* axis;
    int32_t first;
    int32_t last;
    bool wasRestricted;
  };
  std::vector<saved_range_t> savedRanges;
  auto remember = [&savedRanges](TAxis* axis) {
    if (axis) savedRanges.push_back({axis, axis->GetFirst(), axis->GetLast(), axis->TestBit(TAxis::kAxisRange)});
  };
  if (obj->InheritsFrom(THnBase::Class())) {
    auto* histPtr = static_cast<THnBase*>(obj);
    for (int32_t i = 0; i < histPtr->GetNdimensions(); ++i) {
      remember(histPtr->GetAxis(i));
    }
  } else if (obj->InheritsFrom(TH1::Class())) {
    auto* histPtr = static_cast<TH1*>(obj);
    for (int16_t i = 0; i < histPtr->GetDimension(); ++i) {
      remember(GetAxis(histPtr, i));
    }
  }
  auto rangeGuard = make_scope_guard([savedRanges]() {
    for (const auto& saved : savedRanges) {
      if (saved.wasRestricted) {
        saved.axis->SetRange(saved.first, saved.last);
      } else {
        saved.axis->SetRange();  // SetRange(1, nBins) would leave kAxisRange set
      }
    }
  });

  if (obj->InheritsFrom(THnBase::Class()) && !isProfile) {
    THnBase* histPtr = static_cast<THnBase*>(obj);
    // first reset all ranges
    for (int16_t i = 0; i < histPtr->GetNdimensions(); ++i) {
      histPtr->GetAxis(i)->SetRange();
    }
    for (const auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= histPtr->GetNdimensions()) {
        ERROR("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      int32_t minBin = (projInfo.isUserCoord && *projInfo.isUserCoord) ? histPtr->GetAxis(rangeDim)->FindBin(std::get<1>(rangeTuple)) : static_cast<int>(std::get<1>(rangeTuple));
      int32_t maxBin = (projInfo.isUserCoord && *projInfo.isUserCoord) ? histPtr->GetAxis(rangeDim)->FindBin(std::get<2>(rangeTuple)) : static_cast<int>(std::get<2>(rangeTuple));
      histPtr->GetAxis(rangeDim)->SetRange(minBin, maxBin);
    }
    if (projInfo.dims.size() == 2) {
      return histPtr->Projection(projInfo.dims[1], projInfo.dims[0]);
    } else if (projInfo.dims.size() == 1) {
      return histPtr->Projection(projInfo.dims[0]);
    }
  } else if (obj->InheritsFrom(TH3::Class())) {
    TH3* histPtr = static_cast<TH3*>(obj);
    // first reset all ranges
    for (int16_t i = 0; i < 3; ++i) {
      GetAxis(histPtr, i)->SetRange();
    }
    for (const auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= 3) {
        ERROR("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      int32_t minBin = (projInfo.isUserCoord && *projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<1>(rangeTuple)) : static_cast<int32_t>(std::get<1>(rangeTuple));
      int32_t maxBin = (projInfo.isUserCoord && *projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<2>(rangeTuple)) : static_cast<int32_t>(std::get<2>(rangeTuple));
      GetAxis(histPtr, rangeDim)->SetRange(minBin, maxBin);
    }
    if (projInfo.dims.size() == 2) {
      // get string if it is "xy" or "yx" or "zx"...
      if (isProfile) {
        return histPtr->Project3DProfile((GetAxisStr(projInfo.dims[1]) + GetAxisStr(projInfo.dims[0])).data());
      } else {
        return static_cast<TH2*>(histPtr->Project3D((GetAxisStr(projInfo.dims[1]) + GetAxisStr(projInfo.dims[0])).data()));
      }
    } else if (projInfo.dims.size() == 1) {
      return histPtr->Project3D(GetAxisStr(projInfo.dims[0]).data());
    }
  } else if (obj->InheritsFrom(TH2::Class())) {
    TH2* histPtr = static_cast<TH2*>(obj);
    if (projInfo.dims.size() > 1) {
      ERROR("Invalid dimension specified for projecting histogram {}.", obj->GetName());
      return nullopt;
    }
    // first reset all ranges
    for (int16_t i = 0; i < 2; ++i) {
      GetAxis(histPtr, i)->SetRange();
    }
    for (const auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= 2) {
        ERROR("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      int32_t minBin = (projInfo.isUserCoord && *projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<1>(rangeTuple)) : static_cast<int32_t>(std::get<1>(rangeTuple));
      int32_t maxBin = (projInfo.isUserCoord && *projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<2>(rangeTuple)) : static_cast<int32_t>(std::get<2>(rangeTuple));
      GetAxis(histPtr, rangeDim)->SetRange(minBin, maxBin);
    }
    if (projInfo.dims[0] == 0) {
      if (isProfile) {
        return histPtr->ProfileX("_px");
      } else {
        return histPtr->ProjectionX("_px");
      }
    } else if (projInfo.dims[0] == 1) {
      if (isProfile) {
        return histPtr->ProfileY("_py");
      } else {
        return histPtr->ProjectionY("_py");
      }
    } else {
      ERROR("Invalid dimension specified for {} from {} ({}).", (isProfile) ? "profile" : "projection", obj->GetName(), obj->ClassName());
    }
  } else {
    ERROR("Cannot do {} for type {} ({}).", (isProfile) ? "profiles" : "projections", obj->ClassName(), obj->GetName());
  }
  return nullopt;
}

template <typename T>
TAxis* PlotPainter::GetAxis(T* histPtr, int16_t i)
{
  switch (i) {
    case 0:
      return histPtr->GetXaxis();
    case 1:
      return histPtr->GetYaxis();
    case 2:
      return histPtr->GetZaxis();
    default:
      return nullptr;
  }
}

string PlotPainter::GetAxisStr(int16_t i)
{
  switch (i) {
    case 0:
      return "x";
    case 1:
      return "y";
    case 2:
      return "z";
    default:
      return "";
  }
}

//**************************************************************************************************
/**
 * Helper-functions for (interpolated) division of various root data types.
 * Return false if the division is not possible.
 */
//**************************************************************************************************
bool PlotPainter::Divide(TGraph* numerator, TGraph* denominator, bool binomialErrors)
{
  int32_t numN = numerator->GetN();
  double_t* numX = numerator->GetX();
  double_t* numY = numerator->GetY();
  double_t* numEy = numerator->GetEY();
  double_t* numEyLow = nullptr;
  double_t* numEyHigh = nullptr;
  if (auto ptr = dynamic_cast<TGraphAsymmErrors*>(numerator)) {
    numEyLow = ptr->GetEYlow();
    numEyHigh = ptr->GetEYhigh();
  }
  int32_t denomN = denominator->GetN();
  double_t* denomX = denominator->GetX();
  double_t* denomY = denominator->GetY();
  double_t* denomEy = denominator->GetEY();
  double_t* denomEyLow = nullptr;
  double_t* denomEyHigh = nullptr;
  if (auto ptr = dynamic_cast<TGraphAsymmErrors*>(denominator)) {
    denomEyLow = ptr->GetEYlow();
    denomEyHigh = ptr->GetEYhigh();
  }

  bool doInterpol = false;
  if (numN > denomN || (numEy && (denomEyLow && denomEyHigh)) || ((numEyLow && numEyHigh) && denomEy)) {
    doInterpol = true;
  } else {
    // check if graphs have the same x values
    for (int32_t i = 0; i < numN; ++i) {
      if (numX[i] != denomX[i]) {
        doInterpol = true;
        break;
      }
    }
  }

  bool deleteDenom = false;
  if (doInterpol) {
    if (denomN < 2) {
      ERROR("Cannot interpolate {} for the division: it has {} point{}, at least 2 are needed.", denominator->GetName(), denomN, (denomN == 1) ? "" : "s");
      return false;
    }
    // spline interpolation requires sorted values
    if (!denominator->TestBit(TGraph::kIsSortedX)) {
      denominator = static_cast<TGraph*>(denominator->Clone("tmp"));
      denominator->Sort();
      denomX = denominator->GetX();
      denomY = denominator->GetY();
      denomEy = denominator->GetEY();
      if (auto ptr = dynamic_cast<TGraphAsymmErrors*>(denominator)) {
        denomEyLow = ptr->GetEYlow();
        denomEyHigh = ptr->GetEYhigh();
      }
      deleteDenom = true;
    }

    // create splines for values and error envelope of denominator
    vector<double_t> denomShiftLow(denomN);
    vector<double_t> denomShiftHigh(denomN);
    bool errorlessDenom = false;
    for (int32_t i = 0; i < denomN; ++i) {
      if (denomEy) {
        denomShiftLow[i] = denomY[i] - denomEy[i];
        denomShiftHigh[i] = denomY[i] + denomEy[i];
      } else if (denomEyLow && denomEyHigh) {
        denomShiftLow[i] = denomY[i] - denomEyLow[i];
        denomShiftHigh[i] = denomY[i] + denomEyHigh[i];
      } else {
        errorlessDenom = true;
        break;
      }
    }
    TSpline3 denomSpline("denomSpline", denominator);
    optional<TSpline3> denomSplineLow;
    optional<TSpline3> denomSplineHigh;
    if (!errorlessDenom) {
      denomSplineLow.emplace("denomSplineLow", denomX, denomShiftLow.data(), denomN);
      denomSplineHigh.emplace("denomSplineHigh", denomX, denomShiftHigh.data(), denomN);
    }

    // reset denominator pointers
    denomY = nullptr;
    denomEy = nullptr;
    denomEyLow = nullptr;
    denomEyHigh = nullptr;

    // create arrays of interpolated points and errors at numerator positions
    denomY = new double_t[numN];
    if (!errorlessDenom) {
      if (numEy) {
        denomEy = new double_t[numN];
      } else if (numEyLow && numEyHigh) {
        denomEyLow = new double_t[numN];
        denomEyHigh = new double_t[numN];
      }
    }
    for (int32_t i = 0; i < numN; ++i) {
      denomY[i] = denomSpline.Eval(numX[i]);
      if (denomEy) {
        denomEy[i] = std::sqrt(0.5 * (std::pow(denomY[i] - denomSplineLow->Eval(numX[i]), 2) + std::pow(denomSplineHigh->Eval(numX[i]) - denomY[i], 2)));
      } else if (denomEyLow && denomEyHigh) {
        denomEyLow[i] = std::abs(denomY[i] - denomSplineLow->Eval(numX[i]));
        denomEyHigh[i] = std::abs(denomSplineHigh->Eval(numX[i]) - denomY[i]);
      }
    }
  }

  // compute ratio and errors
  for (int32_t i = 0; i < numN; ++i) {
    if (!denomY[i]) {
      numY[i] = 0;
      if (numEy) numEy[i] = 0.;
      if (numEyLow) numEyLow[i] = 0.;
      if (numEyHigh) numEyHigh[i] = 0.;
    } else {
      for (auto [errNum, errDenom] : vector<tuple<double_t*, double_t*>>{{numEy, denomEy}, {numEyLow, denomEyHigh}, {numEyHigh, denomEyLow}}) {
        if (errNum) {
          double_t denomError = (errDenom) ? errDenom[i] : 0.;
          if (binomialErrors) {
            // binomial error propagation (as implemented in root)
            errNum[i] = (numY[i] == denomY[i]) ? 0. : std::sqrt(std::abs(((1. - 2. * numY[i] / denomY[i]) * std::pow(errNum[i], 2) + std::pow(numY[i], 2) * std::pow(denomError, 2) / std::pow(denomY[i], 2)) / std::pow(denomY[i], 2)));
          } else {
            // gaussian error propagation
            errNum[i] = std::sqrt(std::pow(errNum[i] / denomY[i], 2) + std::pow(denomError * numY[i] / std::pow(denomY[i], 2), 2));
          }
        }
      }
      numY[i] = numY[i] / denomY[i];
    }
  }
  if (doInterpol) {
    if (denomY) delete[] denomY;
    if (denomEy) delete[] denomEy;
    if (denomEyLow) delete[] denomEyLow;
    if (denomEyHigh) delete[] denomEyHigh;
  }
  if (deleteDenom) {
    delete denominator;
  }
  return true;
}
bool PlotPainter::Divide(TH1* numerator, TGraph* denominator, bool binomialErrors)
{
  if (numerator->GetDimension() != 1) {
    ERROR("Cannot divide higher dimensional histogram by 1D graph.");
    return false;
  }
  if (numerator->GetXaxis()->IsAlphanumeric()) {
    ERROR("Cannot divide alphanumeric histogram by 1D graph.");
    return false;
  }
  TGraphErrors numeratorGraph(numerator);
  if (!Divide(&numeratorGraph, denominator, binomialErrors)) return false;
  numerator->Reset();
  for (int32_t i = 0; i < numeratorGraph.GetN(); ++i) {
    numerator->SetBinContent(i + 1, numeratorGraph.GetY()[i]);
    numerator->SetBinError(i + 1, numeratorGraph.GetEY()[i]);
  }
  return true;
}
bool PlotPainter::Divide(TGraph* numerator, TH1* denominator, bool binomialErrors)
{
  if (denominator->GetDimension() != 1) {
    ERROR("Cannot divide 1D graph by higher dimensional histogram.");
    return false;
  }
  if (denominator->GetXaxis()->IsAlphanumeric()) {
    ERROR("Cannot divide 1D graph by alphanumeric histogram.");
    return false;
  }
  TGraphErrors denominatorGraph(denominator);
  denominatorGraph.SetName(denominator->GetName());  // used in messages
  return Divide(numerator, &denominatorGraph, binomialErrors);
}
bool PlotPainter::Divide(TH1* numerator, TH1* denominator, bool binomialErrors)
{
  auto sameAxisBinning = [](TAxis* a, TAxis* b) -> bool {
    if (a->GetNbins() != b->GetNbins()) return false;
    if (a->IsAlphanumeric() || b->IsAlphanumeric()) return true;
    for (int32_t i = 1; i <= a->GetNbins() + 1; ++i) {
      if (a->GetBinLowEdge(i) != b->GetBinLowEdge(i)) return false;
    }
    return true;
  };
  bool sameBinning = sameAxisBinning(numerator->GetXaxis(), denominator->GetXaxis());
  if (numerator->GetDimension() >= 2 && denominator->GetDimension() >= 2) {
    sameBinning = sameBinning && sameAxisBinning(numerator->GetYaxis(), denominator->GetYaxis());
  }
  if (numerator->GetDimension() >= 3 && denominator->GetDimension() >= 3) {
    sameBinning = sameBinning && sameAxisBinning(numerator->GetZaxis(), denominator->GetZaxis());
  }
  if (sameBinning) {
    numerator->Divide(numerator, denominator, 1., 1., (binomialErrors) ? "B" : "");
    return true;
  }
  for (TAxis* axis : {numerator->GetXaxis(), numerator->GetYaxis(), numerator->GetZaxis(), denominator->GetXaxis(), denominator->GetYaxis(), denominator->GetZaxis()}) {
    if (axis && axis->IsAlphanumeric()) {
      ERROR("Cannot do interpolated division for alphanumeric histograms.");
      return false;
    }
  }
  if (numerator->GetDimension() == 1 && denominator->GetDimension() == 1) {
    TGraphErrors denominatorGraph(denominator);
    denominatorGraph.SetName(denominator->GetName());  // used in messages
    return Divide(numerator, &denominatorGraph, binomialErrors);
  } else if (numerator->GetDimension() == 2 && denominator->GetDimension() == 2) {
    ERROR("Interpolated division of 2D histograms not yet supported.");
  } else if (numerator->GetDimension() == 3 && denominator->GetDimension() == 3) {
    ERROR("Interpolated division of 3D histograms not yet supported.");
  } else {
    ERROR("Dividing histograms of incompatible dimensions.");
  }
  return false;
}
bool PlotPainter::Divide(TH1* numerator, TF1* denominator, bool binomialErrors)
{
  if (denominator->GetNdim() > numerator->GetDimension()) {
    ERROR("Cannot divide histogram by higher dimensional function.");
    return false;
  }
  numerator->Divide(denominator);
  if (binomialErrors) {
    WARNING("Binomial errors not supported for division by function.");
  }
  return true;
}
bool PlotPainter::Divide(TGraph* numerator, TF1* denominator, bool binomialErrors)
{
  if (denominator->GetNdim() > 1) {
    ERROR("Cannot divide 1D graph by higher dimensional function.");
    return false;
  }
  double_t* x = numerator->GetX();
  double_t* y = numerator->GetY();
  double_t* ey = numerator->GetEY();
  double_t* eyLow = nullptr;
  double_t* eyHigh = nullptr;
  if (auto ptr = dynamic_cast<TGraphAsymmErrors*>(numerator)) {
    eyLow = ptr->GetEYlow();
    eyHigh = ptr->GetEYhigh();
  }
  for (int32_t i = 0; i < numerator->GetN(); ++i) {
    double_t denom = denominator->Eval(x[i]);
    y[i] = (denom) ? y[i] / denom : 0.;
    if (ey) {
      ey[i] = (denom) ? ey[i] / denom : 0.;
    } else if (eyLow && eyHigh) {
      eyLow[i] = (denom) ? eyLow[i] / denom : 0.;
      eyHigh[i] = (denom) ? eyHigh[i] / denom : 0.;
    }
  }
  if (binomialErrors) {
    WARNING("Binomial errors not supported for division by function.");
  }
  return true;
}

//**************************************************************************************************
/**
 * Deletes data points of graph beyond cutoff values.
 */
//**************************************************************************************************
void PlotPainter::SetGraphRange(TGraph* graph, optional<double_t> min, optional<double_t> max)
{
  // sort the points first for the following algorithm to work properly
  graph->Sort();

  int32_t pointsToRemoveHigh{};
  int32_t pointsToRemoveLow{};

  for (int32_t i = 0; i < graph->GetN(); ++i) {
    if (min && graph->GetX()[i] < *min) {
      ++pointsToRemoveLow;
    }
    if (max && graph->GetX()[i] > *max) {
      ++pointsToRemoveHigh;
    }
  }

  for (int32_t i = 0; i < pointsToRemoveHigh; ++i) {
    graph->RemovePoint(graph->GetN() - 1);
  }
  for (int32_t i = 0; i < pointsToRemoveLow; ++i) {
    graph->RemovePoint(0);
  }
}

//**************************************************************************************************
/**
 * Scales histogram axis by a constant value.
 */
//**************************************************************************************************
void PlotPainter::ScaleAxis(TH1* hist, int16_t axisIndex, double_t scaleFactor)
{
  TAxis* axis = GetAxis(hist, axisIndex);
  if (axis->GetXbins()->GetSize() > 0) {
    // variable bin widths
    vector<double_t> newBinEdges;
    auto* edges = axis->GetXbins()->GetArray();
    newBinEdges.reserve(axis->GetXbins()->GetSize());
    for (int32_t i = 0; i < axis->GetXbins()->GetSize(); ++i) {
      newBinEdges.push_back(edges[i] * scaleFactor);
    }
    axis->Set(axis->GetNbins(), newBinEdges.data());
  } else {
    // fixed bin width
    axis->Set(axis->GetNbins(), axis->GetXmin() * scaleFactor, axis->GetXmax() * scaleFactor);
  }
  hist->ResetStats();
}

//**************************************************************************************************
/**
 * Rescales the point coordinates (and associated errors) of a graph along one axis by a constant factor.
 */
//**************************************************************************************************
void PlotPainter::ScaleGraphAxis(TGraph* graph, int16_t axis, double_t scaleFactor)
{
  double_t* values = (axis == 0) ? graph->GetX() : graph->GetY();
  double_t* errors = (axis == 0) ? graph->GetEX() : graph->GetEY();
  for (int32_t i{}; i < graph->GetN(); ++i) {
    values[i] *= scaleFactor;
    if (errors) errors[i] *= scaleFactor;
  }
  if (auto* asymmErrors = dynamic_cast<TGraphAsymmErrors*>(graph)) {
    double_t* errorsLow = (axis == 0) ? asymmErrors->GetEXlow() : asymmErrors->GetEYlow();
    double_t* errorsHigh = (axis == 0) ? asymmErrors->GetEXhigh() : asymmErrors->GetEYhigh();
    for (int32_t i{}; i < graph->GetN(); ++i) {
      errorsLow[i] *= scaleFactor;
      errorsHigh[i] *= scaleFactor;
    }
  }
}

void PlotPainter::ScaleGraphAxis(TGraph2D* graph, int16_t axis, double_t scaleFactor)
{
  double_t* values{};
  double_t* errors{};
  double_t* errorsLow{};
  double_t* errorsHigh{};
  switch (axis) {
    case 0:
      values = graph->GetX();
      errors = graph->GetEX();
      errorsLow = graph->GetEXlow();
      errorsHigh = graph->GetEXhigh();
      break;
    case 1:
      values = graph->GetY();
      errors = graph->GetEY();
      errorsLow = graph->GetEYlow();
      errorsHigh = graph->GetEYhigh();
      break;
    case 2:
      values = graph->GetZ();
      errors = graph->GetEZ();
      errorsLow = graph->GetEZlow();
      errorsHigh = graph->GetEZhigh();
      break;
    default:
      return;
  }
  for (int32_t i{}; i < graph->GetN(); ++i) {
    values[i] *= scaleFactor;
    if (errors) errors[i] *= scaleFactor;
    if (errorsLow) errorsLow[i] *= scaleFactor;
    if (errorsHigh) errorsHigh[i] *= scaleFactor;
  }
}

//**************************************************************************************************
/**
 * Rescales functions or function arguments by a constant factor.
 */
//**************************************************************************************************
TF1* PlotPainter::ScaleFunc(TF1* func, double_t domainFactorX, double_t contentFactor)
{
  func->AddToGlobalList(false);
  auto origFunc = std::shared_ptr<TF1>(func);
  auto* newFunc = new TF1(
    origFunc->GetName(),
    [origFunc, domainFactorX, contentFactor](double_t* x, double_t*) { return contentFactor * origFunc->Eval(x[0] / domainFactorX); },
    origFunc->GetXmin() * domainFactorX, origFunc->GetXmax() * domainFactorX, 0, 1, TF1::EAddToList::kNo);
  CopyFuncAttributes(origFunc.get(), newFunc);
  return newFunc;
}

TF2* PlotPainter::ScaleFunc(TF2* func, double_t domainFactorX, double_t domainFactorY, double_t contentFactor)
{
  func->AddToGlobalList(false);
  auto origFunc = std::shared_ptr<TF2>(func);
  auto* newFunc = new TF2(
    origFunc->GetName(),
    [origFunc, domainFactorX, domainFactorY, contentFactor](double_t* x, double_t*) {
      return contentFactor * origFunc->Eval(x[0] / domainFactorX, x[1] / domainFactorY);
    },
    origFunc->GetXmin() * domainFactorX, origFunc->GetXmax() * domainFactorX,
    origFunc->GetYmin() * domainFactorY, origFunc->GetYmax() * domainFactorY, 0, 2, TF1::EAddToList::kNo);
  CopyFuncAttributes(origFunc.get(), newFunc);
  return newFunc;
}

TF3* PlotPainter::ScaleFunc(TF3* func, double_t domainFactorX, double_t domainFactorY, double_t domainFactorZ, double_t contentFactor)
{
  func->AddToGlobalList(false);
  auto origFunc = std::shared_ptr<TF3>(func);
  auto* newFunc = new TF3(
    origFunc->GetName(),
    [origFunc, domainFactorX, domainFactorY, domainFactorZ, contentFactor](double_t* x, double_t*) {
      return contentFactor * origFunc->Eval(x[0] / domainFactorX, x[1] / domainFactorY, x[2] / domainFactorZ);
    },
    origFunc->GetXmin() * domainFactorX, origFunc->GetXmax() * domainFactorX,
    origFunc->GetYmin() * domainFactorY, origFunc->GetYmax() * domainFactorY,
    origFunc->GetZmin() * domainFactorZ, origFunc->GetZmax() * domainFactorZ, 0, 3, TF1::EAddToList::kNo);
  CopyFuncAttributes(origFunc.get(), newFunc);
  return newFunc;
}

//**************************************************************************************************
/**
 * Returns actual dimensions in pixel of the text with latex formatting.
 */
//**************************************************************************************************
tuple<uint32_t, uint32_t> PlotPainter::GetTextDimensions(TLatex& text, TPad* pad)
{
  uint32_t width{};
  uint32_t height{};
  int16_t font{text.GetTextFont()};

  bool isBatch = gPad->IsBatch();
  if (isBatch) {
    // in batch mode ROOT calculates the bounding LaTex boxes wrongly, therefore disable it for the calculation
    gROOT->SetBatch(false);
    gPad->SetBatch(false);
  }
  if (font % 10 <= 2) {
    text.GetBoundingBox(width, height);
  } else {
    TLatex textBox{text};
    textBox.SetTextFont(font - 1);
    double_t dy{pad->AbsPixeltoY(0) - pad->AbsPixeltoY(static_cast<int32_t>(text.GetTextSize() * mScale))};
    double_t textSize{dy / (pad->GetY2() - pad->GetY1())};
    textBox.SetTextSize(textSize);
    textBox.GetBoundingBox(width, height);
  }
  if (isBatch) {
    gPad->SetBatch(true);
    gROOT->SetBatch(true);
  }
  return {width, height};
}

//**************************************************************************************************
/**
 * Converts NDC text size to pixel.
 */
//**************************************************************************************************
float_t PlotPainter::GetTextSizePixel(float_t textSizeNDC)
{
  int32_t pad_width{gPad->XtoPixel(gPad->GetX2())};
  int32_t pad_height{gPad->YtoPixel(gPad->GetY1())};
  float_t textSizePixel{(pad_width < pad_height) ? textSizeNDC * pad_width : textSizeNDC * pad_height};
  return textSizePixel;
}

//**************************************************************************************************
/**
 * Function to replace placeholders in labels.
 */
//**************************************************************************************************
void PlotPainter::ReplacePlaceholders(string& str, TNamed* data_ptr)
{
  static const std::regex wordsRegex("<(name|title|entries|integral|mean|maximum|minimum).*?>");
  auto wordsBegin = std::sregex_iterator(str.begin(), str.end(), wordsRegex);
  auto wordsEnd = std::sregex_iterator();

  string result;
  size_t lastEnd = 0;  // offset into the original str, just past the previous match

  for (std::sregex_iterator match = wordsBegin; match != wordsEnd; ++match) {
    string matchStr = match->str();
    size_t matchPos = match->position();

    // copy the untouched text since the previous match, then compute the replacement
    result.append(str, lastEnd, matchPos - lastEnd);

    string format{};
    static const std::regex formatRegex("\\[.*?\\]");
    if (auto formatIt = std::sregex_iterator(matchStr.begin(), matchStr.end(), formatRegex); formatIt != std::sregex_iterator()) {
      format = formatIt->str();
      format = format.substr(1, format.size() - 2);
    }
    format.erase(remove(format.begin(), format.end(), '%'), format.end());
    format.erase(remove(format.begin(), format.end(), ' '), format.end());
    if (!(str_contains(format, "e") || str_contains(format, "f") || str_contains(format, "g") || str_contains(format, "E") || str_contains(format, "F") || str_contains(format, "G"))) {
      format = format + "g";
    }
    format = "{:" + format + "}";

    string replaceStr = matchStr;
    if (str_contains(matchStr, "name")) {
      replaceStr = data_ptr->GetName();

      // strip the numeric "<index>:" prefix PlotPainter adds right before drawing
      if (auto colonPos = replaceStr.find(":"); colonPos != string::npos) {
        replaceStr = replaceStr.substr(colonPos + 1);
      }

      // strip everything from a projection/binning suffix onward; those always start with '{',
      // sometimes preceded by "_Proj" or "_Prof" (see proj_info_t/data_info_t::GetNameSuffix())
      if (auto bracePos = replaceStr.find('{'); bracePos != string::npos) {
        for (const std::string_view marker : {"_Proj", "_Prof"}) {
          if (bracePos >= marker.size() && replaceStr.compare(bracePos - marker.size(), marker.size(), marker) == 0) {
            bracePos -= marker.size();
            break;
          }
        }
        replaceStr = replaceStr.substr(0, bracePos);
      }
    } else if (str_contains(matchStr, "title")) {
      replaceStr = data_ptr->GetTitle();
    } else if (data_ptr->InheritsFrom(TH1::Class())) {
      try {
        if (str_contains(matchStr, "entries")) {
          replaceStr = fmt::format(fmt::runtime(format), static_cast<TH1*>(data_ptr)->GetEntries());
        } else if (str_contains(matchStr, "integral")) {
          replaceStr = fmt::format(fmt::runtime(format), static_cast<TH1*>(data_ptr)->Integral());
        } else if (str_contains(matchStr, "mean")) {
          replaceStr = fmt::format(fmt::runtime(format), static_cast<TH1*>(data_ptr)->GetMean());
        } else if (str_contains(matchStr, "maximum")) {
          replaceStr = fmt::format(fmt::runtime(format), static_cast<TH1*>(data_ptr)->GetMaximum());
        } else if (str_contains(matchStr, "minimum")) {
          replaceStr = fmt::format(fmt::runtime(format), static_cast<TH1*>(data_ptr)->GetMinimum());
        }
      } catch (const fmt::format_error& e) {
        ERROR("Incompatible format string in {}: {}.", matchStr, e.what());
        replaceStr = matchStr;
      }
    }

    result += replaceStr;
    lastEnd = matchPos + matchStr.size();
  }
  result.append(str, lastEnd, string::npos);  // copy whatever remains after the last match

  str = std::move(result);
}

//**************************************************************************************************
/**
 * Helper to generate nColors between specified rgb endpoints.
 */
//**************************************************************************************************
vector<int16_t> PlotPainter::GenerateGradientColors(int32_t nColors, const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha, bool savePalette)
{
  if (rgbEndpoints.size() < 2) {
    ERROR("A colour gradient needs at least two endpoints, got {}.", rgbEndpoints.size());
    return {};
  }
  if (nColors < 1) {
    ERROR("Number of gradient colours must be positive, got {}.", nColors);
    return {};
  }
  uint16_t nPoints = rgbEndpoints.size();

  vector<double_t> red;
  vector<double_t> green;
  vector<double_t> blue;
  vector<double_t> stops;

  for (const auto& rgb : rgbEndpoints) {
    red.push_back(std::get<0>(rgb));
    green.push_back(std::get<1>(rgb));
    blue.push_back(std::get<2>(rgb));
    stops.push_back(std::get<3>(rgb));
  }
  int16_t firstColorIndex = TColor::CreateGradientColorTable(nPoints, stops.data(), red.data(), green.data(), blue.data(), nColors, alpha);
  if (firstColorIndex < 0) {
    ERROR("Could not create gradient colour table.");
    return {};
  }
  vector<int16_t> gradientColors(nColors);
  std::iota(gradientColors.begin(), gradientColors.end(), firstColorIndex);

  // TColor::CreateGradientColorTable() changes current palette as side effect
  if (!savePalette) gStyle->SetPalette(kBird);
  return gradientColors;
}

}  // end namespace SciRooPlot
