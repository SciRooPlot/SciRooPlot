/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot -------------------------------------
 * Copyright (c) 2019-2026 Mario Krüger
 * Contact: mario.kruger@cern.ch
 * For a full list of contributors please see doc/CONTRIBUTORS.md.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation in version 3 (or later) of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

// framework dependencies
#include "PlotManager.h"
#include "PlotPainter.h"
#include "Logging.h"
#include "Helpers.h"

// std dependencies
#include <regex>
#include <filesystem>
#include <limits>
#include <unordered_map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <set>
#include <utility>
#include <iostream>

// boost dependencies
#include <boost/property_tree/info_parser.hpp>

// root dependencies
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RCsvDS.hxx"
#include "TApplication.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TError.h"
#include "TFile.h"
#include "TRootCanvas.h"
#include "TCanvas.h"
#include "TKey.h"
#include "TH1.h"
#include "TF1.h"
#include "TF2.h"
#include "TF3.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"
#include "TFolder.h"
#include "TPave.h"
#include "TTree.h"

using boost::property_tree::ptree;
using std::map;
using std::optional;
using std::set;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;

namespace SciRooPlot
{

//**************************************************************************************************
/**
 * Constructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::PlotManager() : mApp(new TApplication("MainApp", 0, nullptr))
{
  ROOT::EnableImplicitMT();
  gROOT->SetWebDisplay("off");
  gErrorIgnoreLevel = kWarning;

  // determine OS dependent offset between window and frame
  // (GetWindowTopY gives the current coordinates of the window, but SetWindowPosition moves the frame instead of the window)
  TCanvas dummyCanvas("dummyCanvas", "dummyCanvas", 1, 1);
  static_cast<TRootCanvas*>(dummyCanvas.GetCanvasImp())->UnmapWindow();
  dummyCanvas.SetCanvasSize(1, 1);
  dummyCanvas.SetWindowPosition(50, 50);
  mWindowOffsetY = dummyCanvas.GetWindowTopY() - static_cast<TRootCanvas*>(dummyCanvas.GetCanvasImp())->GetY();

  // first store all root data structures added to the manager in the global config directory
  string configPath = expand_path((gSystem->Getenv("SCIROOPLOT_CONFIG_PATH")) ? "${SCIROOPLOT_CONFIG_PATH}" : "~/.SciRooPlot");
  gSystem->Setenv("SCIROOPLOT_USER_DATA_DIR", configPath.data());
}

//**************************************************************************************************
/**
 * Save stored plots to .root file.
 */
//**************************************************************************************************
void PlotManager::SavePlotsToRootFile() const
{
  if (!mCanvasRegistry.empty()) {
    TFile outputFile((mOutputDirectory + "/" + mPlotsRootFile).data(), "RECREATE");
    if (outputFile.IsZombie()) {
      return;
    }
    outputFile.cd();
    if (auto nUserColors = TColor::GetFreeColorIndex() - mFirstFreeColorIndex) {
      vector<std::unique_ptr<TBox>> colorBoxes;
      colorBoxes.reserve(nUserColors);
      TCanvas colorCanvas("user_colors", "user defined colors", 800, 200);
      for (int32_t i = 0; i < nUserColors; i++) {
        if (gROOT->GetColor(mFirstFreeColorIndex + i)) {
          auto box = std::make_unique<TBox>(static_cast<double>(i) / nUserColors, 0, static_cast<double>(i + 1) / nUserColors, 1);
          box->SetFillColor(mFirstFreeColorIndex + i);
          box->Draw("SAME");
          colorBoxes.push_back(std::move(box));
        }
      }
      colorCanvas.Write();
    }
    uint32_t nPlots{0u};
    for (const auto& [uniqueName, canvas] : mCanvasRegistry) {
      size_t delimiterPos = uniqueName.find(":");
      string plotName = uniqueName.substr(0, delimiterPos);
      string subfolder = uniqueName.substr(delimiterPos + 1);
      if (!outputFile.GetDirectory(subfolder.data(), kFALSE, "cd")) {
        outputFile.mkdir(subfolder.data());
      }
      outputFile.cd(subfolder.data());
      canvas->Write(plotName.data());
      ++nPlots;
    }
    outputFile.Close();
    INFO("Saved {} plots to file {}/{}.", nPlots, mOutputDirectory, mPlotsRootFile);
  }
}

//**************************************************************************************************
/**
 * Save buffered data .root file.
 */
//**************************************************************************************************
void PlotManager::SaveDataToRootFile() const
{
  TFile outputFile((mOutputDirectory + "/" + mDataRootFile).data(), "RECREATE");
  if (outputFile.IsZombie()) {
    return;
  }
  for (const auto& [dataset, buffer] : mDataBuffer) {
    for (const auto& [dataName, dataPtr] : buffer) {
      if (!dataPtr) continue;
      auto dir = outputFile.mkdir(dataset.data(), "", true);
      string name = split_string(dataPtr->GetName(), ':')[0];
      auto tokens = split_string(name, '/');
      int iToken = 1;
      for (auto token : tokens) {
        if (iToken == tokens.size()) {
          dir->cd();
          dataPtr->Write(token.data());
        } else {
          dir = dir->mkdir(token.data(), "", true);
        }
        ++iToken;
      }
    }
  }
  outputFile.Close();
  INFO("Saved data to file {}/{}.", mOutputDirectory, mDataRootFile);
}

//**************************************************************************************************
/**
 * Sets path for output files. Plots will be stored in hierarchical structure according to defined groups.
 */
//**************************************************************************************************
void PlotManager::SetOutputDirectory(const string& path)
{
  mOutputDirectory = path;
}

//**************************************************************************************************
/**
 * Define input file paths for user defined dataset.
 */
//**************************************************************************************************
void PlotManager::AddDataset(const string& dataset, const vector<string>& inputFiles)
{
  for (auto inputFilePath : inputFiles) {
    if (std::filesystem::path(expand_path(inputFilePath)).is_relative()) {
      WARNING("The path to an input file must not be relative. Skipping {}.", inputFilePath);
      continue;
    }
    auto& files = mInputFiles[dataset];
    if (std::find(files.begin(), files.end(), inputFilePath) == files.end()) {
      files.push_back(inputFilePath);
    }
  }
}
void PlotManager::AddDataset(const std::string& dataset, std::initializer_list<string> inputFiles)
{
  AddDataset(dataset, std::vector<std::string>(inputFiles));
}
void PlotManager::AddDataset(const string& dataset, const string& inputFile)
{
  AddDataset(dataset, {inputFile});
}

//**************************************************************************************************
/**
 * Define input data for user defined unique dataset.
 */
//**************************************************************************************************
void PlotManager::AddDataset(const string& dataset, const vector<TObject*>& inputData)
{
  string fileName = "${SCIROOPLOT_USER_DATA_DIR}/UserData.root";
  string mode = "RECREATE";
  if (std::filesystem::exists(expand_path(fileName))) {
    mode = "UPDATE";
  }
  TFile file(fileName.data(), mode.data());
  TDirectory* dir = file.GetDirectory(dataset.data());
  if (!dir) {
    dir = file.mkdir(dataset.data());
  }
  dir->cd();
  for (auto object : inputData) {
    string name = object->GetName();
    if (name.empty()) {
      WARNING("Cannot add nameless object of type {} to dataset {}", object->ClassName(), dataset);
      continue;
    }
    object->Write();
  }
  file.Close();
  AddDataset(dataset, {fileName + ":" + dataset});
}
void PlotManager::AddDataset(const string& dataset, TObject* inputData)
{
  AddDataset(dataset, vector<TObject*>{inputData});
}

//**************************************************************************************************
/**
 * Save dataset properties currently defined in the manager to a config file.
 */
//**************************************************************************************************
void PlotManager::SaveDatasets(const string& datasetFile) const
{
  ptree inputFileTree;
  for (const auto& inFileTuple : mInputFiles) {
    ptree filesOfDataset;
    for (const auto& fileName : inFileTuple.second) {
      filesOfDataset.add("FILE", fileName);
    }
    inputFileTree.put_child(inFileTuple.first, filesOfDataset);
  }
  std::filesystem::path file = expand_path(datasetFile);
  if (std::filesystem::create_directories(file.parent_path())) {
    INFO("Created config folder: {}", file.parent_path().string());
  }
  using boost::property_tree::write_info;
  write_info(file.string(), inputFileTree);
}

//**************************************************************************************************
/**
 * Load dataset properties from config file into manager.
 */
//**************************************************************************************************
void PlotManager::LoadDatasets(const string& datasetFile)
{
  ptree inputFileTree;
  try {
    using boost::property_tree::read_info;
    read_info(expand_path(datasetFile), inputFileTree);
  } catch (...) {
    ERROR("Cannot load file {}.", datasetFile);
    return;
  }
  for (const auto& inputPair : inputFileTree) {
    const string& dataset = inputPair.first;
    set<string> allFileNames;
    for (const auto& fileEntry : inputPair.second) {
      string fileOrDirName = expand_path(fileEntry.second.get_value<string>());
      if (str_contains(fileOrDirName, ".root", true) || str_contains(fileOrDirName, mTableFileEndings, true)) {
        allFileNames.insert(fileOrDirName);
      } else if (std::filesystem::is_directory(fileOrDirName)) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(fileOrDirName)) {
          if (file.path().extension() == ".root" || str_contains(file.path().extension(), mTableFileEndings)) {
            allFileNames.insert(file.path().string());
          }
        }
      }
    }
    AddDataset(dataset, vector<string>{allFileNames.begin(), allFileNames.end()});
  }
}

//**************************************************************************************************
/**
 * Add plot to the manager.
 */
//**************************************************************************************************
void PlotManager::AddPlot(Plot plot)
{
  if (plot.GetGroup().empty()) {
    ERROR("Cannot add plot ({}) that does not belong to a group.", plot.GetName());
    return;
  } else if (plot.GetGroup() == "BASE_PLOTS") {
    ERROR("You cannot use reserved group name 'BASE_PLOTS'!");
    return;
  }
  mPlots.erase(std::remove_if(mPlots.begin(), mPlots.end(),
                              [&plot](const Plot& curPlot) {
                                bool removePlot = curPlot.GetUniqueName() == plot.GetUniqueName();
                                if (removePlot) WARNING("Plot {} in {} already exists and will be replaced.", curPlot.GetName(), curPlot.GetGroup());
                                return removePlot;
                              }),
               mPlots.end());
  mPlots.push_back(std::move(plot));
}

//**************************************************************************************************
/**
 * Add base plot to the manager.
 */
//**************************************************************************************************
void PlotManager::AddBasePlot(Plot basePlot)
{
  basePlot.SetGroup("BASE_PLOTS");
  mBasePlots.erase(std::remove_if(mBasePlots.begin(), mBasePlots.end(),
                                  [&basePlot](const Plot& curBasePlot) {
                                    bool removePlot = curBasePlot.GetUniqueName() == basePlot.GetUniqueName();
                                    if (removePlot) WARNING("Base plot {} already exists and will be replaced.", curBasePlot.GetName());
                                    return removePlot;
                                  }),
                   mBasePlots.end());
  mBasePlots.push_back(std::move(basePlot));
}

//**************************************************************************************************
/**
 * Add plot showing the specified colors or root color panel as fallback.
 */
//**************************************************************************************************
void PlotManager::AddColorOverview(const string& name, const string& group, const vector<int32_t>& colors)
{
  Plot plot(name, group);
  plot.SetDimensions(800, 800, true);
  plot[1].SetPosition(0., 0., 1., 1.);
  if (colors.empty()) {
    plot.SetPaintColorWheel();
  } else {
    int32_t bestRows = 1;
    size_t bestCols = colors.size();
    double_t bestScore = -1;
    for (int32_t rows = 1; rows <= static_cast<int32_t>(colors.size()); ++rows) {
      size_t cols = (colors.size() + rows - 1) / rows;
      double_t cellW = 1.0 / cols;
      double_t cellH = 1.0 / rows;
      double_t area = cellW * cellH;
      double_t aspectPenalty = fabs(cellW - cellH);
      double_t score = area - 0.3 * aspectPenalty;
      if (score > bestScore) {
        bestScore = score;
        bestRows = rows;
        bestCols = cols;
      }
    }
    for (size_t i = 0; i < colors.size(); ++i) {
      size_t r = i / bestCols;
      size_t c = i % bestCols;
      double_t x = static_cast<double_t>(c) / bestCols;
      double_t y = 1.0 - static_cast<double_t>(r) / bestRows;
      double_t w = 1.0 / bestCols;
      double_t h = 1.0 / bestRows;
      plot[1]
        .AddText(x, y, std::to_string(colors[i]))
        .SetSize(w, h)
        .SetFillStyle(1001)
        .SetFillColor(colors[i]);
    }
  }
  AddPlot(plot);
}

//**************************************************************************************************
/**
 * Save plots matching name and group regex to file.
 */
//**************************************************************************************************
void PlotManager::SavePlots(const string& plotFile, const string& name, const string& group) const
{
  ptree plotTree;
  std::regex groupRegex{group};
  std::regex nameRegex{name};

  for (const vector<Plot>& plots : {std::ref(mBasePlots), std::ref(mPlots)}) {
    for (const Plot& plot : plots) {
      if (plot.GetGroup() != "BASE_PLOTS") {
        if (!std::regex_match(plot.GetGroup(), groupRegex)) continue;
        if (!std::regex_match(plot.GetName(), nameRegex)) continue;
      }
      string displayedName = plot.GetUniqueName();
      std::replace(displayedName.begin(), displayedName.end(), '.', '_');
      plotTree.put_child(displayedName, plot.GetPropertyTree());
    }
  }
  std::filesystem::path filePath = expand_path(plotFile);
  if (std::filesystem::create_directories(filePath.parent_path())) {
    INFO("Created config folder: {}", filePath.parent_path().string());
  }
  using boost::property_tree::write_info;
  write_info(filePath.string(), plotTree);
}

//**************************************************************************************************
/**
 * Function to load plots matching name and group regex from file.
 */
//**************************************************************************************************
void PlotManager::LoadPlots(const string& plotFile, const string& name, const string& group)
{
  uint32_t nFoundPlots{};
  std::regex groupRegex{group};
  std::regex nameRegex{name};
  ptree fileTree;
  try {
    using boost::property_tree::read_info;
    read_info(expand_path(plotFile), fileTree);
  } catch (...) {
    ERROR("Cannot open file {}.", plotFile);
    std::exit(EXIT_FAILURE);
  }

  for (const auto& plotTree : fileTree) {
    const string& curGroup = plotTree.second.get<string>("group");
    if (curGroup == "BASE_PLOTS") {
      Plot plot(plotTree.second);
      AddBasePlot(plot);
      continue;
    }
    if (!std::regex_match(curGroup, groupRegex)) continue;

    const string& curName = plotTree.second.get<string>("name");
    if (!std::regex_match(curName, nameRegex)) continue;

    ++nFoundPlots;
    try {
      Plot plot(plotTree.second);
      AddPlot(plot);
    } catch (...) {
      ERROR("Could not load plot {} from file.", plotTree.first);
    }
  }
  if (nFoundPlots == 0) {
    ERROR("Found no plots matching the request {}{}{} in {}{}{}.", logger::begin_color(logger::Color::Green), name, logger::end_color(), logger::begin_color(logger::Color::Yellow), group, logger::end_color());
  } else if (nFoundPlots > 1) {
    INFO("Found {} plots matching the request.", nFoundPlots);
  }
}

//**************************************************************************************************
/**
 * Generates plots matching name and group regex.
 */
//**************************************************************************************************
void PlotManager::GeneratePlots(const string& mode, const string& name, const string& group)
{
  // first determine which data needs to be loaded
  vector<Plot*> selectedPlots;
  map<int32_t, set<int32_t>> requiredData;

  std::regex groupRegex{group};
  std::regex nameRegex{name};

  for (auto& plot : mPlots) {
    if (!std::regex_match(plot.GetGroup(), groupRegex)) continue;
    if (!std::regex_match(plot.GetName(), nameRegex)) continue;
    selectedPlots.push_back(&plot);

    // determine which input data are needed for plots
    for (auto& [padID, pad] : plot.GetPads()) {
      if (auto& refFunc = pad.GetRefFunc()) {
        mDataBuffer[refFunc->GetDataset()][refFunc->GetName()];
      } else {
        if (plot.GetBasePlotName()) {
          auto it = std::find_if(mBasePlots.begin(), mBasePlots.end(), [&](const auto& basePlot) { return *plot.GetBasePlotName() == basePlot.GetName(); });
          if (it != mBasePlots.end()) {
            if (auto& refFunc = (*it).GetPad(padID).GetRefFunc()) {
              mDataBuffer[refFunc->GetDataset()][refFunc->GetName()];
            }
          }
        }
      }
      for (const auto& data : pad.GetData()) {
        mDataBuffer[data->GetDataset()][data->GetName()];
        if (data->GetDataInfo().dataDims.size()) {
          auto& dataInfos = mDataInfoBuffer[data->GetDataset()][data->GetName()];
          auto iter = std::find_if(dataInfos.begin(), dataInfos.end(), [&](const auto& dataInfo) { return dataInfo.GetNameSuffix() == data->GetDataInfo().GetNameSuffix(); });
          if (iter == dataInfos.end()) {
            mDataInfoBuffer[data->GetDataset()][data->GetName()].push_back(data->GetDataInfo());
          }
        }
        if (data->GetType() == "ratio") {
          const auto& ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
          mDataBuffer[ratio->GetDenomDataset()][ratio->GetDenomName()];
          if (ratio->GetDenomDataInfo().dataDims.size()) {
            auto& dataInfos = mDataInfoBuffer[ratio->GetDenomDataset()][ratio->GetDenomName()];
            auto iter = std::find_if(dataInfos.begin(), dataInfos.end(), [&](const auto& dataInfo) { return dataInfo.GetNameSuffix() == data->GetDataInfo().GetNameSuffix(); });
            if (iter == dataInfos.end()) {
              mDataInfoBuffer[ratio->GetDenomDataset()][ratio->GetDenomName()].push_back(ratio->GetDenomDataInfo());
            }
          }
        }
      }
    }
  }

  if (selectedPlots.empty()) {
    ERROR("No plots were created.");
  }

  try {
    if (!FillBuffer()) PrintBufferStatus(true);
    // generate plots
    for (auto plot : selectedPlots) {
      if (!GeneratePlot(*plot, mode))
        ERROR("Plot {}{}{} from group {}{}{} could not be created.", logger::begin_color(logger::Color::Green), plot->GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), plot->GetGroup(), logger::end_color());
    }
    if (mode == "file") {
      SavePlotsToRootFile();
    } else if (mode == "data") {
      SaveDataToRootFile();
    }
  } catch (...) {
    ERROR("An unexpected error occurred. The application will now exit.");
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
  for (auto& [dataset, buffer] : mDataBuffer) {
    unordered_map<string, vector<string>> requiredData;  // subdir, names
    for (auto& [dataName, dataPtr] : buffer) {
      if (dataPtr) continue;

      // generate user-defined functions on-the-fly
      if (dataset == "USER_FUNCTIONS") {
        TFormula formula("tmp", dataName.data());
        int dim = formula.GetNdim();
        if (dim <= 1) {
          dataPtr.reset(new TF1(dataName.data(), dataName.data()));
        } else if (dim == 2) {
          dataPtr.reset(new TF2(dataName.data(), dataName.data()));
        } else if (dim == 3) {
          dataPtr.reset(new TF3(dataName.data(), dataName.data()));
        } else {
          ERROR("Cannot create function {}.", dataName);
        }
        continue;
      } else if (dataset == "USER_GRAPHS") {
        auto strs = split_string(dataName, ';');
        auto xStrs = split_string(strs[0], ',');
        auto yStrs = split_string(strs[1], ',');
        vector<double_t> x;
        vector<double_t> y;
        for (size_t i = 0; i < xStrs.size(); ++i) {
          x.push_back(std::stod(xStrs[i]));
          y.push_back(std::stod(yStrs[i]));
        }
        if (!x.size() || x.size() != y.size()) {
          ERROR("Incompatible number of points.");
        } else {
          dataPtr.reset(new TGraph(static_cast<int32_t>(x.size()), x.data(), y.data()));
          static_cast<TGraph*>(dataPtr.get())->SetName(dataName.data());
        }
        continue;
      }

      auto pathPos = dataName.find_last_of("/");
      string path;
      string name = dataName;
      if (pathPos != string::npos) {
        path = name.substr(0, pathPos);
        name.erase(0, pathPos + 1);
      }
      requiredData[std::move(path)].push_back(std::move(name));
    }

    // open all input files belonging to the current dataset and extract the data
    for (const auto& inputFileName : mInputFiles[dataset]) {
      if (requiredData.empty()) break;
      if (str_contains(inputFileName, mTableFileEndings, true)) {
        string name = inputFileName.substr(inputFileName.rfind('/') + 1, inputFileName.rfind(".") - inputFileName.rfind('/') - 1);
        ReadDataCSV(inputFileName, name, dataset);
        vector<string>& wantedNames = requiredData[""];
        wantedNames.erase(std::remove_if(wantedNames.begin(), wantedNames.end(), [&](const auto& wantedName) { return wantedName == name; }), wantedNames.end());
        if (wantedNames.empty()) requiredData.erase("");
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
          string suffix = ":" + dataset;
          ReadData(subfolder, names, prefix, suffix, dataset);
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

      for (const auto& pathStr : emptySubDirs) {
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
void PlotManager::PrintBufferStatus(bool onlyMissing) const
{
  INFO("===============================================");
  if (onlyMissing) {
    INFO("================= Missing Data ================");
  } else {
    INFO("================= Data Buffer =================");
  }
  uint32_t nNeededData{};
  uint32_t nAvailableData{};
  for (const auto& [dataset, buffer] : mDataBuffer) {
    bool printDataset = true;
    for (const auto& [dataName, dataPtr] : buffer) {
      ++nNeededData;
      bool show = onlyMissing ? (dataPtr == nullptr) : true;
      if (dataPtr) ++nAvailableData;
      if (show) {
        if (printDataset) INFO("{}", dataset);
        printDataset = false;
        INFO(" - {}{}{}", (dataPtr) ? logger::begin_color(logger::Color::Green) : logger::begin_color(logger::Color::Red), dataName, logger::end_color());
      }
    }
  }
  INFO("Found {}/{} required input data.", nAvailableData, nNeededData);
  INFO("===============================================");
}

//**************************************************************************************************
/**
 * Generates plot.
 */
//**************************************************************************************************
bool PlotManager::GeneratePlot(const Plot& plot, const string& mode)
{
  bool isInteractiveMode = (mode == "show");
  bool isMacroMode = (mode == "macro");

  // if plot already exists, delete the old one first
  if (mCanvasRegistry.find(plot.GetUniqueName()) != mCanvasRegistry.end()) {
    ERROR("Plot {} was already created. Replacing it.", plot.GetUniqueName());
    mCanvasRegistry.erase(plot.GetUniqueName());
  }
  if (plot.GetGroup().empty()) {
    ERROR("No group was specified for plot {}.", plot.GetName());
    return false;
  }
  Plot fullPlot = plot;
  if (plot.GetBasePlotName()) {
    const string& basePlotName = *plot.GetBasePlotName();
    auto iterator = std::find_if(
      mBasePlots.begin(), mBasePlots.end(),
      [&](Plot& basePlot) { return basePlot.GetName() == basePlotName; });
    if (iterator != mBasePlots.end()) {
      fullPlot = *iterator + plot;
    } else {
      WARNING("Could not find base plot named {}.", basePlotName);
    }
  }
  if (mode == "print") {
    INFO("Settings of plot {}{}{} from group {}{}{}:", logger::begin_color(logger::Color::Green), fullPlot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), fullPlot.GetGroup(), logger::end_color());
    Plot::Print(fullPlot.GetPropertyTree(), "");
    return true;
  }

  PlotPainter painter;
  gROOT->SetBatch(!isInteractiveMode && !isMacroMode);
  shared_ptr<TCanvas> canvas{painter.GeneratePlot(fullPlot, mDataBuffer)};
  if (!canvas) return false;

  if (TColor::GetFreeColorIndex() > std::numeric_limits<int16_t>::max()) {
    // there is a natural limit to the number of custom colors since ROOT color indices are of type short
    ERROR("Too many custom colors in one session. Aborting...");
    std::exit(EXIT_FAILURE);
  }
  LOG("Created plot {}{}{} from group {}{}{}.", logger::begin_color(logger::Color::Green), fullPlot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), fullPlot.GetGroup(), logger::end_color());

  // if interactive mode is specified, open window instead of saving the plot
  if (isInteractiveMode) {
    if (auto rc = dynamic_cast<TRootCanvas*>(canvas->GetCanvasImp())) {
      rc->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
    }
    mCanvasRegistry[plot.GetUniqueName()] = canvas;
    mPlotViewHistory.push_back(&plot.GetUniqueName());
    uint32_t curPlotIndex{static_cast<uint32_t>(mPlotViewHistory.size() - 1)};

    // move new canvas to position of previous window
    int32_t curXpos{};
    int32_t curYpos{};
    if (curPlotIndex > 0) {
      curXpos = mCanvasRegistry[*mPlotViewHistory[curPlotIndex - 1]]->GetWindowTopX();
      curYpos = mCanvasRegistry[*mPlotViewHistory[curPlotIndex - 1]]->GetWindowTopY();
      canvas->SetWindowPosition(curXpos, curYpos - mWindowOffsetY);
      static_cast<TRootCanvas*>(mCanvasRegistry[*mPlotViewHistory[curPlotIndex - 1]]->GetCanvasImp())->UnmapWindow();
    }
    canvas->Show();
    bool boxClicked = false;
    while (!gSystem->ProcessEvents() && gROOT->GetSelectedPad()) {
      bool isClick = canvas->GetEvent() == kButton1Double;
      bool isValidKey = canvas->GetEvent() == kKeyPress && (canvas->GetEventX() == 'a' || canvas->GetEventX() == 's');
      if (canvas->GetEvent() == kKeyPress && (canvas->GetEventX() == 'q')) {
        gApplication->Terminate();
      }
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
          if (curPlotIndex == 0) std::exit(EXIT_SUCCESS);
          --curPlotIndex;
        }
        static_cast<TRootCanvas*>(canvas->GetCanvasImp())->UnmapWindow();
        canvas = mCanvasRegistry[*mPlotViewHistory[curPlotIndex]];
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

  if (mode == "file") {
    mCanvasRegistry[plot.GetUniqueName()] = canvas;
    return true;
  }
  if (mode == "data") {
    return true;
  }

  bool isGif = false;
  static string gifName;
  static string gifFolderName;
  string gifRepRate = "+50";  // number of centiseconds between frames

  string fileEnding;
  if (isMacroMode) {
    fileEnding = ".C";
    // object names are converted to variable names and therefore must not contain '/'
    // this should be fixed in ROOT itself as it does not create valid cpp code otherwise
    std::function<void(TPad*)> cleanNames;
    cleanNames = [&](TPad* pad) {
      string saveName = pad->GetName();
      std::replace(saveName.begin(), saveName.end(), '/', '_');
      std::replace(saveName.begin(), saveName.end(), ':', '_');
      pad->SetName(saveName.data());
      TIter next(pad->GetListOfPrimitives());
      TObject* object = nullptr;
      while ((object = next())) {
        string saveName = object->GetName();
        std::replace(saveName.begin(), saveName.end(), '/', '_');
        std::replace(saveName.begin(), saveName.end(), ':', '_');
        static_cast<TNamed*>(object)->SetName(saveName.data());
        if (object->InheritsFrom(TPad::Class())) {
          cleanNames(static_cast<TPad*>(object));
        }
      }
    };
    cleanNames(canvas.get());
  } else if ((mode == "pdf") || (mode == "png") || (mode == "eps") || (mode == "svg") || (mode == "ps") || (mode == "html") || (mode == "json") || (mode == "xml") || (mode == "jpg") || (mode == "root")) {
    fileEnding = "." + mode;
  } else if (str_contains(mode, "gif")) {
    fileEnding = ".gif";
    isGif = true;
    if (auto delimPos = mode.find("+"); delimPos != string::npos) {
      gifRepRate = mode.substr(delimPos);
    }
  }

  if (fileEnding.empty()) {
    ERROR("No valid output format was specified. Cannot save plot.");
    return true;
  }

  string fileName = plot.GetName();
  std::replace(fileName.begin(), fileName.end(), ':', '_');
  std::replace(fileName.begin(), fileName.end(), '/', '_');

  // create output folders and files
  string folderName = mOutputDirectory + "/" + plot.GetGroup();
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
 * Show which plots are currently loaded in the framework.
 */
//**************************************************************************************************
void PlotManager::ListPlots() const
{
  for (const auto& plot : mPlots) {
    INFO(" - {}{}{} in group {}{}{}", logger::begin_color(logger::Color::Green), plot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), plot.GetGroup(), logger::end_color());
  }
}

//**************************************************************************************************
/**
 * Recursively reads data from folder / list and adds it to output data array. Found dataNames are removed from the vectors.
 */
//**************************************************************************************************
void PlotManager::ReadData(TObject* folder, vector<string>& dataNames, const string& prefix, const string& suffix, const string& dataset)
{
  TCollection* itemList = nullptr;
  if (folder->InheritsFrom(TDirectory::Class())) {
    itemList = static_cast<TDirectoryFile*>(folder)->GetListOfKeys();
  } else if (folder->InheritsFrom(TFolder::Class())) {
    itemList = static_cast<TFolder*>(folder)->GetListOfFolders();
  } else if (folder->InheritsFrom(TCollection::Class())) {
    itemList = static_cast<TCollection*>(folder);
  } else {
    ERROR("Data format {} not supported.", folder->ClassName());
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

      string curDataName;  // name of current key or data
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
          ReadData(obj, dataNames, prefix, suffix, dataset);
        } else if (removeFromList) {
          removeFromList = false;
          deleteObject = false;
        }
      } else {
        // the key name supersedes the actual data name (in case they are different when written to file via h->Write("myKeyName"))
        if (curDataName.empty()) curDataName = obj->GetName();
        if (auto it = std::find(dataNames.begin(), dataNames.end(), curDataName); it != dataNames.end()) {
          // demand ownership for object if required for given type
          if (obj->InheritsFrom(TH1::Class())) static_cast<TH1*>(obj)->SetDirectory(0);
          if (obj->InheritsFrom(TGraph2D::Class())) static_cast<TGraph2D*>(obj)->SetDirectory(0);
          itemList->Remove(obj);
          dataNames.erase(it);
          string fullName = prefix + curDataName;
          if (obj->InheritsFrom(TTree::Class())) {
            TTree* tree = static_cast<TTree*>(obj);
            mDataBuffer[dataset][fullName].reset(nullptr);
            // do all requested projections of this tree
            for (auto& dataInfo : mDataInfoBuffer[dataset][fullName]) {
              string dataFullName = fullName + dataInfo.GetNameSuffix();
              try {
                SUPPRESS_STDERR(true);
                if (dataInfo.singleProc()) ROOT::DisableImplicitMT();
                ROOT::RDataFrame df(*tree);
                obj = ProcessData(df, fullName, dataInfo, dataFullName + suffix);
                if (!ROOT::IsImplicitMTEnabled()) ROOT::EnableImplicitMT();
                SUPPRESS_STDERR(false);
              } catch (const std::runtime_error&) {
                ERROR("Invalid query for tree.");
                obj = nullptr;
              }
              mDataBuffer[dataset][dataFullName].reset(obj);
            }
            tree->SetDirectory(0);
            delete tree;
          } else {
            static_cast<TNamed*>(obj)->SetName((fullName + suffix).data());
            mDataBuffer[dataset][fullName].reset(obj);
          }
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
void PlotManager::ReadDataCSV(const string& inputFileName, const string& name, const string& dataset)
{
  if (!file_exists(inputFileName)) {
    ERROR("File {} does not exist.", inputFileName);
    return;
  }
  char delimiter = ',';
  std::vector<char> candidates = {',', ';', '\t', '|', ' '};
  size_t maxCount = 0;
  std::ifstream file(inputFileName);
  int32_t lineCount = 0;
  string line;
  while (lineCount < 100 && std::getline(file, line)) {
    for (char c : candidates) {
      size_t count = std::count(line.begin(), line.end(), c);
      if (count > maxCount) {
        maxCount = count;
        delimiter = c;
      }
    }
    ++lineCount;
  }
  for (auto& dataInfo : mDataInfoBuffer[dataset][name]) {
    string dataName = name + dataInfo.GetNameSuffix();
    TObject* obj = nullptr;
    try {
      SUPPRESS_STDERR(true);
      if (dataInfo.singleProc()) ROOT::DisableImplicitMT();
      ROOT::RDataFrame df = ROOT::RDF::FromCSV(inputFileName, true, delimiter, 50000);
      obj = ProcessData(df, name, dataInfo, dataName + ":" + dataset);
      if (!ROOT::IsImplicitMTEnabled()) ROOT::EnableImplicitMT();
      SUPPRESS_STDERR(false);
    } catch (const std::runtime_error& e) {
      ERROR("Invalid query for table {}.", name);
      std::cout << e.what() << std::endl;
      obj = nullptr;
    }
    mDataBuffer[dataset][dataName].reset(obj);
  }
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
 * Process RDataFrame according to the settings stored in dataInfo.
 */
//**************************************************************************************************
TObject* PlotManager::ProcessData(ROOT::RDataFrame& df, const string& dfName, const Plot::Pad::Data::data_info_t& dataInfo, const string& name) const
{
  bool isProfile = false;
  bool isScatter = false;
  if (dataInfo.isProfileNoScatter) {
    isProfile = *dataInfo.isProfileNoScatter;
    isScatter = !isProfile;
  }
  bool isProjection = (!isProfile && !isScatter);
  bool hasWeights = false;

  auto dataDims = dataInfo.dataDims;  // make copy here so it can be modified
  if (isProfile && dataDims.size() > 3) {
    ERROR("Too many dimensions specified for profile of {}.", dfName);
    return nullptr;
  }
  int32_t axisID = 1;
  for (auto& dataDim : dataDims) {
    if (isProjection || (isProfile && axisID < dataDims.size())) {
      // sanity check for binned axes
      if ((dataDim.nBins && dataDim.edges.size() != 2) || (!dataDim.nBins && dataDim.edges.size() <= 1)) {
        ERROR("Can't project tree {} due to ill defined binning for {}.", dfName, dataDim.var);
        return nullptr;
      }
      if (!std::is_sorted(dataDim.edges.begin(), dataDim.edges.end())) {
        if (!(dataDim.edges.size() == 2 && !dataDim.edges[0] && !dataDim.edges[1])) {
          ERROR("Can't project tree {} due to ill defined binning for {}.", dfName, dataDim.var);
          return nullptr;
        }
      }
    }
    ++axisID;
  }

  TObject* obj = nullptr;
  ROOT::RDF::RNode node = df;  // working node

  if (dataInfo.definitions.keys && dataInfo.definitions.values) {
    for (int i = 0; i < dataInfo.definitions.keys->size(); ++i) {
      node = node.Define(dataInfo.definitions.keys->at(i), dataInfo.definitions.values->at(i));
    }
  }
  if (dataInfo.entries.max) {
    if (dataInfo.entries.min) {
      node = node.Range(*dataInfo.entries.min, *dataInfo.entries.max);
    } else {
      node = node.Range(*dataInfo.entries.max);
    }
  }
  auto nEntriesPreFilter = node.Count();
  if (dataInfo.filters) {
    for (int32_t i = 0; i < dataInfo.filters->size(); ++i) {
      try {
        node = node.Filter(dataInfo.filters->at(i));
      } catch (std::runtime_error) {
        ERROR("Illegal expression in filter: {}.", dataInfo.filters->at(i));
        return nullptr;
      }
    }
    auto nEntriesPostFilter = node.Count();
    if (nEntriesPostFilter && nEntriesPreFilter) {
      INFO("Processing {} entries ({:.2f}%) of {}.", (*nEntriesPostFilter), 100. * (*nEntriesPostFilter) / (*nEntriesPreFilter), dfName);
    }
  } else {
    if (nEntriesPreFilter) {
      INFO("Processing {} entries of {}.", *nEntriesPreFilter, dfName);
    }
  }

  string histTitle;
  axisID = 1;
  for (auto& dataDim : dataDims) {
    string colName = "SRP_AXIS_" + std::to_string(axisID);
    try {
      node = node.Define(colName, dataDim.var);
    } catch (std::runtime_error) {
      ERROR("Illegal expression: {}.", dataDim.var);
      return nullptr;
    }
    if (node.GetColumnType(colName).find("string") != string::npos) {
      ERROR("Variable expression {} is not numeric.", dataDim.var);
      return nullptr;
    }
    if (dataDim.nBins && dataDim.edges.size() == 2 && !dataDim.edges[0] && !dataDim.edges[1]) {
      // auto-detect bin edges given the data
      auto max = node.Max("SRP_AXIS_" + std::to_string(axisID));
      auto min = node.Min("SRP_AXIS_" + std::to_string(axisID));
      double_t margin = 0.01;
      dataDim.edges[0] = (*min) * ((*min) > 0 ? (1. - margin) : (1. + margin));
      dataDim.edges[1] = (*max) * ((*max) > 0 ? (1. + margin) : (1. - margin));
    }
    if (isProfile && axisID == dataDims.size()) {
      histTitle += ";#LT " + dataDim.var + " #GT";
    } else {
      histTitle += ";" + dataDim.var;
    }
    ++axisID;
  }
  if (isScatter) {
    if (dataDims.size() == 2) {
      obj = node.Graph("SRP_AXIS_1", "SRP_AXIS_2")->Clone(name.data());
    } else if (dataDims.size() == 4) {
      obj = node.GraphAsymmErrors("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_4")->Clone(name.data());
    } else if (dataDims.size() == 6) {
      obj = node.GraphAsymmErrors("SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_4", "SRP_AXIS_5", "SRP_AXIS_6")->Clone(name.data());
    } else {
      ERROR("Invalid number of columns for scatter data.");
      return nullptr;
    }
    if (obj && obj->InheritsFrom(TGraph::Class())) {
      TGraph* graph = static_cast<TGraph*>(obj);
      graph->SetTitle(histTitle.data());
      return graph;
    }
  }
  if (!isProfile) {
    histTitle += (dataInfo.weight) ? ";weighted counts" : ";counts";
  }

  if (dataInfo.weight) {
    try {
      node = node.Define("SRP_AXIS_W", *dataInfo.weight);
    } catch (std::runtime_error) {
      ERROR("Illegal expression for weights: {}.", *dataInfo.weight);
      return nullptr;
    }
    hasWeights = true;
  }

  if (dataDims.size() == 1) {
    auto histModel = ROOT::RDF::TH1DModel();
    auto& dataDim1 = dataDims.at(0);

    if (!dataDim1.nBins) {
      histModel = ROOT::RDF::TH1DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data());
    } else {
      histModel = ROOT::RDF::TH1DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1]);
    }
    if (hasWeights) {
      obj = node.Histo1D(histModel, "SRP_AXIS_1", "SRP_AXIS_W")->Clone(name.data());
    } else {
      obj = node.Histo1D(histModel, "SRP_AXIS_1")->Clone(name.data());
    }
  } else if (dataDims.size() == 2) {
    auto& dataDim1 = dataDims.at(0);
    auto& dataDim2 = dataDims.at(1);
    if (isProfile) {
      auto profileModel = ROOT::RDF::TProfile1DModel();
      if (!dataDim1.nBins) {
        profileModel = ROOT::RDF::TProfile1DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data());
      } else {
        profileModel = ROOT::RDF::TProfile1DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1]);
      }
      if (hasWeights) {
        obj = node.Profile1D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W")->Clone(name.data());
      } else {
        obj = node.Profile1D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2")->Clone(name.data());
      }
    } else {
      auto histModel = ROOT::RDF::TH2DModel();
      if (!dataDim1.nBins && !dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (dataDim1.nBins && dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      } else if (dataDim1.nBins && !dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (!dataDim1.nBins && dataDim2.nBins) {
        histModel = ROOT::RDF::TH2DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      }
      if (hasWeights) {
        obj = node.Histo2D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_W")->Clone(name.data());
      } else {
        obj = node.Histo2D(histModel, "SRP_AXIS_1", "SRP_AXIS_2")->Clone(name.data());
      }
    }
  } else if (dataDims.size() == 3) {
    auto& dataDim1 = dataDims.at(0);
    auto& dataDim2 = dataDims.at(1);
    auto& dataDim3 = dataDims.at(2);

    if (isProfile) {
      auto profileModel = ROOT::RDF::TProfile2DModel();
      if (!dataDim1.nBins && !dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (dataDim1.nBins && dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      } else if (dataDim1.nBins && !dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data());
      } else if (!dataDim1.nBins && dataDim2.nBins) {
        profileModel = ROOT::RDF::TProfile2DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1]);
      }
      if (hasWeights) {
        obj = node.Profile2D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W")->Clone(name.data());
      } else {
        obj = node.Profile2D(profileModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3")->Clone(name.data());
      }
    } else {
      auto histModel = ROOT::RDF::TH3DModel();
      if (dataDim1.nBins && dataDim2.nBins && dataDim3.nBins) {
        histModel = ROOT::RDF::TH3DModel("tmp", histTitle.data(), dataDim1.nBins, dataDim1.edges[0], dataDim1.edges[1], dataDim2.nBins, dataDim2.edges[0], dataDim2.edges[1], dataDim3.nBins, dataDim3.edges[0], dataDim3.edges[1]);
      } else {
        // first convert all fixed size bins to variable size bining
        for (auto& dataDim : dataDims) {
          if (dataDim.nBins) {
            double_t binWidth = (dataDim.edges[1] - dataDim.edges[0]) / dataDim.nBins;
            vector<double_t> edges = {dataDim.edges[0]};
            for (int32_t i = 1; i <= dataDim.nBins; ++i) {
              edges.push_back(dataDim.edges[0] + i * binWidth);
            }
            dataDim.edges = edges;
            dataDim.nBins = 0;
          }
        }
        histModel = ROOT::RDF::TH3DModel("tmp", histTitle.data(), static_cast<int32_t>(dataDim1.edges.size()) - 1, dataDim1.edges.data(), static_cast<int32_t>(dataDim2.edges.size()) - 1, dataDim2.edges.data(), static_cast<int32_t>(dataDim3.edges.size()) - 1, dataDim3.edges.data());
      }
      if (hasWeights) {
        obj = node.Histo3D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3", "SRP_AXIS_W")->Clone(name.data());
      } else {
        obj = node.Histo3D(histModel, "SRP_AXIS_1", "SRP_AXIS_2", "SRP_AXIS_3")->Clone(name.data());
      }
    }
  } else {
    if (isProfile) {
      ERROR("Too many dimensions for a profile.");
    } else {
      auto histModel = ROOT::RDF::THnDModel();
      bool allFixed = true;
      vector<int32_t> nBinsVec;
      vector<double_t> xMinVec;
      vector<double_t> xMaxVec;
      for (auto& dataDim : dataDims) {
        if (!dataDim.nBins) {
          allFixed = false;
          nBinsVec.push_back(static_cast<int32_t>(dataDim.edges.size()) - 1);
        } else {
          nBinsVec.push_back(dataDim.nBins);
          xMinVec.push_back(dataDim.edges[0]);
          xMaxVec.push_back(dataDim.edges[1]);
        }
      }

      if (allFixed) {
        histModel = ROOT::RDF::THnDModel("tmp", histTitle.data(), static_cast<int32_t>(dataDims.size()), nBinsVec, xMinVec, xMaxVec);
      } else {
        vector<vector<double_t>> xBins;
        for (auto& dataDim : dataDims) {
          if (dataDim.nBins) {
            double_t binWidth = (dataDim.edges[1] - dataDim.edges[0]) / dataDim.nBins;
            vector<double_t> edges = {dataDim.edges[0]};
            for (int32_t i = 1; i <= dataDim.nBins; ++i) {
              edges.push_back(dataDim.edges[0] + i * binWidth);
            }
            dataDim.edges = edges;
            dataDim.nBins = 0;
          }
          xBins.push_back(dataDim.edges);
        }
        histModel = ROOT::RDF::THnDModel("tmp", histTitle.data(), static_cast<int32_t>(dataDims.size()), nBinsVec, xBins);
      }
      vector<string> colNames;
      axisID = 1;
      for (auto& dataDim : dataDims) {
        colNames.push_back("SRP_AXIS_" + std::to_string(axisID));
        ++axisID;
      }
      if (hasWeights) {
        colNames.push_back("SRP_AXIS_W");
      }
      obj = node.HistoND(histModel, colNames)->Clone(name.data());
    }
  }
  if (obj && obj->InheritsFrom(TH1::Class())) static_cast<TH1*>(obj)->SetDirectory(0);
  return obj;
}

//****************************************************************************************
/**
 * Defines a set of standard base plots with quadratic axis frame and consistent layout among them.
 * The following pre-defined layouts are available: 1d, 2d, 1d_ratio and 1d_3panels.
 * By specifying a screen resolution in dpi, the size of the plot displayed in interactive mode (or when saved as bitmap) can be modified.
 * For multiple of 72dpi the resulting plot will have the best agreement with the pdf version.
 */
//****************************************************************************************
Plot PlotManager::MakeBasePlot(const string& name, double_t screenResolution)
{
  // info: the PDF backend of ROOT can only create plots with resolution of 72 dpi and maximum sizes defined by the A4 format (20cm x 26cm -> 567px x 737px)
  int32_t pixelBase = static_cast<int32_t>(std::round(screenResolution * 567. / 72.));  // 567p / 72dpi == 20cm / 2.54in/cm (final pdf size)
  double_t frameSize = 0.81;                                                            // size of (quadratic) axis frame in percentage of pixelBase
  double_t axisMargin = 0.15;                                                           // margins of x and y axes in units relative to pixelBase side length (corresponds to the space in bottom and left of the plot to the axis frame)
  double_t defaultTextSize = 0.04;                                                      // default text size relative to pixelBase
  double_t axisTitleSize = 0.05;                                                        // text size of axis titles relative to pixelBase
  double_t wideSideScale = 1.3;                                                         // for asymmetric plots: scale of the larger side wrt. pixelBase (corresponds to the maximum / optimum of 26cm / 20cm)

  double_t markerSize = 1.4;
  uint8_t lineWidth = 5;

  const vector<int16_t> defaultColors = {kBlack, kBlue + 1, kRed + 1, kYellow + 1,
                                         kMagenta - 4, kGreen + 3, kOrange + 1,
                                         kViolet - 3, kCyan + 2, kPink + 3, kTeal - 7,
                                         kMagenta + 1, kPink + 8, kCyan - 6,
                                         kMagenta, kRed + 2, kGreen + 2,
                                         kOrange + 2, kMagenta + 2, kYellow + 3,
                                         kGray + 2, kBlue + 2, kYellow + 2,
                                         kRed, kBlue, kMagenta + 3,
                                         kGreen + 4, 28, 8, 15, 17, 12};

  if (name == "1d") {
    // -----------------------------------------------------------------------
    Plot plot(name);
    plot.SetDimensions(pixelBase, pixelBase, true);
    plot.SetTransparent();
    plot[0].SetFrameFill(10, 1001);
    plot[0].SetDefaultColors(defaultColors);
    plot[0].SetDefaultFillStyles({0});
    plot[0].SetDefaultMarkerStyles({kFullCircle});
    plot[0].SetDefaultLineStyles({kSolid});
    plot[0].SetDefaultTextFont(42);
    plot[0].SetDefaultTextSize(defaultTextSize);
    plot[0].SetDefaultMarkerSize(markerSize);
    plot[0].SetDefaultLineWidth(lineWidth);
    plot[0].SetDefaultDrawingOptionHist2d(colz);
    plot[0].SetDefaultDrawingOptionGraph(points);
    plot[0].SetTransparent();
    plot[0].SetMargins(1. - (frameSize + axisMargin), axisMargin, axisMargin, 1. - (frameSize + axisMargin));
    plot[0]['X'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plot[0]['Y'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plot[1].SetPosition(0., 0., 1., 1.);
    return plot;
  }  // -----------------------------------------------------------------------

  if (name == "1d_ratio") {
    // -----------------------------------------------------------------------
    Plot plot(name);
    plot.SetDimensions(pixelBase, static_cast<int32_t>(std::round(wideSideScale * pixelBase)), true);
    plot.SetTransparent();
    plot[0].SetFrameFill(10, 1001);
    plot[0].SetDefaultColors(defaultColors);
    plot[0].SetDefaultFillStyles({0});
    plot[0].SetDefaultMarkerStyles({kFullCircle});
    plot[0].SetDefaultLineStyles({kSolid});
    plot[0].SetDefaultMarkerSize(markerSize);
    plot[0].SetDefaultLineWidth(lineWidth);
    plot[0].SetDefaultDrawingOptionHist2d(colz);
    plot[0].SetDefaultDrawingOptionGraph(points);
    plot[0].SetDefaultTextFont(42);
    plot[0].SetTransparent();
    double_t ratioFraction = 1. - (1. - axisMargin) / wideSideScale;
    plot[1].SetPosition(0., ratioFraction, 1., 1.);
    plot[2].SetPosition(0., 0., 1., ratioFraction);
    plot[1].SetMargins(1. - frameSize / (1 - axisMargin), 0., axisMargin, 1. - (frameSize + axisMargin));
    plot[2].SetMargins(0., 1. / (wideSideScale * ratioFraction) * axisMargin, axisMargin, 1. - (frameSize + axisMargin));
    plot[1].SetDefaultTextSize(1. / (wideSideScale * (1. - ratioFraction)) * defaultTextSize);
    plot[2].SetDefaultTextSize(1. / (wideSideScale * ratioFraction) * defaultTextSize);
    plot[1]['X'].SetTitleSize(0.).SetLabelSize(0.).SetOppositeTicks().SetNoExponent();
    plot[1]['Y'].SetTitleSize(1. / (wideSideScale * (1 - ratioFraction)) * axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plot[2]['X'].SetTitleSize(1. / (wideSideScale * ratioFraction) * axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plot[2]['Y'].SetTitleSize(1. / (wideSideScale * ratioFraction) * axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plot[2]['Y'].SetTitleOffset(0.6);  // MEMO: for some reason the ROOT automatic title offset determination does not work for the lower plot
    plot[2].SetRefFunc("1").SetColor(kBlack);
    plot[2]['X'].SetTickLength(0.06);
    plot[2]['Y'].SetNumDivisions(305).SetTitleCenter();
    return plot;
  }  // -----------------------------------------------------------------------

  if (name == "2d") {
    // -----------------------------------------------------------------------
    Plot plot(name);
    plot.SetDimensions(static_cast<int32_t>(std::round(wideSideScale * pixelBase)), pixelBase, true);
    plot.SetTransparent();
    plot[0].SetFrameFill(10, 1001);
    plot[0].SetDefaultColors(defaultColors);
    plot[0].SetDefaultFillStyles({0});
    plot[0].SetDefaultMarkerStyles({kFullCircle});
    plot[0].SetDefaultLineStyles({kSolid});
    plot[0].SetDefaultDrawingOptionHist2d(colz);
    plot[0].SetDefaultDrawingOptionGraph(points);
    plot[0].SetDefaultTextFont(42);
    plot[0].SetDefaultTextSize(defaultTextSize);
    plot[0].SetDefaultMarkerSize(markerSize);
    plot[0].SetDefaultLineWidth(lineWidth);
    plot[0].SetTransparent();
    plot[0].SetMargins(1. - (frameSize + axisMargin), axisMargin, axisMargin / wideSideScale, 1. - (frameSize + axisMargin) / wideSideScale);
    plot[0]['X'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plot[0]['Y'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plot[0]['Z'].SetTitleSize(axisTitleSize).SetMaxDigits(3).SetTitleOffset(1.5);
    plot[0].SetRedrawAxes();
    plot[1].SetPosition(0., 0., 1., 1.);
    return plot;
  }  // -----------------------------------------------------------------------

  if (name == "1d_3panels") {
    // -----------------------------------------------------------------------
    // extension of the 1d base plot with adjacing panels
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
    // text scaling factors (for the second and third pad the text will be calculated wrt. the width instead of the height (since it is smaller) and therefore need additional scaling
    double_t textScalePad2 = totalPixelHeight / widthFramePixel;
    double_t textScalePad3 = totalPixelHeight / (widthFramePixel + rightMarginPixel);
    // tick length is specified relative to pad width -> since width is different for the three pads, need adjustments to have consistent lengths between the pads
    // gauge it to the first pad -> scale by relative width decrease of pad 2 and 3 wrt pad 1
    double_t tickLengthPad1 = 0.03;
    double_t totalPixelWidthPad1 = widthFramePixel + leftMarginPixel;
    double_t tickLengthPad2 = tickLengthPad1 * widthFramePixel / totalPixelWidthPad1;
    double_t tickLengthPad3 = tickLengthPad1 * (widthFramePixel + rightMarginPixel) / totalPixelWidthPad1;

    Plot plot(name);
    plot.SetDimensions(totalPixelWidth, totalPixelHeight, true);
    plot.SetTransparent();
    plot[0].SetFrameFill(10, 1001);
    plot[0].SetDefaultColors(defaultColors);
    plot[0].SetDefaultFillStyles({0});
    plot[0].SetDefaultMarkerStyles({kFullCircle});
    plot[0].SetDefaultLineStyles({kSolid});
    plot[0].SetDefaultTextFont(42);
    plot[0].SetDefaultTextSize(defaultTextSize);
    // plot[0].SetDefaultLineWidth(1); // adjusted to make it look the same in lower pdf resolution
    plot[0].SetDefaultMarkerSize(markerSize);
    plot[0].SetDefaultLineWidth(lineWidth);
    plot[0].SetDefaultDrawingOptionHist2d(colz);
    plot[0].SetDefaultDrawingOptionGraph(points);
    plot[0].SetTransparent();
    plot[1].SetPosition(0., 0., x1, 1.);
    plot[2].SetPosition(x1, 0., x2, 1.);
    plot[3].SetPosition(x2, 0., 1., 1.);
    plot[1].SetMargins(1. - (frameSize + axisMargin), axisMargin, leftMarginPad1, 0.0);
    plot[2].SetMargins(1. - (frameSize + axisMargin), axisMargin, 0.0, 0.0);
    plot[3].SetMargins(1. - (frameSize + axisMargin), axisMargin, 0.0, rightMarginPad3);
    plot[0]['X'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plot[0]['Y'].SetTitleSize(axisTitleSize).SetOppositeTicks().SetMaxDigits(3);
    plot[2].SetDefaultTextSize(textScalePad2 * defaultTextSize);
    plot[2]['X'].SetTitleSize(textScalePad2 * axisTitleSize);
    plot[2]['Y'].SetTitleSize(textScalePad2 * axisTitleSize);
    plot[3].SetDefaultTextSize(textScalePad3 * defaultTextSize);
    plot[3]['X'].SetTitleSize(textScalePad3 * axisTitleSize);
    plot[3]['Y'].SetTitleSize(textScalePad3 * axisTitleSize);
    plot[2]['Y'].SetTitle("").SetTitleSize(0.).SetLabelSize(0.);
    plot[3]['Y'].SetTitle("").SetTitleSize(0.).SetLabelSize(0.);
    plot[2]['X'].SetTickLength(tickLengthPad2);
    plot[3]['X'].SetTickLength(tickLengthPad3);
    return plot;
  }  // -----------------------------------------------------------------------

  ERROR("Unknown default base plot {}.", name);
  return Plot();
}

//****************************************************************************************
/**
 * Reads relevant paths from config file.
 */
//****************************************************************************************
tuple<string, string, string> PlotManager::GetProjectSettings(string projectName)
{
  string configPath = expand_path((gSystem->Getenv("SCIROOPLOT_CONFIG_PATH")) ? "${SCIROOPLOT_CONFIG_PATH}" : "~/.SciRooPlot");
  if (std::filesystem::path(configPath).is_relative()) {
    ERROR("SCIROOPLOT_CONFIG_PATH must not be relative.");
    std::exit(EXIT_FAILURE);
  }
  if (configPath.empty()) {
    ERROR("SCIROOPLOT_CONFIG_PATH must not be empty.");
    std::exit(EXIT_FAILURE);
  }
  string configFileName = configPath + "/projects.info";

  string outputDir;
  if (file_exists(configFileName)) {
    using boost::property_tree::read_info;
    ptree configTree;
    read_info(configFileName, configTree);
    if (projectName.empty()) {
      if (auto active = configTree.get_child_optional("@current")) {
        projectName = active.get().data();
      }
    }
    if (auto curConfig = configTree.get_child_optional(projectName)) {
      auto tree = curConfig.get();
      if (auto property = tree.get_child_optional("OUT")) {
        outputDir = expand_path(property->get_value<string>());
        if (outputDir.empty()) {
          WARNING(R"(Please run "srp set {} OUT </path/to/store/output>".)", projectName);
        }
      }
    }
  }
  string datasetFile = configPath + "/" + projectName + "/datasets.info";
  string plotFile = configPath + "/" + projectName + "/plots.info";
  if (projectName.empty()) {
    datasetFile = "";
    plotFile = "";
  }
  return {datasetFile, plotFile, outputDir};
}

//****************************************************************************************
/**
 * Saves all required plot definitions and input file locations of the project.
 */
//****************************************************************************************
void PlotManager::SaveProject(const string& projectName)
{
  namespace fs = std::filesystem;
  auto [datasetFile, plotFile, outputDir] = GetProjectSettings(projectName);

  if (fs::create_directories(fs::path(datasetFile).parent_path())) {
    INFO("Created config folder for project {}: {}", projectName, fs::path(datasetFile).parent_path().string());
  }

  // move user data stored in current session to project folder
  fs::path curUserDataFile = fs::path(datasetFile).parent_path() / "../UserData.root";
  if (fs::exists(curUserDataFile)) {
    fs::path userDataFile = fs::path(datasetFile).parent_path() / "UserData.root";
    fs::rename(curUserDataFile, userDataFile);
  }

  SaveDatasets(datasetFile);
  SavePlots(plotFile);

  if (!mOutputDirectory.empty()) {
    WARNING("The output directory of a project can only be modified with the srp app.");
  }

  // create a csv file for tab completion
  std::ofstream tabCompFile;
  tabCompFile.open(fs::path(plotFile).parent_path() / "tabcomp.csv");
  for (const auto& plot : mPlots) {
    string line = plot.GetName() + "," + plot.GetGroup() + "\n";
    tabCompFile << line;
  }
  tabCompFile.close();
}
}  // end namespace SciRooPlot
