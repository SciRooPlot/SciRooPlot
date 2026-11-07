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

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#include "SciRooPlot/Logger.h"

#include <boost/property_tree/info_parser.hpp>

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace SciRooPlot
{

class Config
{
 public:
  static Config& GetMutable();
  static const Config& Get();

  using LogLevel = logger::detail::level;
  static std::string LogLevelName(int level);
  static std::optional<int> ParseLogLevel(const std::string& name);

  using ColorMode = logger::detail::color_mode;
  static std::string ColorModeName(int mode);
  static std::optional<int> ParseColorMode(const std::string& name);

  // a screen or bitmap scale: a finite positive number
  static std::optional<double> ParseScale(const std::string& value);

  int Verbosity() const { return mLogLevel; }
  int Color() const { return mColorMode; }
  const std::string& PlotMode() const { return mPlotMode; }
  bool MatchCaseInsensitive() const { return mMatchCaseInsensitive; }
  bool MatchContains() const { return mMatchContains; }
  double ScreenScale() const { return mScreenScale; }
  double BitmapScale() const { return mBitmapScale; }
  const std::filesystem::path& Path() const { return mPath; }

  const std::string& ProjectsFile() const { return mProjectsFile; }
  const std::string& SettingsFile() const { return mSettingsFile; }

  bool Exists(const std::string& projectName) const;
  std::vector<std::string> ProjectNames() const;
  std::map<std::string, std::string> Properties(const std::string& projectName) const;  // as stored (unexpanded)
  std::string Property(const std::string& projectName, const std::string& property) const;
  std::string Program(const std::string& projectName) const;
  std::string OutputDir(const std::string& projectName) const;

  std::filesystem::path ProjectPath(const std::string& projectName) const;
  std::string PlotsFile(const std::string& projectName) const;
  std::string DataSourcesFile(const std::string& projectName) const;
  const std::string& CurrentProject() const { return mCurrentProject; }
  bool ProjectsFileUnreadable() const { return mProjectsUnreadable; }  // projects.info exists but could not be read

  void SetVerbosity(int logLevel);
  void SetColor(int colorMode);
  void SetPlotMode(const std::string& plotMode);
  bool SetProperty(const std::string& projectName, const std::string& property, const std::string& value);
  void SetMatchCaseInsensitive(bool matchCaseInsensitive);
  void SetMatchContains(bool matchContains);
  void SetScreenScale(double screenScale);
  void SetBitmapScale(double bitmapScale);

  bool Reset();  // false if a project could not be removed
  bool Clean();
  bool Add(const std::string& projectName, const std::string& program, const std::string& outputDir = "");
  bool Remove(const std::string& projectName);
  bool Rename(const std::string& projectName, const std::string& newProjectName);
  bool Select(const std::string& projectName);

  bool Save();

 private:
  Config();
  static Config& Instance();
  void LoadConfig();
  std::optional<bool> ApplySetting(const std::string& key, const std::string& value);
  bool CheckProjectDir(const std::string& projectName) const;  // true if the project's directory is inside the config path
  bool DeleteProjectDir(const std::string& projectName) const;
  static std::string ForeignEntries(const std::filesystem::path& projectPath);  // names in it SciRooPlot did not write, comma separated
  // the files SciRooPlot writes into a project's directory; names of older versions stay in the list
  inline static const std::set<std::string> kProjectFiles{"plots.info", "dataSources.info", "tabcomp.csv", "UserData.root", ".DS_Store"};
  std::vector<std::string> RemoveProjects(const std::vector<std::string>& projectNames, const std::string& nextCurrent);
  bool SaveProjects();  // writes the projects at once (before the files of a project are changed on disk)
  bool ProjectsReadable() const;
  static bool WriteInfoFile(const std::string& file, const boost::property_tree::ptree& tree);
  static bool CheckProjectName(const std::string& projectName);
  static bool CheckPath(const std::string& path);
  static bool CheckProgram(const std::string& program);

  struct Lock {
    explicit Lock(const std::filesystem::path& path);
    ~Lock();
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    int mFd;
  };

  // what this process changed, the only parts written back (the files are read again first)
  std::set<std::string> mChangedSettings;
  bool mRewriteSettings{false};                                     // settings.info had bad entries
  std::set<std::string> mTouchedProjects;                           // added, renamed or removed: written as a whole
  std::map<std::string, std::set<std::string>> mTouchedProperties;  // set by SetProperty: only these properties are written
  bool mCurrentTouched{false};
  bool mProjectsUnreadable{false};  // projects.info exists but could not be read
  int mLogLevel{LogLevel::debug};
  int mColorMode{ColorMode::bright};
  std::string mPlotMode{"show"};
  bool mMatchCaseInsensitive{false};
  bool mMatchContains{false};
  double mScreenScale{1.};  // size factor for plots shown on screen
  double mBitmapScale{1.};  // resolution factor for bitmap files (png, jpg, gif)
  std::filesystem::path mPath;
  std::string mProjectsFile;
  std::string mSettingsFile;

  struct Project {
    Project() = default;
    explicit Project(const boost::property_tree::ptree& tree);
    boost::property_tree::ptree GetTree() const;
    std::string Property(const std::string& property) const;
    std::map<std::string, std::string> mProperties;
  };

  std::map<std::string, Project> mProjects;
  std::string mCurrentProject;
};

}  // namespace SciRooPlot

#endif  // SRC_CONFIG_H_
