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

#include "PlotPainter.h"
#include "PlotManager.h"

using namespace PlottingFramework;
namespace PlottingFramework {

//****************************************************************************************
/**
 * Function to generate the plot.
 */
//****************************************************************************************
shared_ptr<TCanvas> PlotPainter::GeneratePlot(Plot& plot, TObjArray* availableData){

  gStyle->SetOptStat(0); // this needs to be done before creating the canvas! at later stage it would add to list of primitives in pad...

  if(!(plot.GetWidth() || plot.GetHeight()))
  {
    ERROR("No dimensions specified for plot.");
    return nullptr;
  }
  
  TCanvas* canvas_ptr = new TCanvas(plot.GetUniqueName().c_str(), plot.GetUniqueName().c_str(), *plot.GetWidth()+4, *plot.GetHeight()+28);
  // NB.: +4 and +28 are needed to undo hard-coded offsets in TCanvas.cxx line 580
  canvas_ptr->SetMargin(0., 0., 0., 0.);

  // apply user settings for plot
  if(plot.GetFillColor())canvas_ptr->SetFillColor(*plot.GetFillColor());
  if(plot.GetFillStyle())canvas_ptr->SetFillStyle(*plot.GetFillStyle());
  if(plot.IsFixAspectRatio() && *plot.IsFixAspectRatio()) canvas_ptr->SetFixedAspectRatio();
  
  Plot::Pad& defaultPad = plot[0];
  for(auto& [padID, pad]: plot.GetPads())
  {
    if(padID == 0) continue; // pad 0 is used only to define the defaults

    string padOptions = (pad.GetOptions()) ? *pad.GetOptions() : "";
    
    vector<string> errorStyles;
    vector<string> lables;
    TObjArray legendEntries(1);
    
    // Pad placing
    array<double_t, 4> padPos = {0., 0., 1., 1.};
    if(! (pad.GetXLow() || pad.GetYLow() || pad.GetXUp() || pad.GetYUp()) || (*pad.GetXLow() > *pad.GetXUp()) || (*pad.GetYLow() > *pad.GetYUp()) || (*pad.GetXLow() < 0) || (*pad.GetXLow() > 1) || (*pad.GetXUp() < 0) || (*pad.GetXUp() > 1) || (*pad.GetYLow() < 0) || (*pad.GetYLow() > 1) || (*pad.GetYUp() < 0) || (*pad.GetYUp() > 1)
       )
    {
      WARNING("Position of pad {} was not defined properly! Drawing it over whole plot.", padID);
    }else{
      padPos = {*pad.GetXLow(), *pad.GetYLow(), *pad.GetXUp(), *pad.GetYUp()};
    }

    // get the settings for this pad
    optional<float_t> marginTop = (pad.GetMarginTop()) ? pad.GetMarginTop() : defaultPad.GetMarginTop();
    optional<float_t> marginBottom = (pad.GetMarginBottom()) ? pad.GetMarginBottom() : defaultPad.GetMarginBottom();
    optional<float_t> marginLeft = (pad.GetMarginLeft()) ? pad.GetMarginLeft() : defaultPad.GetMarginLeft();
    optional<float_t> marginRight = (pad.GetMarginRight()) ? pad.GetMarginRight() : defaultPad.GetMarginRight();
    optional<int32_t> palette = (pad.GetPalette()) ? pad.GetPalette() : defaultPad.GetPalette();
    optional<int16_t> padFillColor = (pad.GetFillColor()) ? pad.GetFillColor() : defaultPad.GetFillColor();
    optional<int16_t> padFillStyle = (pad.GetFillStyle()) ? pad.GetFillStyle() : defaultPad.GetFillStyle();
    optional<int16_t> frameFillColor = (pad.GetFillColorFrame()) ? pad.GetFillColorFrame() : defaultPad.GetFillColorFrame();
    optional<int16_t> frameFillStyle = (pad.GetFillStyleFrame()) ? pad.GetFillStyleFrame() : defaultPad.GetFillStyleFrame();
    optional<int16_t> frameLineColor = (pad.GetLineColorFrame()) ? pad.GetLineColorFrame() : defaultPad.GetLineColorFrame();
    optional<int16_t> frameLineStyle = (pad.GetLineStyleFrame()) ? pad.GetLineStyleFrame() : defaultPad.GetLineStyleFrame();
    optional<float_t> frameLineWidth = (pad.GetLineWidthFrame()) ? pad.GetLineWidthFrame() : defaultPad.GetLineWidthFrame();

    optional<int16_t> textFont = (pad.GetDefaultTextFont()) ? pad.GetDefaultTextFont() : defaultPad.GetDefaultTextFont();
    optional<float_t> textSize = (pad.GetDefaultTextSize()) ? pad.GetDefaultTextSize() : defaultPad.GetDefaultTextSize();
    optional<int16_t> textColor = (pad.GetDefaultTextColor()) ? pad.GetDefaultTextColor() : defaultPad.GetDefaultTextColor();

    optional<float_t> defaultMarkerSize = (pad.GetDefaultMarkerSize()) ? pad.GetDefaultMarkerSize() : defaultPad.GetDefaultMarkerSize();
    optional<float_t> defaultLineWidth = (pad.GetDefaultLineWidth()) ? pad.GetDefaultLineWidth() : defaultPad.GetDefaultLineWidth();

    
    string padTitle = (pad.GetTitle()) ? *pad.GetTitle() : (defaultPad.GetTitle()) ? *defaultPad.GetTitle() : "";
    
    canvas_ptr->cd();
    string padName = "Pad_" + std::to_string(padID);

    TPad* pad_ptr = new TPad(padName.c_str(), padTitle.c_str(), padPos[0], padPos[1], padPos[2], padPos[3]);
    if(marginTop) pad_ptr->SetTopMargin(*marginTop);
    if(marginBottom) pad_ptr->SetBottomMargin(*marginBottom);
    if(marginLeft) pad_ptr->SetLeftMargin(*marginLeft);
    if(marginRight) pad_ptr->SetRightMargin(*marginRight);
    if(padFillColor) pad_ptr->SetFillColor(*padFillColor);
    if(padFillStyle) pad_ptr->SetFillStyle(*padFillStyle);
    if(frameFillColor) pad_ptr->SetFrameFillColor(*frameFillColor);
    if(frameFillStyle) pad_ptr->SetFrameFillStyle(*frameFillStyle);
    if(frameLineColor) pad_ptr->SetFrameLineColor(*frameLineColor);
    if(frameLineStyle) pad_ptr->SetFrameLineStyle(*frameLineStyle);
    if(frameLineWidth) pad_ptr->SetFrameLineWidth(*frameLineWidth);
    if(palette) gStyle->SetPalette(*palette);

    pad_ptr->SetNumber(padID);
    pad_ptr->Draw();
    pad_ptr->cd();

    gStyle->SetTitleW((pad_ptr->GetX2() - pad_ptr->GetRightMargin()) - (pad_ptr->GetX1() + pad_ptr->GetLeftMargin()));
    gStyle->SetTitleH(pad_ptr->GetTopMargin()*0.8);
    gStyle->SetTitleAlign(kHAlignCenter + kVAlignTop);
    //TGaxis::SetMaxDigits(3);
    //gStyle->SetTextFont(plotStyle.GetTextFont());

    
    if (pad.GetData().empty())
    {
      WARNING("No data to be drawn in pad {}.", padID);
      continue;
    }
    // in case user did not specify which data should define the axis frame, use first per default
    if(pad.GetData()[0]->GetDrawingOptions() != "AXIS") // FIXME: this has to be handled differently!
    {
      // make a copy of first data that will serve as axis frame -- this way global ranges can be set independent from first data
      if(pad.GetData()[0]->GetType() == "ratio"){
        pad.GetData().insert(pad.GetData().begin(), std::make_shared<Plot::Pad::Ratio>(*std::dynamic_pointer_cast<Plot::Pad::Ratio>(pad.GetData()[0])));
      }
      else{
        pad.GetData().insert(pad.GetData().begin(), std::make_shared<Plot::Pad::Data>(*pad.GetData()[0]));
      }
      pad.GetData()[0]->SetLable(""); // axis frame should not appear in legend
    }

    TH1* axisHist_ptr = nullptr;
    bool drawLine = false;
    string drawingOptions = "";
    int dataIndex = 0;
    for(auto& data : pad.GetData())
    {
      drawingOptions += data->GetDrawingOptions();
      
      // obtain a copy of the current data
      if(optional<data_ptr_t> rawData = GetDataClone(data->GetUniqueName(), availableData))
      {
        // retrieve the actual pointer to the data
        std::visit([&, padID = padID](auto&& data_ptr)
        {
          using data_type = std::decay_t<decltype(data_ptr)>;
                    
          optional<drawing_options_t> defaultDrawingOption = data->GetDrawingOptionAlias();

          if(data->GetDrawingOptions() == "" && defaultDrawingOption)
          {
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>)
            {
              if(defaultDrawingOpions_Hist.find(*defaultDrawingOption) != defaultDrawingOpions_Hist.end())
              {
                drawingOptions += defaultDrawingOpions_Hist.at(*defaultDrawingOption);
              }else{
                WARNING("Default drawing opiton not available for type {}.", data_ptr->ClassName());
              }
            }
            else if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph_1d>)
            {
                if(defaultDrawingOpions_Graph.find(*defaultDrawingOption) != defaultDrawingOpions_Graph.end())
                {
                  drawingOptions += defaultDrawingOpions_Graph.at(*defaultDrawingOption);
                }else{
                  WARNING("Default drawing opiton not available for type {}.", data_ptr->ClassName());
                }
            }

          }

          if(data->GetType() == "ratio")
          {
            // obtain a copy of the ratio denominator
            if(optional<data_ptr_t> rawDenomData = GetDataClone(std::dynamic_pointer_cast<Plot::Pad::Ratio>(data)->GetUniqueNameDenom(), availableData))
            {
              // retrieve the actual pointer to the denominator data
              std::visit([&](auto&& denom_data_ptr)
              {
                using denom_data_type = std::decay_t<decltype(denom_data_ptr)>;
                if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>)
                  if constexpr (std::is_convertible_v<denom_data_type, data_ptr_t_hist>)
                {
                  data_ptr->Divide(denom_data_ptr);
                  if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>)
                    data_ptr->GetYaxis()->SetTitle("ratio");
                  if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>)
                    data_ptr->GetZaxis()->SetTitle("ratio");
                }
                delete denom_data_ptr;
               }, *rawDenomData);
            }
          } // end ratio code

          // modify content (FIXME: this should be steered differently)
          // TODO: add functionality for scaling and normalizing graphs
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>)
          {
            if(str_contains(drawingOptions,"smooth"))
            {
              drawingOptions.erase(drawingOptions.find("smooth"), string("smooth").length());
              data_ptr->Smooth();
            }
          }
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>)
          {
            if(drawingOptions.find("normalize") != string::npos){ // FIXME: should this only be normalized in range?
              drawingOptions.erase(drawingOptions.find("normalize"), string("normalize").length());
              data_ptr->Scale(1/data_ptr->Integral(), "width");
            }
            if(data->GetScaleFactor()){
              data_ptr->Scale(*data->GetScaleFactor());
            }
          }
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph_1d>)
          {
            if(data->GetScaleFactor()){
              ScaleGraph((TGraph*)data_ptr, *data->GetScaleFactor());
            }
          }

          // first data is only used to define the axes
          if(dataIndex == 0){
            data_ptr->Draw();
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>)
            {
              axisHist_ptr = data_ptr;
              axisHist_ptr->SetMarkerStyle(-1);
              axisHist_ptr->SetLineWidth(0.);
            }else{
              data_ptr->SetMarkerStyle(-1);
              data_ptr->SetLineWidth(0.);
              axisHist_ptr = data_ptr->GetHistogram();
            }
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>)
            {
              data_ptr->Draw(drawingOptions.c_str()); // z axis is only drawn if specified
              axisHist_ptr->Reset("ICE"); //reset only integral, contents and errors
            }
            axisHist_ptr->Draw("SAME AXIG");
            axisHist_ptr->SetTitle(padTitle.c_str());
            axisHist_ptr->SetName(string("axis_hist_pad_" + std::to_string(padID)).c_str());
            
            
            // apply axis settings
            for(string axisLable : {"X", "Y", "Z"})
            {
              TAxis* axis_ptr = nullptr;
              if(axisLable == "X")
                axis_ptr = axisHist_ptr->GetXaxis();
              else if(axisLable == "Y")
                axis_ptr = axisHist_ptr->GetYaxis();
              else if(axisLable == "Z")
                axis_ptr = axisHist_ptr->GetZaxis();
              if(!axis_ptr) continue;


              optional<int16_t> textFontTitle = textFont;
              optional<int16_t> textFontLable = textFont;
              optional<int16_t> textColorTitle = textColor;
              optional<int16_t> textColorLable = textColor;
              optional<float_t> textSizeTitle = textSize;
              optional<float_t> textSizeLable = textSize;
              
              // first apply default pad values and then settings for this specific pad
              for (Plot::Pad& curPad : {std::ref(defaultPad), std::ref(plot.GetPads()[padID])})
              {
                if(curPad.GetAxes().find(axisLable) != curPad.GetAxes().end())
                {
                  auto axisLayout = curPad[axisLable];
                  if(axisLayout.GetTitle()) axis_ptr->SetTitle((*axisLayout.GetTitle()).c_str());

                  if(axisLayout.GetTitleFont()) textFontTitle = axisLayout.GetTitleFont();
                  if(axisLayout.GetLableFont()) textFontLable = axisLayout.GetLableFont();

                  if(axisLayout.GetTitleColor()) textColorTitle = axisLayout.GetTitleColor();
                  if(axisLayout.GetLableColor()) textColorLable = axisLayout.GetLableColor();

                  if(axisLayout.GetTitleSize()) textSizeTitle = axisLayout.GetTitleSize();
                  if(axisLayout.GetLableSize()) textSizeLable = axisLayout.GetLableSize();

                  if(axisLayout.GetTitleCenter()) axis_ptr->CenterTitle(*axisLayout.GetTitleCenter());
                  if(axisLayout.GetLableCenter()) axis_ptr->CenterLabels(*axisLayout.GetLableCenter());

                  
                  if(axisLayout.GetAxisColor()) axis_ptr->SetAxisColor(*axisLayout.GetAxisColor());
                  
                  if(axisLayout.GetTitleOffset()) axis_ptr->SetTitleOffset(*axisLayout.GetTitleOffset());
                  if(axisLayout.GetLableOffset()) axis_ptr->SetLabelOffset(*axisLayout.GetLableOffset());

                  if(axisLayout.GetTickLength()) axis_ptr->SetTickLength(*axisLayout.GetTickLength());
                  if(axisLayout.GetMaxDigits()) axis_ptr->SetMaxDigits(*axisLayout.GetMaxDigits());
                  
                  if(axisLayout.GetNumDivisions()) axis_ptr->SetNdivisions(*axisLayout.GetNumDivisions());

                  if(axisLayout.GetLog())
                  {
                    if(axisLable == "X") pad_ptr->SetLogx(*axisLayout.GetLog());
                    else if(axisLable == "Y") pad_ptr->SetLogy(*axisLayout.GetLog());
                    else if(axisLable == "Z") pad_ptr->SetLogz(*axisLayout.GetLog());
                  }
                  if(axisLayout.GetGrid())
                  {
                    if(axisLable == "X") pad_ptr->SetGridx(*axisLayout.GetGrid());
                    else if(axisLable == "Y") pad_ptr->SetGridy(*axisLayout.GetGrid());
                  }

                  if(axisLayout.GetMinRange() || axisLayout.GetMaxRange())
                  {
                    pad_ptr->Update(); // needed here so current user ranges correct
                    double_t curRangeMin = (axisLable == "X") ? pad_ptr->GetUxmin() : ((axisLable == "Y") ? pad_ptr->GetUymin() : axisHist_ptr->GetMinimum());
                    double_t curRangeMax = (axisLable == "X") ? pad_ptr->GetUxmax() : ((axisLable == "Y") ? pad_ptr->GetUymax() : axisHist_ptr->GetMaximum());

                    double_t rangeMin = (axisLayout.GetMinRange()) ? *axisLayout.GetMinRange() : curRangeMin;
                    double_t rangeMax = (axisLayout.GetMaxRange()) ? *axisLayout.GetMaxRange() : curRangeMax;

                    axis_ptr->SetRangeUser(rangeMin, rangeMax);
                  }
                  
                }

              }
              if(textFontTitle)   axis_ptr->SetTitleFont(*textFontTitle);
              if(textFontLable)   axis_ptr->SetLabelFont(*textFontLable);
              if(textColorTitle)  axis_ptr->SetTitleColor(*textColorTitle);
              if(textColorLable)  axis_ptr->SetLabelColor(*textColorLable);
              if(textSizeTitle)   axis_ptr->SetTitleSize(*textSizeTitle);
              if(textSizeLable)   axis_ptr->SetLabelSize(*textSizeLable);
            }

            
            // right after drawing the axis, put reference line if requested
            optional<string> refFunc = (plot[padID].GetRefFunc()) ? plot[padID].GetRefFunc() : plot[0].GetRefFunc();
            if(refFunc)
            {
              TF1* line = new TF1("line", (*refFunc).c_str(), data_ptr->GetXaxis()->GetXmin(), data_ptr->GetXaxis()->GetXmax());
              line->SetLineColor(kBlack);
              line->SetLineWidth(2);
              // line->SetLineStyle(9);
              line->Draw("SAME");
            }
            pad_ptr->Update();
          }
          else
          {
            // define data appearance
            optional<int16_t> markerColor   = (data->GetMarkerColor())  ? data->GetMarkerColor()  : std::nullopt;
            optional<int16_t> markerStyle   = (data->GetMarkerStyle())  ? data->GetMarkerStyle()  : std::nullopt;
            optional<float_t> markerSize    = (data->GetMarkerSize())   ? data->GetMarkerSize()   : std::nullopt;
            optional<int16_t> lineColor     = (data->GetLineColor())    ? data->GetLineColor()    : std::nullopt;
            optional<int16_t> lineStyle     = (data->GetLineStyle())    ? data->GetLineStyle()    : std::nullopt;
            optional<float_t> lineWidth     = (data->GetLineWidth())    ? data->GetLineWidth()    : std::nullopt;
            optional<int16_t> fillColor     = (data->GetFillColor())    ? data->GetFillColor()    : std::nullopt;
            optional<int16_t> fillStyle     = (data->GetFillStyle())    ? data->GetFillStyle()    : std::nullopt;
            optional<float_t> fillOpacity   = (data->GetFillOpacity())  ? data->GetFillOpacity()  : std::nullopt;
            //0 : hollow, 1001 : Solid, 3000+pattern_number : pattern
            
            //if(!markerColor) markerColor = GetDefaultColor(dataIndex);
            //if(!lineColor) lineColor = GetDefaultColor(dataIndex);
            //if(!markerStyle) markerStyle = pad.GetDefaultMarkers(0) : plot[0].GetDefaultMarkerSize(0) : GetDefaultMarker(0);
            
            if(markerStyle) data_ptr->SetMarkerStyle(*markerStyle);
            if(markerColor) data_ptr->SetMarkerColor(*markerColor);
            if(markerSize) data_ptr->SetMarkerSize(*markerSize);

            if(lineStyle) data_ptr->SetLineStyle(*lineStyle);
            if(lineColor) data_ptr->SetLineColor(*lineColor);
            if(lineWidth) data_ptr->SetLineWidth(*lineWidth);

            if(fillStyle) data_ptr->SetFillStyle(*fillStyle);
            if(fillOpacity && fillColor && dataIndex != 0) *fillColor = TColor::GetColorTransparent(*fillColor, *fillOpacity);
            if(fillColor) data_ptr->SetFillColor(*fillColor);

            // now define data ranges
            data_ptr->SetMinimum(axisHist_ptr->GetMinimum()); // important for correct display of bar diagrams
            //data_ptr->SetMaximum(axisHist_ptr->GetMaximum());

            double_t rangeMinX = (data->GetMinRangeX()) ? *data->GetMinRangeX() : axisHist_ptr->GetXaxis()->GetXmin();
            double_t rangeMaxX = (data->GetMaxRangeX()) ? *data->GetMaxRangeX() : axisHist_ptr->GetXaxis()->GetXmax();

            double_t rangeMinY = (data->GetMinRangeY()) ? *data->GetMinRangeY() : axisHist_ptr->GetYaxis()->GetXmin();
            double_t rangeMaxY = (data->GetMaxRangeY()) ? *data->GetMaxRangeY() : axisHist_ptr->GetYaxis()->GetXmax();
            
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_func_2d>)
            {
              data_ptr->SetRange(rangeMinX, rangeMinY, rangeMaxX, rangeMaxY);
            }
            else if constexpr (std::is_convertible_v<data_type, data_ptr_t_func>)
            {
              data_ptr->SetRange(rangeMinX, rangeMaxX);
            }
            else if constexpr (std::is_convertible_v<data_type, data_ptr_t_graph>)
            {
              SetGraphRange((TGraph*)data_ptr, data->GetMinRangeX(), data->GetMaxRangeX());
            }
            else
            {
              data_ptr->GetXaxis()->SetRangeUser(rangeMinX, rangeMaxX);
            }
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>)
            {
              data_ptr->GetYaxis()->SetRangeUser(rangeMinY, rangeMaxY);
              // do not draw the Z axis a second time!
              std::replace(drawingOptions.begin(),drawingOptions.end(), 'Z', ' ');
            }

            //DEBUG("Drawing {} ({}) in pad {} with option \"{}\"", data_ptr->GetName(), data_ptr->ClassName(), padID, drawingOptions);
            data_ptr->Draw(drawingOptions.c_str());
            
            // in case a lable was specified for the data, remember this for later
            if (!data->GetLable().empty()) {
              lables.push_back(data->GetLable());
              legendEntries.Add(pad_ptr->GetListOfPrimitives()->Last());
              errorStyles.push_back(drawingOptions);
            }
            pad_ptr->Update(); // adds something to the list of primitives
          }
          dataIndex++;
          drawingOptions = "SAME "; // next data should be drawn to same pad
        }, *rawData);
      }
    } // end data code
    
    // draw text boxes
    uint8_t legendIndex = 1;
    uint8_t textIndex = 1;
    // now place legends, textboxes and shapes
    for(auto& box : pad.GetBoxes())
    {
      if(box->GetType() == "legend")
      {
        string legendName = "LegendBox_" + std::to_string(legendIndex);
        if(lables.empty()) break;
        TLegend* legend = MakeLegend(std::static_pointer_cast<Plot::Pad::LegendBox>(box), pad_ptr, legendEntries, lables, errorStyles);
        legend->SetName(legendName.c_str());
        legend->Draw("SAME");
        legendIndex++;
      }
      else if(box->GetType() == "text")
      {
        string textName = "TextBox_" + std::to_string(textIndex);
        TPaveText* text = MakeText(std::static_pointer_cast<Plot::Pad::TextBox>(box));
        text->SetName(textName.c_str());
        text->Draw("SAME");
        textIndex++;
      }
    }

    bool redrawAxes = (pad.GetRedrawAxes()) ? *pad.GetRedrawAxes() : ((defaultPad.GetRedrawAxes()) ? *defaultPad.GetRedrawAxes() : false);
    if(redrawAxes) axisHist_ptr->Draw("SAME AXIS");

    pad_ptr->Modified();
    pad_ptr->Update();
  }
  
  canvas_ptr->cd();
  canvas_ptr->Modified();
  canvas_ptr->Update();
  /*
  if(plot.IsDrawTimestamps()){
    TTimeStamp time;
    TString stTime;
    stTime = Form("#color[16]{%i}", time.GetDate()); // TODO: make format user definable
    TLatex timeStamp;
    timeStamp.SetNDC();
    timeStamp.SetTextAlign(22);
    timeStamp.SetTextFont(63);
    timeStamp.SetTextSizePixels(10);
    timeStamp.DrawLatex(plotStyle.GetTimestampPositon().first, plotStyle.GetTimestampPositon().second, stTime);
  }
  
  // patch truncated lowest lable
  //TPad* patch = new TPad("patch", "patch", mainPad->GetLeftMargin()/2 ,0, mainPad->GetLeftMargin()-0.004, mStyle.textSize / mainPad->YtoPixel(mainPad->GetY1()));
  //patch->Draw("SAME");
  //patch->SetFillColor(mainPad->GetFillColor());
  //patch->SetBorderMode(0);
  
  // for TView:
  //    canvas_ptr->SetTheta(20);
  //    canvas_ptr->SetPhi(45);
   
   */
  return shared_ptr<TCanvas>(canvas_ptr);
}


//****************************************************************************************
/**
 * Function to retrieve a copy of the stored data properly casted it to its actual ROOT type.
 */
//****************************************************************************************
template <typename T>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if(obj->InheritsFrom(T::Class()))
  {
    return (T*)obj->Clone();
  }
  return std::nullopt;
}
template <typename T, typename Next, typename... Rest>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if(auto returnPointer = GetDataClone<T>(obj))
    return returnPointer;
  return GetDataClone<Next, Rest...>(obj);
}
optional<data_ptr_t> PlotPainter::GetDataClone(string dataName, TObjArray* availableData)
{
  TObject* obj = availableData->FindObject(dataName.c_str());
  if(obj)
  {
    // IMPORTANT: TProfile2D is TH2, TH2 is TH1, TProfile is TH1 --> order matters here!
    if(auto returnPointer = GetDataClone<TProfile2D, TH2, TProfile, TH1, TGraph2D, TGraph, TF2, TF1>(obj))
    {
      return returnPointer;
    }
    else
    {
      ERROR("Input data \"{}\" is of unsupported type {}.", dataName, obj->ClassName());
    }
  }else{
    ERROR("Input data \"{}\" was not loaded.", dataName);
  }
  return std::nullopt;
}

//****************************************************************************************
/**
 * Function to generate the pad legend.
 */
//****************************************************************************************
TLegend* PlotPainter::MakeLegend(shared_ptr<Plot::Pad::LegendBox> legendBox, TPad* pad, TObjArray& legendEntries, vector<string> legendTitles, vector<string>& errorStyles){
  
  // todo this has to be included in legend box...
  double textSizePixel = 24;  //legendBox->GetTextSize;
  int16_t textFont = 43; //legendBox->GetTextFont;
  
  int nColumns = legendBox->GetNumColumns();
  
  string dummyString = "AAA";
  int nEntries = legendEntries.GetEntries();
  if(!legendBox->GetTitle().empty()) nEntries++;
  
  int padWidthPixel = pad->XtoPixel(pad->GetX2()); // todo this is probably incorrect
  int padHeightPixel = pad->YtoPixel(pad->GetY1());
  
  
  // determine max width and height of legend entries
  int iColumn = 0;
  double legendWidthPixel = 0;
  double titleWidthPixel = 0;
  vector<unsigned int> legendWidthPixelPerColumn(nColumns, 0);
  double legendHeightPixel = 0;
  if(!legendBox->GetTitle().empty()) legendTitles.push_back(legendBox->GetTitle());
  int iLegend = 1;
  for(auto& legendTitle : legendTitles)
  {
    if(legendTitle.find("<name>") != string::npos)
    {
      string name = ((TNamed*)legendEntries[iLegend-1])->GetName();
      name = name.substr(0, name.find(gNameGroupSeparator));
      
      legendTitle.replace(legendTitle.find("<name>"), string("<name>").size(), name);
    }
    if(legendTitle.find("<title>") != string::npos)
    {
      string title = ((TNamed*)legendEntries[iLegend-1])->GetTitle();
      legendTitle.replace(legendTitle.find("<title>"), string("<title>").size(), title);
    }
    if(legendTitle.find("<entries>") != string::npos && legendEntries[iLegend-1]->InheritsFrom(TH1::Class()))
    {
      // todo add some formating options for this
      string entries = std::to_string((long int)((TH1*)legendEntries[iLegend-1])->GetEntries());
      legendTitle.replace(legendTitle.find("<entries>"), string("<entries>").size(), entries);
    }
    if(legendTitle.find("<integral>") != string::npos && legendEntries[iLegend-1]->InheritsFrom(TH1::Class()))
    {
      // todo add some formating options for this
      string integral = std::to_string(((TH1*)legendEntries[iLegend-1])->Integral());
      legendTitle.replace(legendTitle.find("<integral>"), string("<integral>").size(), integral);
    }
    if(legendTitle.find("<mean>") != string::npos && legendEntries[iLegend-1]->InheritsFrom(TH1::Class()))
    {
      // todo add some formating options for this
      string mean = std::to_string(((TH1*)legendEntries[iLegend-1])->GetMean());
      legendTitle.replace(legendTitle.find("<mean>"), string("<mean>").size(), mean);
    }
    if(legendTitle.find("<maximum>") != string::npos && legendEntries[iLegend-1]->InheritsFrom(TH1::Class()))
    {
      // todo add some formating options for this
      string maximum = std::to_string(((TH1*)legendEntries[iLegend-1])->GetMaximum());
      legendTitle.replace(legendTitle.find("<maximum>"), string("<maximum>").size(), maximum);
    }
    if(legendTitle.find("<minimum>") != string::npos && legendEntries[iLegend-1]->InheritsFrom(TH1::Class()))
    {
      // todo add some formating options for this
      string minimum = std::to_string(((TH1*)legendEntries[iLegend-1])->GetMinimum());
      legendTitle.replace(legendTitle.find("<minimum>"), string("<minimum>").size(), minimum);
    }
    
    unsigned int height = 0;
    unsigned int width = 0;
    TLatex textLine(0,0, legendTitle.c_str());
    textLine.SetTextFont(textFont);
    textLine.SetTextSize(textSizePixel);
    textLine.GetBoundingBox(width, height);
    if(height > legendHeightPixel) legendHeightPixel = height;
    if(!legendBox->GetTitle().empty() && iLegend == legendTitles.size())
    {
      titleWidthPixel = width;
      continue;
    }
    if(width > legendWidthPixelPerColumn[iColumn]) legendWidthPixelPerColumn[iColumn] = width;
    iColumn++;
    iColumn %= nColumns;
    iLegend++;
  }
  for(auto& length : legendWidthPixelPerColumn) legendWidthPixel += length;
  
  unsigned int markerWidthPixel = 0;
  {
    TLatex textLine(0,0, dummyString.c_str());
    textLine.SetTextFont(textFont);
    textLine.SetTextSize(textSizePixel);
    unsigned int height = 0;
    textLine.GetBoundingBox(markerWidthPixel, height);
  }
  
  double legendWidthNDC =  (double)legendWidthPixel/padWidthPixel;
  double legendHeightNDC =  (double)legendHeightPixel/padHeightPixel;
  double markerWidthNDC =  (double)markerWidthPixel/padWidthPixel;
  double titleWidthNDC =  (double)titleWidthPixel/padWidthPixel;
  
  double totalHeightNDC = (nEntries + 0.5*(nEntries+1))* legendHeightNDC / nColumns;
  double totalWidthNDC = (nColumns + 0.3333) * markerWidthNDC + legendWidthNDC;
  if(titleWidthPixel > legendWidthPixel) totalWidthNDC = (0.3333) * markerWidthNDC + titleWidthNDC;
  
  double upperLeftX = legendBox->GetXPosition();
  double upperLeftY = legendBox->GetYPosition();
  
  if(legendBox->IsAutoPlacement())
  {
    pad->cd();
    pad->Update();
    double lowerLeftX = 0;
    double lowerLeftY = 0;
    double fractionOfTickLenght = 0.9;
    // required distance in pad coordinates of box to objects and tics (set to be 90& of the tick length)
    double marginX = fractionOfTickLenght * gStyle->GetTickLength("Y") * (pad->GetUxmax() - pad->GetUxmin()) / (pad->GetX2()-pad->GetX1());
    double marginY = fractionOfTickLenght * gStyle->GetTickLength("X") * (pad->GetUymax() - pad->GetUymin()) / (pad->GetY2()-pad->GetY1());
    bool foundPosition = false;
    
    // draw temporary boxes to exclude areas outside of the coordinate system
    TBox marginsBottom (pad->GetX1(), pad->GetY1(), pad->GetX2(),pad->GetUymin() + gStyle->GetTickLength("X") * (pad->GetUymax() - pad->GetUymin()));
    TBox marginsTop (pad->GetX1(), pad->GetUymax() - gStyle->GetTickLength("X") * (pad->GetUymax() - pad->GetUymin()), pad->GetX2(),pad->GetY2());
    TBox marginsLeft (pad->GetX1(), pad->GetY1(), pad->GetUxmin() + gStyle->GetTickLength("Y") * (pad->GetUxmax() - pad->GetUxmin()), pad->GetY2());
    TBox marginsRight (pad->GetUxmax() - gStyle->GetTickLength("Y") * (pad->GetUxmax() - pad->GetUxmin()), pad->GetY1(), pad->GetX2(),pad->GetY2());
    marginsBottom.Draw("SAME");
    marginsTop.Draw("SAME");
    marginsLeft.Draw("SAME");
    marginsRight.Draw("SAME");
    
    // find box position that does not collide with any of the drawn objects
    for(TObject* o : *pad->GetListOfPrimitives())
    {
      foundPosition = pad->PlaceBox(o, totalWidthNDC + 2*marginX, totalHeightNDC + 2*marginY, lowerLeftX, lowerLeftY);
    }
    if(foundPosition)
    {
      upperLeftX = lowerLeftX + 2*marginX;
      upperLeftY = lowerLeftY + totalHeightNDC + 2*marginY;
    }
    else{
      WARNING("Could not find enough space to place the legend properly.");
      // just place legend within axis ranges of pad
      upperLeftX = (pad->GetUxmin() - pad->GetX1())/(pad->GetX2()-pad->GetX1()) + (1+1/fractionOfTickLenght)*marginX;
      upperLeftY = (pad->GetUymax() - pad->GetY1())/(pad->GetY2()-pad->GetY1()) - (1+1/fractionOfTickLenght)*marginY;
      
      // && ! legendBox.
      //if(legendBox.IsResizeToFitLgend()){}
      /*
       TODO extent y range if auto placing does not find enough space
       TObject* dummyObject = pad->GetListOfPrimitives()->At(1);
       if(dummyObject && dummyObject->InheritsFrom(TH1::Class())){
       TH1* hist = (TH1*)dummyObject;
       DEBUG("MinX: {}, some ratio: {}.", hist->GetYaxis()->GetXmin(), totalHeightNDC / pad->GetWh());
       hist->GetYaxis()->SetRangeUser(pad->GetUymin(), pad->GetUymax());
       pad->Update();
       }
       */
      
    }
    // now remove temporary boxes again
    pad->GetListOfPrimitives()->Remove(&marginsBottom);
    pad->GetListOfPrimitives()->Remove(&marginsTop);
    pad->GetListOfPrimitives()->Remove(&marginsLeft);
    pad->GetListOfPrimitives()->Remove(&marginsRight);
    
    //pad->DrawCollideGrid();
  }
  else if(legendBox->IsUserCoordinates())
  {
    // convert user coordinates to NDC
    pad->Update();
    upperLeftX = (upperLeftX - pad->GetX1())/(pad->GetX2()-pad->GetX1());
    upperLeftY = (upperLeftY - pad->GetY1())/(pad->GetY2()-pad->GetY1());
  }
  
  TLegend* legend = new TLegend(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "", "NDC");
  if(!legendBox->GetTitle().empty()) legend->SetHeader((legendBox->GetTitle()).c_str(), ""); // option "C" allows to center the header
  
  int i = 0;
  for(auto entry : legendEntries)
  {
    string drawStyle = "EP";

    //if((entry->InheritsFrom("TF1")) || errorStyles[i] == "hist")// || noErrBars)
    if((entry->InheritsFrom("TF1")) || str_contains(errorStyles[i], "C")|| str_contains(errorStyles[i], "L") || str_contains(errorStyles[i], "HIST"))// || noErrBars)
    {
      drawStyle = "L";
    }
    // F for filled hist or bar
    // LEP
    
    legend->AddEntry((TH1*)entry, legendTitles[i].c_str(), drawStyle.c_str());
    i++;
  }
  legend->SetNColumns(nColumns);
  legend->SetMargin((markerWidthNDC*nColumns)/totalWidthNDC);
  legend->SetTextFont(textFont);
  legend->SetTextSize(textSizePixel);
  legend->SetTextAlign(kHAlignLeft+kVAlignBottom);
  legend->SetEntrySeparation(1.0 - 0.5* legendHeightNDC/( 0.5*(totalHeightNDC/(double)legend->GetNRows())));
  
  if(legend->GetHeader())
  {
    ((TLegendEntry*)(legend->GetListOfPrimitives()->At(0)))->SetTextFont(textFont);
    ((TLegendEntry*)(legend->GetListOfPrimitives()->At(0)))->SetTextSize(textSizePixel);
  }
  
  legend->SetLineStyle(legendBox->GetBorderStyle());
  legend->SetLineColor(legendBox->GetBorderColor());
  legend->SetLineWidth(legendBox->GetBorderSize());
  legend->SetFillStyle(0);
  //legend->SetFillStyle(legendBox->GetFillStyle());
  //legend->SetFillColor(legendBox->GetFillColor());
  return legend;
}

//****************************************************************************************
/**
 * Function to generate a text box.
 */
//****************************************************************************************
TPaveText* PlotPainter::MakeText(shared_ptr<Plot::Pad::TextBox> textBox){
  // todo this has to be included in text box...
  float_t textSizePixel = 24;  //textBox->GetTextSize;
  int16_t textFont = 43; //textBox->GetTextFont;
  
  string delimiter = " // ";
  string text = textBox->GetText();
    
  int nLetters = 0;
  vector<string> lines;
  
  size_t pos = string::npos;
  do{
    pos = text.find(delimiter);
    string token = text.substr(0, pos);
    if(token.length() > nLetters) nLetters = token.length();
    lines.push_back(token);
    size_t newStart = pos;
    if (pos != string::npos) newStart += delimiter.length();
    text.erase(0, newStart);
  } while (pos != string::npos);
  
  
  int nLines = lines.size();
  gPad->Update();

  double textSizeNDC = textSizePixel / gPad->YtoPixel(gPad->GetY1());
  double textSizeNDCx = 0.6*textSizePixel / gPad->XtoPixel(gPad->GetX2());
  
  double margin = 0.5*textSizePixel;
  double yWidth = (1.0*nLines + 0.5*(nLines-1))* textSizeNDC;
  double xWidth = nLetters * textSizeNDCx;
  
  double upperLeftX = textBox->GetXPosition();
  double upperLeftY = textBox->GetYPosition();
  
  if(textBox->IsUserCoordinates())
  {
    gPad->Update();
    //      upperLeftX = (upperLeftX - gPad->GetX1())/(gPad->GetX2()-gPad->GetX1());
    //      upperLeftY = (upperLeftY - gPad->GetY1())/(gPad->GetY2()-gPad->GetY1());
    upperLeftX = (upperLeftX - gPad->GetX1())/(gPad->GetX2()-gPad->GetX1());
    upperLeftY = (upperLeftY - gPad->GetY1())/(gPad->GetY2()-gPad->GetY1());
  }

  TPaveText* tPaveText = new TPaveText(upperLeftX, upperLeftY - yWidth, upperLeftX + xWidth, upperLeftY, "NDC");
  
  double boxExtent = 0;
  for(auto &line : lines)
  {
    TText* text = tPaveText->AddText(line.c_str());
    text->SetTextFont(textFont);
    text->SetTextSize(textSizePixel);
    //text->SetTextColor(kOrange);
    double width = text->GetBBox().fWidth;
    if(width > boxExtent) boxExtent = width;
  }
  //tPaveText->SetBBoxX2(tPaveText->GetBBox().fX + boxExtent +2*margin);
  tPaveText->SetBorderSize(1);
  tPaveText->SetLineStyle(textBox->GetBorderStyle());
  tPaveText->SetLineColor(textBox->GetBorderColor());
  tPaveText->SetLineWidth(textBox->GetBorderSize());
  //tPaveText->SetMargin(margin/(tPaveText->GetBBox().fX + boxExtent +2*margin));
  tPaveText->SetTextAlign(12);
  //tPaveText->SetTextAlign(kHAlignLeft+kVAlignBottom);

  tPaveText->SetTextFont(textFont);
  tPaveText->SetTextSize(textSizePixel);
  tPaveText->SetFillStyle(0); //todo fix this hard coded value, 4000 only works for pads!
  //tPaveText->SetFillColor(kRed);
  //tPaveText->SetTextColor(kGreen);
  return tPaveText;
  
}

//****************************************************************************************
/**
 * Helper-function dividing two TGraphs
 */
//****************************************************************************************
TGraph* PlotPainter::DivideTSpline(TGraph* numerator, TGraph* denominator){
  
  TGraph* result = (TGraph*)numerator->Clone("ratio");
  TSpline3* denSpline = new TSpline3("denSpline", denominator);
  
  int nPoints = result->GetN();
  
  double *x = result->GetX();
  double *y = result->GetY();
  double *ey = result->GetEY();
  
  for(int i = 0; i < nPoints; i++) {
    double deonomValiue = denominator->Eval(x[i], denSpline);
    y[i] = y[i] / deonomValiue;
    ey[i] = ey[i] * deonomValiue;
  }
  delete denSpline;
  return result;
}

//****************************************************************************************
/**
 * Helper-function dividing two histograms with different binning.
 */
//****************************************************************************************
TH1* PlotPainter::DivideTSpline(TH1* numerator, TH1* denominator)
{
  TGraph denominatorGraph(denominator);
  TSpline3 denominatorSpline(denominator);
  
  TH1* ratio = (TH1*)numerator->Clone("dummyRatio");
  ratio->Reset();
  
  for(int i = 1; i <= numerator->GetNbinsX(); i++)
  {
    double numeratorValue = numerator->GetBinContent(i);
    double x = numerator->GetBinCenter(i);
    double denomValue = denominatorGraph.Eval(x, &denominatorSpline);
    if(denomValue) ratio->SetBinContent(i, numeratorValue/denomValue);
  }
  return ratio;
}

//****************************************************************************************
/**
 * Deletes data points of graph beyond cutoff values.
 */
//****************************************************************************************
void PlotPainter::SetGraphRange(TGraph* graph, optional<double_t> min, optional<double_t> max)
{
  int16_t pointsToRemoveHigh = 0;
  int16_t pointsToRemoveLow = 0;
  
  for(int16_t i = 0; i < graph->GetN(); i++){
    if(min && graph->GetX()[i] < *min)
    {
      pointsToRemoveLow++;
    }
    if(max && graph->GetX()[i] >= *max)
    {
      pointsToRemoveHigh++;
    }
  }
  
  for(int16_t i = 0; i < pointsToRemoveHigh; i++)
  {
    graph->RemovePoint(graph->GetN()-1);
  }
  for(int16_t i = 0; i < pointsToRemoveLow; i++)
  {
    graph->RemovePoint(0);
  }
  
}

//****************************************************************************************
/**
 * Scales graph by a constant value.
 */
//****************************************************************************************
void PlotPainter::ScaleGraph(TGraph* graph, double_t scale)
{
  for (int i = 0; i < graph->GetN(); i++) graph->GetY()[i] *= scale;
}


} // end namespace PlottingFramework
