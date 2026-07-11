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

#include "PlotManager.h"
#include "Plot.h"
#include "Logging.h"

#include <vector>
#include <iostream>
#include <string>
#include <filesystem>
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <fmt/ranges.h>

#include "TFile.h"
#include "TBrowser.h"
#include "TRootBrowser.h"
#include "TApplication.h"
#include <TROOT.h>
#include <TFolder.h>
#include <TKey.h>
#include <TTree.h>

#include "Helpers.h"

using boost::property_tree::ptree;
using std::string;
using std::vector;

using namespace SciRooPlot;
namespace po = boost::program_options;
void PrintRootFileContents(const string& inputPath);

int main(int argc, char* argv[])
{
  string configPath = Config::Get().Path();
  if (std::filesystem::create_directories(configPath)) {
    INFO("Created config folder: {}", configPath);
  }
  string configFileName = Config::Get().ProjectsFile();

  ptree configTree;
  string activeProject;
  if (file_exists(configFileName)) {
    using boost::property_tree::read_info;
    read_info(configFileName, configTree);
    if (auto active = configTree.get_child_optional("@current")) {
      activeProject = active.get().data();
    }
  }

  string command;
  string project;
  string property;
  string setting;

  try {
    po::options_description arguments("positional arguments");
    arguments.add_options()("command", po::value<string>(), "command")("project", po::value<string>(), "project")("property", po::value<string>(), "property")("setting", po::value<string>(), "setting");
    po::positional_options_description pos;
    pos.add("command", 1);
    pos.add("project", 1);
    pos.add("property", 1);
    pos.add("setting", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(arguments).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.count("command")) {
      command = vm["command"].as<string>();
    } else {
      ERROR("Not enough arguments.");
      return 1;
    }
    if (vm.count("project")) {
      project = vm["project"].as<string>();
      if (project == "@current") {
        project = activeProject;
        if (activeProject.empty()) {
          if ((command != "get") && (command != "path")) {
            ERROR("No current project selected. Please run srp select <PROJECT_NAME>");
          }
          return 1;
        }
      }
    }
    if (vm.count("property")) {
      property = vm["property"].as<string>();
    }
    if (vm.count("setting")) {
      setting = vm["setting"].as<string>();
      if (property == "NAME" && setting == "@current") {
        ERROR("A project must not be called @current.");
        return 1;
      }
    }
  } catch (std::exception& e) {
    ERROR(R"(Exception "{}"! Exiting.)", e.what());
    return 1;
  } catch (...) {
    ERROR("Exception of unknown type! Exiting.");
    return 1;
  }
  static const vector<string> propertyList = {"NAME", "EXE", "OUT"};

  if (command == "help") {
    PRINT("srp (projects | path | reset | clean)");
    PRINT("srp (init-cpp | init-py | remove | select | show) (<PROJECT_NAME> | @current)");
    PRINT("srp (set | get) (<PROJECT_NAME> | @current) (NAME | EXE | OUT) <SETTING>");
    PRINT("srp (open | print) <FILE_NAME>");
  } else if (command == "open") {
    string fileName = project;
    gROOT->SetBatch(false);
    TApplication app("SciRooPlot", nullptr, nullptr);
    TFile* file = TFile::Open(fileName.data());
    if (!file || file->IsZombie()) return 1;
    TBrowser* browser = new TBrowser("TBrowser", file, "SciRooPlot", 1200, 1000);
    if (auto* rb = dynamic_cast<TRootBrowser*>(browser->GetBrowserImp())) {
      rb->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
    }
    app.Run();
  } else if (command == "print") {
    string fileName = project;
    PrintRootFileContents(fileName);
  } else if (command == "path") {
    std::cout << configPath << ((project.empty()) ? "" : "/" + project) << std::endl;
    return 0;
  } else if (command == "reset") {
    configTree.clear();
    activeProject.clear();
  } else if (command == "clean") {
    vector<string> inactiveProjects;
    string firstProject;
    for (auto& entry : configTree) {
      if (entry.first == "@current") continue;
      if (auto curConfig = configTree.get_child_optional(entry.first)) {
        if (auto executable = curConfig.get().get_child_optional("EXE")) {
          if (std::filesystem::exists(std::filesystem::path(executable->get_value<string>()).parent_path())) {
            if (firstProject.empty()) firstProject = entry.first;
            continue;
          }
          inactiveProjects.push_back(entry.first);
        }
      }
    }
    bool updatedActiveProject = false;
    for (auto& inactiveProject : inactiveProjects) {
      PRINT("- deleting project {}", inactiveProject);
      configTree.erase(inactiveProject);
      std::filesystem::remove_all(configPath + "/" + inactiveProject);
      if (inactiveProject == activeProject) {
        activeProject = firstProject;
        updatedActiveProject = true;
      }
    }
    if (updatedActiveProject) {
      INFO("Selecting project {}", activeProject);
    }
  } else if (command == "projects") {
    for (auto& entry : configTree) {
      if (entry.first == "@current") continue;
      PRINT("{}  {}", (entry.first == activeProject) ? "*" : " ", entry.first);
    }
  } else if (command == "remove") {
    if (project.empty()) {
      ERROR("Specify which project to remove.");
      return 1;
    }
    configTree.erase(project);
    std::filesystem::remove_all(configPath + "/" + project);
    if (project == activeProject) {
      string firstProject;
      for (auto& entry : configTree) {
        if (entry.first == "@current") continue;
        if (auto curConfig = configTree.get_child_optional(entry.first)) {
          firstProject = entry.first;
          break;
        }
      }
      activeProject = firstProject;
      if (!activeProject.empty()) {
        INFO("Selecting project {}", activeProject);
      }
    }
  } else if (command == "select") {
    if (project.empty()) {
      ERROR("Specify a project to select.");
      return 1;
    }
    if (auto it = configTree.find(project); it != configTree.not_found()) {
      activeProject = project;
    } else {
      ERROR("Cannot find project {}.", project);
      return 1;
    }
  } else if (command == "show") {
    if (project.empty()) {
      ERROR("Specify a project to show.");
      return 1;
    }
    if (auto curConfig = configTree.get_child_optional(project)) {
      PRINT("{} NAME: {}", (project == activeProject) ? "*" : " ", project);
      for (auto property : propertyList) {
        if (auto entry = curConfig.get().get_child_optional(property)) {
          PRINT("  -{}: {}", property, entry->get_value<string>());
        }
      }
    }
  } else if ((command == "set") || (command == "get")) {
    if (project.empty()) {
      ERROR("Specify project or use @current.");
      return 1;
    }
    if (!std::count(propertyList.begin(), propertyList.end(), property)) {
      ERROR("Specify what you want to {}. Options are ({}).", command, fmt::join(propertyList, " | "));
      return 1;
    }
    if (auto curConfig = configTree.get_child_optional(project)) {
      if (command == "get") {
        if (auto entry = curConfig.get().get_child_optional(property)) {
          std::cout << entry->get_value<string>() << std::endl;
          return 0;
        } else if (property == "NAME") {
          std::cout << project << std::endl;
          return 0;
        }
      } else {
        if (property == "NAME") {
          if (!configTree.get_child_optional(setting)) {
            configTree.add_child(setting, configTree.get_child(project));
            configTree.erase(project);
            std::filesystem::rename(std::filesystem::path(configPath + "/" + project), std::filesystem::path(configPath + "/" + setting));
            PRINT("Renamed project {} to {}. User code should be adjusted accordingly.", project, setting);
            if (activeProject == project) {
              activeProject = setting;
            }
          } else {
            ERROR("Cannot rename {} to {}", project, setting);
          }
        }
      }
    }
    if (command == "set" && property != "NAME") {
      if (std::filesystem::path(expand_path(setting)).is_relative()) {
        ERROR("The {} path must not be relative.", property);
        return 1;
      }
      configTree.put(project + "." + property, setting);
      if (activeProject.empty()) {
        activeProject = project;
      }
    }
  } else {
    ERROR("Invalid arguments.");
  }

  configTree.erase("@current");
  configTree.add("@current", activeProject);

  using boost::property_tree::write_info;
  write_info(configFileName, configTree);

  return 0;
}

void PrintRootFileContents(const string& inputPath)
{
  string fileName = inputPath;
  string startPath;
  if (auto pos = inputPath.find(':'); pos != string::npos) {
    fileName = inputPath.substr(0, pos);
    startPath = inputPath.substr(pos + 1);
  }
  TFile inputFile(fileName.data(), "READ");
  if (inputFile.IsZombie()) {
    ERROR("Could not find file {}.", fileName);
    return;
  }

  auto findObject = [](TObject* root, const string& path) -> TObject* {
    TObject* current = root;
    size_t pos = 0;
    while (current && pos < path.size()) {
      auto next = path.find('/', pos);
      auto part = path.substr(pos, next == string::npos ? string::npos : next - pos);

      if (current->InheritsFrom(TDirectory::Class())) {
        current = static_cast<TDirectory*>(current)->Get(part.data());
      } else if (current->InheritsFrom(TFolder::Class())) {
        current = static_cast<TFolder*>(current)->GetListOfFolders()->FindObject(part.data());
      } else if (current->InheritsFrom(TCollection::Class())) {
        current = static_cast<TCollection*>(current)->FindObject(part.data());
      } else {
        return nullptr;
      }
      pos = next == string::npos ? path.size() : next + 1;
    }
    return current;
  };

  TObject* root = &inputFile;
  if (!startPath.empty()) {
    root = findObject(root, startPath);
    if (!root) {
      PRINT("Path not found: {}", startPath);
      return;
    }
  }

  std::function<void(TObject*, const string&)> loopData =
    [&](TObject* object, const string& path) {
      TCollection* itemList = nullptr;
      if (object->InheritsFrom(TDirectory::Class())) {
        itemList = static_cast<TDirectory*>(object)->GetListOfKeys();
      } else if (object->InheritsFrom(TFolder::Class())) {
        itemList = static_cast<TFolder*>(object)->GetListOfFolders();
      } else if (object->InheritsFrom(TCollection::Class())) {
        itemList = static_cast<TCollection*>(object);
      }
      if (!itemList) {
        string folder = path.substr(0, path.rfind('/'));
        if (!folder.empty()) PRINT("{}/", folder);
        PRINT("- [{}] {}", object->ClassName(), object->GetName());
        return;
      }
      bool printedPath = false;
      TIter next(itemList);
      TObject* obj = nullptr;
      while ((obj = next())) {
        string name = obj->GetName();
        if (obj->IsA() == TKey::Class()) {
          auto* key = static_cast<TKey*>(obj);
          name = key->GetName();
          obj = key->ReadObj();
        }
        if (obj->InheritsFrom(TDirectory::Class()) || obj->InheritsFrom(TFolder::Class()) || obj->InheritsFrom(TCollection::Class())) {
          loopData(obj, path.empty() ? name : path + "/" + name);
        } else {
          if (!printedPath) {
            if (!path.empty()) PRINT("{}/", path);
            printedPath = true;
          }
          PRINT("- [{}] {}", obj->ClassName(), obj->GetName());
        }
        if (auto* tree = dynamic_cast<TTree*>(obj)) {
          TObjArray* branches = tree->GetListOfBranches();
          TIter bnext(branches);
          TBranch* br = nullptr;
          while ((br = static_cast<TBranch*>(bnext()))) {
            PRINT("  -> {}", br->GetName());
          }
        }
      }
    };
  loopData(root, startPath);
}
