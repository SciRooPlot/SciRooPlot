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

#include "SciRooPlot/Logger.h"
#include "SciRooPlot/Plot.h"
#include "SciRooPlot/PlotManager.h"

#include <TApplication.h>
#include <TBranch.h>
#include <TBrowser.h>
#include <TCanvas.h>
#include <TClass.h>
#include <TColorWheel.h>
#include <TDataType.h>
#include <TFile.h>
#include <TFolder.h>
#include <TKey.h>
#include <TROOT.h>
#include <TRootBrowser.h>
#include <TRootCanvas.h>
#include <TTree.h>

#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "Validation.h"

using boost::property_tree::ptree;
using std::string;
using std::vector;

using namespace SciRooPlot;
namespace po = boost::program_options;
bool PrintRootFileContents(const string& inputPath);

int main(int argc, char* argv[])
{
  string command;
  string project;
  string property;
  string setting;

  try {
    string configPath = Config::Get().Path();
    if (configPath.empty()) {
      logger::error("No usable config path; check SCIROOPLOT_CONFIG_PATH.");
      return 1;
    }
    if (std::filesystem::create_directories(configPath)) {
      logger::info("Created config folder: {}.", configPath);
    }

    po::options_description arguments("positional arguments");
    arguments.add_options()("command", po::value<string>(), "command")("project", po::value<string>(), "project")("property", po::value<string>(), "property")("setting", po::value<string>(), "setting");
    po::positional_options_description pos;
    pos.add("command", 1);
    pos.add("project", 1);
    pos.add("property", 1);
    pos.add("setting", 1);

    po::variables_map vm;
    // no options, so an argument starting with a dash (e.g. a negative number) is an ordinary positional one
    po::store(po::command_line_parser(argc, argv).options(arguments).positional(pos).style(po::command_line_style::unix_style & ~(po::command_line_style::allow_short | po::command_line_style::allow_long)).run(), vm);
    po::notify(vm);

    if (vm.count("command")) {
      command = vm["command"].as<string>();
    } else {
      logger::error("Not enough arguments (srp help lists the commands).");
      return 1;
    }
    if (vm.count("project")) {
      project = vm["project"].as<string>();
      if (project == "@current") {
        project = Config::Get().CurrentProject();
        if (project.empty()) {
          if ((command != "get") && (command != "confdir")) {
            logger::error("No project selected. Please run srp select <project>.");
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
    }
  } catch (std::exception& e) {
    logger::error(R"(Exception "{}"! Exiting.)", e.what());
    return 1;
  } catch (...) {
    logger::error("Exception of unknown type! Exiting.");
    return 1;
  }

  if (command == "help") {
    logger::print("===========================================================");
    logger::print("Project management:");
    logger::print("  srp projects");
    logger::print("  srp select  <project>");
    logger::print("  srp show    [<project> | @current]");
    logger::print("  srp stats   (<project> | @current)");
    logger::print("  srp rename  (<project> | @current) <name>");
    logger::print("  srp remove  (<project> | @current)");
    logger::print("-----------------------------------------------------------");
    logger::print("Project initialization:");
    logger::print("  srp add         <project> <program> [<outdir>]");
    logger::print("  srp init-cpp    <project> [<dir>]    (minimal skeleton)");
    logger::print("  srp init-py     <project> [<dir>]");
    logger::print("  srp example-cpp <project> [<dir>]    (commented examples with example data)");
    logger::print("  srp example-py  <project> [<dir>]");
    logger::print("-----------------------------------------------------------");
    logger::print("Project configuration:");
    logger::print("  srp get   (<project> | @current) <property>");
    logger::print("  srp set   (<project> | @current) <property> <value>");
    logger::print("  srp unset (<project> | @current) <property>");
    logger::print("      property = program | outdir | <user-variable> (read in code via GetProjectProperty)");
    logger::print("-----------------------------------------------------------");
    logger::print("Project access:");
    logger::print("  srp confdir [<project> | @current]");
    logger::print("  srp cd      [<project> | @current]");
    logger::print("  srp edit    [<project> | @current]");
    logger::print("  srp out     [<project> | @current]  (opens the output folder)");
    logger::print("-----------------------------------------------------------");
    logger::print("Settings:");
    logger::print("  srp settings");
    logger::print("  srp color       (bright | dark | off)");
    logger::print("  srp verbosity   (debug | log | info | warning | error | silent)");
    logger::print("  srp plotmode    (show | spread | pdf | eps | svg | png | gif | jpg)");
    logger::print("  srp matchmode   (exact | contains)");
    logger::print("  srp matchcase   (sensitive | insensitive)");
    logger::print("  srp screenscale <factor>  (size of plots shown on screen, default 1)");
    logger::print("  srp bitmapscale <factor>  (resolution of png/jpg/gif files, default 1)");
    logger::print("-----------------------------------------------------------");
    logger::print("Maintenance:");
    logger::print("  srp info");
    logger::print("  srp update");
    logger::print("  srp clean");
    logger::print("  srp reset");
    logger::print("-----------------------------------------------------------");
    logger::print("Utilities:");
    logger::print("  srp open  <file>");
    logger::print("  srp print <file>");
    logger::print("  srp colorwheel [<file>]  (ROOT's colours with their names; saved to the file if one is given)");
    logger::print("===========================================================");
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
  } else if (command == "colorwheel") {
    const string fileName = project;  // optional: save the wheel instead of showing it
    gROOT->SetBatch(!fileName.empty());
    std::optional<TApplication> app;
    if (fileName.empty()) app.emplace("SciRooPlot", nullptr, nullptr);
    auto* canvas = new TCanvas("colorwheel", "ROOT colour wheel", 800, 800);
    auto* wheel = new TColorWheel();
    wheel->SetCanvas(canvas);
    wheel->Draw();
    canvas->SetFillColor(kWhite);  // instead of the background colour TColorWheel gives the canvas
    canvas->SetFillStyle(1001);
    canvas->Update();
    if (!fileName.empty()) {
      canvas->SetCanvasSize(800, 800);  // the drawing area, without the window frame
      canvas->SaveAs(fileName.data());
      return 0;
    }
    canvas->ToggleEventStatus();  // the status bar names the colour under the mouse
    if (auto* imp = dynamic_cast<TRootCanvas*>(canvas->GetCanvasImp())) {
      imp->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
    }
    app->Run();
  } else if (command == "print") {
    string fileName = project;
    if (!PrintRootFileContents(fileName)) return 1;
  } else if (command == "settings") {
    Config::Get().ShowSettings();
  } else if (command == "color") {
    if (auto colorMode = Config::ParseColorMode(project)) {
      Config::GetMutable().SetColorScheme(*colorMode);
    } else {
      logger::error("Invalid color scheme (bright | dark | off).");
      return 1;
    }
  } else if (command == "verbosity") {
    if (auto logLevel = Config::ParseLogLevel(project)) {
      Config::GetMutable().SetVerbosity(*logLevel);
    } else {
      logger::error("Invalid verbosity (debug | log | info | warning | error | silent).");
      return 1;
    }
  } else if (command == "plotmode") {
    if (project.empty()) {
      logger::error("Specify default plotmode.");
      return 1;
    }
    if (project != "list" && !is_valid_plot_mode(project)) {
      string modeList;
      for (const auto& mode : plot_modes())
        modeList += mode + ", ";
      logger::error("Invalid plot mode '{}' (valid modes: {}gif+<centiseconds>[++[<repeats>]], list).", project, modeList);
      return 1;
    }
    Config::GetMutable().SetPlotMode(project);
  } else if (command == "matchmode") {
    string matchMode = project;
    if (matchMode == "exact") {
      Config::GetMutable().SetMatchContains(false);
    } else if (matchMode == "contains") {
      Config::GetMutable().SetMatchContains(true);
    } else {
      logger::error("Invalid match mode  (exact | contains).");
      return 1;
    }
  } else if (command == "matchcase") {
    string matchCase = project;
    if (matchCase == "sensitive") {
      Config::GetMutable().SetMatchCaseInsensitive(false);
    } else if (matchCase == "insensitive") {
      Config::GetMutable().SetMatchCaseInsensitive(true);
    } else {
      logger::error("Invalid case sensistivity option for matching (sensitive | insensitive).");
      return 1;
    }
  } else if (command == "screenscale" || command == "bitmapscale") {
    double scale{};
    try {
      size_t parsed{};
      scale = std::stod(project, &parsed);
      if (parsed != project.size()) scale = 0.;
    } catch (const std::exception&) {
      scale = 0.;
    }
    if (!(scale > 0.)) {
      logger::error("Invalid {} '{}' (positive number, e.g. 1.5).", command, project);
      return 1;
    }
    if (command == "screenscale") {
      Config::GetMutable().SetScreenScale(scale);
    } else {
      Config::GetMutable().SetBitmapScale(scale);
    }
  } else if (command == "info") {
    auto getenv = [](const char* name) {
      const char* value = std::getenv(name);
      return value ? value : "<not set>";
    };
    std::cout << "========================================\n";
    std::cout << "SciRooPlot " << SCIROOPLOT_VERSION << "\n";
    std::cout << "========================================\n";
    std::cout << "Installation\n";
    std::cout << "  Prefix: " << getenv("SCIROOPLOT_ROOT") << "\n";
    std::cout << "  Binary: " << getenv("SCIROOPLOT_BIN") << "\n";
    std::cout << "  Library:" << getenv("SCIROOPLOT_LIB") << "\n";
    auto sourceDir = getenv("SCIROOPLOT_SOURCE_DIR");
    std::filesystem::path path(sourceDir);
    if (std::filesystem::exists(path)) {
      std::cout << "  Source: " << std::filesystem::canonical(path).string() << "\n";
    }
    std::cout << "  Env:    " << getenv("SCIROOPLOT_ENV") << "\n";

    std::cout << "\nDependencies\n";
    std::cout << "  ROOT:   " << SCIROOPLOT_ROOT_EXECUTABLE
              << " (version " << SCIROOPLOT_ROOT_VERSION << ")\n";
    std::cout << "  Python: " << getenv("SCIROOPLOT_PYTHON_EXECUTABLE") << "\n";

    std::cout << "\nBuild\n";
    std::cout << "  Type:     " << SCIROOPLOT_BUILD_TYPE << "\n";
    std::cout << "  Commit:   " << SCIROOPLOT_GIT_DESCRIBE << "\n";
    std::cout << "  Compiler: " << SCIROOPLOT_CXX_COMPILER << "\n";

    std::cout << "========================================\n";
  } else if (command == "confdir") {
    std::cout << ((project.empty()) ? Config::Get().Path().string() : Config::Get().ProjectPath(project).string()) << std::endl;
    return 0;
  } else if (command == "rename") {
    if (project.empty()) {
      logger::error("Specify project to rename.");
      return 1;
    }
    if (property.empty()) {
      logger::error("Specify new name for {}.", project);
      return 1;
    }
    if (!Config::GetMutable().Rename(project, property)) return 1;
  } else if (command == "clean") {
    Config::GetMutable().Clean();
  } else if (command == "reset") {
    Config::GetMutable().Reset();
  } else if (command == "projects") {
    Config::Get().ListProjects();
  } else if (command == "remove") {
    if (!Config::GetMutable().Remove(project)) return 1;
  } else if (command == "select") {
    if (!Config::GetMutable().Select(project)) return 1;
  } else if (command == "show") {
    if (!project.empty() && !Config::Get().Exists(project)) {
      logger::error("Cannot find project {}.", project);
      return 1;
    }
    Config::Get().Show(project);
  } else if (command == "add") {
    if (project.empty()) {
      logger::error("Specify project name.");
      return 1;
    }
    if (property.empty()) {
      logger::error("Specify program for project {}.", project);
      return 1;
    }
    if (Config::Get().Exists(project)) {
      logger::error("Project {} already exists. Use srp set {} program <program> to change its program.", project, project);
      return 1;
    }
    if (!Config::GetMutable().SetProgram(project, property)) return 1;
    if (!setting.empty() && !Config::GetMutable().SetOutputDir(project, setting)) {
      Config::GetMutable().Remove(project);  // otherwise the project would be saved with the program only
      return 1;
    }
    Config::GetMutable().Select(project);
    logger::info("Selecting project {}.", project);
  } else if (command == "get") {
    if (project.empty()) {
      logger::error("Specify project or use @current.");
      return 1;
    }
    if (!Config::Get().Exists(project)) {
      logger::error("Cannot find project {}.", project);
      return 1;
    }
    if (property == "program") {
      std::cout << Config::Get().Program(project) << std::endl;
    } else if (property == "outdir") {
      std::cout << Config::Get().OutputDir(project) << std::endl;
    } else {
      std::cout << Config::Get().Property(project, property) << std::endl;
    }
  } else if (command == "unset") {
    if (project.empty()) {
      logger::error("Specify project or use @current.");
      return 1;
    }
    if (!Config::Get().Exists(project)) {
      logger::error("Cannot find project {}.", project);
      return 1;
    }
    if (property.empty()) {
      logger::error("Specify property to unset for {}.", project);
      return 1;
    }
    if (!Config::GetMutable().SetProperty(project, property, "")) return 1;
  } else if (command == "set") {
    if (project.empty()) {
      logger::error("Specify project or use @current.");
      return 1;
    }
    if (!Config::Get().Exists(project)) {
      logger::error("Cannot find project {}. Use srp add or srp init-cpp/init-py to create it.", project);
      return 1;
    }
    if (property.empty()) {
      logger::error("Specify property to set for {}.", project);
      return 1;
    }
    if (setting.empty()) {
      logger::error("Specify new setting for {}.", project);
      return 1;
    }
    if (!Config::GetMutable().SetProperty(project, property, setting)) return 1;
  } else {
    const vector<string> shellCommands = {"update", "edit", "cd", "out", "stats", "init-py", "init-cpp", "example-py", "example-cpp"};
    if (std::find(shellCommands.begin(), shellCommands.end(), command) != shellCommands.end()) {
      logger::error("Source environment script to use this feature.");
    } else {
      logger::error("Unknown command {} (srp help lists the commands).", command);
    }
    return 1;
  }
  return Config::GetMutable().Save() ? 0 : 1;
}

bool PrintRootFileContents(const string& inputPath)
{
  string fileName = inputPath;
  string startPath;
  if (auto pos = inputPath.find(':'); pos != string::npos) {
    fileName = inputPath.substr(0, pos);
    startPath = inputPath.substr(pos + 1);
  }
  TFile inputFile(fileName.data(), "READ");
  if (inputFile.IsZombie()) {
    logger::error("Could not find file {}.", fileName);
    return false;
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
      logger::error("Path not found: {}.", startPath);
      return false;
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
        // listed by the name under which it is stored, which is the one to access it with (the object's own name can differ)
        const auto slashPos = path.rfind('/');
        if (slashPos != string::npos) logger::print("{}/", path.substr(0, slashPos));
        logger::print("- [{}] {}", object->ClassName(), path.substr((slashPos == string::npos) ? 0 : slashPos + 1));
        return;
      }
      bool printedPath = false;
      TIter next(itemList);
      TObject* obj = nullptr;
      while ((obj = next())) {
        string name = obj->GetName();
        bool ownsObj = false;
        if (obj->IsA() == TKey::Class()) {
          auto* key = static_cast<TKey*>(obj);
          name = key->GetName();
          obj = key->ReadObj();
          if (!obj) {
            logger::warning("Skipping key {} of type {}; the class is not available.", name, key->GetClassName());
            continue;
          }
          ownsObj = true;
        }
        if (obj->InheritsFrom(TDirectory::Class()) || obj->InheritsFrom(TFolder::Class()) || obj->InheritsFrom(TCollection::Class())) {
          loopData(obj, path.empty() ? name : path + "/" + name);
        } else {
          if (!printedPath) {
            if (!path.empty()) logger::print("{}/", path);
            printedPath = true;
          }
          logger::print("- [{}] {}", obj->ClassName(), name);
        }
        if (auto* tree = dynamic_cast<TTree*>(obj)) {
          TObjArray* branches = tree->GetListOfBranches();
          TIter bnext(branches);
          TBranch* br = nullptr;
          while ((br = static_cast<TBranch*>(bnext()))) {
            TClass* branchClass = nullptr;
            EDataType branchType = kOther_t;
            br->GetExpectedType(branchClass, branchType);
            string typeName = branchClass ? branchClass->GetName() : TDataType::GetTypeName(branchType);
            logger::print("  -> {} ({})", br->GetName(), typeName);
          }
        }
        if (ownsObj) delete obj;
      }
    };
  loopData(root, startPath);
  return true;
}
