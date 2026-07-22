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
        project = Config::Get().CurrentProject();
        if (project.empty()) {
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

  if (command == "help") {
    PRINT("srp (projects | path | reset | clean)");
    PRINT("srp (init-cpp | init-py | remove | select | show) (<projectName> | @current)");
    PRINT("srp (set | get) (<projectName> | @current) (NAME | EXE | OUT) <setting>");
    PRINT("srp color (bright | dark | off)");
    PRINT("srp verbosity (debug | log | info | warning | error)");
    PRINT("srp (open | print) <fileName>");
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
  } else if (command == "color") {
    string colorSetting = project;
    if (colorSetting == "off") {
      Config::GetMutable().SetColorScheme(Config::ColorMode::off);
    } else if (colorSetting == "dark") {
      Config::GetMutable().SetColorScheme(Config::ColorMode::dark);
    } else if (colorSetting == "bright") {
      Config::GetMutable().SetColorScheme(Config::ColorMode::bright);
    }
  } else if (command == "verbosity") {
    string logLevel = project;
    if (logLevel == "log") {
      Config::GetMutable().SetVerbosity(Config::LogLevel::log);
    } else if (logLevel == "info") {
      Config::GetMutable().SetVerbosity(Config::LogLevel::info);
    } else if (logLevel == "warning") {
      Config::GetMutable().SetVerbosity(Config::LogLevel::warning);
    } else if (logLevel == "error") {
      Config::GetMutable().SetVerbosity(Config::LogLevel::error);
    } else if (logLevel == "debug") {
      Config::GetMutable().SetVerbosity(Config::LogLevel::debug);
    }
  } else if (command == "path") {
    std::cout << ((project.empty()) ? Config::Get().Path().string() : Config::Get().ProjectPath(project).string()) << std::endl;
    return 0;
  } else if (command == "reset") {
    Config::GetMutable().Reset();
  } else if (command == "clean") {
    Config::GetMutable().Clean();
  } else if (command == "projects") {
    Config::Get().ListProjects();
  } else if (command == "remove") {
    Config::GetMutable().Remove(project);
  } else if (command == "select") {
    Config::GetMutable().Select(project);
  } else if (command == "show") {
    Config::Get().Show(project);
  } else if ((command == "set") || (command == "get")) {
    if (project.empty()) {
      ERROR("Specify project or use @current.");
      return 1;
    }
    static const vector<string> propertyList = {"NAME", "EXE", "OUT"};
    if (!std::count(propertyList.begin(), propertyList.end(), property)) {
      ERROR("Specify what you want to {}. Options are ({}).", command, fmt::join(propertyList, " | "));
      return 1;
    }
    if (command == "get") {
      if (property == "NAME") {
        std::cout << project << std::endl;
      } else if (property == "EXE") {
        std::cout << Config::Get().Executable(project) << std::endl;
      } else if (property == "OUT") {
        std::cout << Config::Get().OutputDir(project) << std::endl;
      }
    } else if (command == "set") {
      if (setting.empty()) {
        ERROR("Specify new setting for {}.", project);
        return 1;
      }
      if (property == "NAME") {
        Config::GetMutable().Rename(project, setting);
      } else if (property == "EXE") {
        Config::GetMutable().SetExecutable(project, setting);
      } else if (property == "OUT") {
        Config::GetMutable().SetOutputDir(project, setting);
      }
    }
  } else {
    ERROR("Invalid arguments.");
  }
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
