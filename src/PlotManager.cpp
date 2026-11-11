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

#include "SciRooPlot/Logger.h"
#include "SciRooPlot/PanelLayout.h"

#include <ROOT/RCsvDS.hxx>
#include <ROOT/RDFHelpers.hxx>
#include <ROOT/RDataFrame.hxx>
#include <RVersion.h>
#include <TApplication.h>
#include <TArrow.h>
#include <TBox.h>
#include <TBranch.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TClass.h>
#include <TClonesArray.h>
#include <TCollection.h>
#include <TColor.h>
#include <TEllipse.h>
#include <TError.h>
#include <TExec.h>
#include <TF1.h>
#include <TF2.h>
#include <TF3.h>
#include <TFile.h>
#include <TFolder.h>
#include <TFriendElement.h>
#include <TGClient.h>
#include <TGraph2D.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <THStack.h>
#include <THnBase.h>
#include <TKey.h>
#include <TLeaf.h>
#include <TLeafC.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TLine.h>
#include <TList.h>
#include <TMap.h>
#include <TMath.h>
#include <TMultiGraph.h>
#include <TNamed.h>
#include <TPave.h>
#include <TPaveText.h>
#include <TPolyLine.h>
#include <TROOT.h>
#include <TRefArray.h>
#include <TRootCanvas.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeIndex.h>
#include <TVirtualX.h>
#include <termios.h>

#include <boost/property_tree/info_parser.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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
#include <string_view>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Config.h"
#include "DataFrameRequest.h"
#include "PlotPainter.h"
#include "Validation.h"
#include "util/Paths.h"
#include "util/Regex.h"
#include "util/RootContainers.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"

using boost::property_tree::ptree;
using std::map;
using std::nullopt;
using std::optional;
using std::set;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;

namespace SciRooPlot
{
using util::disable_implicit_mt;
using util::expand_path;
using util::for_each_item;
using util::ignore_root_messages_below;
using util::is_container;
using util::make_scope_guard;
using util::plural;
using util::RegexMatcher;
using util::split_path;
using util::split_string;
using util::str_contains;
using util::str_ends_with;
using util::str_starts_with;

//**************************************************************************************************
/**
 * Constructor for PlotManager.
 */
//**************************************************************************************************
PlotManager::PlotManager(const string& projectName)
  : mProjectName(projectName), mUserDataFile((Config::Get().Path().empty()) ? string{} : (((mProjectName.empty()) ? Config::Get().Path() : Config::Get().ProjectPath(mProjectName)) / (mProjectName.empty() ? "UserData_" + std::to_string(gSystem->GetPid()) + "_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".root" : string{"UserData.root"})).string())
{
  // the name is a folder of the config path, as for srp add
  if (auto illegal = find_illegal_name_char(projectName)) logger::throw_invalid_argument("Project name '{}' contains illegal character '{}'.", projectName, *illegal);
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
  std::error_code ec;  // a destructor must not throw (no permission, a vanished folder)
  if (mProjectName.empty()) std::filesystem::remove(mUserDataFile, ec);
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
      logger::error("Could not create output file {}/{}.", mOutputDirectory, mPlotsRootFile);
      return false;
    }
    outputFile.cd();
    // the custom colours a canvas uses are defined by its first TExec, which also reserves the colour indices of all canvases of the file;
    // ROOT's own colour list is saved with the first canvas as usual
    auto joinPath = [](const string& folder, const string& name) { return (folder.empty()) ? name : folder + "/" + name; };
    // a ROOT directory cannot hold an object and a subdirectory of the same name: a plot named like a subgroup of its group gets a suffix
    set<string> directories;
    set<string> plotPaths;
    for (const auto& [uniqueName, canvas] : mCanvasRegistry) {
      const size_t delimiterPos = uniqueName.find(":");
      const string subfolder = uniqueName.substr(delimiterPos + 1);
      plotPaths.insert(joinPath(subfolder, uniqueName.substr(0, delimiterPos)));
      for (size_t pos = subfolder.find('/');; pos = subfolder.find('/', pos + 1)) {
        directories.insert(subfolder.substr(0, pos));
        if (pos == string::npos) break;
      }
    }
    uint32_t nPlots{0u};
    for (const auto& [uniqueName, canvas] : mCanvasRegistry) {
      size_t delimiterPos = uniqueName.find(":");
      string plotName = uniqueName.substr(0, delimiterPos);
      string subfolder = uniqueName.substr(delimiterPos + 1);
      string keyName = plotName;
      while (directories.count(joinPath(subfolder, keyName)) || (keyName != plotName && plotPaths.count(joinPath(subfolder, keyName)))) {
        keyName += "_plot";
      }
      if (keyName != plotName) {
        logger::warning("Plot {} of group {} is stored as {} in {}, since the group has a subgroup of the same name.", plotName, subfolder, keyName, mPlotsRootFile);
      }
      if (!outputFile.GetDirectory(subfolder.data(), kFALSE, "cd")) {
        outputFile.mkdir(subfolder.data());
      }
      outputFile.cd(subfolder.data());
      ReserveColorIndices(canvas.get());
      canvas->Write(keyName.data());
      ++nPlots;
    }
    outputFile.Close();
    logger::info("Saved {} plots to file {}/{}.", nPlots, mOutputDirectory, mPlotsRootFile);
  }
  return true;
}

//**************************************************************************************************
/**
 * Lets a canvas read back from a file reserve the colour indices of the session that wrote it: all colours it (and every canvas
 * written before it) uses have indices up to the highest one existing now. The first TExec of the canvas creates a colour at this
 * index if there is none, so that whatever canvas of the file is drawn first, the colours the reading session creates afterwards
 * (e.g. for a palette) get higher indices and are not overwritten by the colour definitions of the canvases drawn later.
 */
//**************************************************************************************************
void PlotManager::ReserveColorIndices(TCanvas* canvas)
{
  const int32_t highestColor = gROOT->GetListOfColors()->GetLast();
  const string reservation = fmt::format("if (!gROOT->GetColor({0})) new TColor({0}, 1, 1, 1);", highestColor);
  auto* colorExec = dynamic_cast<TExec*>(canvas->GetListOfPrimitives()->FindObject("define_colors"));
  if (!colorExec) {
    colorExec = new TExec("define_colors", "");
    colorExec->SetBit(kCanDelete);
    canvas->GetListOfPrimitives()->AddFirst(colorExec);
  }
  const string action = colorExec->GetTitle();
  if (str_starts_with(action, "if (!gROOT->GetColor(")) {
    colorExec->SetAction((reservation + action.substr(action.find(';') + 1)).data());  // written before: replace the reservation
  } else {
    colorExec->SetAction((reservation + action).data());
  }
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
    logger::error("Could not create output file {}/{}.", mOutputDirectory, mDataRootFile);
    return false;
  }
  // written in a fixed order (by name), so the same data always gives the same file
  auto sorted = [](const auto& unorderedMap) {
    map<string, const typename std::decay_t<decltype(unorderedMap)>::mapped_type*> view;
    for (const auto& [key, value] : unorderedMap)
      view.emplace(key, &value);
    return view;
  };
  for (const auto& [dataSource, buffer] : sorted(mDataBuffer)) {
    // the folder of a restricted data source (dataSource:some/folder) becomes subdirectories
    string sourcePath = dataSource;
    std::replace(sourcePath.begin(), sourcePath.end(), ':', '/');
    TDirectory* sourceDir = outputFile.mkdir(sourcePath.data(), "", true);
    if (!sourceDir) {
      logger::error("Could not create directory {} in {}.", sourcePath, mDataRootFile);
      continue;
    }
    const bool isUserDefined = is_user_defined(dataSource);
    for (const auto& [dataName, dataPtr] : sorted(*buffer)) {
      if (!*dataPtr) continue;
      // the name is the path of the data within its source, for tree projections followed by the request {...};
      // only that path becomes subdirectories, since request expressions and user-defined functions may contain '/' and ':'
      const size_t pathEnd = (isUserDefined) ? string::npos : dataName.rfind('/', dataName.find('{'));
      TDirectory* dir = sourceDir;
      if (pathEnd != string::npos) {
        dir = sourceDir->mkdir(dataName.substr(0, pathEnd).data(), "", true);
        if (!dir) {
          logger::error("Could not create directory {} for {} in {}.", dataName.substr(0, pathEnd), dataName, mDataRootFile);
          continue;
        }
      }
      dir->cd();
      const string keyName = (pathEnd == string::npos) ? dataName : dataName.substr(pathEnd + 1);
      // the object is written under the key's name as well (its name in memory carries the internal index and data source)
      if (auto* named = dynamic_cast<TNamed*>(dataPtr->get())) {
        const string internalName = named->GetName();
        named->SetName(keyName.data());
        named->Write(keyName.data());
        named->SetName(internalName.data());
      } else {
        (*dataPtr)->Write(keyName.data());
      }
    }
  }
  outputFile.Close();
  logger::info("Saved data to file {}/{}.", mOutputDirectory, mDataRootFile);
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
    logger::error("No output directory was specified: set one with SetOutputDirectory.");
    return false;
  }
  std::filesystem::path outputDir = std::filesystem::path(mOutputDirectory).lexically_normal();
  if (!outputDir.has_filename()) outputDir = outputDir.parent_path();  // strip trailing slash
  std::error_code ec;
  if (!std::filesystem::exists(outputDir.parent_path(), ec)) {
    logger::error("Parent path {} of output directory does not exist{}.", outputDir.parent_path().string(), ec ? " or cannot be read (" + ec.message() + ")" : "");
    return false;
  }
  std::filesystem::create_directory(outputDir, ec);
  if (ec) {
    logger::error("Could not create output directory {}: {}.", outputDir.string(), ec.message());
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
  const auto expandedPath = expand_path(path);
  if (!expandedPath) return;
  // a relative path is taken relative to the current directory, so that it can be stored with the project
  mOutputDirectory = (expandedPath->empty()) ? "" : std::filesystem::absolute(*expandedPath).lexically_normal().string();
}

//**************************************************************************************************
/**
 * Set the names of the ROOT files that the modes file and data write to the output directory (default: Plots.root, Data.root).
 */
//**************************************************************************************************
void PlotManager::SetPlotsRootFile(const string& fileName)
{
  mPlotsRootFile = fileName;
}
void PlotManager::SetDataRootFile(const string& fileName)
{
  mDataRootFile = fileName;
}

//**************************************************************************************************
/**
 * Define the inputs of a data source: files, folders within files (file.root:folder), directories or wildcard patterns.
 */
//**************************************************************************************************
bool PlotManager::IsValidDataSourceName(const string& dataSource)
{
  if (dataSource.empty()) {
    logger::error("Specify a name for the data source.");
    return false;
  }
  if (auto illegal = find_illegal_name_char(dataSource)) {
    logger::error("Data source '{}' contains illegal character '{}'.", dataSource, *illegal);
    return false;
  }
  return true;
}
// an input file.root:folder/sub is split after the .root ending (a directory in the path may contain ':', e.g. a time stamp, or
// even '.root:': split after an existing file, an existing path is not split, otherwise after the first .root)
vector<string> PlotManager::SplitInputPath(const string& input)
{
  const size_t first = input.find(".root:");
  if (first == string::npos) return {input};
  std::error_code ec;
  for (size_t pos = first; pos != string::npos; pos = input.find(".root:", pos + 1)) {
    if (std::filesystem::is_regular_file(input.substr(0, pos + 5), ec)) return {input.substr(0, pos + 5), input.substr(pos + 6)};
  }
  if (std::filesystem::exists(input, ec)) return {input};
  return {input.substr(0, first + 5), input.substr(first + 6)};
}
void PlotManager::AddDataSource(const string& dataSource, const vector<string>& inputs, bool replace)
{
  if (!IsValidDataSourceName(dataSource)) return;
  if (replace) mInputs.erase(dataSource);
  // data read before is read again from the changed inputs (a tree is then chained over the added file too)
  if (replace || !inputs.empty()) ForgetBufferedData(dataSource);
  for (const auto& input : inputs) {
    const auto expandedInput = expand_path(input);
    if (!expandedInput) continue;
    if (std::filesystem::path(*expandedInput).is_relative()) {
      logger::warning("The path of an input must not be relative. Skipping {}.", input);
      continue;
    }
    auto& inputsOfDataSource = mInputs[dataSource];
    if (std::find(inputsOfDataSource.begin(), inputsOfDataSource.end(), input) == inputsOfDataSource.end()) {
      inputsOfDataSource.push_back(input);
    }
  }
}
void PlotManager::AddDataSource(const string& dataSource, std::initializer_list<string> inputs, bool replace)
{
  AddDataSource(dataSource, vector<string>(inputs), replace);
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
  if (!IsValidDataSourceName(dataSource)) return;
  if (mUserDataFile.empty()) {
    logger::error("No valid config path; cannot store the objects of data source {}.", dataSource);
    return;
  }
  // started fresh once per process: a second PlotManager of the same project keeps the data the first one stored
  static std::set<string> startedFiles;
  string mode = (mUserDataFileInitialized || startedFiles.count(mUserDataFile)) ? "UPDATE" : "RECREATE";
  // the folder of the file: the project's folder, or without a project the config path, which does not exist before srp first ran
  std::error_code dirError;
  std::filesystem::create_directories(std::filesystem::path(mUserDataFile).parent_path(), dirError);

  TDirectory::TContext keepCurrentDirectory;  // the calling program's current directory stays as it is
  TFile file(mUserDataFile.data(), mode.data());
  if (file.IsZombie()) {
    logger::error("Could not save ROOT data to data source {}.", dataSource);
    return;
  }
  mUserDataFileInitialized = true;
  startedFiles.insert(mUserDataFile);

  TDirectory* dir = file.GetDirectory(dataSource.data());
  if (dir && replace) {
    file.rmdir(dataSource.data());
    dir = nullptr;
  }
  if (!dir) {
    dir = file.mkdir(dataSource.data());
  }
  if (!dir) {
    logger::error("Could not create directory {} in {}.", dataSource, mUserDataFile);
    return;
  }
  // Lists and directories become folders of the data source (a list without a name, for which ROOT reports its class name,
  // and a file are added item by item), so their items are addressed by path and stored like objects given directly.
  // Folders of the same name are merged; an object replaces one of the same name in its folder.
  std::function<void(TObject*, TDirectory*, const string&)> add = [&](TObject* object, TDirectory* target, const string& folder) {
    const string name = object->GetName();
    if (name.empty()) {
      logger::warning("Cannot add nameless object of type {} to data source {}.", object->ClassName(), dataSource);
      return;
    }
    auto* list = (object->InheritsFrom(TMap::Class())) ? nullptr : dynamic_cast<TCollection*>(object);
    auto* directory = dynamic_cast<TDirectory*>(object);
    if (list || directory) {
      TDirectory* subDir = target;
      string subFolder = folder;
      if (!dynamic_cast<TFile*>(object) && !(list && name == object->ClassName())) {
        subDir = target->GetDirectory(name.data());
        if (!subDir && target->FindKey(name.data())) {
          // an object of that name is stored there already: a folder cannot be made beside it (ROOT's mkdir would refuse with an error)
          logger::warning("Data source {}: the list {}{} cannot be stored beside the object of the same name, its items are left out (names inside a list must be unique).", dataSource, folder, name);
          return;
        }
        if (!subDir) subDir = target->mkdir(name.data());
        if (!subDir) {
          logger::error("Could not create folder {} in data source {}.", folder + name, dataSource);
          return;
        }
        subFolder += name + "/";
      }
      if (list) {
        for (TObject* item : *list) {
          if (item) add(item, subDir, subFolder);
        }
        return;
      }
      // a directory: the objects in memory (possibly newer than what is written), then the latest cycle of each key
      set<string> added;
      for (TObject* item : *directory->GetList()) {
        if (item && added.insert(item->GetName()).second) add(item, subDir, subFolder);
      }
      for (auto* key : TRangeDynCast<TKey>(directory->GetListOfKeys())) {
        if (!key || directory->FindKey(key->GetName()) != key || !added.insert(key->GetName()).second) continue;
        if (KeyInherits(key, TDirectory::Class())) {
          if (auto* keyDir = directory->GetDirectory(key->GetName())) add(keyDir, subDir, subFolder);  // owned by the file
        } else if (!ReadableClass(key)) {
          // ReadObj would return an object that is no TObject (e.g. an RNTuple, whose data stays in its file anyway)
          logger::warning("Cannot add {} of class {} from {} (data source {}): {}.", key->GetName(), key->GetClassName(), directory->GetPath(), dataSource,
                          (std::string_view(key->GetClassName()) == "ROOT::RNTuple") ? "give the file by its path to read an RNTuple" : "only TObjects whose class is known can be added");
        } else if (std::unique_ptr<TObject, container_deleter_t> item{key->ReadObj()}) {
          // read from the file: deleted after writing it (a list with all its items)
          add(item.get(), subDir, subFolder);
        } else {
          logger::warning("Could not read {} of class {} from {} (data source {}).", key->GetName(), key->GetClassName(), directory->GetPath(), dataSource);
        }
      }
      return;
    }
    if (target->FindKey(name.data())) {
      logger::warning("Data source {} receives a second object named {}{}: it replaces the first one.", dataSource, folder, name);
    }
    target->cd();
    // a tree read from a file keeps its entries in that file, writing it would only store the header: store a copy of all entries
    // (GetDirectory()->GetFile() of a tree in memory is the file opened last, here the one written to)
    if (auto* tree = dynamic_cast<TTree*>(object); tree && tree->GetCurrentFile()) {
      TTree* copy = tree->CloneTree(-1, "fast");  // created in the current directory, i.e. in the target folder
      copy->Write(name.data(), TObject::kWriteDelete);
      delete copy;
      return;
    }
    object->Write(name.data(), TObject::kSingleKey | TObject::kWriteDelete);  // single key: a map stays one object
  };
  for (auto object : inputs) {
    if (object) add(object, dir, "");
  }
  file.Close();
  AddDataSource(dataSource, {mUserDataFile + ":" + dataSource}, replace);
}

// data read from the inputs of a data source (also of its folders, dataSource:folder) is read again by the next GeneratePlots
void PlotManager::ForgetBufferedData(const string& dataSource)
{
  auto isPart = [&](const string& source) { return source == dataSource || source.rfind(dataSource + ":", 0) == 0; };
  // tree requests of other data sources joining a tree of this one: the join is in the name suffix as {...@join=tree|source|keys|alias}
  auto joinsIt = [&](const string& name) {
    return name.find("join=") != string::npos && (name.find("|" + dataSource + "|") != string::npos || name.find("|" + dataSource + ":") != string::npos);
  };
  for (auto it = mDataBuffer.begin(); it != mDataBuffer.end();) {
    if (isPart(it->first)) {
      it = mDataBuffer.erase(it);
      continue;
    }
    for (auto data = it->second.begin(); data != it->second.end();)
      data = joinsIt(data->first) ? it->second.erase(data) : std::next(data);
    ++it;
  }
  for (auto it = mDataInfoBuffer.begin(); it != mDataInfoBuffer.end();) {
    it = (isPart(it->first)) ? mDataInfoBuffer.erase(it) : std::next(it);
  }
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
  WriteInfoFile(dataSourcesTree, (file) ? *file : Config::Get().DataSourcesFile(mProjectName), "data sources");
}

//**************************************************************************************************
/**
 * Write the definitions of plots or data sources to a file (creating its folder if needed).
 */
//**************************************************************************************************
void PlotManager::WriteInfoFile(const ptree& tree, const string& path, const string& content)
{
  const std::filesystem::path filePath = expand_path(path).value_or("");
  if (filePath.empty()) {
    logger::error("No file path or project specified. Cannot save {}.", content);
    return;
  }
  std::error_code ec;
  if (filePath.has_parent_path() && std::filesystem::create_directories(filePath.parent_path(), ec)) {
    logger::info("Created config folder: {}.", filePath.parent_path().string());
  }
  try {
    boost::property_tree::write_info(filePath.string(), tree);
  } catch (const std::exception& e) {
    logger::error("Cannot save {} to {}: {}.", content, filePath.string(), e.what());
  }
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
    read_info(expand_path((file) ? *file : Config::Get().DataSourcesFile(mProjectName)).value_or(""), dataSourcesTree);
  } catch (const std::exception& e) {
    logger::error("Cannot load dataSources file: {}.", e.what());
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
    set<string> visited;  // keys can appear in several cycles: the latest directory of a name (newer data of the name does not hide it)
    for (auto* key : TRangeDynCast<TKey>(dir->GetListOfKeys())) {
      if (!key || !KeyInherits(key, TDirectory::Class()) || !visited.insert(key->GetName()).second) continue;
      if (fnmatch(levels[level].data(), key->GetName(), 0) != 0) continue;
      if (auto* subDir = OpenDirectory(dir, key)) {
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
  // an input is a file (device and inode, so the same file reached via a symlink, a hard link or "/./" counts once) plus a folder in it;
  // a file that cannot be found keeps its path as identity (it is reported where it is opened)
  using InputID = tuple<dev_t, ino_t, string>;
  map<InputID, string> seen;  // first spelling of each input
  auto fileID = [](const string& filePart) {
    struct stat info{};
    const string path = expand_path(filePart).value_or("");
    return (stat(path.data(), &info) == 0) ? InputID{info.st_dev, info.st_ino, ""} : InputID{0, 0, path};
  };
  auto addInput = [&](const InputID& file, const string& filePart, const string& folder) {
    const string input = folder.empty() ? filePart : filePart + ":" + folder;
    auto [it, isNew] = seen.try_emplace({std::get<0>(file), std::get<1>(file), std::get<2>(file) + ":" + folder}, input);
    if (isNew) {
      inputs.push_back(input);
    } else if (expand_path(it->second) != expand_path(input)) {
      logger::warning("Skipping input {} of data source {}: same file as input {}.", input, dataSource, it->second);
    }
  };

  // folders matching the (wildcard) path within a root file, e.g. DF_* or run*/sub
  auto expandFolders = [&](const string& fileName, const string& folderPattern) {
    TFile file(fileName.data(), "READ");
    if (file.IsZombie()) {
      logger::warning("Cannot open input file {} (data source {}).", fileName, dataSource);
      return vector<string>{};
    }
    return MatchFolders(&file, folderPattern);
  };

  auto addFile = [&](const string& filePart, const string& folderPart) {
    if (!subFolder.empty() && !str_ends_with(filePart, ".root")) return;  // tables have no folders
    const string folderPath = (subFolder.empty() || folderPart.empty()) ? folderPart + subFolder : folderPart + "/" + subFolder;
    const InputID file = fileID(filePart);
    if (hasWildcard(folderPath)) {
      const auto folders = expandFolders(expand_path(filePart).value_or(""), folderPath);
      // inputs that do not contain the folder of a restricted data source are simply not part of it
      if (folders.empty() && subFolder.empty()) logger::warning("No folders match {} in {} (data source {}).", folderPath, filePart, dataSource);
      for (const auto& folder : folders) {
        addInput(file, filePart, folder);
      }
    } else {
      addInput(file, filePart, folderPath);
    }
  };

  // literal: a match of a pattern is a path, not a pattern again (a file b?.root would match itself forever, run[1]/ no longer match)
  std::function<void(const string&, bool)> addEntry;
  addEntry = [&](const string& entry, bool literal) {
    auto fileAndFolder = SplitInputPath(entry);
    const string& filePart = fileAndFolder[0];
    const string folderPart = (fileAndFolder.size() > 1) ? fileAndFolder[1] : "";

    // a path that exists as written is taken as it is, even with [ ? * in its name
    std::error_code existsError;
    if (!literal && hasWildcard(filePart) && !std::filesystem::exists(expand_path(filePart).value_or(""), existsError)) {
      glob_t matches{};
      const bool found = (glob(expand_path(filePart).value_or("").data(), 0, nullptr, &matches) == 0);
      if (!found) logger::warning("No files match {} (data source {}).", filePart, dataSource);
      for (size_t i = 0; found && i < matches.gl_pathc; ++i) {
        addEntry(string(matches.gl_pathv[i]) + ((fileAndFolder.size() > 1) ? ":" + folderPart : ""), true);
      }
      globfree(&matches);
      return;
    }
    if (str_ends_with(filePart, ".root") || str_ends_with(entry, mTableFileEndings)) {
      addFile(filePart, folderPart);
      return;
    }
    string path = expand_path(entry).value_or("");
    std::error_code dirError;
    if (!std::filesystem::is_directory(path, dirError)) {
      logger::warning("Data source entry {} (data source {}) is neither a recognized file type nor an existing directory. Skipping.", entry, dataSource);
      return;
    }
    // directories that cannot be read are skipped with a warning: their files are missing then (and reported as such where they are needed)
    auto isReadable = [](const std::filesystem::path& dir) { return access(dir.c_str(), R_OK | X_OK) == 0; };
    if (!isReadable(path)) {
      logger::warning("Cannot read directory {} (data source {}). Skipping.", entry, dataSource);
      return;
    }
    vector<string> dirFileNames;
    std::error_code error;
    std::filesystem::recursive_directory_iterator file(path, std::filesystem::directory_options::skip_permission_denied, error);
    for (; !error && file != std::filesystem::recursive_directory_iterator(); file.increment(error)) {
      // linked directories are not followed (they could form cycles or contain the same files twice)
      if (file->is_symlink() && file->is_directory()) {
        logger::warning("Skipping symlinked directory {} (data source {}). Add it as its own entry to include it.", file->path().string(), dataSource);
        continue;
      }
      if (file->is_directory()) {
        if (!isReadable(file->path())) {
          logger::warning("Cannot read directory {} (data source {}). Skipping.", file->path().string(), dataSource);
          file.disable_recursion_pending();
        }
        continue;
      }
      if (file->path().extension() == ".root" || str_ends_with(file->path().string(), mTableFileEndings)) {
        dirFileNames.push_back(file->path().string());
      }
    }
    // any other error ends the walk (the directory iterator cannot continue after it)
    if (error) logger::warning("Could not read all of directory {} (data source {}): {}.", entry, dataSource, error.message());
    if (dirFileNames.empty()) logger::warning("No ROOT or table files found in directory {} (data source {}).", entry, dataSource);
    std::sort(dirFileNames.begin(), dirFileNames.end());
    for (const auto& fileName : dirFileNames) {
      addFile(fileName, "");
    }
  };
  for (const auto& entry : inputsIt->second) {
    addEntry(entry, false);
  }
  return inputs;
}

//**************************************************************************************************
/**
 * Whether a data source is defined (for one restricted to a folder: dataSource:some/folder, the data source it is part of).
 * The data sources of functions and points defined in the plots (AddFunction, AddPoints) always are.
 */
//**************************************************************************************************
bool PlotManager::IsDataSourceDefined(const string& dataSource) const
{
  if (is_user_defined(dataSource)) return true;
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
    logger::error("Cannot add plot ({}) that does not belong to a group.", plot.GetName());
    return;
  } else if (plot.GetGroup() == "BASE_PLOTS") {
    logger::error("You cannot use reserved group name 'BASE_PLOTS'.");
    return;
  }
  // pad 0 only holds the defaults for all pads, nothing added to it is drawn
  if (auto pad0 = plot.GetPads().find(0); pad0 != plot.GetPads().end() && (!pad0->second.GetData().empty() || pad0->second.GetFrame() || !pad0->second.GetLegendBoxes().empty() || !pad0->second.GetTextBoxes().empty() || !pad0->second.GetImageBoxes().empty())) {
    logger::warning("Plot {} in {}: data, frames, legends, texts and images added to pad 0 are not drawn (pad 0 holds the defaults for all pads, the pads are counted from 1).", plot.GetName(), plot.GetGroup());
  }
  // Filter, Define, Join and Entries select the rows of a tree or table: without Project*, Profile* or Scatter nothing is requested from it
  auto warnUnusedRequest = [&plot](const Plot::Pad::Data::data_info_t& info, const string& name) {
    if (!info.dataDims.empty()) return;
    if (info.filters || info.definitions.keys || info.joins || info.entries.min || info.entries.max) {
      logger::warning("Plot {} in {}: Filter, Define, Join and Entries of {} have no effect without Project, Profile or Scatter.", plot.GetName(), plot.GetGroup(), name);
    }
  };
  for (auto& [padID, pad] : plot.GetPads()) {
    for (const auto& data : pad.GetData()) {
      warnUnusedRequest(data->GetDataInfo(), data->GetName());
      if (auto ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data)) warnUnusedRequest(ratio->GetDenomDataInfo(), ratio->GetDenomName());
    }
  }
  mPlots.erase(std::remove_if(mPlots.begin(), mPlots.end(),
                              [&plot](const Plot& curPlot) {
                                bool removePlot = curPlot.GetUniqueName() == plot.GetUniqueName();
                                if (removePlot) logger::warning("Plot {} in {} already exists and will be replaced.", curPlot.GetName(), curPlot.GetGroup());
                                return removePlot;
                              }),
               mPlots.end());
  mPlots.push_back(std::move(plot));
}

BasePlotShape& BasePlotShape::SetAspectRatio(double_t ratio)
{
  if (!std::isfinite(ratio) || ratio <= 0.) {
    logger::warning("BasePlotShape::SetAspectRatio({}) needs a positive finite value, ignoring it.", ratio);
    return *this;
  }
  mAspectRatio = ratio;
  return *this;
}

BasePlotShape& BasePlotShape::SetRatioHeight(double_t height)
{
  if (!std::isfinite(height) || height <= 0.) {
    logger::warning("BasePlotShape::SetRatioHeight({}) needs a positive finite value, ignoring it.", height);
    return *this;
  }
  mRatioHeight = height;
  return *this;
}

BasePlotShape& BasePlotShape::SetPullHeight(double_t height)
{
  if (!std::isfinite(height) || height <= 0.) {
    logger::warning("BasePlotShape::SetPullHeight({}) needs a positive finite value, ignoring it.", height);
    return *this;
  }
  mPullHeight = height;
  return *this;
}

//**************************************************************************************************
/**
 * Base plots with consistent layout among them (see PlotManager.h). The name describes the layout:
 * <type>[_wide|_tall][_<columns>x<rows>][_ratio][_pull][_gap]. The geometry and the sizes come from the PanelLayout
 * (its defaults are the proportions of the 788 x 788 pixel standard plot, or the lengths of sizes; the proportions of
 * wide, tall, ratio and pull panels come from shape), the look is set with the ordinary plot setters, and the conventions
 * of the ratio and pull pads are added afterwards.
 */
//**************************************************************************************************
Plot PlotManager::MakeBasePlot(const string& name, const PanelLayout& sizes, const BasePlotShape& shape)
{
  static const std::regex pattern(R"(^(1d|2d)(?:_(wide|tall))?(?:_([1-9]\d*)x([1-9]\d*))?(_ratio)?(_pull)?(_gap)?$)");
  std::smatch match;
  if (!std::regex_match(name, match, pattern)) {
    logger::error("Unknown base plot {}: the name must follow <type>[_wide|_tall][_<columns>x<rows>][_ratio][_pull][_gap] with type 1d or 2d (e.g. 1d, 1d_ratio, 1d_ratio_pull, 2d, 1d_wide, 1d_3x1, 1d_2x1_ratio).", name);
    return Plot();
  }
  const bool zAxis = (match[1] == "2d");
  const string form = match[2];
  // more than 3 digits (no leading zeros) is more than 255 pads, and could overflow std::stoi
  if (match[3].length() > 3 || match[4].length() > 3) {
    logger::error("Base plot {} would have more than 255 pads.", name);
    return Plot();
  }
  const int32_t nCols = match[3].matched ? std::stoi(match[3]) : 1;
  const int32_t nRows = match[4].matched ? std::stoi(match[4]) : 1;
  const bool gap = match[7].matched;
  // the panels below each main panel: their height and the function of their reference line
  vector<std::pair<double_t, string>> below;
  if (match[5].matched) below.emplace_back(shape.mRatioHeight, "1");
  if (match[6].matched) below.emplace_back(shape.mPullHeight, "0");
  if (zAxis && !below.empty()) {
    logger::error("Unknown base plot {}: ratio and pull panels exist only for 1d base plots.", name);
    return Plot();
  }
  // touching 2d panels have one more pad, the shared colour scale
  const bool sharedScale = zAxis && nCols * nRows > 1 && !gap;
  if (int64_t{nCols} * nRows * static_cast<int64_t>(1 + below.size()) + (sharedScale ? 1 : 0) > 255) {
    logger::error("Base plot {} would have more than 255 pads.", name);
    return Plot();
  }

  // the arrangement of the panels, in units of the standard panel; a grid of 2d panels touching each other shares one colour scale,
  // in a cell right of the first row spanning all rows (its pad follows the panels of the first row)
  const double_t width = (form == "wide") ? shape.mAspectRatio : 1.;
  const double_t height = (form == "tall") ? shape.mAspectRatio : 1.;
  vector<vector<Panel>> rows;
  vector<std::pair<uint8_t, string>> refPads;  // the pads below the main panels and the functions of their reference lines
  vector<uint8_t> panelPads;
  uint8_t padID = 1;
  for (int32_t row = 0; row < nRows; ++row) {
    if (row > 0 && gap) rows.push_back({Gap()});
    vector<Panel> mainRow;
    for (int32_t col = 0; col < nCols; ++col) {
      if (col > 0 && gap) mainRow.push_back(Gap());
      mainRow.push_back((zAxis && !sharedScale) ? Panel(width, height).ColorScale() : Panel(width, height));
      panelPads.push_back(padID++);
    }
    if (sharedScale && row == 0) {
      mainRow.push_back(ColorScale().Span(1, static_cast<uint8_t>(nRows)));
      ++padID;
    }
    rows.push_back(mainRow);
    for (const auto& [panelHeight, refFunc] : below) {
      vector<Panel> belowRow;
      for (int32_t col = 0; col < nCols; ++col) {
        if (col > 0 && gap) belowRow.push_back(Gap());
        belowRow.push_back(Panel(width, panelHeight));
        refPads.emplace_back(padID++, refFunc);
      }
      rows.push_back(belowRow);
    }
  }

  Plot plot(name, PanelLayout(sizes, rows));                            // with the look of the shipped plots (see PanelLayout::ApplyTo)
  if (sharedScale) plot.mSharedAxes.push_back({'Z', panelPads, true});  // part of the layout, like the axes of touching panels

  // conventions of the ratio and pull pads: a line at 1 (0), few divisions, the title in the middle of the short axis
  for (const auto& [refPad, refFunc] : refPads) {
    plot[refPad].SetRefFunc(refFunc);
    plot[refPad]['Y'].SetNumDivisions(305).SetTitleCenter();
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
    logger::error("Cannot add base plot without a name.");
    return;
  }
  // base plots are found by their name alone
  if (!basePlot.GetGroup().empty() && basePlot.GetGroup() != "BASE_PLOTS") logger::warning("Base plot {}: its group {} is ignored (base plots have no group).", basePlot.GetName(), basePlot.GetGroup());
  basePlot.SetGroup("BASE_PLOTS");
  // base plots define the layout: their data would never be read (only the data of the plots is), and would define the frame,
  // take the first default styles and the first data numbers of every plot built on them
  for (auto& [padID, pad] : basePlot.GetPads()) {
    if (pad.GetData().empty() && !pad.GetFrame()) continue;
    logger::warning("Base plot {} has data or a frame in pad {}, which is ignored: data belongs into the plots (shared data e.g. via a loop or a data template).", basePlot.GetName(), padID);
    pad.GetData().clear();
    pad.mFrame.reset();
  }
  mBasePlots.erase(std::remove_if(mBasePlots.begin(), mBasePlots.end(),
                                  [&basePlot](const Plot& curBasePlot) {
                                    bool removePlot = curBasePlot.GetUniqueName() == basePlot.GetUniqueName();
                                    if (removePlot) logger::warning("Base plot {} already exists and will be replaced.", curBasePlot.GetName());
                                    return removePlot;
                                  }),
                   mBasePlots.end());
  mBasePlots.push_back(std::move(basePlot));
}

//**************************************************************************************************
/**
 * Add a plot showing the colours the data of a pad of a base plot gets when it sets none, in this order: the pad's own colour list
 * or gradient, else that of pad 0, else SciRooPlot's (as the painter takes them; the marker colours, which SetDefaultColors sets too).
 * A gradient is shown with its number of colours, or with 8 when that depends on the data.
 */
//**************************************************************************************************
void PlotManager::AddColorOverview(const string& name, const string& group, const string& basePlot, uint8_t padID)
{
  auto base = std::find_if(mBasePlots.begin(), mBasePlots.end(), [&](const auto& plot) { return plot.GetName() == basePlot; });
  if (base == mBasePlots.end()) {
    logger::error("Colour overview {}: there is no base plot {} (add it with AddBasePlot before).", name, basePlot);
    return;
  }
  auto& pads = base->GetPads();
  if (padID == 0 || pads.find(padID) == pads.end()) {
    logger::error("Colour overview {}: base plot {} has no pad {}.", name, basePlot, padID);
    return;
  }
  auto colorsOf = [](const Plot::Pad& pad) -> optional<vector<color_t>> {
    if (pad.GetDefaultMarkerColors()) return pad.GetDefaultMarkerColors();
    const auto& gradient = pad.GetDefaultMarkerColorsGradient();
    if (!gradient.rgbEndpoints) return {};
    vector<color_t> colors;
    for (const auto& [r, g, b] : PlotPainter::GradientRGB(gradient.nColors.value_or(8), *gradient.rgbEndpoints))
      colors.emplace_back(r, g, b);
    return colors;
  };
  const auto padZero = pads.find(0);
  optional<vector<color_t>> colors = colorsOf(pads[padID]);
  if (!colors && padZero != pads.end()) colors = colorsOf(padZero->second);
  AddColorOverview(name, group, colors.value_or(PlotPainter::DefaultColors()));
}

//**************************************************************************************************
/**
 * Add a plot showing the given colours, by default those SciRooPlot gives to data without a colour of its own, each with its name and index.
 */
//**************************************************************************************************
void PlotManager::AddColorOverview(const string& name, const string& group, const vector<color_t>& colors)
{
  const vector<color_t>& shown = colors.empty() ? PlotPainter::DefaultColors() : colors;
  Plot plot(name, group);
  plot.SetDimensions(800, 800, true);
  plot[1].SetPosition(0., 0., 1., 1.);
  int32_t bestRows = 1;
  size_t bestCols = shown.size();
  double_t bestScore = -1;
  for (int32_t rows = 1; rows <= static_cast<int32_t>(shown.size()); ++rows) {
    size_t cols = (shown.size() + rows - 1) / rows;
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
  for (size_t i = 0; i < shown.size(); ++i) {
    size_t r = i / bestCols;
    size_t c = i % bestCols;
    double_t x = static_cast<double_t>(c) / bestCols;
    double_t y = 1.0 - static_cast<double_t>(r) / bestRows;
    double_t w = 1.0 / bestCols;
    double_t h = 1.0 / bestRows;
    // ROOT names the colours of its wheel (kRed+1) and calls 0 and 1 white and black; the others are known by their index only
    string label = shown[i].ToString();  // the index, or #rrggbb
    const auto [red, green, blue] = shown[i].ToFractions();
    if (const auto index = shown[i].GetIndex()) {
      if (const TColor* color = gROOT->GetColor(*index)) {
        string colorName = (*index == kWhite) ? "kWhite" : ((*index == kBlack) ? "kBlack" : color->GetName());
        if (str_starts_with(colorName, "k")) label = fmt::format("#splitline{{{}}}{{{}}}", colorName, *index);
      }
    }
    const bool dark = 0.299 * red + 0.587 * green + 0.114 * blue < 0.5;
    plot[1]
      .AddText(x, y, label)
      .SetSize(w, h)
      .SetFillStyle(1001)
      .SetFillColor(shown[i])
      .SetTextColor(dark ? kWhite : kBlack);
  }
  AddPlot(std::move(plot));
}

//**************************************************************************************************
/**
 * Whether a plot of the group and name matches the requested name and group regex (empty for an invalid expression, which is reported).
 */
//**************************************************************************************************
std::function<bool(const string&, const string&)> PlotManager::PlotMatcher(const string& name, const string& group)
{
  // groups are stored without trailing slashes: in a group copied from a path (group/sub/) the slash says where the group name ends
  string groupPattern = group;
  while (groupPattern.size() > 1 && groupPattern.back() == '/' && groupPattern[groupPattern.size() - 2] != '\\') {
    groupPattern.pop_back();
  }
  const bool endsHere = (groupPattern != group);
  if (endsHere) logger::info("Reading group '{}' as '{}' (the slash ends the group name).", group, groupPattern);
  const bool contains = Config::Get().MatchContains();
  const bool ignoreCase = Config::Get().MatchCaseInsensitive();
  // exact mode matches whole names anyway; contains mode must not take group/ for groupx
  RegexMatcher groupRegex = (endsHere && contains && RegexMatcher(groupPattern, contains, ignoreCase).IsValid())
                              ? RegexMatcher("(?:" + groupPattern + ")(?=/|$)", contains, ignoreCase)
                              : RegexMatcher::WithSubpaths(groupPattern, contains, ignoreCase);
  RegexMatcher nameRegex(name, contains, ignoreCase);
  if (!groupRegex.IsValid() || !nameRegex.IsValid()) {
    logger::error("'{}' is not a valid {} pattern{}.", groupRegex.IsValid() ? name : group, groupRegex.IsValid() ? "name" : "group", WildcardHint(name, group));
    return {};
  }
  return [groupRegex, nameRegex](const string& plotGroup, const string& plotName) { return groupRegex.Matches(plotGroup) && nameRegex.Matches(plotName); };
}

//**************************************************************************************************
/**
 * Hint for patterns written as shell wildcards (QA* instead of QA.*), empty if there are none.
 */
//**************************************************************************************************
string PlotManager::WildcardHint(const string& name, const string& group)
{
  const string groupPattern = RegexMatcher::FromWildcards(group);
  const string namePattern = RegexMatcher::FromWildcards(name);
  if (groupPattern.empty() && namePattern.empty()) return "";
  string suggestion = groupPattern.empty() ? "" : "group '" + groupPattern + "'";
  if (!namePattern.empty()) suggestion += (suggestion.empty() ? "name '" : " and name '") + namePattern + "'";
  return " (patterns are regular expressions: did you mean " + suggestion + "?)";
}

//**************************************************************************************************
/**
 * Save plots matching name and group regex to file.
 */
//**************************************************************************************************
void PlotManager::SavePlots(const string& name, const string& group, const optional<string>& file) const
{
  ptree plotTree;
  const auto matches = PlotMatcher(name, group);
  if (!matches) return;

  for (const vector<Plot>& plots : {std::ref(mBasePlots), std::ref(mPlots)}) {
    for (const Plot& plot : plots) {
      if (plot.GetGroup() != "BASE_PLOTS" && !matches(plot.GetGroup(), plot.GetName())) continue;
      plotTree.put_child(plot.GetUniqueName(), plot.GetPropertyTree());
    }
  }
  WriteInfoFile(plotTree, (file) ? *file : Config::Get().PlotsFile(mProjectName), "plots");
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

  const auto matches = PlotMatcher(name, group);
  if (!matches) return false;

  ptree fileTree;
  const string plotsFile = expand_path((file) ? *file : Config::Get().PlotsFile(mProjectName)).value_or("");
  if (std::error_code ec; !file && !std::filesystem::exists(plotsFile, ec)) {
    logger::error("No plots are saved for project {} yet: run its program first (it saves them with SaveProject).", mProjectName);
    return false;
  }
  try {
    using boost::property_tree::read_info;
    read_info(plotsFile, fileTree);
  } catch (const std::exception& e) {
    logger::error("Cannot open plots file: {}.", e.what());
    return false;
  }

  // keys of the file that a plot does not read (misspelt, a part without its number, renamed in another version) are named: those the plot
  // written again does not hold (a file written by SciRooPlot holds no others)
  const string fileName = (file) ? *file : Config::Get().PlotsFile(mProjectName);
  auto reportUnknownKeys = [&](const Plot& plot, const ptree& read) {
    vector<string> unknown;
    std::function<void(const ptree&, const ptree&, const string&)> compare = [&](const ptree& readNode, const ptree& writtenNode, const string& path) {
      std::map<string, size_t> occurrence;  // repeated keys (the rows of a legend) are compared in their order
      for (const auto& [key, child] : readNode) {
        auto [written, end] = writtenNode.equal_range(key);
        for (size_t i = occurrence[key]++; i > 0 && written != end; --i)
          ++written;
        if (written == end) {
          unknown.push_back(path + key);
        } else if (!child.empty()) {
          compare(child, written->second, path + key + ".");
        }
      }
    };
    compare(read, plot.GetPropertyTree(), "");
    if (unknown.empty()) return;
    const string what = (plot.GetGroup() == "BASE_PLOTS") ? fmt::format("Base plot {}", plot.GetName()) : fmt::format("Plot {} from group {}", plot.GetName(), plot.GetGroup());
    logger::warning("{} ignores what it does not know in {}: {}.", what, fileName, util::join_strings(unknown, ", "));
  };

  uint32_t nPlotsInFile{};
  uint32_t nBasePlotsLoaded{};
  for (const auto& plotTree : fileTree) {
    // an entry without group or name (a damaged or hand-edited file) is skipped, the other plots are still loaded
    const auto curGroupValue = plotTree.second.get_optional<string>("group");
    const auto curNameValue = plotTree.second.get_optional<string>("name");
    if (!curGroupValue || !curNameValue) {
      logger::error("Could not load plot {} from file: it has no {}.", plotTree.first, curGroupValue ? "name" : "group");
      continue;
    }
    const string& curGroup = *curGroupValue;
    if (curGroup == "BASE_PLOTS") {
      try {
        Plot basePlot(plotTree.second);
        reportUnknownKeys(basePlot, plotTree.second);
        AddBasePlot(std::move(basePlot));
        ++nBasePlotsLoaded;
      } catch (const std::exception& e) {
        logger::error("Could not load base plot {} from file: {}.", plotTree.first, util::without_final_period(e.what()));
      }
      continue;
    }
    ++nPlotsInFile;
    if (!matches(curGroup, *curNameValue)) continue;

    ++nMatched;
    try {
      Plot plot(plotTree.second);
      reportUnknownKeys(plot, plotTree.second);
      AddPlot(std::move(plot));
      ++nLoaded;
    } catch (const std::exception& e) {
      logger::error("Could not load plot {} from file: {}.", plotTree.first, util::without_final_period(e.what()));
    }
  }
  if (nPlotsInFile == 0 && nBasePlotsLoaded > 0) {
    // file containing only base plots
    logger::info("Loaded {}.", plural(nBasePlotsLoaded, "base plot"));
    return true;
  }
  if (nMatched == 0) {
    const string hint = WildcardHint(name, group);
    logger::error("Found no plots matching the request {}{}{} in {}{}{}{}.", logger::begin_color(logger::Color::Green), name, logger::end_color(), logger::begin_color(logger::Color::Yellow), group, logger::end_color(), hint.empty() ? " (plot .+ .+ list shows all plots of the project)" : hint);
  } else if (nLoaded == 0) {
    logger::error("{} plot(s) matched the request but none could be loaded.", nMatched);
  } else if (nLoaded > 1) {
    logger::info("Found {} plots matching the request.", nLoaded);
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
    logger::error("Invalid plot mode '{}' (srp help lists the modes).", mode);
    return false;
  }
  // ROOT writes html files from 6.42 on; before, TPad::SaveAs does not know the ending and writes PostScript
  if constexpr (ROOT_VERSION_CODE < ROOT_VERSION(6, 42, 0)) {
    if (mode == "html") {
      logger::error("Mode html needs ROOT 6.42 or newer: ROOT {} cannot write html files.", gROOT->GetVersion());
      return false;
    }
  }
  const bool isInteractiveMode = (mode == "show" || mode == "spread");
  const bool needsDisplay = isInteractiveMode || mode == "macro";
  // said once for all plots (a program plotting in memory has no project directory unless it sets one); print writes nothing
  if (!isInteractiveMode && mode != "print" && mOutputDirectory.empty()) {
    logger::error("No output directory was specified: set one with SetOutputDirectory before writing plots in mode '{}'.", mode);
    return false;
  }
  // plotting uses multi-threading, the classic canvas and fewer ROOT messages; the settings of the calling program are restored afterwards
  const bool wasMTEnabled = ROOT::IsImplicitMTEnabled();
  const bool wasWebDisplay = gROOT->IsWebDisplay();
  const TString webDisplay = gROOT->GetWebDisplay();
  auto globalsGuard = make_scope_guard([wasMTEnabled, wasWebDisplay, webDisplay, wasBatch = gROOT->IsBatch()]() {
    gROOT->SetBatch(wasBatch);
    if (wasWebDisplay) gROOT->SetWebDisplay(webDisplay.Data());
    if (!wasMTEnabled) ROOT::DisableImplicitMT();
  });
  if (!wasMTEnabled) ROOT::EnableImplicitMT();
  gROOT->SetWebDisplay("off");
  auto quiet = ignore_root_messages_below(kWarning);
  // ROOT's "Something strange: num. of critical sectors not equal 2" for the empty lego plot that draws the frame of every 3d view, in
  // spherical coordinates over a narrow range (ROOT bug 66): the picture is the same as without it
  static ErrorHandlerFunc_t outerHandler = nullptr;
  outerHandler = SetErrorHandler([](Int_t level, Bool_t abort, const char* location, const char* msg) {
    if (TString(location) == "TView3D::FindThetaSectors") return;
    outerHandler(level, abort, location, msg);
  });
  auto sectorsGuard = make_scope_guard([]() { SetErrorHandler(outerHandler); });
  // the files opened and closed while plotting would leave gDirectory at gROOT: objects the caller makes afterwards
  // would no longer go into the file it had open
  TDirectory::TContext keepCurrentDirectory;
  mWrittenFiles.clear();

  // a caller that set ROOT's batch mode (gROOT->SetBatch) gets no windows; a canvas made then would still paint through gVirtualX,
  // which is the display interface once ROOT's graphics were initialised, and crash
  if (gROOT->IsBatch()) {
    if (needsDisplay) {
      logger::error("Mode '{}' needs a graphical display, but ROOT runs in batch mode (is DISPLAY set, or was gROOT->SetBatch called?).", mode);
      return false;
    }
  } else if (needsDisplay && !mHasDisplay) {
    // only the modes with windows probe the display: the probe maps a window, which takes the focus
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
  if (needsDisplay && !*mHasDisplay) {
    logger::error("Mode '{}' needs a graphical display (is DISPLAY set?).", mode);
    return false;
  }
  // all other modes draw in batch mode, which needs ROOT's batch graphics: once a window was opened (e.g. by the display check above),
  // gVirtualX is the interface to the display, and since ROOT 6.40 painting a batch canvas through it crashes
  TVirtualX* const displayGraphics = gVirtualX;
  if (!needsDisplay) gVirtualX = gGXBatch;
  auto graphicsGuard = make_scope_guard([displayGraphics]() { gVirtualX = displayGraphics; });
  // first determine which data needs to be loaded
  vector<Plot*> selectedPlots;
  map<int32_t, set<int32_t>> requiredData;

  const auto matches = PlotMatcher(name, group);
  if (!matches) return false;

  // only what the selected plots need is read: the requests of earlier calls and what they could not read are dropped (data read stays buffered)
  mDataInfoBuffer.clear();
  for (auto& [dataSource, buffer] : mDataBuffer) {
    for (auto it = buffer.begin(); it != buffer.end();) {
      it = (it->second) ? std::next(it) : buffer.erase(it);
    }
  }
  for (auto& plot : mPlots) {
    if (!matches(plot.GetGroup(), plot.GetName())) continue;
    selectedPlots.push_back(&plot);
    if (mode == "print") continue;  // printing the definitions needs no data

    // determine which input data are needed for plots
    for (auto& [padID, pad] : plot.GetPads()) {
      if (auto refFunc = RefFuncDrawnIn(plot, padID)) mDataBuffer[refFunc->GetDataSource()][refFunc->GetName()];
      if (padID == 0) continue;  // data and frame of pad 0 are not drawn (AddPlot warns about the data), so they are not read either
      auto padData = pad.GetData();
      if (pad.GetFrame()) padData.push_back(pad.GetFrame());  // the frame of a pad is read like its data
      for (const auto& data : padData) {
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
    logger::error("No plots were created: found no plots matching the request {}{}{} in {}{}{}{}.", logger::begin_color(logger::Color::Green), name, logger::end_color(), logger::begin_color(logger::Color::Yellow), group, logger::end_color(), WildcardHint(name, group));
    return false;
  }
  if (mode == "print") {
    bool allPrinted = true;
    for (auto plot : selectedPlots) {
      allPrinted = GeneratePlot(*plot, mode) && allPrinted;
    }
    return allPrinted;
  }

  size_t nItemsToRead{};
  set<string> sourcesToRead;
  for (const auto& [dataSource, buffer] : mDataBuffer) {
    if (is_user_defined(dataSource)) continue;  // defined in the plots, nothing to read
    for (const auto& [dataName, dataPtr] : buffer) {
      if (IsBuffered(dataSource, dataName)) continue;
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
    logger::info("Reading {} from {}{}.", plural(nItemsToRead, "data item"), (nInputs) ? plural(nInputs, "input") + " in " : "", plural(sourcesToRead.size(), "data source"));
  }
  try {
    FillBuffer();
    {
      // the summary counts everything the plots cannot get, whether not found or failed to read (its details come with each plot)
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
        logger::error("{} of {} plots cannot be created due to {}.", nAffectedPlots, selectedPlots.size(), plural(missingItems.size(), "missing data item"));
      }
    }
    mGifName.clear();
    // the frames of a gif are the plots that can be made, in the order they were defined; the last one closes it (its file name tells ROOT
    // how the animation repeats). The gif is named after the deepest group all frames are in and lies next to the folder of that group
    // (frames of animations/legoRotation: animations/legoRotation.gif); frames without a common group make animation.gif in the output directory
    mGifLastFrame = nullptr;
    mGifSize.reset();
    mGifSizeReported = false;
    if (str_starts_with(mode, "gif")) {
      optional<vector<string>> commonGroup;
      // frames that cannot be made are left out here already (missing data, a missing base plot): the last one written must close the gif
      auto hasBasePlot = [this](const Plot& plot) {
        return !plot.GetBasePlotName() || std::any_of(mBasePlots.begin(), mBasePlots.end(), [&](const Plot& basePlot) { return basePlot.GetName() == *plot.GetBasePlotName(); });
      };
      for (auto plot : selectedPlots) {
        if (!GetMissingData(*plot).empty() || plot->GetGroup().empty() || !hasBasePlot(*plot)) continue;
        mGifLastFrame = plot;
        const vector<string> parts = split_string(plot->GetGroup(), '/');
        if (!commonGroup) {
          commonGroup = parts;
          continue;
        }
        commonGroup->erase(std::mismatch(commonGroup->begin(), commonGroup->end(), parts.begin(), parts.end()).first, commonGroup->end());
      }
      if (commonGroup) {
        string folder = mOutputDirectory;
        for (size_t i = 0; i + 1 < commonGroup->size(); ++i)
          folder += "/" + (*commonGroup)[i];
        mGifName = folder + "/" + (commonGroup->empty() ? string("animation") : commonGroup->back()) + ".gif";
        gSystem->Unlink(mGifName.data());  // the frames are appended to the file
      }
    }
    mExitInteractiveBrowsing = false;
    mDetached.reset();
    mBrowseCanvases.clear();
    if (isInteractiveMode) SetTerminalKeys(true);
    auto terminalKeysGuard = make_scope_guard([]() { SetTerminalKeys(false); });

    // generate plots
    bool allCreated = true;
    auto reportFailure = [&](Plot* plot) {
      auto missingData = GetMissingData(*plot);
      string message = fmt::format("Plot {}{}{} from group {}{}{} could not be created.", logger::begin_color(logger::Color::Green), plot->GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), plot->GetGroup(), logger::end_color());
      for (const auto& [dataSource, name, dataInfo] : missingData) {
        message += "\n         - missing " + DataLocation(dataSource, name);
        if (!dataInfo.dataDims.empty()) message += " [" + dataInfo.GetDescription() + "]";
        if (!IsDataSourceDefined(dataSource)) {
          message += " (data source not defined)";
        } else if (auto it = mNotFoundData.find(dataSource); it != mNotFoundData.end() && it->second.count(name)) {
          message += " (not found in the inputs of " + dataSource + ")";
        }
      }
      logger::error("{}", message);
      allCreated = false;
    };
    if (mode == "show") {
      // plots are made when browsing reaches them (GeneratePlot browses through the ones made and returns when another one is needed)
      mBrowseCanvases.assign(selectedPlots.size(), nullptr);
      mBrowseSkip.assign(selectedPlots.size(), false);
      mBrowseIndex = 0;
      mBrowseForward = true;
      mShownIndex.reset();
      mBrowseHistory.clear();
      mBrowsePosition.reset();
      while (!mExitInteractiveBrowsing) {
        if (!GeneratePlot(*selectedPlots[mBrowseIndex], mode)) {
          reportFailure(selectedPlots[mBrowseIndex]);
          mBrowseSkip[mBrowseIndex] = true;  // continue in the direction browsing went
          auto next = NextBrowseIndex(mBrowseIndex, mBrowseForward);
          if (!next) break;
          mBrowseIndex = *next;
        }
      }
    } else {
      for (auto plot : selectedPlots) {
        if (!GeneratePlot(*plot, mode)) reportFailure(plot);
        if (mExitInteractiveBrowsing) break;
      }
    }
    if (isInteractiveMode) {
      ShowDetached();
      // the browsing window is hidden once browsing has ended, windows kept open stay until the user closes them
      for (const auto& browsed : mBrowseCanvases) {
        if (!browsed) continue;
        Hide(browsed.get());
      }
      WaitForOpenWindows();
    }
    if (!mGifName.empty()) {
      logger::log("Saved gif {}.", mGifName);
    }
    bool saved = true;
    if (mode == "file") {
      saved = SavePlotsToRootFile();
    } else if (mode == "data") {
      saved = SaveDataToRootFile();
    }
    return allCreated && saved;
  } catch (const std::exception& e) {
    logger::error("An unexpected error occurred: {}.", e.what());
    return false;
  }
}

//**************************************************************************************************
/**
 * Whether data is already read (in this or a previous GeneratePlots() call). The results of tree and table requests are
 * stored under the name with the suffix of the request, while the entry of the name itself stays empty.
 */
//**************************************************************************************************
bool PlotManager::IsBuffered(const string& dataSource, const string& name) const
{
  const auto& buffer = mDataBuffer.at(dataSource);
  if (buffer.at(name)) return true;
  auto sourceInfos = mDataInfoBuffer.find(dataSource);
  if (sourceInfos == mDataInfoBuffer.end()) return false;
  auto infos = sourceInfos->second.find(name);
  if (infos == sourceInfos->second.end() || infos->second.empty()) return false;
  return std::all_of(infos->second.begin(), infos->second.end(), [&](const auto& dataInfo) {
    const string nameSuffix = dataInfo.GetNameSuffix();
    if (nameSuffix.empty()) return false;
    auto result = buffer.find(name + nameSuffix);
    return result != buffer.end() && result->second;
  });
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
  mNotFoundData.clear();
  mHiddenDirectories.clear();
  map<string, vector<string>> sourceGroups;  // data source -> requested data sources that are part of it (itself or restricted to a folder)
  for (const auto& [dataSource, buffer] : mDataBuffer) {
    sourceGroups[dataSource.substr(0, dataSource.find(':'))].push_back(dataSource);
  }
  for (const auto& [baseSource, dataSources] : sourceGroups) {
    success &= ReadSourceGroup(baseSource, dataSources);
  }
  return success;
}

//**************************************************************************************************
/**
 * Reads the data requested from a data source and from its folders (dataSource:some/folder) in one pass over its inputs; false if
 * some was not found.
 */
//**************************************************************************************************
bool PlotManager::ReadSourceGroup(const string& baseSource, const vector<string>& dataSources)
{
  map<string, unordered_map<string, set<string>>> requiredData;  // data source -> subdir -> names
  for (const auto& dataSource : dataSources) {
    for (auto& [dataName, dataPtr] : mDataBuffer[dataSource]) {
      if (IsBuffered(dataSource, dataName)) continue;

      // generate user-defined functions on-the-fly
      if (dataSource == kUserFunctions) {
        // a function is given as formula ("[0] * sin(x)") or as C++ lambda ("[](double* x, double* p) { return std::sin(x[0]); }"):
        // a lambda has no parameters that could be set, its dimension is that of the variables it uses (x[0], x[1], x[2])
        static const std::regex lambdaStart(R"(^\s*\[[^\]]*\]\s*\()");
        const bool isLambda = std::regex_search(dataName, lambdaStart);
        int32_t dim{};
        if (isLambda) {
          static const std::regex signature(R"(^\s*\[\s*\]\s*\(\s*(?:const\s+)?(?:double|Double_t)\s*\*\s*(\w+)\s*,\s*(?:const\s+)?(?:double|Double_t)\s*\*\s*(\w*)\s*\))");
          std::smatch names;
          if (!std::regex_search(dataName, names, signature)) {
            logger::error("Function '{}' needs the form [](double* x, double* p) {{ ... }}.", dataName);
            continue;
          }
          const string body = names.suffix();
          const string var = names[1];
          const std::regex variableUse("\\b" + var + "\\b(\\s*\\[\\s*([0-9]+)\\s*\\])?");
          bool isIndexed = true;
          dim = 1;
          for (auto use = std::sregex_iterator(body.begin(), body.end(), variableUse); use != std::sregex_iterator(); ++use) {
            isIndexed = isIndexed && (*use)[2].matched;
            if ((*use)[2].matched) dim = std::max(dim, std::stoi((*use)[2]) + 1);
          }
          if (!isIndexed || dim > 3) {
            logger::error("Function '{}' has to use its variables as {}[0], {}[1] or {}[2].", dataName, var, var, var);
            continue;
          }
          if (const string par = names[2]; !par.empty() && std::regex_search(body, std::regex("\\b" + par + "\\b"))) {
            logger::error("Function '{}' uses parameters, which cannot be set: write their values into the function.", dataName);
            continue;
          }
        }
        bool isValid{};
        int32_t nParameters{};
        {
          auto quiet = ignore_root_messages_below(kFatal);
          if (isLambda) {
            isValid = TFormula("tmp", dataName.data(), dim, 0, false).IsValid();
          } else {
            TFormula formula("tmp", dataName.data(), false);
            isValid = formula.IsValid();
            dim = formula.GetNdim();
            nParameters = formula.GetNpar();
          }
        }
        if (!isValid) {
          logger::error("'{}' is not a valid function expression.", dataName);
          continue;
        }
        // parameters ([0], [a], or those of predefined functions like gaus and pol2) cannot be set and would all be 0
        if (nParameters > 0) {
          logger::error("Function '{}' uses parameters, which cannot be set: write their values into the function.", dataName);
          continue;
        }
        // the lambda constructors of ROOT take the number of parameters and dimensions
        constexpr int32_t nPar = 0;
        if (dim <= 1) {
          auto* func = (isLambda) ? new TF1(dataName.data(), dataName.data(), 0., 1., nPar, 1) : new TF1(dataName.data(), dataName.data());
          func->SetNpx(1000);  // drawn across the whole frame, where ROOT's default of 100 points looks angular
          dataPtr.reset(func);
        } else if (dim == 2) {
          dataPtr.reset((isLambda) ? new TF2(dataName.data(), dataName.data(), 0., 1., 0., 1., nPar, 2) : new TF2(dataName.data(), dataName.data()));
        } else if (dim == 3) {
          dataPtr.reset((isLambda) ? new TF3(dataName.data(), dataName.data(), 0., 1., 0., 1., 0., 1., nPar, 3) : new TF3(dataName.data(), dataName.data()));
        } else {
          logger::error("Cannot create function {}.", dataName);
        }
        continue;
      } else if (dataSource == kUserGraphs) {
        auto strs = split_string(dataName, ';');
        if (strs.size() == 2) {
          auto xStrs = split_string(strs[0], ',');
          auto yStrs = split_string(strs[1], ',');
          if (!xStrs.size() || xStrs.size() != yStrs.size()) {
            logger::error("Incompatible number of points.");
          } else {
            vector<double_t> x;
            vector<double_t> y;
            try {  // independent of the locale, and subnormal values (1e-310) are numbers, not an error as for std::stod
              for (size_t i = 0; i < xStrs.size(); ++i) {
                x.push_back(util::string_to_type<double_t>(xStrs[i]));
                y.push_back(util::string_to_type<double_t>(yStrs[i]));
              }
              dataPtr.reset(new TGraph(static_cast<int32_t>(x.size()), x.data(), y.data()));
              static_cast<TGraph*>(dataPtr.get())->SetName(dataName.data());
            } catch (const std::invalid_argument& e) {
              logger::error("Cannot read the points {}: {}", dataName, e.what());
            }
          }
        }
        continue;
      }

      auto [path, name] = split_path(dataName);
      requiredData[dataSource][std::move(path)].insert(std::move(name));
    }
  }
  auto allFound = [&]() { return std::all_of(requiredData.begin(), requiredData.end(), [](const auto& entry) { return entry.second.empty(); }); };

  if (allFound()) return true;  // nothing left to read, e.g. read by a previous GeneratePlots call

  // open all input files belonging to the data source and extract the data
  mTreeInputs.clear();
  // consecutive inputs in the same file (e.g. its expanded folders) share one open file: the cache of the lists and folders
  // read from it is cleared before the file is closed
  struct open_file_t {
    std::unique_ptr<TFile> file;
    string name;
    read_cache_t readCache;
    void Close()
    {
      readCache.clear();
      file.reset();
      name.clear();
    }
    ~open_file_t() { Close(); }
  } openFile;
  for (const auto& inputRaw : mExpandedInputs[baseSource]) {
    if (allFound()) break;
    string input = expand_path(inputRaw).value_or("");
    if (str_ends_with(input, mTableFileEndings)) {
      // tables have no folders: only part of the data source itself, and only read if requested
      // (a directory source also contains tables nobody asked for, or text files that are no tables at all)
      if (auto it = requiredData.find(baseSource); it != requiredData.end()) {
        if (auto wantedIt = it->second.find(""); wantedIt != it->second.end()) {
          set<string>& wantedNames = wantedIt->second;
          const string name = std::filesystem::path(input).stem().string();
          if (wantedNames.erase(name)) {
            ReadTableData(input, name, baseSource);
            if (wantedNames.empty()) it->second.erase(wantedIt);
          }
        }
      }
    }
    // check if only a sub-folder in input file should be searched
    auto fileNamePath = SplitInputPath(input);
    string& fileName = fileNamePath[0];
    if (!str_ends_with(fileName, ".root")) continue;

    if (fileName != openFile.name) {
      openFile.Close();
      std::error_code existsError;
      if (!std::filesystem::exists(fileName, existsError)) {
        logger::warning("Input file {} not found{} (data source {}).", fileName, existsError ? " (" + existsError.message() + ")" : "", baseSource);
        continue;
      }
      openFile.file = std::make_unique<TFile>(fileName.data(), "READ");
      if (openFile.file->IsZombie()) {
        logger::warning("Cannot open input file {} (data source {}).", fileName, baseSource);
        openFile.file.reset();
        continue;
      }
      openFile.name = fileName;
    }
    TFile& inputFile = *openFile.file;
    read_cache_t& readCache = openFile.readCache;

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
          folder = FindSubDirectory(folder, split_string(folderPath, '/'), readCache);
          if (!folder) {
            // inputs that do not contain the folder of a restricted data source (dataSource:some/folder) are simply not part of it
            if (isRestricted) continue;
            logger::error("Subdirectory {} not found in file {}.", folderPath, fileName);
            continue;
          }
        }

        vector<string> emptySubDirs;
        for (auto& [pathStr, names] : required) {
          if (TObject* subfolder = FindSubDirectory(folder, split_string(pathStr, '/'), readCache)) {
            // recursively traverse the file and look for input files
            string prefix = (pathStr.empty()) ? "" : pathStr + "/";
            string suffix = ":" + dataSource;
            ReadData(subfolder, names, prefix, suffix, dataSource, readCache);
          }
          if (names.empty()) emptySubDirs.push_back(pathStr);
        }

        for (const auto& pathStr : emptySubDirs) {
          required.erase(pathStr);
        }
        // trees can also be in further inputs: keep looking for them
        for (const auto& [treeName, treeInputs] : mTreeInputs[dataSource]) {
          auto [path, name] = split_path(treeName);
          required[path].insert(std::move(name));
        }
      }
    }
  }
  ChainTrees(requiredData);
  mTreeInputs.clear();
  for (const auto& [dataSource, required] : requiredData) {
    for (const auto& [path, names] : required) {
      for (const auto& name : names) {
        mNotFoundData[dataSource].insert((path.empty()) ? name : path + "/" + name);
      }
    }
  }
  return allFound();
}

//**************************************************************************************************
/**
 * Reads the trees and RNTuples found in the inputs of a data source, each over all inputs that contain it.
 */
//**************************************************************************************************
void PlotManager::ChainTrees(map<string, unordered_map<string, set<string>>>& requiredData)
{
  // what a chained tree is made of in words, e.g. "3 files" or "412 folders in 3 files" (empty if it has only one input)
  auto describeInputs = [](const vector<tree_input_t>& treeInputs) -> string {
    if (treeInputs.size() < 2) return "";
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
      const auto [path, name] = split_path(treeName);
      if (auto it = required.find(path); it != required.end()) {
        auto& names = it->second;
        names.erase(name);
        if (names.empty()) required.erase(it);
      }
      // requests that join the same trees share one chain
      using join_t = Plot::Pad::Data::data_info_t::join_t;
      vector<vector<join_t>> joinGroups;
      for (const auto& dataInfo : mDataInfoBuffer[dataSource][treeName]) {
        auto joins = dataInfo.joins.value_or(vector<join_t>{});
        if (std::find(joinGroups.begin(), joinGroups.end(), joins) == joinGroups.end()) joinGroups.push_back(std::move(joins));
      }
      // RNTuples are read by RDataFrame from their files directly; ROOT can chain them only under the same path and cannot join them
      const bool isRNTuple = treeInputs.front().isRNTuple;
      const bool isMixed = std::any_of(treeInputs.begin(), treeInputs.end(), [isRNTuple](const tree_input_t& treeInput) { return treeInput.isRNTuple != isRNTuple; });
      if (isMixed) {
        logger::error("Cannot read {}: it is a TTree in some inputs and an RNTuple in others.", DataLocation(dataSource, treeName));
        continue;
      }
      if (isRNTuple) {
        const string& ntuplePath = treeInputs.front().treePath;
        auto other = std::find_if(treeInputs.begin(), treeInputs.end(), [&ntuplePath](const tree_input_t& treeInput) { return treeInput.treePath != ntuplePath; });
        if (other != treeInputs.end()) {
          logger::error("Cannot read RNTuple {}: ROOT chains RNTuples only under the same path, but it is {} in {} and {} in {}.", DataLocation(dataSource, treeName),
                        ntuplePath, treeInputs.front().file, other->treePath, other->file);
          continue;
        }
        vector<string> files;
        for (const auto& treeInput : treeInputs)
          files.push_back(treeInput.file);
        for (const auto& joins : joinGroups) {
          if (!joins.empty()) {
            logger::error("Cannot join {} to RNTuple {}: ROOT cannot join RNTuples.", joins.front().GetDescription(), DataLocation(dataSource, treeName));
            continue;
          }
          ProcessDataRequests("tree", dataSource, treeName, ":" + dataSource, [ntuplePath, files]() { return std::make_unique<ROOT::RDataFrame>(ntuplePath, files); }, nullptr, describeInputs(treeInputs), joins);
        }
        continue;
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
            logger::error("Cannot join {} to tree {}: {}.", join.GetDescription(), DataLocation(dataSource, treeName), *error);
            failedJoins.insert(join.ToString());
            isJoined = false;
          }
          if (!isJoined) break;
        }
        if (!isJoined) continue;
        ProcessDataRequests("tree", dataSource, treeName, ":" + dataSource, [chain, joinedChains]() { return std::make_unique<ROOT::RDataFrame>(*chain); /* joinedChains: keeps the joined trees alive */ }, chain.get(), describeInputs(treeInputs), joins);
      }
    }
  }
}

//**************************************************************************************************
/**
 * Reference function the painter draws in a pad of a plot, as merged with its base plot in GeneratePlot:
 * the pad's own, else the one of the base plot's pad, else the one of pad 0 of the plot, else of pad 0 of the base plot.
 * It is drawn only in pads with data. An empty one (SetRefFunc("")) means none and hides those further down the list.
 */
//**************************************************************************************************
shared_ptr<Plot::Pad::Data> PlotManager::RefFuncDrawnIn(Plot& plot, uint8_t padID)
{
  auto padIt = plot.GetPads().find(padID);
  if (padID == 0 || padIt == plot.GetPads().end() || padIt->second.GetData().empty()) return nullptr;
  Plot* basePlot = nullptr;
  if (plot.GetBasePlotName()) {
    auto it = std::find_if(mBasePlots.begin(), mBasePlots.end(), [&](const auto& curBasePlot) { return *plot.GetBasePlotName() == curBasePlot.GetName(); });
    if (it != mBasePlots.end()) basePlot = &*it;
  }
  auto refFuncOf = [](Plot* curPlot, uint8_t curPadID) -> shared_ptr<Plot::Pad::Data> {
    if (!curPlot) return nullptr;
    auto curPadIt = curPlot->GetPads().find(curPadID);
    return (curPadIt != curPlot->GetPads().end()) ? curPadIt->second.GetRefFunc() : nullptr;
  };
  for (const uint8_t curPadID : {padID, uint8_t{0}}) {
    if (auto refFunc = refFuncOf(&plot, curPadID)) return refFunc->GetName().empty() ? nullptr : refFunc;
    if (auto refFunc = refFuncOf(basePlot, curPadID)) return refFunc->GetName().empty() ? nullptr : refFunc;
  }
  return nullptr;
}

//**************************************************************************************************
/**
 * Determine which of a plot's required data entries are missing from the buffer.
 */
//**************************************************************************************************
vector<tuple<string, string, Plot::Pad::Data::data_info_t>> PlotManager::GetMissingData(Plot& plot)
{
  vector<tuple<string, string, Plot::Pad::Data::data_info_t>> missing;
  auto checkKey = [&](const string& dataSource, const string& name, const Plot::Pad::Data::data_info_t& dataInfo) {
    auto sourceIt = mDataBuffer.find(dataSource);
    if (sourceIt != mDataBuffer.end()) {
      auto nameIt = sourceIt->second.find(name + dataInfo.GetNameSuffix());
      if (nameIt != sourceIt->second.end() && nameIt->second) return;  // found and not null
    }
    missing.emplace_back(dataSource, name, dataInfo);
  };

  for (auto& [padID, pad] : plot.GetPads()) {
    if (auto refFunc = RefFuncDrawnIn(plot, padID)) checkKey(refFunc->GetDataSource(), refFunc->GetName(), refFunc->GetDataInfo());
    if (padID != 0 && pad.GetFrame()) {
      checkKey(pad.GetFrame()->GetDataSource(), pad.GetFrame()->GetName(), pad.GetFrame()->GetDataInfo());
    }
    if (padID == 0) continue;  // data of pad 0 is not drawn (AddPlot warns about it), so it is not needed either
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
  const bool isInteractiveMode = (mode == "show" || mode == "spread");
  const bool isMacroMode = (mode == "macro");
  const bool isGifMode = str_starts_with(mode, "gif");

  if (plot.GetGroup().empty()) {
    logger::error("No group was specified for plot {}.", plot.GetName());
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
      if (mMissingBasePlots.insert(basePlotName).second) {
        logger::error("Could not find base plot {}: add it with AddBasePlot(PlotManager::MakeBasePlot(\"{}\")) or define it yourself.", basePlotName, basePlotName);
      }
      return false;
    }
  }
  fullPlot += plot;
  if (mode == "print") {
    logger::print("Settings of plot {}{}{} from group {}{}{}:", logger::begin_color(logger::Color::Green), fullPlot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), fullPlot.GetGroup(), logger::end_color());
    Plot::Print(fullPlot.GetPropertyTree(), "");
    return true;
  }

  // the personal scale settings only change the number of pixels of plots on screen and of bitmap files, vector files do not depend on them
  const bool isBitmapMode = (mode == "png") || (mode == "jpg") || isGifMode;
  PlotPainter painter(isInteractiveMode ? Config::Get().ScreenScale() : (isBitmapMode ? Config::Get().BitmapScale() : 1.), isInteractiveMode);
  gROOT->SetBatch(!isInteractiveMode && !isMacroMode);
  shared_ptr<TCanvas> canvas;
  auto sharedAxes = fullPlot.GetSharedAxes();
  // a pad named by ShareAxis that the plot does not define is left out (it would otherwise be created, covering the whole canvas)
  for (auto& [axisLabel, axisGroups] : sharedAxes) {
    for (auto& group : axisGroups) {
      auto undefined = [&, axisLabel = axisLabel](uint8_t padID) {
        if (fullPlot.GetPads().count(padID)) return false;
        logger::warning("Plot {}: pad {} is left out of the pads sharing the {} axis: the plot has no pad {}.", fullPlot.GetName(), padID, axisLabel, padID);
        return true;
      };
      group.pads.erase(std::remove_if(group.pads.begin(), group.pads.end(), undefined), group.pads.end());
    }
    axisGroups.erase(std::remove_if(axisGroups.begin(), axisGroups.end(), [](const auto& group) { return group.pads.size() < 2; }), axisGroups.end());
  }
  for (auto it = sharedAxes.begin(); it != sharedAxes.end();) {
    it = (it->second.empty()) ? sharedAxes.erase(it) : std::next(it);
  }
  // labels and titles a layout hides towards touching panels: shown where a panel has no flat frame, which is known after a first paint
  const auto& allPads = fullPlot.GetPads();
  const bool hidesLabels = std::any_of(allPads.begin(), allPads.end(), [](const auto& pad) { return !pad.second.GetHiddenNextTo().empty(); });
  if (sharedAxes.empty() && !hidesLabels) {
    canvas.reset(Paint(painter, fullPlot).release());
  } else if (!ApplySharedAxes(painter, fullPlot, sharedAxes, allPads, canvas)) {
    return false;
  }
  if (!canvas) return false;
  // ROOT writes images into PDF files only from version 6.42 on: before, they are missing there. Said once per run (a logo is usually
  // on every plot), instead of ROOT's own warning for every image, which is silenced when the file is written
  bool silenceNoPdfImages = false;
  if constexpr (ROOT_VERSION_CODE < ROOT_VERSION(6, 42, 0)) {
    static bool warnedNoPdfImages = false;
    auto hasImages = [](auto& pad) {
      return pad.first != 0 && !pad.second.GetImageBoxes().empty();  // pad 0 is not drawn
    };
    silenceNoPdfImages = mode == "pdf" && std::any_of(fullPlot.GetPads().begin(), fullPlot.GetPads().end(), hasImages);
    if (silenceNoPdfImages && !warnedNoPdfImages) {
      logger::warning("Images are missing in the pdf files: ROOT {} cannot write them there (from 6.42 on it can). Use eps or png for plots with images.", gROOT->GetVersion());
      warnedNoPdfImages = true;
    }
  }

  if (TColor::GetFreeColorIndex() > std::numeric_limits<int16_t>::max()) {
    // ROOT colour indices are of type short, which limits the number of custom colours
    logger::throw_out_of_range("Too many custom colours in one session.");
  }
  // if interactive mode is specified, open window instead of saving the plot
  if (isInteractiveMode) return BrowseInteractively(fullPlot, mode, canvas);

  return WriteOutput(plot, fullPlot, mode, canvas, isMacroMode, isGifMode, silenceNoPdfImages);
}

//**************************************************************************************************
/**
 * Logs that the plot was made.
 */
//**************************************************************************************************
void PlotManager::LogCreated(const Plot& plot)
{
  logger::log("Created plot {}{}{} from group {}{}{}.", logger::begin_color(logger::Color::Green), plot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), plot.GetGroup(), logger::end_color());
}

//**************************************************************************************************
/**
 * Shows the plot in a window instead of saving it: in mode spread next to the others, in mode show in the browsing window, where
 * the keys move to other plots until the window is closed.
 */
//**************************************************************************************************
bool PlotManager::BrowseInteractively(const Plot& fullPlot, const string& mode, shared_ptr<TCanvas>& canvas)
{
  LogCreated(fullPlot);
  if (mode == "spread") {
    OpenWindow(canvas);
    gSystem->ProcessEvents();  // draws the window, and the windows already open react while the next plot is made
    if (const char terminalKey = TerminalKey(); terminalKey == 'q' || terminalKey == 'x') {
      CloseOpenWindows();
      mExitInteractiveBrowsing = true;  // no further plots
    }
    return true;
  }
  KeepOnClose(canvas.get());
  // the plot is shown in the browsing window (where the previous plot was), which keeps it until browsing moves on
  size_t current = mBrowseIndex;
  // made before when browsing comes back to it past a plot that failed: replaced by the new one
  shared_ptr<TCanvas> previous = mBrowseCanvases[current];
  mBrowseCanvases[current] = canvas;
  if (mBrowsePosition) {
    ShowAt(canvas.get(), mBrowsePosition->first, mBrowsePosition->second);
  } else {
    canvas->Show();
  }
  if (previous) {
    Hide(previous.get());
    previous.reset();
  }
  if (mShownIndex && *mShownIndex != current && mBrowseCanvases[*mShownIndex]) Hide(mBrowseCanvases[*mShownIndex].get());
  mShownIndex = current;
  // the plots under the one shown, as on a stack: detaching the plot uncovers the one shown before it
  auto putOnTop = [this](size_t index) {
    mBrowseHistory.erase(std::remove(mBrowseHistory.begin(), mBrowseHistory.end(), index), mBrowseHistory.end());
    mBrowseHistory.push_back(index);
  };
  putOnTop(current);
  ShowDetached();
  bool boxClicked = false;
  // an event stays the canvas' current one until the next: the one a canvas has when it is shown again is old (e.g. the key that left it)
  auto oldEvent = CanvasEvent(canvas.get());
  // browsing moves on to plot target, at mBrowsePosition: shown at once if it was made already, otherwise GeneratePlots makes it (returns true)
  auto browseTo = [&](size_t target) {
    mBrowseIndex = target;
    if (!mBrowseCanvases[target]) return true;
    if (mShownIndex) Hide(canvas.get());
    current = target;
    canvas = mBrowseCanvases[current];
    ShowAt(canvas.get(), mBrowsePosition->first, mBrowsePosition->second);
    mShownIndex = current;
    putOnTop(current);
    oldEvent = CanvasEvent(canvas.get());
    ShowDetached();
    return false;
  };
  while (!gSystem->ProcessEvents() && gROOT->GetSelectedPad()) {
    bool closeAll = false;
    HandleOpenWindows(closeAll);
    const char terminalKey = TerminalKey();
    const bool newEvent = (CanvasEvent(canvas.get()) != oldEvent);
    const char windowKey = (newEvent && canvas->GetEvent() == kKeyPress) ? static_cast<char>(canvas->GetEventX()) : 0;
    const char key = (terminalKey) ? terminalKey : windowKey;
    if (closeAll || key == 'x' || terminalKey == 'q') {
      CloseOpenWindows();
      mExitInteractiveBrowsing = true;
      break;
    }
    if (IsClosed(canvas.get()) || windowKey == 'q') {
      mExitInteractiveBrowsing = true;
      break;
    }
    bool isClick = newEvent && canvas->GetEvent() == kButton1Double;
    bool isValidKey = (key == 'a' || key == 's');
    bool isDetachKey = (key == 'd');
    if (isDetachKey && *NextBrowseIndex(current, true) == current) {
      oldEvent = CanvasEvent(canvas.get());  // the last plot left to browse stays in the browsing window
    } else if (isDetachKey) {
      // the plot moves into a window of its own, one step right and down from the browsing window, and is in front;
      // the browsing window shows the plot shown before it (the next one if there was none). Window managers put the window mapped last in front:
      // the plot is hidden and shown in its window only after the browsing window shows the other plot (ShowDetached)
      const int32_t browseX = canvas->GetWindowTopX();
      const int32_t browseY = canvas->GetWindowTopY();
      Hide(canvas.get());
      mDetached = {canvas, {browseX, browseY}};
      mBrowsePosition = std::make_pair(browseX, browseY);
      const string uniqueName = canvas->GetTitle();  // <name>:<group>
      const size_t separator = uniqueName.rfind(':');
      logger::log("Detached plot {}{}{} from group {}{}{}.", logger::begin_color(logger::Color::Green), uniqueName.substr(0, separator), logger::end_color(), logger::begin_color(logger::Color::Yellow), uniqueName.substr(separator + 1), logger::end_color());
      mBrowseSkip[current] = true;  // no longer browsed
      mBrowseCanvases[current] = nullptr;
      mShownIndex.reset();
      mBrowseHistory.pop_back();
      mBrowseForward = true;
      if (browseTo((mBrowseHistory.empty()) ? *NextBrowseIndex(current, true) : mBrowseHistory.back())) break;
    } else if (auto report = (isClick) ? PositionReport(canvas->GetSelected(), canvas->GetSelectedPad()) : nullopt) {
      if (!boxClicked) logger::info("{}", *report);
      boxClicked = true;
    } else if (isClick || isValidKey) {
      // browsing goes round: after the last plot comes the first one, before the first one the last one (plots in between are not made)
      mBrowseForward = (isValidKey) ? (key == 's') : ((double_t)canvas->GetEventX() / (double_t)canvas->GetWw() > 0.5);
      auto target = NextBrowseIndex(current, mBrowseForward);
      if (target && *target != current) {
        mBrowsePosition = std::make_pair(canvas->GetWindowTopX(), canvas->GetWindowTopY());
        if (browseTo(*target)) break;
      } else {
        oldEvent = CanvasEvent(canvas.get());  // a single plot: nothing to change to
      }
    } else {
      boxClicked = false;
    }
    gSystem->Sleep(20);
  }
  return true;
}

//**************************************************************************************************
/**
 * Writes the plot to its file in the output directory, or a frame of the gif being made (mode file keeps the canvas for the ROOT file
 * written at the end of the run, mode data writes no file per plot).
 */
//**************************************************************************************************
bool PlotManager::WriteOutput(const Plot& plot, const Plot& fullPlot, const string& mode, shared_ptr<TCanvas>& canvas, bool isMacroMode, bool isGifMode,
                              bool silenceNoPdfImages)
{
  if (!CreateOutputDirectory()) return false;

  // a histogram drawn more than once in a pad (the axis histogram for the axes and again for the grid) is one object with several
  // entries in the pad: where the canvas itself is stored, each entry gets its own copy, since a macro would declare the object twice
  // (for 2D histograms this does not compile) and a canvas read back from a file and deleted without being drawn would delete it twice
  if (mode == "file" || mode == "macro" || mode == "root" || mode == "xml") {
    std::function<void(TPad*)> cloneRedrawnHistograms;
    cloneRedrawnHistograms = [&](TPad* pad) {
      set<TObject*> drawnObjects;
      int32_t nClones{};
      for (auto link = pad->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
        TObject* object = link->GetObject();
        if (auto subPad = dynamic_cast<TPad*>(object)) {
          cloneRedrawnHistograms(subPad);
        } else if (!drawnObjects.insert(object).second && object->InheritsFrom(TH1::Class())) {
          auto clone = static_cast<TH1*>(object->Clone((string(object->GetName()) + "_" + std::to_string(++nClones)).data()));
          clone->SetDirectory(nullptr);
          clone->SetBit(kCanDelete);
          clone->SetBit(kCannotPick);  // only repaints axes or grid: a click goes to the objects below it
          link->SetObject(clone);
        }
      }
    };
    cloneRedrawnHistograms(canvas.get());
  }
  // the setup of a pad (palette, text format, kept axis ranges ...) is a TExec before its frame; a macro draws them in that order, and the
  // frame drawn without SAME clears the pad, setup included: a copy drawn right before the pad in the pad holding it runs before it is painted
  if (isMacroMode) {
    std::function<void(TPad*)> copySetups;
    copySetups = [&](TPad* pad) {
      for (auto link = pad->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
        auto subPad = dynamic_cast<TPad*>(link->GetObject());
        if (!subPad) continue;
        copySetups(subPad);
        // not always the first primitive: a colour-scale cell gets it once its scale is drawn, and painting on a display puts the frame in front
        TExec* setup{nullptr};
        for (auto* primitive : *subPad->GetListOfPrimitives()) {
          if (primitive->InheritsFrom(TExec::Class()) && str_starts_with(primitive->GetName(), "setup_pad_")) {
            setup = static_cast<TExec*>(primitive);
            break;
          }
        }
        if (!setup) continue;
        auto copy = new TExec(setup->GetName(), setup->GetTitle());
        copy->SetBit(kCanDelete);
        pad->GetListOfPrimitives()->AddBefore(link, copy);
      }
    };
    copySetups(canvas.get());
  }

  if (mode == "file") {
    mCanvasRegistry[plot.GetUniqueName()] = canvas;
    LogCreated(fullPlot);
    return true;
  }
  if (mode == "data") {
    LogCreated(fullPlot);
    return true;
  }

  string frameSuffix = "+50";      // ROOT's syntax for a frame of an animated gif: +<centiseconds it is shown>
  string lastFrameSuffix = "+50";  // the last frame: +NN plays the animation once, ++NN++ loops endlessly, +NN++RR repeats it RR times

  // names that become C++ identifiers in macros must not contain special characters and must not be keywords (a plot named "auto")
  auto sanitize = [](string s) {
    for (auto& c : s) {
      if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') c = '_';
    }
    if (!s.empty() && std::isdigit(static_cast<unsigned char>(s.front()))) s = "_" + s;
    static const set<string> keywords = {
      "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"};
    // nor names ROOT's interpreter knows as a type (std::ratio, std::vector, TH1, ...) or a function of the C library (rand, abort, ...):
    // the macro function would be ambiguous; nor main, which must return int
    if (keywords.count(s) || s == "main" || gInterpreter->CheckClassInfo(s.data(), false, false) || gInterpreter->CheckClassTemplate(s.data()) ||
        gROOT->GetGlobalFunction(s.data(), nullptr, false)) {
      s += "_";
    }
    return s;
  };

  string fileEnding;
  if (isMacroMode) {
    fileEnding = ".C";
    // ROOT converts the names of all drawn objects to valid variable names, but not the one of the canvas itself
    canvas->SetName(sanitize(canvas->GetName()).data());
  } else if ((mode == "pdf") || (mode == "png") || (mode == "eps") || (mode == "svg") || (mode == "ps") || (mode == "html") || (mode == "json") || (mode == "xml") || (mode == "jpg") || (mode == "root")) {
    fileEnding = "." + mode;
  } else if (isGifMode) {
    fileEnding = ".gif";
    if (mode.size() > 4) {  // gif+<centiseconds>[++[<repeats>]], checked by is_valid_plot_mode
      const size_t loopPos = mode.find("++");
      const string delay = mode.substr(4, (loopPos == string::npos) ? string::npos : loopPos - 4);
      frameSuffix = "+" + delay;
      lastFrameSuffix = frameSuffix;
      if (loopPos != string::npos) {
        const string repeats = mode.substr(loopPos + 2);
        lastFrameSuffix = repeats.empty() ? "++" + delay + "++" : "+" + delay + "++" + repeats;
      }
    }
  }

  if (fileEnding.empty()) {
    logger::error("No valid output format was specified. Cannot save plot.");
    return false;
  }

  string fileName = plot.GetName();
  // ROOT names the macro function after the file, so it must be a valid identifier as well
  if (isMacroMode) fileName = sanitize(fileName);

  // create output folders and files
  string folderName = mOutputDirectory + "/" + plot.GetGroup();
  string fullName = folderName + "/" + fileName + fileEnding;
  // names that would give the same file, as the sanitised macro names (eff-pi and eff_pi) or names differing only in case on a
  // case-insensitive file system (Eff and eff, the default of macOS): the later plot gets a number, so it does not replace the earlier one
  if (!(isGifMode && !mGifName.empty())) {
    const string plotID = plot.GetGroup() + ":" + plot.GetName();
    auto fileKey = [&](const string& name) {
      string key = folderName + "/" + name + fileEnding;
      std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return key;
    };
    string uniqueName = fileName;
    for (int32_t n = 2; mWrittenFiles.count(fileKey(uniqueName)) && mWrittenFiles[fileKey(uniqueName)] != plotID; ++n) {
      uniqueName = fileName + "_" + std::to_string(n);
    }
    if (uniqueName != fileName) {
      logger::warning("Plot {} of group {} is saved as {}{}, since {} has the same file name.", plot.GetName(), plot.GetGroup(), uniqueName, fileEnding, mWrittenFiles[fileKey(fileName)]);
      fileName = uniqueName;
      fullName = folderName + "/" + fileName + fileEnding;
    }
    mWrittenFiles[fileKey(fileName)] = plotID;
  }

  if (isGifMode && !mGifName.empty()) {
    fullName = mGifName;
    folderName = std::filesystem::path(mGifName).parent_path().string();
    fullName += (&plot == mGifLastFrame) ? lastFrameSuffix : frameSuffix;
    // ROOT appends every frame at its own size, but the gif has the size of the first: viewers cut larger frames
    const std::pair<uint32_t, uint32_t> size{canvas->GetWw(), canvas->GetWh()};
    if (!mGifSize) {
      mGifSize = size;
    } else if (size != *mGifSize && !mGifSizeReported) {
      logger::warning("The frames of {} differ in size: plot {} of group {} is {}x{}, the first frame {}x{}. Viewers show every frame in the size of the first.", mGifName, plot.GetName(), plot.GetGroup(), size.first, size.second, mGifSize->first, mGifSize->second);
      mGifSizeReported = true;
    }
  }
  std::error_code ec;
  std::filesystem::create_directories(folderName, ec);
  if (ec) {
    logger::error("Could not create output directory {}: {}.", folderName, ec.message());
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
  if (mode == "root" || mode == "xml") ReserveColorIndices(canvas.get());
  // only ROOT's "PDF not implemented yet" of TASImage::Paint is dropped (SciRooPlot warned above), every other message passes
  static ErrorHandlerFunc_t rootErrorHandler = nullptr;
  if (silenceNoPdfImages) {
    rootErrorHandler = SetErrorHandler([](Int_t level, Bool_t abort, const char* location, const char* msg) {
      if (TString(location) == "TASImage::Paint" && TString(msg).Contains("PDF")) return;
      rootErrorHandler(level, abort, location, msg);
    });
  }
  auto errorHandlerGuard = make_scope_guard([silenceNoPdfImages]() {
    if (silenceNoPdfImages) SetErrorHandler(rootErrorHandler);
  });
  canvas->SaveAs(fullName.data());
  std::error_code writtenError;
  if (!isGifMode && !std::filesystem::is_regular_file(fullName, writtenError)) {
    logger::error("Could not write {}.", fullName);
    return false;
  }
  if (isMacroMode) RepairMacro(fullName);
  LogCreated(fullPlot);
  return true;
}

//**************************************************************************************************
/**
 * Paints the plot; an exception while painting (thrown by ROOT for data it cannot paint) loses only this plot, not the rest of the run.
 */
//**************************************************************************************************
std::unique_ptr<TCanvas> PlotManager::Paint(PlotPainter& painter, Plot& toPaint)
{
  try {
    return painter.GeneratePlot(toPaint, mDataBuffer);
  } catch (const std::exception& e) {
    logger::error("Plot {} could not be painted: {}.", toPaint.GetName(), e.what());
    return nullptr;
  }
}

//**************************************************************************************************
/**
 * Paints a plot with shared axes, or with labels a layout hides towards touching panels: a first paint gives the ranges the pads get
 * on their own, the plot is painted again with the common ones if a pad got another range (false: the first paint failed).
 */
//**************************************************************************************************
bool PlotManager::ApplySharedAxes(PlotPainter& painter, Plot& fullPlot, std::map<char, vector<Plot::shared_group_t>>& sharedAxes, const std::map<uint8_t, Plot::Pad>& allPads,
                                  shared_ptr<TCanvas>& canvas)
{
  // the colour axis shared by a z group is z, for a 3d histogram its contents (C): its range is shared, a log scale and time format are not
  // (ROOT colours the boxes linearly)
  set<uint8_t> contentPads;
  if (sharedAxes.count('Z')) {
    for (const auto& [padID, pad] : fullPlot.GetPads()) {
      if (padID != 0 && PlotPainter::IsContentScale(pad, mDataBuffer)) contentPads.insert(padID);
    }
  }
  auto axisOf = [&](char axisLabel, uint8_t padID) { return (axisLabel == 'Z' && contentPads.count(padID)) ? 'C' : axisLabel; };
  // pads sharing an axis: a log scale, time format or range set in one of them (else in pad 0) holds for all of them
  map<char, vector<vector<uint8_t>>> groups;
  for (const auto& [axisLabel, axisGroups] : sharedAxes) {
    for (const auto& group : axisGroups) {
      groups[axisLabel].push_back(group.pads);
      // contents: the setting of a z group holds for the contents of its 3d histograms too (their range, not a log scale or time format)
      auto common = [&, axisLabel = axisLabel](const char* what, auto get, auto set, bool contents) {
        optional<std::decay_t<decltype(*get(std::declval<const Plot::Pad::Axis&>()))>> value;
        uint8_t valuePad{};  // the first pad of the group setting it, whose value is used
        for (uint8_t padID : group.pads) {
          if (!contents && axisOf(axisLabel, padID) == 'C') continue;
          const auto& axes = fullPlot[padID].GetAxes();
          auto axis = axes.find(axisOf(axisLabel, padID));
          if (axis == axes.end() || !get(axis->second)) continue;
          if (!value) {
            value = get(axis->second);
            valuePad = padID;
          } else if (*value != *get(axis->second)) {
            logger::warning("Plot {}: the pads sharing the {} axis with pad {} set different {}: the one of pad {} is used.", fullPlot.GetName(), axisLabel, padID, what, valuePad);
          }
        }
        if (!value) return;
        for (uint8_t padID : group.pads) {
          if (contents || axisOf(axisLabel, padID) != 'C') set(fullPlot[padID][axisOf(axisLabel, padID)], *value);
        }
      };
      common("log scales", [](const Plot::Pad::Axis& axis) { return axis.GetLog(); }, [](Plot::Pad::Axis& axis, bool isLog) { axis.SetLog(isLog); }, false);
      common("time formats", [](const Plot::Pad::Axis& axis) { return axis.GetTimeFormat(); }, [](Plot::Pad::Axis& axis, const string& format) { axis.SetTimeFormat(format); }, false);
      common("minima", [](const Plot::Pad::Axis& axis) { return axis.GetMinRange(); }, [](Plot::Pad::Axis& axis, double_t min) { axis.SetMinRange(min); }, true);
      common("maxima", [](const Plot::Pad::Axis& axis) { return axis.GetMaxRange(); }, [](Plot::Pad::Axis& axis, double_t max) { axis.SetMaxRange(max); }, true);
      if (axisLabel == 'Z') {
        auto isLog = [&](uint8_t padID) {
          const auto& axes = fullPlot[padID].GetAxes();
          return !contentPads.count(padID) && axes.count('Z') && axes.at('Z').GetLog().value_or(false);
        };
        auto content = std::find_if(group.pads.begin(), group.pads.end(), [&](uint8_t padID) { return contentPads.count(padID) > 0; });
        if (content != group.pads.end() && std::any_of(group.pads.begin(), group.pads.end(), isLog)) {
          logger::warning("Plot {}: pad {} draws a 3d histogram, whose boxes ROOT colours linearly: they do not follow the log scale of its z group.", fullPlot.GetName(), *content);
        }
        // one colour scale for the group, but each pad is painted with its own palette: the scale fits only some of them
        const auto& pads = fullPlot.GetPads();
        auto ownPalette = [](const Plot::Pad& p) { return p.GetPalette() || p.GetPaletteGradient().rgbEndpoints; };
        // the palette the painter makes of the settings: a gradient (255 colours unless given) or a ROOT palette (kBird unless given), alpha 1 unless given
        auto palette = [](const Plot::Pad& p) {
          const auto& gradient = p.GetPaletteGradient();
          const bool isGradient = gradient.rgbEndpoints.has_value();
          return std::make_tuple((isGradient) ? 0 : p.GetPalette().value_or(kBird), gradient.rgbEndpoints, gradient.alpha.value_or(1.f), (isGradient) ? gradient.nColors.value_or(255) : 0);
        };
        const Plot::Pad* first{nullptr};
        uint8_t firstPad{};
        for (uint8_t padID : group.pads) {
          auto pad = pads.find(padID);
          if (pad == pads.end()) continue;
          const Plot::Pad* used = &pad->second;  // the palette a pad is painted with: its own, else the one of pad 0
          const bool own = ownPalette(*used);
          if (!own && pads.count(0)) used = &pads.at(0);
          if (!first) {
            first = used;
            firstPad = padID;
          } else if (palette(*first) != palette(*used)) {
            // named: the pad that sets its own palette (the first one painted differently from the others may take pad 0's)
            logger::warning("Plot {}: pad {} sets another palette than the pads it shares one colour scale with; separate the panels with a gap for own scales.", fullPlot.GetName(), (own) ? padID : firstPad);
            break;
          }
        }
      }
    }
  }
  painter.SetSharedAxes(groups);
  // the automatic ranges: the union of the ranges the pads get on their own (with their room), known after a first paint;
  // the plot is painted again only if a pad got another range than the union
  Plot firstPlot = fullPlot;  // painting changes the plot (e.g. the legend rows of the data)
  std::unique_ptr<TCanvas> firstCanvas = Paint(painter, firstPlot);
  if (!firstCanvas) return false;
  const auto ranges = painter.GetFrameRanges();
  set<std::pair<uint8_t, char>> commonRanges;
  bool paintAgain{};
  for (const auto& [axisLabel, axisGroups] : sharedAxes) {
    for (const auto& group : axisGroups) {
      vector<uint8_t> members;
      double_t min = std::numeric_limits<double_t>::infinity();
      double_t max = -min;
      for (uint8_t padID : group.pads) {
        auto range = ranges.find({padID, axisLabel});
        if (range == ranges.end()) {
          if (!group.fromLayout && fullPlot.GetPads().count(padID)) {
            auto& pad = fullPlot.GetPads().at(padID);
            const bool drawsNothing = pad.GetData().empty() && !pad.GetFrame() && !pad.GetRefFunc();
            logger::warning("Plot {}: pad {} is left out of the pads sharing the {} axis: {}.", fullPlot.GetName(), padID, axisLabel,
                            (drawsNothing) ? "it draws nothing" : ((axisLabel == 'Z') ? "it draws no 2d histogram, graph or function and no 3d histogram (or a polar drawing)" : "it has no flat axis frame (a 3d view or a polar drawing)"));
          }
          continue;
        }
        members.push_back(padID);
        min = std::min(min, range->second.first);
        max = std::max(max, range->second.second);
      }
      if (members.size() < 2) continue;
      for (uint8_t padID : members) {
        const auto& [padMin, padMax] = ranges.at({padID, axisLabel});
        if (!TMath::AreEqualRel(padMin, min, 1e-9) || !TMath::AreEqualRel(padMax, max, 1e-9)) paintAgain = true;
        const char padAxis = (axisLabel == 'Z' && painter.GetContentScales().count(padID)) ? 'C' : axisLabel;
        Plot::Pad::Axis& axis = fullPlot[padID][padAxis];
        const auto& defaults = fullPlot[0].GetAxes();
        const bool defaultMin = defaults.count(padAxis) && defaults.at(padAxis).GetMinRange();
        const bool defaultMax = defaults.count(padAxis) && defaults.at(padAxis).GetMaxRange();
        if (!axis.GetMinRange() && !defaultMin) axis.SetMinRange(min);
        if (!axis.GetMaxRange() && !defaultMax) axis.SetMaxRange(max);
        commonRanges.insert({padID, padAxis});
      }
    }
  }
  // a 3D view or polar drawing shows the labels and title the layout hides (painted again); a flat panel beside one keeps them hidden,
  // though the axis is not shared: it has no room for them
  const auto flatPads = painter.GetFlatPads();
  auto isFlat = [&](uint8_t padID) { return flatPads.count(padID) > 0; };
  for (const auto& [padID, pad] : allPads) {
    for (const auto& [axisLabel, touching] : pad.GetHiddenNextTo()) {
      if (!isFlat(padID)) {
        paintAgain = true;
        continue;
      }
      // (a touching panel that draws nothing has no frame either)
      auto drawsView = [&](uint8_t other) {
        auto found = allPads.find(other);
        return !isFlat(other) && found != allPads.end() && (!found->second.GetData().empty() || found->second.GetFrame());
      };
      auto unshared = std::find_if(touching.begin(), touching.end(), drawsView);
      const auto axis = pad.GetAxes().find(axisLabel);
      const bool ownLabels = axis != pad.GetAxes().end() && axis->second.GetLabelSize();
      if (unshared != touching.end() && !ownLabels) {
        logger::warning("Plot {}: the {} labels and title of pad {} are hidden beside pad {}, which shares no axis with it (a 3D view or a polar drawing); separate them with a Gap() to show them.",
                        fullPlot.GetName(), static_cast<char>(std::tolower(axisLabel)), padID, *unshared);
      }
    }
  }
  if (paintAgain) painter.SetFlatPads(flatPads);
  if (!paintAgain) {
    canvas.reset(firstCanvas.release());
  } else {
    // the messages of the first paint are not repeated
    firstCanvas.reset();
    painter.SetSharedAxes(groups, commonRanges);
    auto quiet = ignore_root_messages_below(kError);
    logger::muted() = true;
    auto unmute = make_scope_guard([]() { logger::muted() = false; });
    canvas.reset(Paint(painter, fullPlot).release());
  }
  return true;
}

//**************************************************************************************************
/**
 * Adds to a macro written by ROOT what its SavePrimitive leaves out: the max digits of an axis, which ROOT writes into SetNdivisions
 * (n / 1000000) but drops when the macro calls it, so the divisions are no longer scaled by the pad width (crowded labels in narrow
 * panels), and line colour 1 of histograms, which ROOT leaves out as default while a histogram made by the macro takes the colour of the
 * style (black histograms came back blue). The added lines change nothing where ROOT restores both itself.
 */
//**************************************************************************************************
void PlotManager::RepairMacro(const string& path)
{
  vector<string> lines;
  {
    std::ifstream in(path);
    for (string line; std::getline(in, line);)
      lines.push_back(line);
  }
  static const std::regex newHist(R"(^\s*(TH[123][A-Z]\w*|TProfile\w*)\s*\*\s*(\w+)\s*=\s*new\s)");
  static const std::regex divisions(R"(^(\s*)(\S+->Get[XYZ]axis\(\))->SetNdivisions\((\d+)\);\s*$)");
  auto indent = [](const string& line) { return line.substr(0, line.find_first_not_of(' ')); };
  set<string> withoutColor;
  std::smatch match;
  for (const auto& line : lines) {
    if (line.find(" = new ") != string::npos && std::regex_search(line, match, newHist)) withoutColor.insert(match[2]);
  }
  for (const auto& line : lines) {
    const size_t call = line.find("->SetLineColor(");
    if (call != string::npos) withoutColor.erase(line.substr(indent(line).size(), call - indent(line).size()));
  }
  std::ofstream out(path);
  string made;  // a histogram without line colour, whose constructor statement has not ended yet
  string madeIndent;
  for (const auto& line : lines) {
    out << line << '\n';
    if (line.find(" = new ") != string::npos && std::regex_search(line, match, newHist) && withoutColor.count(match[2])) {
      made = match[2];
      madeIndent = indent(line);
    }
    // right after the statement making it: ROOT writes loops without braces, so not before the first call on it
    if (!made.empty() && !line.empty() && line.back() == ';' && indent(line) == madeIndent) {
      out << madeIndent << made << "->SetLineColor(1);\n";
      made.clear();
    }
    if (line.find("->SetNdivisions(") != string::npos && std::regex_match(line, match, divisions) && std::stoll(match[3]) >= 1000000) {
      out << match[1] << match[2] << "->SetMaxDigits(" << std::stoll(match[3]) / 1000000 << ");\n";
    }
  }
  if (!out) logger::warning("Could not complete the macro {}.", path);
}

//**************************************************************************************************
/**
 * Windows closed by the user are only hidden and marked as closed: the canvases stay owned by SciRooPlot, which deletes them.
 * (By default ROOT deletes a closed canvas itself.) The mark is the unique ID, which canvases do not use otherwise.
 */
//**************************************************************************************************
void PlotManager::KeepOnClose(TCanvas* canvas)
{
  canvas->SetUniqueID(0);
  if (auto rc = dynamic_cast<TRootCanvas*>(canvas->GetCanvasImp())) {
    rc->DontCallClose();
    rc->Connect("CloseWindow()", "TCanvas", canvas, "SetUniqueID(=1)");
    rc->Connect("CloseWindow()", "TRootCanvas", rc, "UnmapWindow()");
  }
}
bool PlotManager::IsClosed(const TCanvas* canvas)
{
  return canvas->GetUniqueID() == 1;
}
void PlotManager::Close(TCanvas* canvas)
{
  canvas->SetUniqueID(1);
  Hide(canvas);
}
void PlotManager::Hide(TCanvas* canvas)
{
  if (auto rc = dynamic_cast<TRootCanvas*>(canvas->GetCanvasImp())) rc->UnmapWindow();
}
void PlotManager::CloseOpenWindows()
{
  for (auto& window : mOpenWindows) {
    if (!IsClosed(window.canvas.get())) Close(window.canvas.get());
  }
}
// an event stays the canvas' current one until the next: comparing with the one handled tells whether there is a new one
PlotManager::canvas_event_t PlotManager::CanvasEvent(const TCanvas* canvas)
{
  return {canvas->GetEvent(), canvas->GetEventX(), canvas->GetEventY()};
}

//**************************************************************************************************
/**
 * Shows a window with the top left corner of its drawing area at the given screen position (as GetWindowTopX/Y give it).
 * Window managers add a decoration and may place a window by their own rules when it is mapped: the window is moved until it is
 * where it should be (at most three corrections, waiting at most 0.5 s for the window to be mapped).
 */
//**************************************************************************************************
void PlotManager::ShowAt(TCanvas* canvas, int32_t topX, int32_t topY) const
{
  int32_t x = topX;
  int32_t y = topY - mWindowOffsetY;  // first guess, SetWindowPosition places the window frame
  canvas->SetWindowPosition(x, y);
  canvas->Show();
  auto canvasImp = dynamic_cast<TRootCanvas*>(canvas->GetCanvasImp());
  for (int32_t i = 0; i < 25 && canvasImp && !canvasImp->IsMapped(); ++i) {
    gSystem->ProcessEvents();
    gSystem->Sleep(20);
  }
  gSystem->ProcessEvents();
  for (int32_t correction = 0; correction < 3; ++correction) {
    const int32_t shownX = canvas->GetWindowTopX();
    const int32_t shownY = canvas->GetWindowTopY();
    if (shownX == topX && shownY == topY) break;
    x += topX - shownX;
    y += topY - shownY;
    canvas->SetWindowPosition(x, y);
    // the window manager moves the window asynchronously: wait until it has (at most 0.2 s)
    for (int32_t i = 0; i < 10 && canvas->GetWindowTopX() == shownX && canvas->GetWindowTopY() == shownY; ++i) {
      gSystem->Sleep(20);
      gSystem->ProcessEvents();
    }
  }
}

//**************************************************************************************************
/**
 * Opens a window that stays open until the user closes it. The windows are placed as a cascade, each shifted by one step to the right
 * and down from where the previous one is, so the titles of the ones below stay visible; the first one is placed in the top left
 * corner of the screen (as far as the window manager allows, e.g. below a panel), or one step from the window at followTop (the top
 * left corner of its drawing area, as GetWindowTopX/Y give it). Without cascade, the window is always placed one step from followTop. A window reaching beyond the bottom of the screen starts a new cascade
 * at the height of the first window (shifted to the right), one beyond the right edge starts at its left edge. The cascade follows
 * where the windows actually are.
 */
//**************************************************************************************************
void PlotManager::OpenWindow(shared_ptr<TCanvas> canvas, optional<std::pair<int32_t, int32_t>> followTop, bool cascade)
{
  KeepOnClose(canvas.get());
  constexpr int32_t step = 30;
  auto previous = std::find_if(mOpenWindows.rbegin(), mOpenWindows.rend(), [](const auto& window) { return !IsClosed(window.canvas.get()); });
  if (cascade && previous != mOpenWindows.rend()) followTop = std::make_pair(previous->canvas->GetWindowTopX(), previous->canvas->GetWindowTopY());
  if (!followTop) {
    // the first window goes to the top left corner, where the window manager allows it (e.g. below a panel)
    canvas->SetWindowPosition(0, 0);
    canvas->Show();
    const auto event = CanvasEvent(canvas.get());
    mOpenWindows.push_back({std::move(canvas), event});
    return;
  }
  int32_t topX = followTop->first + step;
  int32_t topY = followTop->second + step;
  auto first = std::find_if(mOpenWindows.begin(), mOpenWindows.end(), [](const auto& window) { return !IsClosed(window.canvas.get()); });
  if (first != mOpenWindows.end()) {
    if (topY + static_cast<int32_t>(canvas->GetWindowHeight()) > static_cast<int32_t>(gClient->GetDisplayHeight())) topY = first->canvas->GetWindowTopY();
    if (topX + static_cast<int32_t>(canvas->GetWindowWidth()) > static_cast<int32_t>(gClient->GetDisplayWidth())) topX = first->canvas->GetWindowTopX();
  }
  ShowAt(canvas.get(), topX, topY);
  const auto event = CanvasEvent(canvas.get());  // e.g. the key that detached it
  mOpenWindows.push_back({std::move(canvas), event});
}

//**************************************************************************************************
/**
 * Shows the plot detached last in its own window, one step right and down from where the browsing window was.
 */
//**************************************************************************************************
void PlotManager::ShowDetached()
{
  if (!mDetached) return;
  auto [canvas, browseTop] = *mDetached;
  mDetached.reset();
  OpenWindow(canvas, browseTop, false);
}

//**************************************************************************************************
/**
 * The plot browsing moves on to from plot index (circular, skipping detached plots and those that could not be made).
 */
//**************************************************************************************************
optional<size_t> PlotManager::NextBrowseIndex(size_t index, bool forward) const
{
  const size_t nPlots = mBrowseSkip.size();
  for (size_t step = 1; step <= nPlots; ++step) {
    const size_t candidate = (forward) ? (index + step) % nPlots : (index + nPlots - step) % nPlots;
    if (!mBrowseSkip[candidate]) return candidate;
  }
  return nullopt;
}

//**************************************************************************************************
/**
 * The line a double-click in show mode prints for a legend, text, statistics box, image or shape: what was clicked, named as the user
 * added it ("Pad 1, arrow 3": the nth of its kind in the pad), in yellow, and its
 * position in green, written in brackets as the arguments of the call that sets it (AddText(x, y, ...), AddArrow(pos1, pos2), ...) in the
 * language of the project's program and in the coordinates the user gave, so that it can be copied into the code.
 * Nothing for other objects (a click there browses).
 */
//**************************************************************************************************
optional<string> PlotManager::PositionReport(TObject* selected, TVirtualPad* pad) const
{
  if (!selected || !pad) return nullopt;
  // an image is drawn in a sub-pad of the pad it belongs to
  TVirtualPad* imagePad = (str_starts_with(pad->GetName(), "ImageBox_") && pad->GetMother() != pad) ? pad : nullptr;
  if (imagePad) pad = imagePad->GetMother();
  const bool isPython = str_ends_with(Config::Get().Program(mProjectName), ".py");
  // always with the pad, so that the line reads the same in every plot
  const string padName = pad->GetName();
  const string padLabel = (str_starts_with(padName, "Pad_")) ? "Pad " + padName.substr(4) + ", " : "";
  // axis values from pad coordinates (log10 on a log axis), fractions of the pad from axis values
  auto axisValue = [&](double_t fraction, bool isX) {
    const double_t lo = (isX) ? pad->GetX1() : pad->GetY1(), hi = (isX) ? pad->GetX2() : pad->GetY2();
    const double_t padCoord = lo + fraction * (hi - lo);
    return ((isX) ? pad->GetLogx() : pad->GetLogy()) ? std::pow(10., padCoord) : padCoord;
  };
  auto fraction = [&](double_t value, bool isX) {
    const double_t lo = (isX) ? pad->GetX1() : pad->GetY1(), hi = (isX) ? pad->GetX2() : pad->GetY2();
    const double_t padCoord = ((isX) ? pad->GetLogx() : pad->GetLogy()) ? std::log10(value) : value;
    return (padCoord - lo) / (hi - lo);
  };
  // a position in NDC in the coordinate system it was given in: fractions of the pad or the frame, or axis values
  const auto frame = PlotPainter::FrameInPad(pad);
  auto given = [&](double_t ndc, bool isX, coordinates_t coordinates) {
    if (coordinates == user_coordinates) return axisValue(ndc, isX);
    if (coordinates == frame_coordinates) return (isX) ? (ndc - frame[0]) / (frame[2] - frame[0]) : (ndc - frame[1]) / (frame[3] - frame[1]);
    return ndc;
  };
  // the coordinate system a box was drawn with (TObject::SetUniqueID)
  auto boxCoordinates = [](uint32_t uniqueID) { return (uniqueID <= static_cast<uint32_t>(user_coordinates)) ? static_cast<coordinates_t>(uniqueID) : pad_coordinates; };
  // fractions with 3 digits, axis values with 4
  auto number = [](double_t value, coordinates_t coordinates) { return (coordinates == user_coordinates) ? fmt::format("{:.4g}", value) : fmt::format("{:.3g}", value); };
  // a hint at the content: the start of the first line or row
  auto excerpt = [](string first, bool more) {
    if (first.size() > 20) {
      size_t cut = 19;
      while (cut > 0 && (static_cast<unsigned char>(first[cut]) & 0xC0) == 0x80)
        --cut;  // not inside a UTF-8 character
      first.resize(cut);
      more = true;
    }
    return fmt::format(" \"{}{}\"", first, (more) ? "…" : "");
  };
  auto line = [&](const string& what, const string& position) {
    return fmt::format("{}{}{}{}: {}({}){}", logger::begin_color(logger::Color::Yellow), padLabel, what, logger::end_color(), logger::begin_color(logger::Color::Green), position, logger::end_color());
  };

  if (imagePad) {
    // the upper left corner, as AddImage(x, y, ...) and SetPosition(x, y) take it, and the file name as hint
    const auto coordinates = boxCoordinates(imagePad->GetUniqueID());
    const double_t x = given(imagePad->GetXlowNDC(), true, coordinates);
    const double_t y = given(imagePad->GetYlowNDC() + imagePad->GetHNDC(), false, coordinates);
    const string what = "image " + string(imagePad->GetName()).substr(9) + excerpt(std::filesystem::path(imagePad->GetTitle()).filename().string(), false);
    return line(what, number(x, coordinates) + ", " + number(y, coordinates));
  }

  if (auto* box = dynamic_cast<TPave*>(selected)) {
    const string name = box->GetName();
    string what;
    if (auto* legend = dynamic_cast<TLegend*>(box); legend && str_starts_with(name, "LegendBox_")) {
      what = "legend " + name.substr(10);
      TList* rows = legend->GetListOfPrimitives();
      if (rows && rows->GetSize() > 0) what += excerpt(static_cast<TLegendEntry*>(rows->First())->GetLabel(), rows->GetSize() > 1);
    } else if (auto* text = dynamic_cast<TPaveText*>(box); text && str_starts_with(name, "TextBox_")) {
      what = "text " + name.substr(8);
      if (text->GetSize() > 0) what += excerpt(text->GetLine(0)->GetTitle(), text->GetSize() > 1);
    } else if (str_starts_with(name, "stats_")) {
      what = "statistics box of data " + name.substr(6);
    } else {
      return nullopt;  // e.g. the colour scale
    }
    // the upper left corner, as AddText(x, y, ...), AddLegend(x, y) and SetPosition(x, y) take it
    const auto coordinates = boxCoordinates(box->GetUniqueID());
    const double_t x = given(box->GetX1NDC(), true, coordinates);
    const double_t y = given(box->GetY2NDC(), false, coordinates);
    return line(what, number(x, coordinates) + ", " + number(y, coordinates));
  }

  using Shape = Plot::Pad::Shape;
  using kind_t = Shape::kind_t;
  const auto id = Shape::DecodeDrawnID(selected->GetUniqueID());
  if (!id) return nullopt;
  // positions as given (axis values or fractions of the pad or the frame), points as the user's language writes a pair
  const bool isUser = (id->coordinates == user_coordinates);
  auto nx = [&](double_t x) { return number((isUser) ? x : given(fraction(x, true), true, id->coordinates), id->coordinates); };
  auto ny = [&](double_t y) { return number((isUser) ? y : given(fraction(y, false), false, id->coordinates), id->coordinates); };
  auto point = [&](double_t x, double_t y) { return (isPython) ? "(" + nx(x) + ", " + ny(y) + ")" : "{" + nx(x) + ", " + ny(y) + "}"; };
  string position;
  if (auto* drawnLine = dynamic_cast<TLine*>(selected)) {  // also arrows
    if (id->kind == kind_t::vertical_line) {
      position = nx(drawnLine->GetX1());
    } else if (id->kind == kind_t::horizontal_line) {
      position = ny(drawnLine->GetY1());
    } else {
      position = point(drawnLine->GetX1(), drawnLine->GetY1()) + ", " + point(drawnLine->GetX2(), drawnLine->GetY2());
    }
  } else if (auto* region = dynamic_cast<TBox*>(selected)) {
    if (id->kind == kind_t::region_x) {
      position = nx(region->GetX1()) + ", " + nx(region->GetX2());
    } else if (id->kind == kind_t::region_y) {
      position = ny(region->GetY1()) + ", " + ny(region->GetY2());
    } else {
      position = point(region->GetX1(), region->GetY1()) + ", " + point(region->GetX2(), region->GetY2());
    }
  } else if (auto* ellipse = dynamic_cast<TEllipse*>(selected)) {
    // the radii are in pad coordinates: as fractions of the pad (or the frame) divided by its extent
    const bool inFrame = (id->coordinates == frame_coordinates);
    const double_t rx = (isUser) ? ellipse->GetR1() : ellipse->GetR1() / (pad->GetX2() - pad->GetX1()) / ((inFrame) ? frame[2] - frame[0] : 1.);
    const double_t ry = (isUser) ? ellipse->GetR2() : ellipse->GetR2() / (pad->GetY2() - pad->GetY1()) / ((inFrame) ? frame[3] - frame[1] : 1.);
    position = point(ellipse->GetX1(), ellipse->GetY1()) + ", " + number(rx, id->coordinates) + ", " + number(ry, id->coordinates);
  } else if (auto* polygon = dynamic_cast<TPolyLine*>(selected)) {
    for (int32_t i = 0; i + 1 < polygon->GetN(); ++i) {  // the last point closes the polygon
      position += ((i) ? ", " : "") + point(polygon->GetX()[i], polygon->GetY()[i]);
    }
    position = (isPython) ? "[" + position + "]" : "{" + position + "}";
  } else {
    return nullopt;
  }
  return line(Shape::KindLabel(id->kind) + " " + std::to_string(id->number), position);
}

//**************************************************************************************************
/**
 * Handles the pending events of the windows kept open ('q' closes one, 'x' all of them, a double-click reports a position); returns whether any is still open.
 */
//**************************************************************************************************
bool PlotManager::HandleOpenWindows(bool& closeAll)
{
  bool anyOpen = false;
  for (auto& window : mOpenWindows) {
    TCanvas* canvas = window.canvas.get();
    if (IsClosed(canvas)) continue;
    if (auto event = CanvasEvent(canvas); event != window.handledEvent) {  // each event is handled once
      window.handledEvent = event;
      if (canvas->GetEvent() == kKeyPress) {
        const char key = static_cast<char>(canvas->GetEventX());
        if (key == 'q') {
          Close(canvas);
          continue;
        }
        if (key == 'x') closeAll = true;
      } else if (auto report = (canvas->GetEvent() == kButton1Double) ? PositionReport(canvas->GetSelected(), canvas->GetSelectedPad()) : nullopt) {
        logger::info("{}", *report);
      }
    }
    anyOpen = true;
  }
  return anyOpen;
}

//**************************************************************************************************
/**
 * Waits until the user has closed all windows kept open ('q' closes one, 'x' all of them).
 */
//**************************************************************************************************
void PlotManager::WaitForOpenWindows()
{
  const bool anyOpen = std::any_of(mOpenWindows.begin(), mOpenWindows.end(), [](const auto& window) { return !IsClosed(window.canvas.get()); });
  if (!anyOpen) {
    mOpenWindows.clear();
    return;
  }
  logger::info("Waiting until all windows are closed (key q closes one, x all of them; q or x in the terminal closes all).");
  while (!gSystem->ProcessEvents()) {
    bool closeAll = false;
    const char terminalKey = TerminalKey();
    if (!HandleOpenWindows(closeAll) || closeAll || terminalKey == 'q' || terminalKey == 'x') break;
    gSystem->Sleep(20);
  }
  mOpenWindows.clear();
}

//**************************************************************************************************
/**
 * Keys typed in the terminal steer the windows as well: while windows are open, the terminal delivers single keys without echo.
 * Only for a terminal of a process in the foreground (a process in the background reading the terminal would be stopped).
 * The settings are restored when the windows are closed, at exit, and on the signals that end the program (e.g. Ctrl-C).
 */
//**************************************************************************************************
static termios sSavedTerminal{};  // the settings before the keys were switched on (static: the signal handler restores them)
static bool sTerminalKeysOn{false};
static std::array<struct sigaction, 4> sSavedSignalActions{};
static constexpr std::array<int, 4> sTerminatingSignals{SIGINT, SIGTERM, SIGHUP, SIGQUIT};
void PlotManager::SetTerminalKeys(bool on)
{
  if (!on) {
    if (!sTerminalKeysOn) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &sSavedTerminal);
    sTerminalKeysOn = false;
    for (size_t i = 0; i < sTerminatingSignals.size(); ++i) {
      sigaction(sTerminatingSignals[i], &sSavedSignalActions[i], nullptr);
    }
    return;
  }
  if (sTerminalKeysOn || !isatty(STDIN_FILENO) || tcgetpgrp(STDIN_FILENO) != getpgrp() || tcgetattr(STDIN_FILENO, &sSavedTerminal) != 0) return;
  termios keys = sSavedTerminal;
  keys.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
  keys.c_cc[VMIN] = 0;  // read() returns at once, also without a key
  keys.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &keys) != 0) return;
  sTerminalKeysOn = true;
  static bool atExitRegistered{false};
  if (!atExitRegistered) {
    std::atexit([]() { SetTerminalKeys(false); });
    atExitRegistered = true;
  }
  struct sigaction restoreAndEnd{};
  restoreAndEnd.sa_handler = [](int signal) {
    tcsetattr(STDIN_FILENO, TCSANOW, &sSavedTerminal);
    std::signal(signal, SIG_DFL);
    std::raise(signal);
  };
  for (size_t i = 0; i < sTerminatingSignals.size(); ++i) {
    sigaction(sTerminatingSignals[i], &restoreAndEnd, &sSavedSignalActions[i]);
  }
}
char PlotManager::TerminalKey()
{
  char key{};
  if (!sTerminalKeysOn || tcgetpgrp(STDIN_FILENO) != getpgrp() || read(STDIN_FILENO, &key, 1) != 1) return 0;
  return key;
}

//**************************************************************************************************
/**
 * Show which plots are currently loaded in the framework.
 */
//**************************************************************************************************
void PlotManager::ListPlots() const
{
  for (const auto& plot : mPlots) {
    logger::print(" - {}{}{} in group {}{}{}", logger::begin_color(logger::Color::Green), plot.GetName(), logger::end_color(), logger::begin_color(logger::Color::Yellow), plot.GetGroup(), logger::end_color());
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
  mBrowseCanvases.clear();
}

//**************************************************************************************************
/**
 * Class of the object behind a key, if it can be read (objects of an emulated class without dictionary cannot be read as TObject).
 */
//**************************************************************************************************
TClass* PlotManager::ReadableClass(TKey* key)
{
  TClass* cl = TClass::GetClass(key->GetClassName());
  return (cl && cl->IsLoaded() && cl->IsTObject()) ? cl : nullptr;
}

//**************************************************************************************************
/**
 * Whether the object behind a key is of the given class (e.g. a directory or a tree), without reading it.
 */
//**************************************************************************************************
bool PlotManager::KeyInherits(TKey* key, const TClass* base)
{
  const TClass* cl = TClass::GetClass(key->GetClassName());
  return cl && cl->InheritsFrom(base);
}

//**************************************************************************************************
/**
 * The latest key of a name in a directory that holds a directory, folder or collection: a newer key of the same name holding data
 * does not hide it (open a directory found this way with OpenDirectory).
 */
//**************************************************************************************************
TKey* PlotManager::FindContainerKey(TDirectory* directory, const string& name)
{
  TKey* latest = directory->FindKey(name.data());
  if (!latest || is_container(ReadableClass(latest))) return latest;
  for (auto* key : TRangeDynCast<TKey>(directory->GetListOfKeys())) {  // the cycles of a name are listed from the latest
    if (key && name == key->GetName() && is_container(ReadableClass(key))) return key;
  }
  return latest;
}

//**************************************************************************************************
/**
 * The sub-directory behind a key of a directory: by its name (the instance ROOT keeps in memory), by name and cycle only where a newer
 * key of the name hides it from a lookup by name (ROOT then reads it again).
 */
//**************************************************************************************************
TDirectory* PlotManager::OpenDirectory(TDirectory* directory, TKey* key)
{
  const bool isLatest = (directory->FindKey(key->GetName()) == key);
  return directory->GetDirectory(isLatest ? key->GetName() : fmt::format("{};{}", key->GetName(), key->GetCycle()).data());
}

//**************************************************************************************************
/**
 * Delete a list or folder read from a file with everything in it. Each object is deleted exactly once, also if it is in several lists
 * (ROOT restores objects shared by several lists as shared), independent of the ownership settings of the collections.
 */
//**************************************************************************************************
void PlotManager::container_deleter_t::operator()(TObject* container) const
{
  vector<TObject*> objects;
  set<TObject*> found;
  std::function<void(TObject*)> collect = [&](TObject* obj) {
    if (!found.insert(obj).second) return;
    objects.push_back(obj);
    if (dynamic_cast<TClonesArray*>(obj) || dynamic_cast<TRefArray*>(obj)) return;  // own their content in a special way, do not own references
    if (auto* stack = dynamic_cast<THStack*>(obj)) {
      // a stack does not delete its histograms
      if (stack->GetHists()) {
        for (TObject* hist : *stack->GetHists())
          collect(hist);
      }
      return;
    }
    if (auto* multiGraph = dynamic_cast<TMultiGraph*>(obj)) {
      // a multigraph deletes its graphs: they are taken out of it and deleted with the rest
      if (auto* graphs = multiGraph->GetListOfGraphs()) {
        for (TObject* graph : *graphs)
          collect(graph);
        graphs->Clear("nodelete");
      }
      return;
    }
    if (auto* map = dynamic_cast<TMap*>(obj)) {
      for (TObject* item : *map) {
        if (auto* pair = dynamic_cast<TPair*>(item)) {
          if (pair->Key()) collect(pair->Key());
          if (pair->Value()) collect(pair->Value());
        }
      }
      map->SetOwnerKeyValue(false, false);
      map->Clear();  // deletes the pairs only
      return;
    }
    TCollection* items = (dynamic_cast<TFolder*>(obj)) ? static_cast<TFolder*>(obj)->GetListOfFolders() : dynamic_cast<TCollection*>(obj);
    if (!items) return;
    for (TObject* item : *items) {
      if (item) collect(item);
    }
    items->SetOwner(false);
    items->Clear("nodelete");
  };
  collect(container);
  for (TObject* obj : objects) {
    delete obj;
  }
}

//**************************************************************************************************
/**
 * Recursively reads data from a directory, folder or collection and adds it to the data buffer. Found dataNames are removed from the set.
 * The first match is the one in the current level; deeper levels are searched only for names not found there.
 * Nothing is taken out of the containers searched (data is read from a file key or copied), so they can be searched again for other names.
 * Lists and folders read from the file are deleted after searching them, unless they are in readCache (then they live as long as the input file).
 */
//**************************************************************************************************
void PlotManager::ReadData(TObject* folder, set<string>& dataNames, const string& prefix, const string& suffix, const string& dataSource, read_cache_t& readCache)
{
  // store data found (owned: read from a file key, otherwise it belongs to an in-memory container and is copied)
  auto store = [&](TObject* obj, const string& dataName, bool isOwned) {
    dataNames.erase(dataName);
    const string fullName = prefix + dataName;
    if (auto* tree = dynamic_cast<TTree*>(obj)) {
      mDataBuffer[dataSource][fullName].reset(nullptr);
      if (mTreeInputs[dataSource].find(fullName) != mTreeInputs[dataSource].end()) {
        logger::warning("Ignoring tree {} (data source {}) in a list: a tree stored in a list cannot be chained with the same tree in other inputs.", fullName, dataSource);
      } else {
        // trees in lists live in memory only, which RDataFrame can process only single-threaded
        auto mtGuard = disable_implicit_mt();
        ProcessDataRequests("tree", dataSource, fullName, suffix, [tree]() { return std::make_unique<ROOT::RDataFrame>(*tree); }, tree);
      }
      if (isOwned) delete tree;
      return;
    }
    // requests (Project, Profile, Scatter) need a tree or table: otherwise the data would only be reported missing
    if (auto infos = mDataInfoBuffer[dataSource].find(fullName); infos != mDataInfoBuffer[dataSource].end()) {
      for (const auto& info : infos->second) {
        if (info.dataDims.empty()) continue;
        logger::error("Cannot read {} ({}): it is a {}, Project, Profile and Scatter apply to trees and tables only.", DataLocation(dataSource, fullName), info.GetDescription(), obj->ClassName());
      }
    }
    if (!dynamic_cast<TNamed*>(obj)) {
      logger::error("Input data {} (data source {}) is of unsupported type {}.", fullName, dataSource, obj->ClassName());
      if (isOwned) delete obj;
      return;
    }
    if (!isOwned && !(obj = obj->Clone())) {
      logger::error("Could not copy input data {} (data source {}).", fullName, dataSource);
      return;
    }
    // detach from the directory ROOT attached it to (e.g. histograms read from a file or cloned)
    if (auto addToDirectory = obj->IsA()->GetDirectoryAutoAdd()) addToDirectory(obj, nullptr);
    if (!PrepareInput(obj, DataLocation(dataSource, fullName))) {
      delete obj;
      return;
    }
    static_cast<TNamed*>(obj)->SetName((fullName + suffix).data());
    mDataBuffer[dataSource][fullName].reset(obj);
  };

  if (auto* directory = dynamic_cast<TDirectory*>(folder)) {
    for (bool traverse : {false, true}) {
      set<string> visited;  // lists and folders can appear in several cycles: only the latest one (first in the keys) is searched
      for (auto* key : TRangeDynCast<TKey>(directory->GetListOfKeys())) {
        if (dataNames.empty()) return;
        if (!key) continue;
        const string name = key->GetName();
        if (!traverse && !dataNames.count(name)) continue;
        TClass* keyClass = ReadableClass(key);
        if (traverse) {
          if (!is_container(keyClass)) continue;
          if (!visited.insert(name).second) {
            if (keyClass->InheritsFrom(TDirectory::Class())) WarnHiddenDirectory(directory, name);
            continue;
          }
          if (keyClass->InheritsFrom(TDirectory::Class())) {
            if (auto* subDir = OpenDirectory(directory, key)) {
              ReadData(subDir, dataNames, prefix, suffix, dataSource, readCache);
            }
          } else if (auto cached = readCache.find(key); cached != readCache.end()) {
            ReadData(cached->second.get(), dataNames, prefix, suffix, dataSource, readCache);
          } else if (std::unique_ptr<TObject, container_deleter_t> container{key->ReadObj()}) {
            ReadData(container.get(), dataNames, prefix, suffix, dataSource, readCache);
          }
          continue;
        }
        if (is_container(keyClass)) continue;
        // an RNTuple is no TObject: it has no readable class, but RDataFrame reads it like a tree
        const bool isRNTuple = !keyClass && std::string_view(key->GetClassName()) == "ROOT::RNTuple";
        if (!keyClass && !isRNTuple) {
          logger::warning("Skipping {} (data source {}): unknown or unsupported class {}.", name, dataSource, key->GetClassName());
          continue;
        }
        if (isRNTuple || (keyClass->InheritsFrom(TTree::Class()) && !keyClass->InheritsFrom(TChain::Class()))) {
          // trees stored in files are chained over all files of the data source (processed once all files are searched)
          dataNames.erase(name);
          mDataBuffer[dataSource][prefix + name].reset(nullptr);
          string dirPath = directory->GetPath();  // file.root:/some/folder
          dirPath.erase(0, dirPath.find(":/") + 2);
          mTreeInputs[dataSource][prefix + name].push_back({directory->GetFile()->GetName(), (dirPath.empty() ? "" : dirPath + "/") + name, mIsFolderInput, isRNTuple});
          continue;
        }
        if (TObject* obj = key->ReadObj()) {
          store(obj, name, true);
        } else {
          logger::warning("Could not read {} of class {} (data source {}); the file may be corrupted.", name, key->GetClassName(), dataSource);
        }
      }
    }
  } else if (is_container(folder->IsA())) {
    for (bool traverse : {false, true}) {
      for_each_item(folder, [&](const string& name, TObject* obj) {
        if (traverse && is_container(obj->IsA())) {
          ReadData(obj, dataNames, prefix, suffix, dataSource, readCache);
        } else if (!traverse && dataNames.count(name) && !is_container(obj->IsA())) {
          store(obj, name, false);
        }
        return dataNames.empty();
      });
      if (dataNames.empty()) return;
    }
  } else {
    logger::error("Data format {} not supported.", folder->ClassName());
  }
}

//**************************************************************************************************
/**
 * Prepares an object read from an input for plotting; returns false if it cannot be plotted.
 * Display settings stored with a histogram or graph (axis ranges, minimum and maximum) are kept with the object: the painter applies them
 * only where the object defines the frame of a pad and discards them everywhere else (see PlotPainter::GetDataClone).
 * A function given by a formula has to compile: one calling C++ code unknown to ROOT's interpreter crashes when it is evaluated.
 */
//**************************************************************************************************
bool PlotManager::PrepareInput(TObject* obj, const string& location)
{
  if (auto* func = dynamic_cast<TF1*>(obj); func && func->GetFormula()) {
    const TString expression = func->GetFormula()->GetExpFormula();
    bool isValid{};
    {
      auto quiet = ignore_root_messages_below(kFatal);
      // a C++ lambda given as text (TF1 f("f", "[](double* x, double* p) {...}", xmin, xmax, npar)) is compiled as such
      isValid = (func->GetFormula()->TestBit(TFormula::kLambda)) ? TFormula("probe", expression, func->GetNdim(), func->GetNpar(), false).IsValid()
                                                                 : TFormula("probe", expression, false).IsValid();
    }
    if (!isValid) {
      logger::error("Function {} cannot be evaluated: its formula '{}' does not compile. If it calls C++ code of your own, declare that code to ROOT's interpreter before generating the plots (gInterpreter->Declare).",
                    location, expression.Data());
    }
    return isValid;
  }

  return true;
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
  std::error_code existsError;
  if (!std::filesystem::exists(inputFileName, existsError)) {
    logger::error("File {} does not exist{}.", inputFileName, existsError ? " or cannot be read (" + existsError.message() + ")" : "");
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
  bool hasBom = false;            // Excel's "CSV UTF-8" starts the file with a byte order mark, which ROOT makes part of the first column name
  bool isBomLineSkipped = false;  // the line with the mark is not data (a comment): ROOT can simply skip it
  while (lines.size() < 100 && std::getline(file, line)) {
    ++lineNumber;
    if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
      hasBom = true;
      line.erase(0, 3);
      isBomLineSkipped = trim(line).empty() || trim(line)[0] == '#';
    }
    line = trim(line);
    if (line.empty()) continue;
    if (line[0] == '#') {
      hasComments = true;
      continue;
    }
    lines.emplace_back(lineNumber, line);
  }
  if (lines.empty()) {
    logger::error("Table {} does not contain any data.", inputFileName);
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
      auto nWords = [](const string& text) {
        std::istringstream words(text);
        return std::distance(std::istream_iterator<string>(words), std::istream_iterator<string>());
      };
      for (const auto& [number, content] : lines) {
        const size_t n = split(content, guess).size();
        if (n == maxCount + 1) continue;
        string hint;
        // as many words as the header, but other gaps between them (the header may be aligned too)
        if ((guess == ' ' || guess == '\t') && nWords(content) == nWords(lines[0].second)) {
          hint = " (columns seem to be aligned with several spaces or tabs; use exactly one delimiter between the values)";
        }
        logger::error("Cannot read table {}: {}{}.", inputFileName, describeMismatch(guess, maxCount + 1, number, n), hint);
        return;
      }
    }
  }

  // ROOT takes the type of a column from its first value: an integer makes it an integer column, so later decimals are truncated
  // (0.5 is read as 0) and inf or nan cannot be read, while numbers it does not recognise as such (1e-3, inf, nan) make it a text column.
  // Such columns are declared as floating point, from all values of the table (checked as long as the type of the column is open).
  enum class column_kind_t { open,
                             integer,
                             text_like,
                             done };
  const auto columnNames = split(lines[0].second, delimiter);
  vector<column_kind_t> columnKinds(columnNames.size(), column_kind_t::open);
  vector<bool> forceDouble(columnNames.size(), false);
  vector<bool> hasEmptyCell(columnNames.size(), false);  // an integer column with an empty cell is read as floating point: ROOT stores 0 for it otherwise
  auto isInteger = [](const string& value) {
    const size_t start = (value[0] == '+' || value[0] == '-') ? 1 : 0;
    return value.size() > start && value.find_first_not_of("0123456789", start) == string::npos;
  };
  auto isNumber = [](const string& value) {
    char* end{};
    std::strtod(value.data(), &end);
    return end == value.data() + value.size();
  };
  auto checkTypes = [&](const vector<string>& fields) {
    for (size_t column = 0; column < std::min(fields.size(), columnKinds.size()); ++column) {
      column_kind_t& kind = columnKinds[column];
      if (kind == column_kind_t::done) continue;
      const string value = trim(fields[column]);
      if (value.empty()) {  // the type comes from the values that are there
        hasEmptyCell[column] = true;
        continue;
      }
      if (kind == column_kind_t::open) {
        // as ROOT: integers and numbers with a decimal point are numeric, everything else text
        static const std::regex decimal(R"([+-]?(\d+\.\d*|\d*\.\d+)([eE][+-]?\d+)?)");
        if (isInteger(value)) {
          kind = column_kind_t::integer;
        } else if (std::regex_match(value, decimal) || !isNumber(value)) {
          kind = column_kind_t::done;
        } else {
          kind = column_kind_t::text_like;
          forceDouble[column] = true;
        }
      } else if (kind == column_kind_t::integer) {
        if (!isInteger(value)) {
          forceDouble[column] = isNumber(value);  // any other value fails with an integer as well as with a floating point type
          kind = column_kind_t::done;
        }
      } else if (!isNumber(value)) {  // text-like numbers: a column with real text stays text
        forceDouble[column] = false;
        kind = column_kind_t::done;
      }
    }
  };
  for (size_t i = 1; i < lines.size(); ++i) {
    checkTypes(split(lines[i].second, delimiter));
  }
  // check the remaining lines as well, ROOT's csv reader does not tolerate a varying number of fields
  while (std::getline(file, line)) {
    ++lineNumber;
    line = trim(line);
    if (line.empty()) continue;
    if (line[0] == '#') {  // a comment also after the lines above (ROOT then needs to know)
      hasComments = true;
      continue;
    }
    const auto fields = split(line, delimiter);
    if (fields.size() != nFields) {
      logger::error("Cannot read table {}: {}.", inputFileName, describeMismatch(delimiter, nFields, lineNumber, fields.size()));
      return;
    }
    checkTypes(fields);
  }
  for (size_t column = 0; column < columnKinds.size(); ++column) {
    if (columnKinds[column] == column_kind_t::integer && hasEmptyCell[column]) forceDouble[column] = true;  // the empty cells become NaN
  }

  // ROOT does not trim the fields: names and values with surrounding spaces end up as such (values as text)
  if (delimiter != ' ' && delimiter != '\t') {
    for (const auto& [number, content] : {lines[0], lines[std::min<size_t>(1, lines.size() - 1)]}) {
      const auto fields = split(content, delimiter);
      if (std::any_of(fields.begin(), fields.end(),
                      [&](const string& field) { return field != trim(field); })) {
        logger::warning("Table {} has spaces around its delimiters (line {}), which become part of the column names and values (read as text); remove them.", inputFileName, number);
        break;
      }
    }
  }

  // numbers with decimal commas cannot be read by ROOT (they end up as text)
  if (delimiter != ',' && lines.size() > 1) {
    static const std::regex decimalComma(R"(\s*[+-]?\d+,\d+\s*)");
    for (const auto& field : split(lines[1].second, delimiter)) {
      if (std::regex_match(field, decimalComma)) {
        logger::warning("Table {} seems to use decimal commas (e.g. '{}'), which are not read as numbers; use decimal points instead.", inputFileName, trim(field));
        break;
      }
    }
  }

  ROOT::RDF::RCsvDS::ROptions options;
  auto columnName = [&](const string& field) {
    string name = trim(field);
    if (name.size() > 1 && name.front() == '"' && name.back() == '"') name = name.substr(1, name.size() - 2);
    return name;
  };
  for (size_t column = 0; column < columnNames.size(); ++column) {
    const string name = columnName(columnNames[column]);
    if (forceDouble[column] && !name.empty()) options.fColumnTypes[name] = 'D';
  }
  if (isBomLineSkipped) {
    options.fSkipFirstNLines = 1;
  } else if (hasBom) {
    for (const auto& field : columnNames) {
      options.fColumnNames.push_back(columnName(field));
    }
  }
  options.fDelimiter = delimiter;
  options.fLeftTrim = true;  // as for the checks above: only the line as a whole is trimmed
  options.fRightTrim = true;
  if (hasComments) options.fComment = '#';
  options.fLinesChunkSize = 50000;
  ProcessDataRequests("table", dataSource, name, ":" + dataSource, [&]() { return std::make_unique<ROOT::RDataFrame>(ROOT::RDF::FromCSV(inputFileName, options)); }, nullptr);
}

//**************************************************************************************************
/**
 * Find the directory, folder or collection at the path below folder (folder itself for an empty path).
 * Nothing is taken out of the structure, so further paths can be looked up in it. Directories belong to their file,
 * lists and folders read from file keys are kept in readCache (shared by all paths into them while the file is read).
 */
//**************************************************************************************************
TObject* PlotManager::FindSubDirectory(TObject* folder, const vector<string>& subDirs, read_cache_t& readCache)
{
  for (const auto& subDir : subDirs) {
    TObject* subFolder{nullptr};
    if (auto* directory = dynamic_cast<TDirectory*>(folder)) {
      TKey* key = FindContainerKey(directory, subDir);
      TClass* keyClass = (key) ? ReadableClass(key) : nullptr;
      if (key && !keyClass) {
        logger::warning("Cannot look into {} in {}: objects of class {} cannot be read (no dictionary loaded or not a TObject).", subDir, directory->GetPath(), key->GetClassName());
      } else if (keyClass && keyClass->InheritsFrom(TDirectory::Class())) {
        subFolder = OpenDirectory(directory, key);
      } else if (is_container(keyClass)) {
        WarnHiddenDirectory(directory, subDir);
        auto& container = readCache[key];
        if (!container) container.reset(key->ReadObj());
        subFolder = container.get();
      }
    } else {
      // a container of this name (there could also be data of the same name)
      for_each_item(folder, [&](const string& name, TObject* item) {
        if (name == subDir && is_container(item->IsA())) subFolder = item;
        return subFolder != nullptr;
      });
    }
    if (!subFolder) return nullptr;
    folder = subFolder;
  }
  return (folder && is_container(folder->IsA())) ? folder : nullptr;
}

//**************************************************************************************************
/**
 * A directory with a list or folder of its name written after it (ROOT allows only that order) is hidden: paths and the search find the
 * latest key of the name, so nothing in the directory can be read. Reported once per reading of the inputs.
 */
//**************************************************************************************************
void PlotManager::WarnHiddenDirectory(TDirectory* directory, const string& name)
{
  TKey* latest = FindContainerKey(directory, name);
  if (!latest || KeyInherits(latest, TDirectory::Class())) return;
  for (auto* key : TRangeDynCast<TKey>(directory->GetListOfKeys())) {
    if (!key || name != key->GetName() || !KeyInherits(key, TDirectory::Class())) continue;
    string path = directory->GetPath();  // file.root:/some/folder
    path += (path.back() == '/' ? "" : "/") + name;
    if (mHiddenDirectories.insert(path).second) {
      logger::warning("Directory {} cannot be read: a {} of the same name written after it hides it (rename one of them).", path, latest->GetClassName());
    }
    return;
  }
}

//**************************************************************************************************
/**
 * Find a tree in the inputs of a data source (the same way as when reading the data source).
 */
//**************************************************************************************************
vector<PlotManager::tree_input_t> PlotManager::FindTreeInputs(const string& dataSource, const string& treeName)
{
  vector<tree_input_t> treeInputs;
  string path;
  string name;
  std::tie(path, name) = split_path(treeName);  // not a structured binding: the lambda below captures them
  // first match should be the one in the current folder; traverse deeper only if not found
  std::function<optional<string>(TDirectory*)> find;
  find = [&](TDirectory* dir) -> optional<string> {
    if (auto* key = dir->GetKey(name.data()); key && KeyInherits(key, TTree::Class())) {
      string dirPath = dir->GetPath();  // file.root:/some/folder
      dirPath.erase(0, dirPath.find(":/") + 2);
      return (dirPath.empty() ? "" : dirPath + "/") + name;
    }
    set<string> visited;  // keys can appear in several cycles: the latest directory of a name (newer data of the name does not hide it)
    for (auto* key : TRangeDynCast<TKey>(dir->GetListOfKeys())) {
      if (!key || !KeyInherits(key, TDirectory::Class()) || !visited.insert(key->GetName()).second) continue;
      if (auto* subDir = OpenDirectory(dir, key)) {
        if (auto treePath = find(subDir)) return treePath;
      }
    }
    return nullopt;
  };
  // the directory at a path below dir, level by level as when reading
  auto subDirectory = [](TDirectory* dir, const string& subPath) -> TDirectory* {
    for (const auto& level : split_string(subPath, '/')) {
      TKey* key = (dir) ? FindContainerKey(dir, level) : nullptr;
      dir = (key && KeyInherits(key, TDirectory::Class())) ? OpenDirectory(dir, key) : nullptr;
    }
    return dir;
  };
  auto expandedIt = mExpandedInputs.find(dataSource);
  for (const auto& inputRaw : (expandedIt != mExpandedInputs.end()) ? expandedIt->second : ExpandInputs(dataSource)) {
    const auto fileNamePath = SplitInputPath(expand_path(inputRaw).value_or(""));
    const string& fileName = fileNamePath[0];
    std::error_code existsError;
    if (!str_ends_with(fileName, ".root") || !std::filesystem::exists(fileName, existsError)) continue;
    TFile file(fileName.data(), "READ");
    if (file.IsZombie()) continue;
    TDirectory* dir = &file;
    if (fileNamePath.size() > 1) dir = subDirectory(dir, fileNamePath[1]);
    if (dir && !path.empty()) dir = subDirectory(dir, path);
    if (!dir) continue;
    if (auto treePath = find(dir)) treeInputs.push_back({fileName, *treePath, fileNamePath.size() > 1, false});
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
  if (const string treeAlias = split_path(treeName).second; join.GetAlias() == treeAlias) {
    return fmt::format("the columns of both would be called {}.column, please specify an alias", treeAlias);
  }
  map<string, std::unique_ptr<TFile>> openFiles;  // each file is opened only once for the following checks
  auto getTree = [&](const string& fileName, const string& treePath) -> TTree* {
    auto& file = openFiles[fileName];
    if (!file) file = std::make_unique<TFile>(fileName.data(), "READ");
    if (!file || file->IsZombie()) return nullptr;
    return file->Get<TTree>(treePath.data());
  };
  TTree* firstTree = getTree(treeInputs.front().file, treeInputs.front().treePath);
  // a branch of the tree with sub-columns (leaflist, split object) named like the alias would make alias.column ambiguous
  if (auto* branch = (firstTree) ? firstTree->GetBranch(join.GetAlias().data()) : nullptr; branch && (branch->GetNleaves() > 1 || branch->GetListOfBranches()->GetEntries() > 0)) {
    return fmt::format("{} has a branch {}, please specify a different alias", treeName, join.GetAlias());
  }
  // the entries of a tree written in parallel (multi-threaded Snapshot) are not in their original order: ROOT refuses to match them row by row
  auto reshuffled = [&](const TTree* tree, const string& description) -> optional<string> {
    if (!join.keys.empty() || !tree || !tree->TestBit(TTree::kEntriesReshuffled)) return nullopt;
    return fmt::format("the entries of {} were reshuffled when it was written (multi-threaded Snapshot), it can only be joined by key", description);
  };
  if (auto error = reshuffled(firstTree, treeName)) return error;
  auto joinedChain = std::make_shared<TChain>(join.GetAlias().data());
  const TTree* firstJoinedTree{};
  if (join.dataSource.empty()) {
    // taken from each input of the tree: the name of the joined tree starts where the name of the tree starts
    const auto nLevels = std::count(treeName.begin(), treeName.end(), '/') + 1;
    for (const auto& treeInput : treeInputs) {
      string basePath = treeInput.treePath;
      for (int64_t level = 0; level < nLevels; ++level) {
        basePath = split_path(basePath).first;
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
        if (auto error = reshuffled(tree, treeInput.file + ":" + treeInput.treePath)) return error;
        if (auto error = reshuffled(joinedTree, treeInput.file + ":" + path)) return error;
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
    if (auto error = reshuffled(firstJoinedTree, join.tree)) return error;
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
      auto quiet = ignore_root_messages_below(kFatal);  // duplicates and keys that are too large are reported below
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
  return nullopt;
}

//**************************************************************************************************
/**
 * Process all requests (projections, profiles, scatter plots) of one tree or table.
 * The requests are booked together, so the data is read in one go (plus one pass for requests that auto-detect their axis ranges).
 * Requests that must be processed sequentially (scatter plots, entry ranges) share a separate single-threaded pass.
 */
//**************************************************************************************************
void PlotManager::ProcessDataRequests(const string& type, const string& dataSource, const string& name, const string& objNameSuffix,
                                      const std::function<std::unique_ptr<ROOT::RDataFrame>()>& makeDataFrame, TTree* tree, const string& inputsDescription,
                                      const optional<vector<Plot::Pad::Data::data_info_t::join_t>>& joins)
{
  using data_info_t = Plot::Pad::Data::data_info_t;
  auto start = std::chrono::steady_clock::now();

  // extract the reason for a failed data query: the first compiler error or error of ROOT's tree reader (if any) or the exception message
  auto getErrorReason = [](const std::exception& exception, const string& rootOutput) {
    std::istringstream lines(rootOutput);
    for (string line; std::getline(lines, line);) {
      const auto compilerError = line.find("error: ");
      const auto readerError = (line.rfind("Error in <TTreeReader", 0) == 0) ? line.find(">: ") : string::npos;
      if (compilerError != string::npos || readerError != string::npos) {
        string reason = line.substr((compilerError != string::npos) ? compilerError + 7 : readerError + 3);
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
  const auto& buffer = mDataBuffer[dataSource];
  for (auto& dataInfo : mDataInfoBuffer[dataSource][name]) {
    // results of an earlier GeneratePlots() call are kept until ClearDataBuffer(), like all other data
    if (auto result = buffer.find(name + dataInfo.GetNameSuffix()); result != buffer.end() && result->second) {
      continue;
    }
    // requests with other joins are processed on another data frame
    if (joins && dataInfo.joins.value_or(vector<Plot::Pad::Data::data_info_t::join_t>{}) != *joins) continue;
    if (dataInfo.dataDims.empty()) {
      logger::error("Cannot plot {} {} directly, specify what to extract from it (Project, Profile or Scatter).", type, DataLocation(dataSource, name));
      continue;
    }
    if (dataInfo.joins && !joins) {
      logger::error("Cannot join trees to {} {} ({}): only trees stored in files can be joined.", type, DataLocation(dataSource, name), dataInfo.GetDescription());
      continue;
    }
    (dataInfo.singleProc() ? sequentialInfos : parallelInfos).push_back(&dataInfo);
  }
  if (parallelInfos.empty() && sequentialInfos.empty()) return;

  // C strings (leaves of type C, e.g. Branch("name", buffer, "name/C") or uproot's mktree) are read by RDataFrame as RVec<Char_t>
  // padded with null characters, which cannot be compared with a string: they are given to the requests as string
  // (a redefined column is only evaluated where a request reads it)
  vector<string> cStringColumns;
  if (tree) {
    tree->LoadTree(0);  // a chain has no leaves before its first tree is loaded
    auto collect = [&cStringColumns](TTree* leavesTree, const string& prefix) {
      if (!leavesTree || !leavesTree->GetListOfLeaves()) return;
      for (auto* obj : *leavesTree->GetListOfLeaves()) {
        auto* leaf = static_cast<TLeaf*>(obj);
        if (leaf->IsA() != TLeafC::Class()) continue;
        auto* branch = leaf->GetBranch();
        cStringColumns.push_back(prefix + ((branch->GetNleaves() == 1) ? branch->GetFullName() : leaf->GetFullName()).Data());
      }
    };
    collect(tree->GetTree(), "");
    if (auto* friends = tree->GetListOfFriends()) {
      for (auto* obj : *friends) {
        auto* friendElement = static_cast<TFriendElement*>(obj);
        collect(friendElement->GetTree(), string(friendElement->GetName()) + ".");
      }
    }
  }

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
    // errors of ROOT's tree reader (a column of another type in one of the chained inputs) go there too: they are the reason given below
    static std::mutex readerMutex;
    static OutputCapture* readerOutput = nullptr;
    static ErrorHandlerFunc_t outerHandler = nullptr;
    OutputCapture* const enclosingOutput = readerOutput;  // a group processed while another one is
    {
      std::lock_guard<std::mutex> lock(readerMutex);
      readerOutput = &rootOutput;
    }
    if (!enclosingOutput) {
      outerHandler = SetErrorHandler([](Int_t level, Bool_t abort, const char* location, const char* msg) {
        if (level >= kError && location && std::string_view(location).rfind("TTreeReader", 0) == 0) {
          std::lock_guard<std::mutex> lock(readerMutex);
          if (readerOutput) {
            std::ostream(readerOutput) << "Error in <" << location << ">: " << msg << '\n';
            return;
          }
        }
        (outerHandler) ? outerHandler(level, abort, location, msg) : DefaultErrorHandler(level, abort, location, msg);
      });
    }
    auto readerGuard = make_scope_guard([enclosingOutput]() {
      {
        std::lock_guard<std::mutex> lock(readerMutex);
        readerOutput = enclosingOutput;
      }
      if (!enclosingOutput) SetErrorHandler(outerHandler);
    });
    vector<DataFrameRequest<data_info_t>> requests;  // successfully booked requests
    vector<const data_info_t*> rejected;             // requests already reported as invalid while preparing them
    try {
      auto* cerrBuffer = std::cerr.rdbuf(&rootOutput);  // capture ROOT output, only shown if something goes wrong
      auto cerrGuard = make_scope_guard([cerrBuffer]() { std::cerr.rdbuf(cerrBuffer); });
      auto mtGuard = disable_implicit_mt(sequential);
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
      ROOT::RDF::RNode base = *df;
      for (const auto& column : cStringColumns) {
        if (!df->HasColumn(column) || df->GetColumnType(column) != "ROOT::VecOps::RVec<Char_t>") continue;
        base = base.Redefine(column, [](const ROOT::RVec<Char_t>& chars) { return string(chars.data(), strnlen(chars.data(), chars.size())); }, {column});
      }
      ROOT::RDF::RNode matched = base;
      optional<ROOT::RDF::RResultPtr<ULong64_t>> nMatched;
      if (!joinKeyColumns.empty()) {
        for (const auto& column : joinKeyColumns) {
          matched = matched.FilterAvailable(column);
        }
        nMatched = matched.Count();
      }

      for (auto info : infos) {
        DataFrameRequest<data_info_t> request(info, fmt::format("{} {}", type, DataLocation(dataSource, name)), name + info->GetNameSuffix() + objNameSuffix);
        auto outputStart = rootOutput.Text().size();  // only consider ROOT output caused by this request
        try {
          // the requests share the filter on the matching rows (evaluated once per row), except those with an entry range:
          // their range refers to all rows of the tree, so they apply the filter after it
          const bool hasEntryRange = info->entries.max.has_value();
          if (request.Prepare((hasEntryRange) ? base : matched, (hasEntryRange) ? joinKeyColumns : vector<string>{})) {
            requests.push_back(std::move(request));
          } else {
            rejected.push_back(info);
          }
        } catch (const std::exception& e) {
          if (type == "table" && dynamic_cast<const std::invalid_argument*>(&e)) throw;  // the csv reader or an expression: handled below
          rejected.push_back(info);
          logger::error("Invalid query for {} {} ({}): {}.", type, DataLocation(dataSource, name), info->GetDescription(), getErrorReason(e, rootOutput.Text().substr(outputStart)));
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
    } catch (const std::exception& e) {
      if (type == "table" && dynamic_cast<const std::invalid_argument*>(&e)) {
        // thrown by the csv reader if a value does not match the column type deduced from the first lines, but just as well by an
        // expression of a request (e.g. std::stod): only one by one the requests that are fine are kept
        if (infos.size() > 1) {
          for (auto info : infos) {
            if (std::find(rejected.begin(), rejected.end(), info) == rejected.end()) processGroup({info}, sequential);
          }
        } else {
          logger::error("Cannot read {} {} ({}): it contains a value that does not match the type of its column, or an expression failed ({}).", type, DataLocation(dataSource, name), infos[0]->GetDescription(), e.what());
        }
      } else if (requests.size() > 1) {
        // an invalid expression only shows up when the event loop starts and then spoils all requests of this data frame: process them one by one
        for (auto& request : requests) {
          processGroup({request.GetInfo()}, sequential);
        }
      } else if (requests.size() == 1) {
        logger::error("Invalid query for {} {} ({}): {}{}.", type, DataLocation(dataSource, name), requests[0].GetInfo()->GetDescription(), getErrorReason(e, rootOutput.Text()), chainHint);
      } else {
        logger::error("Cannot read {} {}: {}{}.", type, DataLocation(dataSource, name), getErrorReason(e, rootOutput.Text()), chainHint);
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
  if (nUnmatched && *nUnmatched) joinsDescription += fmt::format(", {} without a match skipped", plural(*nUnmatched, "row"));
  string message = fmt::format(" - {} {} entries{}{}, {}, {}, {:.1f} s", DataLocation(dataSource, name), nEntries.value_or(0), (inputsDescription.empty()) ? "" : " from " + inputsDescription, joinsDescription, plural(nRequests, "request"), plural(nPasses, "pass", "es"), seconds);
  logger::info("{}", message);
  for (const auto& [info, counts] : entryCounts) {
    if (!counts.second) logger::warning("No entries selected in {} {} ({}).", type, DataLocation(dataSource, name), info->GetDescription());
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
    logger::error("Pass name of project to plot manager to read its property {}.", property);
    return "";
  }
  return Config::Get().Property(mProjectName, property);
}

//**************************************************************************************************
/**
 * Saves all required plot definitions and input file locations of the project.
 */
//**************************************************************************************************
void PlotManager::SaveProject() const
{
  if (mProjectName.empty()) {
    logger::error("Pass name of project to plot manager to save the project.");
    return;
  }
  if (Config::Get().Path().empty()) {
    logger::error("No valid config path; cannot save project {}.", mProjectName);
    return;
  }
  namespace fs = std::filesystem;

  std::error_code ec;
  if (fs::create_directories(Config::Get().ProjectPath(mProjectName), ec)) {
    logger::info("Created config folder for project {}: {}.", mProjectName, Config::Get().ProjectPath(mProjectName).string());
  } else if (ec) {
    logger::error("Cannot create config folder {} of project {}: {}.", Config::Get().ProjectPath(mProjectName).string(), mProjectName, ec.message());
    return;
  }

  SaveDataSources();
  SavePlots();

  if (!mOutputDirectory.empty()) {
    if (!Config::Get().Exists(mProjectName)) {
      // not registered yet (e.g. the program was run before srp add): a project without program would block the registration
      logger::info("Project {} is not registered yet, the output directory is not stored: srp add {} <program> [<outdir>].", mProjectName, mProjectName);
    } else if (mOutputDirectory != Config::Get().OutputDir(mProjectName)) {
      Config::GetMutable().SetProperty(mProjectName, "outdir", mOutputDirectory);
      Config::GetMutable().Save();
    }
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
