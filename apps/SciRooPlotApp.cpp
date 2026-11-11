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
#include <TKey.h>
#include <TROOT.h>
#include <TRootBrowser.h>
#include <TRootCanvas.h>
#include <TSystem.h>
#include <TTree.h>

#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/version.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "Config.h"
#include "Validation.h"
#include "util/RootContainers.h"
#include "util/ScopeGuard.h"

using boost::property_tree::ptree;
using std::string;
using std::vector;

using namespace SciRooPlot;
namespace po = boost::program_options;
bool PrintRootFileContents(const string& inputPath);
int RunCommand(const string& command, const string& project, const string& property, const string& setting);
std::unique_ptr<TApplication> OpenWindowApplication(const string& command);

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
      if (project == "@current" && command == "add") {
        logger::error("A project cannot be called @current.");
        return 1;
      }
      if (project == "@current") {
        project = Config::Get().CurrentProject();
        if (project.empty()) {
          if ((command != "get") && (command != "confdir")) {
            logger::error("No project selected (srp select <project> selects one).");
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
    logger::error(R"(Exception "{}", exiting.)", e.what());
    return 1;
  } catch (...) {
    logger::error("Exception of unknown type, exiting.");
    return 1;
  }
  // the commands too: an unreadable or looping config folder makes the file system calls throw
  try {
    return RunCommand(command, project, property, setting);
  } catch (std::exception& e) {
    logger::error(R"(Exception "{}", exiting.)", e.what());
    return 1;
  } catch (...) {
    logger::error("Exception of unknown type, exiting.");
    return 1;
  }
}

int RunCommand(const string& command, const string& project, const string& property, const string& setting)
{
  // help, info, projects and settings: headings, then names and their explanations in two columns
  bool firstHeading = true;
  auto heading = [&firstHeading](const string& title) {
    if (!firstHeading) logger::print("");
    firstHeading = false;
    logger::print("{}{}{}", logger::begin_color(logger::Color::Yellow), title, logger::end_color());
  };
  auto rows = [](int column) {
    return [column](const string& name, const string& text) {
      logger::print("  {}{:<{}}{}  {}", logger::begin_color(logger::Color::Cyan), name, column, logger::end_color(), text);
    };
  };

  if (command == "help") {
    const auto row = rows(39);
    heading("Projects");
    row("srp projects [<project>]", "all projects or one: program, folders, plots, inputs");
    row("srp select <project>", "select a project");
    row("srp rename <project> <name>", "rename a project");
    row("srp remove <project>", "remove a project (its code is kept)");
    heading("New projects");
    row("srp init-cpp <project> [<dir>]", "minimal C++ skeleton");
    row("srp init-py <project> [<dir>]", "minimal Python skeleton");
    row("srp example-cpp <project> [<dir>]", "commented C++ examples with data");
    row("srp example-py <project> [<dir>]", "commented Python examples with data");
    row("srp add <project> <program> [<outdir>]", "register an existing program");
    heading("Project properties");
    row("srp get <project> <property>", "print a property");
    row("srp set <project> <property> <value>", "set a property");
    row("srp unset <project> <property>", "clear a property");
    heading("Project folders");
    row("srp confdir [<project>]", "print the config folder");
    row("srp cd [<project>]", "go to the program folder");
    row("srp edit [<project>]", "open the source files");
    row("srp out [<project>]", "open the output folder");
    heading("Plotting");
    row("plot <group> <name> [<mode>]", "create the matching plots");
    heading("Plot modes");
    row("show", "on screen, one after the other");
    row("spread", "on screen, all at once");
    row("pdf eps ps svg png jpg", "one image file per plot");
    row("gif[+<centiseconds>[++[<repeats>]]]", "all plots as one animation, ++ loops it");
    row("root html json xml", "one ROOT or web file per plot");
    row("macro", "one ROOT macro per plot");
    row("file", "all canvases in one ROOT file");
    row("data", "all input objects in one ROOT file");
    row("print", "the definitions in the terminal");
    row("list", "the names in the terminal");
    heading("Settings");
    row("srp settings", "show the settings");
    row("srp plotmode <mode>", "default plot mode (initially show)");
    row("srp color (bright | dark | off)", "colours of the messages");
    row("srp verbosity <level>", "lowest level of messages shown");
    row("srp matchmode (exact | contains)", "patterns match whole names or parts");
    row("srp matchcase (sensitive | insensitive)", "case sensitivity of the patterns");
    row("srp screenscale <factor>", "size of plots on screen (default 1)");
    row("srp bitmapscale <factor>", "resolution of png/jpg/gif (default 1)");
    heading("Maintenance");
    row("srp info", "installation and build details");
    row("srp update", "update SciRooPlot");
    row("srp update --reinstall", "fresh build of a source installation");
    row("srp clean", "remove projects whose code is gone");
    row("srp reset", "remove all projects (their code is kept)");
    row("srp help", "this overview");
    heading("Utilities");
    row("srp print <file>[:<path>]", "objects in a ROOT file (below a path)");
    row("srp open <file>", "open a ROOT file in a TBrowser");
    row("srp colorwheel [<file>]", "ROOT's colours with their names");
    heading("Environment");
    row("SCIROOPLOT_CONFIG_PATH", "config folder (default ~/.SciRooPlot)");
    heading("Arguments");
    row("<project>", "a project name (@current: selected project)");
    row("<property>", "program | outdir | <variable>");
    row("<variable>", "your own, read with GetProjectProperty");
    row("<group> <name>", "regular expressions for plot groups and names");
    row("<mode>", "one of the plot modes");
    row("<level>", "debug | log | info | warning | error | silent");
  } else if (command == "open") {
    const string& fileName = project;
    if (fileName.empty()) {
      logger::error("Specify a file: srp open <file>.");
      return 1;
    }
    std::error_code existsError;
    if (!std::filesystem::exists(fileName, existsError)) {
      logger::error("Could not find file {}.", fileName);
      return 1;
    }
    TFile* file{};
    {
      auto quiet = util::ignore_root_messages_below(kFatal);  // a file that is not a ROOT file is reported below
      file = TFile::Open(fileName.data());
    }
    if (!file || file->IsZombie()) {
      logger::error("Cannot read {}: srp open shows ROOT files.", fileName);
      return 1;
    }
    gROOT->SetWebDisplay("off");  // the classic browser, as the plots are shown in classic canvases (closing it ends the program)
    auto app = OpenWindowApplication("open");
    if (!app) return 1;
    TBrowser* browser = new TBrowser("TBrowser", file, "SciRooPlot", 1200, 1000);
    if (auto* rb = dynamic_cast<TRootBrowser*>(browser->GetBrowserImp())) {
      rb->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
    }
    app->Run();
  } else if (command == "colorwheel") {
    const string fileName = project;  // optional: save the wheel instead of showing it
    std::unique_ptr<TApplication> app;
    if (fileName.empty()) {
      app = OpenWindowApplication("colorwheel");
      if (!app) return 1;
    } else {
      gROOT->SetBatch(true);
    }
    auto* canvas = new TCanvas("colorwheel", "ROOT colour wheel", 800, 800);
    auto* wheel = new TColorWheel();
    wheel->SetCanvas(canvas);
    wheel->Draw();
    canvas->SetFillColor(kWhite);  // instead of the background colour TColorWheel gives the canvas
    canvas->SetFillStyle(1001);
    canvas->Update();
    if (!fileName.empty()) {
      canvas->SetCanvasSize(800, 800);  // the drawing area, without the window frame
      std::error_code removeError;
      std::filesystem::remove(fileName, removeError);  // ROOT reports a failed write but returns nothing: the file must be new
      canvas->SaveAs(fileName.data());
      std::error_code existsError;
      if (!std::filesystem::is_regular_file(fileName, existsError)) {
        logger::error("Could not write the colour wheel to {}.", fileName);
        return 1;
      }
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
    // named as the srp commands that change them
    const Config& config = Config::Get();
    const auto row = rows(15);
    heading("Settings");
    row("verbosity", Config::LogLevelName(config.Verbosity()));
    row("color", Config::ColorModeName(config.Color()));
    row("plotmode", config.PlotMode());
    row("matchmode", config.MatchContains() ? "contains" : "exact");
    row("matchcase", config.MatchCaseInsensitive() ? "insensitive" : "sensitive");
    row("screenscale", fmt::format("{}", config.ScreenScale()));
    row("bitmapscale", fmt::format("{}", config.BitmapScale()));
    row("current project", config.CurrentProject().empty() ? "none" : config.CurrentProject());
  } else if (command == "color") {
    if (auto colorMode = Config::ParseColorMode(project)) {
      Config::GetMutable().SetColor(*colorMode);
    } else {
      logger::error("Invalid color '{}' (bright | dark | off).", project);
      return 1;
    }
  } else if (command == "verbosity") {
    if (auto logLevel = Config::ParseLogLevel(project)) {
      Config::GetMutable().SetVerbosity(*logLevel);
    } else {
      logger::error("Invalid verbosity '{}' (debug | log | info | warning | error | silent).", project);
      return 1;
    }
  } else if (command == "plotmode") {
    if (project.empty()) {
      logger::error("Specify default plotmode.");
      return 1;
    }
    if (project != "list" && !is_valid_plot_mode(project)) {
      logger::error("Invalid plot mode '{}' (srp help lists the modes).", project);
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
      logger::error("Invalid match mode (exact | contains).");
      return 1;
    }
  } else if (command == "matchcase") {
    string matchCase = project;
    if (matchCase == "sensitive") {
      Config::GetMutable().SetMatchCaseInsensitive(false);
    } else if (matchCase == "insensitive") {
      Config::GetMutable().SetMatchCaseInsensitive(true);
    } else {
      logger::error("Invalid case sensitivity option for matching (sensitive | insensitive).");
      return 1;
    }
  } else if (command == "screenscale" || command == "bitmapscale") {
    const auto scale = Config::ParseScale(project);
    if (!scale) {
      logger::error("Invalid {} '{}' (positive number, e.g. 1.5).", command, project);
      return 1;
    }
    if (command == "screenscale") {
      Config::GetMutable().SetScreenScale(*scale);
    } else {
      Config::GetMutable().SetBitmapScale(*scale);
    }
  } else if (command == "info") {
    auto getenv = [](const char* name) {
      const char* value = std::getenv(name);
      return value ? value : "<not set>";
    };
    const auto row = rows(15);
    heading(fmt::format("SciRooPlot {}", SCIROOPLOT_VERSION));
    heading("Installation");
    row("prefix", getenv("SCIROOPLOT_ROOT"));
    row("programs", getenv("SCIROOPLOT_BIN"));
    row("library", getenv("SCIROOPLOT_LIB"));
    const std::filesystem::path sourceDir(getenv("SCIROOPLOT_SOURCE_DIR"));
    if (std::filesystem::exists(sourceDir)) row("source", std::filesystem::canonical(sourceDir).string());
    row("env script", getenv("SCIROOPLOT_ENV"));
    heading("Configuration");
    row("folder", Config::Get().Path().string() + (std::getenv("SCIROOPLOT_CONFIG_PATH") ? "  (SCIROOPLOT_CONFIG_PATH)" : "  (default)"));
    row("current project", Config::Get().CurrentProject().empty() ? "none" : Config::Get().CurrentProject());
    heading("Dependencies");
    row("ROOT", fmt::format("{}  {}", SCIROOPLOT_ROOT_VERSION, SCIROOPLOT_ROOT_EXECUTABLE));
    row("Python", getenv("SCIROOPLOT_PYTHON_EXECUTABLE"));
    string boostVersion = BOOST_LIB_VERSION;
    std::replace(boostVersion.begin(), boostVersion.end(), '_', '.');
    row("Boost", boostVersion);
    row("fmt", fmt::format("{}.{}.{}  {}", FMT_VERSION / 10000, FMT_VERSION / 100 % 100, FMT_VERSION % 100, SCIROOPLOT_FMT_INFO));
    row("pybind11", SCIROOPLOT_PYBIND11_INFO);
    heading("Build");
    row("type", SCIROOPLOT_BUILD_TYPE);
    row("commit", SCIROOPLOT_GIT_DESCRIBE);
    row("compiler", SCIROOPLOT_CXX_COMPILER);
  } else if (command == "confdir") {
    // also the folder of a project saved by its program but not yet added (env.sh reads plots.info there)
    const bool unknown = find_illegal_name_char(project) || (!Config::Get().Exists(project) && !std::filesystem::is_directory(Config::Get().ProjectPath(project)));
    if (!project.empty() && unknown) {
      logger::error("Cannot find project {}.", project);
      return 1;
    }
    std::cout << ((project.empty()) ? Config::Get().Path().string() : Config::Get().ProjectPath(project).string()) << std::endl;
    return 0;
  } else if (command == "rename") {
    if (!Config::GetMutable().Rename(project, property)) return 1;
  } else if (command == "clean") {
    if (!Config::GetMutable().Clean()) return 1;
  } else if (command == "reset") {
    if (!Config::GetMutable().Reset()) return 1;
  } else if (command == "projects") {
    if (!project.empty() && !Config::Get().Exists(project)) {
      logger::error("Cannot find project {}.", project);
      return 1;
    }
    // without a project: every project, the current one marked
    const vector<string> projects = project.empty() ? Config::Get().ProjectNames() : vector<string>{project};
    if (projects.empty() && Config::Get().ProjectsFileUnreadable()) {
      logger::error("Cannot list the projects: {} could not be read. Fix or remove it.", Config::Get().ProjectsFile());
      return 1;
    }
    if (projects.empty()) {
      logger::print("No projects yet: srp init-py <project>, srp init-cpp <project> or srp add <project> <program>.");
      return 0;
    }
    auto count = [](size_t n, const string& what) { return fmt::format("{} {}{}", n, what, (n == 1) ? "" : "s"); };
    bool readable = true;
    for (const string& name : projects) {
      // the tab-completion list has one line <name>,<group> per plot
      size_t nPlots = 0;
      std::set<string> groups;
      std::ifstream plotList(Config::Get().ProjectPath(name) / "tabcomp.csv");
      for (string line; std::getline(plotList, line);) {
        if (line.empty()) continue;
        ++nPlots;
        groups.insert(line.substr(line.rfind(',') + 1));
      }
      size_t nDataSources = 0;
      size_t nInputs = 0;
      if (std::filesystem::exists(Config::Get().DataSourcesFile(name))) {
        ptree dataSources;
        try {
          boost::property_tree::read_info(Config::Get().DataSourcesFile(name), dataSources);
          nDataSources = dataSources.size();
          for (const auto& [sourceName, dataSource] : dataSources)
            nInputs += dataSource.count("INPUT");
        } catch (const boost::property_tree::ptree_error& e) {
          logger::error("Cannot read {} ({}).", Config::Get().DataSourcesFile(name), e.what());
          readable = false;
        }
      }
      string lastSaved = "never";
      if (FileStat_t fileStat; gSystem->GetPathInfo(Config::Get().PlotsFile(name).data(), fileStat) == 0) {
        const std::time_t time = fileStat.fMtime;
        std::tm localTime{};
        char buffer[32];
        if (localtime_r(&time, &localTime) && std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime)) lastSaved = buffer;
      }
      // program and output folder first, then the user's variables as stored, then what the program saved
      auto properties = Config::Get().Properties(name);
      size_t width = 12;
      for (const auto& [property, value] : properties)
        width = std::max(width, property.size());
      const auto row = rows(static_cast<int>(width));
      const bool markCurrent = project.empty() && name == Config::Get().CurrentProject();
      heading(fmt::format("Project {}{}", name, markCurrent ? "  (current)" : ""));
      for (const char* property : {"program", "outdir"}) {
        auto it = properties.find(property);
        row(property, it == properties.end() || it->second.empty() ? "-" : it->second);
        if (it != properties.end()) properties.erase(it);
      }
      for (const auto& [property, value] : properties)
        if (!value.empty()) row(property, value);
      row("plots", fmt::format("{} in {}", nPlots, count(groups.size(), "group")));
      row("data sources", fmt::format("{} with {}", nDataSources, count(nInputs, "input")));
      row("last saved", lastSaved);
    }
    if (!readable) return 1;
  } else if (command == "remove") {
    if (!Config::GetMutable().Remove(project)) return 1;
  } else if (command == "select") {
    if (!Config::GetMutable().Select(project)) return 1;
  } else if (command == "add") {
    if (!Config::GetMutable().Add(project, property, setting)) return 1;
  } else if (command == "get") {
    if (project.empty()) {
      logger::error("Specify project or use @current.");
      return 1;
    }
    if (!Config::Get().Exists(project)) {
      logger::error("Cannot find project {}.", project);
      return 1;
    }
    if (property.empty()) {
      logger::error("Specify a property of {}.", project);
      return 1;
    }
    if (property == "program" || property == "outdir") {
      const string path = (property == "program") ? Config::Get().Program(project) : Config::Get().OutputDir(project);
      if (path.empty() && !Config::Get().Property(project, property).empty()) return 1;  // the path could not be expanded (reported)
      std::cout << path << std::endl;
    } else {
      std::cout << Config::Get().Property(project, property) << std::endl;
    }
  } else if (command == "unset") {
    if (!Config::GetMutable().SetProperty(project, property, "")) return 1;
  } else if (command == "set") {
    if (Config::Get().Exists(project) && !property.empty() && setting.empty()) {
      logger::error("Specify new setting for {}.", project);
      return 1;
    }
    if (!Config::GetMutable().SetProperty(project, property, setting)) return 1;
  } else {
    const vector<string> shellCommands = {"update", "edit", "cd", "out", "init-py", "init-cpp", "example-py", "example-cpp"};
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
  // split after the .root ending, as the inputs of data sources (a directory in the path may contain ':')
  // (with '.root:' also in a directory name: after an existing file, an existing path is not split)
  std::error_code ec;
  size_t split = inputPath.find(".root:");
  for (size_t pos = split; pos != string::npos; pos = inputPath.find(".root:", pos + 1)) {
    if (std::filesystem::is_regular_file(inputPath.substr(0, pos + 5), ec)) {
      split = pos;
      break;
    }
    if (inputPath.find(".root:", pos + 1) == string::npos && std::filesystem::exists(inputPath, ec)) split = string::npos;
  }
  if (split != string::npos) {
    fileName = inputPath.substr(0, split + 5);
    startPath = inputPath.substr(split + 6);
  }
  while (!startPath.empty() && startPath.back() == '/')
    startPath.pop_back();
  if (fileName.empty()) {
    logger::error("Specify a file: srp print <file>[:<path>].");
    return false;
  }
  std::error_code existsError;
  if (!std::filesystem::exists(fileName, existsError)) {
    logger::error("Could not find file {}.", fileName);
    return false;
  }
  std::unique_ptr<TFile> inputFile;
  {
    auto quiet = util::ignore_root_messages_below(kFatal);  // a file that is not a ROOT file is reported below
    inputFile.reset(TFile::Open(fileName.data(), "READ"));
  }
  if (!inputFile || inputFile->IsZombie()) {
    logger::error("Cannot read {}: srp print lists the contents of ROOT files.", fileName);
    return false;
  }

  // a path goes down as the reader does: the latest cycle of a directory name, the value of a map under its key
  auto findObject = [](TObject* root, const string& path) -> TObject* {
    TObject* current = root;
    size_t pos = 0;
    while (current && pos < path.size()) {
      auto next = path.find('/', pos);
      auto part = path.substr(pos, next == string::npos ? string::npos : next - pos);

      if (current->InheritsFrom(TDirectory::Class())) {
        current = static_cast<TDirectory*>(current)->Get(part.data());
      } else if (util::is_container(current->IsA())) {
        TObject* found{};
        util::for_each_item(current, [&](const string& name, TObject* item) {
          if (name == part) found = item;
          return found != nullptr;
        });
        current = found;
      } else {
        return nullptr;
      }
      pos = next == string::npos ? path.size() : next + 1;
    }
    return current;
  };

  TObject* root = inputFile.get();
  if (!startPath.empty()) {
    root = findObject(root, startPath);
    if (!root) {
      logger::error("Path not found: {}.", startPath);
      return false;
    }
  }

  auto printBranches = [](TObject* object) {
    auto* tree = dynamic_cast<TTree*>(object);
    if (!tree) return;
    TIter next(tree->GetListOfBranches());
    while (auto* branch = static_cast<TBranch*>(next())) {
      TClass* branchClass = nullptr;
      EDataType branchType = kOther_t;
      branch->GetExpectedType(branchClass, branchType);
      logger::print("  -> {} ({})", branch->GetName(), branchClass ? branchClass->GetName() : TDataType::GetTypeName(branchType));
    }
  };

  std::function<void(TObject*, const string&)> loopData =
    [&](TObject* object, const string& path) {
      if (!util::is_container(object->IsA())) {
        // listed by the name under which it is stored, which is the one to access it with (the object's own name can differ)
        const auto slashPos = path.rfind('/');
        if (slashPos != string::npos) logger::print("{}/", path.substr(0, slashPos));
        logger::print("- [{}] {}", object->ClassName(), path.substr((slashPos == string::npos) ? 0 : slashPos + 1));
        printBranches(object);
        return;
      }
      bool printedPath = false;
      auto printPath = [&]() {
        if (!printedPath && !path.empty()) logger::print("{}/", path);
        printedPath = true;
      };
      // the objects of this level first, then the containers: listed after a container, they would look as if inside it
      vector<std::tuple<TObject*, string, bool>> containers;
      auto listItem = [&](TObject* obj, const string& name, bool ownsObj) {
        if (util::is_container(obj->IsA())) {
          containers.emplace_back(obj, name, ownsObj);
          return;
        }
        printPath();
        logger::print("- [{}] {}", obj->ClassName(), name);
        printBranches(obj);
        if (ownsObj) delete obj;
      };
      if (auto* directory = dynamic_cast<TDirectory*>(object)) {
        // the latest cycle of a name (listed first), as the reader takes it: one for data, one for a directory, list or folder
        std::set<std::tuple<string, bool>> listed;
        for (auto* key : TRangeDynCast<TKey>(directory->GetListOfKeys())) {
          if (!key) continue;
          const string name = key->GetName();
          const TClass* keyClass = TClass::GetClass(key->GetClassName());
          if (!listed.emplace(name, util::is_container(keyClass)).second) continue;
          // a key of a class that is no TObject (an RNTuple, a std::vector) cannot be read as one
          if (keyClass && !keyClass->IsTObject()) {
            printPath();
            logger::print("- [{}] {}", key->GetClassName(), name);
            continue;
          }
          TObject* obj = key->ReadObj();
          if (!obj) {
            logger::warning("Skipping key {} of type {}; the class is not available.", name, key->GetClassName());
            continue;
          }
          listItem(obj, name, true);
        }
      } else {
        util::for_each_item(object, [&](const string& name, TObject* item) {
          listItem(item, name, false);
          return false;
        });
      }
      for (auto& [container, name, ownsContainer] : containers) {
        loopData(container, path.empty() ? name : path + "/" + name);
        if (ownsContainer) delete container;
      }
    };
  loopData(root, startPath);
  return true;
}

// the application for a command that opens a window, or null (reported) without a graphical display: ROOT then falls back to batch
// mode after its own errors, and the application would run forever without a window to close
std::unique_ptr<TApplication> OpenWindowApplication(const string& command)
{
  gROOT->SetBatch(false);
  std::unique_ptr<TApplication> app;
  {
    auto quiet = util::ignore_root_messages_below(kFatal);
    app = std::make_unique<TApplication>("SciRooPlot", nullptr, nullptr);
  }
  if (gROOT->IsBatch()) {
    logger::error("srp {} needs a graphical display (is DISPLAY set?).", command);
    return nullptr;
  }
  return app;
}
