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

#include "SciRooPlot/Config.h"

#include "SciRooPlot/Helpers.h"
#include "SciRooPlot/Logging.h"

#include <TSystem.h>

#include <boost/property_tree/info_parser.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using boost::property_tree::ptree;
using boost::property_tree::read_info;
using boost::property_tree::write_info;
using std::string;
using std::vector;

namespace SciRooPlot
{
string Config::LogLevelName(int level)
{
  switch (level) {
    case LogLevel::silent:
      return "silent";
    case LogLevel::error:
      return "error";
    case LogLevel::warning:
      return "warning";
    case LogLevel::info:
      return "info";
    case LogLevel::log:
      return "log";
    case LogLevel::debug:
      return "debug";
    default:
      return "unknown";
  }
}

std::optional<int> Config::ParseLogLevel(const string& name)
{
  if (name == "silent") return LogLevel::silent;
  if (name == "error") return LogLevel::error;
  if (name == "warning") return LogLevel::warning;
  if (name == "info") return LogLevel::info;
  if (name == "log") return LogLevel::log;
  if (name == "debug") return LogLevel::debug;
  return std::nullopt;
}

string Config::ColorModeName(int mode)
{
  switch (mode) {
    case ColorMode::bright:
      return "bright";
    case ColorMode::dark:
      return "dark";
    case ColorMode::off:
      return "off";
    default:
      return "unknown";
  }
}

std::optional<int> Config::ParseColorMode(const string& name)
{
  if (name == "bright") return ColorMode::bright;
  if (name == "dark") return ColorMode::dark;
  if (name == "off") return ColorMode::off;
  return std::nullopt;
}

Config& Config::Instance()
{
  static Config config;
  return config;
}
Config& Config::GetMutable()
{
  auto& config = Instance();
  config.mModified = true;
  return config;
}
const Config& Config::Get()
{
  return Instance();
}

void Config::LoadConfig()
{
  const char* envPath = gSystem->Getenv("SCIROOPLOT_CONFIG_PATH");
  std::string configPath = expand_path(envPath ? "${SCIROOPLOT_CONFIG_PATH}" : "~/.SciRooPlot");
  if (configPath.empty()) {
    std::cerr << "SCIROOPLOT_CONFIG_PATH must not be empty." << std::endl;
    return;
  }
  mPath = configPath;
  if (mPath.is_relative()) {
    std::cerr << "SCIROOPLOT_CONFIG_PATH must not be relative." << std::endl;
    mPath = "";
    return;
  }

  mProjectsFile = mPath / "projects.info";
  mSettingsFile = mPath / "settings.info";

  // read global settings
  if (file_exists(mSettingsFile)) {
    try {
      ptree settingsTree;
      read_info(mSettingsFile, settingsTree);
      for (auto& setting : settingsTree) {

        if (setting.first == "logLevel") {
          const int level = std::stoi(setting.second.data());
          if (level < LogLevel::silent || level > LogLevel::debug) {
            std::cerr << "Ignoring out-of-range logLevel " << level << " in " << mSettingsFile << "." << std::endl;
          } else {
            mLogLevel = level;
          }
          continue;
        }
        if (setting.first == "colorMode") {
          const int mode = std::stoi(setting.second.data());
          if (mode < ColorMode::bright || mode > ColorMode::off) {
            std::cerr << "Ignoring out-of-range colorMode " << mode << " in " << mSettingsFile << "." << std::endl;
          } else {
            mColorMode = mode;
          }
          continue;
        }
        if (setting.first == "plotMode") {
          mPlotMode = setting.second.data();
          continue;
        }
        if (setting.first == "matchCaseInsensitive") {
          mMatchCaseInsensitive = setting.second.get_value<bool>();
          continue;
        }
        if (setting.first == "matchContains") {
          mMatchContains = setting.second.get_value<bool>();
          continue;
        }
      }
    } catch (const std::exception& e) {
      std::cerr << "Cannot load settings file " << mSettingsFile << ": " << e.what() << ". Using default settings." << std::endl;
    }
  }

  // read project settings
  if (file_exists(mProjectsFile)) {
    try {
      ptree projectsTree;
      read_info(mProjectsFile, projectsTree);
      for (auto& project : projectsTree) {
        if (project.first == "@current") {
          mCurrentProject = project.second.data();
          continue;
        }
        if (project.second.empty()) continue;
        mProjects.insert_or_assign(project.first, Project(project.second));
      }
    } catch (const std::exception& e) {
      std::cerr << "Cannot load projects file " << mProjectsFile << ": " << e.what() << ". No projects loaded." << std::endl;
    }
  }
}

void Config::SaveConfig()
{
  if (!mModified) return;

  // save settings tree
  ptree settingsTree;
  settingsTree.add("logLevel", mLogLevel);
  settingsTree.add("colorMode", mColorMode);
  settingsTree.add("plotMode", mPlotMode);
  settingsTree.add("matchCaseInsensitive", mMatchCaseInsensitive);
  settingsTree.add("matchContains", mMatchContains);
  write_info(SettingsFile(), settingsTree);

  // save projects tree
  ptree projectsTree;
  for (const auto& [projectName, project] : mProjects) {
    projectsTree.put_child(projectName, project.GetTree());
  }
  projectsTree.add("@current", mCurrentProject);
  write_info(ProjectsFile(), projectsTree);
}

bool Config::Exists(const std::string& projectName) const
{
  if (mProjects.find(projectName) != mProjects.end()) {
    return true;
  }
  return false;
}

void Config::ListProjects() const
{
  for (const auto& [projectName, project] : mProjects) {
    PRINT("{}  {}", (mCurrentProject == projectName) ? "*" : " ", projectName);
  }
}

void Config::ShowSettings() const
{
  PRINT("Verbosity:       {}", LogLevelName(mLogLevel));
  PRINT("Color:           {}", ColorModeName(mColorMode));
  PRINT("Plot mode:       {}", mPlotMode);
  PRINT("Match mode:      {}", mMatchContains ? "contains" : "exact");
  PRINT("Match case:      {}", mMatchCaseInsensitive ? "insensitive" : "sensitive");
  PRINT("Current project: {}", mCurrentProject.empty() ? "<none>" : mCurrentProject);
}

void Config::Rename(const std::string& projectName, const std::string& newProjectName)
{
  if (mProjects.find(projectName) == mProjects.end()) {
    ERROR("Cannot find project {}.", projectName);
    return;
  }
  if (newProjectName.empty()) {
    ERROR("Specify new name for {}.", projectName);
    return;
  }
  if (auto illegal = find_illegal_name_char(newProjectName)) {
    ERROR("Project name '{}' contains illegal character '{}'.", newProjectName, *illegal);
    return;
  }
  if (mProjects.find(newProjectName) != mProjects.end()) {
    ERROR("Project {} already exists.", newProjectName);
    return;
  }
  auto node = mProjects.extract(projectName);
  if (!node.empty()) {
    std::filesystem::path projectPath = ProjectPath(projectName);
    if (std::filesystem::exists(projectPath)) {
      std::filesystem::rename(projectPath, projectPath.parent_path() / newProjectName);
    }
    node.key() = newProjectName;
    mProjects.insert(std::move(node));
    if (mCurrentProject == projectName) {
      mCurrentProject = newProjectName;
    }
    PRINT("Renamed project {} to {}. User code should be adjusted accordingly.", projectName, newProjectName);
  }
}

void Config::Reset()
{
  mProjects.clear();
  mCurrentProject.clear();
}

void Config::Clean()
{
  if (mPath.empty()) {
    ERROR("No valid config path; refusing to clean.");
    return;
  }
  string firstProject;
  vector<string> inactiveProjects;
  for (const auto& [projectName, project] : mProjects) {
    const string rawProgram = project.Property("program");
    bool isStale = false;
    if (rawProgram.empty()) {
      isStale = true;
    } else {
      const string program = expand_path(rawProgram);
      if (program.empty()) {
        WARNING("Cannot expand program path '{}' of project {}; keeping it.", rawProgram, projectName);
      } else {
        isStale = !std::filesystem::exists(std::filesystem::path(program).parent_path());
      }
    }
    if (isStale) {
      inactiveProjects.push_back(projectName);
    } else if (firstProject.empty()) {
      firstProject = projectName;
    }
  }
  bool updatedActiveProject = false;
  for (const auto& inactiveProject : inactiveProjects) {
    const auto projectPath = ProjectPath(inactiveProject);
    if (!projectPath.is_absolute() || projectPath.parent_path() != mPath) {
      ERROR("Refusing to delete {} for project {}: not inside the config directory.", projectPath.string(), inactiveProject);
      continue;
    }
    std::error_code ec;
    std::filesystem::remove_all(projectPath, ec);
    if (ec) {
      ERROR("Could not delete {}: {}", projectPath.string(), ec.message());
      continue;
    }
    PRINT("- deleting project {}", inactiveProject);
    mProjects.erase(inactiveProject);
    if (mCurrentProject == inactiveProject) {
      mCurrentProject = firstProject;
      updatedActiveProject = true;
    }
  }
  if (updatedActiveProject && !mCurrentProject.empty()) {
    INFO("Selecting project {}.", mCurrentProject);
  }
}

void Config::Show(const string& projectNameIn) const
{
  for (const auto& [projectName, project] : mProjects) {
    if (!projectNameIn.empty() && (projectName != projectNameIn)) {
      continue;
    }
    PRINT("{} {}", (projectName == mCurrentProject) ? "*" : " ", projectName);
    for (const auto& [property, value] : project.mProperties) {
      if (!value.empty()) {
        PRINT("   - {}: {}", property, value);
      }
    }
  }
}

void Config::Remove(const string& projectName)
{
  if (projectName.empty()) {
    ERROR("Specify which project to remove.");
    return;
  }
  mProjects.erase(projectName);
  if (mCurrentProject == projectName) {
    mCurrentProject.clear();
    auto it = mProjects.begin();
    if (it != mProjects.end()) {
      mCurrentProject = it->first;
      INFO("Selecting project {}", mCurrentProject);
    }
  }
}

void Config::Select(const string& projectName)
{
  if (projectName.empty()) {
    ERROR("Specify a project to select.");
    return;
  }
  if (mProjects.find(projectName) != mProjects.end()) {
    mCurrentProject = projectName;
  } else {
    ERROR("Cannot find project {}.", projectName);
  }
}

std::filesystem::path Config::ProjectPath(const string& projectName) const
{
  if (projectName.empty()) return projectName;
  return mPath / projectName;
}

std::string Config::PlotsFile(const string& projectName) const
{
  if (projectName.empty()) return projectName;
  return mPath / projectName / "plots.info";
}

std::string Config::DataSourcesFile(const string& projectName) const
{
  if (projectName.empty()) return projectName;
  return mPath / projectName / "dataSources.info";
}

void Config::SetProgram(const string& projectName, const string& program)
{
  if (projectName.empty()) {
    ERROR("Specify project name.");
    return;
  }
  if (auto illegal = find_illegal_name_char(projectName)) {
    ERROR("Project name '{}' contains illegal character '{}'.", projectName, *illegal);
    return;
  }
  if (std::filesystem::path(expand_path(program)).is_relative()) {
    ERROR("The path must not be relative.");
    return;
  }
  mProjects[projectName].mProperties["program"] = program;
}

string Config::Program(const string& projectName) const
{
  auto it = mProjects.find(projectName);
  if (it != mProjects.end()) {
    const auto& project = it->second;
    return expand_path(project.Property("program"));
  }
  return {};
}

void Config::SetOutputDir(const string& projectName, const string& outputDir)
{
  if (projectName.empty()) {
    ERROR("Specify project name.");
    return;
  }
  if (auto illegal = find_illegal_name_char(projectName)) {
    ERROR("Project name '{}' contains illegal character '{}'.", projectName, *illegal);
    return;
  }
  if (std::filesystem::path(expand_path(outputDir)).is_relative()) {
    ERROR("The path must not be relative.");
    return;
  }
  mProjects[projectName].mProperties["outdir"] = outputDir;
}

string Config::Project::Property(const string& property) const
{
  auto it = mProperties.find(property);
  if (it != mProperties.end()) {
    return it->second;
  }
  return {};
}

void Config::SetProperty(const string& projectName, const string& property, const string& value)
{
  if (projectName.empty()) {
    ERROR("Specify project name.");
    return;
  }
  if (property.empty()) {
    ERROR("Specify property to set for {}.", projectName);
    return;
  }
  if (auto illegal = find_illegal_name_char(projectName)) {
    ERROR("Project name '{}' contains illegal character '{}'.", projectName, *illegal);
    return;
  }
  if (auto illegal = find_illegal_name_char(property)) {
    ERROR("Property '{}' contains illegal character '{}'.", property, *illegal);
    return;
  }
  auto& properties = mProjects[projectName].mProperties;
  properties[property] = value;
  bool allEmpty = std::all_of(properties.begin(), properties.end(), [](const auto& p) { return p.second.empty(); });
  if (allEmpty) {
    INFO("Project {} has no properties left after unsetting '{}'. Removing project.", projectName, property);
    Remove(projectName);
  }
}

string Config::Property(const string& projectName, const string& property) const
{
  auto it = mProjects.find(projectName);
  if (it != mProjects.end()) {
    const auto& project = it->second;
    return project.Property(property);
  }
  return {};
}

string Config::OutputDir(const string& projectName) const
{
  auto it = mProjects.find(projectName);
  if (it != mProjects.end()) {
    const auto& project = it->second;
    return expand_path(project.Property("outdir"));
  }
  return {};
}

Config::Project::Project(const ptree& tree)
{
  for (const auto& [key, child] : tree) {
    if (!child.empty()) {
      continue;
    }
    if (key.empty()) {
      std::cerr << "Ignoring property with empty name." << std::endl;
      continue;
    }
    if (auto illegal = find_illegal_name_char(key)) {
      std::cerr << "Ignoring property '" << key << "' with illegal character '" << *illegal << "'." << std::endl;
      continue;
    }
    if (auto value = child.get_value_optional<std::string>()) {
      mProperties[key] = *value;
    }
  }
}

ptree Config::Project::GetTree() const
{
  ptree tree;
  for (const auto& [property, value] : mProperties) {
    if (!value.empty()) {
      tree.add(property, mProperties.at(property));
    }
  }
  return tree;
}

void Config::SetMatchCaseInsensitive(bool matchCaseInsensitive)
{
  mMatchCaseInsensitive = matchCaseInsensitive;
}

void Config::SetMatchContains(bool matchContains)
{
  mMatchContains = matchContains;
}

}  // namespace SciRooPlot
