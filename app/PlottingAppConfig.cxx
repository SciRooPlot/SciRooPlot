/*
********************************************************************************
* --------------------------------- SciRooPlot ---------------------------------
* Copyright (c) 2019-2025 Mario Krüger
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
*******************************************************************************
*/

#include "SciRooPlot.h"
#include "PlotManager.h"
#include "Plot.h"
#include "Logging.h"

#include <vector>
#include <iostream>
#include <string>
#include <filesystem>
#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "Helpers.h"

using namespace SciRooPlot;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  string configPath = (gSystem->Getenv("SCIROOPLOT_CONFIG_PATH")) ? "${SCIROOPLOT_CONFIG_PATH}" : "~/.SciRooPlot";
  configPath = expand_path(configPath);
  if (std::filesystem::path(configPath).is_relative()) {
    ERROR("SCIROOPLOT_CONFIG_PATH must not be relative.");
    return 1;
  }
  if (std::filesystem::create_directories(configPath)) {
    INFO("Created config folder: {}", configPath);
  }
  string configFileName = configPath + "/projects.xml";

  ptree configTree;
  string activeProject;
  if (file_exists(configFileName)) {
    using boost::property_tree::read_xml;
    read_xml(configFileName, configTree, boost::property_tree::xml_parser::trim_whitespace);
    if (auto active = configTree.get_child_optional("active")) {
      activeProject = active.get().data();
    }
  }

  string command;
  string project;
  string setting;

  try {
    po::options_description arguments("positional arguments");
    arguments.add_options()("command", po::value<string>(), "command")("project", po::value<string>(), "project")("setting", po::value<string>(), "setting");
    po::positional_options_description pos;
    pos.add("command", 1);
    pos.add("project", 1);
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
      if (project == "active") {
        ERROR(R"(Your project cannot be called "active".)");
        return 1;
      }
    }
    if (vm.count("setting")) {
      setting = vm["setting"].as<string>();
    }
  } catch (std::exception& e) {
    ERROR(R"(Exception "{}"! Exiting.)", e.what());
    return 1;
  } catch (...) {
    ERROR("Exception of unknown type! Exiting.");
    return 1;
  }
  static const vector<string> commandList = {"executable", "outputDir"};

  if (command == "help") {
    PRINT("plot-config (list | active | base | path | reset | clean)");
    PRINT("plot-config (init | remove | switch | show) <PROJECT_NAME>");
    PRINT("plot-config (executable | outputDir | rename) <PROJECT_NAME> <SETTING>")
  } else if (command == "base") {
    std::cout << configPath << std::endl;
  } else if (command == "path") {
    std::cout << configPath + "/" + activeProject << std::endl;
  } else if (command == "reset") {
    configTree.clear();
    activeProject.clear();
  } else if (command == "clean") {
    vector<string> inactiveProjects;
    for (auto& entry : configTree) {
      string project = entry.first;
      if (project == "active") continue;
      if (auto curConfig = configTree.get_child_optional(project)) {
        if (auto executable = curConfig.get().get_child_optional("executable")) {
          if (std::filesystem::exists(std::filesystem::path(executable->get_value<string>()).parent_path())) continue;
          inactiveProjects.push_back(project);
        }
      }
    }
    for (auto& project : inactiveProjects) {
      PRINT("- deleting project {}", project);
      configTree.erase(project);
      std::filesystem::remove_all(configPath + "/" + project);
      if (project == activeProject) {
        activeProject.clear();
      }
    }
  } else if (command == "list") {
    for (auto& entry : configTree) {
      string project = entry.first;
      if (project == "active") continue;
      if (project == activeProject) {
        PRINT("* {}", project);
      } else {
        PRINT("  {}", project);
      }
    }
  } else if (command == "active") {
    std::cout << activeProject << std::endl;
  } else if (command == "remove") {
    if (project.empty()) {
      ERROR("Specify which project to remove.");
      return 1;
    }
    configTree.erase(project);
    std::filesystem::remove_all(configPath + "/" + project);
    if (project == activeProject) {
      activeProject.clear();
    }
  } else if (command == "switch") {
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
    string selectedProject = activeProject;
    if (!project.empty()) {
      selectedProject = project;
    }
    if (auto curConfig = configTree.get_child_optional(selectedProject)) {
      PRINT("Project: {}", selectedProject);
      for (auto curCommand : commandList)
        if (auto property = curConfig.get().get_child_optional(curCommand)) {
          PRINT("- {}: {}", curCommand, property->get_value<string>());
        }
    }
  } else if (command == "rename") {
    if (configTree.get_child_optional(project) && !configTree.get_child_optional(setting)) {
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
  } else if (std::count(commandList.begin(), commandList.end(), command)) {
    if (setting.empty()) {
      string selectedProject = activeProject;
      if (!project.empty()) {
        selectedProject = project;
      }
      if (auto curConfig = configTree.get_child_optional(selectedProject)) {
        if (auto property = curConfig.get().get_child_optional(command)) {
          std::cout << property->get_value<string>() << std::endl;
          return 0;
        }
      }
    } else if (!project.empty()) {
      if (auto curConfig = configTree.get_child_optional(project)) {
        curConfig.get().erase(command);
        INFO(R"(Updating "{}" in project "{}".)", command, project);
      }
      configTree.add(project + "." + command, setting);
      if (activeProject.empty()) {
        activeProject = project;
      }
    }
  } else {
    ERROR("Invalid arguments.");
  }

  configTree.erase("active");
  configTree.add("active", activeProject);

  using boost::property_tree::xml_writer_settings;
  xml_writer_settings<std::string> settings('\t', 1);
  using boost::property_tree::write_xml;
  write_xml(configFileName, configTree, std::locale(), settings);

  std::ofstream tabCompFile;
  tabCompFile.open(std::filesystem::path(configFileName).replace_extension("csv").string());
  for (auto& entry : configTree) {
    string project = entry.first;
    if (project == "active") continue;
    tabCompFile << project << "\n";
  }
  tabCompFile.close();

  return 0;
}
