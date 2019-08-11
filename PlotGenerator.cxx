#include "PlotGenerator.h"


namespace PlottingFramework {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::string;
  using std::vector;
  using std::tuple;
  using std::pair;
  using std::map;
  using std::set;
  
 
  
  
  //****************************************************************************************
  /**
   * Applies style of plot to canvas.
   */
  //****************************************************************************************
  void PlotManager::ApplyStyleSettings(CanvasStyle& canvasStyle, TCanvas* canvas, string controlString)
  {
    
    // apply title offsets
    int padID = 1;
    for(auto& padStyle : canvasStyle.pads)
    {
      TPad* pad = (TPad*)canvas->GetPad(padID);
      TH1* hist = (TH1*)pad->GetListOfPrimitives()->At(0);
      if(!hist) cout << "Error:: no histo found!" << endl;
      hist->GetXaxis()->SetTitleOffset(padStyle.titleOffset[0]);
      hist->GetYaxis()->SetTitleOffset(padStyle.titleOffset[1]);
      hist->GetZaxis()->SetTitleOffset(padStyle.titleOffset[2]);
      //    hist->LabelsOption("v");
      padID++;
    }
    
    // special palette settings and temporary hacks
    if(canvas->GetPad(1)->GetListOfPrimitives()->At(0)->InheritsFrom("TH2")){
      canvas->GetPad(1)->SetRightMargin(0.12+0.06);
      canvas->GetPad(1)->SetTopMargin(0.12-0.05);
      canvas->GetPad(1)->SetBottomMargin(0.12+0.02);
      TH2* temp = ((TH2*)canvas->GetPad(1)->GetListOfPrimitives()->At(0));
      temp->GetXaxis()->SetTitleOffset(1.1); //1.1
      temp->GetYaxis()->SetTitleOffset(1.1); //1.3
      temp->GetZaxis()->SetTitleOffset(1.6); //1.6
      canvas->GetPad(1)->Update();
      TPaletteAxis* palette = (TPaletteAxis*)temp->GetListOfFunctions()->FindObject("palette");
      palette->SetX2NDC(0.865); //0.88
      palette->SetTitleOffset();
      canvas->GetPad(1)->Update();
    }
    // Control string
    if(controlString.find("logX") != string::npos)
    {
      canvas->GetPad(1)->SetLogx(1);
      if(canvas->GetPad(2)) canvas->GetPad(2)->SetLogx(1);
    }
    if(controlString.find("logY") != string::npos)
    {
      canvas->GetPad(1)->SetLogy(1);
    }
    if(controlString.find("logZ") != string::npos)
    {
      canvas->GetPad(1)->SetLogz(1);
    }
    if(controlString.find("gridY") != string::npos)
    {
      canvas->GetPad(1)->SetGridy(1);
    }
    if(controlString.find("ratioGridY") != string::npos)
    {
      if(canvas->GetPad(2)) canvas->GetPad(2)->SetGridy(1);
    }
    
    canvas->cd();
    if(mStyle.drawTimestamps){
      TTimeStamp time;
      TString stTime;
      stTime=Form("#color[16]{%i}",time.GetDate());
      TLatex *timeStamp = new TLatex();
      timeStamp->SetNDC();
      timeStamp->SetTextAlign(22);
      timeStamp->SetTextFont(63);
      timeStamp->SetTextSizePixels(10);
      timeStamp->DrawLatex(mStyle.timestampPosition[0], mStyle.timestampPosition[1], stTime);
    }
    
    
    
  }
  
  
  //****************************************************************************************
  /**
   * Creates canvas based on style of plot.
   */
  //****************************************************************************************
  TCanvas* PlotManager::MakeCanvas(string name, CanvasStyle& canvasStyle)
  {
    // create canvas
    TCanvas* canvas = new TCanvas(name.c_str(), name.c_str(), canvasStyle.canvasWidth, canvasStyle.canvasHeight);
    canvas->SetFillStyle(mStyle.fillStyleCanvas);
    
    // create pads defined by style
    int padID = 1;
    for(auto& padStyle : canvasStyle.pads)
    {
      canvas->cd();
      string padName = "Pad_" + std::to_string(padID);
      TPad* pad = new TPad(padName.c_str(),"", padStyle.position[0], padStyle.position[1], padStyle.position[2], padStyle.position[3]);
      // apply pad settings
      pad->SetFillStyle(mStyle.fillStyleCanvas);
      pad->SetTopMargin(padStyle.margin[0]);
      pad->SetBottomMargin(padStyle.margin[1]);
      pad->SetLeftMargin(padStyle.margin[2]);
      pad->SetRightMargin(padStyle.margin[3]);
      pad->SetNumber(padID);
      
      pad->Draw();
      padID++;
    }
    return canvas;
  }
  
  
  
  
  
  
  
}
