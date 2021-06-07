// PlottingFramework
//
// Copyright (C) 2019-2021  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see doc/CONTRIBUTORS.md
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

// framework dependencies
#include "PlotPainter.h"
#include "PlottingFramework.h"
#include "Logging.h"
#include "Helpers.h"

// std dependencies
#include <regex>
#include <numeric>

// root dependencies
#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "THn.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"
#include "TGraph2DErrors.h"
#include "TGraphSmooth.h"
#include "TF1.h"
#include "TF2.h"

#include "TFrame.h"
#include "TLegendEntry.h"
#include "TIterator.h"
#include "TObjArray.h"
#include "TObject.h"
#include "TObjectTable.h"
#include "TExec.h"
#include "TTimeStamp.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TText.h"
#include "TPaveText.h"
#include "TLatex.h"
#include "TSpline.h"
#include "TView.h"
#include "TApplication.h"
#include "TGWindow.h"

namespace PlottingFramework
{

//**************************************************************************************************
/**
 * Function to generate the plot.
 */
//**************************************************************************************************
unique_ptr<TCanvas> PlotPainter::GeneratePlot(Plot& plot, const unordered_map<string, unordered_map<string, std::unique_ptr<TObject>>>& dataBuffer)
{
  gStyle->SetOptStat(0); // this needs to be done before creating the canvas! at later stage it would add to list of primitives in pad...

  if (!(plot.GetWidth() || plot.GetHeight())) {
    ERROR("No dimensions specified for plot.");
    return nullptr;
  }
  bool fail = false;
  unique_ptr<TCanvas> canvas_ptr{new TCanvas(plot.GetUniqueName().data(), plot.GetUniqueName().data(), *plot.GetWidth() + 4, *plot.GetHeight() + 28)};
  // NB.: +4 and +28 are needed to undo hard-coded offsets in TCanvas.cxx line 595
  canvas_ptr->SetMargin(0., 0., 0., 0.);

  // apply user settings for plot
  if (plot.GetFillColor()) canvas_ptr->SetFillColor(*plot.GetFillColor());
  if (plot.GetFillStyle()) canvas_ptr->SetFillStyle(*plot.GetFillStyle());
  if (plot.IsFixAspectRatio()) canvas_ptr->SetFixedAspectRatio(*plot.IsFixAspectRatio());

  auto& padDefaults = plot[0];
  for (const auto& [padID, dummy] : plot.GetPads()) {
    if (padID == 0) continue; // pad 0 is used only to define the defaults
    auto& pad = plot[padID];  // needed because processData lambda cannot capture variable from structured binding ('dummy')

    // Pad placing
    array<double_t, 4> padPos = {0., 0., 1., 1.};
    if (!(pad.GetXLow() || pad.GetYLow() || pad.GetXUp() || pad.GetYUp()) || (*pad.GetXLow() > *pad.GetXUp()) || (*pad.GetYLow() > *pad.GetYUp()) || (*pad.GetXLow() < 0) || (*pad.GetXLow() > 1) || (*pad.GetXUp() < 0) || (*pad.GetXUp() > 1) || (*pad.GetYLow() < 0) || (*pad.GetYLow() > 1) || (*pad.GetYUp() < 0) || (*pad.GetYUp() > 1)) {
      WARNING("Position of pad {} was not defined properly! Drawing it over whole plot.", padID);
    } else {
      padPos = {*pad.GetXLow(), *pad.GetYLow(), *pad.GetXUp(), *pad.GetYUp()};
    }

    // get the settings for this pad
    auto& textFont = get_first(pad.GetDefaultTextFont(), padDefaults.GetDefaultTextFont());
    auto& textSize = get_first(pad.GetDefaultTextSize(), padDefaults.GetDefaultTextSize());
    auto& textColor = get_first(pad.GetDefaultTextColor(), padDefaults.GetDefaultTextColor());

    canvas_ptr->cd();
    string padName = "Pad_" + std::to_string(padID);

    TPad* pad_ptr = new TPad(padName.data(), "", padPos[0], padPos[1], padPos[2], padPos[3]);

    if (auto& marginTop = get_first(pad.GetMarginTop(), padDefaults.GetMarginTop())) pad_ptr->SetTopMargin(*marginTop);
    if (auto& marginBottom = get_first(pad.GetMarginBottom(), padDefaults.GetMarginBottom())) pad_ptr->SetBottomMargin(*marginBottom);
    if (auto& marginLeft = get_first(pad.GetMarginLeft(), padDefaults.GetMarginLeft())) pad_ptr->SetLeftMargin(*marginLeft);
    if (auto& marginRight = get_first(pad.GetMarginRight(), padDefaults.GetMarginRight())) pad_ptr->SetRightMargin(*marginRight);
    if (auto& padFillColor = get_first(pad.GetFillColor(), padDefaults.GetFillColor())) pad_ptr->SetFillColor(*padFillColor);
    if (auto& padFillStyle = get_first(pad.GetFillStyle(), padDefaults.GetFillStyle())) pad_ptr->SetFillStyle(*padFillStyle);
    if (auto& frameFillColor = get_first(pad.GetFillColorFrame(), padDefaults.GetFillColorFrame())) pad_ptr->SetFrameFillColor(*frameFillColor);
    if (auto& frameFillStyle = get_first(pad.GetFillStyleFrame(), padDefaults.GetFillStyleFrame())) pad_ptr->SetFrameFillStyle(*frameFillStyle);
    if (auto& frameLineColor = get_first(pad.GetLineColorFrame(), padDefaults.GetLineColorFrame())) pad_ptr->SetFrameLineColor(*frameLineColor);
    if (auto& frameLineStyle = get_first(pad.GetLineStyleFrame(), padDefaults.GetLineStyleFrame())) pad_ptr->SetFrameLineStyle(*frameLineStyle);
    if (auto& frameLineWidth = get_first(pad.GetLineWidthFrame(), padDefaults.GetLineWidthFrame())) pad_ptr->SetFrameLineWidth(*frameLineWidth);

    if (pad.GetDefaultMarkerColorsGradient().rgbEndpoints) {
      auto& gradient = pad.GetDefaultMarkerColorsGradient();
      pad.SetDefaultMarkerColors(GenerateGradientColors(get_first_or((int32_t)pad.GetData().size(), gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    } else if (padDefaults.GetDefaultMarkerColorsGradient().rgbEndpoints) {
      auto& gradient = padDefaults.GetDefaultMarkerColorsGradient();
      padDefaults.SetDefaultMarkerColors(GenerateGradientColors(get_first_or((int32_t)padDefaults.GetData().size(), gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    }
    if (pad.GetDefaultLineColorsGradient().rgbEndpoints) {
      auto& gradient = pad.GetDefaultLineColorsGradient();
      pad.SetDefaultLineColors(GenerateGradientColors(get_first_or((int32_t)pad.GetData().size(), gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    } else if (padDefaults.GetDefaultLineColorsGradient().rgbEndpoints) {
      auto& gradient = padDefaults.GetDefaultLineColorsGradient();
      padDefaults.SetDefaultLineColors(GenerateGradientColors(get_first_or((int32_t)padDefaults.GetData().size(), gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    }
    if (pad.GetDefaultFillColorsGradient().rgbEndpoints) {
      auto& gradient = pad.GetDefaultFillColorsGradient();
      pad.SetDefaultFillColors(GenerateGradientColors(get_first_or((int32_t)pad.GetData().size(), gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    } else if (padDefaults.GetDefaultFillColorsGradient().rgbEndpoints) {
      auto& gradient = padDefaults.GetDefaultFillColorsGradient();
      padDefaults.SetDefaultFillColors(GenerateGradientColors(get_first_or((int32_t)padDefaults.GetData().size(), gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha)));
    }
    // TODO: color gradient feature can be used for 2d palette as well
    if (auto& palette = get_first(pad.GetPalette(), padDefaults.GetPalette())) gStyle->SetPalette(*palette);

    pad_ptr->SetNumber(padID);
    pad_ptr->Draw();
    pad_ptr->cd();

    if (pad.GetData().empty()) {
      if (pad.GetLegendBoxes().empty() && pad.GetTextBoxes().empty()) {
        WARNING("Nothing to be drawn in pad {}.", padID);
        continue;
      }
    } else {
      // find data that should define the axis frame
      auto framePos = std::find_if(pad.GetData().begin(), pad.GetData().end(),
                                   [](auto& curData) { return curData->GetDefinesFrame(); });
      uint8_t frameDataID = (framePos != pad.GetData().end()) ? framePos - pad.GetData().begin() : 0u;
      // make a copy of data that will serve as axis frame and put it in front of data vector
      if (pad.GetData()[frameDataID]->GetType() == "ratio") {
        pad.GetData().insert(pad.GetData().begin(), std::make_shared<Plot::Pad::Ratio>(*std::dynamic_pointer_cast<Plot::Pad::Ratio>(pad.GetData()[frameDataID])));
      } else {
        pad.GetData().insert(pad.GetData().begin(), std::make_shared<Plot::Pad::Data>(*pad.GetData()[frameDataID]));
      }
      pad.GetData()[0]->SetLegendLabel(""); // axis frame should not appear in legend
    }

    TH1* axisHist_ptr{nullptr};
    string drawingOptions = "";
    uint16_t dataIndex{};
    for (auto& data : pad.GetData()) {
      if (data->GetDrawingOptions()) drawingOptions += *data->GetDrawingOptions();
      // obtain a copy of the current data
      // retrieve the actual pointer to the data
      auto processData = [&, padID = padID](auto&& data_ptr) {
        using data_type = std::decay_t<decltype(data_ptr)>;
        data_ptr->SetTitle(""); // FIXME: only make this invisible but don't remove useful metadata

        optional<drawing_options_t> defaultDrawingOption = data->GetDrawingOptionAlias();

        if (!data->GetDrawingOptions()) {
          // FIXME: avoid code duplication here by implementing this in more clever way
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>) {
            if (!defaultDrawingOption) {
              if (pad.GetDefaultDrawingOptionHist2d())
                defaultDrawingOption = pad.GetDefaultDrawingOptionHist2d();
              else if (padDefaults.GetDefaultDrawingOptionHist2d())
                defaultDrawingOption = padDefaults.GetDefaultDrawingOptionHist2d();
            }

            if (defaultDrawingOption && defaultDrawingOptions_Hist2d.find(*defaultDrawingOption) != defaultDrawingOptions_Hist2d.end()) {
              drawingOptions += defaultDrawingOptions_Hist2d.at(*defaultDrawingOption);
            }
          } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>) {
            if (!defaultDrawingOption)
              defaultDrawingOption = (pad.GetDefaultDrawingOptionHist())
                                       ? pad.GetDefaultDrawingOptionHist()
                                     : (padDefaults.GetDefaultDrawingOptionHist())
                                       ? padDefaults.GetDefaultDrawingOptionHist()
                                       : std::nullopt;

            if (defaultDrawingOption && defaultDrawingOptions_Hist.find(*defaultDrawingOption) != defaultDrawingOptions_Hist.end()) {
              drawingOptions += defaultDrawingOptions_Hist.at(*defaultDrawingOption);
            }
          } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph_1d>) {
            if (!defaultDrawingOption)
              defaultDrawingOption = (pad.GetDefaultDrawingOptionGraph())
                                       ? pad.GetDefaultDrawingOptionGraph()
                                     : (padDefaults.GetDefaultDrawingOptionGraph())
                                       ? padDefaults.GetDefaultDrawingOptionGraph()
                                       : std::nullopt;

            if (defaultDrawingOption && defaultDrawingOptions_Graph.find(*defaultDrawingOption) != defaultDrawingOptions_Graph.end()) {
              drawingOptions += defaultDrawingOptions_Graph.at(*defaultDrawingOption);
            }
          }
        }

        if (data->GetType() == "ratio") {
          // retrieve the actual pointer to the denominator data
          auto processDenominator = [&](auto&& denom_data_ptr) {
            using denom_data_type = std::decay_t<decltype(denom_data_ptr)>;
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>) {
              if constexpr (std::is_convertible_v<denom_data_type, data_ptr_t_hist>) {
                string divideOpt = (std::dynamic_pointer_cast<Plot::Pad::Ratio>(data)->GetIsCorrelated()) ? "B"
                                                                                                          : "";
                if (!data_ptr->Divide(data_ptr, denom_data_ptr, 1., 1., divideOpt.data())) {
                  WARNING("Could not divide histograms properly. Trying approximated division via spline interpolation. Errors will not be fully correct!");
                  DivideHistosInterpolated(data_ptr, denom_data_ptr);
                }
                if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>) {
                  data_ptr->GetZaxis()->SetTitle("ratio");
                } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>) {
                  data_ptr->GetYaxis()->SetTitle("ratio");
                }
              } else if constexpr (std::is_convertible_v<denom_data_type, data_ptr_t_graph>) {
                ERROR("Cannot divide histogram by graph.");
                //DivideHistGraphInterpolated(data_ptr, denom_data_ptr);
              }
            } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph_1d>) {
              if constexpr (std::is_convertible_v<denom_data_type, data_ptr_t_graph_1d>) {
                if (!DivideGraphs(data_ptr, denom_data_ptr)) // first try if exact division is possible
                {
                  WARNING("In general graphs cannot be divided. Trying approximated division via spline interpolation. Errors will not be fully correct!");
                  DivideGraphsInterpolated(data_ptr, denom_data_ptr);
                }
              } else if constexpr (std::is_convertible_v<denom_data_type, data_ptr_t_hist_1d>) {
                ERROR("Cannot divide graph by histogram.");
                //DivideGraphHistInterpolated(data_ptr, denom_data_ptr);
              }
            } else {
              ERROR("Unsupported division");
            }
            delete denom_data_ptr;
          };

          auto data_denom = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
          auto rawDenomData = GetDataClone(dataBuffer.at(data_denom->GetDenomIdentifier()).at(data_denom->GetDenomName()).get(), data_denom->GetProjInfoDenom());

          if (rawDenomData) {
            std::visit(processDenominator, *rawDenomData);
          } else {
            fail = true;
          }
        } // end ratio code

        // modify content (FIXME: this should be steered differently)
        // FIXME: probably this should be done after setting ranges but axis ranges depend on scaling!
        if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>) {
          if (str_contains(drawingOptions, "smooth")) {
            drawingOptions.erase(drawingOptions.find("smooth"), string("smooth").length());
            data_ptr->Smooth();
          }
        }
        if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>) {
          optional<double_t> scaleFactor;
          string scaleMode{};

          if (data->GetNormMode()) {
            double integral = data_ptr->Integral(); // integral in viewing range
            if (integral == 0.) {
              ERROR("Cannot normalize histogram because integral is zero.");
            } else {
              scaleFactor = 1. / integral;
            }
            if (*data->GetNormMode() > 0) scaleMode = "width";
          }
          if (data->GetScaleFactor()) {
            scaleFactor = (scaleFactor) ? (*scaleFactor) * (*data->GetScaleFactor())
                                        : (*data->GetScaleFactor());
          }
          if (scaleFactor) data_ptr->Scale(*scaleFactor);
        } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph_1d>) {
          // FIXME: violating DRY principle...
          optional<double_t> scaleFactor;
          string scaleMode{};

          if (data->GetNormMode()) {
            double integral = data_ptr->Integral(); // integral in viewing range
            if (integral == 0.) {
              ERROR("Cannot normalize graph because integral is zero.");
            } else {
              scaleFactor = 1. / integral;
            }
            if (*data->GetNormMode() > 0) {
              ERROR("Cannot normalize graph by width.");
            }
          }
          if (data->GetScaleFactor()) {
            scaleFactor = (scaleFactor) ? (*scaleFactor) * (*data->GetScaleFactor())
                                        : (*data->GetScaleFactor());
          }
          if (scaleFactor) ScaleGraph((TGraph*)data_ptr, *scaleFactor);
        }

        // first data is only used to define the axes
        if (dataIndex == 0) {
          bool isDrawn = false;
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>) {
            if (str_contains(drawingOptions, "Z")) {
              data_ptr->Draw(drawingOptions.data()); // z axis is only drawn if specified
              // reset the axis histogram which now owns the z axis, but keep default range
              // defined by the data
              double_t zMin = data_ptr->GetMinimum();
              double_t zMax = data_ptr->GetMaximum();
              data_ptr->Reset("ICE"); // reset integral, contents and errors
              data_ptr->SetMinimum(zMin);
              data_ptr->SetMaximum(zMax);
              isDrawn = true;
            }
          }
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>) {
            axisHist_ptr = data_ptr;
          } else {
            data_ptr->Draw();
            axisHist_ptr = data_ptr->GetHistogram();
          }
          if (!isDrawn) axisHist_ptr->Draw("AXIS");
          axisHist_ptr->Draw("SAME AXIG");
          axisHist_ptr->SetName(string("axis_hist_pad_" + std::to_string(padID)).data());

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
            auto textFontLabel = textFont;
            auto textColorTitle = textColor;
            auto textColorLabel = textColor;
            auto textSizeTitle = textSize;
            auto textSizeLabel = textSize;

            // first apply default pad values and then settings for this specific pad
            for (Plot::Pad& curPad : {std::ref(padDefaults), std::ref(plot.GetPads()[padID])}) {
              if (curPad.GetAxes().find(axisLabel) != curPad.GetAxes().end()) {
                auto& axisLayout = curPad[axisLabel];
                if (axisLayout.GetTitle()) axis_ptr->SetTitle((*axisLayout.GetTitle()).data());

                if (axisLayout.GetTitleFont()) textFontTitle = axisLayout.GetTitleFont();
                if (axisLayout.GetLabelFont()) textFontLabel = axisLayout.GetLabelFont();

                if (axisLayout.GetTitleColor()) textColorTitle = axisLayout.GetTitleColor();
                if (axisLayout.GetLabelColor()) textColorLabel = axisLayout.GetLabelColor();

                if (axisLayout.GetTitleSize()) textSizeTitle = axisLayout.GetTitleSize();
                if (axisLayout.GetLabelSize()) textSizeLabel = axisLayout.GetLabelSize();

                if (axisLayout.GetTitleCenter()) axis_ptr->CenterTitle(*axisLayout.GetTitleCenter());
                if (axisLayout.GetLabelCenter()) axis_ptr->CenterLabels(*axisLayout.GetLabelCenter());

                if (axisLayout.GetAxisColor()) axis_ptr->SetAxisColor(*axisLayout.GetAxisColor());

                if (axisLayout.GetTitleOffset()) axis_ptr->SetTitleOffset(*axisLayout.GetTitleOffset());
                if (axisLayout.GetLabelOffset()) axis_ptr->SetLabelOffset(*axisLayout.GetLabelOffset());

                if (axisLayout.GetTickLength()) axis_ptr->SetTickLength(*axisLayout.GetTickLength());
                if (axisLayout.GetMaxDigits()) axis_ptr->SetMaxDigits(*axisLayout.GetMaxDigits());

                if (axisLayout.GetNumDivisions()) axis_ptr->SetNdivisions(*axisLayout.GetNumDivisions());

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
                if (axisLayout.GetOppositeTicks()) {
                  if (axisLabel == 'X') {
                    pad_ptr->SetTickx(*axisLayout.GetOppositeTicks());
                  } else if (axisLabel == 'Y') {
                    pad_ptr->SetTicky(*axisLayout.GetOppositeTicks());
                  }
                }
                if (axisLayout.GetTimeFormat()) {
                  axis_ptr->SetTimeDisplay(1);
                  axis_ptr->SetTimeFormat((*axisLayout.GetTimeFormat()).data());
                }
                if (axisLayout.GetTickOrientation()) {
                  axis_ptr->SetTicks((*axisLayout.GetTickOrientation()).data());
                }

                if (axisLayout.GetMinRange() || axisLayout.GetMaxRange()) {
                  pad_ptr->Update(); // needed here so current user ranges correct
                  double_t curRangeMin = (axisLabel == 'X')
                                           ? pad_ptr->GetUxmin()
                                           : ((axisLabel == 'Y') ? pad_ptr->GetUymin() : axisHist_ptr->GetMinimum());
                  double_t curRangeMax = (axisLabel == 'X')
                                           ? pad_ptr->GetUxmax()
                                           : ((axisLabel == 'Y') ? pad_ptr->GetUymax() : axisHist_ptr->GetMaximum());

                  double_t rangeMin = (axisLayout.GetMinRange()) ? *axisLayout.GetMinRange() : curRangeMin;
                  double_t rangeMax = (axisLayout.GetMaxRange()) ? *axisLayout.GetMaxRange() : curRangeMax;

                  axis_ptr->SetRangeUser(rangeMin, rangeMax);
                }
              }
            }
            if (textFontTitle) axis_ptr->SetTitleFont(*textFontTitle);
            if (textFontLabel) axis_ptr->SetLabelFont(*textFontLabel);
            if (textColorTitle) axis_ptr->SetTitleColor(*textColorTitle);
            if (textColorLabel) axis_ptr->SetLabelColor(*textColorLabel);
            if (textSizeTitle) axis_ptr->SetTitleSize(*textSizeTitle);
            if (textSizeLabel) axis_ptr->SetLabelSize(*textSizeLabel);
          }

          // right after drawing the axis, put reference line if requested
          optional<string> refFunc = (pad.GetRefFunc()) ? pad.GetRefFunc() : padDefaults.GetRefFunc();
          if (refFunc) {
            TF1* line = new TF1("line", (*refFunc).data(), data_ptr->GetXaxis()->GetXmin(), data_ptr->GetXaxis()->GetXmax());
            line->SetLineColor(kBlack);
            line->SetLineWidth(2);
            // line->SetLineStyle(9);
            line->Draw("SAME");
          }
          pad_ptr->Update();
        } else {
          // define data appearance
          if (auto& markerColor = get_first(data->GetMarkerColor(),
                                            pick(dataIndex, pad.GetDefaultMarkerColors()),
                                            pick(dataIndex, padDefaults.GetDefaultMarkerColors()))) {
            data_ptr->SetMarkerColor(*markerColor);
          }
          if (auto& markerStyle = get_first(data->GetMarkerStyle(),
                                            pick(dataIndex, pad.GetDefaultMarkerStyles()),
                                            pick(dataIndex, padDefaults.GetDefaultMarkerStyles()))) {
            data_ptr->SetMarkerStyle(*markerStyle);
          }
          if (auto& markerSize = get_first(data->GetMarkerSize(),
                                           pad.GetDefaultMarkerSize(),
                                           padDefaults.GetDefaultMarkerSize())) {
            data_ptr->SetMarkerSize(*markerSize);
          }
          if (auto& lineColor = get_first(data->GetLineColor(),
                                          pick(dataIndex, pad.GetDefaultLineColors()),
                                          pick(dataIndex, padDefaults.GetDefaultLineColors()))) {
            data_ptr->SetLineColor(*lineColor);
          }
          if (auto& lineStyle = get_first(data->GetLineStyle(),
                                          pick(dataIndex, pad.GetDefaultLineStyles()),
                                          pick(dataIndex, padDefaults.GetDefaultLineStyles()))) {
            data_ptr->SetLineStyle(*lineStyle);
          }
          if (auto& lineWidth = get_first(data->GetLineWidth(),
                                          pad.GetDefaultLineWidth(),
                                          padDefaults.GetDefaultLineWidth())) {
            data_ptr->SetLineWidth(*lineWidth);
          }
          if (auto& fillColor = get_first(data->GetFillColor(),
                                          pick(dataIndex, pad.GetDefaultFillColors()),
                                          pick(dataIndex, padDefaults.GetDefaultFillColors()))) {
            data_ptr->SetFillColor(*fillColor);
          }
          if (auto& fillStyle = get_first(data->GetFillStyle(),
                                          pick(dataIndex, pad.GetDefaultFillStyles()),
                                          pick(dataIndex, padDefaults.GetDefaultFillStyles()))) {
            data_ptr->SetFillStyle(*fillStyle);
          }
          if (auto& fillOpacity = get_first(data->GetFillOpacity(),
                                            pad.GetDefaultFillOpacity(),
                                            padDefaults.GetDefaultFillOpacity())) {
            data_ptr->SetFillColor(TColor::GetColorTransparent(data_ptr->GetFillColor(), *fillOpacity));
          }

          // now define data ranges
          if (axisHist_ptr->GetMinimum()) {
            // TODO: check if this still works for bar histos
            data_ptr->SetMinimum(axisHist_ptr->GetMinimum()); // important for correct display of bar diagrams
          }
          // data_ptr->SetMaximum(axisHist_ptr->GetMaximum());

          double_t rangeMinX = (data->GetMinRangeX()) ? *data->GetMinRangeX()
                                                      : axisHist_ptr->GetXaxis()->GetXmin();
          double_t rangeMaxX = (data->GetMaxRangeX()) ? *data->GetMaxRangeX()
                                                      : axisHist_ptr->GetXaxis()->GetXmax();

          double_t rangeMinY = (data->GetMinRangeY()) ? *data->GetMinRangeY()
                                                      : axisHist_ptr->GetYaxis()->GetXmin();
          double_t rangeMaxY = (data->GetMaxRangeY()) ? *data->GetMaxRangeY()
                                                      : axisHist_ptr->GetYaxis()->GetXmax();

          if constexpr (std::is_convertible_v<data_type, data_ptr_t_func_2d>) {
            data_ptr->SetRange(rangeMinX, rangeMinY, rangeMaxX, rangeMaxY);
          } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_func>) {
            data_ptr->SetRange(rangeMinX, rangeMaxX);
          } else if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph>) {
            SetGraphRange((TGraph*)data_ptr, data->GetMinRangeX(), data->GetMaxRangeX());
          } else {
            data_ptr->GetXaxis()->SetRangeUser(rangeMinX, rangeMaxX);
          }
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>) {
            data_ptr->GetYaxis()->SetRangeUser(rangeMinY, rangeMaxY);
            // do not draw the Z axis a second time!
            std::replace(drawingOptions.begin(), drawingOptions.end(), 'Z', ' ');

            if (auto& contours = data->GetContours()) {
              data_ptr->SetContour((*contours).size(), (*contours).data());
            } else if (auto& nContours = data->GetNContours()) {
              data_ptr->SetContour(*nContours);
            }
          }
          if (data->GetTextFormat()) gStyle->SetPaintTextFormat((*data->GetTextFormat()).data());

          data_ptr->Draw(drawingOptions.data());

          // in case a label was specified for the data, add it to corresponding legend
          if (data->GetLegendLabel() && !(*data->GetLegendLabel()).empty()) {
            // by default place legend entries in first legend
            uint8_t legendID{1u};
            // explicit user choice overrides this
            if (data->GetLegendID()) legendID = *data->GetLegendID();

            auto& boxVector = pad.GetLegendBoxes();
            if (legendID > 0u && legendID <= boxVector.size()) {
              boxVector[legendID - 1]->AddEntry(*data->GetLegendLabel(), data_ptr->GetName());
            } else {
              ERROR(R"(Invalid legend label ({}) specified for data "{}" in "{}".)", legendID, data->GetName(), data->GetInputID());
            }
          }
          pad_ptr->Update(); // adds something to the list of primitives
        }
        ++dataIndex;
        drawingOptions = "SAME "; // next data should be drawn to same pad
      };

      optional<data_ptr_t> rawData = GetDataClone(dataBuffer.at(data->GetInputID()).at(data->GetName()).get(), data->GetProjInfo());
      if (rawData) {
        std::visit(processData, *rawData);
      } else {
        fail = true;
      }
    } // end data code

    if (fail) {
      return nullptr;
    }

    // now place legends, text-boxes and shapes
    uint8_t legendIndex{1u};
    for (auto& box : pad.GetLegendBoxes()) {
      string legendName = "LegendBox_" + std::to_string(legendIndex);
      box->MergeLegendEntries(); // apply individual user settings on top of automatic entries
      // apply default text properties of pad to the box
      if (!box->GetTextFont() && textFont) box->SetTextFont(*textFont);
      if (!box->GetTextSize() && textSize) box->SetTextSize(*textSize);
      if (!box->GetTextColor() && textColor) box->SetTextColor(*textColor);
      TPave* legend = GenerateBox(box, pad_ptr);
      if (legend) {
        legend->SetName(legendName.data());
        legend->Draw("SAME");
        ++legendIndex;
      } else {
        ERROR("Legend {} was not added since it is empty.", legendIndex);
      }
    }
    uint8_t textIndex{1u};
    for (auto& box : pad.GetTextBoxes()) {
      string textName = "TextBox_" + std::to_string(textIndex);
      // apply default text properties of pad to the box
      if (!box->GetTextFont() && textFont) box->SetTextFont(*textFont);
      if (!box->GetTextSize() && textSize) box->SetTextSize(*textSize);
      if (!box->GetTextColor() && textColor) box->SetTextColor(*textColor);
      TPave* text = GenerateBox(box, pad_ptr);
      if (text) {
        text->SetName(textName.data());
        text->Draw("SAME");
        ++textIndex;
      } else {
        ERROR("Text {} was not added since it is empty.", textIndex);
      }
    }

    bool redrawAxes = (pad.GetRedrawAxes())
                        ? *pad.GetRedrawAxes()
                        : ((padDefaults.GetRedrawAxes()) ? *padDefaults.GetRedrawAxes() : false);
    if (redrawAxes && axisHist_ptr) axisHist_ptr->Draw("SAME AXIS");

    pad_ptr->Modified();
    pad_ptr->Update();
  }

  canvas_ptr->cd();
  canvas_ptr->Modified();
  canvas_ptr->Update();
  return canvas_ptr;
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
    auto textFont{box->GetTextFont()};
    auto textSize{box->GetTextSize()};

    auto borderColor{box->GetBorderColor()};
    auto borderStyle{box->GetBorderStyle()};
    auto borderWidth{box->GetBorderWidth()};

    auto fillColor{box->GetFillColor()};
    auto fillStyle{box->GetFillStyle()};
    auto fillOpacity{box->GetFillOpacity()};

    vector<string> lines;
    if constexpr (isLegend) {
      std::for_each(box->GetEntries().begin(), box->GetEntries().end(),
                    [&lines](const auto& entry) {
                      if (entry.GetLabel()) lines.push_back(*entry.GetLabel());
                    });
    } else {
      // split text string to vector
      string delimiter{" // "};
      string text{box->GetText()};
      size_t pos{};
      size_t last{};
      std::string token;
      while ((pos = text.find(delimiter, last)) != std::string::npos) {
        token = text.substr(last, pos - last);
        lines.push_back(token);
        last = pos + delimiter.length();
      }
      lines.push_back(text.substr(last));
    }

    float_t text_size = (textSize) ? *textSize : 24.f;
    int16_t text_font = (textFont) ? *textFont : 43;
    uint8_t nColumns{1u};
    if constexpr (isLegend) {
      if (box->GetNumColumns()) nColumns = *box->GetNumColumns();
    }
    uint16_t nLines = lines.size();
    if (nLines < 1) return;

    int32_t padWidthPixel = pad->XtoPixel(pad->GetX2()); // looks correct, but why does it work??
    int32_t padHeightPixel = pad->YtoPixel(pad->GetY1());

    // determine max width and height of legend entries
    uint8_t iColumn{};
    double_t legendWidthPixel{};
    double_t titleWidthPixel{};
    vector<uint32_t> legendWidthPixelPerColumn(nColumns, 0);

    double_t lineHeightPixel{text_size};
    if (text_font % 10 <= 2) lineHeightPixel = GetTextSizePixel(text_size);

    uint8_t lineID{};
    for (auto& line : lines) {
      if constexpr (isLegend) {
        auto& entry = box->GetEntries()[lineID];
        if (entry.GetRefDataName()) {
          // FIXME: this gives always the first -> problem when drawing the same histogram twice!
          TNamed* data_ptr = (TNamed*)pad->FindObject((*entry.GetRefDataName()).data());
          if (!data_ptr) ERROR(R"(Object belonging to legend entry "{}" not found.)", line);
          ReplacePlaceholders(line, data_ptr);
        }
      }

      // determine width and height of line to find max width and height (per column)
      TLatex textLine(0, 0, line.data());
      textLine.SetTextFont(text_font);
      textLine.SetTextSize(text_size);
      auto [width, height] = GetTextDimensions(textLine);
      if (height > lineHeightPixel) lineHeightPixel = height;

      if (width > legendWidthPixelPerColumn[iColumn]) legendWidthPixelPerColumn[iColumn] = width;
      ++iColumn;
      iColumn %= nColumns;
      ++lineID;
    }
    for (auto& length : legendWidthPixelPerColumn)
      legendWidthPixel += length;

    uint32_t markerWidthPixel{};
    if constexpr (isLegend) {
      string markerDummyString = "-+-"; // defines width of marker
      TLatex markerDummy(0, 0, markerDummyString.data());
      markerDummy.SetTextFont(text_font);
      markerDummy.SetTextSize(text_size);
      auto [w, h] = GetTextDimensions(markerDummy);
      markerWidthPixel = w;
    }

    double_t legendWidthNDC = (double_t)legendWidthPixel / padWidthPixel;
    double_t lineHeightNDC = (double_t)lineHeightPixel / padHeightPixel;
    double_t markerWidthNDC = (double_t)markerWidthPixel / padWidthPixel;
    double_t titleWidthNDC = (double_t)titleWidthPixel / padWidthPixel;

    double_t totalWidthNDC{};
    double_t totalHeightNDC{};
    double_t marginNDC{0.01};
    double_t lineSpacing{0.3}; // fraction of line hight

    if constexpr (isLegend) {
      totalWidthNDC = 3 * marginNDC + markerWidthNDC + legendWidthNDC;
      totalHeightNDC = (nLines + lineSpacing * (nLines + 1)) * lineHeightNDC / nColumns;
    } else {
      totalWidthNDC = 2 * marginNDC + legendWidthNDC;
      totalHeightNDC = (nLines + 0.5 * (nLines - 1)) * lineHeightNDC;
    }

    if (titleWidthPixel > legendWidthPixel) {
      totalWidthNDC = (0.3333) * markerWidthNDC + titleWidthNDC;
    }

    double_t upperLeftX{box->GetXPosition()};
    double_t upperLeftY{box->GetYPosition()};

    if (box->IsAutoPlacement()) {
      pad->cd();
      pad->Update();
      double_t lowerLeftX{};
      double_t lowerLeftY{};
      // minimum distance of box to objects and ticks (in units of tick length)
      double_t fractionOfTickLength{0.9};
      double_t marginX = fractionOfTickLength * gStyle->GetTickLength("Y") * (pad->GetUxmax() - pad->GetUxmin()) / (pad->GetX2() - pad->GetX1());
      double_t marginY = fractionOfTickLength * gStyle->GetTickLength("X") * (pad->GetUymax() - pad->GetUymin()) / (pad->GetY2() - pad->GetY1());
      bool foundPosition = false;

      // draw temporary boxes to exclude areas outside of the coordinate system
      TBox marginsBottom(
        pad->GetX1(), pad->GetY1(), pad->GetX2(),
        pad->GetUymin() + gStyle->GetTickLength("X") * (pad->GetUymax() - pad->GetUymin()));
      TBox marginsTop(
        pad->GetX1(),
        pad->GetUymax() - gStyle->GetTickLength("X") * (pad->GetUymax() - pad->GetUymin()),
        pad->GetX2(), pad->GetY2());
      TBox marginsLeft(
        pad->GetX1(), pad->GetY1(),
        pad->GetUxmin() + gStyle->GetTickLength("Y") * (pad->GetUxmax() - pad->GetUxmin()),
        pad->GetY2());
      TBox marginsRight(
        pad->GetUxmax() - gStyle->GetTickLength("Y") * (pad->GetUxmax() - pad->GetUxmin()),
        pad->GetY1(), pad->GetX2(), pad->GetY2());
      marginsBottom.Draw("SAME");
      marginsTop.Draw("SAME");
      marginsLeft.Draw("SAME");
      marginsRight.Draw("SAME");

      // find box position that does not collide with any of the drawn objects
      for (TObject* o : *pad->GetListOfPrimitives()) {
        foundPosition = pad->PlaceBox(o, totalWidthNDC + 2 * marginX, totalHeightNDC + 2 * marginY, lowerLeftX, lowerLeftY);
      }
      if (foundPosition) {
        upperLeftX = lowerLeftX + 2 * marginX;
        upperLeftY = lowerLeftY + totalHeightNDC + 2 * marginY;
      } else {
        WARNING("Could not find enough space to place the {} properly.", (isLegend) ? "legend" : "text");
        // just place legend within axis ranges of pad
        upperLeftX = (pad->GetUxmin() - pad->GetX1()) / (pad->GetX2() - pad->GetX1()) + (1 + 1 / fractionOfTickLength) * marginX;
        upperLeftY = (pad->GetUymax() - pad->GetY1()) / (pad->GetY2() - pad->GetY1()) - (1 + 1 / fractionOfTickLength) * marginY;
      }
      // now remove temporary boxes again
      pad->GetListOfPrimitives()->Remove(&marginsBottom);
      pad->GetListOfPrimitives()->Remove(&marginsTop);
      pad->GetListOfPrimitives()->Remove(&marginsLeft);
      pad->GetListOfPrimitives()->Remove(&marginsRight);
      // pad->DrawCollideGrid();
    } else if (box->IsUserCoordinates()) {
      // convert user coordinates to NDC
      pad->Update();
      upperLeftX = (upperLeftX - pad->GetX1()) / (pad->GetX2() - pad->GetX1());
      upperLeftY = (upperLeftY - pad->GetY1()) / (pad->GetY2() - pad->GetY1());
    }

    if constexpr (isLegend) {
      TLegend* legend = new TLegend(upperLeftX, upperLeftY - totalHeightNDC,
                                    upperLeftX + totalWidthNDC, upperLeftY, "", "NDC");
      legend->SetMargin((2 * marginNDC + markerWidthNDC) / totalWidthNDC);
      legend->SetTextAlign(kHAlignLeft + kVAlignCenter);
      legend->SetNColumns(nColumns);

      legend->SetTextFont(text_font);
      legend->SetTextSize(text_size);

      if (textColor) legend->SetTextColor(*textColor);
      if (textSize) legend->SetTextSize(*textSize);
      if (textFont) legend->SetTextFont(*textFont);

      for (auto& entry : box->GetEntries()) {
        string label = entry.GetLabel() ? *entry.GetLabel() : ""; // FIXME: this is equal to lines[i]
        string drawStyle = entry.GetDrawStyle() ? *entry.GetDrawStyle() : "";

        // TLegendEntry* curEntry = legend->AddEntry((TObject*)nullptr, label.data(), drawStyle.data());
        TLegendEntry* curEntry = nullptr;
        if (entry.GetRefDataName()) {
          TNamed* data_ptr = (TNamed*)pad->FindObject((*entry.GetRefDataName()).data());
          // TODO: here we need to check that it exists

          if (drawStyle.empty()) {
            drawStyle = (box->GetDefaultDrawStyle()) ? *box->GetDefaultDrawStyle() : "EP";

            string drawingOption = data_ptr->GetDrawOption();
            std::for_each(drawingOption.begin(), drawingOption.end(),
                          [](char& c) { c = ::toupper(c); });

            if ((data_ptr->InheritsFrom("TF1")) || str_contains(drawingOption, "C") || str_contains(drawingOption, "L") || str_contains(drawingOption, "HIST")) {
              drawStyle = "L";
            } else if (data_ptr->InheritsFrom("TH1") && (str_contains(drawingOption, "HIST") || str_contains(drawingOption, "B")) && ((TH1*)data_ptr)->GetFillStyle() != 0) {
              drawStyle = "F";
            }
          }
          curEntry = legend->AddEntry((TObject*)data_ptr, label.data(), drawStyle.data());
          curEntry->SetObject((TObject*)nullptr);

          TAttMarker markerAttr;
          TAttLine lineAttr;
          TAttFill fillAttr;
          if (auto ptr = dynamic_cast<TAttMarker*>(data_ptr)) ptr->Copy(markerAttr);
          if (auto ptr = dynamic_cast<TAttLine*>(data_ptr)) ptr->Copy(lineAttr);
          if (auto ptr = dynamic_cast<TAttFill*>(data_ptr)) ptr->Copy(fillAttr);

          curEntry->SetMarkerColor(markerAttr.GetMarkerColor());
          curEntry->SetMarkerStyle(markerAttr.GetMarkerStyle());
          curEntry->SetMarkerSize(markerAttr.GetMarkerSize());

          if (box->GetDefaultLineColor()) {
            curEntry->SetLineColor(*box->GetDefaultLineColor());
          } else {
            curEntry->SetLineColor(lineAttr.GetLineColor());
          }
          if (box->GetDefaultLineStyle()) {
            curEntry->SetLineStyle(*box->GetDefaultLineStyle());
          } else {
            curEntry->SetLineStyle(lineAttr.GetLineStyle());
          }
          if (box->GetDefaultLineWidth()) {
            curEntry->SetLineWidth(*box->GetDefaultLineWidth());
          } else {
            curEntry->SetLineWidth(lineAttr.GetLineWidth());
          }
          curEntry->SetFillColor(fillAttr.GetFillColor());
          curEntry->SetFillStyle(fillAttr.GetFillStyle());
        } else {
          curEntry = legend->AddEntry((TObject*)nullptr, label.data(), drawStyle.data());
        }

        if (entry.GetMarkerColor()) curEntry->SetMarkerColor(*entry.GetMarkerColor());
        if (entry.GetMarkerStyle()) curEntry->SetMarkerStyle(*entry.GetMarkerStyle());
        if (entry.GetMarkerSize()) curEntry->SetMarkerSize(*entry.GetMarkerSize());

        if (entry.GetLineColor()) curEntry->SetLineColor(*entry.GetLineColor());
        if (entry.GetLineStyle()) curEntry->SetLineStyle(*entry.GetLineStyle());
        if (entry.GetLineWidth()) curEntry->SetLineWidth(*entry.GetLineWidth());

        if (entry.GetFillColor()) curEntry->SetFillColor(*entry.GetFillColor());
        if (entry.GetFillStyle()) curEntry->SetFillStyle(*entry.GetFillStyle());
        if (entry.GetFillOpacity() && entry.GetFillColor()) curEntry->SetFillColor(TColor::GetColorTransparent(*entry.GetFillColor(), *entry.GetFillOpacity()));

        if (entry.GetTextColor()) curEntry->SetTextColor(*entry.GetTextColor());
        if (entry.GetTextFont()) curEntry->SetTextFont(*entry.GetTextFont());
        if (entry.GetTextSize()) curEntry->SetTextSize(*entry.GetTextSize());
      }

      if (legend->GetHeader()) {
        ((TLegendEntry*)(legend->GetListOfPrimitives()->At(0)))->SetTextFont(text_font);
        ((TLegendEntry*)(legend->GetListOfPrimitives()->At(0)))->SetTextSize(text_size);
      }
      returnBox = (TPave*)legend;
    } else {
      TPaveText* paveText = new TPaveText(upperLeftX, upperLeftY - totalHeightNDC,
                                          upperLeftX + totalWidthNDC, upperLeftY, "NDC");
      paveText->SetMargin(marginNDC / totalWidthNDC);
      paveText->SetTextAlign(kHAlignLeft + kVAlignCenter);

      paveText->SetTextFont(text_font);
      paveText->SetTextSize(text_size);
      if (textColor) paveText->SetTextColor(*textColor);

      for (auto& line : lines) {
        TText* text = paveText->AddText(line.data());
        text->SetTextFont(text_font);
        text->SetTextSize(text_size);
      }
      returnBox = (TPave*)paveText;
    }

    if (returnBox) {
      if (borderStyle) returnBox->SetLineStyle(*borderStyle);
      if (borderColor) returnBox->SetLineColor(*borderColor);
      if (borderWidth) {
        returnBox->SetLineWidth(*borderWidth);
      } else {
        returnBox->SetLineWidth(0); // TODO: steer via pad defaults
      }
      if (fillStyle) {
        returnBox->SetFillStyle(*fillStyle);
      } else {
        returnBox->SetFillStyle(0); // TODO: steer via pad defaults
      }
      if (fillColor) returnBox->SetFillColor(*fillColor);
      if (fillOpacity && fillColor) returnBox->SetFillColor(TColor::GetColorTransparent(*fillColor, *fillOpacity));
    }
  };
  std::visit(processBox, boxVariant);
  return returnBox;
}

//**************************************************************************************************
/**
 * Functions to retrieve a copy or projection of the stored data properly casted it to its actual ROOT type.
 */
//**************************************************************************************************
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj, const std::optional<Plot::Pad::Data::proj_info_t>& projInfo)
{
  if (obj) {
    if (projInfo) {
      bool addDirStatus = TH1::AddDirectoryStatus();
      TH1::AddDirectory(false);
      string name = ((TNamed*)obj)->GetName();
      name += projInfo->GetNameSuffix();
      if (auto returnPointer = GetProjection(obj, *projInfo)) {
        std::visit([&name](auto&& ptr) { ptr->SetName(name.data()); }, *returnPointer);
        TH1::AddDirectory(addDirStatus);
        return returnPointer;
      } else {
        ERROR(R"(Projection failed for "{}".)", ((TNamed*)obj)->GetName());
      }
    } else {
      // TProfile2D is TH2, TH2 is TH1, TProfile is TH1
      if (auto returnPointer = GetDataClone<TProfile2D, TH2, TProfile, TH1, TGraph2D, TGraph, TF2, TF1>(obj)) {
        return returnPointer;
      } else {
        ERROR(R"(Input data "{}" is of unsupported type {}.)", ((TNamed*)obj)->GetName(), obj->ClassName());
      }
    }
  }
  return std::nullopt;
}

template <typename T>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if (obj && obj->InheritsFrom(T::Class())) {
    return (T*)obj->Clone();
  }
  return std::nullopt;
}

template <typename T, typename Next, typename... Rest>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if (auto returnPointer = GetDataClone<T>(obj)) return returnPointer;
  return GetDataClone<Next, Rest...>(obj);
}

optional<data_ptr_t> PlotPainter::GetProjection(TObject* obj, Plot::Pad::Data::proj_info_t projInfo)
{
  // only 1d and 2d histograms are valid outputs! (could be extended to 3d if there is a way to plot this)
  if (projInfo.dims.size() == 0 || projInfo.dims.size() > 2) {
    ERROR(R"(Invalid number of dimensions specified for projection of histogram "{}")", ((TNamed*)obj)->GetName());
    return std::nullopt;
  }

  if (obj->InheritsFrom(THnBase::Class())) {
    THnBase* histPtr = (THnBase*)obj;
    // first reset all ranges in case this histogram was previously used
    for (int16_t i = 0; i < histPtr->GetNdimensions(); ++i) {
      histPtr->GetAxis(i)->SetRange();
    }
    for (auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= histPtr->GetNdimensions()) {
        ERROR(R"(Invalid dimension specified for setting ranges of histogram "{}")", ((TNamed*)obj)->GetName());
        return std::nullopt;
      }
      int32_t minBin = (projInfo.isUserCoord) ? histPtr->GetAxis(rangeDim)->FindBin(std::get<1>(rangeTuple)) : static_cast<int>(std::get<1>(rangeTuple));
      int32_t maxBin = (projInfo.isUserCoord) ? histPtr->GetAxis(rangeDim)->FindBin(std::get<2>(rangeTuple)) : static_cast<int>(std::get<2>(rangeTuple));
      histPtr->GetAxis(rangeDim)->SetRange(minBin, maxBin);
    }
    if (projInfo.dims.size() == 2) {
      return histPtr->Projection(projInfo.dims[1], projInfo.dims[0]);
    } else if (projInfo.dims.size() == 1) {
      return histPtr->Projection(projInfo.dims[0]);
    }
  } else if (obj->InheritsFrom(TH3::Class())) {
    TH3* histPtr = (TH3*)obj;
    // first reset all ranges in case this histogram was previously used
    for (int16_t i = 0; i < 3; ++i) {
      GetAxis(histPtr, i)->SetRange();
    }
    for (auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= 3) {
        ERROR(R"(Invalid dimension specified for setting ranges of histogram "{}")", ((TNamed*)obj)->GetName());
        return std::nullopt;
      }
      int32_t minBin = (projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<1>(rangeTuple)) : static_cast<int>(std::get<1>(rangeTuple));
      int32_t maxBin = (projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<2>(rangeTuple)) : static_cast<int>(std::get<2>(rangeTuple));
      GetAxis(histPtr, rangeDim)->SetRange(minBin, maxBin);
    }
    if (projInfo.dims.size() == 2) {
      // get string if it is "xy" or "yx" or "zx"...
      return histPtr->Project3D((GetAxisStr(projInfo.dims[1]) + GetAxisStr(projInfo.dims[0])).data());
    } else if (projInfo.dims.size() == 1) {
      return histPtr->Project3D(GetAxisStr(projInfo.dims[0]).data());
    }
  } else if (obj->InheritsFrom(TH2::Class())) {
    TH2* histPtr = (TH2*)obj;
    if (projInfo.dims.size() > 1) {
      ERROR(R"(Invalid dimension specified for projecting histogram "{}")", ((TNamed*)obj)->GetName());
      return std::nullopt;
    }
    int32_t minBin = 0;
    int32_t maxBin = -1;

    for (auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= 2) {
        ERROR(R"(Invalid dimension specified for setting ranges of histogram "{}")", ((TNamed*)obj)->GetName());
        return std::nullopt;
      }
      minBin = (projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<1>(rangeTuple)) : static_cast<int>(std::get<1>(rangeTuple));
      maxBin = (projInfo.isUserCoord) ? GetAxis(histPtr, rangeDim)->FindBin(std::get<2>(rangeTuple)) : static_cast<int>(std::get<2>(rangeTuple));
    }
    if (projInfo.dims[0] == 0) {
      return histPtr->ProjectionX("_px", minBin, maxBin);
    } else if (projInfo.dims[0] == 1) {
      return histPtr->ProjectionY("_py", minBin, maxBin);
    } else {
      ERROR(R"(Invalid dimension specified for projection from "{}" ({}).)", ((TNamed*)obj)->GetName(), obj->ClassName());
    }
  } else {
    ERROR(R"(Cannot do projections for type {} ("{}").)", obj->ClassName(), ((TNamed*)obj)->GetName());
  }
  return std::nullopt;
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

std::string PlotPainter::GetAxisStr(int16_t i)
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
 * Helper-function dividing two TGraphs.
 * This is meant only for the rare use case, where the x values of all points are are exactly the same.
 * In this scenario the values and errors can be calculated exactly. If this condition is not met, the function will return false.
 */
//**************************************************************************************************
bool PlotPainter::DivideGraphs(TGraph* numerator, TGraph* denominator)
{
  // first check if graphs indeed have the same x values
  for (int32_t i = 0; i < numerator->GetN(); ++i) {
    if (numerator->GetX()[i] != denominator->GetX()[i]) return false;
  }
  // now divide
  for (int32_t i = 0; i < numerator->GetN(); ++i) {
    bool illegalDivision = false;
    if (denominator->GetY()[i] == 0.) {
      ERROR("Dividing by zero!");
      illegalDivision = true;
    }
    numerator->GetY()[i] = (illegalDivision) ? 0. : numerator->GetY()[i] / denominator->GetY()[i];
    numerator->GetEY()[i] = (illegalDivision)
                              ? 0.
                              : numerator->GetEY()[i] / denominator->GetY()[i] + denominator->GetEY()[i] * numerator->GetY()[i] / (denominator->GetY()[i] * denominator->GetY()[i]);
  }
  return true;
}

//**************************************************************************************************
/**
 * Helper-function dividing two TGraphs.
 * This is only a proxy for the ratio as it depends on an interpolation. Therefore also the uncertainties are not fully correct!
 */
//**************************************************************************************************
void PlotPainter::DivideGraphsInterpolated(TGraph* numerator, TGraph* denominator)
{
  TSpline3 denSpline("denSpline", denominator);

  int32_t nPoints = numerator->GetN();

  double_t* x = numerator->GetX();
  double_t* y = numerator->GetY();
  double_t* ey = numerator->GetEY();

  for (int32_t i = 0; i < nPoints; ++i) {
    double_t denomValue = denominator->Eval(x[i], &denSpline);
    double_t newValue = 0.;
    double_t newError = 0.;
    if (denomValue) {
      newValue = y[i] / denomValue;
      newError = ey[i] / denomValue; // Only proxy. Ignoring error of denominator!
    } else {
      ERROR("Dividing by zero!");
    }
    y[i] = newValue;
    ey[i] = newError;
  }
}

//**************************************************************************************************
/**
 * Helper-function dividing hist by graph.
 * This is only a proxy for the ratio as it depends on an interpolation. Therefore also the uncertainties are not fully correct!
 */
//**************************************************************************************************
void PlotPainter::DivideHistGraphInterpolated(TH1* numerator, TGraph* denominator)
{
  TGraph numeratorGraph(numerator);
  DivideGraphsInterpolated(&numeratorGraph, denominator);
  // FIXME: numerator graph must be put in hist afterwards!
}

//**************************************************************************************************
/**
 * Helper-function dividing graph by hist.
 * This is only a proxy for the ratio as it depends on an interpolation. Therefore also the uncertainties are not fully correct!
 */
//**************************************************************************************************
void PlotPainter::DivideGraphHistInterpolated(TGraph* numerator, TH1* denominator)
{
  TGraph denominatorGraph(denominator);
  DivideGraphsInterpolated(numerator, &denominatorGraph);
}

//**************************************************************************************************
/**
 * Helper-function dividing two 1d histograms with different binning.
 * This is only a proxy for the ratio as it depends on an interpolation. Therefore also the uncertainties are not fully correct!
 */
//**************************************************************************************************
void PlotPainter::DivideHistosInterpolated(TH1* numerator, TH1* denominator)
{
  TGraph denominatorGraph(denominator);
  TSpline3 denominatorSpline(denominator);

  for (int32_t i = 1; i <= numerator->GetNbinsX(); ++i) {
    double_t numeratorValue{numerator->GetBinContent(i)};
    double_t numeratorError{numerator->GetBinError(i)};
    double_t x{numerator->GetBinCenter(i)};
    double_t denomValue{denominatorGraph.Eval(x, &denominatorSpline)};
    double_t newValue{};
    double_t newError{};
    if (denomValue) {
      newValue = numeratorValue / denomValue;
      // uncertainty of denominator is not taken into account (cannot be extracted from spline)
      newError = numeratorError / denomValue;
    } else {
      // ERROR("Dividing by zero in bin {}!", i);
    }
    numerator->SetBinContent(i, newValue);
    numerator->SetBinError(i, newError);
  }
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

  int16_t pointsToRemoveHigh{};
  int16_t pointsToRemoveLow{};

  for (int16_t i = 0; i < graph->GetN(); ++i) {
    if (min && graph->GetX()[i] < *min) {
      ++pointsToRemoveLow;
    }
    if (max && graph->GetX()[i] >= *max) {
      ++pointsToRemoveHigh;
    }
  }

  for (int16_t i = 0; i < pointsToRemoveHigh; ++i) {
    graph->RemovePoint(graph->GetN() - 1);
  }
  for (int16_t i = 0; i < pointsToRemoveLow; ++i) {
    graph->RemovePoint(0);
  }
}

//**************************************************************************************************
/**
 * Scales graph by a constant value.
 */
//**************************************************************************************************
void PlotPainter::ScaleGraph(TGraph* graph, double_t scale)
{
  for (int32_t i{}; i < graph->GetN(); ++i) {
    graph->GetY()[i] *= scale;
    if (graph->GetEY()) graph->GetEY()[i] *= scale;
  }
}

//**************************************************************************************************
/**
 * Smoothes 1d graph in range.
 */
//**************************************************************************************************
void PlotPainter::SmoothGraph(TGraph* graph, optional<double_t> min, optional<double_t> max)
{
  TGraphSmooth smoother;
  TGraph* smoothGraph = smoother.SmoothSuper(graph);
  for (int32_t i{}; i < graph->GetN(); ++i) {
    double_t curX = graph->GetX()[i];
    if (min && curX < *min) continue;
    if (max && curX > *max) continue;
    graph->GetY()[i] = smoothGraph->GetY()[i];
  }
  delete smoothGraph;
}

//**************************************************************************************************
/**
 * Smoothes 1d hist in range.
 */
//**************************************************************************************************
void PlotPainter::SmoothHist(TH1* hist, optional<double_t> min, optional<double_t> max)
{
  double_t minRange = hist->GetXaxis()->GetXmin();
  double_t maxRange = hist->GetXaxis()->GetXmax();

  if (min && max)
    hist->GetXaxis()->SetRangeUser(*min, *max);
  else if (min && !max)
    hist->GetXaxis()->SetRangeUser(*min, maxRange);
  else if (!min && max)
    hist->GetXaxis()->SetRangeUser(minRange, *max);

  hist->Smooth(100, "R");
}

//**************************************************************************************************
/**
 * Returns actual dimensions in pixel of the text with latex formatting.
 */
//**************************************************************************************************
std::tuple<uint32_t, uint32_t> PlotPainter::GetTextDimensions(TLatex& text)
{
  uint32_t width{};
  uint32_t height{};
  int16_t font{text.GetTextFont()};

  if (font % 10 <= 2) {
    text.GetBoundingBox(width, height);
  } else {
    TLatex textBox{text};
    textBox.SetTextFont(font - 1);
    TVirtualPad* pad = gROOT->GetSelectedPad();
    double_t dy{pad->AbsPixeltoY(0) - pad->AbsPixeltoY((int32_t)(text.GetTextSize()))};
    double_t textSize{dy / (pad->GetY2() - pad->GetY1())};
    textBox.SetTextSize(textSize);
    textBox.GetBoundingBox(width, height);
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
  std::regex words_regex("<.*?>");
  auto words_begin = std::sregex_iterator(str.begin(), str.end(), words_regex);
  auto words_end = std::sregex_iterator();

  for (std::sregex_iterator match = words_begin; match != words_end; ++match) {
    std::string match_str = (*match).str();

    string format{};

    // check if user specified different formatting (e.g. via <mean[%2.6]>)
    std::regex format_regex("\\[.*?\\]");
    auto format_it = std::sregex_iterator(match_str.begin(), match_str.end(), format_regex);
    if (format_it != std::sregex_iterator()) {
      format = (*format_it).str();
      format = format.substr(1, format.size() - 2);
    }
    // allow printf style and protect against wrong usage
    format.erase(remove(format.begin(), format.end(), '%'), format.end());
    format.erase(remove(format.begin(), format.end(), ' '), format.end());

    // if no valid formatting pattern is given, fall back to 'general' mode
    if (!(str_contains(format, "e") || str_contains(format, "f") || str_contains(format, "g") || str_contains(format, "E") || str_contains(format, "F") || str_contains(format, "G"))) {
      format = format + "g";
    }
    format = "{:" + format + "}";

    string replace_str;
    if (str_contains(match_str, "name")) {
      replace_str = data_ptr->GetName();
      replace_str = replace_str.substr(0, replace_str.find(gNameGroupSeparator));
    } else if (str_contains(match_str, "title")) {
      replace_str = data_ptr->GetTitle();
    } else if (data_ptr->InheritsFrom(TH1::Class())) {
      try {
        if (str_contains(match_str, "entries")) {
          replace_str = fmt::format(format, ((TH1*)data_ptr)->GetEntries());
        } else if (str_contains(match_str, "integral")) {
          replace_str = fmt::format(format, ((TH1*)data_ptr)->Integral());
        } else if (str_contains(match_str, "mean")) {
          replace_str = fmt::format(format, ((TH1*)data_ptr)->GetMean());
        } else if (str_contains(match_str, "maximum")) {
          replace_str = fmt::format(format, ((TH1*)data_ptr)->GetMaximum());
        } else if (str_contains(match_str, "minimum")) {
          replace_str = fmt::format(format, ((TH1*)data_ptr)->GetMinimum());
        }
      } catch (...) {
        ERROR(R"(Incompatible format string in "{}".)", match_str);
        replace_str = match_str;
      }
    }
    str.replace(str.find(match_str), string(match_str).size(), replace_str);
  }
}

//**************************************************************************************************
/**
 * Helper to generate nColors between specified rgb endpoints.
 */
//**************************************************************************************************
vector<int16_t> PlotPainter::GenerateGradientColors(int32_t nColors, const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha, bool savePalette)
{
  uint16_t nPoints = rgbEndpoints.size();

  vector<double_t> red;
  vector<double_t> green;
  vector<double_t> blue;
  vector<double_t> stops;

  for (auto& rgb : rgbEndpoints) {
    red.push_back(std::get<0>(rgb));
    green.push_back(std::get<1>(rgb));
    blue.push_back(std::get<2>(rgb));
    stops.push_back(std::get<3>(rgb));
  }
  int16_t firstColorIndex = TColor::CreateGradientColorTable(nPoints, stops.data(), red.data(), green.data(), blue.data(), nColors, alpha);

  std::vector<int16_t> gradientColors(nColors);
  std::iota(gradientColors.begin(), gradientColors.end(), firstColorIndex);

  // TColor::CreateGradientColorTable() changes current palette as side effect
  if (!savePalette) gStyle->SetPalette(kBird);
  return gradientColors;
}

} // end namespace PlottingFramework
