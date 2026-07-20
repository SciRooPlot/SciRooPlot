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

#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

#include <string>
#include <filesystem>
#include <map>
#include <boost/property_tree/info_parser.hpp>

namespace SciRooPlot
{

class Config
{
 public:
  static Config& GetMutable();
  static const Config& Get();

  enum LogLevel {
    Off,
    Log,
    Info,
    Warning,
    Error,
    Debug,
  };

  enum ColorMode {
    bright,
    dark,
    off,
  };

  int Verbosity() const { return mLogLevel; }
  int ColorScheme() const { return mColorMode; }
  const std::filesystem::path& Path() const { return mPath; }

  const std::string& ProjectsFile() const { return mProjectsFile; }
  const std::string& SettingsFile() const { return mSettingsFile; }

  bool Exists(const std::string& projectName) const;

  const std::string Executable(const std::string& projectName) const;
  const std::string OutputDir(const std::string& projectName) const;

  const std::filesystem::path ProjectPath(const std::string& projectName) const;
  const std::string PlotsFile(const std::string& projectName) const;
  const std::string DatasetsFile(const std::string& projectName) const;
  const std::string& CurrentProject() const { return mCurrentProject; }

  void SetVerbosity(int logLevel) { mLogLevel = logLevel; }
  void SetColorScheme(int colorMode) { mColorMode = colorMode; }
  void SetOutputDir(const std::string& projectName, const std::string& outputDir);
  void SetExecutable(const std::string& projectName, const std::string& executable);

 private:
  Config() { LoadConfig(); }
  ~Config() { SaveConfig(); }
  static Config& Instance();
  void LoadConfig();
  void SaveConfig();

  bool mModified;
  int mLogLevel{LogLevel::Debug};
  int mColorMode{ColorMode::bright};
  std::filesystem::path mPath;
  std::string mProjectsFile;
  std::string mSettingsFile;

  struct Project {
    Project() = default;
    Project(const boost::property_tree::ptree& tree);
    boost::property_tree::ptree GetTree();
    std::string mExecutable;
    std::string mOutputDir;
  };

  std::map<std::string, Project> mProjects;
  std::string mCurrentProject;
};

}  // namespace SciRooPlot

#endif  // INCLUDE_CONFIG_H_
