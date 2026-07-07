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

#include "PlotManager.h"
#include "Plot.h"
#include "Logging.h"

#include <string>
#include <vector>
#include <filesystem>
#include <boost/program_options.hpp>

#include "Helpers.h"

using std::string;
using std::vector;

using namespace SciRooPlot;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  auto [datasetFile, plotFile, outputDir] = PlotManager::GetProjectSettings();

  // check if specified input files exist
  if (datasetFile.empty() || plotFile.empty()) {
    ERROR("No plotting project is selected. Exiting.");
    return 1;
  }
  if (!file_exists(datasetFile)) {
    ERROR(R"(File "{}" does not exist! Exiting.)", datasetFile);
    return 1;
  }
  if (!file_exists(plotFile)) {
    ERROR(R"(File "{}" does not exist! Exiting.)", plotFile);
    return 1;
  }

  string group;
  string name;
  string mode;

  // handle user inputs
  try {
    po::options_description arguments("positional arguments");
    arguments.add_options()("group", po::value<string>(), "group")("name", po::value<string>(), "name")("mode", po::value<string>(), "mode");
    po::positional_options_description pos;
    pos.add("group", 1);
    pos.add("name", 1);
    pos.add("mode", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(arguments).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.count("group")) {
      group = vm["group"].as<string>();
    }
    if (vm.count("name")) {
      name = vm["name"].as<string>();
    }
    if (vm.count("mode")) {
      mode = vm["mode"].as<string>();
    }
  } catch (std::exception& e) {
    ERROR(R"(Exception "{}"! Exiting.)", e.what());
    return 1;
  } catch (...) {
    ERROR("Exception of unknown type! Exiting.");
    return 1;
  }

  if (mode.empty()) mode = "show";

  if (group.empty() || name.empty()) {
    ERROR("No plots were specified.");
    return 1;
  }

  // create plotting environment
  PlotManager pm;
  pm.SetOutputDirectory(outputDir);
  gSystem->Setenv("SCIROOPLOT_USER_DATA_DIR", std::filesystem::path(datasetFile).parent_path().string().data());

  group += "(/.*)?";  // search also in subgroups
  pm.LoadPlots(plotFile, name, group);
  if (mode == "list") {
    pm.ListPlots();
  } else {
    pm.LoadDatasets(datasetFile);
    if (outputDir.empty() && mode != "show") {
      WARNING(R"(Please run "srp set <project> OUT <dir>" to set the output directory.)");
    }
    pm.GeneratePlots(mode);
  }
  return 0;
}
