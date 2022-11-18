// SciRooPlot
//
// Copyright (C) 2019-2022  Mario Krüger
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
#include "PlotManager.h"
#include "PlotPainter.h"
#include "Logging.h"
#include "Helpers.h"

// std dependencies
#include <regex>
#include <filesystem>

// boost dependencies
#include <boost/property_tree/xml_parser.hpp>

// root dependencies
#include "TApplication.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TError.h"
#include "TFile.h"
#include "TRootCanvas.h"
#include "TCanvas.h"
#include "TKey.h"
#include "TH1.h"
#include "TGraphErrors.h"
#include "TFolder.h"
#include "TPave.h"

namespace SciRooPlot
{

//**************************************************************************************************
/**
 * Constructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::PlotManager() : mApp(new TApplication("MainApp", 0, nullptr)), mOutputFileName("ResultPlots.root")
{
  TQObject::Connect("TGMainFrame", "CloseWindow()", "TApplication", gApplication, "Terminate()");
  gErrorIgnoreLevel = kWarning;

  // determine OS dependent offset between window and frame
  // (GetWindowTopY gives the current coordinates of the window, but SetWindowPosition moves the frame instead of the window)
  TCanvas dummyCanvas("dummyCanvas", "dummyCanvas", 1, 1);
  static_cast<TRootCanvas*>(dummyCanvas.GetCanvasImp())->UnmapWindow();
  dummyCanvas.SetCanvasSize(1, 1);
  dummyCanvas.SetWindowPosition(50, 50);
  mWindowOffsetY = dummyCanvas.GetWindowTopY() - static_cast<TRootCanvas*>(dummyCanvas.GetCanvasImp())->GetY();
}

//**************************************************************************************************
/**
 * Destructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::~PlotManager()
{
  if (mSaveToRootFile) SavePlotsToFile();
}

//**************************************************************************************************
/**
 * Save stored plots to .root file.
 */
//**************************************************************************************************
void PlotManager::SavePlotsToFile() const
{
  if (!mPlotLedger.empty()) {
    TFile outputFile((mOutputDirectory + "/" + mOutputFileName).data(), "RECREATE");
    if (outputFile.IsZombie()) {
      return;
    }
    outputFile.cd();
    uint32_t nPlots{0u};
    for (auto& [uniqueName, canvas] : mPlotLedger) {
      size_t delimiterPos = uniqueName.find(gNameGroupSeparator.data());
      string plotName = uniqueName.substr(0, delimiterPos);
      string subfolder = uniqueName.substr(delimiterPos + gNameGroupSeparator.size());
      std::replace(subfolder.begin(), subfolder.end(), ':', '/');
      if (!outputFile.GetDirectory(subfolder.data(), kFALSE, "cd")) {
        outputFile.mkdir(subfolder.data());
      }
      outputFile.cd(subfolder.data());
      canvas->Write(plotName.data());
      ++nPlots;
    }
    outputFile.Close();
    INFO("Saved {} plots to file {}.", nPlots, mOutputFileName);
  }
}

//**************************************************************************************************
/**
 * Properly delete all loaded root raw data.
 */
//**************************************************************************************************
void PlotManager::ClearDataBuffer()
{
  mDataBuffer.clear();
};

//**************************************************************************************************
/**
 * Sets path for output files. Plots wil be stored in hierarchical structure according to figure groups and categories.
 */
//**************************************************************************************************
void PlotManager::SetOutputDirectory(const string& path)
{
  mOutputDirectory = path;
}

//**************************************************************************************************
/**
 * Whether or not to append the figure group and category to the plot name when saved to file.
 */
//**************************************************************************************************
void PlotManager::SetUseUniquePlotNames(bool useUniquePlotNames)
{
  mUseUniquePlotNames = useUniquePlotNames;
}

//**************************************************************************************************
/**
 * The output file name for saving the plots into one single root file.
 */
//**************************************************************************************************
void PlotManager::SetOutputFileName(const string& fileName)
{
  mOutputFileName = fileName;
}

//**************************************************************************************************
/**
 * Define input file paths for user defined unique inputIdentifier.
 */
//**************************************************************************************************
void PlotManager::AddInputDataFiles(const string& inputIdentifier, const vector<string>& inputFilePathList)
{
  if (mInputFiles.find(inputIdentifier) != mInputFiles.end()) {
    WARNING("Replacing input identifier {}.", inputIdentifier);
  }
  mInputFiles[inputIdentifier] = inputFilePathList;
}
void PlotManager::AddInputDataFile(const string& inputIdentifier, const string& inputFilePath)
{
  vector<string> inputFilePathList = {inputFilePath};
  AddInputDataFiles(inputIdentifier, inputFilePathList);
}

//**************************************************************************************************
/**
 * Dump input file identifiers and paths that are currently defined in the manager to a config file.
 */
//**************************************************************************************************
void PlotManager::DumpInputDataFiles(const string& configFileName) const
{
  ptree inputFileTree;
  for (auto& inFileTuple : mInputFiles) {
    ptree filesOfIdentifier;
    for (auto& fileName : inFileTuple.second) {
      filesOfIdentifier.add("FILE", fileName);
    }
    inputFileTree.put_child(inFileTuple.first, filesOfIdentifier);
  }
  using boost::property_tree::xml_writer_settings;
  xml_writer_settings<std::string> settings('\t', 1);
  using boost::property_tree::write_xml;
  write_xml(expand_path(configFileName), inputFileTree, std::locale(), settings);
}

//**************************************************************************************************
/**
 * Load input file identifiers and paths from inputFile into manager.
 */
//**************************************************************************************************
void PlotManager::LoadInputDataFiles(const string& configFileName)
{
  ptree inputFileTree;
  try {
    using boost::property_tree::read_xml;
    read_xml(expand_path(configFileName), inputFileTree);
    INFO("Reading input files from {}.", configFileName);
  } catch (...) {
    ERROR("Cannot load file {}.", configFileName);
    return;
  }
  for (auto& inputPair : inputFileTree) {
    const string& inputIdentifier = inputPair.first;
    set<string> allFileNames;
    for (auto& fileEntry : inputPair.second) {
      string fileOrDirName = expand_path(fileEntry.second.get_value<string>());
      if (str_contains(fileOrDirName, ".root", true) || str_contains(fileOrDirName, ".csv", true)) {
        allFileNames.insert(fileOrDirName);
      } else if (std::filesystem::is_directory(fileOrDirName)) {
        for (auto& file : std::filesystem::recursive_directory_iterator(fileOrDirName)) {
          if (file.path().extension() == ".root" || file.path().extension() == ".csv") {
            allFileNames.insert(file.path().string());
          }
        }
      }
    }
    AddInputDataFiles(inputIdentifier, {allFileNames.begin(), allFileNames.end()});
  }
}

//**************************************************************************************************
/**
 * Add pre-defined plot to the manager. Plot will be moved and no longer accessible from outside the manager.
 */
//**************************************************************************************************
void PlotManager::AddPlot(Plot& plot)
{
  if (plot.GetFigureGroup() == "PLOT_TEMPLATES") {
    ERROR("You cannot use reserved group name 'PLOT_TEMPLATES'!");
  }
  mPlots.erase(std::remove_if(mPlots.begin(), mPlots.end(),
                              [plot](Plot& curPlot) mutable {
                                bool removePlot = curPlot.GetUniqueName() == plot.GetUniqueName();
                                if (removePlot) WARNING("Plot {} in {} already exists and will be replaced.", curPlot.GetName(), curPlot.GetFigureGroup());
                                return removePlot;
                              }),
               mPlots.end());
  mPlots.push_back(std::move(plot));
}

//**************************************************************************************************
/**
 * Add template for plots, that share some common properties.
 */
//**************************************************************************************************
void PlotManager::AddPlotTemplate(Plot& plotTemplate)
{
  plotTemplate.SetFigureGroup("PLOT_TEMPLATES");
  mPlotTemplates.erase(std::remove_if(mPlotTemplates.begin(), mPlotTemplates.end(),
                                      [plotTemplate](Plot& curPlotTemplate) mutable {
                                        bool removePlot = curPlotTemplate.GetUniqueName() == plotTemplate.GetUniqueName();
                                        if (removePlot) WARNING("Plot template {} already exists and will be replaced.", curPlotTemplate.GetName());
                                        return removePlot;
                                      }),
                       mPlotTemplates.end());
  mPlotTemplates.push_back(std::move(plotTemplate));
}

//**************************************************************************************************
/**
 * Dump plots to xml file.
 */
//**************************************************************************************************
void PlotManager::DumpPlots(const string& plotFileName, const string& figureGroup,
                            const vector<string>& plotNames) const
{
  set<string> usedTemplates;
  std::for_each(mPlots.begin(), mPlots.end(), [&usedTemplates](auto& plot) {
    if (plot.GetPlotTemplateName()) usedTemplates.insert(*plot.GetPlotTemplateName());
  });

  ptree plotTree;
  for (const vector<Plot>& plots : {std::ref(mPlotTemplates), std::ref(mPlots)}) {
    for (const Plot& plot : plots) {

      if (plot.GetFigureGroup() == "PLOT_TEMPLATES" && usedTemplates.find(plot.GetName()) == usedTemplates.end()) {
        continue;
      } else if (!figureGroup.empty()) {
        if (plot.GetFigureGroup() != figureGroup) continue;
        if (!plotNames.empty()) {
          bool found = false;
          for (auto& plotName : plotNames) {
            if (plotName == plot.GetName()) found = true;
          }
          if (!found) continue;
        }
      }
      string displayedName = plot.GetUniqueName();
      std::replace(displayedName.begin(), displayedName.end(), '.', '_');
      std::replace(displayedName.begin(), displayedName.end(), '/', '|');
      plotTree.put_child(("GROUP::" + plot.GetFigureGroup() + ".PLOT::" + displayedName), plot.GetPropertyTree());
    }
  }
  using boost::property_tree::xml_writer_settings;
  xml_writer_settings<std::string> settings('\t', 1);
  using boost::property_tree::write_xml;
  write_xml(expand_path(plotFileName), plotTree, std::locale(), settings);
  INFO("Wrote plot definitions to {}.", plotFileName);
}
void PlotManager::DumpPlot(const string& plotFileName, const string& figureGroup,
                           const string& plotName) const
{
  DumpPlots(plotFileName, figureGroup, {plotName});
}

//**************************************************************************************************
/**
 * Read and cache plots defined in xml file.
 */
//**************************************************************************************************
ptree& PlotManager::ReadPlotTemplatesFromFile(const string& plotFileName)
{
  if (mPropertyTreeCache.find(plotFileName) == mPropertyTreeCache.end()) {
    try {
      using boost::property_tree::read_xml;
      read_xml(expand_path(plotFileName), mPropertyTreeCache[plotFileName]);
      INFO("Reading plot definitions from {}.", plotFileName);
    } catch (...) {
      ERROR("Cannot load file {}.", plotFileName);
      std::exit(EXIT_FAILURE);
    }
  }
  return mPropertyTreeCache[plotFileName];
}

//**************************************************************************************************
/**
 * Generates plot based on plot template.
 */
//**************************************************************************************************
bool PlotManager::GeneratePlot(const Plot& plot, const string& outputMode)
{
  // if plot already exists, delete the old one first
  if (mPlotLedger.find(plot.GetUniqueName()) != mPlotLedger.end()) {
    ERROR("Plot {} was already created. Replacing it.", plot.GetUniqueName());
    mPlotLedger.erase(plot.GetUniqueName());
  }
  if (plot.GetFigureGroup().empty()) {
    ERROR("No figure group was specified for plot {}.", plot.GetName());
    return false;
  }
  if (outputMode == "file") {
    mSaveToRootFile = true;
  }
  Plot fullPlot = plot;
  if (plot.GetPlotTemplateName()) {
    const string& plotTemplateName = *plot.GetPlotTemplateName();
    auto iterator = std::find_if(
      mPlotTemplates.begin(), mPlotTemplates.end(),
      [&](Plot& plotTemplate) { return plotTemplate.GetName() == plotTemplateName; });
    if (iterator != mPlotTemplates.end()) {
      fullPlot = *iterator + plot;
    } else {
      WARNING("Could not find plot template named {}.", plotTemplateName);
    }
  }
  PlotPainter painter;
  bool isInteractiveMode = (outputMode == "interactive");
  bool isMacroMode = (outputMode == "macro");
  gROOT->SetBatch(!isInteractiveMode && !isMacroMode);
  shared_ptr<TCanvas> canvas{painter.GeneratePlot(fullPlot, mDataBuffer)};
  if (!canvas) return false;
  LOG("Created " GREEN_ "{}" _END " from group " YELLOW_ "{}" _END ".", fullPlot.GetName(), fullPlot.GetFigureGroup() + ((fullPlot.GetFigureCategory()) ? "/" + *fullPlot.GetFigureCategory() : ""));

  // if interactive mode is specified, open window instead of saving the plot
  if (isInteractiveMode) {
    mPlotLedger[plot.GetUniqueName()] = canvas;
    mPlotViewHistory.push_back(&plot.GetUniqueName());
    uint32_t curPlotIndex{static_cast<uint32_t>(mPlotViewHistory.size() - 1)};

    // move new canvas to position of previous window
    int32_t curXpos{};
    int32_t curYpos{};
    if (curPlotIndex > 0) {
      curXpos = mPlotLedger[*mPlotViewHistory[curPlotIndex - 1]]->GetWindowTopX();
      curYpos = mPlotLedger[*mPlotViewHistory[curPlotIndex - 1]]->GetWindowTopY();
      canvas->SetWindowPosition(curXpos, curYpos - mWindowOffsetY);
      static_cast<TRootCanvas*>(mPlotLedger[*mPlotViewHistory[curPlotIndex - 1]]->GetCanvasImp())->UnmapWindow();
    }
    canvas->Show();
    bool boxClicked = false;
    while (!gSystem->ProcessEvents() && gROOT->GetSelectedPad()) {
      bool isClick = canvas->GetEvent() == kButton1Double;
      bool isValidKey = canvas->GetEvent() == kKeyPress && (canvas->GetEventX() == 'a' || canvas->GetEventX() == 's');
      auto selectedBox = dynamic_cast<TPave*>(canvas->GetSelected());
      if (isClick && selectedBox) {
        if (!boxClicked) INFO("Current position of {}: ({:.3g}, {:.3g}).", selectedBox->GetName(), selectedBox->GetX1NDC(), selectedBox->GetY2NDC());
        boxClicked = true;
      } else if (isClick || isValidKey) {
        curXpos = canvas->GetWindowTopX();
        curYpos = canvas->GetWindowTopY();
        bool forward = false;
        if (isValidKey) {
          forward = (canvas->GetEventX() == 's');
        } else {
          forward = ((double_t)canvas->GetEventX() / (double_t)canvas->GetWw() > 0.5);
        }
        if (forward) {
          if (curPlotIndex == mPlotViewHistory.size() - 1) break;
          ++curPlotIndex;
        } else {
          if (curPlotIndex == 0) std::exit(EXIT_FAILURE);
          --curPlotIndex;
        }
        static_cast<TRootCanvas*>(canvas->GetCanvasImp())->UnmapWindow();
        canvas = mPlotLedger[*mPlotViewHistory[curPlotIndex]];
        canvas->SetWindowPosition(curXpos, curYpos - mWindowOffsetY);
        canvas->Show();
      } else {
        boxClicked = false;
      }
      gSystem->Sleep(20);
    }
    return true;
  }

  if (mOutputDirectory.empty()) {
    ERROR("No output directory was specified. Cannot save plot.");
    return true;
  }
  auto parentDir = (mOutputDirectory.back() == '/') ? std::filesystem::path(mOutputDirectory).parent_path().parent_path() : std::filesystem::path(mOutputDirectory).parent_path();
  if (!std::filesystem::exists(parentDir)) {
    ERROR("Parent path {} of output directory does not exist.", parentDir.string());
    return true;
  }

  if (outputMode == "file") {
    mPlotLedger[plot.GetUniqueName()] = canvas;
    return true;
  }

  bool isGif = false;
  static string gifName;
  static string gifFolderName;
  string gifRepRate = "+50"; // number of centiseconds between frames

  string fileEnding;
  if (outputMode == "pdf") {
    fileEnding = ".pdf";
  } else if (isMacroMode) {
    fileEnding = ".C";

    // object names are converted to variable names and therefore must not contain '/'
    // TODO: this should be fixed in ROOT itself as it does not create valid cpp code otherwise
    std::function<void(TPad*)> cleanNames;
    cleanNames = [&](TPad* pad) {
      TIter next(pad->GetListOfPrimitives());
      TObject* object = nullptr;
      while ((object = next())) {
        string saveName = object->GetName();
        std::replace(saveName.begin(), saveName.end(), '/', '_');
        static_cast<TNamed*>(object)->SetName(saveName.data());
        if (object->InheritsFrom(TPad::Class())) {
          cleanNames(static_cast<TPad*>(object));
        }
      }
    };
    cleanNames(canvas.get());
    string saveName = canvas->GetName();
    std::replace(saveName.begin(), saveName.end(), '/', '_');
    canvas->SetName(saveName.data());

  } else if (outputMode == "png") {
    fileEnding = ".png";
  } else if (outputMode == "eps") {
    fileEnding = ".eps";
  } else if (outputMode == "svg") {
    fileEnding = ".svg";
  } else if (str_contains(outputMode, "gif")) {
    fileEnding = ".gif";
    isGif = true;
    if (auto delimPos = outputMode.find("+"); delimPos != string::npos) {
      gifRepRate = outputMode.substr(delimPos);
    }
  }

  if (fileEnding.empty()) {
    ERROR("No valid output format was specified. Cannot save plot.");
    return true;
  }

  string fileName = (mUseUniquePlotNames) ? plot.GetUniqueName() : plot.GetName();
  std::replace(fileName.begin(), fileName.end(), '/', '_');

  // create output folders and files
  string folderName = mOutputDirectory + "/" + plot.GetFigureGroup();
  if (plot.GetFigureCategory()) folderName += "/" + *plot.GetFigureCategory();
  string fullName = folderName + "/" + fileName + fileEnding;

  if (isGif) {
    if (gifName.empty()) {
      gSystem->Unlink(fullName.data());
      LOG("Saving gif {}", fullName);
      fullName += gifRepRate;
      gifName = fullName;
      gifFolderName = folderName;
    } else {
      fullName = gifName;
      folderName = gifFolderName;
    }
  }
  gSystem->Exec((string("mkdir -p ") + folderName).data());
  canvas->SaveAs(fullName.data());
  // reset TCandle range options to their default values after drawing data
  TCandle::SetBoxRange(0.5);
  TCandle::SetWhiskerRange(0.75);
  return true;
}

//**************************************************************************************************
/**
 * Creates plots.
 */
//**************************************************************************************************
void PlotManager::CreatePlots(const string& figureGroup, const string& figureCategory,
                              vector<string> plotNames, const string& outputMode)
{
  // first determine which data needs to be loaded
  vector<Plot*> selectedPlots;
  map<int32_t, set<int32_t>> requiredData;

  for (auto& plot : mPlots) {
    if (!figureGroup.empty() && !(plot.GetFigureGroup() == figureGroup)) {
      continue;
    } else if (!figureCategory.empty() && !(plot.GetFigureCategory() && *plot.GetFigureCategory() == figureCategory)) {
      continue;
    } else if (!plotNames.empty() && std::find(plotNames.begin(), plotNames.end(), plot.GetName()) == plotNames.end()) {
      continue;
    }

    if (!plotNames.empty()) {
      plotNames.erase(std::remove(plotNames.begin(), plotNames.end(), plot.GetName()), plotNames.end());
    }
    selectedPlots.push_back(&plot);

    // determine which input data are needed for plots
    for (auto& [padID, pad] : plot.GetPads()) {
      for (auto& data : pad.GetData()) {
        mDataBuffer[data->GetInputID()][data->GetName()];
        if (data->GetType() == "ratio") {
          const auto& ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
          mDataBuffer[ratio->GetDenomIdentifier()][ratio->GetDenomName()];
        }
      }
    }
  }

  // were definitions for all requested plots available?
  if (!plotNames.empty()) {
    for (auto& plotName : plotNames) {
      WARNING("Could not find plot " GREEN_ "{}" _END " in group " YELLOW_ "{}" _END ".", plotName, figureGroup + ((!figureCategory.empty()) ? "/" + figureCategory : ""));
    }
  }

  if (!FillBuffer()) PrintBufferStatus(true);
  // generate plots
  for (auto plot : selectedPlots) {
    if (!GeneratePlot(*plot, outputMode))
      ERROR("Plot " GREEN_ "{}" _END " from group " YELLOW_ "{}" _END " could not be created.", plot->GetName(), plot->GetFigureGroup() + ((plot->GetFigureCategory()) ? "/" + *plot->GetFigureCategory() : ""));
  }
}

//**************************************************************************************************
/**
 * Fills all the nodes defined in buffer hash map with data read from files.
 */
//**************************************************************************************************
bool PlotManager::FillBuffer()
{
  bool success = true;
  for (auto& [inputID, buffer] : mDataBuffer) {
    unordered_map<string, vector<string>> requiredData; // subdir, names
    for (auto& [dataName, dataPtr] : buffer) {
      if (dataPtr) continue;
      auto pathPos = dataName.find_last_of("/");
      string path;
      string name = dataName;
      if (pathPos != string::npos) {
        path = name.substr(0, pathPos);
        name.erase(0, pathPos + 1);
      }
      requiredData[std::move(path)].push_back(std::move(name));
    }

    // open all input files belonging to the current inputID and extract the data
    for (auto& inputFileName : mInputFiles[inputID]) {
      if (requiredData.empty()) break;
      if (str_contains(inputFileName, ".csv", true)) {
        string graphName = inputFileName.substr(inputFileName.rfind('/') + 1, inputFileName.rfind(".csv") - inputFileName.rfind('/') - 1);
        ReadDataCSV(inputFileName, graphName, inputID);
        vector<string>& names = requiredData[""];
        names.erase(std::remove_if(names.begin(), names.end(), [&](auto& name) { return name == graphName; }), names.end());
        if (names.empty()) requiredData.erase("");
      }
      if (!str_contains(inputFileName, ".root", true)) continue;
      // check if only a sub-folder in input file should be searched
      auto fileNamePath = split_string(inputFileName, ':');
      string& fileName = fileNamePath[0];

      if (!std::filesystem::exists(fileName)) {
        WARNING("Input file {} not found.", fileName);
        continue;
      }
      TFile inputFile(fileName.data(), "READ");
      if (inputFile.IsZombie()) {
        WARNING("Cannot open input file {}.", fileName);
        continue;
      }

      TObject* folder = &inputFile;

      // find top level entry point for this input file
      if (fileNamePath.size() > 1) {
        auto filePath = split_string(fileNamePath[1], '/');
        // append sub-specification from input name
        folder = FindSubDirectory(folder, filePath);
        if (!folder) {
          ERROR("Subdirectory {} not found in file {}.", fileNamePath[1], fileName);
          return false;
        }
      }

      vector<string> emptySubDirs;
      for (auto& [pathStr, names] : requiredData) {
        auto path = split_string(pathStr, '/');
        TObject* subfolder = FindSubDirectory(folder, path);
        if (subfolder) {
          // recursively traverse the file and look for input files
          string prefix = (pathStr.empty()) ? "" : pathStr + "/";
          string suffix = gNameGroupSeparator + inputID;
          ReadData(subfolder, names, prefix, suffix, inputID);
          // in case a subdirectory was opened, properly delete it
          if (!path.empty() && subfolder != &inputFile) {
            delete subfolder;
            subfolder = nullptr;
          }
        }
        if (names.empty()) emptySubDirs.push_back(pathStr);
      }
      // finally also remove top level folder
      if (folder != &inputFile) {
        delete folder;
        folder = nullptr;
      }

      for (auto& pathStr : emptySubDirs) {
        requiredData.erase(pathStr);
      }
    }
    success &= requiredData.empty();
  }
  return success;
}

//**************************************************************************************************
/**
 * Show which data could and could not be found.
 */
//**************************************************************************************************
void PlotManager::PrintBufferStatus(bool missingOnly) const
{
  INFO("===============================================");
  if (missingOnly) {
    INFO("================= Missing Data ================");
  } else {
    INFO("================= Data Buffer =================");
  }
  uint32_t nNeededData{};
  uint32_t nAvailableData{};
  for (auto& [inputID, buffer] : mDataBuffer) {
    bool printInputID = true;
    for (auto& [dataName, dataPtr] : buffer) {
      ++nNeededData;
      bool show = missingOnly ? (dataPtr == nullptr) : true;
      if (dataPtr) ++nAvailableData;
      if (show) {
        if (printInputID) INFO("{}", inputID);
        printInputID = false;
        INFO(" - {}{}{}", (dataPtr) ? GREEN_ : RED_, dataName, _END);
      }
    }
  }
  INFO("Found {}/{} required input data.", nAvailableData, nNeededData);
  INFO("===============================================");
}

//**************************************************************************************************
/**
 * Show which plots are currently loaded in the framework.
 */
//**************************************************************************************************
void PlotManager::PrintLoadedPlots() const
{
  INFO("===============================================");
  INFO("================ Loaded Plots =================");

  string figureGroup;
  for (auto& plot : mPlots) {
    uint8_t nInputs{};
    if (figureGroup != plot.GetFigureGroup()) {
      figureGroup = plot.GetFigureGroup();
      INFO("{}", figureGroup);
    }
    INFO(" - {}{}", plot.GetName(), (plot.GetFigureCategory()) ? "" : " (" + *plot.GetFigureCategory() + ")");
    INFO("     ndata = {}", plot.GetDataCount());
  }
  INFO("{} plots were loaded.", mPlots.size());
  INFO("===============================================");
}

//**************************************************************************************************
/**
 * Recursively reads data from folder / list and adds it to output data array. Found dataNames are removed from the vectors.
 */
//**************************************************************************************************
void PlotManager::ReadData(TObject* folder, vector<string>& dataNames, const string& prefix, const string& suffix, const string& inputID)
{
  TCollection* itemList = nullptr;
  if (folder->InheritsFrom(TDirectory::Class())) {
    itemList = static_cast<TDirectoryFile*>(folder)->GetListOfKeys();
  } else if (folder->InheritsFrom(TFolder::Class())) {
    itemList = static_cast<TFolder*>(folder)->GetListOfFolders();
  } else if (folder->InheritsFrom(TCollection::Class())) {
    itemList = static_cast<TCollection*>(folder);
  } else {
    ERROR("Data-format {} not supported.", folder->ClassName());
    return;
  }
  itemList->SetOwner();

  // first match should always be the one in current level; traverse deeper only if not found
  for (bool traverse : {false, true}) {

    TIter iterator = itemList->begin();
    TObject* obj{};
    bool deleteObject;
    bool removeFromList;

    while (iterator != itemList->end()) {
      obj = *iterator;
      deleteObject = true;
      removeFromList = true;

      string curDataName; // name of current key or data
      // read actual object to memory when traversing a directory
      if (obj->IsA() == TKey::Class()) {
        string className = static_cast<TKey*>(obj)->GetClassName();
        curDataName = obj->GetName();

        bool isTraversable = str_contains(className, "TDirectory") || str_contains(className, "TFolder") || str_contains(className, "TList") || str_contains(className, "THashList") || str_contains(className, "TObjArray");
        if ((traverse && isTraversable) || std::find(dataNames.begin(), dataNames.end(), curDataName) != dataNames.end()) {
          obj = static_cast<TKey*>(obj)->ReadObj();
          removeFromList = false;
        } else {
          ++iterator;
          continue;
        }
      }

      // in case this object is directory or list, repeat the same for this substructure
      if (obj->InheritsFrom(TDirectory::Class()) || obj->InheritsFrom(TFolder::Class()) || obj->InheritsFrom(TCollection::Class())) {
        if (traverse) {
          ReadData(obj, dataNames, prefix, suffix, inputID);
        } else if (removeFromList) {
          removeFromList = false;
          deleteObject = false;
        }
      } else {
        // the key name supersedes the actual data name (in case they are different when written to file via h->Write("myKeyName"))
        if (curDataName.empty()) curDataName = obj->GetName();
        if (auto it = std::find(dataNames.begin(), dataNames.end(), curDataName); it != dataNames.end()) {
          if (obj->InheritsFrom(TH1::Class())) static_cast<TH1*>(obj)->SetDirectory(0); // demand ownership for histogram
          itemList->Remove(obj);
          dataNames.erase(it);
          string fullName = prefix + curDataName;
          static_cast<TNamed*>(obj)->SetName((fullName + suffix).data());
          mDataBuffer[inputID][fullName].reset(obj);
          removeFromList = false;
          deleteObject = false;
        }
      }

      // increase iterator before removing objects from collection
      ++iterator;
      if (removeFromList) {
        if (!itemList->Remove(obj)) {
          ERROR("Could not remove item {} ({}) from collection {}.", obj->GetName(), static_cast<void*>(obj), itemList->GetName());
        }
      }
      if (deleteObject) {
        delete obj;
      }
      if (dataNames.empty()) return;
    }
  }
}

//**************************************************************************************************
/**
 * Read data from csv file.
 */
//**************************************************************************************************
void PlotManager::ReadDataCSV(const string& inputFileName, const string& graphName, const string& inputIdentifier)
{
  // extract from path the csv file name that will then become graph name TODO: protect this against wrong usage...
  string delimiter = "\t"; // TODO: this must somehow be user definable
  string pattern = "%lg %lg %lg %lg";
  TGraphErrors* graph = new TGraphErrors(inputFileName.data(), pattern.data(), delimiter.data());
  string uniqueName = graphName + gNameGroupSeparator + inputIdentifier;
  graph->SetName(uniqueName.data());
  mDataBuffer[inputIdentifier][graphName].reset(graph);
}

//**************************************************************************************************
/**
 * Recursively search for sub folder in file.
 */
//**************************************************************************************************
TObject* PlotManager::FindSubDirectory(TObject* folder, vector<string>& subDirs) const
{
  if (!folder) return nullptr;
  bool deleteFolder = true;
  if (folder->InheritsFrom(TFile::Class())) deleteFolder = false;

  if (subDirs.empty()) {
    if (folder->InheritsFrom(TDirectory::Class()) || folder->InheritsFrom(TFolder::Class()) || folder->InheritsFrom(TCollection::Class())) {
      return folder;
    } else {
      return nullptr;
    }
  }
  TObject* subFolder{nullptr};
  if (folder->InheritsFrom(TDirectory::Class())) {
    TKey* key = static_cast<TDirectory*>(folder)->FindKey(subDirs[0].data());
    if (key) {
      subFolder = key->ReadObj();
    } else {
      subFolder = static_cast<TDirectory*>(folder)->FindObject(subDirs[0].data());
    }
    deleteFolder = false;
  } else if (folder->InheritsFrom(TCollection::Class()) || folder->InheritsFrom(TFolder::Class())) {
    subFolder = static_cast<TCollection*>(folder)->FindObject(subDirs[0].data());
    if (subFolder) {
      static_cast<TCollection*>(subFolder)->SetOwner();
      // if subfolder is part of list, remove it first to avoid double deletion
      static_cast<TCollection*>(folder)->Remove(subFolder);
    }
  }
  if (deleteFolder) {
    delete folder;
  }
  if (!subFolder) return nullptr;
  subDirs.erase(subDirs.begin());
  return FindSubDirectory(subFolder, subDirs);
}

//**************************************************************************************************
/**
 * Function to find plots in file via regex match of user inputs
 */
//**************************************************************************************************
void PlotManager::ExtractPlotsFromFile(const string& plotFileName,
                                       const string& mode,
                                       const string& plotName,
                                       const string& group,
                                       const string& category)
{
  uint32_t nFoundPlots{};
  bool isSearchRequest = (mode == "find");

  std::regex groupRegex{group};
  std::regex categoryRegex{category};
  std::regex plotNameRegex{plotName};

  ptree& inputTree = ReadPlotTemplatesFromFile(plotFileName);
  for (auto& plotGroupTree : inputTree) {
    // first filter by group
    string groupIdentifier = plotGroupTree.first.substr(string("GROUP::").size());
    bool isTemplate = (groupIdentifier == "PLOT_TEMPLATES");

    if (!isTemplate && !std::regex_match(groupIdentifier, groupRegex)) {
      continue;
    }

    for (auto& plotTree : plotGroupTree.second) {
      if (isTemplate) {
        Plot plot(plotTree.second);
        AddPlotTemplate(plot);
        continue;
      }

      const string& plotName = plotTree.second.get<string>("name");
      const string& figureGroup = plotTree.second.get<string>("figure_group");
      optional<string> figureCategoryOpt;
      read_from_tree(plotTree.second, figureCategoryOpt, "figure_category");
      string figureCategory = (figureCategoryOpt) ? *figureCategoryOpt : "";

      if (!std::regex_match(figureCategory, categoryRegex) || !std::regex_match(plotName, plotNameRegex)) {
        continue;
      }

      ++nFoundPlots;
      if (isSearchRequest) {
        INFO(" - " GREEN_ "{}" _END " in group " YELLOW_ "{}" _END, plotName, figureGroup + ((!figureCategory.empty()) ? "/" + figureCategory : ""));
      } else {
        try {
          Plot plot(plotTree.second);
          AddPlot(plot);
        } catch (...) {
          ERROR("Could not generate plot {} from XML file.", plotTree.first);
        }
      }
    }
  }
  if (nFoundPlots == 0) {
    ERROR("Found no plots matching the request " GREEN_ "{}" _END " in " YELLOW_ "{}/{}" _END ".", plotName, group, category);
  } else {
    INFO("Found {} plot{} matching the request.", nFoundPlots, (nFoundPlots == 1) ? "" : "s");
  }
  if (!isSearchRequest && mode != "load") {
    // now produce the loaded plots
    CreatePlots("", "", {}, mode);
  }
}

//****************************************************************************************
/**
 * Defines a set of standard plot templates with quadratic axis frame and consistent layout among them.
 * The following pre-defined layouts are available: 1d, 2d, 1d_ratio and 1d_3panels.
 * By specifying a screen resolution in dpi, the size of the plot displayed in interactive mode (or when saved as bitmap) can be modified.
 * For multiple of 72dpi the resulting plot will have the best agreement with the pdf version.
 */
//****************************************************************************************
Plot PlotManager::GetPlotTemplate(const string& plotTemplateName, double_t screenResolution)
{
  // info: the PDF backend of ROOT can only create plots with resolution of 72 dpi and maximum sizes defined by the A4 format (20cm x 26cm -> 567px x 737px)
  int32_t pixelBase = int32_t(std::round(screenResolution * 567. / 72.)); // 567p / 72dpi == 20cm / 2.54in/cm (final pdf size)
  double_t frameSize = 0.81;                                              // size of (quadratic) axis frame in percentage of pixelBase
  double_t axisMargin = 0.16;                                             // margins of x and y axes in units relative to pixelBase side length (corresponds to the space in bottom and left of the plot to the axis frame)
  double_t defaultTextSize = 0.04;                                        // default text size relative to pixelBase
  double_t axisTitleSize = 0.05;                                          // text size of axis titles relative to pixelBase
  double_t wideSideScale = 1.3;                                           // for asymmetric plots: scale of the larger side wrt. pixelBase (corresponds to the maximum / optimum of 26cm / 20cm)

  double_t markerSize = 1.4;
  uint8_t lineWidth = 5;

  vector<int16_t> goodColors = {kBlack, kBlue + 1, kRed + 1, kYellow + 1,
                                kMagenta - 4, kGreen + 3, kOrange + 1,
                                kViolet - 3, kCyan + 2, kPink + 3, kTeal - 7,
                                kMagenta + 1, kPink + 8, kCyan - 6,
                                kMagenta, kRed + 2, kGreen + 2,
                                kOrange + 2, kMagenta + 2, kYellow + 3,
                                kGray + 2, kBlue + 2, kYellow + 2,
                                kRed, kBlue, kMagenta + 3,
                                kGreen + 4, 28, 8, 15, 17, 12};

  if (plotTemplateName == "1d") {
    // -----------------------------------------------------------------------
    Plot plotTemplate(plotTemplateName, "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(pixelBase, pixelBase, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultMarkerColors(goodColors);
    plotTemplate[0].SetDefaultLineColors(goodColors);
    plotTemplate[0].SetDefaultFillColors(goodColors);
    plotTemplate[0].SetDefaultFillStyles({0});
    plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
    plotTemplate[0].SetDefaultLineStyles({kSolid});
    plotTemplate[0].SetDefaultTextFont(42);
    plotTemplate[0].SetDefaultTextSize(defaultTextSize);
    plotTemplate[0].SetDefaultMarkerSize(markerSize);
    plotTemplate[0].SetDefaultLineWidth(lineWidth);
    plotTemplate[0].SetDefaultDrawingOptionGraph(points);
    plotTemplate[0].SetTransparent();
    plotTemplate[0].SetMargins(1. - (frameSize + axisMargin), axisMargin, axisMargin, 1. - (frameSize + axisMargin));
    plotTemplate[0]['X'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plotTemplate[0]['Y'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[1].SetPosition(0., 0., 1., 1.);
    return plotTemplate;
  } // -----------------------------------------------------------------------

  if (plotTemplateName == "1d_ratio") {
    // -----------------------------------------------------------------------
    Plot plotTemplate(plotTemplateName, "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(pixelBase, int32_t(std::round(wideSideScale * pixelBase)), true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultMarkerColors(goodColors);
    plotTemplate[0].SetDefaultLineColors(goodColors);
    plotTemplate[0].SetDefaultFillColors(goodColors);
    plotTemplate[0].SetDefaultFillStyles({0});
    plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
    plotTemplate[0].SetDefaultLineStyles({kSolid});
    plotTemplate[0].SetDefaultMarkerSize(markerSize);
    plotTemplate[0].SetDefaultLineWidth(lineWidth);
    plotTemplate[0].SetDefaultDrawingOptionGraph(points);
    plotTemplate[0].SetDefaultTextFont(42);
    plotTemplate[0].SetTransparent();
    double_t ratioFraction = 1. - (1. - axisMargin) / wideSideScale;
    plotTemplate[1].SetPosition(0., ratioFraction, 1., 1.);
    plotTemplate[2].SetPosition(0., 0., 1., ratioFraction);
    plotTemplate[1].SetMargins(1. - frameSize / (1 - axisMargin), 0., axisMargin, 1. - (frameSize + axisMargin));
    plotTemplate[2].SetMargins(0., 1. / (wideSideScale * ratioFraction) * axisMargin, axisMargin, 1. - (frameSize + axisMargin));
    plotTemplate[1].SetDefaultTextSize(1. / (wideSideScale * (1. - ratioFraction)) * defaultTextSize);
    plotTemplate[2].SetDefaultTextSize(1. / (wideSideScale * ratioFraction) * defaultTextSize);
    plotTemplate[1]['X'].SetTitleSize(0.).SetLabelSize(0.).SetOppositeTicks().SetNoExponent();
    plotTemplate[1]['Y'].SetTitleSize(1. / (wideSideScale * (1 - ratioFraction)) * axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[2]['X'].SetTitleSize(1. / (wideSideScale * ratioFraction) * axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plotTemplate[2]['Y'].SetTitleSize(1. / (wideSideScale * ratioFraction) * axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[2]['Y'].SetTitleOffset(0.6); // FIXME: for some reason the ROOT automatic title offset determination does not work for the lower plot
    plotTemplate[2].SetRefFunc("1");
    plotTemplate[2]['X'].SetTickLength(0.06);
    plotTemplate[2]['Y'].SetNumDivisions(305).SetTitleCenter();
    return plotTemplate;
  } // -----------------------------------------------------------------------

  if (plotTemplateName == "2d") {
    // -----------------------------------------------------------------------
    Plot plotTemplate(plotTemplateName, "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(int32_t(std::round(wideSideScale * pixelBase)), pixelBase, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultDrawingOptionHist2d(colz);
    plotTemplate[0].SetDefaultTextFont(42);
    plotTemplate[0].SetDefaultTextSize(defaultTextSize);
    plotTemplate[0].SetDefaultMarkerSize(markerSize);
    plotTemplate[0].SetDefaultLineWidth(lineWidth);
    plotTemplate[0].SetTransparent();
    plotTemplate[0].SetMargins(1. - (frameSize + axisMargin), axisMargin, axisMargin / wideSideScale, 1. - (frameSize + axisMargin) / wideSideScale);
    plotTemplate[0]['X'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plotTemplate[0]['Y'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[0]['Z'].SetTitleSize(axisTitleSize).SetMaxDigits(3).SetTitleOffset(1.5);
    plotTemplate[0].SetRedrawAxes();
    plotTemplate[1].SetPosition(0., 0., 1., 1.);
    return plotTemplate;
  } // -----------------------------------------------------------------------

  if (plotTemplateName == "1d_3panels") {
    // -----------------------------------------------------------------------
    // extension of the 1d template with adjacing panels
    // first calculate some quantities for the size conversions
    double_t totalPixelHeight = pixelBase;
    double_t leftMarginPixel = std::round(axisMargin * totalPixelHeight);
    double_t rightMarginPixel = std::round((1 - (frameSize + axisMargin)) * totalPixelHeight);
    double_t widthFramePixel = totalPixelHeight - leftMarginPixel - rightMarginPixel;
    double_t totalPixelWidth = 3 * widthFramePixel + leftMarginPixel + rightMarginPixel;
    // margins
    double_t leftMarginPad1 = leftMarginPixel / (leftMarginPixel + widthFramePixel);
    double_t rightMarginPad3 = rightMarginPixel / (rightMarginPixel + widthFramePixel);
    // positions
    double_t x1 = (widthFramePixel + leftMarginPixel) / totalPixelWidth;
    double_t x2 = x1 + widthFramePixel / totalPixelWidth;
    // text scaling factors (for the second and third pad the text will be calculated wrt. the widht instead of the hight (since it is smaller) and therefore need additional scaling
    double_t textScalePad2 = totalPixelHeight / widthFramePixel;
    double_t textScalePad3 = totalPixelHeight / (widthFramePixel + rightMarginPixel);
    // tick length is specified relative to pad width -> since width is different for the three pads, need adjustments to have consistent lengths between the pads
    // gauge it to the first pad -> scale by relative width decrease of pad 2 and 3 wrt pad 1
    double_t tickLengthPad1 = 0.03;
    double_t totalPixelWidthPad1 = widthFramePixel + leftMarginPixel;
    double_t tickLengthPad2 = tickLengthPad1 * widthFramePixel / totalPixelWidthPad1;
    double_t tickLengthPad3 = tickLengthPad1 * (widthFramePixel + rightMarginPixel) / totalPixelWidthPad1;

    Plot plotTemplate(plotTemplateName, "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(totalPixelWidth, totalPixelHeight, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultFillStyles({0});
    plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
    plotTemplate[0].SetDefaultLineStyles({kSolid});
    plotTemplate[0].SetDefaultTextFont(42);
    plotTemplate[0].SetDefaultTextSize(defaultTextSize);
    // plotTemplate[0].SetDefaultLineWidth(1); // adjusted to make it look the same in lower pdf resolution
    plotTemplate[0].SetDefaultMarkerSize(markerSize);
    plotTemplate[0].SetDefaultLineWidth(lineWidth);
    plotTemplate[0].SetDefaultDrawingOptionGraph(points);
    plotTemplate[0].SetTransparent();
    plotTemplate[1].SetPosition(0., 0., x1, 1.);
    plotTemplate[2].SetPosition(x1, 0., x2, 1.);
    plotTemplate[3].SetPosition(x2, 0., 1., 1.);
    plotTemplate[1].SetMargins(1. - (frameSize + axisMargin), axisMargin, leftMarginPad1, 0.0);
    plotTemplate[2].SetMargins(1. - (frameSize + axisMargin), axisMargin, 0.0, 0.0);
    plotTemplate[3].SetMargins(1. - (frameSize + axisMargin), axisMargin, 0.0, rightMarginPad3);
    plotTemplate[0]['X'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plotTemplate[0]['Y'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[2].SetDefaultTextSize(textScalePad2 * defaultTextSize);
    plotTemplate[2]['X'].SetTitleSize(textScalePad2 * axisTitleSize);
    plotTemplate[2]['Y'].SetTitleSize(textScalePad2 * axisTitleSize);
    plotTemplate[3].SetDefaultTextSize(textScalePad3 * defaultTextSize);
    plotTemplate[3]['X'].SetTitleSize(textScalePad3 * axisTitleSize);
    plotTemplate[3]['Y'].SetTitleSize(textScalePad3 * axisTitleSize);
    plotTemplate[2]['Y'].SetTitle("").SetTitleSize(0.).SetLabelSize(0.);
    plotTemplate[3]['Y'].SetTitle("").SetTitleSize(0.).SetLabelSize(0.);
    plotTemplate[2]['X'].SetTickLength(tickLengthPad2);
    plotTemplate[3]['X'].SetTickLength(tickLengthPad3);
    return plotTemplate;
  } // -----------------------------------------------------------------------

  ERROR("Unknown plot template {}.", plotTemplateName);
  return Plot();
}
} // end namespace SciRooPlot
