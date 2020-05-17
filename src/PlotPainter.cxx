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
    vector<TObject*> legendEntries;
    
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
    
    if (pad.GetData().empty())
    {
      WARNING("No data to be drawn in pad {}.", padID);
      continue;
    }
    
    // find data that should define the axis frame
    auto framePos = std::find_if(pad.GetData().begin(), pad.GetData().end(), [](auto curData){return curData->GetDefinesFrame();});
    uint8_t frameDataID = (framePos != pad.GetData().end()) ? framePos - pad.GetData().begin() : 0;
    // make a copy of data that will serve as axis frame and put it in front of data vector
    if(pad.GetData()[frameDataID]->GetType() == "ratio"){
      pad.GetData().insert(pad.GetData().begin(), std::make_shared<Plot::Pad::Ratio>(*std::dynamic_pointer_cast<Plot::Pad::Ratio>(pad.GetData()[frameDataID])));
    }
    else{
      pad.GetData().insert(pad.GetData().begin(), std::make_shared<Plot::Pad::Data>(*pad.GetData()[frameDataID]));
    }
    pad.GetData()[0]->SetLegendLable(""); // axis frame should not appear in legend

    TH1* axisHist_ptr = nullptr;
    bool drawLine = false;
    string drawingOptions = "";
    int dataIndex = 0;
    for(auto& data : pad.GetData())
    {
      if(data->GetDrawingOptions()) drawingOptions += *data->GetDrawingOptions();
      // obtain a copy of the current data
      if(optional<data_ptr_t> rawData = GetDataClone(data->GetUniqueName(), availableData))
      {
        // retrieve the actual pointer to the data
        std::visit([&, padID = padID](auto&& data_ptr)
        {
          using data_type = std::decay_t<decltype(data_ptr)>;

          // hack
          if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_1d>)
          {
            if(data->GetUniqueName().find("meanPt_IN_Models") != string::npos)
            {
              double_t rangeMinX = *data->GetMinRangeX();
              double_t rangeMaxX = *data->GetMaxRangeX();
              //data_ptr->GetXaxis()->SetRangeUser(rangeMinX, rangeMaxX);
              data_ptr->GetXaxis()->SetRangeUser(10, rangeMaxX);

              data_ptr->Smooth(100, "R");
            }
          }
          // hack
          
          
          data_ptr->SetTitle("");

          optional<drawing_options_t> defaultDrawingOption = data->GetDrawingOptionAlias();

          if(!data->GetDrawingOptions() && defaultDrawingOption)
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
            bool isDrawn = false;
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist_2d>)
            {
              if(drawingOptions.find("Z") != string::npos){
                data_ptr->Draw(drawingOptions.c_str()); // z axis is only drawn if specified
                data_ptr->Reset("ICE"); //reset only integral, contents and errors
                isDrawn = true;
              }
            }
            if constexpr (std::is_convertible_v<data_type, data_ptr_t_hist>)
            {
              axisHist_ptr = data_ptr;
            }else
            {
              data_ptr->Draw();
              axisHist_ptr = data_ptr->GetHistogram();
            }
            if(!isDrawn) axisHist_ptr->Draw("AXIS");
            axisHist_ptr->Draw("SAME AXIG");
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
                  if(axisLayout.GetOppositeTicks())
                  {
                    if(axisLable == "X") pad_ptr->SetTickx(*axisLayout.GetOppositeTicks());
                    else if(axisLable == "Y") pad_ptr->SetTicky(*axisLayout.GetOppositeTicks());
                  }
                  if(axisLayout.GetTimeFormat())
                  {
                    axis_ptr->SetTimeDisplay(1);
                    axis_ptr->SetTimeFormat((*axisLayout.GetTimeFormat()).c_str());
                  }
                  if(axisLayout.GetTickOrientation())
                  {
                    axis_ptr->SetTicks((*axisLayout.GetTickOrientation()).c_str());
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

            data_ptr->Draw(drawingOptions.c_str());
            
            // in case a lable was specified for the data, remember this for later
            if (data->GetLegendLable() && !(*data->GetLegendLable()).empty()) {
              lables.push_back(*data->GetLegendLable());
              legendEntries.push_back(pad_ptr->GetListOfPrimitives()->Last());
              errorStyles.push_back(drawingOptions);
            }
            pad_ptr->Update(); // adds something to the list of primitives
          }
          dataIndex++;
          drawingOptions = "SAME "; // next data should be drawn to same pad
        }, *rawData);
      }
    } // end data code
    
    
    if(!padTitle.empty())
    {
      // dummy title feature (will be improved once text boxes are implemented propely)
      //TPaveText* titleBox = MakeText(std::make_shared<Plot::Pad::TextBox>(false, false, 0.5, 0.98, kSolid, 0., kWhite, padTitle));
      //titleBox->Draw("SAME");
    }
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
        TPave* legend = GenerateBox(box, pad_ptr, lables, legendEntries);
        legend->SetName(legendName.c_str());
        legend->Draw("SAME");
        legendIndex++;
      }
      else if(box->GetType() == "text")
      {
        string textName = "TextBox_" + std::to_string(textIndex);
        TPave* text = GenerateBox(box, pad_ptr, {}, {});
        text->SetName(textName.c_str());
        text->Draw("SAME");
        textIndex++;
      }
    }
    
    // begin hack
    
    {
      float_t lineWidth = 3.;
      float_t textSize = 28;
      if(plot.GetName() == "meanPt_energies")
      {
        array<double_t, 4> pos = {0.56, 0.43, 0.82, 0.52};
        
        TLegend* legend = new TLegend(pos[0], pos[1], pos[2], pos[3]);
        legend->SetFillStyle(0);
        legend->SetLineWidth(0.);
        legend->SetTextFont(43);
        legend->SetTextSize(textSize);
        //legend->SetEntrySeparation(1.0);
        legend->SetTextAlign(kHAlignLeft+kVAlignCenter);

        auto pythia = legend->AddEntry((TObject*)0, "Pythia", "l");
        pythia->SetLineWidth(lineWidth);
        pythia->SetLineStyle(kSolid);

        auto epos = legend->AddEntry((TObject*)0, "EPOS-LHC", "l");
        epos->SetLineWidth(lineWidth);
        epos->SetLineStyle(kDashed);
        
        legend->Draw();
      }
      if(plot.GetName() == "meanPt_energies_ratio")
      {
        array<double_t, 4> pos = {0.487, 0.812, 0.747, 0.902};
        
        TLegend* legend = new TLegend(pos[0], pos[1], pos[2], pos[3]);
        legend->SetFillStyle(0);
        legend->SetLineWidth(0.);
        legend->SetTextFont(43);
        legend->SetTextSize(textSize);
        //legend->SetEntrySeparation(1.0);
        legend->SetTextAlign(kHAlignLeft+kVAlignCenter);

        auto pythia = legend->AddEntry((TObject*)0, "Pythia", "l");
        pythia->SetLineWidth(lineWidth);
        pythia->SetLineStyle(kSolid);

        auto epos = legend->AddEntry((TObject*)0, "EPOS-LHC", "l");
        epos->SetLineWidth(lineWidth);
        epos->SetLineStyle(kDashed);
        
        legend->Draw();
      }
      if(plot.GetName() == "meanPt_systems_lin")
      {
        array<double_t, 4> pos = {0.139, 0.724, 0.459, 0.903};
        
        TLegend* legend = new TLegend(pos[0], pos[1], pos[2], pos[3]);
        legend->SetFillStyle(0);
        legend->SetLineWidth(0.);
        legend->SetTextFont(43);
        legend->SetTextSize(textSize);
        //legend->SetEntrySeparation(1.0);
        legend->SetTextAlign(kHAlignLeft+kVAlignCenter);

        auto pythia = legend->AddEntry((TObject*)0, "Pythia (pp)", "l");
        pythia->SetLineWidth(lineWidth);
        pythia->SetLineStyle(kSolid);
        auto angantyr = legend->AddEntry((TObject*)0, "Angantyr (p-Pb, Pb-Pb)", "l");
        angantyr->SetLineWidth(lineWidth);
        angantyr->SetLineStyle(9);

        auto epos = legend->AddEntry((TObject*)0, "EPOS-LHC (pp, p-Pb)", "l");
        epos->SetLineWidth(lineWidth);
        epos->SetLineStyle(kDashed);
        auto epos3 = legend->AddEntry((TObject*)0, "EPOS3 (Pb-Pb)", "l");
        epos3->SetLineWidth(lineWidth);
        epos3->SetLineStyle(6);

        legend->Draw();
      }
      if(plot.GetName() == "meanPt_systems")
      {
        array<double_t, 4> pos = {0.466, 0.156, 0.73, 0.345};
        
        TLegend* legend = new TLegend(pos[0], pos[1], pos[2], pos[3]);
        legend->SetFillStyle(0);
        legend->SetLineWidth(0.);
        legend->SetTextFont(43);
        legend->SetTextSize(textSize);
        //legend->SetEntrySeparation(1.0);
        legend->SetTextAlign(kHAlignLeft+kVAlignCenter);

        auto pythia = legend->AddEntry((TObject*)0, "Pythia (pp)", "l");
        pythia->SetLineWidth(lineWidth);
        pythia->SetLineStyle(kSolid);
        auto angantyr = legend->AddEntry((TObject*)0, "Angantyr (p-Pb, Pb-Pb)", "l");
        angantyr->SetLineWidth(lineWidth);
        angantyr->SetLineStyle(9);

        auto epos = legend->AddEntry((TObject*)0, "EPOS-LHC (pp, p-Pb)", "l");
        epos->SetLineWidth(lineWidth);
        epos->SetLineStyle(kDashed);
        auto epos3 = legend->AddEntry((TObject*)0, "EPOS3 (Pb-Pb)", "l");
        epos3->SetLineWidth(lineWidth);
        epos3->SetLineStyle(6);

        legend->Draw();
      }
    
    }
    
    
    
    
    
    // end hack
    
    
    
    
    
    

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


//****************************************************************************************
/**
 * Returns actual dimensions in pixel of the text with latex formatting.
 */
//****************************************************************************************
std::tuple<uint32_t, uint32_t> PlotPainter::GetTextDimensions(TLatex& text)
{
  uint32_t width = 0;
  uint32_t height = 0;
  int16_t font = text.GetTextFont();

  if (font%10<=2) {
    text.GetBoundingBox(width, height);
  }else{
    TLatex t2 = text;
    t2.SetTextFont(font-1);
    TVirtualPad *pad = gROOT->GetSelectedPad();
    Float_t dy = pad->AbsPixeltoY(0) - pad->AbsPixeltoY((Int_t)(text.GetTextSize()));
    Float_t tsize = dy/(pad->GetY2() - pad->GetY1());
    t2.SetTextSize(tsize);
    t2.GetBoundingBox(width, height);
  }
  return {width, height};
}







//****************************************************************************************
/**
 * Function to generate a legend or text box.
 */
//****************************************************************************************
TPave* PlotPainter::GenerateBox(shared_ptr<Plot::Pad::Box> box, TPad* pad, vector<string> lines, vector<TObject*> legendEntries){
  

  optional<int16_t> textColor = box->GetTextColor();
  optional<int16_t> textFont  = box->GetTextFont();
  optional<float_t> textSize  = box->GetTextSize();

  optional<int16_t> borderColor = box->GetBorderColor();
  optional<int16_t> borderStyle = box->GetBorderStyle();
  optional<float_t> borderWidth = box->GetBorderWidth();

  optional<int16_t> fillColor   = box->GetFillColor();
  optional<int16_t> fillStyle   = box->GetFillStyle();
  optional<float_t> fillOpacity = box->GetFillOpacity();

  
  bool isLegend = (box->GetType() == "legend");
  if(!isLegend)
  {
    // split text string to vector
    string delimiter = " // ";
    string text = std::static_pointer_cast<Plot::Pad::TextBox>(box)->GetText();
    size_t pos = 0;
    size_t last = 0;
    std::string token;
    while ((pos = text.find(delimiter, last)) != std::string::npos)
    {
      token = text.substr(last, pos-last);
      lines.push_back(token);
      last = pos + delimiter.length();
    }
    lines.push_back(text.substr(last));
  }

  float_t text_size = (textSize) ? *textSize : 24;
  int16_t text_font = (textFont) ? *textFont : 43;
  int nColumns = 1;//box->GetNumColumns();
  
  int nEntries = legendEntries.size();
  int nLines = lines.size();
  //if(!legendBox->GetTitle().empty()) nEntries++;
  
  int padWidthPixel = pad->XtoPixel(pad->GetX2()); // looks correct, but why does it work??
  int padHeightPixel = pad->YtoPixel(pad->GetY1());
  

  // determine max width and height of legend entries
  uint8_t iColumn = 0;
  double legendWidthPixel = 0;
  double titleWidthPixel = 0;
  vector<unsigned int> legendWidthPixelPerColumn(nColumns, 0);
  double legendHeightPixel = 0;
  //if(!legendBox->GetTitle().empty()) legendTitles.push_back(legendBox->GetTitle());
  int iLegend = 1;
  for(auto& legendTitle : lines)
  {
    if(isLegend)
    {
      // first replace placeholders for content dependent lables
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
    }
    
    // determine width and height of line to find max width and height (per column)
    TLatex textLine(0,0, legendTitle.c_str());
    textLine.SetTextFont(text_font);
    textLine.SetTextSize(text_size);
    auto [width, height] = GetTextDimensions(textLine);

    if(height > legendHeightPixel) legendHeightPixel = height;
    //if(!legendBox->GetTitle().empty() && iLegend == legendTitles.size())
    //{
      //titleWidthPixel = width;
      //continue;
    //}
    if(width > legendWidthPixelPerColumn[iColumn]) legendWidthPixelPerColumn[iColumn] = width;
    iColumn++;
    iColumn %= nColumns;
    iLegend++;
  }
  for(auto& length : legendWidthPixelPerColumn) legendWidthPixel += length;
    
  uint32_t markerWidthPixel = 0;
  if(isLegend)
  {
    string markerDummyString = "-+-"; // defines width of marker
    TLatex markerDummy(0,0, markerDummyString.c_str());
    markerDummy.SetTextFont(text_font);
    markerDummy.SetTextSize(text_size);
    auto [w, h] = GetTextDimensions(markerDummy);
    markerWidthPixel = w;
  }

  double legendWidthNDC =  (double)legendWidthPixel/padWidthPixel;
  double legendHeightNDC =  (double)legendHeightPixel/padHeightPixel;
  double markerWidthNDC =  (double)markerWidthPixel/padWidthPixel;
  double titleWidthNDC =  (double)titleWidthPixel/padWidthPixel;
  
  double totalWidthNDC = 0.;
  double totalHeightNDC = 0.;
  double marginNDC = 0.01;
  if(isLegend)
  {
    totalWidthNDC = 3*marginNDC + markerWidthNDC + legendWidthNDC;
    totalHeightNDC = (nLines + 0.5*(nLines+1))* legendHeightNDC / nColumns;
  }else{
    totalWidthNDC = 2*marginNDC + legendWidthNDC;
    totalHeightNDC = (nLines + 0.5*(nLines-1))* legendHeightNDC;

  }
  
  if(titleWidthPixel > legendWidthPixel) totalWidthNDC = (0.3333) * markerWidthNDC + titleWidthNDC;
  
  double upperLeftX = box->GetXPosition();
  double upperLeftY = box->GetYPosition();
  
  if(box->IsAutoPlacement())
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
    }
    // now remove temporary boxes again
    pad->GetListOfPrimitives()->Remove(&marginsBottom);
    pad->GetListOfPrimitives()->Remove(&marginsTop);
    pad->GetListOfPrimitives()->Remove(&marginsLeft);
    pad->GetListOfPrimitives()->Remove(&marginsRight);
    //pad->DrawCollideGrid();
  }
  else if(box->IsUserCoordinates())
  {
    // convert user coordinates to NDC
    pad->Update();
    upperLeftX = (upperLeftX - pad->GetX1())/(pad->GetX2()-pad->GetX1());
    upperLeftY = (upperLeftY - pad->GetY1())/(pad->GetY2()-pad->GetY1());
  }
  
  if(isLegend)
  {
    TLegend* legend = new TLegend(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "", "NDC");
    legend->SetMargin((2*marginNDC + markerWidthNDC)/totalWidthNDC);
    legend->SetTextAlign(kHAlignLeft+kVAlignCenter);
    legend->SetNColumns(nColumns);

    legend->SetTextFont(text_font);
    legend->SetTextSize(text_size);
    
    int i = 0;
    for(auto entry : legendEntries)
    {
      string drawingOption = entry->GetDrawOption();
      string drawStyle = "EP";

      if((entry->InheritsFrom("TF1")) || str_contains(drawingOption, "C")|| str_contains(drawingOption, "L") || str_contains(drawingOption, "HIST"))
      {
        drawStyle = "L";
      }
      if(entry->InheritsFrom("TH1") && (str_contains(drawingOption, "HIST") || str_contains(drawingOption, "B"))  && ((TH1*)entry)->GetFillStyle() != 0)
      {
        drawStyle = "F";
      }
      legend->AddEntry((TH1*)entry, lines[i].c_str(), drawStyle.c_str());
      i++;
    }
    
    if(legend->GetHeader())
    {
      ((TLegendEntry*)(legend->GetListOfPrimitives()->At(0)))->SetTextFont(text_font);
      ((TLegendEntry*)(legend->GetListOfPrimitives()->At(0)))->SetTextSize(text_size);
    }
    
    if(borderStyle) legend->SetLineStyle(*borderStyle);
    if(borderColor) legend->SetLineColor(*borderColor);
    if(borderWidth) legend->SetLineWidth(*borderWidth);
    
    if(fillStyle) legend->SetFillStyle(*fillStyle);
    if(fillColor) legend->SetFillColor(*fillColor);
    if(fillOpacity && fillColor) legend->SetFillColor(TColor::GetColorTransparent(*fillColor, *fillOpacity));

    return legend;
    
  } else{
    
    TPaveText* paveText = new TPaveText(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "NDC");
    paveText->SetMargin(marginNDC/totalWidthNDC);
    paveText->SetTextAlign(kHAlignLeft+kVAlignCenter);

    paveText->SetTextFont(text_font);
    paveText->SetTextSize(text_size);

    if(borderStyle) paveText->SetLineStyle(*borderStyle);
    if(borderColor) paveText->SetLineColor(*borderColor);
    if(borderWidth) paveText->SetLineWidth(*borderWidth);
    
    if(fillStyle) paveText->SetFillStyle(*fillStyle);
    if(fillColor) paveText->SetFillColor(*fillColor);
    if(fillOpacity && fillColor) paveText->SetFillColor(TColor::GetColorTransparent(*fillColor, *fillOpacity));

    for(auto& line : lines)
    {
      TText* text = paveText->AddText(line.c_str());
      text->SetTextFont(text_font);
      text->SetTextSize(text_size);
    }
    return paveText;
  }
  
}
