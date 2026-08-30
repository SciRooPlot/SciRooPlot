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

#ifndef INCLUDE_SCIROOPLOT_CONFIG_H_
#define INCLUDE_SCIROOPLOT_CONFIG_H_

#include <boost/property_tree/info_parser.hpp>

#include <filesystem>
#include <map>
#include <string>

namespace SciRooPlot
{

class Config
{
 public:
  static Config& GetMutable();
  static const Config& Get();

  enum LogLevel {
    silent,
    error,
    warning,
    info,
    log,
    debug,
  };

  enum ColorMode {
    bright,
    dark,
    off,
  };

  int Verbosity() const { return mLogLevel; }
  int ColorScheme() const { return mColorMode; }
  const std::string& PlotMode() const { return mPlotMode; }
  bool MatchCaseInsensitive() const { return mMatchCaseInsensitive; }
  bool MatchContains() const { return mMatchContains; }
  const std::filesystem::path& Path() const { return mPath; }

  const std::string& ProjectsFile() const { return mProjectsFile; }
  const std::string& SettingsFile() const { return mSettingsFile; }

  bool Exists(const std::string& projectName) const;
  void ListProjects() const;

  void Show(const std::string& projectName) const;
  std::string Property(const std::string& projectName, const std::string& property) const;
  std::string Program(const std::string& projectName) const;
  std::string OutputDir(const std::string& projectName) const;

  std::filesystem::path ProjectPath(const std::string& projectName) const;
  std::string PlotsFile(const std::string& projectName) const;
  std::string DataSourcesFile(const std::string& projectName) const;
  const std::string& CurrentProject() const { return mCurrentProject; }

  void SetVerbosity(int logLevel) { mLogLevel = logLevel; }
  void SetColorScheme(int colorMode) { mColorMode = colorMode; }
  void SetPlotMode(const std::string& plotMode) { mPlotMode = plotMode; }
  void SetProperty(const std::string& projectName, const std::string& property, const std::string& value);
  void SetOutputDir(const std::string& projectName, const std::string& outputDir);
  void SetProgram(const std::string& projectName, const std::string& program);
  void SetMatchCaseInsensitive(bool matchCaseInsensitive);
  void SetMatchContains(bool matchContains);

  void Reset();
  void Clean();
  void Remove(const std::string& projectName);
  void Rename(const std::string& projectName, const std::string& newProjectName);
  void Select(const std::string& projectName);

 private:
  Config() { LoadConfig(); }
  ~Config() { SaveConfig(); }
  static Config& Instance();
  void LoadConfig();
  void SaveConfig();

  bool mModified{false};
  int mLogLevel{LogLevel::debug};
  int mColorMode{ColorMode::bright};
  std::string mPlotMode{"show"};
  int mMatchCaseInsensitive{false};
  int mMatchContains{false};
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

#endif  // INCLUDE_SCIROOPLOT_CONFIG_H_
