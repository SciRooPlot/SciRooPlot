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
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <sys/file.h>
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
      const auto valid = ApplySetting(key, node.data());
      if (!valid) {
        std::cerr << "Ignoring unknown setting " << key << " in " << mSettingsFile << "." << std::endl;
      } else if (!*valid) {
        std::cerr << "Ignoring invalid " << key << " '" << node.data() << "' in " << mSettingsFile << " (srp " << key << " sets it)." << std::endl;
      }
      if (!valid || !*valid) mRewriteSettings = true;  // the file is written again without it (or with the default)
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
      mProjectsUnreadable = true;
    }
  }
}

// sets a setting from its value in settings.info (those of the srp commands); nullopt for an unknown key, false for a bad value
std::optional<bool> Config::ApplySetting(const string& key, const string& value)
{
  if (key == "verbosity") {
    if (auto level = ParseLogLevel(value)) mLogLevel = *level;
    return ParseLogLevel(value).has_value();
  }
  if (key == "color") {
    if (auto mode = ParseColorMode(value)) mColorMode = *mode;
    return ParseColorMode(value).has_value();
  }
  if (key == "plotmode") {
    const bool valid = (value == "list" || is_valid_plot_mode(value));
    if (valid) mPlotMode = value;
    return valid;
  }
  if (key == "matchmode") {
    const bool valid = (value == "exact" || value == "contains");
    if (valid) mMatchContains = (value == "contains");
    return valid;
  }
  if (key == "matchcase") {
    const bool valid = (value == "sensitive" || value == "insensitive");
    if (valid) mMatchCaseInsensitive = (value == "insensitive");
    return valid;
  }
  if (key == "screenscale" || key == "bitmapscale") {
    auto scale = ParseScale(value);
    if (scale) (key == "screenscale" ? mScreenScale : mBitmapScale) = *scale;
    return scale.has_value();
  }
  return std::nullopt;
}

// an exclusive lock of the config path for reading and writing its files: other srp commands and programs wait meanwhile
// (without the lock file, e.g. in a read-only config path, the files are written unlocked)
Config::Lock::Lock(const std::filesystem::path& path) : mFd(open((path / ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644))
{
  if (mFd >= 0) flock(mFd, LOCK_EX);
}
Config::Lock::~Lock()
{
  if (mFd >= 0) close(mFd);  // releases the lock
}

// writes to a temporary file and renames it, so an interrupted write cannot leave a truncated file behind
bool Config::WriteInfoFile(const string& file, const ptree& tree)
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

// the files are read again under the lock and only the settings and projects this process changed are written,
// so that srp commands and programs running at the same time do not undo each other's changes
bool Config::Save()
{
  const bool settingsChanged = !mChangedSettings.empty() || mRewriteSettings;
  if (!settingsChanged && mTouchedProjects.empty() && mTouchedProperties.empty() && !mCurrentTouched) return true;
  if (mPath.empty()) {
    logger::error("No valid config path; cannot save the configuration.");
    return false;
  }
  bool savedSettings = true;
  if (settingsChanged) {
    Lock lock(mPath);
    std::error_code existsError;
    if (std::filesystem::exists(mSettingsFile, existsError)) {
      ptree settingsTree;
      try {
        read_info(mSettingsFile, settingsTree);
      } catch (const std::exception&) {
      }
      for (const auto& [key, node] : settingsTree) {
        if (!mChangedSettings.count(key)) ApplySetting(key, node.data());  // bad entries were reported at start and are dropped
      }
    }
    ptree tree;
    tree.add("verbosity", LogLevelName(mLogLevel));
    tree.add("color", ColorModeName(mColorMode));
    tree.add("plotmode", mPlotMode);
    tree.add("matchmode", mMatchContains ? "contains" : "exact");
    tree.add("matchcase", mMatchCaseInsensitive ? "insensitive" : "sensitive");
    tree.add("screenscale", mScreenScale);
    tree.add("bitmapscale", mBitmapScale);
    // a failed write is reported once and not repeated at exit
    mChangedSettings.clear();
    mRewriteSettings = false;
    savedSettings = WriteInfoFile(SettingsFile(), tree);
  }
  const bool savedProjects = SaveProjects();
  return savedSettings && savedProjects;
}

// a projects file that could not be read is never written over (it would lose every project): changes to projects fail until it is fixed
bool Config::ProjectsReadable() const
{
  if (mProjectsUnreadable) logger::error("Cannot change projects: {} could not be read. Fix or remove it.", mProjectsFile);
  return !mProjectsUnreadable;
}

bool Config::SaveProjects()
{
  if (mProjectsUnreadable) return true;  // nothing was changed (ProjectsReadable)
  if (mTouchedProjects.empty() && mTouchedProperties.empty() && !mCurrentTouched) return true;
  if (mPath.empty()) {
    logger::error("No valid config path; cannot save the configuration.");
    return false;
  }
  // the projects as they are on disk now, with the ones this process added, changed or removed (and its current project) replaced
  Lock lock(mPath);
  std::map<string, Project> projects;
  string current;
  std::error_code existsError;
  if (std::filesystem::exists(mProjectsFile, existsError)) {
    try {
      ptree diskTree;
      read_info(mProjectsFile, diskTree);
      for (const auto& [name, node] : diskTree) {
        if (name == "@current") {
          current = node.data();
        } else if (!node.empty() && !name.empty() && !find_illegal_name_char(name)) {
          projects.insert_or_assign(name, Project(node));
        }
      }
    } catch (const std::exception& e) {
      logger::error("Cannot save the configuration: {} could not be read ({}). Fix or remove it.", mProjectsFile, e.what());
      mProjectsUnreadable = true;  // reported once: not repeated at exit, later changes fail (ProjectsReadable)
      return false;
    }
  }
  for (const auto& name : mTouchedProjects) {
    if (auto it = mProjects.find(name); it != mProjects.end()) {
      projects.insert_or_assign(name, it->second);
    } else {
      projects.erase(name);
    }
  }
  // a property set here replaces only that property: another process may have set others of the same project meanwhile
  for (const auto& [name, properties] : mTouchedProperties) {
    auto it = mProjects.find(name);
    if (mTouchedProjects.count(name) || it == mProjects.end()) continue;
    auto [target, added] = projects.try_emplace(name, it->second);  // removed meanwhile: written as known here
    if (added) continue;
    for (const auto& property : properties) {
      target->second.mProperties[property] = it->second.Property(property);
    }
  }
  if (mCurrentTouched || !projects.count(current)) current = (mCurrentTouched || projects.count(mCurrentProject)) ? mCurrentProject : "";
  if (!current.empty() && !projects.count(current)) current = projects.empty() ? "" : projects.begin()->first;
  mProjects = std::move(projects);
  mCurrentProject = current;
  mTouchedProjects.clear();
  mTouchedProperties.clear();
  mCurrentTouched = false;

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

std::vector<std::string> Config::ProjectNames() const
{
  std::vector<std::string> names;
  names.reserve(mProjects.size());
  for (const auto& [projectName, project] : mProjects)
    names.push_back(projectName);
  return names;
}

bool Config::Rename(const std::string& projectName, const std::string& newProjectName)
{
  if (!ProjectsReadable()) return false;
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
  std::error_code newPathError;
  if (std::filesystem::exists(newProjectPath, newPathError) || newPathError) {
    logger::error("Cannot rename {} to {}: {} already exists.", projectName, newProjectName, newProjectPath.string());
    return false;
  }
  if (!CheckProjectDir(projectName)) return false;
  if (const string foreign = ForeignEntries(projectPath); !foreign.empty()) {
    logger::error("Cannot rename {}: it also holds {} (not written by this version of SciRooPlot).", projectPath.string(), foreign);
    return false;
  }

  // the configuration is saved first: if that fails nothing has changed on disk, and projects.info never names a directory that is gone
  const string previousCurrent = mCurrentProject;
  auto renameEntry = [this](const string& from, const string& to) {
    auto node = mProjects.extract(from);
    node.key() = to;
    mProjects.insert(std::move(node));
    mTouchedProjects.insert({from, to});
    if (mCurrentProject == from) {
      mCurrentProject = to;
      mCurrentTouched = true;
    }
  };
  renameEntry(projectName, newProjectName);
  if (!SaveProjects()) {
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
      SaveProjects();
      return false;
    }
  }
  // the inputs of in-memory data sources are the UserData.root of the project's folder: they move with it
  // (compared as resolved paths: the program may have spelled the configuration path differently, e.g. through a symlink)
  if (const string dataSources = DataSourcesFile(newProjectName); std::filesystem::exists(dataSources, existsError)) {
    Lock lock(mPath);
    ptree tree;
    try {
      read_info(dataSources, tree);
    } catch (const std::exception& e) {
      logger::warning("Could not update the in-memory data sources in {} ({}): run the program of {} again.", dataSources, e.what(), newProjectName);
      tree.clear();
    }
    std::error_code ec;
    const auto oldUserData = std::filesystem::weakly_canonical(projectPath / "UserData.root", ec);
    const string newUserData = (newProjectPath / "UserData.root").string();
    bool changed = false;
    for (auto& [dataSource, inputs] : tree) {
      for (auto& [key, input] : inputs) {
        if (key != "INPUT") continue;
        const string value = input.get_value<string>();
        const size_t split = value.find(".root:");
        const string file = value.substr(0, (split == string::npos) ? value.size() : split + 5);
        if (std::filesystem::weakly_canonical(file, ec) != oldUserData || ec) continue;
        input.put_value(newUserData + value.substr(file.size()));
        changed = true;
      }
    }
    if (changed && !WriteInfoFile(dataSources, tree)) {
      logger::warning("Could not update the in-memory data sources in {}: run the program of {} again.", dataSources, newProjectName);
    }
  }
  logger::print("Renamed project {} to {}. User code should be adjusted accordingly.", projectName, newProjectName);
  return true;
}

// false if a project could not be removed (as for srp remove)
bool Config::Reset()
{
  if (!ProjectsReadable()) return false;
  if (mPath.empty()) {
    logger::error("No valid config path; refusing to reset.");
    return false;
  }
  vector<string> projectNames;
  for (const auto& [projectName, project] : mProjects) {
    projectNames.push_back(projectName);
  }
  return RemoveProjects(projectNames, "").size() == projectNames.size();
}

bool Config::Clean()
{
  if (!ProjectsReadable()) return false;
  if (mPath.empty()) {
    logger::error("No valid config path; refusing to clean.");
    return false;
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
  const auto removed = RemoveProjects(inactiveProjects, firstProject);
  for (const auto& projectName : removed) {
    logger::print("- deleting project {}", projectName);
  }
  return removed.size() == inactiveProjects.size();
}

std::map<string, string> Config::Properties(const string& projectName) const
{
  auto it = mProjects.find(projectName);
  return (it != mProjects.end()) ? it->second.mProperties : std::map<string, string>{};
}

bool Config::Remove(const string& projectName)
{
  if (!ProjectsReadable()) return false;
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
    if (node) {
      mTouchedProjects.insert(projectName);
      removed.insert(std::move(node));
    }
  }
  if (removed.empty()) return {};
  const string previousCurrent = mCurrentProject;
  if (removed.count(mCurrentProject)) {
    mCurrentProject = mProjects.count(nextCurrent) ? nextCurrent : (mProjects.empty() ? "" : mProjects.begin()->first);
    mCurrentTouched = true;
  }
  auto restore = [&](const string& projectName) {
    mProjects.insert(removed.extract(projectName));
    mTouchedProjects.insert(projectName);
  };
  if (!SaveProjects()) {
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
      if (projectName == previousCurrent) {
        mCurrentProject = previousCurrent;
        mCurrentTouched = true;
      }
      restored = true;
    }
  }
  if (restored) SaveProjects();
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
  return true;
}

// the entries of a project's directory that SciRooPlot did not write there: with a config path that is a general folder
// (SCIROOPLOT_CONFIG_PATH=$HOME), a project named like a folder of the user (Documents) is that folder
string Config::ForeignEntries(const std::filesystem::path& projectPath)
{
  string foreign;
  std::error_code ec;
  for (std::filesystem::directory_iterator entry(projectPath, ec), end; !ec && entry != end; entry.increment(ec)) {
    const string name = entry->path().filename().string();
    std::error_code typeError;
    if (!entry->is_regular_file(typeError) || !kProjectFiles.count(name)) foreign += (foreign.empty() ? "" : ", ") + name;
  }
  return foreign;
}

bool Config::DeleteProjectDir(const string& projectName) const
{
  if (!CheckProjectDir(projectName)) return false;
  const auto projectPath = ProjectPath(projectName);
  std::error_code ec;
  const auto status = std::filesystem::symlink_status(projectPath, ec);
  if (ec || status.type() == std::filesystem::file_type::not_found) return true;
  if (status.type() != std::filesystem::file_type::directory) {  // a link: removed, not what it points to
    std::filesystem::remove(projectPath, ec);
  } else {
    // only SciRooPlot's files are deleted, the directory only if nothing else is in it
    const string foreign = ForeignEntries(projectPath);
    for (const auto& file : kProjectFiles) {
      if (!ec) std::filesystem::remove(projectPath / file, ec);
    }
    if (!ec && foreign.empty()) std::filesystem::remove(projectPath, ec);
    if (!ec && !foreign.empty()) {
      logger::warning("Kept {}: it also holds {} (not written by this version of SciRooPlot).", projectPath.string(), foreign);
    }
  }
  if (ec) {
    logger::error("Could not delete {}: {}.", projectPath.string(), ec.message());
    return false;
  }
  return true;
}

bool Config::Select(const string& projectName)
{
  if (!ProjectsReadable()) return false;
  if (projectName.empty()) {
    logger::error("Specify a project to select.");
    return false;
  }
  if (!Exists(projectName)) {
    logger::error("Cannot find project {}.", projectName);
    return false;
  }
  mCurrentProject = projectName;
  mCurrentTouched = true;
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
  if (!ProjectsReadable()) return false;
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
  mTouchedProjects.insert(projectName);
  mCurrentProject = projectName;
  mCurrentTouched = true;
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
  if (!ProjectsReadable()) return false;
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
  if (property == "program" && value.empty()) {
    logger::error("The program of project {} cannot be unset (srp set {} program <path> changes it, srp remove {} removes the project).", projectName, projectName, projectName);
    return false;
  }
  if ((property == "program" || property == "outdir") && !value.empty() && !CheckPath(value)) return false;
  auto& properties = mProjects[projectName].mProperties;
  properties[property] = value;
  mTouchedProperties[projectName].insert(property);
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
  mChangedSettings.insert("verbosity");
}

void Config::SetColor(int colorMode)
{
  mColorMode = colorMode;
  mChangedSettings.insert("color");
}

void Config::SetPlotMode(const string& plotMode)
{
  mPlotMode = plotMode;
  mChangedSettings.insert("plotmode");
}

void Config::SetMatchCaseInsensitive(bool matchCaseInsensitive)
{
  mMatchCaseInsensitive = matchCaseInsensitive;
  mChangedSettings.insert("matchcase");
}

void Config::SetMatchContains(bool matchContains)
{
  mMatchContains = matchContains;
  mChangedSettings.insert("matchmode");
}

void Config::SetScreenScale(double screenScale)
{
  mScreenScale = screenScale;
  mChangedSettings.insert("screenscale");
}

void Config::SetBitmapScale(double bitmapScale)
{
  mBitmapScale = bitmapScale;
  mChangedSettings.insert("bitmapscale");
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
