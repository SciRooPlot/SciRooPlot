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

#include <ROOT/RCsvDS.hxx>
#include <ROOT/RDataFrame.hxx>
#include <TApplication.h>
#include <TBranch.h>
#include <TBrowser.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TClass.h>
#include <TColorWheel.h>
#include <TDataType.h>
#include <TEfficiency.h>
#include <TF2.h>
#include <TF3.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TH1.h>
#include <TH2Poly.h>
#include <THashList.h>
#include <THnBase.h>
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
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "Config.h"
#include "Validation.h"
#include "util/HepData.h"
#include "util/Paths.h"
#include "util/RootContainers.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"
#include "util/Tables.h"

using boost::property_tree::ptree;
using std::string;
using std::vector;

using namespace SciRooPlot;
namespace po = boost::program_options;

// the settings changed by srp <setting> <value>, in the order srp settings shows them
constexpr const char* kSettings[] = {"verbosity", "color", "plotmode", "matchmode", "matchcase", "screenscale", "bitmapscale", "cache"};

int run_command(const string& command, const string& project, const string& property, const string& setting);
string setting_value(const string& setting);
bool print_root_file_contents(const string& inputPath);
bool print_table_contents(const string& fileName);
bool print_data_source(const string& dataSource, const string& path);
bool is_data_source_of(const string& project, const string& dataSource);
TObject* find_in_file(TObject* root, const string& path);
void walk_contents(TObject* object, const string& path, const std::function<void(const string&, const string&, TObject*)>& onObject);
std::unique_ptr<TApplication> open_window_application(const string& command);

//**************************************************************************************************
/**
 * srp: reads the command and its arguments, then runs it.
 */
//**************************************************************************************************
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

    if (argc > 5) {
      logger::error("Too many arguments (srp help lists the commands; quote a value containing spaces).");
      return 1;
    }
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
    // the most words a command takes (with the command itself), as srp help shows them
    static const std::map<string, int> maxWords{{"help", 1}, {"info", 1}, {"settings", 1}, {"clean", 1}, {"reset", 1}, {"projects", 2}, {"select", 2}, {"remove", 2}, {"confdir", 2}, {"print", 2}, {"plot", 3}, {"open", 2}, {"colorwheel", 2}, {"rename", 3}, {"get", 3}, {"unset", 3}, {"cache", 3}, {"add", 4}, {"set", 4}, {"plotmode", 2}, {"color", 2}, {"verbosity", 2}, {"matchmode", 2}, {"matchcase", 2}, {"screenscale", 2}, {"bitmapscale", 2}};
    if (auto words = maxWords.find(command); words != maxWords.end() && argc - 1 > words->second) {
      logger::error("Too many arguments for srp {} (srp help shows its arguments; quote a value containing spaces).", command);
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
          logger::error("No project selected (srp select <project> selects one).");
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
    return run_command(command, project, property, setting);
  } catch (std::exception& e) {
    logger::error(R"(Exception "{}", exiting.)", e.what());
    return 1;
  } catch (...) {
    logger::error("Exception of unknown type, exiting.");
    return 1;
  }
}

//**************************************************************************************************
/**
 * Help, info, projects and settings: headings, then names and their explanations in two columns.
 */
//**************************************************************************************************
struct listing_t {
  bool firstHeading = true;
  void Heading(const string& title)
  {
    if (!firstHeading) logger::print("");
    firstHeading = false;
    logger::print("{}{}{}", logger::begin_color(logger::Color::Yellow), title, logger::end_color());
  }
  static std::function<void(const string&, const string&)> Rows(int column)
  {
    return [column](const string& name, const string& text) {
      logger::print("  {}{:<{}}{}  {}", logger::begin_color(logger::Color::Cyan), name, column, logger::end_color(), text);
    };
  }
};

//**************************************************************************************************
/**
 * srp help.
 */
//**************************************************************************************************
int command_help(listing_t& listing)
{
  auto heading = [&listing](const string& title) { listing.Heading(title); };
  const auto rows = &listing_t::Rows;
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
  row("srp unset <project> <property>", "clear a property (a setting: its default)");
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
  row("srp settings", "show the settings (a setting without value: its value)");
  row("srp plotmode <mode>", "default plot mode (initially show)");
  row("srp color (bright | dark | off)", "colours of the messages");
  row("srp verbosity <level>", "lowest level of messages shown");
  row("srp matchmode (exact | contains)", "patterns match whole names or parts");
  row("srp matchcase (sensitive | insensitive)", "case sensitivity of the patterns");
  row("srp screenscale <factor>", "size of plots on screen (default 1)");
  row("srp bitmapscale <factor>", "resolution of png/jpg/gif (default 1)");
  row("srp cache (on [<folder>] | off)", "local copies of remote inputs (default off)");
  heading("Maintenance");
  row("srp info", "installation and build details");
  row("srp update", "update SciRooPlot");
  row("srp reinstall", "reinstall SciRooPlot from scratch");
  row("srp clean", "remove projects whose code is gone");
  row("srp reset", "remove all projects (their code is kept)");
  row("srp help", "this overview");
  heading("Utilities");
  row("srp print <file>[:<path>]", "objects in a ROOT file (below a path), one object with details");
  row("srp print hepdata:<record>[:<table>]", "data names of a HEPData record (or downloaded file)");
  row("srp print <table file>", "data name, rows and column types of a table");
  row("srp print <data source>[:<path>]", "objects of a data source of the current project as read");
  row("srp plot <file>[:<path>] <name>", "show one object of a ROOT file on screen");
  row("srp open <file>", "open a ROOT file in a TBrowser");
  row("srp colorwheel [<file>]", "ROOT's colours with their names");
  heading("Environment");
  row("SCIROOPLOT_CONFIG_PATH", "config folder (default ~/.SciRooPlot)");
  heading("Arguments");
  row("<project>", "a project name (@current: selected project)");
  row("<property>", "program | outdir | <setting>");
  row("<setting>", "declared and read by the program with Setting");
  row("<group> <name>", "regular expressions for plot groups and names");
  row("<mode>", "one of the plot modes");
  row("<level>", "debug | hint | log | info | warning | error | silent");
  return 0;
}

//**************************************************************************************************
/**
 * srp open.
 */
//**************************************************************************************************
int command_open(const string& project)
{
  if (project.empty()) {
    logger::error("Specify a file: srp open <file>.");
    return 1;
  }
  // ~ and environment variables as in the inputs of data sources
  const auto expanded = util::expand_path(project);
  if (!expanded) return 1;  // reported
  const string& fileName = *expanded;
  std::error_code existsError;
  if (util::is_missing_local_file(fileName, existsError)) {  // a remote file is opened through ROOT, as the inputs of data sources
    logger::error("Could not find file {}.", fileName);
    return 1;
  }
  TFile* file{};
  {
    auto quiet = util::ignore_root_messages_below(kFatal);  // a file that is not a ROOT file is reported below
    file = util::open_input_file(fileName).release();
  }
  if (!file || file->IsZombie()) {
    logger::error("Cannot read {}: srp open shows ROOT files.", fileName);
    return 1;
  }
  gROOT->SetWebDisplay("off");  // the classic browser, as the plots are shown in classic canvases (closing it ends the program)
  auto app = open_window_application("open");
  if (!app) return 1;
  TBrowser* browser = new TBrowser("TBrowser", file, "SciRooPlot", 1200, 1000);
  if (auto* rb = dynamic_cast<TRootBrowser*>(browser->GetBrowserImp())) {
    rb->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
  }
  app->Run();
  return 0;
}

//**************************************************************************************************
/**
 * srp plot.
 */
//**************************************************************************************************
int command_plot(const string& project, const string& property)
{
  // one object shown in the shipped look, without a project: the file (or a folder in it) is the data source, the name a path below it
  if (project.empty() || property.empty()) {
    logger::error("Specify a file and an object: srp plot <file>[:<path>] <name>.");
    return 1;
  }
  if (util::is_hepdata_input(project)) {
    logger::error("srp plot shows objects of ROOT files: srp print {} lists the data names of the record, which a plotting program reads from a data source {}.", project, project);
    return 1;
  }
  const auto input = util::expand_path(project);  // as in the inputs of data sources
  if (!input) return 1;                           // reported
  const auto fileAndFolder = util::split_input_path(*input);
  std::error_code existsError;
  if (util::is_missing_local_file(fileAndFolder[0], existsError)) {
    logger::error("Could not find file {}.", fileAndFolder[0]);
    return 1;
  }
  // 2d data gets the base plot with colour scale; data not found is reported by the plot
  bool is2d{};
  {
    std::unique_ptr<TFile> file;
    {
      auto quiet = util::ignore_root_messages_below(kFatal);  // a file that is not a ROOT file is reported below
      file = util::open_input_file(fileAndFolder[0]);
    }
    if (!file || file->IsZombie()) {
      logger::error("Cannot read {}: srp plot shows objects of ROOT files.", fileAndFolder[0]);
      return 1;
    }
    const string folder = (fileAndFolder.size() > 1) ? fileAndFolder[1] + "/" : "";
    TObject* object = find_in_file(file.get(), folder + property);
    // what the reader takes apart (folders, stacks) or reads through projections (trees, n-dimensional histograms) cannot be shown as is
    const string printLine = fmt::format("srp print {}:{}", project, property);
    if (object && util::is_container(object->IsA())) {
      logger::error("{} holds other objects ({}): {} lists them, srp plot shows one of them.", property, object->ClassName(), printLine);
      return 1;
    }
    if (object && (object->InheritsFrom(TTree::Class()) || object->InheritsFrom(THnBase::Class()) || string(object->ClassName()) == "ROOT::RNTuple")) {
      logger::error("{} is a {}: srp plot shows histograms, graphs and functions; {} lists its {}, and a plotting program projects it (Project1D, ...).", property, object->ClassName(), printLine, (object->InheritsFrom(THnBase::Class())) ? "axes" : "columns");
      return 1;
    }
    if (auto* hist = dynamic_cast<TH1*>(object)) {
      is2d = hist->GetDimension() > 1;
    } else if (auto* function = dynamic_cast<TF1*>(object)) {
      is2d = function->GetNdim() > 1;
    } else if (auto* efficiency = dynamic_cast<TEfficiency*>(object)) {
      is2d = efficiency->GetDimension() > 1;
    } else {
      is2d = dynamic_cast<TGraph2D*>(object);
    }
  }
  // the plot is named after the object, with the characters a plot name cannot hold as _
  string plotName = property;
  for (auto& c : plotName) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') c = '_';
  }
  const string basePlot = (is2d) ? "2d" : "1d";
  PlotManager plotManager;
  plotManager.AddDataSource("file", *input);
  plotManager.AddBasePlot(PlotManager::MakeBasePlot(basePlot));
  Plot plot(plotName, "srp", basePlot);
  plot[1].AddData(property, "file");
  plotManager.AddPlot(plot);
  if (!plotManager.GeneratePlots("show")) return 1;
  return 0;
}

//**************************************************************************************************
/**
 * srp colorwheel.
 */
//**************************************************************************************************
int command_colorwheel(const string& project)
{
  // optional: save the wheel instead of showing it (~ and environment variables expanded, as ROOT would when saving)
  const auto expanded = util::expand_path(project);
  if (!expanded) return 1;  // reported
  const string fileName = *expanded;
  if (!fileName.empty()) {
    // ROOT writes PostScript for any other ending (it compares them case-sensitively: .PNG too; .tif too, .tiff it cannot write)
    static const std::set<string> imageEndings = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".xpm", ".svg", ".pdf", ".eps", ".ps"};
    if (!imageEndings.count(std::filesystem::path(fileName).extension().string())) {
      logger::error("Cannot write the colour wheel to {}: give a file name ending in .png, .pdf, .svg, .eps, .jpg or .gif (in lower case).", fileName);
      return 1;
    }
  }
  std::unique_ptr<TApplication> app;
  if (fileName.empty()) {
    app = open_window_application("colorwheel");
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
    {
      auto quiet = util::ignore_root_messages_below(kWarning);  // ROOT's "Info in <TCanvas::Print>: ... has been created"
      canvas->SaveAs(fileName.data());
    }
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
  return 0;
}

//**************************************************************************************************
/**
 * srp settings.
 */
//**************************************************************************************************
int command_settings(listing_t& listing)
{
  auto heading = [&listing](const string& title) { listing.Heading(title); };
  const auto rows = &listing_t::Rows;
  // named as the srp commands that change them
  const auto row = rows(15);
  heading("Settings");
  for (const char* setting : kSettings) {
    row(setting, setting_value(setting));
  }
  row("current project", Config::Get().CurrentProject().empty() ? "none" : Config::Get().CurrentProject());
  return 0;
}

//**************************************************************************************************
/**
 * srp info.
 */
//**************************************************************************************************
int command_info(listing_t& listing)
{
  auto heading = [&listing](const string& title) { listing.Heading(title); };
  const auto rows = &listing_t::Rows;
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
  return 0;
}

//**************************************************************************************************
/**
 * srp projects.
 */
//**************************************************************************************************
int command_projects(const string& project, listing_t& listing)
{
  auto heading = [&listing](const string& title) { listing.Heading(title); };
  const auto rows = &listing_t::Rows;
  if (!project.empty() && !Config::Get().Exists(project)) {
    Config::Get().ReportUnknown(project);
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
      if (time == 0) {  // made older than the program by srp set
        lastSaved = "before a setting changed (plot runs the program again)";
      } else if (localtime_r(&time, &localTime) && std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime)) {
        lastSaved = buffer;
      }
    }
    // program and output folder first, then the settings the program declares, values left from settings it no longer declares,
    // then what the program saved
    auto properties = Config::Get().Properties(name);
    const auto declared = Config::Get().DeclaredSettings(name);
    if (!declared) readable = false;
    const ptree settings = declared.value_or(ptree{});
    size_t width = 12;
    for (const auto& [property, value] : properties)
      width = std::max(width, property.size());
    for (const auto& [setting, declaration] : settings)
      width = std::max(width, setting.size());
    const auto row = rows(static_cast<int>(width));
    const bool markCurrent = project.empty() && name == Config::Get().CurrentProject();
    heading(fmt::format("Project {}{}", name, markCurrent ? "  (current)" : ""));
    for (const char* property : {"program", "outdir"}) {
      auto it = properties.find(property);
      row(property, it == properties.end() || it->second.empty() ? "-" : it->second);
      if (it != properties.end()) properties.erase(it);
    }
    // the value (with the default where it differs), the descriptions aligned behind
    vector<std::tuple<string, string, string>> settingRows;
    size_t valueWidth = 0;
    for (const auto& [setting, declaration] : settings) {
      const string defaultValue = declaration.get<string>("default", "");
      auto it = properties.find(setting);
      const string value = (it == properties.end() || it->second.empty()) ? defaultValue : it->second;
      // a value the declaration no longer allows: the program uses the default
      const string allowed = Config::AllowedSettingValues(declaration, value);
      string shown;
      if (!allowed.empty()) {
        shown = (defaultValue.empty() ? "-" : defaultValue) + "  (the value set, " + value + ", is not " + allowed + ")";
      } else {
        shown = (value.empty() ? "-" : value) + ((value == defaultValue) ? "" : "  (default " + (defaultValue.empty() ? "-" : defaultValue) + ")");
      }
      valueWidth = std::max(valueWidth, shown.size());
      settingRows.emplace_back(setting, shown, declaration.get<string>("description", ""));
      if (it != properties.end()) properties.erase(it);
    }
    for (const auto& [setting, shown, description] : settingRows)
      row(setting, description.empty() ? shown : fmt::format("{:<{}}  {}", shown, valueWidth, description));
    for (const auto& [property, value] : properties)
      if (!value.empty()) row(property, value + "  (not a setting of the program: srp unset removes it)");
    row("plots", fmt::format("{} in {}", nPlots, util::plural(groups.size(), "group")));
    row("data sources", fmt::format("{} with {}", nDataSources, util::plural(nInputs, "input")));
    row("last saved", lastSaved);
  }
  if (!readable) return 1;
  return 0;
}

//**************************************************************************************************
/**
 * srp get.
 */
//**************************************************************************************************
int command_get(const string& project, const string& property)
{
  if (project.empty()) {
    logger::error("Specify project or use @current.");
    return 1;
  }
  if (!Config::Get().Exists(project)) {
    Config::Get().ReportUnknown(project);
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
    const auto declaration = Config::Get().DeclaredSetting(project, property);
    if (!declaration) return 1;
    const string value = Config::Get().Property(project, property);
    const string defaultValue = declaration->get<string>("default", "");
    // a value the declaration no longer allows: the program uses the default
    if (const string allowed = Config::AllowedSettingValues(*declaration, value); !value.empty() && !allowed.empty()) {
      logger::warning("The value '{}' set for setting {} of project {} is not {}: the default '{}' is used.", value, property, project, allowed, defaultValue);
      std::cout << defaultValue << std::endl;
    } else {
      std::cout << (value.empty() ? defaultValue : value) << std::endl;
    }
  }
  return 0;
}

//**************************************************************************************************
/**
 * Runs an srp command (0 on success, 1 with an error reported).
 */
//**************************************************************************************************
int run_command(const string& command, const string& project, const string& property, const string& setting)
{
  listing_t listing;  // help, info, projects and settings

  if (command == "help") {
    return command_help(listing);
  } else if (command == "open") {
    return command_open(project);
  } else if (command == "plot") {
    return command_plot(project, property);
  } else if (command == "colorwheel") {
    return command_colorwheel(project);
  } else if (command == "print" && util::is_hepdata_input(project)) {
    if (!util::print_hepdata(project, Config::Get().CacheFolder())) return 1;
  } else if (command == "print" && util::str_ends_with(project, util::kTableFileEndings)) {
    const auto fileName = util::expand_path(project);
    if (!fileName || !print_table_contents(*fileName)) return 1;
  } else if (command == "print") {
    const auto fileName = util::expand_path(project);  // as in the inputs of data sources
    // a data source of the current project, unless a file of that name exists
    const string sourceName = project.substr(0, project.find(':'));
    std::error_code existsError;
    const bool isFile = fileName && !util::split_input_path(*fileName)[0].empty() && std::filesystem::is_regular_file(util::split_input_path(*fileName)[0], existsError);
    if (!isFile && !Config::Get().CurrentProject().empty() && is_data_source_of(Config::Get().CurrentProject(), sourceName)) {
      if (!print_data_source(sourceName, (project.find(':') == string::npos) ? "" : project.substr(project.find(':') + 1))) return 1;
    } else if (!fileName || !print_root_file_contents(*fileName)) {
      return 1;
    }
  } else if (command == "settings") {
    return command_settings(listing);
  } else if (std::find(std::begin(kSettings), std::end(kSettings), command) != std::end(kSettings)) {
    // without a value: the current one
    if (project.empty()) {
      std::cout << setting_value(command) << std::endl;
      return 0;
    }
    // srp cache on <folder>: the folder is the argument after on
    if (command == "cache" && !setting.empty()) {
      logger::error("Invalid cache '{} {} {}' (on | on <absolute folder> | off).", project, property, setting);
      return 1;
    }
    const string value = (command == "cache" && !property.empty()) ? project + " " + property : project;
    if (!Config::GetMutable().SetSetting(command, value)) return 1;
  } else if (command == "info") {
    return command_info(listing);
  } else if (command == "confdir") {
    // also the folder of a project saved by its program but not yet added (env.sh reads plots.info there)
    const bool unknown = find_illegal_name_char(project) || (!Config::Get().Exists(project) && !std::filesystem::is_directory(Config::Get().ProjectPath(project)));
    if (!project.empty() && unknown) {
      Config::Get().ReportUnknown(project);
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
    return command_projects(project, listing);
  } else if (command == "remove") {
    if (!Config::GetMutable().Remove(project)) return 1;
  } else if (command == "select") {
    if (!Config::GetMutable().Select(project)) return 1;
  } else if (command == "add") {
    if (!Config::GetMutable().Add(project, property, setting)) return 1;
  } else if (command == "get") {
    return command_get(project, property);
  } else if (command == "unset") {
    if (!Config::GetMutable().SetProperty(project, property, "")) return 1;
  } else if (command == "set") {
    if (Config::Get().Exists(project) && !property.empty() && setting.empty()) {
      logger::error("Specify a value for {} of {}.", property, project);
      return 1;
    }
    if (!Config::GetMutable().SetProperty(project, property, setting)) return 1;
  } else {
    const vector<string> shellCommands = {"update", "reinstall", "edit", "cd", "out", "init-py", "init-cpp", "example-py", "example-cpp"};
    if (std::find(shellCommands.begin(), shellCommands.end(), command) != shellCommands.end()) {
      logger::error("Source environment script to use this feature.");
    } else {
      logger::error("Unknown command {} (srp help lists the commands).", command);
    }
    return 1;
  }
  return Config::GetMutable().Save() ? 0 : 1;
}

//**************************************************************************************************
/**
 * The current value of a setting of kSettings, as srp settings shows it.
 */
//**************************************************************************************************
string setting_value(const string& setting)
{
  const Config& config = Config::Get();
  if (setting == "verbosity") return Config::LogLevelName(config.VerbositySetting());
  if (setting == "color") return Config::ColorModeName(config.Color());
  if (setting == "plotmode") return config.PlotMode();
  if (setting == "matchmode") return config.MatchContains() ? "contains" : "exact";
  if (setting == "matchcase") return config.MatchCaseInsensitive() ? "insensitive" : "sensitive";
  if (setting == "screenscale") return fmt::format("{}", config.ScreenScale());
  if (setting == "cache") return config.CacheSettingValue();
  return fmt::format("{}", config.BitmapScale());
}

//**************************************************************************************************
/**
 * srp print <table file>: the data name of a table, its rows, delimiter and each column with the type the reader of the data sources
 * gives it (false with an error).
 */
//**************************************************************************************************
bool print_table_contents(const string& fileName)
{
  std::error_code existsError;
  if (!std::filesystem::exists(fileName, existsError)) {
    logger::error("Could not find file {}.", fileName);
    return false;
  }
  const auto options = util::table_options(fileName);
  if (!options) return false;
  ROOT::RDataFrame table = ROOT::RDF::FromCSV(fileName, *options);
  ULong64_t nRows = 0;
  try {
    nRows = *table.Count();  // reads every value: a cell not of its column's type throws
  } catch (const std::exception& e) {
    // ROOT's reason is the bare name of the conversion that failed (stoll, stod): left out, it says nothing to a user
    const string reason = util::without_final_period(e.what());
    logger::error("Cannot read table {}: it contains a value that does not match the type of its column{}.", fileName, (reason.find(' ') == string::npos) ? "" : " (" + reason + ")");
    return false;
  }
  const char delimiter = options->fDelimiter;
  const string delimiterName = (delimiter == '\t') ? "tabs" : ((delimiter == ' ') ? "spaces" : fmt::format("'{}'", delimiter));
  logger::print("- [table] {}: {}, separated by {}", std::filesystem::path(fileName).stem().string(), util::plural(nRows, "row"), delimiterName);
  const ROOT::RDF::RCsvDS columns(fileName, *options);  // in the order of the file
  for (const auto& column : columns.GetColumnNames())
    logger::print("  -> {} ({})", column, columns.GetTypeName(column));
  return true;
}

//**************************************************************************************************
/**
 * The object at a path below a file, directory or collection, found as the reader does: the latest directory, folder or collection of a
 * name (a newer key of the name holding data does not hide it), the value of a map under its key (null if there is none).
 */
//**************************************************************************************************
TObject* find_in_file(TObject* root, const string& path)
{
  TObject* current = root;
  size_t pos = 0;
  while (current && pos < path.size()) {
    auto next = path.find('/', pos);
    auto part = path.substr(pos, next == string::npos ? string::npos : next - pos);

    if (auto* directory = dynamic_cast<TDirectory*>(current)) {
      TKey* key = util::find_container_key(directory, part);
      TClass* keyClass = (key) ? util::readable_class(key) : nullptr;
      if (!keyClass) return nullptr;
      current = (keyClass->InheritsFrom(TDirectory::Class())) ? static_cast<TObject*>(util::open_directory(directory, key)) : key->ReadObj();
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
}

//**************************************************************************************************
/**
 * The objects below a file, directory or collection as the reader sees them: each object that is no container with its path (as stored,
 * the name to access it with) and class, the objects of a level before the contents of its containers; of a directory the latest cycle of
 * a name. The object is null for a key of a class that is no TObject (an RNTuple, a std::vector).
 */
//**************************************************************************************************
void walk_contents(TObject* object, const string& path, const std::function<void(const string&, const string&, TObject*)>& onObject)
{
  auto pathOf = [&](const string& name) { return path.empty() ? name : path + "/" + name; };
  vector<std::tuple<TObject*, string, bool>> containers;
  auto listItem = [&](TObject* obj, const string& name, bool ownsObj) {
    if (util::is_container(obj->IsA())) {
      containers.emplace_back(obj, name, ownsObj);
      return;
    }
    onObject(pathOf(name), obj->ClassName(), obj);
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
      if (keyClass && !keyClass->IsTObject()) {
        onObject(pathOf(name), key->GetClassName(), nullptr);
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
    walk_contents(container, pathOf(name), onObject);
    if (ownsContainer) delete container;
  }
}

//**************************************************************************************************
/**
 * Whether a project has a data source of this name (in its stored data sources).
 */
//**************************************************************************************************
bool is_data_source_of(const string& project, const string& dataSource)
{
  std::error_code existsError;
  if (dataSource.empty() || !std::filesystem::exists(Config::Get().DataSourcesFile(project), existsError)) return false;
  ptree dataSources;
  try {
    boost::property_tree::read_info(Config::Get().DataSourcesFile(project), dataSources);
  } catch (const boost::property_tree::ptree_error&) {
    return false;
  }
  return dataSources.find(dataSource) != dataSources.not_found();
}

//**************************************************************************************************
/**
 * srp print <data source>[:<path>]: the contents of a data source of the current project as the reader sees them (its inputs expanded as
 * when reading): each object once, with a note where it is in several inputs (trees and RNTuples are chained over them, of other data the
 * first input is read) or not in all; tables by their data name. A path naming one object gives its details as for one file (of the first
 * input holding it; a chained tree with its entries in all). The inputs themselves only at verbosity debug.
 */
//**************************************************************************************************
bool print_data_source(const string& dataSource, const string& path)
{
  PlotManager plotManager(Config::Get().CurrentProject());
  plotManager.LoadDataSources();
  const vector<string> inputs = plotManager.ExpandInputs(dataSource);
  if (inputs.empty()) {
    logger::error("Data source {} has no inputs that can be read.", dataSource);
    return false;
  }
  struct found_t {
    string className;
    vector<string> inputs;  // holding it, in their order
  };
  vector<string> order;
  std::map<string, found_t> found;
  auto record = [&](const string& objectPath, const string& className, const string& input) {
    auto [it, isNew] = found.try_emplace(objectPath, found_t{className, {}});
    if (isNew) order.push_back(objectPath);
    it->second.inputs.push_back(input);
  };
  size_t nRootInputs{};  // the notes count the inputs that can hold ROOT objects
  for (const auto& input : inputs) {
    logger::debug("  input {}", input);
    const string expanded = util::expand_path(input).value_or(input);  // what the details below open again
    const auto fileAndFolder = util::split_input_path(expanded);
    if (util::str_ends_with(fileAndFolder[0], util::kTableFileEndings)) {
      if (path.empty()) record(std::filesystem::path(fileAndFolder[0]).stem().string(), "table", expanded);
      continue;
    }
    ++nRootInputs;
    std::unique_ptr<TFile> file;
    {
      auto quiet = util::ignore_root_messages_below(kFatal);
      file = util::open_input_file(fileAndFolder[0]);
    }
    if (!file || file->IsZombie()) {
      logger::warning("Cannot open input {}.", input);
      continue;
    }
    const string folder = (fileAndFolder.size() > 1) ? fileAndFolder[1] : "";
    TObject* root = find_in_file(file.get(), folder);
    TObject* start = (root && !path.empty()) ? find_in_file(root, path) : root;
    if (!start) continue;
    if (!util::is_container(start->IsA())) {
      record(path, start->ClassName(), expanded);
    } else {
      walk_contents(start, path, [&](const string& objectPath, const string& className, TObject*) { record(objectPath, className, expanded); });
    }
  }
  if (order.empty()) {
    logger::error("Path not found in data source {}: {}.", dataSource, path);
    return false;
  }
  auto isChained = [](const string& className) { return className == "TTree" || className == "TNtuple" || className == "TNtupleD" || className == "ROOT::RNTuple" || className == "ROOT::Experimental::RNTuple"; };
  logger::print("data source {}: {}", dataSource, util::plural(inputs.size(), "input"));
  auto note = [&](const found_t& object) -> string {
    const size_t n = object.inputs.size();
    if (nRootInputs <= 1 || object.className == "table") return "";
    if (isChained(object.className)) return (n == 1) ? fmt::format("in 1 of {} inputs", nRootInputs) : fmt::format("chained over {}{} inputs", n, (n < nRootInputs) ? fmt::format(" of {}", nRootInputs) : "");
    if (n == 1) return fmt::format("in 1 of {} inputs", nRootInputs);
    return fmt::format("in {} of {} inputs: read from the first", n, nRootInputs);
  };
  // one object: its details as for one file, then where it is
  if (order.size() == 1 && order[0] == path && !path.empty()) {
    const found_t& object = found[path];
    const auto fileAndFolder = util::split_input_path(object.inputs.front());
    const string first = fileAndFolder[0] + ":" + ((fileAndFolder.size() > 1) ? fileAndFolder[1] + "/" : "") + path;
    if (!print_root_file_contents(first)) return false;
    if (isChained(object.className) && object.inputs.size() > 1 && object.className != "ROOT::RNTuple") {
      TChain chain(path.substr(path.rfind('/') + 1).data());
      for (const auto& input : object.inputs) {
        const auto parts = util::split_input_path(input);
        chain.Add((parts[0] + "/" + ((parts.size() > 1) ? parts[1] + "/" : "") + path).data());
      }
      logger::print("  {}: {} entries in all", note(object), chain.GetEntries());
    } else if (const string where = note(object); !where.empty()) {
      logger::print("  {}", where);
    }
    return true;
  }
  string printedFolder = "-";
  for (const auto& objectPath : order) {
    const found_t& object = found[objectPath];
    const auto [folderPart, name] = util::split_path(objectPath);
    if (folderPart != printedFolder && !folderPart.empty()) logger::print("{}/", folderPart);
    printedFolder = folderPart;
    const string where = note(object);
    logger::print("- [{}] {}{}", object.className, name, where.empty() ? "" : " (" + where + ")");
  }
  return true;
}

//**************************************************************************************************
/**
 * srp print <file>[:<path>]: the contents of a ROOT file (false with an error).
 */
//**************************************************************************************************
bool print_root_file_contents(const string& inputPath)
{
  // split after the .root ending, as the inputs of data sources
  const auto fileAndPath = util::split_input_path(inputPath);
  const string fileName = fileAndPath[0];
  string startPath = (fileAndPath.size() > 1) ? fileAndPath[1] : "";
  while (!startPath.empty() && startPath.back() == '/')
    startPath.pop_back();
  if (fileName.empty()) {
    logger::error("Specify a file: srp print <file>[:<path>].");
    return false;
  }
  std::error_code existsError;
  if (util::is_missing_local_file(fileName, existsError)) {
    logger::error("Could not find file {}.", fileName);
    return false;
  }
  std::unique_ptr<TFile> inputFile;
  {
    auto quiet = util::ignore_root_messages_below(kFatal);  // a file that is not a ROOT file is reported below
    inputFile = util::open_input_file(fileName);
  }
  if (!inputFile || inputFile->IsZombie()) {
    logger::error("Cannot read {}: srp print lists the contents of ROOT files.", fileName);
    return false;
  }

  TObject* root = inputFile.get();
  if (!startPath.empty()) {
    root = find_in_file(root, startPath);
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

  // one object named by its path: besides its class, what choosing ranges, binning and axes needs (no ROOT warnings about the data)
  auto printDetails = [&](TObject* object, const string& path) {
    logger::print("- [{}] {}", object->ClassName(), path);
    if (std::strlen(object->GetTitle())) logger::print("  title \"{}\"", object->GetTitle());
    auto number = [](double_t value) { return fmt::format("{:.4g}", value); };
    auto printAxis = [&](const string& name, const TAxis* axis) {
      string line = fmt::format("  {}: {} {} from {} to {}", name, axis->GetNbins(), (axis->IsVariableBinSize()) ? "variable bins" : "bins", number(axis->GetXmin()), number(axis->GetXmax()));
      if (axis->GetLabels()) line += fmt::format(", {} bin labels", axis->GetLabels()->GetSize());
      if (std::strlen(axis->GetTitle())) line += fmt::format(", title \"{}\"", axis->GetTitle());
      logger::print("{}", line);
    };
    auto printRange = [&](const string& name, double_t low, double_t high) { logger::print("  {}: {} to {}", name, number(low), number(high)); };
    // the binned axes of a histogram: x, y and z as far as it has them
    auto printHistAxes = [&](const TH1* hist) {
      const vector<string> names{"x", "y", "z"};
      const vector<const TAxis*> axes{hist->GetXaxis(), hist->GetYaxis(), hist->GetZaxis()};
      for (int32_t dim = 0; dim < hist->GetDimension(); ++dim)
        printAxis(names[dim], axes[dim]);
    };

    if (auto* hist = dynamic_cast<TH1*>(object)) {
      // the bins inside the axes (a TH2Poly: its polygons); apart from them the under- and overflow, and the smallest content above 0
      // (where a log axis starts)
      double_t integral{}, flows{}, minimum{}, maximum{}, minimumAboveZero{};
      int32_t nBins{}, nFilled{};
      auto count = [&](double_t content) {
        if (nBins++ == 0 || content < minimum) minimum = content;
        if (nBins == 1 || content > maximum) maximum = content;
        if (content > 0. && (minimumAboveZero == 0. || content < minimumAboveZero)) minimumAboveZero = content;
        integral += content;
        if (content != 0.) ++nFilled;
      };
      if (auto* poly = dynamic_cast<TH2Poly*>(hist)) {
        printRange("x", poly->GetXaxis()->GetXmin(), poly->GetXaxis()->GetXmax());
        printRange("y", poly->GetYaxis()->GetXmin(), poly->GetYaxis()->GetXmax());
        for (int32_t bin = 1; bin <= poly->GetNumberOfBins(); ++bin)
          count(poly->GetBinContent(bin));
      } else {
        printHistAxes(hist);
        for (int32_t bin = 0; bin < hist->GetNcells(); ++bin) {
          if (hist->IsBinUnderflow(bin) || hist->IsBinOverflow(bin)) {
            flows += hist->GetBinContent(bin);
          } else {
            count(hist->GetBinContent(bin));
          }
        }
      }
      // a profile holds means: their sum is no integral
      const bool isProfile = TString(hist->ClassName()).BeginsWith("TProfile");
      string statistics = fmt::format("  entries {}", number(hist->GetEntries()));
      if (!isProfile) statistics += fmt::format(", integral {}, under- and overflow {}", number(integral), number(flows));
      logger::print("{}", statistics);
      if (nBins) {
        const string aboveZero = (minimum <= 0. && minimumAboveZero > 0.) ? fmt::format(" (smallest above 0: {})", number(minimumAboveZero)) : "";
        logger::print("  {} {} to {}{}, {} of {} bins not 0", (isProfile) ? "means" : "contents", number(minimum), number(maximum), aboveZero, nFilled, nBins);
      }
    } else if (auto* graph = dynamic_cast<TGraph*>(object)) {
      logger::print("  {} points", graph->GetN());
      if (graph->GetN()) {
        const auto [xMin, xMax] = std::minmax_element(graph->GetX(), graph->GetX() + graph->GetN());
        const auto [yMin, yMax] = std::minmax_element(graph->GetY(), graph->GetY() + graph->GetN());
        printRange("x", *xMin, *xMax);
        printRange("y", *yMin, *yMax);
      }
    } else if (auto* graph2d = dynamic_cast<TGraph2D*>(object)) {
      logger::print("  {} points", graph2d->GetN());
      if (graph2d->GetN()) {
        printRange("x", graph2d->GetXmin(), graph2d->GetXmax());
        printRange("y", graph2d->GetYmin(), graph2d->GetYmax());
        printRange("z", graph2d->GetZmin(), graph2d->GetZmax());
      }
    } else if (auto* function = dynamic_cast<TF1*>(object)) {
      const TString formula = function->GetExpFormula();
      logger::print("  {}, {} parameters", (formula.IsNull()) ? "C++ function" : fmt::format("formula {}", formula.Data()), function->GetNpar());
      printRange("x", function->GetXmin(), function->GetXmax());
      if (auto* function2d = dynamic_cast<TF2*>(function)) printRange("y", function2d->GetYmin(), function2d->GetYmax());
      if (auto* function3d = dynamic_cast<TF3*>(function)) printRange("z", function3d->GetZmin(), function3d->GetZmax());
    } else if (auto* histN = dynamic_cast<THnBase*>(object)) {
      for (int32_t dim = 0; dim < histN->GetNdimensions(); ++dim)
        printAxis(fmt::format("axis {}", dim), histN->GetAxis(dim));
      // a THnSparse holds only the bins filled, a THn every bin (with under- and overflow)
      Long64_t nFilled{};
      for (Long64_t bin = 0; bin < histN->GetNbins(); ++bin) {
        if (histN->GetBinContent(bin) != 0.) ++nFilled;
      }
      logger::print("  entries {}, {} bins filled", number(histN->GetEntries()), nFilled);
    } else if (auto* efficiency = dynamic_cast<TEfficiency*>(object)) {
      const TH1* total = efficiency->GetTotalHistogram();
      printHistAxes(total);
      logger::print("  passed {} of {} entries", number(efficiency->GetPassedHistogram()->GetEntries()), number(total->GetEntries()));
    } else if (auto* tree = dynamic_cast<TTree*>(object)) {
      logger::print("  entries {}", tree->GetEntries());
      printBranches(tree);
    }
  };

  // the folder of a listed object is printed once, before its first object
  string printedFolder = "-";
  auto listObject = [&](const string& path, const string& className, TObject* object) {
    const auto [folder, name] = util::split_path(path);
    if (folder != printedFolder && !folder.empty()) logger::print("{}/", folder);
    printedFolder = folder;
    logger::print("- [{}] {}", className, name);
    if (object) printBranches(object);
  };
  if (!util::is_container(root->IsA())) {
    printDetails(root, startPath);
  } else {
    walk_contents(root, startPath, listObject);
  }
  return true;
}

//**************************************************************************************************
/**
 * The application for a command that opens a window, or null (reported) without a graphical display: ROOT then falls back to batch
 * mode after its own errors, and the application would run forever without a window to close.
 */
//**************************************************************************************************
std::unique_ptr<TApplication> open_window_application(const string& command)
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
