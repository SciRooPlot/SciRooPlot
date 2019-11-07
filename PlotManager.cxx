#include "PlotManager.h"

using namespace PlottingFramework;
namespace PlottingFramework{// BEGIN namespace PlottingFramework

//****************************************************************************************
/**
 * Constructor for PlotManager
 */
//****************************************************************************************
PlotManager::PlotManager() : mApp("MainApp", 0, 0)
{
  TQObject::Connect("TGMainFrame", "CloseWindow()", "TApplication", gApplication, "Terminate()");

  mDataLedger = new TObjArray(1);
  mDataLedger->SetOwner();
  
  mOutputFileName = "ResultPlots.root";
  
  mUseUniquePlotNames = true;
  DefineDefaultPlottingStyles();
  gErrorIgnoreLevel = kWarning;
}

//****************************************************************************************
/**
 * Destructor for PlotManager
 */
//****************************************************************************************
PlotManager::~PlotManager()
{
  mDataLedger->Delete();
  if(!mPlotLedger.empty()){
    
    
    cout << "Saving plots to file " << mOutputFileName << endl;
    
    TFile outputFile(mOutputFileName.c_str(), "RECREATE");
    if (outputFile.IsZombie()){
      return;
    }
    outputFile.cd();
    
    
    for(auto& plotTuple : mPlotLedger){
    
      auto canvas = plotTuple.second;
      string uniqueName = plotTuple.first;
      size_t delimiterPos = uniqueName.find(gNameGroupSeparator.c_str());
      string plotName = uniqueName.substr(0, delimiterPos);
      string inputIdentifier = uniqueName.substr(delimiterPos + gNameGroupSeparator.size());
      
      if(!outputFile.GetDirectory(inputIdentifier.c_str(), kFALSE, "cd"))
      {
        outputFile.mkdir(inputIdentifier.c_str());
      }
      outputFile.cd(inputIdentifier.c_str());
      canvas->Write(plotName.c_str());
    }
    outputFile.Close();
    mPlotLedger.clear();
  }
}

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// --------------------------------------- I/O -------------------------------------------
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

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
 * Define input file paths for user defined unique inputIDentifier.
 */
//****************************************************************************************
void PlotManager::AddInputDataFiles(string inputIdentifier, vector<string> inputFilePathList)
{
  if(mInputFiles.find(inputIdentifier) != mInputFiles.end() ){
    cout << "WARNING: replacing input identifier " << inputIdentifier << "." << endl;
  }
  mInputFiles[inputIdentifier] = inputFilePathList;
}
void PlotManager::AddInputDataFile(string inputIdentifier, string inputFilePath)
{
  vector<string> inputFilePathList = {inputFilePath};
  AddInputDataFiles(inputIdentifier, inputFilePathList);
}

//****************************************************************************************
/**
 * Dump input file identifiers and paths that are currently defined in the manager to a config file.
 */
//****************************************************************************************
void PlotManager::DumpInputDataFiles(string configFileName){
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

//****************************************************************************************
/**
 * Load input file identifiers and paths from inputFile into manager.
 */
//****************************************************************************************
void PlotManager::LoadInputDataFiles(string configFileName)
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
    AddInputDataFiles(inputIdentifier, allFileNames);
  }
}


//****************************************************************************************
/**
 * Add pre-defined plot to the manager. Plot will be moved and no longer accessible from outside the manager.
 */
//****************************************************************************************
void PlotManager::AddPlot(Plot& plot) {
  mPlots.erase(std::remove_if(mPlots.begin(), mPlots.end(), [plot](Plot& curPlot) mutable { return curPlot.GetUniqueName() == plot.GetUniqueName(); } ), mPlots.end());
  mPlots.push_back(std::move(plot));
}

//****************************************************************************************
/**
 * Dump plots to xml file.
 */
//****************************************************************************************
void PlotManager::DumpPlots(string plotFileName, string figureGroup, vector<string> plotNames)
{
  ptree plotTree;
  for(auto& plot : mPlots){
    if(figureGroup != "")
    {
      if(plot.GetFigureGroup() != figureGroup) continue;
      if(!plotNames.empty()){
        bool found = false;
        for(auto& plotName : plotNames)
        {
          if(plotName == plot.GetName()) found = true;
        }
        if(!found) continue;
      }
    }
    string displayedName = plot.GetName();
    std::replace(displayedName.begin(),displayedName.end(), '.', '_');
    plotTree.put_child(("GROUP::" + plot.GetFigureGroup() + ".PLOT::" + displayedName + gNameGroupSeparator + plot.GetFigureGroup()), plot.GetPropetyTree());
  }
  boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
  write_xml(plotFileName + ".XML", plotTree, std::locale(), settings);
}
void PlotManager::DumpPlot(string plotFileName, string figureGroup, string plotName)
{
  DumpPlots(plotFileName, figureGroup, {plotName});
}

//****************************************************************************************
/**
 * Load plots from xml file.
 */
//****************************************************************************************
void PlotManager::LoadPlots(string plotFileName, string figureGroup, vector<string> plotNames){
  ptree inputTree;
  read_xml(plotFileName + ".XML", inputTree);
  
  for(auto& plotGroupTree : inputTree){
    if(figureGroup != "" && plotGroupTree.first != "GROUP::"+figureGroup) continue;
    for(auto& plotTree : plotGroupTree.second){
      bool found = false;
      if(plotNames.empty()){
        found = true;
      }else{
        for(auto& plotName : plotNames){
          string entryName = "PLOT::" + plotName + gNameGroupSeparator + figureGroup;
          if(entryName.find(plotTree.first) != string::npos) found = true;
        }
      }
      if(!found) continue;
      try{
        Plot plot(plotTree.second);
        AddPlot(plot);
      }catch(...)
      {
        cout << "ERROR: could not generate plot " << plotTree.first << " from xml file." << endl;
      }
    }
  }
}
void PlotManager::LoadPlot(string plotFileName, string figureGroup, string plotName)
{
  LoadPlots(plotFileName, figureGroup, {plotName});
}

//****************************************************************************************
/**
 * Generates plot based on plot template.
 */
//****************************************************************************************
void PlotManager::GeneratePlot(Plot& plot, string outputMode)
{
  bool isPlotStyleBooked = false;
  for(auto& plotStyle : mPlotStyles)
  {
    if(plotStyle.GetName() == plot.GetPlotStyle()) isPlotStyleBooked = true;
  }
  if(!isPlotStyleBooked)
  {
    cout << "ERROR: PlotStyle " << plot.GetPlotStyle() << " is not booked. Cannot create plot " << plot.GetUniqueName() << "." << endl;
    return;
  }
  if(plot.GetFigureGroup() == ""){
    cout << "ERROR: Please specify a figure group!" << endl;
    return;
  }
  
  // in case plot should be savet to file, make sure to delete the old one first
  if(outputMode.find("file") != string::npos && mPlotLedger.find(plot.GetUniqueName()) != mPlotLedger.end()){
    mPlotLedger.erase(plot.GetUniqueName());
  }

  PlotStyle& plotStyle = GetPlotStyle(plot.GetPlotStyle());
  shared_ptr<TCanvas> canvas = PlotGenerator::GeneratePlot(plot, plotStyle, mDataLedger);
  if(!canvas) return;
  
  // if interactive mode is specified, open window instead of saving the plot
  if(outputMode.find("interactive") != string::npos){
    gStyle->SetOptStat(0);
    canvas->WaitPrimitive();
    return;
  }
  string subFolder = plot.GetFigureCategory();
  string fileEnding = ".pdf";
  if(outputMode.find("macro") != string::npos){
    fileEnding = ".C";
  }
  
  string fileName = plot.GetUniqueName();
  
  if(outputMode.find("file") != string::npos){
    mPlotLedger[plot.GetUniqueName()] = canvas;
    return;
  }
  
  if(!mUseUniquePlotNames) fileName = plot.GetName();
  // create output folders and files
  string folderName = mOutputDirectory + "/" + plot.GetFigureGroup();
  if(subFolder != "") folderName += "/" + subFolder;
  gSystem->Exec((string("mkdir -p ") + folderName).c_str());
  canvas->SaveAs((folderName + "/" + fileName + fileEnding).c_str());
}


//****************************************************************************************
/**
 * Creates plots.
 */
//****************************************************************************************
void PlotManager::CreatePlots(string figureGroup, vector<string> plotNames, string outputMode)
{
  map<int, set<int>> requiredData;
  bool saveAll = (figureGroup == "");
  bool saveSpecificPlots = !saveAll && !plotNames.empty();
  vector<Plot*> selectedPlots;

  for(auto& plot : mPlots)
  {
    if(!saveAll && !(plot.GetFigureGroup() == figureGroup)) continue;
    if(saveSpecificPlots && std::find(plotNames.begin(), plotNames.end(), plot.GetName()) == plotNames.end()) continue;
    plotNames.erase(std::remove(plotNames.begin(), plotNames.end(), plot.GetName()), plotNames.end());
    selectedPlots.push_back(&plot);
    
    // check which input data are needed for plot creation and accumulate in requiredData map
    for(auto& padData : plot.GetData())
    {
      for(auto& data : padData.second)
      {
        // if this data entry is not loaded already, add it to required data
        if(mLoadedData[GetNameRegisterID(data->GetInputIdentifier())].find(GetNameRegisterID(data->GetName())) == mLoadedData[GetNameRegisterID(data->GetInputIdentifier())].end())
        {
          requiredData[GetNameRegisterID(data->GetInputIdentifier())].insert(GetNameRegisterID(data->GetName()));
        }
        // for ratios also do the same for denominator
        if(data->GetType() == "ratio" && mLoadedData[GetNameRegisterID(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetDenomIdentifier())].find(GetNameRegisterID(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetDenomName())) == mLoadedData[GetNameRegisterID(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetDenomIdentifier())].end())
        {
        requiredData[GetNameRegisterID(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetDenomIdentifier())].insert(GetNameRegisterID(std::dynamic_pointer_cast<Plot::Ratio>(data)->GetDenomName()));
        }
      }
    }
  }

  // were definitions for all requeseted plots available?
  if(!plotNames.empty()){
    cout << "The following plots are not defined:" << endl;
    for(auto& plotName : plotNames)
    {
      cout << " - " << plotName << endl;
    }
  }
  // now load all required data that was not yet available from input files
  LoadData(requiredData);
  // generate plots and check if data for the plots was available in the files
  bool isAllPlotsCreated = true;
  for(auto plot : selectedPlots){
    if(IsPlotPossible(*plot))
      GeneratePlot(*plot, outputMode);
    else
    {
      isAllPlotsCreated = false;
      cout << "ERROR: Plot '" << plot->GetName() << "' in figure group '" << plot->GetFigureGroup() << "' could not be created." << endl;
    }
  }

  if(isAllPlotsCreated) return;
  
  /*
  // in case some plots could not be created give feedback to user
  cout << endl << "--------------------------------------" << endl;
  cout << "The following data was not available: " << endl;
  for(auto& inputIDTuple : requiredData)
  {
    for(auto& loadedData : mLoadedData[inputIDTuple.first]){
      requiredData[inputIDTuple.first].erase(loadedData);
    }
    if(!inputIDTuple.second.empty())
    {
      cout << endl << "  For input identifier '" << inputIDTuple.first << "':" << endl;
      for(auto& inputDataName : inputIDTuple.second){
        cout << "   - " << inputDataName << endl;
      }
    }
  }
  cout << endl << "--------------------------------------" << endl << endl;
   */

}

void PlotManager::CreatePlot(string plotName, string figureGroup, string outputMode)
{
  CreatePlots(figureGroup, {plotName}, outputMode);
}

void PlotManager::CreatePlotsFromFile(string plotFileName, string figureGroup, vector<string> plotNames, string outputMode){
  LoadPlots(plotFileName, figureGroup, plotNames);
  CreatePlots(figureGroup, plotNames, outputMode);
}

void PlotManager::CreatePlotFromFile(string plotFileName, string figureGroup, string plotName, string outputMode){
  LoadPlots(plotFileName, figureGroup, {plotName});
  CreatePlots(figureGroup, {plotName}, outputMode);
}


int PlotManager::GetNameRegisterID(const string& name)
{
  if( mNameRegister.find(name) == mNameRegister.end())
  {
    mNameRegister[name] = mNameRegister.size();
  }
  return mNameRegister[name];
}

const string& PlotManager::GetNameRegisterName(int nameID)
{
  for(auto& registerTuple : mNameRegister){
    if(registerTuple.second == nameID) return registerTuple.first;
  }
}


//****************************************************************************************
/**
 * Show the data that are currently booked in the manager.
 */
//****************************************************************************************
void PlotManager::ListData()
{
  cout << "Loaded input data:" << endl;
  for(const auto& data : *mDataLedger)
  {
    cout << " - " << ((TNamed*)data)->GetName() << endl;
  }
}

//****************************************************************************************
/**
 * Recursively search for data in file/folder/list  and return copy thereof.
 */
//****************************************************************************************
TObject* PlotManager::GetDataClone(TObject* folder, string dataName)
{
  TList* itemList = nullptr;

  if(folder->InheritsFrom("TDirectoryFile")){
    itemList = ((TDirectoryFile*)folder)->GetListOfKeys();
  }
  else if(folder->InheritsFrom("TList"))
  {
    itemList = (TList*)folder;
  }
  else{
    cout << "ERROR: Dataformat not supported." << endl;
    return nullptr;
  }

  TIter next(itemList);
  TObject* obj = nullptr;   // object found in the file
  TObject* data = nullptr;  // copy of this object that will then be used by manager
  while((obj = next())){
    if(obj->IsA() == TKey::Class()) obj = ((TKey*)obj)->ReadObj();
    if(obj->IsA() == TDirectoryFile::Class()) {
      data = GetDataClone(obj, dataName);
    }else if(obj->IsA() == TList::Class()) {
      data = GetDataClone(obj, dataName);
    }else{
      if(((TNamed*)obj)->GetName() != dataName) continue;
      // todo: select here that only known root input types are beeing processed
      data = obj->Clone("dummyDataName");
      if(data->InheritsFrom("TH1"))
        ((TH1*)data)->SetDirectory(0); // demand ownership for histogram
    }
    if(data) break;
  }
  return data;
}

//****************************************************************************************
/**
 * Recursively search for sub folder in file.
 */
//****************************************************************************************
TObject* PlotManager::FindSubDirectory(TObject* folder, vector<string> subDirs)
{
  if(!folder) return nullptr;
  if(subDirs.size() == 0){
    if(folder->InheritsFrom("TDirectory") || folder->InheritsFrom("TList")){
      return folder;
    }
    else{
      return nullptr;
    }
  }
  TObject* subFolder = nullptr;
  if(folder->InheritsFrom("TDirectory"))
  {
    TKey* key = ((TDirectory*)folder)->FindKey(subDirs[0].c_str());
    if(key){
      subFolder = key->ReadObj();
    }
    else{
      subFolder = ((TDirectory*)folder)->FindObject(subDirs[0].c_str());
    }
  }
  if(folder->InheritsFrom("TList"))
  {
    subFolder = ((TList*)folder)->FindObject(subDirs[0].c_str());
  }
  subDirs.erase(subDirs.begin());
  return FindSubDirectory(subFolder, subDirs);
}

//****************************************************************************************
/**
 * Load required data to the manager.
 */
//****************************************************************************************
void PlotManager::LoadData(map<int, set<int>>& requiredData)
{
  for(auto& inputIdData : requiredData)
  {
    const string& inputIdentifier = GetNameRegisterName(inputIdData.first);
    for(auto& inputFileName : mInputFiles[inputIdentifier])
    {
      
      if(inputFileName.find(".csv") != string::npos){
        
        // extract from path the csv file name that will then become graph name TODO: protect this against wrong usage...
        string graphName = inputFileName.substr(inputFileName.rfind('/') + 1, inputFileName.rfind(".csv") - inputFileName.rfind('/') - 1);
        string delimiter = "\t"; // todo: this must somehow be user definable
        string pattern = "%lg %lg %lg %lg";
        TGraphErrors* graph = new TGraphErrors(inputFileName.c_str(), pattern.c_str(), delimiter.c_str());
        string uniqueName = graphName + gNameGroupSeparator + inputIdentifier;
        ((TNamed*)graph)->SetName(uniqueName.c_str());
        mDataLedger->Add(graph);
        mLoadedData[GetNameRegisterID(inputIdentifier)].insert(GetNameRegisterID(graphName));
        continue;
      }
      else if(inputFileName.find(".root") == string::npos)
      {
        cout << "ERROR: File format of '" << inputFileName << "' not supported." << endl;
        return;
      }

      // first split input file and sub-folders
      std::istringstream ss(inputFileName);
      vector<string> tokens;
      string token;
      while(std::getline(ss, token, ':')) {
          tokens.push_back(token);
      }
      string fileName = tokens[0];
      
      TFile inputFile(fileName.c_str(), "READ");
      if (inputFile.IsZombie()) {
        return;
      }
      TObject* folder = &inputFile;
      
      if(tokens.size() > 1)
      {
        std::istringstream path(tokens[1]);
        vector<string> subDirs;
        string directory;
        while(std::getline(path, directory, '/')) {
           subDirs.push_back(directory);
        }
        folder = FindSubDirectory(folder, subDirs);
        if(!folder){
          cout << "ERROR: subdirectory '" << tokens[1] << "'' not found in '" << fileName << "'."<< endl;
          return;
        }
      }
      for(auto& dataNameID : inputIdData.second)
      {
        TObject* data = GetDataClone(folder, GetNameRegisterName(dataNameID));
        if(data)
        {
          string uniqueName = GetNameRegisterName(dataNameID) + gNameGroupSeparator + inputIdentifier;
          ((TNamed*)data)->SetName(uniqueName.c_str());
//          cout << "   Loaded " << uniqueName << endl;
          mDataLedger->Add(data);
          mLoadedData[inputIdData.first].insert(dataNameID);
        }
      }
    }
  }
}

//****************************************************************************************
/**
 * Load required data to the manager.
 */
//****************************************************************************************
/*
void PlotManager::LoadDataOld(map<string, set<string>>& requiredData)
{
  for(auto& inputIdData : requiredData)
  {
    string inputIdentifier = inputIdData.first;
    for(auto& inputFileName : mInputFiles[inputIdentifier])
    {
      
      if(inputFileName.find(".csv") != string::npos){
        
        // extract from path the csv file name that will then become graph name TODO: protect this against wrong usage...
        string graphName = inputFileName.substr(inputFileName.rfind('/') + 1, inputFileName.rfind(".csv") - inputFileName.rfind('/') - 1);
        string delimiter = "\t"; // todo: this mus somehow be user definable
        string pattern = "%lg %lg %lg %lg";
        TGraphErrors* graph = new TGraphErrors(inputFileName.c_str(), pattern.c_str(), delimiter.c_str());
        string uniqueName = graphName + gNameGroupSeparator + inputIdentifier;
        ((TNamed*)graph)->SetName(uniqueName.c_str());
        mDataLedger->Add(graph);
        mLoadedData[inputIdentifier].insert(graphName);
        continue;
      }
      else if(inputFileName.find(".root") == string::npos)
      {
        cout << "ERROR: File format of '" << inputFileName << "' not supported." << endl;
        return;
      }

      // first split input file and sub-folders
      std::istringstream ss(inputFileName);
      vector<string> tokens;
      string token;
      while(std::getline(ss, token, ':')) {
          tokens.push_back(token);
      }
      string fileName = tokens[0];
      
      TFile inputFile(fileName.c_str(), "READ");
      if (inputFile.IsZombie()) {
        return;
      }
      TObject* folder = &inputFile;
      
      if(tokens.size() > 1)
      {
        std::istringstream path(tokens[1]);
        vector<string> subDirs;
        string directory;
        while(std::getline(path, directory, '/')) {
           subDirs.push_back(directory);
        }
        folder = FindSubDirectory(folder, subDirs);
        if(!folder){
          cout << "ERROR: subdirectory '" << tokens[1] << "'' not found in '" << fileName << "'."<< endl;
          return;
        }
      }
      
      for(auto& dataName : inputIdData.second)
      {
        TObject* data = GetDataClone(folder, dataName);
        if(data)
        {
          string uniqueName = dataName + gNameGroupSeparator + inputIdentifier;
          ((TNamed*)data)->SetName(uniqueName.c_str());
          //cout << "Loaded " << uniqueName << endl;
          mDataLedger->Add(data);
          mLoadedData[inputIdentifier].insert(dataName);
        }
      }
    }
  }
}
 */


void PlotManager::PrintStatus()
{
  cout << "=================== Manager Status ===========================" << endl;
  cout << "Input identifiers: " << mInputFiles.size() << endl;
  cout << "Plot styles: " << mPlotStyles.size() << endl;
  cout << "Plot definitions: " << mPlots.size() << endl;
  cout << "Loaded data inputs: " << mDataLedger->GetEntries() << endl;
  cout << "==============================================================" << endl;

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
        TObject* obj = mDataLedger->FindObject(dataName.c_str());
        if(!obj) return false;
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


} // END namespace PlottingFramework
