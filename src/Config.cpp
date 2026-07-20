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

#include <iostream>
#include <string>
#include <boost/property_tree/info_parser.hpp>

#include "TSystem.h"

#include "Config.h"
#include "Helpers.h"
#include "Logging.h"

using boost::property_tree::ptree;
using boost::property_tree::read_info;
using boost::property_tree::write_info;
using std::string;

namespace SciRooPlot
{

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
    ptree settingsTree;
    read_info(mSettingsFile, settingsTree);
    for (auto& setting : settingsTree) {

      if (setting.first == "logLevel") {
        mLogLevel = std::stoi(setting.second.data());
        continue;
      }
      if (setting.first == "colorMode") {
        mColorMode = std::stoi(setting.second.data());
        continue;
      }
    }
  }

  // read project settings
  if (file_exists(mProjectsFile)) {
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
  }
}

void Config::SaveConfig()
{
  if (!mModified) return;

  // save settings tree
  ptree settingsTree;
  settingsTree.add("logLevel", mLogLevel);
  settingsTree.add("colorMode", mColorMode);
  write_info(SettingsFile(), settingsTree);

  // save projects tree
  ptree projectsTree;
  for (auto [projectName, project] : mProjects) {
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

const std::filesystem::path Config::ProjectPath(const string& projectName) const
{
  if (projectName.empty()) return projectName;
  return mPath / projectName;
}

const std::string Config::PlotsFile(const string& projectName) const
{
  if (projectName.empty()) return projectName;
  return mPath / projectName / "plots.info";
}
const std::string Config::DataSourcesFile(const string& projectName) const
{
  if (projectName.empty()) return projectName;
  return mPath / projectName / "dataSources.info";
}

void Config::SetExecutable(const string& projectName, const string& executable)
{
  mProjects[projectName].mExecutable = executable;
}

const string Config::Executable(const string& projectName) const
{
  auto it = mProjects.find(projectName);
  if (it != mProjects.end()) {
    const auto& project = it->second;
    return project.mExecutable;
  }
  return {};
}

void Config::SetOutputDir(const string& projectName, const string& outputDir)
{
  mProjects[projectName].mOutputDir = outputDir;
}

const string Config::OutputDir(const string& projectName) const
{
  auto it = mProjects.find(projectName);
  if (it != mProjects.end()) {
    const auto& project = it->second;
    return project.mOutputDir;
  }
  return {};
}

Config::Project::Project(const ptree& tree)
{
  if (auto executable = tree.get_optional<string>("EXE")) {
    mExecutable = *executable;
  }
  if (auto outputDir = tree.get_optional<string>("OUT")) {
    mOutputDir = *outputDir;
  }
}

ptree Config::Project::GetTree()
{
  ptree tree;
  if (!mExecutable.empty()) {
    tree.add("EXE", mExecutable);
  }
  if (!mOutputDir.empty()) {
    tree.add("OUT", mOutputDir);
  }
  return tree;
}

}  // namespace SciRooPlot
