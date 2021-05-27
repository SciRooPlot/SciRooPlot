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

namespace PlottingFramework
{

//**************************************************************************************************
/**
 * Constructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::PlotManager() : mApp(new TApplication("MainApp", 0, nullptr)), mSaveToRootFile(false), mOutputFileName("ResultPlots.root"), mUseUniquePlotNames(false)
{
  TQObject::Connect("TGMainFrame", "CloseWindow()", "TApplication", gApplication, "Terminate()");
  gErrorIgnoreLevel = kWarning;
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
void PlotManager::SavePlotsToFile()
{
  if (!mPlotLedger.empty()) {
    TFile outputFile(mOutputFileName.data(), "RECREATE");
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
    INFO(R"(Saved {} plots to file "{}".)", nPlots, mOutputFileName);
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
 * Sets path for output files.
 * @param outputFileName: Path to the output directory
 */
//**************************************************************************************************
void PlotManager::SetOutputDirectory(const string& path)
{
  mOutputDirectory = path;
}
void PlotManager::SetUseUniquePlotNames(bool useUniquePlotNames)
{
  mUseUniquePlotNames = useUniquePlotNames;
}
void PlotManager::SetOutputFileName(const string& fileName)
{
  mOutputFileName = fileName;
}

//**************************************************************************************************
/**
 * Define input file paths for user defined unique inputIdentifier.
 */
//**************************************************************************************************
void PlotManager::AddInputDataFiles(const string& inputIdentifier,
                                    const vector<string>& inputFilePathList)
{
  if (mInputFiles.find(inputIdentifier) != mInputFiles.end()) {
    WARNING(R"(Replacing input identifier "{}".)", inputIdentifier);
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
void PlotManager::DumpInputDataFiles(const string& configFileName)
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
  } catch (...) {
    ERROR(R"(Cannot load file "{}".)", configFileName);
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
  if (plot.GetFigureGroup() == "TEMPLATES") {
    ERROR(R"(You cannot use reserved group name "TEMPLATES"!)");
  }
  mPlots.erase(std::remove_if(mPlots.begin(), mPlots.end(),
                              [plot](Plot& curPlot) mutable {
                                bool removePlot = curPlot.GetUniqueName() == plot.GetUniqueName();
                                if (removePlot) WARNING(R"(Plot "{}" in "{}" already exists and will be replaced.)", curPlot.GetName(), curPlot.GetFigureGroup());
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
  plotTemplate.SetFigureGroup("TEMPLATES");
  mPlotTemplates.erase(std::remove_if(mPlotTemplates.begin(), mPlotTemplates.end(),
                                      [plotTemplate](Plot& curPlotTemplate) mutable {
                                        bool removePlot = curPlotTemplate.GetUniqueName() == plotTemplate.GetUniqueName();
                                        if (removePlot) WARNING(R"(Plot template "{}" already exists and will be replaced.)", curPlotTemplate.GetName());
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
                            const vector<string>& plotNames)
{
  set<string> usedTemplates;
  std::for_each(mPlots.begin(), mPlots.end(), [&usedTemplates](auto& plot) {
    if (plot.GetPlotTemplateName()) usedTemplates.insert(*plot.GetPlotTemplateName());
  });

  ptree plotTree;
  for (vector<Plot>& plots : {std::ref(mPlotTemplates), std::ref(mPlots)}) {
    for (Plot& plot : plots) {

      if (plot.GetFigureGroup() == "TEMPLATES" && usedTemplates.find(plot.GetName()) == usedTemplates.end()) {
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
}
void PlotManager::DumpPlot(const string& plotFileName, const string& figureGroup,
                           const string& plotName)
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
    } catch (...) {
      ERROR(R"(Cannot load file "{}".)", plotFileName);
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
bool PlotManager::GeneratePlot(Plot& plot, const string& outputMode)
{
  // if plot already exists, delete the old one first
  if (mPlotLedger.find(plot.GetUniqueName()) != mPlotLedger.end()) {
    ERROR(R"(Plot "{}" was already created. Replacing it.)", plot.GetUniqueName());
    mPlotLedger.erase(plot.GetUniqueName());
  }
  if (plot.GetFigureGroup().empty()) {
    ERROR("No figure group was specified.");
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
      WARNING(R"(Could not find plot template named "{}".)", plotTemplateName);
    }
  }
  PlotPainter painter;
  shared_ptr<TCanvas> canvas = painter.GeneratePlot(fullPlot, mDataBuffer);
  if (!canvas) return false;
  LOG("Created \033[1;32m{}\033[0m from group \033[1;33m{}\033[0m", fullPlot.GetName(), fullPlot.GetFigureGroup() + ((fullPlot.GetFigureCategory()) ? ":" + *fullPlot.GetFigureCategory() : ""));

  // if interactive mode is specified, open window instead of saving the plot
  if (outputMode == "interactive") {

    mPlotLedger[plot.GetUniqueName()] = canvas;
    mPlotViewHistory.push_back(&plot.GetUniqueName());
    uint32_t curPlotIndex{static_cast<uint32_t>(mPlotViewHistory.size() - 1)};

    // move new canvas to position of previous window
    int32_t curXpos{};
    int32_t curYpos{};
    const int32_t windowOffsetY{28}; // might depend on os (was 22)
    if (curPlotIndex > 0) {
      curXpos = mPlotLedger[*mPlotViewHistory[curPlotIndex - 1]]->GetWindowTopX();
      curYpos = mPlotLedger[*mPlotViewHistory[curPlotIndex - 1]]->GetWindowTopY();
      canvas->SetWindowPosition(curXpos, curYpos - windowOffsetY);
    }
    bool boxClicked = false;
    while (!gSystem->ProcessEvents() && gROOT->GetSelectedPad()) {
      bool isClick = canvas->GetEvent() == kButton1Double;
      bool isValidKey = canvas->GetEvent() == kKeyPress && (canvas->GetEventX() == 'a' || canvas->GetEventX() == 's');
      auto selectedBox = dynamic_cast<TPave*>(canvas->GetSelected());
      if (isClick && selectedBox) {
        if (!boxClicked) INFO("Current position of {}: ({:.3g}, {:.3g})", selectedBox->GetName(), selectedBox->GetX1NDC(), selectedBox->GetY2NDC());
        boxClicked = true;
      } else if (isClick || isValidKey) {
        curXpos = canvas->GetWindowTopX();
        curYpos = canvas->GetWindowTopY();
        TRootCanvas* canvasWindow = ((TRootCanvas*)canvas->GetCanvasImp());
        canvasWindow->UnmapWindow();
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
          if (curPlotIndex == 0) std::exit(EXIT_FAILURE); // TODO: properly propagate this to caller
          --curPlotIndex;
        }
        canvas = mPlotLedger[*mPlotViewHistory[curPlotIndex]];
        canvas->SetWindowPosition(curXpos, curYpos - windowOffsetY);
        ((TRootCanvas*)canvas->GetCanvasImp())->MapRaised();
      } else {
        boxClicked = false;
      }
      gSystem->Sleep(20);
    }
    return true;
  }

  const optional<string>& subFolder = plot.GetFigureCategory();
  string fileEnding = ".pdf";
  if (outputMode == "macro") {
    fileEnding = ".C";
  } else if (outputMode == "png") {
    fileEnding = ".png";
  } else if (outputMode == "eps") {
    fileEnding = ".eps";
  }

  string fileName = plot.GetUniqueName();

  if (outputMode == "file") {
    mPlotLedger[plot.GetUniqueName()] = canvas;
    return true;
  }

  if (!mUseUniquePlotNames) fileName = plot.GetName();
  std::replace(fileName.begin(), fileName.end(), '/', '_');
  std::replace(fileName.begin(), fileName.end(), ':', '_');

  // create output folders and files
  string folderName = mOutputDirectory + "/" + plot.GetFigureGroup();
  if (subFolder) folderName += "/" + *subFolder;
  gSystem->Exec((string("mkdir -p ") + folderName).data());
  canvas->SaveAs((folderName + "/" + fileName + fileEnding).data());
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
  map<int32_t, set<int32_t>> requiredData;
  bool saveAll = (figureGroup.empty());
  bool saveSpecificPlots = !saveAll && !plotNames.empty();
  vector<Plot*> selectedPlots;

  // first determine which data needs to be loaded
  for (auto& plot : mPlots) {
    if (!saveAll && !(plot.GetFigureGroup() == figureGroup && plot.GetFigureCategory() && *plot.GetFigureCategory() == figureCategory))
      continue;
    if (saveSpecificPlots && std::find(plotNames.begin(), plotNames.end(), plot.GetName()) == plotNames.end())
      continue;
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
      WARNING(R"(Could not find plot "{}" in group "{}")", plotName,
              figureGroup + ((!figureCategory.empty()) ? ":" + figureCategory : ""));
    }
  }

  if (!FillBuffer()) PrintBufferStatus(true);
  // generate plots
  for (auto plot : selectedPlots) {
    if (!GeneratePlot(*plot, outputMode))
      ERROR(R"(Plot "{}" in figure group "{}" could not be created.)", plot->GetName(), plot->GetFigureGroup());
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

      TFile inputFile(fileName.data(), "READ");
      if (inputFile.IsZombie()) {
        ERROR(R"(Input file "{}" not found.)", fileName);
        break;
      }

      TObject* folder = &inputFile;

      // find top level entry point for this input file
      if (fileNamePath.size() > 1) {
        auto filePath = split_string(fileNamePath[1], '/');
        // append sub-specification from input name
        folder = FindSubDirectory(folder, filePath);
        if (!folder) {
          ERROR(R"(Subdirectory "{}" not found in file "{}".)", fileNamePath[1], fileName);
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
void PlotManager::PrintBufferStatus(bool missingOnly)
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
      string colorStart = (dataPtr) ? "\033[32m" : "\033[31m";
      string colorEnd = (dataPtr) ? "\033[0m" : "\033[0m";
      bool show = missingOnly ? (dataPtr == nullptr) : true;
      if (dataPtr) ++nAvailableData;
      if (show) {
        if (printInputID) INFO("{}", inputID);
        printInputID = false;
        INFO(" - {}{}{}", colorStart, dataName, colorEnd);
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
void PlotManager::PrintLoadedPlots()
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
    INFO("     ndata = {}", plot.InputDataCount());
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
  if (folder->InheritsFrom("TDirectory")) {
    itemList = ((TDirectoryFile*)folder)->GetListOfKeys();
  } else if (folder->InheritsFrom("TFolder")) {
    itemList = ((TFolder*)folder)->GetListOfFolders();
  } else if (folder->InheritsFrom("TCollection")) {
    itemList = (TCollection*)folder;
  } else {
    ERROR("Data-format not supported.");
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

      // read actual object to memory when traversing a directory
      if (obj->IsA() == TKey::Class()) {
        string className = ((TKey*)obj)->GetClassName();
        string keyName = ((TKey*)obj)->GetName();

        bool isTraversable = str_contains(className, "TDirectory") || str_contains(className, "TFolder") || str_contains(className, "TList") || str_contains(className, "TObjArray");
        if ((traverse && isTraversable) || std::find(dataNames.begin(), dataNames.end(), keyName) != dataNames.end()) {
          obj = ((TKey*)obj)->ReadObj();
          removeFromList = false;
        } else {
          ++iterator;
          continue;
        }
      }

      // in case this object is directory or list, repeat the same for this substructure
      if (obj->InheritsFrom("TDirectory") || obj->InheritsFrom("TFolder") || obj->InheritsFrom("TCollection")) {
        if (traverse) ReadData(obj, dataNames, prefix, suffix, inputID);
      } else {
        auto it = std::find(dataNames.begin(), dataNames.end(), ((TNamed*)obj)->GetName());
        if (it != dataNames.end()) {
          if (obj->InheritsFrom("TH1")) ((TH1*)obj)->SetDirectory(0); // demand ownership for histogram
          // re-name data
          string fullName = prefix + ((TNamed*)obj)->GetName();
          ((TNamed*)obj)->SetName((fullName + suffix).data());
          dataNames.erase(it); // TODO: why not erase-remove?
          mDataBuffer[inputID][fullName].reset(obj);
          deleteObject = false;
        }
      }

      // increase iterator before removing objects from collection
      ++iterator;
      if (removeFromList) {
        if (itemList->Remove(obj) == nullptr) {
          ERROR(R"(Could not remove item "{}" ({}) from collection "{}".)", ((TNamed*)obj)->GetName(),
                (void*)obj, itemList->GetName());
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
  ((TNamed*)graph)->SetName(uniqueName.data());
  mDataBuffer[inputIdentifier][graphName].reset(graph);
}

//**************************************************************************************************
/**
 * Recursively search for sub folder in file.
 */
//**************************************************************************************************
TObject* PlotManager::FindSubDirectory(TObject* folder, vector<string>& subDirs)
{
  if (!folder) return nullptr;
  bool deleteFolder = true;
  if (folder->InheritsFrom("TFile")) deleteFolder = false;

  if (subDirs.empty()) {
    if (folder->InheritsFrom("TDirectory") || folder->InheritsFrom("TFolder") || folder->InheritsFrom("TCollection")) {
      return folder;
    } else {
      return nullptr;
    }
  }
  TObject* subFolder{nullptr};
  if (folder->InheritsFrom("TDirectory")) {
    TKey* key = ((TDirectory*)folder)->FindKey(subDirs[0].data());
    if (key) {
      subFolder = key->ReadObj();
    } else {
      subFolder = ((TDirectory*)folder)->FindObject(subDirs[0].data());
    }
    deleteFolder = false;
  } else if (folder->InheritsFrom("TCollection") || folder->InheritsFrom("TFolder")) {
    subFolder = ((TCollection*)folder)->FindObject(subDirs[0].data());
    if (subFolder) {
      ((TCollection*)subFolder)->SetOwner();
      // if subfolder is part of list, remove it first to avoid double deletion
      ((TCollection*)folder)->Remove(subFolder);
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
                                       const vector<string>& figureGroupsWithCategoryUser,
                                       const vector<string>& plotNamesUser, const string& mode)
{
  uint32_t nFoundPlots{};
  bool isSearchRequest = (mode == "find") ? true : false;
  vector<std::pair<std::regex, std::regex>> groupCategoryRegex;
  groupCategoryRegex.reserve(figureGroupsWithCategoryUser.size());
  for (auto& figureGroupWithCategoryUser : figureGroupsWithCategoryUser) {
    // by default select all groups and all categories
    string group = ".*";
    string category = ".*";
    vector<string> groupCat = split_string(figureGroupWithCategoryUser, ':');
    if (groupCat.size() > 0 && !groupCat[0].empty()) group = groupCat[0];
    if (groupCat.size() > 1 && !groupCat[1].empty()) category = groupCat[1];
    if (groupCat.size() > 2) {
      ERROR(R"(Do not put ":" in your regular expressions! Colons should be used solely to separate figureGroup and figureCategory)");
      return;
    }
    groupCategoryRegex.push_back(std::make_pair(std::regex(group), std::regex(category)));
  }

  std::vector<std::regex> plotNamesRegex;
  plotNamesRegex.reserve(plotNamesUser.size());
  std::transform(plotNamesUser.begin(), plotNamesUser.end(), std::back_inserter(plotNamesRegex),
                 [](auto& plotNameUser) { return std::regex(plotNameUser); });

  ptree& inputTree = ReadPlotTemplatesFromFile(plotFileName);
  for (auto& plotGroupTree : inputTree) {
    // first filter by group
    string groupIdentifier = plotGroupTree.first.substr(string("GROUP::").size());
    bool isTemplate = (groupIdentifier == "TEMPLATES");
    if (std::find_if(groupCategoryRegex.begin(), groupCategoryRegex.end(),
                     [groupIdentifier](std::pair<std::regex, std::regex>& curGroupCatRegex) {
                       return std::regex_match(groupIdentifier, curGroupCatRegex.first);
                     }) == groupCategoryRegex.end()) {
      if (!isTemplate) continue;
    }

    for (auto& plotTree : plotGroupTree.second) {
      const string& plotName = plotTree.second.get<string>("name");
      const string& figureGroup = plotTree.second.get<string>("figure_group");
      optional<string> figureCategoryOpt;
      read_from_tree(plotTree.second, figureCategoryOpt, "figure_category");
      string figureCategory = (figureCategoryOpt) ? *figureCategoryOpt : "";

      if (std::find_if(
            groupCategoryRegex.begin(), groupCategoryRegex.end(),
            [figureGroup, figureCategory](std::pair<std::regex, std::regex>& curGroupCatRegex) {
              return std::regex_match(figureGroup, curGroupCatRegex.first) && std::regex_match(figureCategory, curGroupCatRegex.second);
            }) == groupCategoryRegex.end()) {
        if (!isTemplate) continue;
      }

      if (std::find_if(plotNamesRegex.begin(), plotNamesRegex.end(),
                       [plotName](std::regex& curPlotNameRegex) {
                         return std::regex_match(plotName, curPlotNameRegex);
                       }) == plotNamesRegex.end()) {
        if (!isTemplate) continue;
      }

      if (isTemplate) {
        Plot plot(plotTree.second);
        AddPlotTemplate(plot);
        continue;
      }

      ++nFoundPlots;
      if (isSearchRequest) {
        INFO(" - \033[1;32m{}\033[0m in group \033[1;33m{}\033[0m", plotName,
             figureGroup + ((!figureCategory.empty()) ? ":" + figureCategory : ""));
      } else {
        try {
          Plot plot(plotTree.second);
          AddPlot(plot);
        } catch (...) {
          ERROR(R"(Could not generate plot "{}" from XML file.)", plotTree.first);
        }
      }
    }
  }
  if (nFoundPlots == 0) {
    ERROR("Requested plots are not defined.");
  } else {
    INFO("Found {} plot{} matching the request.", nFoundPlots, (nFoundPlots == 1) ? "" : "s");
  }
  if (!isSearchRequest && mode != "load") {
    // now produce the loaded plots
    CreatePlots("", "", {}, mode);
  }
}

} // end namespace PlottingFramework
