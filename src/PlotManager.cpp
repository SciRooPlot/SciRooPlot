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
 * The GNU General Public License can be found here: <https://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#include "SciRooPlot/PlotManager.h"

#include "SciRooPlot/Logging.h"
#include "SciRooPlot/PanelLayout.h"

#include <ROOT/RCsvDS.hxx>
#include <ROOT/RDFHelpers.hxx>
#include <ROOT/RDataFrame.hxx>
#include <TApplication.h>
#include <TBranch.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TClass.h>
#include <TCollection.h>
#include <TError.h>
#include <TF1.h>
#include <TF2.h>
#include <TF3.h>
#include <TFile.h>
#include <TFolder.h>
#include <TGraph2D.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TKey.h>
#include <TLeaf.h>
#include <TList.h>
#include <TPave.h>
#include <TROOT.h>
#include <TRootCanvas.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeIndex.h>

#include <boost/property_tree/info_parser.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fnmatch.h>
#include <fstream>
#include <functional>
#include <glob.h>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DataFrameRequest.h"
#include "PlotPainter.h"
#include "Validation.h"
#include "util/Paths.h"
#include "util/Regex.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"

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
using util::expand_path;
using util::file_exists;
using util::make_scope_guard;
using util::RegexMatcher;
using util::split_string;
using util::str_contains;
using util::str_ends_with;

//**************************************************************************************************
/**
 * Constructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::PlotManager(const std::string& projectName)
  : mProjectName(projectName), mUserDataFile(((mProjectName.empty()) ? Config::Get().Path() : Config::Get().ProjectPath(mProjectName)) / (mProjectName.empty() ? "UserData_" + std::to_string(gSystem->GetPid()) + "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".root" : string{"UserData.root"}))
{
  if (!gApplication) {
    new TApplication("MainApp", 0, nullptr);
  }

  if (!projectName.empty() && Config::Get().Exists(projectName)) {
    mOutputDirectory = Config::Get().OutputDir(projectName);
  }
}

//**************************************************************************************************
/**
 * Destructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::~PlotManager()
{
  if (mProjectName.empty() && std::filesystem::exists(mUserDataFile)) {
    std::filesystem::remove(mUserDataFile);
  }
}

//**************************************************************************************************
/**
 * Save stored plots to .root file.
 */
//**************************************************************************************************
bool PlotManager::SavePlotsToRootFile() const
{
  if (!mCanvasRegistry.empty()) {
    if (!CreateOutputDirectory()) return false;
    TFile outputFile((mOutputDirectory + "/" + mPlotsRootFile).data(), "RECREATE");
    if (outputFile.IsZombie()) {
      ERROR("Could not create output file {}/{}.", mOutputDirectory, mPlotsRootFile);
      return false;
    }
    outputFile.cd();
    // the colours a pad uses are defined by its setup TExec; ROOT's own colour list is saved with the first canvas as usual
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
  return true;
}

//**************************************************************************************************
/**
 * Save buffered data .root file.
 */
//**************************************************************************************************
bool PlotManager::SaveDataToRootFile() const
{
  if (!CreateOutputDirectory()) return false;
  TFile outputFile((mOutputDirectory + "/" + mDataRootFile).data(), "RECREATE");
  if (outputFile.IsZombie()) {
    ERROR("Could not create output file {}/{}.", mOutputDirectory, mDataRootFile);
    return false;
  }
  for (const auto& [dataSource, buffer] : mDataBuffer) {
    // the folder of a restricted data source (dataSource:some/folder) becomes subdirectories
    string sourcePath = dataSource;
    std::replace(sourcePath.begin(), sourcePath.end(), ':', '/');
    TDirectory* sourceDir = outputFile.mkdir(sourcePath.data(), "", true);
    if (!sourceDir) {
      ERROR("Could not create directory {} in {}.", sourcePath, mDataRootFile);
      continue;
    }
    const bool isUserDefined = (dataSource == "USER_FUNCTIONS" || dataSource == "USER_GRAPHS");
    for (const auto& [dataName, dataPtr] : buffer) {
      if (!dataPtr) continue;
      // the name is the path of the data within its source, for tree projections followed by the request {...};
      // only that path becomes subdirectories, since request expressions and user-defined functions may contain '/' and ':'
      const size_t pathEnd = (isUserDefined) ? string::npos : dataName.rfind('/', dataName.find('{'));
      TDirectory* dir = sourceDir;
      if (pathEnd != string::npos) {
        dir = sourceDir->mkdir(dataName.substr(0, pathEnd).data(), "", true);
        if (!dir) {
          ERROR("Could not create directory {} for {} in {}.", dataName.substr(0, pathEnd), dataName, mDataRootFile);
          continue;
        }
      }
      dir->cd();
      dataPtr->Write(((pathEnd == string::npos) ? dataName : dataName.substr(pathEnd + 1)).data());
    }
  }
  outputFile.Close();
  INFO("Saved data to file {}/{}.", mOutputDirectory, mDataRootFile);
  return true;
}

//**************************************************************************************************
/**
 * Create the output directory if needed; like for the other output modes, its parent must already exist.
 */
//**************************************************************************************************
bool PlotManager::CreateOutputDirectory() const
{
  if (mOutputDirectory.empty()) {
    ERROR("No output directory was specified.");
    return false;
  }
  std::filesystem::path outputDir = std::filesystem::path(mOutputDirectory).lexically_normal();
  if (!outputDir.has_filename()) outputDir = outputDir.parent_path();  // strip trailing slash
  if (!std::filesystem::exists(outputDir.parent_path())) {
    ERROR("Parent path {} of output directory does not exist.", outputDir.parent_path().string());
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directory(outputDir, ec);
  if (ec) {
    ERROR("Could not create output directory {}: {}.", outputDir.string(), ec.message());
    return false;
  }
  return true;
}

//**************************************************************************************************
/**
 * Sets path for output files. Plots will be stored in hierarchical structure according to defined groups.
 */
//**************************************************************************************************
void PlotManager::SetOutputDirectory(const string& path)
{
  mOutputDirectory = expand_path(path);
}

//**************************************************************************************************
/**
 * Define the inputs of a data source: files, folders within files (file.root:folder), directories or wildcard patterns.
 */
//**************************************************************************************************
void PlotManager::AddDataSource(const string& dataSource, const vector<string>& inputs, bool replace)
{
  if (dataSource.empty()) {
    ERROR("Specify a name for the data source.");
    return;
  }
  if (auto illegal = find_illegal_name_char(dataSource)) {
    ERROR("DataSource '{}' contains illegal character '{}'.", dataSource, *illegal);
    return;
  }
  if (replace) {
    mInputs.erase(dataSource);
  }
  for (const auto& input : inputs) {
    if (std::filesystem::path(expand_path(input)).is_relative()) {
      WARNING("The path of an input must not be relative. Skipping {}.", input);
      continue;
    }
    auto& inputsOfDataSource = mInputs[dataSource];
    if (std::find(inputsOfDataSource.begin(), inputsOfDataSource.end(), input) == inputsOfDataSource.end()) {
      inputsOfDataSource.push_back(input);
    }
  }
}
void PlotManager::AddDataSource(const std::string& dataSource, std::initializer_list<string> inputs, bool replace)
{
  AddDataSource(dataSource, std::vector<std::string>(inputs), replace);
}
void PlotManager::AddDataSource(const string& dataSource, const string& input, bool replace)
{
  AddDataSource(dataSource, {input}, replace);
}

//**************************************************************************************************
/**
 * Define input data for user defined unique dataSource.
 */
//**************************************************************************************************
void PlotManager::AddDataSource(const string& dataSource, const vector<TObject*>& inputs, bool replace)
{
  if (dataSource.empty()) {
    ERROR("Specify a name for the data source.");
    return;
  }
  if (auto illegal = find_illegal_name_char(dataSource)) {
    ERROR("DataSource '{}' contains illegal character '{}'.", dataSource, *illegal);
    return;
  }
  string mode = mUserDataFileInitialized ? "UPDATE" : "RECREATE";
  if (!mProjectName.empty()) {
    std::filesystem::create_directories(Config::Get().ProjectPath(mProjectName));
  }

  TFile file(mUserDataFile.data(), mode.data());
  if (file.IsZombie()) {
    ERROR("Could not save ROOT data to data source {}.", dataSource);
    return;
  }
  mUserDataFileInitialized = true;

  TDirectory* dir = file.GetDirectory(dataSource.data());
  if (dir && replace) {
    file.rmdir(dataSource.data());
    dir = nullptr;
  }
  if (!dir) {
    dir = file.mkdir(dataSource.data());
  }
  if (!dir) {
    ERROR("Could not create directory {} in {}.", dataSource, mUserDataFile);
    return;
  }
  dir->cd();
  set<string> addedNames;
  for (auto object : inputs) {
    if (!object) continue;
    string name = object->GetName();
    if (name.empty()) {
      WARNING("Cannot add nameless object of type {} to dataSource {}.", object->ClassName(), dataSource);
      continue;
    }
    if (!addedNames.insert(name).second) {
      WARNING("Data source {} receives multiple objects named {}. Only the last one is kept.", dataSource, name);
    }
    object->Write(nullptr, TObject::kWriteDelete);
  }
  file.Close();
  AddDataSource(dataSource, {mUserDataFile + ":" + dataSource}, replace);
}
void PlotManager::AddDataSource(const string& dataSource, TObject* input, bool replace)
{
  AddDataSource(dataSource, vector<TObject*>{input}, replace);
}

//**************************************************************************************************
/**
 * Save dataSource properties currently defined in the manager to a config file.
 */
//**************************************************************************************************
void PlotManager::SaveDataSources(const optional<string>& file) const
{
  ptree dataSourcesTree;
  for (const auto& [dataSource, inputs] : mInputs) {
    ptree inputsOfDataSource;
    for (const auto& input : inputs) {
      inputsOfDataSource.add("INPUT", input);
    }
    dataSourcesTree.put_child(dataSource, inputsOfDataSource);
  }
  std::filesystem::path filePath = expand_path((file) ? *file : Config::Get().DataSourcesFile(mProjectName));
  if (std::filesystem::create_directories(filePath.parent_path())) {
    INFO("Created config folder: {}.", filePath.parent_path().string());
  }
  using boost::property_tree::write_info;
  write_info(filePath.string(), dataSourcesTree);
}

//**************************************************************************************************
/**
 * Load dataSource properties from config file into manager.
 */
//**************************************************************************************************
void PlotManager::LoadDataSources(const optional<string>& file, bool replace)
{
  ptree dataSourcesTree;
  try {
    using boost::property_tree::read_info;
    read_info(expand_path((file) ? *file : Config::Get().DataSourcesFile(mProjectName)), dataSourcesTree);
  } catch (const std::exception& e) {
    ERROR("Cannot load dataSources file: {}.", e.what());
    return;
  }
  for (const auto& [dataSource, inputsTree] : dataSourcesTree) {
    vector<string> inputs;  // as they were added (directories and wildcards are expanded when reading)
    for (const auto& input : inputsTree) {
      inputs.push_back(input.second.get_value<string>());
    }
    AddDataSource(dataSource, inputs, replace);
  }
}

//**************************************************************************************************
/**
 * Paths of the folders matching a (wildcard) path within a directory, e.g. DF_* (in the order of the file).
 */
//**************************************************************************************************
vector<string> PlotManager::MatchFolders(TDirectory* topDir, const string& pattern)
{
  vector<string> folders;
  std::function<void(TDirectory*, const vector<string>&, size_t, const string&)> match;
  match = [&](TDirectory* dir, const vector<string>& levels, size_t level, const string& path) {
    if (level == levels.size()) {
      folders.push_back(path);
      return;
    }
    set<string> visited;  // keys can appear in several cycles
    for (auto* key : TRangeDynCast<TKey>(dir->GetListOfKeys())) {
      if (!key || !visited.insert(key->GetName()).second) continue;
      if (fnmatch(levels[level].data(), key->GetName(), 0) != 0) continue;
      if (!TClass::GetClass(key->GetClassName()) || !TClass::GetClass(key->GetClassName())->InheritsFrom(TDirectory::Class())) continue;
      if (auto* subDir = dir->GetDirectory(key->GetName())) {
        match(subDir, levels, level + 1, path.empty() ? key->GetName() : path + "/" + key->GetName());
      }
    }
  };
  match(topDir, split_string(pattern, '/'), 0, "");
  return folders;
}

//**************************************************************************************************
/**
 * Resolve the inputs of a data source as added into the list of inputs to be searched (files or folders within files):
 * - files keep the order in which they were added, directories are replaced by the files they contain
 *   (recursively, in alphabetical order since directory iteration order is unspecified),
 * - wildcards (*, ?, [...]) in file paths are expanded to all matching files (alphabetical order),
 * - wildcards in the folder after the file name (file.root:DF_*) are expanded to all matching folders (order in the file).
 * Inputs reached more than once are only kept at their first position.
 * A data source restricted to a folder (dataSource:some/folder) consists of that folder within each root file input of the data source
 * (the folder may contain wildcards as well; tables have no folders and are skipped).
 */
//**************************************************************************************************
vector<string> PlotManager::ExpandInputs(const string& dataSource) const
{
  vector<string> inputs;
  const auto subFolderPos = dataSource.find(':');
  const string subFolder = (subFolderPos == string::npos) ? "" : dataSource.substr(subFolderPos + 1);
  auto inputsIt = mInputs.find(dataSource.substr(0, subFolderPos));
  if (inputsIt == mInputs.end()) return inputs;

  auto hasWildcard = [](const string& str) { return str.find_first_of("*?[") != string::npos; };
  set<string> seen;
  auto addInput = [&](const string& input) {
    if (seen.insert(expand_path(input)).second) inputs.push_back(input);
  };

  // folders matching the (wildcard) path within a root file, e.g. DF_* or run*/sub
  auto expandFolders = [&](const string& fileName, const string& folderPattern) {
    TFile file(fileName.data(), "READ");
    if (file.IsZombie()) {
      WARNING("Cannot open input file {} (data source {}).", fileName, dataSource);
      return vector<string>{};
    }
    return MatchFolders(&file, folderPattern);
  };

  auto addFile = [&](const string& filePart, const string& folderPart) {
    if (!subFolder.empty() && !str_ends_with(filePart, ".root")) return;  // tables have no folders
    const string folderPath = (subFolder.empty() || folderPart.empty()) ? folderPart + subFolder : folderPart + "/" + subFolder;
    if (hasWildcard(folderPath)) {
      const auto folders = expandFolders(expand_path(filePart), folderPath);
      // inputs that do not contain the folder of a restricted data source are simply not part of it
      if (folders.empty() && subFolder.empty()) WARNING("No folders match {} in {} (data source {}).", folderPath, filePart, dataSource);
      for (const auto& folder : folders) {
        addInput(filePart + ":" + folder);
      }
    } else {
      addInput(folderPath.empty() ? filePart : filePart + ":" + folderPath);
    }
  };

  std::function<void(const string&)> addEntry;
  addEntry = [&](const string& entry) {
    auto fileAndFolder = split_string(entry, ':', true);
    const string& filePart = fileAndFolder[0];
    const string folderPart = (fileAndFolder.size() > 1) ? fileAndFolder[1] : "";

    if (hasWildcard(filePart)) {
      glob_t matches{};
      const bool found = (glob(expand_path(filePart).data(), 0, nullptr, &matches) == 0);
      if (!found) WARNING("No files match {} (data source {}).", filePart, dataSource);
      for (size_t i = 0; found && i < matches.gl_pathc; ++i) {
        addEntry(string(matches.gl_pathv[i]) + ((fileAndFolder.size() > 1) ? ":" + folderPart : ""));
      }
      globfree(&matches);
      return;
    }
    if (str_ends_with(filePart, ".root") || str_ends_with(entry, mTableFileEndings)) {
      addFile(filePart, folderPart);
      return;
    }
    string path = expand_path(entry);
    if (!std::filesystem::is_directory(path)) {
      WARNING("Data source entry {} (data source {}) is neither a recognized file type nor an existing directory. Skipping.", entry, dataSource);
      return;
    }
    vector<string> dirFileNames;
    for (const auto& file : std::filesystem::recursive_directory_iterator(path)) {
      if (file.path().extension() == ".root" || str_contains(file.path().extension(), mTableFileEndings)) {
        dirFileNames.push_back(file.path().string());
      }
    }
    std::sort(dirFileNames.begin(), dirFileNames.end());
    for (const auto& fileName : dirFileNames) {
      addFile(fileName, "");
    }
  };
  for (const auto& entry : inputsIt->second) {
    addEntry(entry);
  }
  return inputs;
}

//**************************************************************************************************
/**
 * Whether a data source is defined (for one restricted to a folder: dataSource:some/folder, the data source it is part of).
 */
//**************************************************************************************************
bool PlotManager::IsDataSourceDefined(const string& dataSource) const
{
  return mInputs.find(dataSource.substr(0, dataSource.find(':'))) != mInputs.end();
}

//**************************************************************************************************
/**
 * How data is referred to in messages: dataSource:name, or dataSource:folder/name for a data source restricted to a folder.
 */
//**************************************************************************************************
string PlotManager::DataLocation(const string& dataSource, const string& name)
{
  return dataSource + ((dataSource.find(':') == string::npos) ? ":" : "/") + name;
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
    ERROR("You cannot use reserved group name 'BASE_PLOTS'.");
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
 * Base plots with consistent layout among them (see PlotManager.h). The name describes the layout:
 * <type>[_wide|_tall][_<columns>x<rows>][_ratio][_gap]. The geometry and the sizes come from the PanelLayout
 * (its defaults are the proportions of the 788 x 788 pixel standard plot), the look is set with the ordinary
 * plot setters, and the conventions of the ratio pads are added afterwards.
 */
//**************************************************************************************************
Plot PlotManager::MakeBasePlot(const string& name)
{
  static const std::regex pattern(R"(^(1d|2d)(?:_(wide|tall))?(?:_([1-9]\d*)x([1-9]\d*))?(_ratio)?(_gap)?$)");
  std::smatch match;
  if (!std::regex_match(name, match, pattern)) {
    ERROR("Unknown base plot {}: the name must follow <type>[_wide|_tall][_<columns>x<rows>][_ratio][_gap] with type 1d or 2d (e.g. 1d, 1d_ratio, 2d, 1d_wide, 1d_3x1, 1d_2x1_ratio).", name);
    return Plot();
  }
  const bool zAxis = (match[1] == "2d");
  const string shape = match[2];
  const int32_t nCols = match[3].matched ? std::stoi(match[3]) : 1;
  const int32_t nRows = match[4].matched ? std::stoi(match[4]) : 1;
  const bool ratio = match[5].matched;
  const bool gap = match[6].matched;
  if (nCols * nRows * (ratio ? 2 : 1) > 255) {
    ERROR("Base plot {} would have more than 255 pads.", name);
    return Plot();
  }

  // the arrangement of the panels, in units of the standard panel
  const double_t golden = 1.618;      // aspect ratio of wide and tall panels
  const double_t ratioHeight = 0.37;  // height of the ratio panels
  const double_t width = (shape == "wide") ? golden : 1.;
  const double_t height = (shape == "tall") ? golden : 1.;
  vector<vector<Panel>> rows;
  vector<uint8_t> ratioPads;
  uint8_t padID = 1;
  for (int32_t row = 0; row < nRows; ++row) {
    if (row > 0 && gap) rows.push_back({Gap()});
    vector<Panel> mainRow;
    vector<Panel> ratioRow;
    for (int32_t col = 0; col < nCols; ++col) {
      if (col > 0 && gap) {
        mainRow.push_back(Gap());
        ratioRow.push_back(Gap());
      }
      mainRow.push_back(zAxis ? Panel(width, height).ZAxis() : Panel(width, height));
      ratioRow.push_back(Panel(width, ratioHeight));
    }
    rows.push_back(mainRow);
    padID += nCols;
    if (ratio) {
      rows.push_back(ratioRow);
      for (int32_t col = 0; col < nCols; ++col)
        ratioPads.push_back(padID++);
    }
  }

  // the sizes shared by all base plots, in pixels of the canvas (these are the defaults of PanelLayout)
  PanelLayout sizes;
  sizes.SetTextSize(31.52).SetTitleSize(39.4).SetLineWidth(5).SetMarkerSize(1.4);

  // the look shared by all base plots: fonts, colours, markers and axis conventions
  Plot plot(name, PanelLayout(sizes, rows));
  plot.SetTransparent();
  plot[0].SetTransparent();
  plot[0].SetFrameFill(10, 1001);
  plot[0].SetDefaultTextFont(42);
  plot[0].SetDefaultColors({kBlack, kBlue + 1, kRed + 1, kYellow + 1, kMagenta - 4, kGreen + 3, kOrange + 1,
                            kViolet - 3, kCyan + 2, kPink + 3, kTeal - 7, kMagenta + 1, kPink + 8, kCyan - 6,
                            kMagenta, kRed + 2, kGreen + 2, kOrange + 2, kMagenta + 2, kYellow + 3,
                            kGray + 2, kBlue + 2, kYellow + 2, kRed, kBlue, kMagenta + 3, kGreen + 4, 28, 8, 15, 17, 12});
  plot[0].SetDefaultMarkerStyles({kFullCircle});
  plot[0].SetDefaultLineStyles({kSolid});
  plot[0].SetDefaultFillStyles({0});
  plot[0].SetDefaultDrawingOptionHist2d(colz);
  plot[0].SetDefaultDrawingOptionGraph(points);
  plot[0]['X'].SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
  plot[0]['Y'].SetOppositeTicks().SetMaxDigits(3);
  plot[0]['Z'].SetMaxDigits(3);
  if (zAxis) plot[0].SetRedrawAxes();

  // conventions of the ratio pads
  for (const uint8_t ratioPad : ratioPads) {
    plot[ratioPad].SetRefFunc("1");
    plot[ratioPad]['Y'].SetNumDivisions(305).SetTitleCenter();
  }
  return plot;
}

//**************************************************************************************************
/**
 * Add base plot to the manager.
 */
//**************************************************************************************************
void PlotManager::AddBasePlot(Plot basePlot)
{
  if (basePlot.GetName().empty()) {
    ERROR("Cannot add base plot without a name.");
    return;
  }
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
  AddPlot(std::move(plot));
}

//**************************************************************************************************
/**
 * Save plots matching name and group regex to file.
 */
//**************************************************************************************************
void PlotManager::SavePlots(const string& name, const string& group, const optional<string>& file) const
{
  ptree plotTree;
  RegexMatcher groupRegex = RegexMatcher::WithSubpaths(group, Config::Get().MatchContains(), Config::Get().MatchCaseInsensitive());
  RegexMatcher nameRegex(name, Config::Get().MatchContains(), Config::Get().MatchCaseInsensitive());
  if (!groupRegex.IsValid() || !nameRegex.IsValid()) {
    ERROR("Invalid regular expression.");
    return;
  }

  for (const vector<Plot>& plots : {std::ref(mBasePlots), std::ref(mPlots)}) {
    for (const Plot& plot : plots) {
      if (plot.GetGroup() != "BASE_PLOTS") {
        if (!groupRegex.Matches(plot.GetGroup())) continue;
        if (!nameRegex.Matches(plot.GetName())) continue;
      }
      plotTree.put_child(plot.GetUniqueName(), plot.GetPropertyTree());
    }
  }
  std::filesystem::path filePath = expand_path((file) ? *file : Config::Get().PlotsFile(mProjectName));
  if (filePath.empty()) {
    ERROR("No file path or project specified. Cannot save plots.");
    return;
  }
  if (std::filesystem::create_directories(filePath.parent_path())) {
    INFO("Created config folder: {}.", filePath.parent_path().string());
  }
  using boost::property_tree::write_info;
  write_info(filePath.string(), plotTree);
}

//**************************************************************************************************
/**
 * Function to load plots matching name and group regex from file.
 */
//**************************************************************************************************
bool PlotManager::LoadPlots(const string& name, const string& group, const optional<string>& file)
{
  uint32_t nMatched{};
  uint32_t nLoaded{};

  RegexMatcher groupRegex = RegexMatcher::WithSubpaths(group, Config::Get().MatchContains(), Config::Get().MatchCaseInsensitive());
  RegexMatcher nameRegex(name, Config::Get().MatchContains(), Config::Get().MatchCaseInsensitive());
  if (!groupRegex.IsValid() || !nameRegex.IsValid()) {
    ERROR("Invalid regular expression.");
    return false;
  }

  ptree fileTree;
  try {
    using boost::property_tree::read_info;
    read_info(expand_path(expand_path((file) ? *file : Config::Get().PlotsFile(mProjectName))), fileTree);
  } catch (const std::exception& e) {
    ERROR("Cannot open plots file: {}.", e.what());
    return false;
  }

  uint32_t nPlotsInFile{};
  uint32_t nBasePlotsLoaded{};
  for (const auto& plotTree : fileTree) {
    const string& curGroup = plotTree.second.get<string>("group");
    if (curGroup == "BASE_PLOTS") {
      try {
        Plot basePlot(plotTree.second);
        AddBasePlot(std::move(basePlot));
        ++nBasePlotsLoaded;
      } catch (const std::exception& e) {
        ERROR("Could not load base plot {} from file: {}.", plotTree.first, e.what());
      }
      continue;
    }
    ++nPlotsInFile;
    if (!groupRegex.Matches(curGroup)) continue;
    if (!nameRegex.Matches(plotTree.second.get<string>("name"))) continue;

    ++nMatched;
    try {
      Plot plot(plotTree.second);
      AddPlot(std::move(plot));
      ++nLoaded;
    } catch (const std::exception& e) {
      ERROR("Could not load plot {} from file: {}.", plotTree.first, e.what());
    }
  }
  if (nPlotsInFile == 0 && nBasePlotsLoaded > 0) {
    // file containing only base plots
    INFO("Loaded {} base plot{}.", nBasePlotsLoaded, (nBasePlotsLoaded == 1) ? "" : "s");
    return true;
  }
  if (nMatched == 0) {
    ERROR("Found no plots matching the request {}{}{} in {}{}{}.", logger::begin_color(logger::Color::Green), name, logger::end_color(), logger::begin_color(logger::Color::Yellow), group, logger::end_color());
  } else if (nLoaded == 0) {
    ERROR("{} plot(s) matched the request but none could be loaded.", nMatched);
  } else if (nLoaded > 1) {
    INFO("Found {} plots matching the request.", nLoaded);
  }
  return nLoaded != 0;
}

//**************************************************************************************************
/**
 * Generates plots matching name and group regex.
 */
//**************************************************************************************************
bool PlotManager::GeneratePlots(const string& mode, const string& name, const string& group)
{
  if (!is_valid_plot_mode(mode)) {
    string modeList;
    for (const auto& validMode : plot_modes())
      modeList += validMode + ", ";
    ERROR("Invalid mode '{}' (valid modes: {}gif+<centiseconds>).", mode, modeList);
    return false;
  }
  // plotting uses multi-threading, the classic canvas and fewer ROOT messages; the settings of the calling program are restored afterwards
  const bool wasMTEnabled = ROOT::IsImplicitMTEnabled();
  const bool wasWebDisplay = gROOT->IsWebDisplay();
  const TString webDisplay = gROOT->GetWebDisplay();
  auto globalsGuard = make_scope_guard([wasMTEnabled, wasWebDisplay, webDisplay, errorLevel = gErrorIgnoreLevel]() {
    gErrorIgnoreLevel = errorLevel;
    if (wasWebDisplay) gROOT->SetWebDisplay(webDisplay.Data());
    if (!wasMTEnabled) ROOT::DisableImplicitMT();
  });
  if (!wasMTEnabled) ROOT::EnableImplicitMT();
  gROOT->SetWebDisplay("off");
  gErrorIgnoreLevel = std::max<Int_t>(gErrorIgnoreLevel, kWarning);

  if (!mHasDisplay) {
    // determine OS dependent offset between window and frame
    // (GetWindowTopY gives the current coordinates of the window, but SetWindowPosition moves the frame instead of the window)
    mHasDisplay = false;
    TCanvas dummyCanvas("dummyCanvas", "dummyCanvas", 1, 1);
    if (auto canvasImp = dynamic_cast<TRootCanvas*>(dummyCanvas.GetCanvasImp())) {
      mHasDisplay = true;
      canvasImp->UnmapWindow();
      dummyCanvas.SetCanvasSize(1, 1);
      dummyCanvas.SetWindowPosition(50, 50);
      mWindowOffsetY = dummyCanvas.GetWindowTopY() - canvasImp->GetY();
    }
  }
  if ((mode == "show" || mode == "macro") && !*mHasDisplay) {
    ERROR("Mode '{}' needs a graphical display (is DISPLAY set?).", mode);
    return false;
  }
  // first determine which data needs to be loaded
  vector<Plot*> selectedPlots;
  map<int32_t, set<int32_t>> requiredData;

  RegexMatcher groupRegex = RegexMatcher::WithSubpaths(group, Config::Get().MatchContains(), Config::Get().MatchCaseInsensitive());
  RegexMatcher nameRegex(name, Config::Get().MatchContains(), Config::Get().MatchCaseInsensitive());
  if (!groupRegex.IsValid() || !nameRegex.IsValid()) {
    ERROR("Invalid regular expression.");
    return false;
  }

  for (auto& plot : mPlots) {
    if (!groupRegex.Matches(plot.GetGroup())) continue;
    if (!nameRegex.Matches(plot.GetName())) continue;
    selectedPlots.push_back(&plot);

    // determine which input data are needed for plots
    for (auto& [padID, pad] : plot.GetPads()) {
      if (auto& refFunc = pad.GetRefFunc()) {
        mDataBuffer[refFunc->GetDataSource()][refFunc->GetName()];
      } else {
        if (plot.GetBasePlotName()) {
          auto it = std::find_if(mBasePlots.begin(), mBasePlots.end(), [&](const auto& basePlot) { return *plot.GetBasePlotName() == basePlot.GetName(); });
          if (it != mBasePlots.end()) {
            if (auto& refFunc = (*it).GetPad(padID).GetRefFunc()) {
              mDataBuffer[refFunc->GetDataSource()][refFunc->GetName()];
            }
          }
        }
      }
      for (const auto& data : pad.GetData()) {
        mDataBuffer[data->GetDataSource()][data->GetName()];
        // also register requests without projection info: trees and tables then report that they cannot be plotted directly
        auto& dataInfos = mDataInfoBuffer[data->GetDataSource()][data->GetName()];
        const string nameSuffix = data->GetDataInfo().GetNameSuffix();
        auto iter = std::find_if(dataInfos.begin(), dataInfos.end(), [&](const auto& dataInfo) { return dataInfo.GetNameSuffix() == nameSuffix; });
        if (iter == dataInfos.end()) {
          dataInfos.push_back(data->GetDataInfo());
        }
        if (data->GetType() == "ratio") {
          const auto& ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
          mDataBuffer[ratio->GetDenomDataSource()][ratio->GetDenomName()];
          auto& denomDataInfos = mDataInfoBuffer[ratio->GetDenomDataSource()][ratio->GetDenomName()];
          const string denomNameSuffix = ratio->GetDenomDataInfo().GetNameSuffix();
          auto iter = std::find_if(denomDataInfos.begin(), denomDataInfos.end(), [&](const auto& dataInfo) { return dataInfo.GetNameSuffix() == denomNameSuffix; });
          if (iter == denomDataInfos.end()) {
            denomDataInfos.push_back(ratio->GetDenomDataInfo());
          }
        }
      }
    }
  }

  if (selectedPlots.empty()) {
    ERROR("No plots were created: found no plots matching the request {}{}{} in {}{}{}.", logger::begin_color(logger::Color::Green), name, logger::end_color(), logger::begin_color(logger::Color::Yellow), group, logger::end_color());
    return false;
  }

  size_t nItemsToRead{};
  set<string> sourcesToRead;
  for (const auto& [dataSource, buffer] : mDataBuffer) {
    for (const auto& [dataName, dataPtr] : buffer) {
      if (dataPtr) continue;
      ++nItemsToRead;
      sourcesToRead.insert(dataSource);
    }
  }
  mExpandedInputs.clear();
  size_t nInputs{};
  for (const auto& dataSource : sourcesToRead) {
    // data sources restricted to a folder are read together with the data source they are part of
    const string baseSource = dataSource.substr(0, dataSource.find(':'));
    if (mExpandedInputs.find(baseSource) == mExpandedInputs.end()) nInputs += (mExpandedInputs[baseSource] = ExpandInputs(baseSource)).size();
  }
  if (nItemsToRead) {
    auto plural = [](size_t n, const string& word) { return fmt::format("{} {}{}", n, word, (n == 1) ? "" : "s"); };
    INFO("Reading {} from {}{}.", plural(nItemsToRead, "data item"), (nInputs) ? plural(nInputs, "input") + " in " : "", plural(sourcesToRead.size(), "data source"));
  }
  try {
    if (!FillBuffer()) {
      // PrintBufferStatus(true);
      uint32_t nAffectedPlots{};
      set<std::pair<string, string>> missingItems;  // (dataSource, name+suffix)
      for (auto plot : selectedPlots) {
        auto missing = GetMissingData(*plot);
        if (!missing.empty()) {
          ++nAffectedPlots;
          for (const auto& [dataSource, name, dataInfo] : missing) {
            missingItems.emplace(dataSource, name + dataInfo.GetNameSuffix());
          }
        }
      }
      if (nAffectedPlots > 0) {
        ERROR("{} of {} plots cannot be created due to {} missing data item{}.", nAffectedPlots, selectedPlots.size(), missingItems.size(), (missingItems.size() == 1) ? "" : "s");
      }
    }
    mGifName.clear();
    mExitInteractiveBrowsing = false;
    mPlotViewHistory.clear();

    // generate plots
    bool allCreated = true;
    for (auto plot : selectedPlots) {
      if (!GeneratePlot(*plot, mode)) {
        auto missingData = GetMissingData(*plot);
        string message = fmt::format("Plot {}{}{} from group {}{}{} could not be created.", logger::begin_color(logger::Color::Green), plot->GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), plot->GetGroup(), logger::end_color());
        for (const auto& [dataSource, name, dataInfo] : missingData) {
          message += "\n         - missing " + DataLocation(dataSource, name);
          if (!dataInfo.dataDims.empty()) message += " [" + dataInfo.GetDescription() + "]";
          message += (!IsDataSourceDefined(dataSource)) ? " (data source not defined)" : "";
        }
        ERROR("{}", message);
        allCreated = false;
      }
      if (mExitInteractiveBrowsing) break;
    }
    if (!mGifName.empty()) {
      LOG("Saved gif {}.", mGifName);
    }
    bool saved = true;
    if (mode == "file") {
      saved = SavePlotsToRootFile();
    } else if (mode == "data") {
      saved = SaveDataToRootFile();
    }
    return allCreated && saved;
  } catch (const std::exception& e) {
    ERROR("An unexpected error occurred: {}.", e.what());
    return false;
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
  // data sources restricted to a folder (dataSource:some/folder) are read in the same pass as the data source they are part of,
  // so each input file is opened only once
  map<string, vector<string>> sourceGroups;  // data source -> requested data sources that are part of it (itself or restricted to a folder)
  for (const auto& [dataSource, buffer] : mDataBuffer) {
    sourceGroups[dataSource.substr(0, dataSource.find(':'))].push_back(dataSource);
  }
  for (const auto& [baseSource, dataSources] : sourceGroups) {
    map<string, unordered_map<string, set<string>>> requiredData;  // data source -> subdir -> names
    for (const auto& dataSource : dataSources) {
      for (auto& [dataName, dataPtr] : mDataBuffer[dataSource]) {
        if (dataPtr) continue;

        // generate user-defined functions on-the-fly
        if (dataSource == "USER_FUNCTIONS") {
          bool isValid{};
          int dim{};
          {
            auto errGuard = make_scope_guard([lvl = gErrorIgnoreLevel]() { gErrorIgnoreLevel = lvl; });
            gErrorIgnoreLevel = kFatal;
            TFormula formula("tmp", dataName.data(), false);
            isValid = formula.IsValid();
            dim = formula.GetNdim();
          }
          if (!isValid) {
            ERROR("'{}' is not a valid function expression.", dataName);
            continue;
          }
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
        } else if (dataSource == "USER_GRAPHS") {
          auto strs = split_string(dataName, ';');
          if (strs.size() == 2) {
            auto xStrs = split_string(strs[0], ',');
            auto yStrs = split_string(strs[1], ',');
            if (!xStrs.size() || xStrs.size() != yStrs.size()) {
              ERROR("Incompatible number of points.");
            } else {
              vector<double_t> x;
              vector<double_t> y;
              for (size_t i = 0; i < xStrs.size(); ++i) {
                x.push_back(std::stod(xStrs[i]));
                y.push_back(std::stod(yStrs[i]));
              }
              dataPtr.reset(new TGraph(static_cast<int32_t>(x.size()), x.data(), y.data()));
              static_cast<TGraph*>(dataPtr.get())->SetName(dataName.data());
            }
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
        requiredData[dataSource][std::move(path)].insert(std::move(name));
      }
    }
    auto allFound = [&]() { return std::all_of(requiredData.begin(), requiredData.end(), [](const auto& entry) { return entry.second.empty(); }); };

    // open all input files belonging to the data source and extract the data
    mTreeInputs.clear();
    auto expandedIt = mExpandedInputs.find(baseSource);
    for (const auto& inputRaw : (expandedIt != mExpandedInputs.end()) ? expandedIt->second : ExpandInputs(baseSource)) {
      if (allFound()) break;
      string input = expand_path(inputRaw);
      if (str_ends_with(input, mTableFileEndings)) {
        // tables have no folders: only part of the data source itself
        if (auto it = requiredData.find(baseSource); it != requiredData.end()) {
          string name = input.substr(input.rfind('/') + 1, input.rfind(".") - input.rfind('/') - 1);
          ReadTableData(input, name, baseSource);
          set<string>& wantedNames = it->second[""];
          wantedNames.erase(name);
          if (wantedNames.empty()) it->second.erase("");
        }
      }
      // check if only a sub-folder in input file should be searched
      auto fileNamePath = split_string(input, ':', true);
      string& fileName = fileNamePath[0];
      if (!str_ends_with(fileName, ".root")) continue;

      if (!std::filesystem::exists(fileName)) {
        WARNING("Input file {} not found (data source {}).", fileName, baseSource);
        continue;
      }
      TFile inputFile(fileName.data(), "READ");
      if (inputFile.IsZombie()) {
        WARNING("Cannot open input file {} (data source {}).", fileName, baseSource);
        continue;
      }

      for (auto& [dataSource, required] : requiredData) {
        if (required.empty()) continue;
        // where to search in this input: the folder of the input, for a restricted data source its folder within it
        const string inputFolder = (fileNamePath.size() > 1) ? fileNamePath[1] : "";
        const auto subFolderPos = dataSource.find(':');
        const bool isRestricted = (subFolderPos != string::npos);
        vector<string> folderPaths{inputFolder};
        if (isRestricted) {
          const string subFolder = dataSource.substr(subFolderPos + 1);
          const string folderPath = (inputFolder.empty()) ? subFolder : inputFolder + "/" + subFolder;
          folderPaths = (folderPath.find_first_of("*?[") != string::npos) ? MatchFolders(&inputFile, folderPath) : vector<string>{folderPath};
        }
        for (const auto& folderPath : folderPaths) {
          mIsFolderInput = !folderPath.empty();
          TObject* folder = &inputFile;

          // find top level entry point for this input file
          if (!folderPath.empty()) {
            auto filePath = split_string(folderPath, '/');
            folder = FindSubDirectory(folder, filePath);
            if (!folder) {
              // inputs that do not contain the folder of a restricted data source (dataSource:some/folder) are simply not part of it
              if (isRestricted) continue;
              ERROR("Subdirectory {} not found in file {}.", folderPath, fileName);
              continue;
            }
          }

          vector<string> emptySubDirs;
          for (auto& [pathStr, names] : required) {
            auto path = split_string(pathStr, '/');
            TObject* subfolder = FindSubDirectory(folder, path);
            if (subfolder) {
              // recursively traverse the file and look for input files
              string prefix = (pathStr.empty()) ? "" : pathStr + "/";
              string suffix = ":" + dataSource;
              ReadData(subfolder, names, prefix, suffix, dataSource);
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
            required.erase(pathStr);
          }
          // trees can also be in further inputs: keep looking for them
          for (const auto& [treeName, treeInputs] : mTreeInputs[dataSource]) {
            const auto pathPos = treeName.find_last_of('/');
            auto& names = required[(pathPos == string::npos) ? "" : treeName.substr(0, pathPos)];
            const string name = treeName.substr(pathPos + 1);
            names.insert(name);
          }
        }
      }
    }
    // what a chained tree is made of in words, e.g. "3 files" or "412 folders in 3 files" (empty if it has only one input)
    auto describeInputs = [](const vector<tree_input_t>& treeInputs) -> string {
      if (treeInputs.size() < 2) return "";
      auto plural = [](size_t n, const string& word) { return fmt::format("{} {}{}", n, word, (n == 1) ? "" : "s"); };
      set<string> wholeFiles;
      set<string> filesWithFolders;
      size_t nFolders{};
      for (const auto& treeInput : treeInputs) {
        if (treeInput.isFolderInput) {
          ++nFolders;
          filesWithFolders.insert(treeInput.file);
        } else {
          wholeFiles.insert(treeInput.file);
        }
      }
      vector<string> descriptions;
      if (!wholeFiles.empty()) descriptions.push_back(plural(wholeFiles.size(), "file"));
      if (nFolders) descriptions.push_back(plural(nFolders, "folder") + " in " + plural(filesWithFolders.size(), "file"));
      return (descriptions.size() == 1) ? descriptions[0] : descriptions[0] + " and " + descriptions[1];
    };
    // process the trees found: chained over all inputs of the data source that contain them
    for (const auto& [dataSource, treesInputs] : mTreeInputs) {
      auto& required = requiredData[dataSource];
      for (const auto& [treeName, treeInputs] : treesInputs) {
        const auto pathPos = treeName.find_last_of('/');
        const string pathStr = (pathPos == string::npos) ? "" : treeName.substr(0, pathPos);
        if (auto it = required.find(pathStr); it != required.end()) {
          auto& names = it->second;
          names.erase(treeName.substr(pathPos + 1));
          if (names.empty()) required.erase(it);
        }
        // requests that join the same trees share one chain
        using join_t = Plot::Pad::Data::data_info_t::join_t;
        vector<vector<join_t>> joinGroups;
        for (const auto& dataInfo : mDataInfoBuffer[dataSource][treeName]) {
          auto joins = dataInfo.joins.value_or(vector<join_t>{});
          if (std::find(joinGroups.begin(), joinGroups.end(), joins) == joinGroups.end()) joinGroups.push_back(std::move(joins));
        }
        set<string> failedJoins;  // reported already
        for (const auto& joins : joinGroups) {
          auto chain = std::make_shared<TChain>(treeInputs.front().treePath.data());
          for (const auto& treeInput : treeInputs) {
            chain->AddFile(treeInput.file.data(), TTree::kMaxEntries, treeInput.treePath.data());
          }
          vector<shared_ptr<TChain>> joinedChains;
          bool isJoined = true;
          for (const auto& join : joins) {
            if (failedJoins.count(join.ToString())) {
              isJoined = false;
            } else if (auto error = AttachJoin(*chain, treeName, treeInputs, join, joinedChains)) {
              ERROR("Cannot join {} to tree {}: {}.", join.GetDescription(), DataLocation(dataSource, treeName), *error);
              failedJoins.insert(join.ToString());
              isJoined = false;
            }
            if (!isJoined) break;
          }
          if (!isJoined) continue;
          ProcessDataRequests("tree", dataSource, treeName, ":" + dataSource, [chain, joinedChains]() { return std::make_unique<ROOT::RDataFrame>(*chain); /* joinedChains: keeps the joined trees alive */ }, describeInputs(treeInputs), joins);
        }
      }
    }
    mTreeInputs.clear();
    success &= allFound();
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
  if (onlyMissing) {
    DEBUG("================= Missing Data ================");
  } else {
    DEBUG("================= Data Buffer =================");
  }
  uint32_t nNeededData{};
  uint32_t nAvailableData{};
  for (const auto& [dataSource, buffer] : mDataBuffer) {
    bool printDataSource = true;
    for (const auto& [dataName, dataPtr] : buffer) {
      ++nNeededData;
      bool show = onlyMissing ? (dataPtr == nullptr) : true;
      if (dataPtr) ++nAvailableData;
      if (show) {
        if (printDataSource) DEBUG("{}{}", dataSource, (!IsDataSourceDefined(dataSource)) ? " (data source not defined)" : "");
        printDataSource = false;
        DEBUG(" - {}{}{}", (dataPtr) ? logger::begin_color(logger::Color::Green) : logger::begin_color(logger::Color::Red), dataName, logger::end_color());
      }
    }
  }
  DEBUG("Found {}/{} required input data.", nAvailableData, nNeededData);
  DEBUG("===============================================");
}

//**************************************************************************************************
/**
 * Determine which of a plot's required data entries are missing from the buffer.
 */
//**************************************************************************************************
vector<std::tuple<string, string, Plot::Pad::Data::data_info_t>> PlotManager::GetMissingData(Plot& plot)
{
  vector<std::tuple<string, string, Plot::Pad::Data::data_info_t>> missing;
  auto checkKey = [&](const string& dataSource, const string& name, const Plot::Pad::Data::data_info_t& dataInfo) {
    auto sourceIt = mDataBuffer.find(dataSource);
    if (sourceIt != mDataBuffer.end()) {
      auto nameIt = sourceIt->second.find(name + dataInfo.GetNameSuffix());
      if (nameIt != sourceIt->second.end() && nameIt->second) return;  // found and not null
    }
    missing.emplace_back(dataSource, name, dataInfo);
  };

  for (auto& [padID, pad] : plot.GetPads()) {
    if (auto& refFunc = pad.GetRefFunc()) {
      checkKey(refFunc->GetDataSource(), refFunc->GetName(), refFunc->GetDataInfo());
    } else if (plot.GetBasePlotName()) {
      auto it = std::find_if(mBasePlots.begin(), mBasePlots.end(), [&](const auto& basePlot) { return *plot.GetBasePlotName() == basePlot.GetName(); });
      if (it != mBasePlots.end()) {
        if (auto& baseRefFunc = (*it).GetPad(padID).GetRefFunc()) {
          checkKey(baseRefFunc->GetDataSource(), baseRefFunc->GetName(), baseRefFunc->GetDataInfo());
        }
      }
    }
    for (const auto& data : pad.GetData()) {
      checkKey(data->GetDataSource(), data->GetName(), data->GetDataInfo());
      if (data->GetType() == "ratio") {
        const auto& ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
        checkKey(ratio->GetDenomDataSource(), ratio->GetDenomName(), ratio->GetDenomDataInfo());
      }
    }
  }
  return missing;
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

  if (plot.GetGroup().empty()) {
    ERROR("No group was specified for plot {}.", plot.GetName());
    return false;
  }
  Plot fullPlot;
  if (plot.GetBasePlotName()) {
    const string& basePlotName = *plot.GetBasePlotName();
    auto iterator = std::find_if(
      mBasePlots.begin(), mBasePlots.end(),
      [&](Plot& basePlot) { return basePlot.GetName() == basePlotName; });
    if (iterator != mBasePlots.end()) {
      fullPlot = *iterator;
    } else {
      ERROR("Could not find base plot named {}.", basePlotName);
    }
  }
  fullPlot += plot;
  if (mode == "print") {
    INFO("Settings of plot {}{}{} from group {}{}{}:", logger::begin_color(logger::Color::Green), fullPlot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), fullPlot.GetGroup(), logger::end_color());
    Plot::Print(fullPlot.GetPropertyTree(), "");
    return true;
  }

  // the personal scale settings only change the number of pixels of plots on screen and of bitmap files, vector files do not depend on them
  const bool isBitmapMode = (mode == "png") || (mode == "jpg") || str_contains(mode, "gif");
  PlotPainter painter(isInteractiveMode ? Config::Get().ScreenScale() : (isBitmapMode ? Config::Get().BitmapScale() : 1.));
  gROOT->SetBatch(!isInteractiveMode && !isMacroMode);
  shared_ptr<TCanvas> canvas{painter.GeneratePlot(fullPlot, mDataBuffer)};
  if (!canvas) return false;

  if (TColor::GetFreeColorIndex() > std::numeric_limits<int16_t>::max()) {
    // there is a natural limit to the number of custom colors since ROOT color indices are of type short
    logger::throw_out_of_range("Too many custom colors in one session. Aborting...");
  }
  auto logCreated = [&]() {
    LOG("Created plot {}{}{} from group {}{}{}.", logger::begin_color(logger::Color::Green), fullPlot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), fullPlot.GetGroup(), logger::end_color());
  };

  // if interactive mode is specified, open window instead of saving the plot
  if (isInteractiveMode) {
    logCreated();
    if (auto rc = dynamic_cast<TRootCanvas*>(canvas->GetCanvasImp())) {
      rc->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
    }
    mPlotViewHistory.push_back(canvas);
    uint32_t curPlotIndex{static_cast<uint32_t>(mPlotViewHistory.size() - 1)};

    // move new canvas to position of previous window
    int32_t curXpos{};
    int32_t curYpos{};
    if (curPlotIndex > 0) {
      curXpos = mPlotViewHistory[curPlotIndex - 1]->GetWindowTopX();
      curYpos = mPlotViewHistory[curPlotIndex - 1]->GetWindowTopY();
      canvas->SetWindowPosition(curXpos, curYpos - mWindowOffsetY);
      static_cast<TRootCanvas*>(mPlotViewHistory[curPlotIndex - 1]->GetCanvasImp())->UnmapWindow();
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
          if (curPlotIndex == 0) {
            mExitInteractiveBrowsing = true;
            break;
          }
          --curPlotIndex;
        }
        static_cast<TRootCanvas*>(canvas->GetCanvasImp())->UnmapWindow();
        canvas = mPlotViewHistory[curPlotIndex];
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
    return false;
  }
  auto parentDir = (mOutputDirectory.back() == '/') ? std::filesystem::path(mOutputDirectory).parent_path().parent_path() : std::filesystem::path(mOutputDirectory).parent_path();
  if (!std::filesystem::exists(parentDir)) {
    ERROR("Parent path {} of output directory does not exist.", parentDir.string());
    return false;
  }

  if (mode == "file") {
    mCanvasRegistry[plot.GetUniqueName()] = canvas;
    logCreated();
    return true;
  }
  if (mode == "data") {
    logCreated();
    return true;
  }

  bool isGif = false;
  string gifRepRate = "+50";  // number of centiseconds between frames

  // names that become C++ identifiers in macros must not contain special characters
  auto sanitize = [](string s) {
    for (auto& c : s) {
      if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') c = '_';
    }
    if (!s.empty() && std::isdigit(static_cast<unsigned char>(s.front()))) s = "_" + s;
    return s;
  };

  string fileEnding;
  if (isMacroMode) {
    fileEnding = ".C";
    // ROOT converts the names of all drawn objects to valid variable names, but not the one of the canvas itself
    canvas->SetName(sanitize(canvas->GetName()).data());
    // objects drawn twice in a pad (e.g. the axis histogram) are declared twice in the macro for 2D histograms, which does not compile
    std::function<void(TPad*)> cloneRedrawnHistograms;
    cloneRedrawnHistograms = [&](TPad* pad) {
      std::set<TObject*> drawnObjects;
      int32_t nClones{};
      for (auto link = pad->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
        TObject* object = link->GetObject();
        if (auto subPad = dynamic_cast<TPad*>(object)) {
          cloneRedrawnHistograms(subPad);
        } else if (!drawnObjects.insert(object).second && object->InheritsFrom(TH1::Class())) {
          auto clone = static_cast<TH1*>(object->Clone((string(object->GetName()) + "_" + std::to_string(++nClones)).data()));
          clone->SetDirectory(nullptr);
          clone->SetBit(kCanDelete);
          link->SetObject(clone);
        }
      }
    };
    cloneRedrawnHistograms(canvas.get());
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
    return false;
  }

  string fileName = plot.GetName();
  std::replace(fileName.begin(), fileName.end(), ':', '_');
  std::replace(fileName.begin(), fileName.end(), '/', '_');
  // ROOT names the macro function after the file, so it must be a valid identifier as well
  if (isMacroMode) fileName = sanitize(fileName);

  // create output folders and files
  string folderName = mOutputDirectory + "/" + plot.GetGroup();
  string fullName = folderName + "/" + fileName + fileEnding;

  if (isGif) {
    if (mGifName.empty()) {
      gSystem->Unlink(fullName.data());
      mGifName = fullName;
    } else {
      fullName = mGifName;
      folderName = std::filesystem::path(mGifName).parent_path().string();
    }
    fullName += gifRepRate;
  }
  std::error_code ec;
  std::filesystem::create_directories(folderName, ec);
  if (ec) {
    ERROR("Could not create output directory {}: {}.", folderName, ec.message());
    return false;
  }
  float_t previousLineScalePS = gStyle->GetLineScalePS();
  auto lineScaleGuard = make_scope_guard([previousLineScalePS]() { gStyle->SetLineScalePS(previousLineScalePS); });
  if ((mode == "pdf") || (mode == "eps") || (mode == "ps")) {
    float_t paperWidthCm{};
    float_t paperHeightCm{};
    gStyle->GetPaperSize(paperWidthCm, paperHeightCm);
    double_t canvasWidthPx = canvas->GetWw();
    double_t canvasHeightPx = canvas->GetWh();
    double_t aspectRatio = canvasHeightPx / canvasWidthPx;
    double_t pageWidthCm = (aspectRatio <= paperHeightCm / paperWidthCm) ? paperWidthCm : paperHeightCm / aspectRatio;
    double_t pageWidthPt = pageWidthCm / 2.54 * 72.;
    gStyle->SetLineScalePS(4. * pageWidthPt / canvasWidthPx);
  }
  canvas->SaveAs(fullName.data());
  if (!isGif && !std::filesystem::is_regular_file(fullName)) {
    ERROR("Could not write {}.", fullName);
    return false;
  }
  logCreated();
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
 * Clear the data loaded for plots of previous GeneratePlots() calls.
 */
//**************************************************************************************************
void PlotManager::ClearDataBuffer()
{
  mDataBuffer.clear();
  mDataInfoBuffer.clear();
}

//**************************************************************************************************
/**
 * Clear the canvas registry of already produced canvae that would all be saved in "file" mode.
 */
//**************************************************************************************************
void PlotManager::ClearCanvasRegistry()
{
  mCanvasRegistry.clear();
  mPlotViewHistory.clear();
}

//**************************************************************************************************
/**
 * Recursively reads data from folder / list and adds it to output data array. Found dataNames are removed from the set.
 */
//**************************************************************************************************
void PlotManager::ReadData(TObject* folder, set<string>& dataNames, const string& prefix, const string& suffix, const string& dataSource)
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
        TKey* key = static_cast<TKey*>(obj);
        string className = key->GetClassName();
        curDataName = key->GetName();

        bool isTraversable = str_contains(className, "TDirectory") || str_contains(className, "TFolder") || str_contains(className, "TList") || str_contains(className, "THashList") || str_contains(className, "TObjArray");
        const bool isRequested = dataNames.count(curDataName) > 0;
        if ((traverse && isTraversable) || isRequested) {
          TClass* keyClass = TClass::GetClass(className.data());
          if (!keyClass || !keyClass->IsTObject()) {
            if (isRequested) WARNING("Skipping {} (data source {}): unknown or unsupported class {}.", curDataName, dataSource, className);
            ++iterator;
            continue;
          }
          obj = key->ReadObj();
          if (!obj) {
            if (isRequested) WARNING("Could not read {} of class {} (data source {}); the file may be corrupted.", curDataName, className, dataSource);
            ++iterator;
            continue;
          }
          removeFromList = false;
        } else {
          ++iterator;
          continue;
        }
      }

      // in case this object is directory or list, repeat the same for this substructure
      if (obj->InheritsFrom(TDirectory::Class()) || obj->InheritsFrom(TFolder::Class()) || obj->InheritsFrom(TCollection::Class())) {
        if (traverse) {
          ReadData(obj, dataNames, prefix, suffix, dataSource);
        } else if (removeFromList) {
          removeFromList = false;
          deleteObject = false;
        }
      } else {
        // the key name supersedes the actual data name (in case they are different when written to file via h->Write("myKeyName"))
        if (curDataName.empty()) curDataName = obj->GetName();
        if (auto it = dataNames.find(curDataName); it != dataNames.end()) {
          // demand ownership for object if required for given type
          if (obj->InheritsFrom(TH1::Class())) static_cast<TH1*>(obj)->SetDirectory(0);
          if (obj->InheritsFrom(TGraph2D::Class())) static_cast<TGraph2D*>(obj)->SetDirectory(0);
          itemList->Remove(obj);
          dataNames.erase(it);
          string fullName = prefix + curDataName;
          if (obj->InheritsFrom(TTree::Class())) {
            TTree* tree = static_cast<TTree*>(obj);
            mDataBuffer[dataSource][fullName].reset(nullptr);
            if (TDirectory* dir = tree->GetDirectory(); dir && dir->GetFile() && dir->GetKey(curDataName.data())) {
              // trees stored in files are chained over all files of the data source (processed once all files are searched)
              string dirPath = dir->GetPath();  // file.root:/some/folder
              dirPath.erase(0, dirPath.find(":/") + 2);
              mTreeInputs[dataSource][fullName].push_back({dir->GetFile()->GetName(), (dirPath.empty() ? "" : dirPath + "/") + curDataName, mIsFolderInput});
            } else if (mTreeInputs[dataSource].find(fullName) != mTreeInputs[dataSource].end()) {
              WARNING("Ignoring tree {} (data source {}) in a list: a tree stored in a list cannot be chained with the same tree in other inputs.", fullName, dataSource);
            } else {
              // trees in lists live in memory only, which RDataFrame can process only single-threaded
              const bool wasMTEnabled = ROOT::IsImplicitMTEnabled();
              const UInt_t nThreads = ROOT::GetThreadPoolSize();
              ROOT::DisableImplicitMT();
              auto mtGuard = make_scope_guard([wasMTEnabled, nThreads]() { if (wasMTEnabled) ROOT::EnableImplicitMT(nThreads); });
              ProcessDataRequests("tree", dataSource, fullName, suffix, [tree]() { return std::make_unique<ROOT::RDataFrame>(*tree); });
            }
            tree->SetDirectory(0);
            delete tree;
          } else if (auto* namedObj = dynamic_cast<TNamed*>(obj)) {
            namedObj->SetName((fullName + suffix).data());
            mDataBuffer[dataSource][fullName].reset(obj);
          } else {
            ERROR("Input data {} (data source {}) is of unsupported type {}.", fullName, dataSource, obj->ClassName());
            delete obj;
          }
          removeFromList = false;
          deleteObject = false;
        }
      }

      // increase iterator before removing objects from collection
      ++iterator;
      if (removeFromList) {
        if (!itemList->Remove(obj)) {
          WARNING("Could not remove item {} ({}) from collection {}.", obj->GetName(), static_cast<void*>(obj), itemList->GetName());
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
 * Read table data from file.
 * The delimiter is the candidate that splits all lines (incl. header) into the same number of fields.
 * Lines starting with '#' are treated as comments if the first lines contain any.
 * Files with a varying number of fields are refused, since ROOT's csv reader cannot handle them.
 */
//**************************************************************************************************
void PlotManager::ReadTableData(const string& inputFileName, const string& name, const string& dataSource)
{
  if (!file_exists(inputFileName)) {
    ERROR("File {} does not exist.", inputFileName);
    return;
  }
  auto trim = [](const string& line) {
    const auto begin = line.find_first_not_of(" \t\r");
    if (begin == string::npos) return string{};
    return line.substr(begin, line.find_last_not_of(" \t\r") - begin + 1);
  };
  // split a line into fields (delimiters within double quotes do not count)
  auto split = [](const string& line, char delimiter) {
    vector<string> fields(1);
    bool quoted = false;
    for (char c : line) {
      if (c == '"') quoted = !quoted;
      if (c == delimiter && !quoted) {
        fields.emplace_back();
      } else {
        fields.back() += c;
      }
    }
    return fields;
  };

  // first data lines (incl. header) with their line numbers, skipping blank lines and comments
  std::ifstream file(inputFileName);
  vector<std::pair<size_t, string>> lines;
  string line;
  size_t lineNumber = 0;
  bool hasComments = false;
  while (lines.size() < 100 && std::getline(file, line)) {
    ++lineNumber;
    line = trim(line);
    if (line.empty()) continue;
    if (line[0] == '#') {
      hasComments = true;
      continue;
    }
    lines.emplace_back(lineNumber, line);
  }
  if (lines.empty()) {
    ERROR("Table {} does not contain any data.", inputFileName);
    return;
  }

  // the delimiter must give the same number of fields in all lines, the one giving most fields wins
  char delimiter = ',';
  size_t nFields = 1;
  for (char candidate : {',', ';', '\t', '|', ' '}) {
    const size_t nHeaderFields = split(lines[0].second, candidate).size();
    if (nHeaderFields <= nFields) continue;
    bool consistent = std::all_of(lines.begin(), lines.end(), [&](const auto& entry) { return split(entry.second, candidate).size() == nHeaderFields; });
    if (consistent) {
      delimiter = candidate;
      nFields = nHeaderFields;
    }
  }
  auto delimiterName = [](char c) -> string {
    if (c == '\t') return "tab";
    if (c == ' ') return "space";
    return string("'") + c + "'";
  };
  auto describeMismatch = [&](char c, size_t nExpected, size_t badLine, size_t nFound) {
    return fmt::format("the header has {} fields separated by {}, but line {} has {}", nExpected, delimiterName(c), badLine, nFound);
  };
  if (nFields == 1) {
    // no consistent delimiter: report what goes wrong with the delimiter that is most frequent in the header
    char guess = ',';
    size_t maxCount = 0;
    for (char candidate : {',', ';', '\t', '|', ' '}) {
      size_t count = split(lines[0].second, candidate).size() - 1;
      if (count > maxCount) {
        maxCount = count;
        guess = candidate;
      }
    }
    if (maxCount) {
      for (const auto& [number, content] : lines) {
        const size_t n = split(content, guess).size();
        if (n == maxCount + 1) continue;
        string hint;
        if (guess == ' ' || guess == '\t') {
          std::istringstream words(content);
          if (std::distance(std::istream_iterator<string>(words), std::istream_iterator<string>()) == static_cast<std::ptrdiff_t>(maxCount + 1)) {
            hint = " (columns seem to be aligned with several spaces or tabs; use exactly one delimiter between the values)";
          }
        }
        ERROR("Cannot read table {}: {}{}.", inputFileName, describeMismatch(guess, maxCount + 1, number, n), hint);
        return;
      }
    }
  }

  // check the remaining lines as well, ROOT's csv reader does not tolerate a varying number of fields
  auto checkLine = [&](size_t number, const string& content) {
    const size_t n = split(content, delimiter).size();
    if (n == nFields) return true;
    ERROR("Cannot read table {}: {}.", inputFileName, describeMismatch(delimiter, nFields, number, n));
    return false;
  };
  while (std::getline(file, line)) {
    ++lineNumber;
    line = trim(line);
    if (line.empty()) continue;
    if (line[0] == '#' && hasComments) continue;
    if (!checkLine(lineNumber, line)) return;
  }

  // ROOT does not trim the fields: names and values with surrounding spaces end up as such (values as text)
  if (delimiter != ' ' && delimiter != '\t') {
    for (const auto& [number, content] : {lines[0], lines[std::min<size_t>(1, lines.size() - 1)]}) {
      const auto fields = split(content, delimiter);
      if (std::any_of(fields.begin(), fields.end(),
                      [&](const string& field) { return field != trim(field); })) {
        WARNING("Table {} has spaces around its delimiters (line {}), which become part of the column names and values (read as text); remove them.", inputFileName, number);
        break;
      }
    }
  }

  // numbers with decimal commas cannot be read by ROOT (they end up as text)
  if (delimiter != ',' && lines.size() > 1) {
    static const std::regex decimalComma(R"(\s*[+-]?\d+,\d+\s*)");
    for (const auto& field : split(lines[1].second, delimiter)) {
      if (std::regex_match(field, decimalComma)) {
        WARNING("Table {} seems to use decimal commas (e.g. '{}'), which are not read as numbers; use decimal points instead.", inputFileName, trim(field));
        break;
      }
    }
  }

  ROOT::RDF::RCsvDS::ROptions options;
  // ROOT reads numbers in exponent notation without decimal point (e.g. 1e-3) as text: declare such columns as numeric
  if (lines.size() > 1) {
    static const std::regex number(R"([+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?)");
    static const std::regex exponent(R"([+-]?\d+[eE][+-]?\d+)");
    const auto columnNames = split(lines[0].second, delimiter);
    for (size_t column = 0; column < columnNames.size(); ++column) {
      bool allNumbers = true;
      bool hasExponent = false;
      for (size_t i = 1; i < lines.size() && allNumbers; ++i) {
        const string value = trim(split(lines[i].second, delimiter)[column]);
        allNumbers = std::regex_match(value, number);
        hasExponent = hasExponent || std::regex_match(value, exponent);
      }
      string columnName = trim(columnNames[column]);
      if (columnName.size() > 1 && columnName.front() == '"' && columnName.back() == '"') columnName = columnName.substr(1, columnName.size() - 2);
      if (allNumbers && hasExponent && !columnName.empty()) options.fColumnTypes[columnName] = 'D';
    }
  }
  options.fDelimiter = delimiter;
  options.fLeftTrim = true;  // as for the checks above: only the line as a whole is trimmed
  options.fRightTrim = true;
  if (hasComments) options.fComment = '#';
  options.fLinesChunkSize = 50000;
  ProcessDataRequests("table", dataSource, name, ":" + dataSource, [&]() { return std::make_unique<ROOT::RDataFrame>(ROOT::RDF::FromCSV(inputFileName, options)); });
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
  } else if (folder->InheritsFrom(TFolder::Class())) {
    subFolder = static_cast<TFolder*>(folder)->FindObject(subDirs[0].data());
    if (subFolder) {
      static_cast<TFolder*>(subFolder)->SetOwner();
      // if subfolder is part of list, remove it first to avoid double deletion
      static_cast<TFolder*>(folder)->Remove(subFolder);
    }
  } else if (folder->InheritsFrom(TCollection::Class())) {
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
 * Find a tree in the inputs of a data source (the same way as when reading the data source).
 */
//**************************************************************************************************
vector<PlotManager::tree_input_t> PlotManager::FindTreeInputs(const string& dataSource, const string& treeName)
{
  vector<tree_input_t> treeInputs;
  const auto pathPos = treeName.find_last_of('/');
  const string path = (pathPos == string::npos) ? "" : treeName.substr(0, pathPos);
  const string name = treeName.substr(pathPos + 1);
  // first match should be the one in the current folder; traverse deeper only if not found
  std::function<optional<string>(TDirectory*)> find;
  find = [&](TDirectory* dir) -> optional<string> {
    auto isA = [](TKey* key, TClass* cl) {
      auto* keyClass = TClass::GetClass(key->GetClassName());
      return keyClass && keyClass->InheritsFrom(cl);
    };
    if (auto* key = dir->GetKey(name.data()); key && isA(key, TTree::Class())) {
      string dirPath = dir->GetPath();  // file.root:/some/folder
      dirPath.erase(0, dirPath.find(":/") + 2);
      return (dirPath.empty() ? "" : dirPath + "/") + name;
    }
    set<string> visited;  // keys can appear in several cycles
    for (auto* key : TRangeDynCast<TKey>(dir->GetListOfKeys())) {
      if (!key || !visited.insert(key->GetName()).second || !isA(key, TDirectory::Class())) continue;
      if (auto* subDir = dir->GetDirectory(key->GetName())) {
        if (auto treePath = find(subDir)) return treePath;
      }
    }
    return std::nullopt;
  };
  auto expandedIt = mExpandedInputs.find(dataSource);
  for (const auto& inputRaw : (expandedIt != mExpandedInputs.end()) ? expandedIt->second : ExpandInputs(dataSource)) {
    const auto fileNamePath = split_string(expand_path(inputRaw), ':', true);
    const string& fileName = fileNamePath[0];
    if (!str_ends_with(fileName, ".root") || !std::filesystem::exists(fileName)) continue;
    TFile file(fileName.data(), "READ");
    if (file.IsZombie()) continue;
    TDirectory* dir = &file;
    if (fileNamePath.size() > 1) dir = dir->GetDirectory(fileNamePath[1].data());
    if (dir && !path.empty()) dir = dir->GetDirectory(path.data());
    if (!dir) continue;
    if (auto treePath = find(dir)) treeInputs.push_back({fileName, *treePath, fileNamePath.size() > 1});
  }
  return treeInputs;
}

//**************************************************************************************************
/**
 * Join a tree to a chain: its rows are matched with the rows of the chain either by their number or by the values of key columns.
 * Returns why this is not possible (if so).
 */
//**************************************************************************************************
optional<string> PlotManager::AttachJoin(TChain& chain, const string& treeName, const vector<tree_input_t>& treeInputs,
                                         const Plot::Pad::Data::data_info_t::join_t& join, vector<shared_ptr<TChain>>& joinedChains)
{
  if (const string treeAlias = treeName.substr(treeName.find_last_of('/') + 1); join.GetAlias() == treeAlias) {
    return fmt::format("the columns of both would be called {}.column, please specify an alias", treeAlias);
  }
  std::map<string, std::unique_ptr<TFile>> openFiles;  // each file is opened only once for the following checks
  auto getTree = [&](const string& fileName, const string& treePath) -> TTree* {
    auto& file = openFiles[fileName];
    if (!file) file.reset(TFile::Open(fileName.data(), "READ"));
    if (!file || file->IsZombie()) return nullptr;
    return file->Get<TTree>(treePath.data());
  };
  TTree* firstTree = getTree(treeInputs.front().file, treeInputs.front().treePath);
  // a branch of the tree with sub-columns (leaflist, split object) named like the alias would make alias.column ambiguous
  if (auto* branch = (firstTree) ? firstTree->GetBranch(join.GetAlias().data()) : nullptr; branch && (branch->GetNleaves() > 1 || branch->GetListOfBranches()->GetEntries() > 0)) {
    return fmt::format("{} has a branch {}, please specify a different alias", treeName, join.GetAlias());
  }
  auto joinedChain = std::make_shared<TChain>(join.GetAlias().data());
  const TTree* firstJoinedTree{};
  if (join.dataSource.empty()) {
    // taken from each input of the tree: the name of the joined tree starts where the name of the tree starts
    const auto nLevels = std::count(treeName.begin(), treeName.end(), '/') + 1;
    for (const auto& treeInput : treeInputs) {
      string basePath = treeInput.treePath;
      for (int64_t level = 0; level < nLevels; ++level) {
        const auto pos = basePath.find_last_of('/');
        basePath = (pos == string::npos) ? "" : basePath.substr(0, pos);
      }
      const string path = (basePath.empty()) ? join.tree : basePath + "/" + join.tree;
      auto* joinedTree = getTree(treeInput.file, path);
      if (!joinedTree) return fmt::format("{}:{} does not exist", treeInput.file, path);
      if (!firstJoinedTree) firstJoinedTree = joinedTree;
      // without keys the rows of the trees in each input belong together
      if (join.keys.empty()) {
        auto* tree = getTree(treeInput.file, treeInput.treePath);
        const Long64_t nRows = (tree) ? tree->GetEntries() : 0;
        if (joinedTree->GetEntries() != nRows) return fmt::format("{}:{} has {} rows, {} has {}", treeInput.file, path, joinedTree->GetEntries(), treeInput.treePath, nRows);
      }
      joinedChain->AddFile(treeInput.file.data(), TTree::kMaxEntries, path.data());
    }
  } else {
    if (!IsDataSourceDefined(join.dataSource)) return fmt::format("data source {} is not defined", join.dataSource);
    const auto joinedInputs = FindTreeInputs(join.dataSource, join.tree);
    if (joinedInputs.empty()) return fmt::format("it was not found in data source {}", join.dataSource);
    for (const auto& joinedInput : joinedInputs) {
      joinedChain->AddFile(joinedInput.file.data(), TTree::kMaxEntries, joinedInput.treePath.data());
    }
    firstJoinedTree = getTree(joinedInputs.front().file, joinedInputs.front().treePath);
    // without keys the rows of all inputs are matched in the order of the inputs
    if (join.keys.empty() && joinedChain->GetEntries() != chain.GetEntries()) {
      return fmt::format("it has {} rows, {} has {}", joinedChain->GetEntries(), treeName, chain.GetEntries());
    }
  }
  if (!join.keys.empty()) {
    for (const auto& key : join.keys) {
      for (const auto* tree : {static_cast<const TTree*>(firstTree), firstJoinedTree}) {
        // keys are top-level columns with a single value, the join filter addresses the first one as alias.key
        auto* branch = (tree) ? const_cast<TTree*>(tree)->GetBranch(key.data()) : nullptr;
        auto* leaf = (branch && branch->GetNleaves() == 1) ? static_cast<TLeaf*>(branch->GetListOfLeaves()->At(0)) : nullptr;
        const string treeDescription = (tree == firstTree) ? treeName : join.tree;
        if (!leaf) return fmt::format("{} has no column {}", treeDescription, key);
        // the values are compared as integers
        const string type = leaf->GetTypeName();
        if (str_contains(type, "Float") || str_contains(type, "Double") || str_contains(type, "float") || str_contains(type, "double")) {
          return fmt::format("key column {} of {} is of type {}, but keys must be integers", key, treeDescription, type);
        }
      }
    }
    auto index = [&]() {
      auto errorLevelGuard = make_scope_guard([level = gErrorIgnoreLevel]() { gErrorIgnoreLevel = level; });
      gErrorIgnoreLevel = kFatal;  // duplicates and keys that are too large are reported below
      return std::make_unique<TTreeIndex>(joinedChain.get(), join.keys[0].data(), (join.keys.size() > 1) ? join.keys[1].data() : "0");
    }();
    if (index->IsZombie()) return "cannot build an index of its keys";
    const Long64_t* major = index->GetIndexValues();
    const Long64_t* minor = index->GetIndexValuesMinor();
    // ROOT looks up the keys of the tree as double, which holds integers exactly only below 2^53
    constexpr Long64_t maxKey = Long64_t{1} << 53;
    auto isTooLarge = [](Long64_t value) { return value >= maxKey || value <= -maxKey; };
    for (Long64_t i = 0; i < index->GetN(); ++i) {
      for (size_t k = 0; k < join.keys.size(); ++k) {
        const Long64_t value = (k == 0) ? major[i] : minor[i];
        if (isTooLarge(value)) return fmt::format("{} = {} cannot be matched exactly, keys must be below 2^53", join.keys[k], value);
      }
      // each key may appear only once, otherwise it is undefined which of the rows is matched
      if (i > 0 && major[i] == major[i - 1] && minor[i] == minor[i - 1]) {
        return (join.keys.size() > 1) ? fmt::format("{} = {} and {} = {} appears in more than one row", join.keys[0], major[i], join.keys[1], minor[i]) : fmt::format("{} = {} appears in more than one row", join.keys[0], major[i]);
      }
    }
    joinedChain->SetTreeIndex(index.release());  // owned by the chain
  }
  chain.AddFriend(joinedChain.get(), join.GetAlias().data());
  joinedChains.push_back(joinedChain);
  return std::nullopt;
}

//**************************************************************************************************
/**
 * Process all requests (projections, profiles, scatter plots) of one tree or table.
 * The requests are booked together, so the data is read in one go (plus one pass for requests that auto-detect their axis ranges).
 * Requests that must be processed sequentially (scatter plots, entry ranges) share a separate single-threaded pass.
 */
//**************************************************************************************************
void PlotManager::ProcessDataRequests(const string& type, const string& dataSource, const string& name, const string& objNameSuffix,
                                      const std::function<std::unique_ptr<ROOT::RDataFrame>()>& makeDataFrame, const string& inputsDescription,
                                      const optional<vector<Plot::Pad::Data::data_info_t::join_t>>& joins)
{
  using data_info_t = Plot::Pad::Data::data_info_t;
  auto start = std::chrono::steady_clock::now();

  // extract the reason for a failed data query: the first compiler error printed by ROOT (if any) or the exception message
  auto getErrorReason = [](const std::exception& exception, const string& rootOutput) {
    std::istringstream lines(rootOutput);
    for (string line; std::getline(lines, line);) {
      if (auto pos = line.find("error: "); pos != string::npos) {
        string reason = line.substr(pos + 7);
        if (str_ends_with(reason, ".")) reason.pop_back();
        return reason;
      }
    }
    string reason = exception.what();
    reason.erase(0, reason.find_first_not_of(" \n"));
    reason = reason.substr(0, reason.find('\n'));
    if (str_ends_with(reason, ".")) reason.pop_back();
    return reason;
  };

  vector<const data_info_t*> parallelInfos;
  vector<const data_info_t*> sequentialInfos;
  for (auto& dataInfo : mDataInfoBuffer[dataSource][name]) {
    // requests with other joins are processed on another data frame
    if (joins && dataInfo.joins.value_or(vector<Plot::Pad::Data::data_info_t::join_t>{}) != *joins) continue;
    if (dataInfo.dataDims.empty()) {
      ERROR("Cannot plot {} {} directly, specify what to extract from it (Project, Profile or Scatter).", type, DataLocation(dataSource, name));
      continue;
    }
    if (dataInfo.joins && !joins) {
      ERROR("Cannot join trees to {} {} ({}): only trees stored in files can be joined.", type, DataLocation(dataSource, name), dataInfo.GetDescription());
      continue;
    }
    (dataInfo.singleProc() ? sequentialInfos : parallelInfos).push_back(&dataInfo);
  }
  if (parallelInfos.empty() && sequentialInfos.empty()) return;

  // errors that only show up while reading may come from inputs of a chained tree with a different structure
  const string chainHint = (inputsDescription.empty()) ? "" : fmt::format(" (the tree is chained from {}: do all inputs of the data source have the same columns?)", inputsDescription);
  optional<ULong64_t> nEntries;
  optional<ULong64_t> nUnmatched;  // rows skipped since a tree joined by key has no matching row
  uint32_t nPasses{};
  uint32_t nRequests{};
  vector<std::pair<const data_info_t*, std::pair<optional<ULong64_t>, ULong64_t>>> entryCounts;  // (entries before filter, after filter)

  // captures what ROOT writes to std::cerr
  class OutputCapture : public std::streambuf
  {
   public:
    string Text()
    {
      std::lock_guard<std::mutex> lock(mMutex);
      return mText;
    }

   protected:
    int_type overflow(int_type ch) override
    {
      if (!traits_type::eq_int_type(ch, traits_type::eof())) {
        std::lock_guard<std::mutex> lock(mMutex);
        mText += traits_type::to_char_type(ch);
      }
      return traits_type::not_eof(ch);
    }
    std::streamsize xsputn(const char_type* text, std::streamsize count) override
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mText.append(text, static_cast<size_t>(count));
      return count;
    }

   private:
    std::mutex mMutex;
    string mText;
  };

  // process a group of requests on a common data frame
  std::function<void(const vector<const data_info_t*>&, bool)> processGroup;
  processGroup = [&](const vector<const data_info_t*>& infos, bool sequential) {
    OutputCapture rootOutput;
    vector<DataFrameRequest<data_info_t>> requests;  // successfully booked requests
    try {
      auto* cerrBuffer = std::cerr.rdbuf(&rootOutput);  // capture ROOT output, only shown if something goes wrong
      auto cerrGuard = make_scope_guard([cerrBuffer]() { std::cerr.rdbuf(cerrBuffer); });
      const bool wasMTEnabled = ROOT::IsImplicitMTEnabled();
      const UInt_t nThreads = ROOT::GetThreadPoolSize();
      if (sequential) ROOT::DisableImplicitMT();
      auto mtGuard = make_scope_guard([sequential, wasMTEnabled, nThreads]() { if (sequential && wasMTEnabled) ROOT::EnableImplicitMT(nThreads); });
      auto df = makeDataFrame();
      auto nEntriesTotal = df->Count();
      auto passGuard = make_scope_guard([&]() { nPasses += df->GetNRuns(); });

      // rows without a matching row in a tree joined by key are skipped (its columns have no values for them)
      vector<string> joinKeyColumns;
      if (joins) {
        for (const auto& join : *joins) {
          if (!join.keys.empty()) joinKeyColumns.push_back(join.GetAlias() + "." + join.keys[0]);
        }
      }
      optional<ROOT::RDF::RResultPtr<ULong64_t>> nMatched;
      if (!joinKeyColumns.empty()) {
        ROOT::RDF::RNode matched = *df;
        for (const auto& column : joinKeyColumns) {
          matched = matched.FilterAvailable(column);
        }
        nMatched = matched.Count();
      }

      for (auto info : infos) {
        DataFrameRequest<data_info_t> request(info, fmt::format("{} {}", type, DataLocation(dataSource, name)), name + info->GetNameSuffix() + objNameSuffix);
        auto outputStart = rootOutput.Text().size();  // only consider ROOT output caused by this request
        try {
          if (request.Prepare(*df, joinKeyColumns)) requests.push_back(std::move(request));
        } catch (const std::invalid_argument&) {
          throw;
        } catch (const std::exception& e) {
          ERROR("Invalid query for {} {} ({}): {}.", type, DataLocation(dataSource, name), info->GetDescription(), getErrorReason(e, rootOutput.Text().substr(outputStart)));
        }
      }
      // a pass to determine the axis ranges is only required if some request wants them to be auto-detected
      for (auto& request : requests) {
        if (request.NeedsAutoRange()) {
          *nEntriesTotal;  // starts the event loop for everything booked so far
          break;
        }
      }
      vector<ROOT::RDF::RResultHandle> handles;
      for (auto& request : requests) {
        if (request.Book()) {
          for (auto& handle : request.GetHandles())
            handles.push_back(handle);
        }
      }
      if (handles.empty()) return;
      handles.emplace_back(nEntriesTotal);
      vector<ROOT::RDF::RResultHandle> pendingHandles;
      for (auto& handle : handles) {
        if (!handle.IsReady()) pendingHandles.push_back(handle);
      }
      if (!pendingHandles.empty()) ROOT::RDF::RunGraphs(pendingHandles);

      nEntries = *nEntriesTotal;
      if (nMatched && !nUnmatched) nUnmatched = *nEntriesTotal - **nMatched;
      for (auto& request : requests) {
        if (!request.IsBooked()) continue;
        mDataBuffer[dataSource][name + request.GetInfo()->GetNameSuffix()].reset(request.GetResult());
        entryCounts.push_back({request.GetInfo(), {request.GetEntriesPreFilter(), request.GetEntriesPostFilter()}});
        ++nRequests;
      }
    } catch (const std::invalid_argument&) {
      // thrown by the csv reader if a value does not match the column type deduced from the first lines
      ERROR("Cannot read {} {}: it contains a value that does not match the type of its column.", type, DataLocation(dataSource, name));
    } catch (const std::exception& e) {
      if (requests.size() > 1) {
        // an invalid expression only shows up when the event loop starts and then spoils all requests of this data frame: process them one by one
        for (auto& request : requests) {
          processGroup({request.GetInfo()}, sequential);
        }
      } else if (requests.size() == 1) {
        ERROR("Invalid query for {} {} ({}): {}{}.", type, DataLocation(dataSource, name), requests[0].GetInfo()->GetDescription(), getErrorReason(e, rootOutput.Text()), chainHint);
      } else {
        ERROR("Cannot read {} {}: {}{}.", type, DataLocation(dataSource, name), getErrorReason(e, rootOutput.Text()), chainHint);
      }
    }
  };
  if (!parallelInfos.empty()) processGroup(parallelInfos, false);
  if (!sequentialInfos.empty()) processGroup(sequentialInfos, true);
  if (!nRequests) return;

  double_t seconds = std::chrono::duration<double_t>(std::chrono::steady_clock::now() - start).count();
  string joinsDescription;
  if (joins) {
    for (const auto& join : *joins) {
      joinsDescription += ((joinsDescription.empty()) ? ", joined " : ", ") + join.GetDescription();
    }
  }
  if (nUnmatched && *nUnmatched) joinsDescription += fmt::format(", {} row{} without a match skipped", *nUnmatched, (*nUnmatched == 1) ? "" : "s");
  string message = fmt::format(" - {} {} entries{}{}, {} request{}, {} pass{}, {:.1f} s", DataLocation(dataSource, name), nEntries.value_or(0), (inputsDescription.empty()) ? "" : " from " + inputsDescription, joinsDescription, nRequests, (nRequests == 1) ? "" : "s", nPasses, (nPasses == 1) ? "" : "es", seconds);
  /*
  for (const auto& [info, counts] : entryCounts) {
    const auto& [nPreFilter, nPostFilter] = counts;
    message += fmt::format("\n           · {}: {} entries", info->GetDescription(), nPostFilter);
    if (nPreFilter && *nPreFilter) message += fmt::format(" ({:.1f}%)", 100. * nPostFilter / *nPreFilter);
  }
  */
  INFO("{}", message);
  for (const auto& [info, counts] : entryCounts) {
    if (!counts.second) WARNING("No entries selected in {} {} ({}).", type, DataLocation(dataSource, name), info->GetDescription());
  }
}

//**************************************************************************************************
/**
 * Get a project variable (set via 'srp set <project> <property> <value>').
 */
//**************************************************************************************************
string PlotManager::GetProjectProperty(const string& property) const
{
  if (mProjectName.empty()) {
    ERROR("Pass name of project to plot manager to read its property {}.", property);
    return "";
  }
  return Config::Get().Property(mProjectName, property);
}

//****************************************************************************************
/**
 * Saves all required plot definitions and input file locations of the project.
 */
//****************************************************************************************
void PlotManager::SaveProject() const
{
  if (mProjectName.empty()) {
    ERROR("Pass name of project to plot manager to save the project.");
    return;
  }
  namespace fs = std::filesystem;

  if (fs::create_directories(Config::Get().ProjectPath(mProjectName))) {
    INFO("Created config folder for project {}: {}.", mProjectName, Config::Get().ProjectPath(mProjectName).string());
  }

  SaveDataSources();
  SavePlots();

  if (!mOutputDirectory.empty() && mOutputDirectory != Config::Get().OutputDir(mProjectName)) {
    Config::GetMutable().SetOutputDir(mProjectName, mOutputDirectory);
    Config::GetMutable().Save();
  }

  // create a csv file for tab completion
  std::ofstream tabCompFile;
  tabCompFile.open(Config::Get().ProjectPath(mProjectName) / "tabcomp.csv");
  for (const auto& plot : mPlots) {
    string line = plot.GetName() + "," + plot.GetGroup() + "\n";
    tabCompFile << line;
  }
  tabCompFile.close();
}
}  // end namespace SciRooPlot
