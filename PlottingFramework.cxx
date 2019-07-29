#include "PlottingFramework.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace PlottingProject {
  
  using boost::property_tree::ptree;
  using boost::property_tree::write_xml;
  using boost::property_tree::read_xml;
  
  
  // -----------------------------------------------------------------------------
  // -----------------------------------------------------------------------------
  // --------------------------- Interface functions -----------------------------
  // -----------------------------------------------------------------------------
  // -----------------------------------------------------------------------------
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Constructor for PlottingFramework.
  ///
  /// @param name     Name of analysis
  
  PlottingFramework::PlottingFramework()
  {
    /*
     ptree config;
     config.put("irgendwas.ptee", 30);
     config.put("irgendwas.mee", "Sebastian");
     write_xml("myXML.xml", config);
     
     
     ptree configIn;
     read_xml("myXML.xml", configIn);
     float  = configIn.get<float>("main.float");
     cout << v << endl;
     write_xml("myXML.xml", configIn);
     
     //ptree_bad_path exception. If value could not be translated, it will be ptree_bad_data. Both of them derive from ptree_error
     */
    cout << "Doing plotting ..." << endl;
    cout << endl;
    
    mHistoLedger = new TObjArray(1);
    mHistoLedger->SetOwner();
    
    mPlotLedger = new TObjArray(1);
    mPlotLedger->SetOwner();
    
    DefineDefaultPlottingStyle();
    SetGlobalStyle();
    gErrorIgnoreLevel = kWarning;
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Destructor for OfflineAnalysis.
  /// Deletes ledger and the contained histograms from the heap.
  
  PlottingFramework::~PlottingFramework()
  {
    mHistoLedger->Delete();
    mPlotLedger->Delete();
    //  fOutputFile->Close();
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Adds input histograms from file to internal histogram ledger.
  ///
  /// @param datasetIdentifier          Unique identifier for current dataset (is added as suffix to histogram name)
  /// @param inputFileName              Path to file containing the data measurement
  
  void PlottingFramework::AddHistosFromInputFile(string datasetIdentifier, string inputFileName)
  {
    TFile inputFile(inputFileName.c_str(), "READ");
    if (inputFile.IsZombie()) {
      mListOfMissingFiles.push_back(inputFileName);
      //cout << "ERROR: Could not open " << inputFileName << "." << endl;
      return;
    }
    if (std::find(mDatasetIdentifiers.begin(), mDatasetIdentifiers.end(), datasetIdentifier) != mDatasetIdentifiers.end())
    {
      if(inputFileName.find("_Syst") == string::npos){
        cout << "ERROR: Dataset "  << datasetIdentifier <<  " was already booked!" << endl;
        return;
      }
    }
    BookHistos(inputFile, datasetIdentifier);
    mDatasetIdentifiers.push_back(datasetIdentifier);
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Check if datasets are loaded in plotting framework.
  ///
  
  bool PlottingFramework::ContainsDatasets(std::initializer_list<string> requiredDatasets){
    for(string requiredDataset : requiredDatasets){
      if (std::find(mDatasetIdentifiers.begin(), mDatasetIdentifiers.end(), requiredDataset) == mDatasetIdentifiers.end())
      {
        cout << "Dataset " << requiredDataset << " was not loaded in framework." << endl;
        return false;
      }
    }
    return true;
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Function that recursively traverses file structure of input file and adds histograms to internal ledger
  ///
  /// @param folder                     Current folder to search for histograms
  /// @param datasetIdentifier          Unique identifier for current dataset (is added as suffix to histogram name)
  
  void PlottingFramework::BookHistos(TDirectoryFile& folder, string datasetIdentifier)
  {
    TIter nextKey(folder.GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)nextKey())) {
      TNamed* obj = (TNamed*) key->ReadObj();
      if(obj->IsA() == TDirectoryFile::Class()) {
        BookHistos(*((TDirectoryFile*)obj), datasetIdentifier);
      }else{
        string newName = string(obj->GetName()) + "_@_" + datasetIdentifier;
        TH1* hist = (TH1*)obj->Clone(newName.c_str());
        hist->SetDirectory(0);
        mHistoLedger->Add(hist);
      }
    }
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Opens the output file.
  ///
  /// @param outputFileName      Path to the output file
  
  void PlottingFramework::SetOutputDirectory(string path)
  {
    mOutputDirectory = path;
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Dummy
  ///
  /// @param dummy      dummy
  
  void PlottingFramework::SavePlot(string plotName, string figureGroup, string subFolder)
  {
    string folderName = mOutputDirectory + "/" + figureGroup;
    if(subFolder != "") folderName += "/" + subFolder;
    
    gSystem->Exec((string("mkdir -p ") + folderName).c_str());
    
    string name = plotName + "_@_" + figureGroup;
    TH1* plot = (TH1*)mPlotLedger->FindObject(name.c_str());
    if (!plot)
    {
      //cout << "ERROR: Plot " << name << " was not booked." << endl;
    }else{
      plot->SaveAs((folderName + "/" + name + ".pdf").c_str());
    }
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Function to write all plots to root file.
  ///
  /// @param outputFileName      Name of output file (including .root extension)
  
  void PlottingFramework::WritePlotsToFile(string outputFileName)
  {
    TFile outputFile((mOutputDirectory + "/" + outputFileName).c_str(), "RECREATE");
    outputFile.cd();
    mPlotLedger->Write();
  }
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Dummy
  ///
  /// @param dummy      dummy
  
  void PlottingFramework::CreateDatasetPlots(string datasetIdentifier)
  {
  }
  
  void PlottingFramework::ApplyStyleSettings(CanvasStyle& canvasStyle, TCanvas* canvas, string controlString)
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
  
  TCanvas* PlottingFramework::MakeCanvas(string name, CanvasStyle& canvasStyle)
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
  
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Dummy
  ///
  /// @param dummy      dummy
  
  void PlottingFramework::CreateNewPlot(Plot& plot, string canvasStyleName, bool saveToConfig)
  {
    if(!IsPlotPossible(plot)){
      mListOfMissingPlots.push_back(plot.GetName());
      return;
    }
    if(plot.GetFigureGroup() == ""){
      cout << "ERROR: Please specify a figure group!" << endl;
      return;
    }
    string name = plot.GetName() + "_@_" + plot.GetFigureGroup();
    string controlString = plot.GetControlString();
    
    CanvasStyle* tempPtr = GetCanvasStyle(canvasStyleName);
    if(!tempPtr)
    {
      cout << "ERROR: Canvas style '"  << canvasStyleName << "' is not defined." << endl;
      return;
    }
    CanvasStyle& canvasStyle = *tempPtr;
    if(canvasStyle.pads.size() != plot.GetNumSubPlots())
    {
      cout << "ERROR: Number of pads in style '"  << canvasStyleName << "' does not match the number of subplots needed for plotting '" << plot.GetName() << "'." << endl;
      return;
    }
    TCanvas* canvas = MakeCanvas(name, canvasStyle);
    DrawPlotInCanvas(plot, canvas);
    ApplyStyleSettings(canvasStyle, canvas, controlString);
    
    mPlotLedger->Add(canvas);
  }
  
  void PlottingFramework::CutHistogram(TH1* hist, double cutoff, double cutoffLow)
  {
    if(cutoff < -997) return;
    int cutoffBin = hist->GetXaxis()->FindBin(cutoff);
    for(int i = cutoffBin; i <= hist->GetNbinsX(); i++){
      hist->SetBinContent(i, 0);
      hist->SetBinError(i, 0);
    }
    if(cutoffLow < -997) return;
    int cutoffBinLow = hist->GetXaxis()->FindBin(cutoffLow);
    for(int i = 1; i <= cutoffBinLow; i++){
      hist->SetBinContent(i, 0);
      hist->SetBinError(i, 0);
    }
    
  }
  
  bool PlottingFramework::IsPlotPossible(Plot &plot)
  {
    for(auto& histoTemplate : plot.GetHistoTemplates())
    {
      string histName = histoTemplate.name;
      TObject* obj = mHistoLedger->FindObject(histName.c_str());
      if(!obj)
      {
        mListOfMissingHistograms.push_back(histName);
        return false;
      }
    }
    for(auto& ratioTemplate : plot.GetRatioTemplates())
    {
      string ratioName = ratioTemplate.name;
      string delimiter = " / ";
      string numerator = ratioName.substr(0, ratioName.find(delimiter));
      string denominator = ratioName.substr(ratioName.find(delimiter)+ delimiter.length());
      
      TObject* obj1 = mHistoLedger->FindObject(numerator.c_str());
      TObject* obj2 = mHistoLedger->FindObject(denominator.c_str());
      if(!obj1)
      {
        cout << "ERROR: Histogram " << numerator << " not found!" << endl;
        return false;
      }
      if(!obj2)
      {
        cout << "Histogram " << denominator << " not found!" << endl;
        return false;
      }
    }
    return true;
  }
  
  void PlottingFramework::DrawPlotInCanvas(Plot &plot, TCanvas* canvas)
  {
    canvas->cd();
    
    if(plot.IsRatioPlot())
    {
      TPad* mainPad = (TPad*)canvas->GetListOfPrimitives()->At(0);
      TPad* ratioPad = (TPad*)canvas->GetListOfPrimitives()->At(1);
      if(!mainPad || !ratioPad) cout << "ERROR: Pads not found!" << endl;
      mainPad->cd();
      
      string drawingOptions = "";
      int defaultValueIndex = 0; // for markers and colors
      for(auto& histoTemplate : plot.GetHistoTemplates()){
        TH1* histo = GetHistClone(histoTemplate.name);
        histo->UseCurrentStyle();
        if(!histo) cout << "ERROR: " << endl;
        ApplyHistoSettings(histo, histoTemplate, drawingOptions, defaultValueIndex, plot.GetControlString());
        histo->Draw(drawingOptions.c_str());
        drawingOptions = "SAME";
        defaultValueIndex++;
      }
      // patch truncated lowest lable
      // TODO: add possibility to swich that off
      TPad* patch = new TPad("patch", "patch", mainPad->GetLeftMargin()/2 ,0, mainPad->GetLeftMargin()-0.004, mStyle.textSize / mainPad->YtoPixel(mainPad->GetY1()));
      patch->Draw("SAME");
      patch->SetFillColor(mainPad->GetFillColor());
      patch->SetBorderMode(0);
      
      ratioPad->cd();
      drawingOptions = "";
      defaultValueIndex = 0; // for markers and colors
      for(auto& ratioTemplate : plot.GetRatioTemplates()){
        TH1* ratio = GetRatio(ratioTemplate.name);
        if(!ratio) cout << "ERROR: " << endl;
        ratio->UseCurrentStyle();
        ratio->GetYaxis()->CenterTitle(1);
        ratio->GetXaxis()->SetTickLength(0.06);
        ratio->GetYaxis()->SetNdivisions(305); //506
        ApplyHistoSettings(ratio, ratioTemplate, drawingOptions, defaultValueIndex+1, plot.GetControlString());
        
        if(defaultValueIndex == 0)
        {
          TH1* dummyHist = (TH1*)ratio->Clone("dummy");
          dummyHist->SetLineColor(0); // make plot invisible
          TF1* line = new TF1("line", "1", dummyHist->GetXaxis()->GetXmin(), dummyHist->GetXaxis()->GetXmax());
          dummyHist->Draw();
          line->SetLineColor(GetDefaultColor(defaultValueIndex));
          line->SetLineWidth(2);
          //        line->SetLineStyle(9);
          line->Draw("SAME");
          drawingOptions += " SAME";
        }
        
        ratio->Draw(drawingOptions.c_str());
        drawingOptions = "SAME";
        defaultValueIndex++;
      }
      SetAxes(mainPad, plot);
      SetAxes(ratioPad, plot);
      
      // patch truncated uppest lable
      // TODO: add possibility to swich that off
      TPad* patchRatio = new TPad("patchRatio", "patchRatio", ratioPad->GetLeftMargin()/2 , 1, ratioPad->GetLeftMargin()-0.004, 1- (mStyle.textSize / ratioPad->YtoPixel(ratioPad->GetY1())), ratioPad->GetLeftMargin()-0.004);
      patchRatio->Draw("SAME");
      patchRatio->SetFillColor(ratioPad->GetFillColor());
      patchRatio->SetBorderMode(0);
      
      
    }
    else
    {
      TPad* mainPad = (TPad*)canvas->GetListOfPrimitives()->At(0);
      mainPad->cd();
      string drawingOptions = "";
      int defaultValueIndex = 0; // for markers and colors
      for(auto& histoTemplate : plot.GetHistoTemplates()){
        TH1* histo = GetHistClone(histoTemplate.name);
        CutHistogram(histo, histoTemplate.cutoff, histoTemplate.cutoffLow);
        histo->UseCurrentStyle();
        if(!histo) cout << "ERROR: " << endl;
        if(plot.GetControlString().find("normalize") != string::npos) {std::cout << histo->GetName() << ", norm " << histo->Integral() << std::endl; histo->Scale(1/histo->Integral(), "");} // width??
        if(histoTemplate.color < 0) {defaultValueIndex += histoTemplate.color; histoTemplate.marker = 0;  histoTemplate.color = 0;}
        ApplyHistoSettings(histo, histoTemplate, drawingOptions, defaultValueIndex, plot.GetControlString());
        histo->Draw(drawingOptions.c_str());
        drawingOptions = "SAME";
        defaultValueIndex++;
      }
      for(auto& graphTemplate : plot.mGraphs){
        TGraph* graph = GetGraphClone(graphTemplate.name);
        graph->UseCurrentStyle();
        graph->SetLineWidth(4);
        graph->SetLineColor(kBlack);
        graph->Draw(drawingOptions.c_str());
        drawingOptions = "SAME";
        defaultValueIndex++;
      }
      
      SetAxes(mainPad, plot);
      
      
    }
    
    
    
    //  ((TPad*)canvas->GetListOfPrimitives()->At(0))->cd();
    //  cout << "gpad " << gPad->GetName() <<endl;
    for(auto& legendBoxTemplate : plot.GetLegends())
    {
      ((TPad*)canvas->GetListOfPrimitives()->At(0))->cd();
      TLegend* tempLegend = MakeLegend((TPad*)canvas->GetListOfPrimitives()->At(0), legendBoxTemplate, plot.GetHistoTemplates());
      if(!tempLegend){
        cout << "ERROR: could not add legend box." << endl;
        continue;
      }
      tempLegend->Draw("SAME");
    }
    
    for(auto& textBoxTemplate : plot.GetTextBoxes())
    {
      ((TPad*)canvas->GetListOfPrimitives()->At(0))->cd();
      TPaveText* tempText = MakeText(textBoxTemplate);
      if(!tempText){
        cout << "ERROR: could not add text box." << endl;
        continue;
      }
      tempText->Draw("SAME");
    }
  }
  
  TLegend* PlottingFramework::MakeLegend(TPad* pad, Plot::TextBox& legendBoxTemplate, vector<Plot::Histogram>& histos){
    
    int nLetters = 0;
    TObjArray legendEntries(1);
    vector<string> errorStyles;
    vector<string> lables;
    for(auto& histo : histos)
    {
      if(histo.lable.empty()) continue;
      TObject* pointer = pad->GetListOfPrimitives()->FindObject(histo.name.c_str());
      if(!pointer) cout << "ERROR: could not find " << histo.name << " in pad." << endl;
      legendEntries.Add(pointer);
      lables.push_back(histo.lable);
      if(histo.lable.length() > nLetters) nLetters = histo.lable.length();
      errorStyles.push_back(histo.errorStyle);
    }
    
    int nColums = legendBoxTemplate.nColumns;
    
    int nEntries = legendEntries.GetEntries();
    double textSizeNDC = mStyle.textSize / pad->YtoPixel(pad->GetY1());
    double textSizeNDCx = 0.6*mStyle.textSize / pad->XtoPixel(pad->GetX2());
    double markerSpace = 2.5*textSizeNDCx;
    double yWidth = (1.0*nEntries + 0.5*(nEntries-1))* textSizeNDC / nColums;
    double xWidth = (markerSpace + nLetters * textSizeNDCx) * nColums;
    
    
    
    TLegend* legend = new TLegend(legendBoxTemplate.x, legendBoxTemplate.y - yWidth, legendBoxTemplate.x + xWidth, legendBoxTemplate.y, legendBoxTemplate.text.c_str(), "NDC");
    
    // TODO fix header not shown!!
    legend->SetNColumns(nColums);
    
    int i = 0;
    for(auto pointer : legendEntries)
    {
      string drawStyle = "ep";
      
      if((pointer->InheritsFrom("TF1")) || errorStyles[i] == "hist")// || noErrBars)
      {
        drawStyle = "l";
      }
      
      legend->AddEntry((TH1*)pointer, lables[i].c_str(), drawStyle.c_str());
      i++;
    }
    
    legend->SetLineStyle(legendBoxTemplate.borderStyle);
    legend->SetLineColor(legendBoxTemplate.borderColor);
    legend->SetLineWidth(legendBoxTemplate.borderSize);
    legend->SetMargin(markerSpace*nColums/xWidth);
    legend->SetTextAlign(12);
    legend->SetTextFont(mStyle.textFont);
    legend->SetTextSize(mStyle.textSize);
    legend->SetFillStyle(mStyle.fillStyleCanvas);
    
    return legend;
  }
  
  
  TPaveText* PlottingFramework::MakeText(Plot::TextBox& textBoxTemplate){
    
    string delimiter = " // ";
    string text = textBoxTemplate.text;
    
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
    double textSizeNDC = mStyle.textSize / gPad->YtoPixel(gPad->GetY1());
    double textSizeNDCx = 0.6*mStyle.textSize / gPad->XtoPixel(gPad->GetX2());
    
    double margin = 0.5*mStyle.textSize;
    double yWidth = (1.0*nLines + 0.5*(nLines-1))* textSizeNDC;
    double xWidth = nLetters * textSizeNDCx;
    
    string option = "NDC"; // use pad coordinates by default
    if(textBoxTemplate.userCoordinates) option = "";
    
    TPaveText* tPaveText = new TPaveText(textBoxTemplate.x, textBoxTemplate.y - yWidth, textBoxTemplate.x + xWidth, textBoxTemplate.y, option.c_str());
    
    double boxExtent = 0;
    for(auto &line : lines)
    {
      TText* text = tPaveText->AddText(line.c_str());
      text->SetTextFont(mStyle.textFont);
      text->SetTextSize(mStyle.textSize);
      double width = text->GetBBox().fWidth;
      if(width > boxExtent) boxExtent = width;
    }
    tPaveText->SetBBoxX2(tPaveText->GetBBox().fX + boxExtent +2*margin);
    tPaveText->SetBorderSize(1);
    tPaveText->SetLineStyle(textBoxTemplate.borderStyle);
    tPaveText->SetLineColor(textBoxTemplate.borderColor);
    tPaveText->SetLineWidth(textBoxTemplate.borderSize);
    tPaveText->SetMargin(margin/(tPaveText->GetBBox().fX + boxExtent +2*margin));
    tPaveText->SetTextAlign(12);
    tPaveText->SetTextFont(mStyle.textFont);
    tPaveText->SetTextSize(mStyle.textSize);
    tPaveText->SetFillStyle(mStyle.fillStyleCanvas);
    return tPaveText;
  }
  
  
  void PlottingFramework::SetAxes(TPad* pad, Plot &plot)
  {
    vector<double>& xRange = plot.GetAxisRange("X");
    vector<double>& yRangeDist = plot.GetAxisRange("Y");
    vector<double>& yRangeRatio = plot.GetAxisRange("ratio");
    vector<double>& zRange = plot.GetAxisRange("Z");
    
    string xTitle = plot.GetAxisTitle("X");
    string yTitleDist = plot.GetAxisTitle("Y");
    string yTitleRatio = plot.GetAxisTitle("ratio");
    string zTitle = plot.GetAxisTitle("Z");
    
    
    vector<double> yRange;
    string yTitle;
    
    // ask if it is ratio plot and if this pad is the ratio pad
    // TODO fix this dirty hack...
    if(plot.IsRatioPlot() && string(pad->GetName()) == "Pad_2")
    {
      yRange = yRangeRatio;
      yTitle = yTitleRatio;
    }
    else{
      yRange = yRangeDist;
      yTitle = yTitleDist;
    }
    
    TObject* obj = pad->GetListOfPrimitives()->At(0);
    
    if(obj->InheritsFrom("TH1"))
    {
      if(!xRange.empty())
      {
        ((TH1*)obj)->GetXaxis()->SetRangeUser(xRange[0], xRange[1]);
      }
      if(!yRange.empty())
      {
        ((TH1*)obj)->GetYaxis()->SetRangeUser(yRange[0], yRange[1]);
      }
      if(!zRange.empty())
      {
        ((TH1*)obj)->GetZaxis()->SetRangeUser(zRange[0], zRange[1]);
      }
      if(!xTitle.empty()) ((TH1*)obj)->GetXaxis()->SetTitle(xTitle.c_str());
      if(!yTitle.empty()) ((TH1*)obj)->GetYaxis()->SetTitle(yTitle.c_str());
      if(!zTitle.empty()) ((TH1*)obj)->GetZaxis()->SetTitle(zTitle.c_str());
      
    }
    if(obj->InheritsFrom("TGraph"))
    {
      if(!xRange.empty())
      {
        ((TGraph*)obj)->GetXaxis()->SetRangeUser(xRange[0], xRange[1]);
      }
      if(!yRange.empty())
      {
        ((TGraph*)obj)->GetYaxis()->SetRangeUser(yRange[0], yRange[1]);
      }
      if(!xTitle.empty()) ((TGraph*)obj)->GetXaxis()->SetTitle(xTitle.c_str());
      if(!yTitle.empty()) ((TGraph*)obj)->GetYaxis()->SetTitle(yTitle.c_str());
      
    }
    if(obj->InheritsFrom("TF1"))
    {
      if(!xRange.empty())
      {
        ((TF1*)obj)->GetXaxis()->SetRangeUser(xRange[0], xRange[1]);
      }
      if(!yRange.empty())
      {
        ((TF1*)obj)->GetYaxis()->SetRangeUser(yRange[0], yRange[1]);
      }
      if(!xTitle.empty()) ((TF1*)obj)->GetXaxis()->SetTitle(xTitle.c_str());
      if(!yTitle.empty()) ((TF1*)obj)->GetYaxis()->SetTitle(yTitle.c_str());
    }
  }
  
  
  void PlottingFramework::ApplyHistoSettings(TH1* histo, Plot::Histogram &histoTemplate, string &drawingOptions, int defaultValueIndex, string controlString)
  {
    histo->SetTitle("");
    
    if(histo->InheritsFrom("TH2")){
      drawingOptions += " COLZ";
      return;
    }
    int markerStyle = (histoTemplate.marker) ? histoTemplate.marker : GetDefaultMarker(defaultValueIndex);
    histo->SetMarkerStyle(markerStyle);
    
    int color = (histoTemplate.color) ? histoTemplate.color : GetDefaultColor(defaultValueIndex);
    histo->SetMarkerColor(color);
    histo->SetLineColor(color);
    
    string errorStyle = histoTemplate.errorStyle;
    if(errorStyle == "none")
    {
      histo->SetLineWidth(0);
    }
    if(errorStyle == "hist")
    {
      drawingOptions += " HIST";
    }
    else if(errorStyle == "band")
    {
      drawingOptions += " E5";
      
      histo->SetMarkerSize(0.);
      histo->SetFillColor(color);
      histo->SetFillStyle(1);
    }
    else if(errorStyle == "boxes")
    {
      TExec errorBoxesOn("errorBoxesOn","gStyle->SetErrorX(0.48)");
      errorBoxesOn.Draw();
      histo->SetFillStyle(0);
      drawingOptions += " E2";
      TExec errorBoxesOff("errorBoxesOff","gStyle->SetErrorX(0)");
      errorBoxesOff.Draw();
    }
    
    if(controlString.find("thick") != string::npos){
      histo->SetLineWidth(mStyle.lineWidthThick);
      histo->SetMarkerSize(mStyle.markerSizeThick);
    }
    
  }
  
  TGraphErrors* PlottingFramework::GetGraphClone(string graphName)
  {
    TGraphErrors* graph = nullptr;
    TObject* obj = mHistoLedger->FindObject(graphName.c_str());
    if(obj)
    {
      graph = (TGraphErrors*)obj->Clone();
      //graph->SetDirectory(0);
    }else{
      cout << "ERROR: Could not find " << graphName << "." << endl;
    }
    return graph;
  }
  
  
  TH1* PlottingFramework::GetHistClone(string histName)
  {
    TH1* histo = nullptr;
    TObject* obj = mHistoLedger->FindObject(histName.c_str());
    if(obj)
    {
      histo = (TH1*)obj->Clone();
      histo->SetDirectory(0);
    }else{
      cout << "ERROR: Could not find " << histName << "." << endl;
    }
    return histo;
  }
  
  
  TH1* PlottingFramework::GetRatio(string ratioName)
  {
    string delimiter = " / ";
    string numerator = ratioName.substr(0, ratioName.find(delimiter));
    string denominator = ratioName.substr(ratioName.find(delimiter)+ delimiter.length());
    
    TH1* ratio = GetHistClone(numerator.c_str());
    TH1* temp = GetHistClone(denominator.c_str());
    
    if(true)//ratio->GetNbinsX() == temp->GetNbinsX())
    {
      ratio->Divide(temp);
    }
    else
    {
      // if binning not the same
      TH1* ratioOld = ratio;
      ratio = DivideWithTSpline(ratioOld, temp);
      delete ratioOld;
    }
    
    delete temp;
    ratio->GetYaxis()->SetTitle("ratio");
    return ratio;
  }
  
  TH1* PlottingFramework::DivideWithTSpline(TH1* numerator, TH1* denominator)
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
      //cout << "i: " << i << " x: " << x << " denomValue: " << denomValue << endl;
      if(denomValue) ratio->SetBinContent(i, numeratorValue/denomValue);
    }
    return ratio;
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Set color palette for 2D plots.
  ///
  /// @param palette      Integer representing ROOT internal color palette
  ///
  /// This is a selection of available palettes:
  /// - Standard: kBird, 1 (ROOT5 default), kDarkRainBow, kTemperatureMap, kLightTemperature, kRainBow
  /// - Gray: kGreyScale, kGreyYellow, kPearl, kPigeon
  /// - Green: kAvocado, kBlueGreenYellow, kAlpine, kGistEarth, kViridis
  /// - RedBlue: kThermometer, kCandy, kBlackBody, kCherry, kRedBlue
  /// - RedGreen: kRose, kWaterMelon
  /// - Blue: kLake, kAtlantic
  /// - BlueYellow: kBlueYellow, kStarryNight
  /// - Brown: kCoffee, kFall, kSandyTerrain, kCopper
  /// - Purple: kFuchsia
  /// - Intense: kGreenPink, kOcean, kDarkBodyRadiator, kInvertedDarkBodyRadiator, kSunset, kVisibleSpectrum
  
  
  void PlottingFramework::SetPalette(int palette)
  {
    mStyle.palette = palette;
    gStyle->SetPalette(mStyle.palette);
  }
  
  
  // call once
  void PlottingFramework::SetGlobalStyle()
  {
    // General settings
    TGaxis::SetMaxDigits(3);
    gStyle->SetOptStat(0);  // switch of stat boxes
    if (mStyle.useCMYK) gStyle->SetColorModelPS(1);
    gStyle->SetPalette(mStyle.palette);
    
    // Set Fonts
    gStyle->SetTextFont(mStyle.textFont);
    
    gStyle->SetLabelFont(mStyle.lableFont, "X");
    gStyle->SetLabelFont(mStyle.lableFont, "Y");
    gStyle->SetLabelFont(mStyle.lableFont, "Z");
    
    gStyle->SetTitleFont(mStyle.titleFont, "X");
    gStyle->SetTitleFont(mStyle.titleFont, "Y");
    gStyle->SetTitleFont(mStyle.titleFont, "Z");
    
    // multpt plots:
    //  if(control.Contains("2D")){titleOffsetY= 1.0; titleOffsetX= 1.1;}
    
    //  gStyle->SetTitleOffset(mStyle.fTitleOffsetX, "X");
    //  gStyle->SetTitleOffset(mStyle.fTitleOffsetY, "Y");
    //  gStyle->SetTitleOffset(mStyle.fTitleOffsetZ, "Z");
    
    gStyle->SetMarkerSize(mStyle.markerSize);
    
    gStyle->SetLabelSize(mStyle.lableSizeScaling * mStyle.textSize, "X");
    gStyle->SetLabelSize(mStyle.lableSizeScaling * mStyle.textSize, "Y");
    gStyle->SetLabelSize(mStyle.lableSizeScaling * mStyle.textSize, "Z");
    
    gStyle->SetTitleSize(mStyle.titleSizeScaling * mStyle.textSize, "X");
    gStyle->SetTitleSize(mStyle.titleSizeScaling * mStyle.textSize, "Y");
    gStyle->SetTitleSize(mStyle.titleSizeScaling * mStyle.textSize, "Z");
    
  }
  
  int PlottingFramework::GetDefaultColor(int colorIndex)
  {
    return mStyle.defaultColors[colorIndex % mStyle.defaultColors.size()];
  }
  int PlottingFramework::GetDefaultMarker(int markerIndex)
  {
    return mStyle.defaultMarkers[markerIndex % mStyle.defaultMarkers.size()];
  }
  
  void PlottingFramework::DefineDefaultPlottingStyle()
  {
    // Define global style standards
    mStyle.textFont = 43;
    mStyle.lableFont = 43;
    mStyle.titleFont = 43;
    mStyle.pixelBaseLength = 710;
    mStyle.aspectRatio = 1.41428571429; // corresponding to DIN A4
    mStyle.fillStyleCanvas = 4000; // 4000 transparent, 4100 opaqe
    mStyle.drawTimestamps = false;
    mStyle.timestampPosition.push_back(0.05);
    mStyle.timestampPosition.push_back(0.02);
    mStyle.textSize = 17000 / mStyle.pixelBaseLength;
    mStyle.lableSizeScaling       = 1.00;   // Factor the label will be larger than the textsize
    mStyle.titleSizeScaling       = 1.20;   // Factor the title will be larger than the textsize
    mStyle.markerSize = 1.2;
    mStyle.markerSizeThick = 2.2;
    mStyle.lineWidth = 1.0;
    mStyle.lineWidthThick = 2.0;
    
    mStyle.palette = kBird;
    mStyle.defaultMarkers = {kFullSquare, kFullCircle, kFullCross, kFullDiamond, kFullStar, kOpenCircle, kOpenSquare, kOpenCross, kOpenDiamond, kOpenStar};
    mStyle.defaultMarkersFull = {kFullSquare, kFullCircle, kFullCross, kFullDiamond, kFullStar, kFullCircle, kFullCircle, kFullCircle, kFullCircle, kFullCircle, kFullCircle};
    mStyle.defaultMarkersOpen = {kOpenSquare, kOpenCircle, kOpenCross, kOpenDiamond, kOpenStar, kOpenCircle, kOpenCircle, kOpenCircle, kOpenCircle, kOpenCircle, kOpenCircle};
    mStyle.defaultColors = {kBlack, kBlue+1, kRed+2, kGreen+2, kTeal-7, kCyan+2, kMagenta-4, kGreen+3, kOrange+1, kViolet-3, kPink+3, kOrange+2, kYellow+3, kGray+2};
    
    
    
    
    // Define specific canvas styles:
    { //--------------------------------------------------------------
      CanvasStyle canvas;
      canvas.styleName = "default";
      canvas.canvasWidth = mStyle.pixelBaseLength;
      canvas.canvasHeight = mStyle.pixelBaseLength;
      PadStyle pad1;
      // position: xlow, ylow, xup, yup
      pad1.position.push_back(0);
      pad1.position.push_back(0.);
      pad1.position.push_back(1.);
      pad1.position.push_back(1.);
      // margin: top, bottom, left, right
      pad1.margin.push_back(0.12-0.05);
      pad1.margin.push_back(0.12+0.02);
      pad1.margin.push_back(0.12);
      pad1.margin.push_back(0.12-0.05);
      // titleOffset: x, y, z
      pad1.titleOffset.push_back(1.1);
      pad1.titleOffset.push_back(1.4);//1.3
      pad1.titleOffset.push_back(1.0);
      
      canvas.pads.push_back(pad1);
      
      mCanvStyles.push_back(canvas);
    } //--------------------------------------------------------------
    
    { //--------------------------------------------------------------
      CanvasStyle canvas;
      canvas.styleName = "default ratio";
      canvas.canvasWidth = mStyle.pixelBaseLength;
      canvas.canvasHeight = mStyle.pixelBaseLength;
      double ratioPadSize = 0.28; //relative size of ratio
      // upper pad
      PadStyle pad1;
      // position: xlow, ylow, xup, yup
      pad1.position.push_back(0.0);
      pad1.position.push_back(ratioPadSize);
      pad1.position.push_back(1.0);
      pad1.position.push_back(1.0);
      // margin: top, bottom, left, right
      pad1.margin.push_back(0.05); //0.03
      pad1.margin.push_back(0.0);
      pad1.margin.push_back(0.14);
      pad1.margin.push_back(0.05);
      // titleOffset: x, y, z
      pad1.titleOffset.push_back(3.1);
      pad1.titleOffset.push_back(1.5);
      pad1.titleOffset.push_back(1.0);
      canvas.pads.push_back(pad1);
      // ratio pad
      PadStyle pad2;
      // position: xlow, ylow, xup, yup
      pad2.position.push_back(0.);
      pad2.position.push_back(0.);
      pad2.position.push_back(1.);
      pad2.position.push_back(ratioPadSize);
      // margin: top, bottom, left, right
      pad2.margin.push_back(0.015);
      pad2.margin.push_back(0.4); //0.35
      pad2.margin.push_back(0.14); // 0.12
      pad2.margin.push_back(0.05);
      // titleOffset: x, y, z
      pad2.titleOffset.push_back(4.1);
      pad2.titleOffset.push_back(1.5); //1.3
      pad2.titleOffset.push_back(1.0);
      canvas.pads.push_back(pad2);
      
      mCanvStyles.push_back(canvas);
    } //--------------------------------------------------------------
    
    /*
     //"horizontal"
     canvas->SetLeftMargin(mStyle.fMargin*mStyle.fAspectRatio);
     canvas->SetRightMargin(mStyle.fMargin*mStyle.fAspectRatio);
     canvas->SetTopMargin(mStyle.fMargin - 0.03);
     canvas->SetBottomMargin(mStyle.fMargin + 0.03);
     //"vertical"
     canvas->SetLeftMargin(mStyle.fMargin);
     canvas->SetRightMargin(mStyle.fMargin);
     canvas->SetTopMargin(mStyle.fMargin*mStyle.fAspectRatio - 0.05);
     canvas->SetBottomMargin(mStyle.fMargin*mStyle.fAspectRatio + 0.05);
     // else
     canvas->SetLeftMargin(mStyle.fMargin);
     canvas->SetRightMargin(mStyle.fMargin);
     canvas->SetTopMargin(mStyle.fMargin-0.1);
     canvas->SetBottomMargin(mStyle.fMargin+0.1);
     
     
     // Horizontal canvas style
     fHorizontalCanvStyle.fCanvasWidth = mStyle.fPixelBaseLength * mStyle.fAspectRatio;
     fHorizontalCanvStyle.fCanvasHeight = mStyle.fPixelBaseLength;
     
     fHorizontalCanvStyle.fMarginTop = 0.03;
     fHorizontalCanvStyle.fMarginBottom = 0.0;
     fHorizontalCanvStyle.fMarginLeft = 0.12;
     fHorizontalCanvStyle.fMarginRight = 0.05;
     
     fHorizontalCanvStyle.fMarginTop = 0.0;
     fHorizontalCanvStyle.fMarginBottom = 0.35; //0.3
     fHorizontalCanvStyle.fMarginLeft = 0.12;
     fHorizontalCanvStyle.fMarginRight = 0.05;
     
     // Vertical canvas style
     fVerticalCanvStyle.fCanvasWidth = mStyle.fPixelBaseLength;
     fVerticalCanvStyle.fCanvasHeight = mStyle.fPixelBaseLength * mStyle.fAspectRatio;
     
     fVerticalCanvStyle.fMarginTop = 0.03;
     fVerticalCanvStyle.fMarginBottom = 0.0;
     fVerticalCanvStyle.fMarginLeft = 0.12;
     fVerticalCanvStyle.fMarginRight = 0.05;
     
     fVerticalCanvStyle.fMarginTop = 0.0;
     fVerticalCanvStyle.fMarginBottom = 0.35; //0.3
     fVerticalCanvStyle.fMarginLeft = 0.12;
     fVerticalCanvStyle.fMarginRight = 0.05;
     
     */
    
  }
  
  PlottingFramework::CanvasStyle* PlottingFramework::GetCanvasStyle(string styleName)
  {
    for(auto& canvasStyle : mCanvStyles)
    {
      if(canvasStyle.styleName == styleName) return &canvasStyle;
    }
    cout << "ERROR: nullptr returned." << endl;
    return nullptr;
  }
  
  
  void PlottingFramework::SetTransparentCanvas(bool transparent)
  {
    if(transparent) mStyle.fillStyleCanvas = 4000;
    else mStyle.fillStyleCanvas = 4100;
  }
  
  void PlottingFramework::ListAvailableCanvasStyles()
  {
    for(auto& canvasStyle : mCanvStyles)
    {
      cout << "======== " << canvasStyle.styleName << " ========" << endl;
      cout << "  >> Width  : " << canvasStyle.canvasWidth << endl;
      cout << "  >> Height : " << canvasStyle.canvasHeight << endl;
      cout << "  >> nPads  : " << canvasStyle.pads.size() << endl;
      int padID = 1;
      string lines = string((int)((canvasStyle.styleName.length()+17 - 7) / 2) , '-');
      for(auto& padStyle : canvasStyle.pads)
      {
        cout << "  " << lines << " Pad " << padID << " " << lines << endl;
        cout << "    << Position (xlow, ylow, xup, yup)   : (" << padStyle.position[0] << ", " << padStyle.position[1] << ", " << padStyle.position[2] << ", " << padStyle.position[3] << ")" << endl;
        cout << "    << Margin (top, bottom, left, right) : (" << padStyle.margin[0] << ", " << padStyle.margin[1] << ", " << padStyle.margin[2] << ", " << padStyle.margin[3] << ")" << endl;
        cout << "    << Title offset (x, y, z)            : (" << padStyle.titleOffset[0] << ", " << padStyle.titleOffset[1] << ", " << padStyle.titleOffset[2] << ")" << endl;
        
        padID++;
      }
      cout << "========" << string((int)canvasStyle.styleName.length() +2, '=') << "========" << endl << endl;
      
    }
  }



  void PlottingFramework::PrintErrors(bool printMissingPlots){
    if(!mListOfMissingPlots.empty()){
      if(!mListOfMissingFiles.empty()){
        cout << endl << "The following input files could not be loaded: " << endl;
        for(auto& name : mListOfMissingFiles){
          cout << " - " << name << endl;
        }
      }
      if(!mListOfMissingHistograms.empty()){
        cout << endl << "The following histograms were not found: " << endl;
        for(auto& name : mListOfMissingHistograms){
          cout << " - " << name << endl;
        }
      }
      if(printMissingPlots)
      {
        cout << endl << "Therefore the following plots could not be created: " << endl;
        for(auto& name : mListOfMissingPlots){
          cout << " - " << name << endl;
        }
      }
    }
  }
 
}
