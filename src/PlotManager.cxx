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

#include "PlotManager.h"

using namespace PlottingFramework;
namespace PlottingFramework{

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
  
  mSaveToRootFile = false;
  mOutputFileName = "ResultPlots.root";
  
  mUseUniquePlotNames = false;
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
    if(mSaveToRootFile == true){
      TFile outputFile(mOutputFileName.c_str(), "RECREATE");
      if (outputFile.IsZombie()){
        return;
      }
      outputFile.cd();
      int nPlots{0};
      for(auto& plotTuple : mPlotLedger){
        auto canvas = plotTuple.second;
        string uniqueName = plotTuple.first;
        size_t delimiterPos = uniqueName.find(gNameGroupSeparator.c_str());
        string plotName = uniqueName.substr(0, delimiterPos);
        string subfolder = uniqueName.substr(delimiterPos + gNameGroupSeparator.size());
        std::replace(subfolder.begin(),subfolder.end(), ':', '/');
        if(!outputFile.GetDirectory(subfolder.c_str(), kFALSE, "cd"))
        {
          outputFile.mkdir(subfolder.c_str());
        }
        outputFile.cd(subfolder.c_str());
        canvas->Write(plotName.c_str());
        nPlots++;
      }
      outputFile.Close();
      INFO("Saved {} plots to file \"{}\".", nPlots, mOutputFileName);
    }
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
    WARNING("Replacing input identifier {}.", inputIdentifier);
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
  write_xml(gSystem->ExpandPathName(configFileName.c_str()), inputFileTree, std::locale(), settings);
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
    read_xml(gSystem->ExpandPathName(configFileName.c_str()), inputFileTree);
  }catch(...){
    ERROR("Cannot load file {}.", configFileName);
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
    string displayedName = plot.GetUniqueName();
    std::replace(displayedName.begin(),displayedName.end(), '.', '_');
    std::replace(displayedName.begin(),displayedName.end(), '/', '|');
    plotTree.put_child(("GROUP::" + plot.GetFigureGroup() + ".PLOT::" + displayedName), plot.GetPropetyTree());
  }
  boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
  write_xml(gSystem->ExpandPathName(plotFileName.c_str()), plotTree, std::locale(), settings);
}
void PlotManager::DumpPlot(string plotFileName, string figureGroup, string plotName)
{
  DumpPlots(plotFileName, figureGroup, {plotName});
}

//****************************************************************************************
/**
 * Read and cache plots defined in xml file.
 */
//****************************************************************************************
ptree& PlotManager::ReadPlotTemplatesFromFile(string& plotFileName){
  if (mPlotTemplateCache.find(plotFileName) == mPlotTemplateCache.end())
  {
    ptree tempTree;
    read_xml(gSystem->ExpandPathName(plotFileName.c_str()), tempTree);
    mPlotTemplateCache[plotFileName] = std::move(tempTree);
  }
  return mPlotTemplateCache[plotFileName];
}

//****************************************************************************************
/**
 * Generates plot based on plot template.
 */
//****************************************************************************************
void PlotManager::GeneratePlot(Plot& plot, string outputMode)
{
  // if plot already exists, delete the old one first
  if( mPlotLedger.find(plot.GetUniqueName()) != mPlotLedger.end())
  {
    ERROR("Plot {} was already created. Replacing it.", plot.GetUniqueName());
    mPlotLedger.erase(plot.GetUniqueName());
  }
  
  bool isPlotStyleBooked = false;
  for(auto& plotStyle : mPlotStyles)
  {
    if(plotStyle.GetName() == plot.GetPlotStyle()) isPlotStyleBooked = true;
  }
  if(!isPlotStyleBooked)
  {
    ERROR("PlotStyle {} is not booked. Cannot create plot {}.", plot.GetPlotStyle(), plot.GetUniqueName());
    return;
  }
  if(plot.GetFigureGroup() == ""){
    ERROR("No figure gropu was specified.");
    return;
  }
  
  if(outputMode.find("file") != string::npos){
    mSaveToRootFile = true;
  }
  
  PlotStyle& plotStyle = GetPlotStyle(plot.GetPlotStyle());
  shared_ptr<TCanvas> canvas = PlottingTools::GeneratePlot(plot, plotStyle, mDataLedger);
  if(!canvas) return;
  LOG("Created plot \"{}\".", canvas->GetName());
  
  // if interactive mode is specified, open window instead of saving the plot
  if(outputMode.find("interactive") != string::npos){
    
    mPlotLedger[plot.GetUniqueName()] = canvas;
    mPlotViewHistory.push_back(plot.GetUniqueName());
    int currPlotIndex = mPlotViewHistory.size()-1;
    
    // move new canvas to position of previous window
    int curXpos;
    int curYpos;
    const int windowOffsetY = 22; // might depend on os, probably comes from move operation called via TVirtualX interface
    if(currPlotIndex > 0){
      curXpos = mPlotLedger[mPlotViewHistory[currPlotIndex-1]]->GetWindowTopX();
      curYpos = mPlotLedger[mPlotViewHistory[currPlotIndex-1]]->GetWindowTopY();
      canvas->SetWindowPosition(curXpos, curYpos - windowOffsetY);
    }
    while(gROOT->GetSelectedPad() && !gSystem->ProcessEvents()) {
      if(canvas->GetEvent() == kButton1Double){
        curXpos = canvas->GetWindowTopX();
        curYpos = canvas->GetWindowTopY();
        TRootCanvas* canvasWindow = ((TRootCanvas*)canvas->GetCanvasImp());
        canvasWindow->UnmapWindow();
        bool forward = ((double)canvas->GetEventX()/(double)canvas->GetWw() > 0.5);
        if(forward){
          if(currPlotIndex == mPlotViewHistory.size()-1) break;
          currPlotIndex++;
        }
        else{
          if(currPlotIndex != 0)
            currPlotIndex--;
        }
        canvas = mPlotLedger[mPlotViewHistory[currPlotIndex]];
        canvas->SetWindowPosition(curXpos, curYpos - windowOffsetY);
        ((TRootCanvas*)canvas->GetCanvasImp())->MapRaised();
      }
      gSystem->Sleep(20);
    }
    return;
  }
  
  string subFolder = plot.GetFigureCategory();
  string fileEnding = ".pdf";
  if(outputMode.find("macro") != string::npos){
    fileEnding = ".C";
  }
  if(outputMode.find("bitmap") != string::npos){
    fileEnding = ".png";
  }
  if(outputMode.find("eps") != string::npos){
    fileEnding = ".eps";
  }
  
  string fileName = plot.GetUniqueName();
  
  if(outputMode.find("file") != string::npos){
    mPlotLedger[plot.GetUniqueName()] = canvas;
    return;
  }
  
  if(!mUseUniquePlotNames) fileName = plot.GetName();
  std::replace(fileName.begin(),fileName.end(), '/', '_');
  std::replace(fileName.begin(),fileName.end(), ':', '_');

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
void PlotManager::CreatePlots(string figureGroup, string figureCategory, vector<string> plotNames, string outputMode)
{
  map<int, set<int>> requiredData;
  bool saveAll = (figureGroup == "");
  bool saveSpecificPlots = !saveAll && !plotNames.empty();
  vector<Plot*> selectedPlots;
  
  // first determine which data needs to be loaded
  for(auto& plot : mPlots)
  {
    if(!saveAll && !(plot.GetFigureGroup() == figureGroup && plot.GetFigureCategory() == figureCategory)) continue;
    if(saveSpecificPlots && std::find(plotNames.begin(), plotNames.end(), plot.GetName()) == plotNames.end()) continue;
    if(!plotNames.empty()) plotNames.erase(std::remove(plotNames.begin(), plotNames.end(), plot.GetName()), plotNames.end());
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
    for(auto& plotName : plotNames)
    {
      WARNING("Could not find plot \"{}\" in group \"{}\"", plotName, figureGroup + ((figureCategory != "") ? ":" + figureCategory : ""));
    }
  }
  
  // now read the data
  for(auto& inputIDTuple : requiredData){
    vector<string> dataNames;
    vector<string> uniqueDataNames;
    for(auto& dataNameID : inputIDTuple.second)
    {
      dataNames.push_back(GetNameRegisterName(dataNameID));
      uniqueDataNames.push_back(GetNameRegisterName(dataNameID) + gNameGroupSeparator + GetNameRegisterName(inputIDTuple.first));
    }
    ReadDataFromCSVFiles(*mDataLedger, mInputFiles[GetNameRegisterName(inputIDTuple.first)], GetNameRegisterName(inputIDTuple.first));
    ReadDataFromFiles(*mDataLedger, mInputFiles[GetNameRegisterName(inputIDTuple.first)], dataNames, uniqueDataNames);
    // TODO: Loaded data has to be updated!!
  }
  
  
  // generate plots
  for(auto plot : selectedPlots){
    if(IsPlotPossible(*plot))
      GeneratePlot(*plot, outputMode);
    else
    {
      ERROR("Plot \"{}\" in figure group \"{}\" could not be created.", plot->GetName(), plot->GetFigureGroup()  + ((plot->GetFigureCategory() != "") ? ":" + plot->GetFigureCategory() : ""));
    }
  }
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
  return mOutputDirectory; //FIXME: this nonsense-fix is just to get rid of the warning! Re-think returning ref to member..
}

//****************************************************************************************
/**
 * Function to find plots in file via regexp match of user inputs
 */
//****************************************************************************************
void PlotManager::ExtractPlotsFromFile(string plotFileName, vector<string> figureGroupsWithCategoryUser, vector<string> plotNamesUser, string mode)
{
  int nFoundPlots = 0;
  bool isSearchRequest = (mode == "find") ? true : false;
  vector< std::pair<std::regex, std::regex> > groupCategoryRegex;
  for(auto& figureGroupWithCategoryUser : figureGroupsWithCategoryUser)
  {
    // by default select all groups and all categories
    string group = ".*";
    string category = ".*";
    vector<string> groupCat = splitString(figureGroupWithCategoryUser, ':');
    if(groupCat.size() > 0 && !groupCat[0].empty()) group = groupCat[0];
    if(groupCat.size() > 1 && !groupCat[1].empty()) category = groupCat[1];
    if(groupCat.size() > 2)
    {
      ERROR("Do not put \":\" in your regular expressions! Colons should be used solely to separate figureGroup and figureCategory");
      return;
    }
    std::regex groupRegex(group);
    std::regex categoryRegex(category);
    groupCategoryRegex.push_back(std::make_pair(groupRegex, categoryRegex));
  }

  vector< std::regex > plotNamesRegex;
  for(auto& plotNameUser : plotNamesUser)
  {
    std::regex plotNameRegex(plotNameUser);
    plotNamesRegex.push_back(plotNameRegex);
  }
  
  ptree& inputTree = ReadPlotTemplatesFromFile(plotFileName);
  for(auto& plotGroupTree : inputTree){
    // first filter by group
    string groupIdentifier = plotGroupTree.first.substr(string("GROUP::").size());
    if(std::find_if(groupCategoryRegex.begin(), groupCategoryRegex.end(), [groupIdentifier](std::pair<std::regex, std::regex>& curGroupCatRegex){return std::regex_match(groupIdentifier, curGroupCatRegex.first);}) == groupCategoryRegex.end())
    {
      continue;
    }

    for(auto& plotTree : plotGroupTree.second){
      string plotName = plotTree.second.get<string>("name");
      string figureGroup = plotTree.second.get<string>("figureGroup");
      string figureCategory = plotTree.second.get<string>("figureCategory");
      
      if(std::find_if(groupCategoryRegex.begin(), groupCategoryRegex.end(), [figureGroup, figureCategory](std::pair<std::regex, std::regex>& curGroupCatRegex){return std::regex_match(figureGroup, curGroupCatRegex.first) && std::regex_match(figureCategory, curGroupCatRegex.second);}) == groupCategoryRegex.end())
      {
        continue;
      }

      if(std::find_if(plotNamesRegex.begin(), plotNamesRegex.end(), [plotName](std::regex& curPlotNameRegex){return std::regex_match(plotName, curPlotNameRegex);}) == plotNamesRegex.end())
      {
        continue;
      }
      nFoundPlots++;
      if(isSearchRequest)
      {
        PRINT("-- found plot \033[1;32m{}\033[0m in group \033[1;33m{}\033[0m", plotName, figureGroup + ((figureCategory != "") ? ":" + figureCategory : ""));
      }
      else{
        try{
          Plot plot(plotTree.second);
          AddPlot(plot);
        }catch(...)
        {
          ERROR("Could not generate plot {} from XML file.", plotTree.first);
        }
      }
    }
  }
  if(nFoundPlots == 0){
    ERROR("Requested plots are not defined.");
  }else{
    INFO("Found {} plots matching the request.", nFoundPlots);
  }
  if(!isSearchRequest && mode != "load")
  {
     // now produce the loaded plots
      CreatePlots("", "", {}, mode);
  }
}

// Helper function to split a string
vector<string> PlotManager::splitString(string argString, char deliminator)
{
  vector<string> arguments;
  string currArg;
  std::istringstream argStream(argString);
  while(std::getline(argStream, currArg, deliminator)) {
    arguments.push_back(currArg);
  }
  return arguments;
}


//****************************************************************************************
/**
 * Show the data that are currently booked in the manager.
 */
//****************************************************************************************
void PlotManager::ListData()
{
  PRINT("Loaded input data:");
  for(const auto& data : *mDataLedger)
  {
    PRINT(" - ", ((TNamed*)data)->GetName());
  }
}

//****************************************************************************************
/**
 * Read data from csv file.
 */
//****************************************************************************************
void PlotManager::ReadDataFromCSVFiles(TObjArray& outputDataArray, vector<string> fileNames, string inputIdentifier){
  
  for(auto& inputFileName : fileNames)
  {
    if(inputFileName.find(".csv") == string::npos) continue;
    // extract from path the csv file name that will then become graph name TODO: protect this against wrong usage...
    string graphName = inputFileName.substr(inputFileName.rfind('/') + 1, inputFileName.rfind(".csv") - inputFileName.rfind('/') - 1);
    string delimiter = "\t"; // todo: this must somehow be user definable
    string pattern = "%lg %lg %lg %lg";
    TGraphErrors* graph = new TGraphErrors(inputFileName.c_str(), pattern.c_str(), delimiter.c_str());
    string uniqueName = graphName + gNameGroupSeparator + inputIdentifier;
    ((TNamed*)graph)->SetName(uniqueName.c_str());
    outputDataArray.Add(graph);
  }
}

//****************************************************************************************
/**
 * Recursively read data from root file.
 */
//****************************************************************************************
void PlotManager::ReadDataFromFiles(TObjArray& outputDataArray, vector<string> fileNames, vector<string> dataNames, vector<string> newDataNames){
  
  // first determine if which sub-folders are required by the user
  set<string> dataSubSpecs;
  for(auto& dataName : dataNames)
  {
    auto pathPos = dataName.find_last_of("/");
    string subSpec;
    if(pathPos != string::npos){
      subSpec = dataName.substr(0, pathPos);
    }
    dataSubSpecs.insert(subSpec);
  }
  
  
  

  for(auto& inputFileName : fileNames)
  {
    if(dataNames.empty()) break;
    if(inputFileName.find(".root") == string::npos) continue;
    // first check if sub-structure is defined
    std::istringstream ss(inputFileName);
    vector<string> tokens;
    string token;
    while(std::getline(ss, token, ':')) {
      tokens.push_back(token);
    }
    string fileName = tokens[0];
            
    TFile inputFile(fileName.c_str(), "READ");
    if (inputFile.IsZombie()) {
      ERROR("Input file {} not found.", fileName);
      break;
    }
    
    TObject* folder = &inputFile;
    
    // find top level entry point for this input file
    if(tokens.size() > 1)
    {
      std::istringstream path(tokens[1]);
      vector<string> subDirs;
      string directory;
      while(std::getline(path, directory, '/')) {
        subDirs.push_back(directory);
      }
      // append subspecification from input name
      folder = FindSubDirectory(folder, subDirs);
      if(!folder){
        ERROR("Subdirectory \"{}\" not found in \"{}\".", tokens[1], fileName);
        return;
      }
    }
    
    // there might be subfolders explicitly specified by the user
    vector<string> remainingDataNames;
    vector<string> remainingNewDataNames;
    for(auto& dataSubSpec : dataSubSpecs)
    {
      vector<string> curDataNames;
      vector<string> curNewDataNames;
      
      for(int i = 0; i < dataNames.size(); i++)
      {
        if(dataNames[i] == "") continue;
        auto pathPos = dataNames[i].find_last_of("/");
        string tempSubSpec;
        if(pathPos != string::npos){
          tempSubSpec = dataNames[i].substr(0, pathPos);
        }
        if(tempSubSpec == dataSubSpec)
        {
          curDataNames.push_back(std::move(dataNames[i]));
          curNewDataNames.push_back(std::move(newDataNames[i]));
        }
      }
      if(curDataNames.empty()) continue;
      // now find subdirectory belonging to subspec
      std::istringstream path(dataSubSpec);
      vector<string> subDirs;
      string directory;
      while(std::getline(path, directory, '/')) {
        subDirs.push_back(directory);
      }
      // append subspecification from input name
      TObject* subfolder = FindSubDirectory(folder, subDirs);
      if(subfolder){
        // recursively traverse the file and look for input files
        ReadData(subfolder, outputDataArray, curDataNames, curNewDataNames);
        if(subfolder != &inputFile) {delete subfolder; subfolder = nullptr;}
      }
      std::move(std::begin(curDataNames), std::end(curDataNames), std::back_inserter(remainingDataNames));
      std::move(std::begin(curNewDataNames), std::end(curNewDataNames), std::back_inserter(remainingNewDataNames));
      }
    
    // get rid of empty strings left after moving
    dataNames.erase(std::remove(dataNames.begin(), dataNames.end(), ""), dataNames.end());
    newDataNames.erase(std::remove(newDataNames.begin(), newDataNames.end(), ""), newDataNames.end());
    
    // now move unfound dataNames back to the original vector, so they can be searched in next file
    std::move(std::begin(remainingDataNames), std::end(remainingDataNames), std::back_inserter(dataNames));
    std::move(std::begin(remainingNewDataNames), std::end(remainingNewDataNames), std::back_inserter(newDataNames));
  }

  if(!dataNames.empty()){
    WARNING("Not all required inputs could be found.");
    PRINT_SEPARATOR;
    PRINT(" Data:");
    for(auto& dataName : dataNames) PRINT("  - {}", dataName);
    PRINT(" not found in any of the following files:");
    for(auto& inputFileName : fileNames) PRINT("  - {}", inputFileName);
    PRINT_SEPARATOR;
  }
}

//****************************************************************************************
/**
 * Recursively search for sub folder in file.
 */
//****************************************************************************************
TObject* PlotManager::FindSubDirectory(TObject* folder, vector<string> subDirs)
{
  if(!folder) return nullptr;
  bool deleteFolder = true;
  if(folder->InheritsFrom("TFile")) deleteFolder = false;
  
  if(subDirs.size() == 0){
    if(folder->InheritsFrom("TDirectory") || folder->InheritsFrom("TCollection")){
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
    deleteFolder = false;
  }
  else if(folder->InheritsFrom("TCollection"))
  {
    subFolder = ((TCollection*)folder)->FindObject(subDirs[0].c_str());
    if(subFolder){
      ((TCollection*)subFolder)->SetOwner();
      // if subfolder is part of list, remove it first to avoid double deletion
      ((TCollection*)folder)->Remove(subFolder);
    }
  }
  if(deleteFolder) {
    delete folder;
  }
  if(!subFolder) return nullptr;
  subDirs.erase(subDirs.begin());
  return FindSubDirectory(subFolder, subDirs);
}

//****************************************************************************************
/**
 * Recursively reads data from folder / list and adds it to output data array. found dataNames are remeoved from the vectors
 */
//****************************************************************************************
void PlotManager::ReadData(TObject* folder, TObjArray& outputDataArray, vector<string>& dataNames, vector<string>& newDataNames)
{
  if(dataNames.empty()) return; // nothing to do...

  // consistency check for name vectors
  if(!newDataNames.empty() && newDataNames.size() != dataNames.size()){
    ERROR("newDataNames vector has the wrong size");
    return;
  }

  // first determine what is the required subspecification (path) that needs to be prepended
  auto pathPos = dataNames[0].find_last_of("/");
  string subSpec;
  if(pathPos != string::npos){
    subSpec = dataNames[0].substr(0, pathPos);
  }
  if(subSpec != "") subSpec = subSpec + "/";

  
  TCollection* itemList = nullptr;
  if(folder->InheritsFrom("TDirectory")){
    itemList = ((TDirectoryFile*)folder)->GetListOfKeys();
  }
  else if(folder->InheritsFrom("TCollection"))
  {
    itemList = (TCollection*)folder;
  }
  else{
    ERROR("Dataformat not supported.");
    return;
  }
  itemList->SetOwner();
  
  TIter iterator = itemList->begin();
  TObject* obj = *iterator;
  bool deleteObject;
  bool removeFromList;
  
  while(iterator != itemList->end()){
    obj = *iterator;
    deleteObject = true;
    removeFromList = true;
    
    // read actual object to memory when traversing a directory
    if(obj->IsA() == TKey::Class()){
      string className = ((TKey*)obj)->GetClassName();
      string keyName = ((TKey*)obj)->GetName();

      bool isTraversable = className.find("TDirectory") != string::npos || className.find("TList") != string::npos || className.find("TObjArray") != string::npos;
      if(isTraversable || std::find(dataNames.begin(), dataNames.end(), subSpec+keyName) != dataNames.end())
      {
        obj = ((TKey*)obj)->ReadObj();
        removeFromList = false;
      }else{
        ++iterator;
        continue;
      }
    }
    
    // in case this object is directory or list, repeat the same for this substructure
    if(obj->InheritsFrom("TDirectory") || obj->InheritsFrom("TCollection")) {
      ReadData(obj, outputDataArray, dataNames, newDataNames);
    }else{
      auto it = std::find(dataNames.begin(), dataNames.end(), subSpec+((TNamed*)obj)->GetName());
      if(it != dataNames.end()){
        // TODO: select here that only known root input types are beeing processed
        if(obj->InheritsFrom("TH1"))
          ((TH1*)obj)->SetDirectory(0); // demand ownership for histogram
        
        // re-name data
        if(!newDataNames.empty()){
          auto vectorIndex = it - dataNames.begin();
          ((TNamed*)obj)->SetName((newDataNames[vectorIndex]).c_str());
          newDataNames.erase(std::find(newDataNames.begin(), newDataNames.end(), newDataNames[vectorIndex]));
        }
        dataNames.erase(it);
        outputDataArray.Add(obj);
        deleteObject = false;
      }
    }
    
    // increase iterator before removing objects from collection
    ++iterator;
    if(removeFromList) {
      if(itemList->Remove(obj) == nullptr)
      {
        ERROR("Could not remove item {} ({}) from collection {}.", ((TNamed*)obj)->GetName(), (void*)obj, itemList->GetName());
      }
    }
    if(deleteObject){
      delete obj;
    }
    if(dataNames.empty()) break;
  }
}

//****************************************************************************************
/**
 * Print manager status.
 */
//****************************************************************************************
void PlotManager::PrintStatus()
{
  INFO("Printing manager status is not implemented yet.");
}


//****************************************************************************************
/**
 * Print available plot styles.
 */
//****************************************************************************************
void PlotManager::ListPlotStyles()
{
  INFO("Listing PlotStyles is not implemented yet.");
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
  }
}

PlotStyle& PlotManager::GetPlotStyle(string plotStyleName)
{
  for(auto& plotStyle : mPlotStyles)
  {
    if(plotStyle.GetName() == plotStyleName) return plotStyle;
  }
  ERROR("PlotStyle named {} not found.", plotStyleName);
  return GetPlotStyle("default"); // if style not found return default style
}

} // end namespace PlottingFramework
