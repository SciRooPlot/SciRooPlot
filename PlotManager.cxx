#include "PlotManager.h"

using namespace PlottingFramework;
namespace PlottingFramework {

  //****************************************************************************************
  /**
   * Constructor for Plotting Framework.
   */
  //****************************************************************************************
  PlotManager::PlotManager()
  {    
    mHistoLedger = new TObjArray(1);
    mHistoLedger->SetOwner();
    
    mPlotLedger = new TObjArray(1);
    mPlotLedger->SetOwner();
    
    DefineDefaultPlottingStyles();
    gErrorIgnoreLevel = kWarning;
  }
  
  //****************************************************************************************
  /**
   * Destructor for Plotting Framework.
   * Deletes ledger and the contained histograms from the heap.
   */
  //****************************************************************************************
  PlotManager::~PlotManager()
  {
    mHistoLedger->Delete();
    mPlotLedger->Delete();
  }
  
  void PlotManager::AddInputFilePaths(string inputIdentifier, vector<string> inputFilePathList)
  {
    if(mInputFiles.find(inputIdentifier) != mInputFiles.end() ){
      cout << "WARNING: replacing input identifier " << inputIdentifier << "." << endl;
    }
    mInputFiles[inputIdentifier] = inputFilePathList;
  }
  
  
  void PlotManager::DumpInputFiles(string configFileName){
    ptree inputFileTree;
    for(auto& inFileTuple : mInputFiles){
      ptree filesOfIdentifier;
      int fileID = 1;
      for(auto& fileName : inFileTuple.second){
        filesOfIdentifier.put("FILE_" + std::to_string(fileID), fileName);
        fileID++;
      }
      inputFileTree.put_child(inFileTuple.first, filesOfIdentifier);
    }
    boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
    write_xml(configFileName + ".XML", inputFileTree, std::locale(), settings);
  }
  
  
  void PlotManager::LoadInputFiles(string configFileName)
  {
    ptree inputFileTree;
    try{
      read_xml(configFileName + ".XML", inputFileTree);
    }catch(...){
      cout << "ERROR: Cannot load file " << configFileName << ".XML" << endl;
      return;
    }
    for(auto& inputPair : inputFileTree){
      string inputIdentifier = inputPair.first;
      vector<string> allFileNames;
      for(auto& file : inputPair.second){
        string fileName = inputFileTree.get<string>(inputIdentifier + "." + file.first);
        allFileNames.push_back(fileName);
      }
      AddInputFilePaths(inputIdentifier, allFileNames);
    }
  }
  
  //****************************************************************************************
  /**
   * Adds input histograms from file to internal histogram ledger.
   * @param inputIdentifier: Unique identifier for current dataset
   * (is added as suffix to histogram name)
   * @param inputFileName: Path to file containing the data measurement
   */
  //****************************************************************************************
  void PlotManager::AddHistosFromInputFile(string inputIdentifier, string inputFileName)
  {
    TFile inputFile(inputFileName.c_str(), "READ");
    if (inputFile.IsZombie()) {
      mListOfMissingFiles.push_back(inputFileName);
      //cout << "ERROR: Could not open " << inputFileName << "." << endl;
      return;
    }
    if (std::find(mDatasetIdentifiers.begin(), mDatasetIdentifiers.end(), inputIdentifier) != mDatasetIdentifiers.end())
    {
      if(inputFileName.find("_Syst") == string::npos){
        cout << "ERROR: Dataset "  << inputIdentifier <<  " was already booked!" << endl;
        return;
      }
    }
    BookHistos(inputFile, inputIdentifier);
    mDatasetIdentifiers.push_back(inputIdentifier);
  }
  
  //****************************************************************************************
  /**
   * Adds input histograms from file to internal histogram ledger.
   * @param inputIdentifier: Unique identifier for current dataset
   * (is added as suffix to histogram name)
   * @param inputFileName: Path to file containing the data measurement
   */
  //****************************************************************************************
  void PlotManager::OpenInputFiles(string inputIdentifier)
  {
    if(std::find(mDatasetIdentifiers.begin(), mDatasetIdentifiers.end(), inputIdentifier) != mDatasetIdentifiers.end()) return;
    // todo also override datasetidentifiers when addpath is called?
    for(auto& inputTuple : mInputFiles){
      if(inputTuple.first == inputIdentifier){
        for(auto& fileName : inputTuple.second){
          //cout << "loading " << inputIdentifier << ": " << fileName << endl;
          AddHistosFromInputFile(inputIdentifier, fileName);
        }
      }
    }
  }
  
  void PlotManager::OpenRequiredFiles(Plot& plot)
  {
    for(auto& pad : plot.mData){
      for(auto& data : pad.second){
        OpenInputFiles(data->GetInputIdentifier());
        if(data->GetType() == "ratio") OpenInputFiles(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetDenomIdentifier());
      }
    }
  }

  //****************************************************************************************
  /**
   * Check if datasets are loaded in plotting framework.
   */
  //****************************************************************************************
  // TODO at the moment this is obsolete since vector is only filled when plots are created
  bool PlotManager::ContainsDatasets(std::initializer_list<string> requiredDatasets){
    for(string requiredDataset : requiredDatasets){
      if (std::find(mDatasetIdentifiers.begin(), mDatasetIdentifiers.end(), requiredDataset) == mDatasetIdentifiers.end())
      {
        //cout << "Dataset " << requiredDataset << " was not loaded in framework." << endl;
        return false;
      }
    }
    return true;
  }
  
  
  //****************************************************************************************
  /**
   * Recursively traverses folder structure of input file and
   * adds histograms to internal ledger
   * @todo add option for other internal structures like TLists
   * @param folder: Current folder to search for histograms
   * @param inputIdentifier: Unique identifier for current input file
   * (is added as suffix to histogram name)
   */
  //****************************************************************************************
  void PlotManager::BookHistos(TDirectoryFile& folder, string inputIdentifier)
  {
    TIter nextKey(folder.GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)nextKey())) {
      TNamed* obj = (TNamed*) key->ReadObj();
      if(obj->IsA() == TDirectoryFile::Class()) {
        BookHistos(*((TDirectoryFile*)obj), inputIdentifier);
      }else{
        string uniqueName = string(obj->GetName()) + "_@_" + inputIdentifier;
        TH1* hist = (TH1*)obj->Clone(uniqueName.c_str());
        hist->SetDirectory(0);
        mHistoLedger->Add(hist);
      }
    }
  }
  
  
  //****************************************************************************************
  /**
   * Sets path for output files.
   * @param outputFileName: Path to the output directory
   */
  //****************************************************************************************
  void PlotManager::SetOutputDirectory(string path)
  {
    mOutputDirectory = path;
  }
  
  
  //****************************************************************************************
  /**
   * Saves specified plot from certain figure group into sub directory of output folder.
   * By default the plot is then removed from the plot ledger and heap memory is freed.
   * @todo add option for output formats other than pdf
   * @todo steer plot deletion globally
   * @param plotName: Name of the plot to save
   * @param figureGroup: Figure group in which the plot was created
   * @param subFolder: Sub directory in output folder set via SetOutputDirectory()
   * @param deletePlot: Delete plot from framework after saving to file
   */
  //****************************************************************************************
  
  void PlotManager::SavePlot(string plotName, string figureGroup, string subFolder, bool deletePlot)
  {
    string folderName = mOutputDirectory + "/" + figureGroup;
    if(subFolder != "") folderName += "/" + subFolder;

    gSystem->Exec((string("mkdir -p ") + folderName).c_str());
    
    string internalName = plotName + "_@_" + figureGroup;
    TCanvas* plot = (TCanvas*)mPlotLedger->FindObject(internalName.c_str());
    if (!plot)
    {
      //cout << "ERROR: Plot " << name << " was not booked." << endl;
    }else{
      string name = internalName; // TODO override with optional name specified in plot
      plot->SaveAs((folderName + "/" + name + ".pdf").c_str());
      if(deletePlot) { mPlotLedger->Remove(plot); delete plot; }
    }
  }
    
 
  //****************************************************************************************
  /**
   * Creates plot based on externally defined plot template.
   * @param plot: reference to plot blueprint
   * @param canvasStyleName: name of predefined canvas style
   * to find available canvas styles use: ListAvailableCanvasStyles()
   */
  //****************************************************************************************
  void PlotManager::CreatePlot(Plot& plot, string plotStyleName)
  {
    if(plotStyleName == "") plotStyleName = plot.GetPlotStyle();
    OpenRequiredFiles(plot);
    if(!IsPlotPossible(plot)){
      mListOfMissingPlots.push_back(plot.GetName());
      return;
    }
    if(plot.GetFigureGroup() == ""){
      cout << "ERROR: Please specify a figure group!" << endl;
      return;
    }
    string name = plot.GetName() + "_@_" + plot.GetFigureGroup();
//    string& controlString = plot.GetControlString();

    PlotStyle& plotStyle = GetPlotStyle(plotStyleName);
    TCanvas* canvas = PlotGenerator::GeneratePlot(plot, plotStyle, mHistoLedger);
    if(!canvas) return;
    
    // todo maybe get rid of ledger and save function....
    mPlotLedger->Add(canvas);
    SavePlot(plot.GetName(), plot.GetFigureGroup());
  }
  
  void PlotManager::CreatePlot(string plotName, string figureGroup, string plotStyleName)
  {
    for(auto& plot : mSavedPlots)
    {
      cout << plot.GetName() << "  " << plotName << endl;
      if(plot.GetName() == plotName)
      {
        cout << "creating plot " << plotName << endl;
        CreatePlot(plot, plotStyleName);
      }
    }
  }
  
   void PlotManager::CreatePlots(string figureGroup)
  {
    bool saveAll = (figureGroup == "");
    for(auto& plot : mSavedPlots)
    {
      if(!saveAll && !(plot.GetFigureGroup() == figureGroup))
        continue;
      CreatePlot(plot);
    }
  }
  
  
  void PlotManager::DumpPlots(string plotFileName){
    ptree plotTree;
    for(auto& plot : mSavedPlots){
      string displayedName = plot.GetName();
      std::replace(displayedName.begin(),displayedName.end(), '.', '_');
      plotTree.put_child(("GROUP::" + plot.GetFigureGroup() + ".PLOT::" + displayedName + "_@_" + plot.GetFigureGroup()), plot.GetPropetyTree());
    }
    boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
    write_xml(plotFileName + ".XML", plotTree, std::locale(), settings);
  }

void PlotManager::LoadPlots(string plotFileName, string figureGroup, vector<string> plotNames){
  ptree inputTree; // structure: plotGroup.plot.pad.{DATA,BOXES,AXES}.specificTree
  read_xml(plotFileName + ".XML", inputTree);

  for(auto& plotGroupTree : inputTree){
    if(figureGroup != "" && plotGroupTree.first != "GROUP::"+figureGroup) continue;
    for(auto& plotTree : plotGroupTree.second){
      bool found = false;
      if(plotNames.empty()){
        found = true;
      }else{
        for(auto& plotName : plotNames){
          string entryName = "PLOT::" + plotName + "_@_" + figureGroup;
          if(entryName.find(plotTree.first) != string::npos) found = true;
        }
      }
      if(!found) continue;
      AddPlot(plotTree.second);
    }
  }
}


  void PlotManager::ListPlotStyles()
  {
    for(auto& plotStyle : mPlotStyles)
    {
      cout << "======== " << plotStyle.GetName() << " ========" << endl;
      cout << "  >> Width  : " << plotStyle.GetWidth() << endl;
      cout << "  >> Height : " << plotStyle.GetHeight() << endl;
      cout << "  >> nPads  : " << plotStyle.GetNPads() << endl;
      int padID = 1;
      string lines = string((int)((plotStyle.GetName().length()+17 - 7) / 2) , '-');
      for(auto& padStyle : plotStyle.GetPadStyles())
      {
        cout << "  " << lines << " Pad " << padID << " " << lines << endl;
        cout << "    << Position (xlow, ylow, xup, yup)   : (" << padStyle.GetXLow() << ", " << padStyle.GetYLow() << ", " << padStyle.GetXUp() << ", " << padStyle.GetYUp() << ")" << endl;
        cout << "    << Margin (top, bottom, left, right) : (" << padStyle.GetTopMargin() << ", " << padStyle.GetBottomMargin() << ", " << padStyle.GetLeftMargin() << ", " << padStyle.GetRightMargin() << ")" << endl;
        cout << "    << Title offset (x, y, z)            : (" << padStyle.GetTitleOffsetX() << ", " << padStyle.GetTitleOffsetY() << ", " << padStyle.GetTitleOffsetZ() << ")" << endl;
        
        padID++;
      }
      cout << "========" << string((int)plotStyle.GetName().length() +2, '=') << "========" << endl << endl;
      
    }
  }
  
  //****************************************************************************************
  /**
   * Internal function to check if all input histograms are available to create the plot.
   * @param plot: reference to plot template
   */
  //****************************************************************************************
  bool PlotManager::IsPlotPossible(Plot &plot)
  {
    for(auto& padData : plot.GetData())
      for(auto data : padData.second)
      {
        vector<string> dataNames;
        dataNames.push_back(data->GetUniqueName());
        if(data->GetType() == "ratio") dataNames.push_back(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetUniqueNameDenom());

        for(auto& dataName : dataNames)
        {
          TObject* obj = mHistoLedger->FindObject(dataName.c_str());
          if(!obj)
          {
            mListOfMissingHistograms.push_back(dataName);
            return false;
          }
        }
      }
    return true;
  }
  
    
  void PlotManager::DefineDefaultPlottingStyles()
  {
    vector<int> goodColors =
    { kBlack, kBlue+1, kRed+2, kGreen+2, kTeal-7, kCyan+2,
      kMagenta-4, kGreen+3, kOrange+1, kViolet-3, kPink+3,
      kOrange+2, kYellow+3, kGray+2
    };
    vector<int> goodMarkers =
    { kFullSquare, kFullCircle, kFullCross, kFullDiamond,
      kFullStar, kOpenCircle, kOpenSquare, kOpenCross,
      kOpenDiamond, kOpenStar
    };
    vector<int> goodMarkersFull =
    { kFullSquare, kFullCircle, kFullCross, kFullDiamond,
      kFullStar, kFullCircle, kFullCircle, kFullCircle,
      kFullCircle, kFullCircle, kFullCircle
    };
    vector<int> goodMarkersOpen =
    { kOpenSquare, kOpenCircle, kOpenCross, kOpenDiamond,
      kOpenStar, kOpenCircle, kOpenCircle, kOpenCircle,
      kOpenCircle, kOpenCircle, kOpenCircle
    };
    
    {// Definition of "default" style
      PlotStyle myStyle("default");
      
      //myStyle.LinkAxes("X", {1,2});
      myStyle.SetFromat(710, 1);
      myStyle.SetTransparent();
//      myStyle.SetDefault2DStyle("SURF3");
//      myStyle.SetDefault2DStyle("COLZ CONT3");
      myStyle.SetDefault2DStyle("COLZ");

      myStyle.SetTextFont(4); // allowed font values: 1-15
      myStyle.SetLableFont(4);
      myStyle.SetTitleFont(4);
      myStyle.SetTextSize(24); // in pixels
      myStyle.SetLableSizeScaling(1.0); // make this axis and pad dependent??
      myStyle.SetTitleSizeScaling(1.2);

      myStyle.SetMarkerSize(1.2);
      myStyle.SetMarkerSizeThick(2.2);
      myStyle.SetLineWidth(1.0);
      myStyle.SetLineWidthThick(2.0);
      myStyle.SetPalette(kBird);
      myStyle.SetTimestampPosition(0.05, 0.02);
      myStyle.SetDrawTimestamps(false);

      myStyle.SetDefaultColors(goodColors);
      myStyle.SetDefaultMarkers(goodMarkers);
      myStyle.SetDefaultMarkersFull(goodMarkersFull);
      myStyle.SetDefaultMarkersOpen(goodMarkersOpen);
      
      vector<PlotStyle::PadStyle> pads;
      pads.push_back(PlotStyle::PadStyle(""));
      pads[0].SetCorners({0,0}, {1.0,1.0});
      pads[0].SetMargins({0.07, 0.14, 0.12, 0.07});
      pads[0].SetTitleOffsets({1.1, 1.4, 1.0});
      myStyle.AddPadStyles(pads);
      mPlotStyles.push_back(std::move(myStyle)); // maybe AddPlotStyle(PlotStyle&)
    }
    {// Definition of "default" style
      PlotStyle myStyle("default ratio");
      
      myStyle.LinkAxes("X", {1,2}); // scale x axis in data and ratio in same way automatically
      myStyle.SetFromat(710, 1.0);
      myStyle.SetTransparent();
      
      myStyle.SetTextFont(4); // allowed font values: 1-15
      myStyle.SetLableFont(4);
      myStyle.SetTitleFont(4);
      myStyle.SetTextSize(24); // in pixels
      myStyle.SetLableSizeScaling(1.0); // make this axis and pad dependent??
      myStyle.SetTitleSizeScaling(1.2);
      
      myStyle.SetMarkerSize(1.2);
      myStyle.SetMarkerSizeThick(2.2);
      myStyle.SetLineWidth(1.0);
      myStyle.SetLineWidthThick(2.0);
      myStyle.SetPalette(kBird);
      myStyle.SetTimestampPosition(0.05, 0.02);
      myStyle.SetDrawTimestamps(false);
      
      myStyle.SetDefaultColors(goodColors);
      myStyle.SetDefaultMarkers(goodMarkers);
      myStyle.SetDefaultMarkersFull(goodMarkersFull);
      myStyle.SetDefaultMarkersOpen(goodMarkersOpen);
      
      vector<PlotStyle::PadStyle> pads;
      pads.push_back(PlotStyle::PadStyle(""));
      pads[0].SetCorners({0.0,0.28}, {1.0,1.0});
      pads[0].SetMargins({0.05, 0.0, 0.14, 0.05});
      pads[0].SetTitleOffsets({3.1, 1.5, 1.0});
      pads.push_back(PlotStyle::PadStyle(""));
      pads[1].SetCorners({0.0,0.0}, {1.0,0.28});
      pads[1].SetMargins({0.015, 0.4, 0.14, 0.05});
      pads[1].SetTitleOffsets({4.1, 1.5, 1.0});
      
      
      myStyle.AddPadStyles(pads);
      mPlotStyles.push_back(std::move(myStyle)); // maybe AddPlotStyle(PlotStyle&)
    }    /*
    // BEGIN TODO
    
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
    
    */
    // END TODO
    
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
  
  PlotStyle& PlotManager::GetPlotStyle(string plotStyleName)
  {
    for(auto& plotStyle : mPlotStyles)
    {
      if(plotStyle.GetName() == plotStyleName) return plotStyle;
    }
    cout << "ERROR: plot style named " << plotStyleName << " not found." << endl;
    return GetPlotStyle("default"); // if style not found return default style
  }
  
  void PlotManager::PrintErrors(bool printMissingPlots){
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
