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

#include "Config.h"

#include "SciRooPlot/Logger.h"

#include <TSystem.h>

#include <boost/property_tree/info_parser.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "Validation.h"
#include "util/Paths.h"

using boost::property_tree::ptree;
using boost::property_tree::read_info;
using boost::property_tree::write_info;
using std::string;
using std::vector;

namespace SciRooPlot
{
using util::expand_path;

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
std::optional<double> Config::ParseScale(const string& value)
{
  try {
    size_t parsed{};
    const double scale = std::stod(value, &parsed);
    if (parsed == value.size() && std::isfinite(scale) && scale > 0.) return scale;
  } catch (const std::exception&) {
  }
  return std::nullopt;
}

Config::Config()
{
  LoadConfig();
  std::atexit([] { Instance().Save(); });
}
Config& Config::Instance()
{
  // never destroyed: a destructor would write the files during static destruction, where a failure can neither be reported nor handled
  static Config* config = new Config();
  return *config;
}
Config& Config::GetMutable()
{
  return Instance();
}
const Config& Config::Get()
{
  return Instance();
}

void Config::LoadConfig()
{
  const char* envPath = gSystem->Getenv("SCIROOPLOT_CONFIG_PATH");
  std::string configPath = expand_path(envPath ? "${SCIROOPLOT_CONFIG_PATH}" : "~/.SciRooPlot").value_or("");
  if (configPath.empty()) {
    std::cerr << "SCIROOPLOT_CONFIG_PATH must not be empty." << std::endl;
    return;
  }
  // normalized (no trailing slash, '.' or '..'), so that paths derived from it can be compared with it
  mPath = std::filesystem::path(configPath).lexically_normal();
  if (!mPath.has_filename() && mPath.has_relative_path()) mPath = mPath.parent_path();
  if (mPath.is_relative()) {
    std::cerr << "SCIROOPLOT_CONFIG_PATH must not be relative." << std::endl;
    mPath = "";
    return;
  }

  mProjectsFile = mPath / "projects.info";
  mSettingsFile = mPath / "settings.info";

  // read global settings: the keys and values are those of the srp commands; a bad value is ignored on its own
  std::error_code existsError;  // a config path that cannot be checked (a symlink loop, no permission) counts as empty instead of aborting
  if (std::filesystem::exists(mSettingsFile, existsError)) {
    ptree settingsTree;
    try {
      read_info(mSettingsFile, settingsTree);
    } catch (const std::exception& e) {
      std::cerr << "Cannot load settings file " << mSettingsFile << ": " << e.what() << ". Using default settings." << std::endl;
    }
    for (const auto& [key, node] : settingsTree) {
      const string value = node.data();
      bool valid = true;
      if (key == "verbosity") {
        if (auto level = ParseLogLevel(value))
          mLogLevel = *level;
        else
          valid = false;
      } else if (key == "color") {
        if (auto mode = ParseColorMode(value))
          mColorMode = *mode;
        else
          valid = false;
      } else if (key == "plotmode") {
        if (value == "list" || is_valid_plot_mode(value))
          mPlotMode = value;
        else
          valid = false;
      } else if (key == "matchmode") {
        if (value == "exact" || value == "contains")
          mMatchContains = (value == "contains");
        else
          valid = false;
      } else if (key == "matchcase") {
        if (value == "sensitive" || value == "insensitive")
          mMatchCaseInsensitive = (value == "insensitive");
        else
          valid = false;
      } else if (key == "screenscale" || key == "bitmapscale") {
        if (auto scale = ParseScale(value))
          (key == "screenscale" ? mScreenScale : mBitmapScale) = *scale;
        else
          valid = false;
      } else {
        std::cerr << "Ignoring unknown setting " << key << " in " << mSettingsFile << "." << std::endl;
        mModified = true;  // the file is written again without it
        continue;
      }
      if (!valid) {
        std::cerr << "Ignoring invalid " << key << " '" << value << "' in " << mSettingsFile << " (srp " << key << " sets it)." << std::endl;
        mModified = true;  // the file is written again with the default
      }
    }
  }

  // read project settings
  if (std::filesystem::exists(mProjectsFile, existsError)) {
    try {
      ptree projectsTree;
      read_info(mProjectsFile, projectsTree);
      for (auto& project : projectsTree) {
        if (project.first == "@current") {
          mCurrentProject = project.second.data();
          continue;
        }
        if (project.second.empty()) continue;
        // the name is a directory in the config path: one written by hand or by an old version could be '..' or '.',
        // and remove, clean and reset would then delete the folder above the config path or the config path itself
        if (project.first.empty() || find_illegal_name_char(project.first)) {
          std::cerr << "Ignoring project '" << project.first << "' in " << mProjectsFile << ": the name may contain only letters, digits, _ and -." << std::endl;
          continue;
        }
        mProjects.insert_or_assign(project.first, Project(project.second));
      }
    } catch (const std::exception& e) {
      std::cerr << "Cannot load projects file " << mProjectsFile << ": " << e.what() << ". No projects loaded." << std::endl;
    }
  }
}

// writes to a temporary file and renames it, so an interrupted write cannot leave a truncated file behind
static bool WriteInfoFile(const string& file, const ptree& tree)
{
  const string tmpFile = file + "." + std::to_string(getpid()) + ".tmp";
  try {
    write_info(tmpFile, tree);
    std::filesystem::rename(tmpFile, file);
  } catch (const std::exception& e) {
    std::error_code ec;
    std::filesystem::remove(tmpFile, ec);
    logger::error("Cannot save the configuration: {}.", e.what());
    return false;
  }
  return true;
}

bool Config::Save()
{
  if (!mModified) return true;
  mModified = false;  // a failed write is reported once and not repeated at exit
  if (mPath.empty()) {
    logger::error("No valid config path; cannot save the configuration.");
    return false;
  }
  // save settings tree
  ptree settingsTree;
  settingsTree.add("verbosity", LogLevelName(mLogLevel));
  settingsTree.add("color", ColorModeName(mColorMode));
  settingsTree.add("plotmode", mPlotMode);
  settingsTree.add("matchmode", mMatchContains ? "contains" : "exact");
  settingsTree.add("matchcase", mMatchCaseInsensitive ? "insensitive" : "sensitive");
  settingsTree.add("screenscale", mScreenScale);
  settingsTree.add("bitmapscale", mBitmapScale);
  const bool savedSettings = WriteInfoFile(SettingsFile(), settingsTree);
  const bool savedProjects = SaveProjects();
  return savedSettings && savedProjects;
}

bool Config::SaveProjects() const
{
  if (mPath.empty()) {
    logger::error("No valid config path; cannot save the configuration.");
    return false;
  }
  ptree projectsTree;
  for (const auto& [projectName, project] : mProjects) {
    projectsTree.put_child(projectName, project.GetTree());
  }
  projectsTree.add("@current", mCurrentProject);
  return WriteInfoFile(ProjectsFile(), projectsTree);
}

bool Config::Exists(const std::string& projectName) const
{
  return mProjects.find(projectName) != mProjects.end();
}

void Config::ListProjects() const
{
  for (const auto& [projectName, project] : mProjects) {
    logger::print("{}  {}", (mCurrentProject == projectName) ? "*" : " ", projectName);
  }
}

void Config::ShowSettings() const
{
  logger::print("Verbosity:       {}", LogLevelName(mLogLevel));
  logger::print("Color:           {}", ColorModeName(mColorMode));
  logger::print("Plot mode:       {}", mPlotMode);
  logger::print("Match mode:      {}", mMatchContains ? "contains" : "exact");
  logger::print("Match case:      {}", mMatchCaseInsensitive ? "insensitive" : "sensitive");
  logger::print("Screen scale:    {}", mScreenScale);
  logger::print("Bitmap scale:    {}", mBitmapScale);
  logger::print("Current project: {}", mCurrentProject.empty() ? "<none>" : mCurrentProject);
}

bool Config::Rename(const std::string& projectName, const std::string& newProjectName)
{
  if (mPath.empty()) {
    logger::error("No valid config path; cannot rename.");
    return false;
  }
  if (projectName.empty()) {
    logger::error("Specify project to rename.");
    return false;
  }
  if (!Exists(projectName)) {
    logger::error("Cannot find project {}.", projectName);
    return false;
  }
  if (newProjectName.empty()) {
    logger::error("Specify new name for {}.", projectName);
    return false;
  }
  if (!CheckProjectName(newProjectName)) return false;
  if (Exists(newProjectName)) {
    logger::error("Project {} already exists.", newProjectName);
    return false;
  }

  const std::filesystem::path projectPath = ProjectPath(projectName);
  const std::filesystem::path newProjectPath = ProjectPath(newProjectName);
  if (std::filesystem::exists(newProjectPath)) {
    logger::error("Cannot rename {} to {}: {} already exists.", projectName, newProjectName, newProjectPath.string());
    return false;
  }
  if (!CheckProjectDir(projectName)) return false;

  // the configuration is saved first: if that fails nothing has changed on disk, and projects.info never names a directory that is gone
  const string previousCurrent = mCurrentProject;
  auto renameEntry = [this](const string& from, const string& to) {
    auto node = mProjects.extract(from);
    node.key() = to;
    mProjects.insert(std::move(node));
  };
  renameEntry(projectName, newProjectName);
  if (mCurrentProject == projectName) mCurrentProject = newProjectName;
  if (!SaveNow()) {
    renameEntry(newProjectName, projectName);
    mCurrentProject = previousCurrent;
    return false;
  }
  std::error_code existsError;
  if (std::filesystem::exists(projectPath, existsError)) {
    std::error_code ec;
    std::filesystem::rename(projectPath, newProjectPath, ec);
    if (ec) {
      logger::error("Could not rename {} to {}: {}.", projectPath.string(), newProjectPath.string(), ec.message());
      renameEntry(newProjectName, projectName);
      mCurrentProject = previousCurrent;
      SaveNow();
      return false;
    }
  }
  logger::print("Renamed project {} to {}. User code should be adjusted accordingly.", projectName, newProjectName);
  return true;
}

void Config::Reset()
{
  if (mPath.empty()) {
    logger::error("No valid config path; refusing to reset.");
    return;
  }
  vector<string> projectNames;
  for (const auto& [projectName, project] : mProjects) {
    projectNames.push_back(projectName);
  }
  RemoveProjects(projectNames, "");
}

void Config::Clean()
{
  if (mPath.empty()) {
    logger::error("No valid config path; refusing to clean.");
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
      const auto program = expand_path(rawProgram);
      if (!program) {
        logger::warning("Cannot expand program path '{}' of project {}; keeping it.", rawProgram, projectName);
      } else {
        // a folder that cannot be checked (no permission, a symlink loop, a stale mount) is kept, not taken as gone
        std::error_code ec;
        const bool folderExists = std::filesystem::exists(std::filesystem::path(*program).parent_path(), ec);
        if (ec) {
          logger::warning("Cannot check the program folder of project {} ({}); keeping it.", projectName, ec.message());
        } else {
          isStale = !folderExists;
        }
      }
    }
    if (isStale) {
      inactiveProjects.push_back(projectName);
    } else if (firstProject.empty()) {
      firstProject = projectName;
    }
  }
  for (const auto& removed : RemoveProjects(inactiveProjects, firstProject)) {
    logger::print("- deleting project {}", removed);
  }
}

void Config::Show(const string& projectNameIn) const
{
  for (const auto& [projectName, project] : mProjects) {
    if (!projectNameIn.empty() && (projectName != projectNameIn)) {
      continue;
    }
    logger::print("{} {}", (projectName == mCurrentProject) ? "*" : " ", projectName);
    for (const auto& [property, value] : project.mProperties) {
      if (!value.empty()) {
        logger::print("   - {}: {}", property, value);
      }
    }
  }
}

bool Config::Remove(const string& projectName)
{
  if (projectName.empty()) {
    logger::error("Specify which project to remove.");
    return false;
  }
  if (mPath.empty()) {
    logger::error("No valid config path; refusing to remove.");
    return false;
  }
  if (!Exists(projectName)) {
    logger::error("Cannot find project {}.", projectName);
    return false;
  }
  const string otherProject = (mProjects.begin()->first != projectName) ? mProjects.begin()->first : (mProjects.size() > 1 ? std::next(mProjects.begin())->first : "");
  return !RemoveProjects({projectName}, otherProject).empty();
}

/**
 * Removes the projects from the configuration and deletes their directories in the config path; returns the ones removed.
 * The configuration is saved first: if that fails nothing is deleted. A directory that cannot be deleted afterwards
 * keeps its project registered (saved again), so projects.info always matches the directories.
 */
vector<string> Config::RemoveProjects(const vector<string>& projectNames, const string& nextCurrent)
{
  std::map<string, Project> removed;
  for (const auto& projectName : projectNames) {
    if (!CheckProjectDir(projectName)) continue;  // stays registered
    auto node = mProjects.extract(projectName);
    if (node) removed.insert(std::move(node));
  }
  if (removed.empty()) return {};
  const string previousCurrent = mCurrentProject;
  if (removed.count(mCurrentProject)) mCurrentProject = mProjects.count(nextCurrent) ? nextCurrent : (mProjects.empty() ? "" : mProjects.begin()->first);
  auto restore = [&](const string& projectName) { mProjects.insert(removed.extract(projectName)); };
  if (!SaveNow()) {
    while (!removed.empty())
      restore(removed.begin()->first);
    mCurrentProject = previousCurrent;
    return {};
  }
  vector<string> deleted;
  bool restored = false;
  for (auto it = removed.begin(); it != removed.end();) {
    const string projectName = it->first;
    ++it;
    if (DeleteProjectDir(projectName)) {
      deleted.push_back(projectName);
    } else {
      restore(projectName);
      if (projectName == previousCurrent) mCurrentProject = previousCurrent;
      restored = true;
    }
  }
  if (restored) SaveNow();
  if (mCurrentProject != previousCurrent && !mCurrentProject.empty()) logger::info("Selecting project {}.", mCurrentProject);
  return deleted;
}

bool Config::CheckProjectDir(const string& projectName) const
{
  // only the project's directory in the config path (plots.info, dataSources.info, ...), never the user code or the output directory
  const auto projectPath = ProjectPath(projectName);
  // compared after normalising: the parent of config/.. and of config/. is config, but they are not inside it
  if (projectName.empty() || find_illegal_name_char(projectName) || !projectPath.is_absolute() ||
      projectPath.lexically_normal().parent_path() != mPath.lexically_normal()) {
    logger::error("Refusing to change {} for project {}: not inside the config directory.", projectPath.string(), projectName);
    return false;
  }
  // a config path set to a general folder (SCIROOPLOT_CONFIG_PATH=$HOME) makes project names real folders (Documents):
  // a directory is changed only if it holds nothing but the files SciRooPlot writes there
  static const std::set<string> kProjectFiles = {"plots.info", "dataSources.info", "tabcomp.csv", "UserData.root"};
  std::error_code ec;
  const auto status = std::filesystem::symlink_status(projectPath, ec);
  if (ec || status.type() == std::filesystem::file_type::not_found) return true;  // nothing there to change
  if (status.type() == std::filesystem::file_type::symlink) return true;          // the link itself is removed, not what it points to
  if (status.type() != std::filesystem::file_type::directory) {
    logger::error("Refusing to change {} for project {}: it is not a directory.", projectPath.string(), projectName);
    return false;
  }
  for (std::filesystem::directory_iterator entry(projectPath, ec), end; !ec && entry != end; entry.increment(ec)) {
    const string name = entry->path().filename().string();
    const bool isOwnFile = entry->is_regular_file(ec) && (kProjectFiles.count(name) || (name.size() > 4 && name.compare(name.size() - 4, 4, ".tmp") == 0));
    if (!isOwnFile) {
      logger::error("Refusing to change {} for project {}: it contains {}, which SciRooPlot did not write. Is SCIROOPLOT_CONFIG_PATH a folder of its own?",
                    projectPath.string(), projectName, name);
      return false;
    }
  }
  if (ec) {
    logger::error("Refusing to change {} for project {}: cannot read it ({}).", projectPath.string(), projectName, ec.message());
    return false;
  }
  return true;
}

bool Config::DeleteProjectDir(const string& projectName) const
{
  if (!CheckProjectDir(projectName)) return false;
  const auto projectPath = ProjectPath(projectName);
  std::error_code ec;
  std::filesystem::remove_all(projectPath, ec);
  if (ec) {
    logger::error("Could not delete {}: {}.", projectPath.string(), ec.message());
    return false;
  }
  return true;
}

bool Config::SaveNow()
{
  mModified = true;  // the settings are still written at exit
  return SaveProjects();
}

bool Config::Select(const string& projectName)
{
  if (projectName.empty()) {
    logger::error("Specify a project to select.");
    return false;
  }
  if (!Exists(projectName)) {
    logger::error("Cannot find project {}.", projectName);
    return false;
  }
  mCurrentProject = projectName;
  mModified = true;
  return true;
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

// a project name without illegal characters
bool Config::CheckProjectName(const string& projectName)
{
  if (auto illegal = find_illegal_name_char(projectName)) {
    logger::error("Project name '{}' contains illegal character '{}'.", projectName, *illegal);
    return false;
  }
  return true;
}

// a program or output directory: absolute once ~ and environment variables are expanded
bool Config::CheckPath(const string& path)
{
  const auto expandedPath = expand_path(path);
  if (!expandedPath) return false;
  if (std::filesystem::path(*expandedPath).is_relative()) {
    logger::error("The path {} must not be relative.", path);
    return false;
  }
  return true;
}

bool Config::Add(const string& projectName, const string& program, const string& outputDir)
{
  if (projectName.empty()) {
    logger::error("Specify project name.");
    return false;
  }
  if (!CheckProjectName(projectName)) return false;
  if (Exists(projectName)) {
    logger::error("Project {} already exists. Use srp set {} program <program> to change its program.", projectName, projectName);
    return false;
  }
  if (program.empty()) {
    logger::error("Specify program for project {}.", projectName);
    return false;
  }
  // checked before anything is stored, so that a failing add leaves no project behind
  if (!CheckPath(program) || (!outputDir.empty() && !CheckPath(outputDir))) return false;
  auto& properties = mProjects[projectName].mProperties;
  properties["program"] = program;
  if (!outputDir.empty()) properties["outdir"] = outputDir;
  mCurrentProject = projectName;
  mModified = true;
  logger::info("Selecting project {}.", projectName);
  return true;
}

string Config::Project::Property(const string& property) const
{
  auto it = mProperties.find(property);
  if (it != mProperties.end()) {
    return it->second;
  }
  return {};
}

bool Config::SetProperty(const string& projectName, const string& property, const string& value)
{
  if (projectName.empty()) {
    logger::error("Specify project or use @current.");
    return false;
  }
  if (!Exists(projectName)) {
    logger::error("Cannot find project {}. Use srp add or srp init-cpp/init-py to create it.", projectName);
    return false;
  }
  if (property.empty()) {
    logger::error("Specify a property of {}.", projectName);
    return false;
  }
  if (auto illegal = find_illegal_name_char(property)) {
    logger::error("Property '{}' contains illegal character '{}'.", property, *illegal);
    return false;
  }
  if ((property == "program" || property == "outdir") && !value.empty() && !CheckPath(value)) return false;
  auto& properties = mProjects[projectName].mProperties;
  properties[property] = value;
  mModified = true;
  bool allEmpty = std::all_of(properties.begin(), properties.end(), [](const auto& p) { return p.second.empty(); });
  if (allEmpty) {
    logger::info("Project {} has no properties left after unsetting '{}'. Removing project.", projectName, property);
    return Remove(projectName);
  }
  return true;
}

string Config::Property(const string& projectName, const string& property) const
{
  auto it = mProjects.find(projectName);
  return (it != mProjects.end()) ? it->second.Property(property) : "";
}

string Config::Program(const string& projectName) const
{
  return expand_path(Property(projectName, "program")).value_or("");
}

string Config::OutputDir(const string& projectName) const
{
  return expand_path(Property(projectName, "outdir")).value_or("");
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

void Config::SetVerbosity(int logLevel)
{
  mLogLevel = logLevel;
  mModified = true;
}

void Config::SetColor(int colorMode)
{
  mColorMode = colorMode;
  mModified = true;
}

void Config::SetPlotMode(const string& plotMode)
{
  mPlotMode = plotMode;
  mModified = true;
}

void Config::SetMatchCaseInsensitive(bool matchCaseInsensitive)
{
  mMatchCaseInsensitive = matchCaseInsensitive;
  mModified = true;
}

void Config::SetMatchContains(bool matchContains)
{
  mMatchContains = matchContains;
  mModified = true;
}

void Config::SetScreenScale(double screenScale)
{
  mScreenScale = screenScale;
  mModified = true;
}

void Config::SetBitmapScale(double bitmapScale)
{
  mBitmapScale = bitmapScale;
  mModified = true;
}

// the settings the logger of the public headers needs, without exposing the configuration itself
int logger::detail::verbosity()
{
  return Config::Get().Verbosity();
}

int logger::detail::color()
{
  return Config::Get().Color();
}

}  // namespace SciRooPlot
