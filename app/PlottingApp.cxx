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

#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "Helpers.h"

using namespace SciRooPlot;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  auto [inputFiles, plotDefinitions, outputDir] = PlotManager::GetProjectSettings();

  // check if specified input files exist
  if (!file_exists(inputFiles)) {
    ERROR(R"(File "{}" does not exists! Exiting.)", inputFiles);
    return 1;
  }
  if (!file_exists(plotDefinitions)) {
    ERROR(R"(File "{}" does not exists! Exiting.)", plotDefinitions);
    return 1;
  }

  string mode;
  string figureGroupAndCategory;
  string plotNames;

  // handle user inputs
  try {
    po::options_description arguments("positional arguments");
    arguments.add_options()("figureGroupAndCategory", po::value<string>(), "figure group")("plotNames", po::value<string>(), "plot name")("mode", po::value<string>(), "mode");
    po::positional_options_description pos;
    pos.add("figureGroupAndCategory", 1);
    pos.add("plotNames", 1);
    pos.add("mode", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(arguments).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.count("figureGroupAndCategory")) {
      figureGroupAndCategory = vm["figureGroupAndCategory"].as<string>();
    }
    if (vm.count("plotNames")) {
      plotNames = vm["plotNames"].as<string>();
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

  if (mode.empty()) mode = "interactive";

  if (plotNames.empty()) {
    ERROR("No plots were specified.");
    return 1;
  }

  // create plotting environment
  PlotManager plotManager;
  plotManager.SetOutputDirectory(outputDir);

  string group = ".+";
  string category = ".*";

  vector<string> groupCat = split_string(figureGroupAndCategory, '/', true);
  if (groupCat.size() > 0 && !groupCat[0].empty()) {
    group = groupCat[0];
  }
  if (groupCat.size() > 1 && !groupCat[1].empty()) {
    category = groupCat[1];
    category += "(/.*)?"; // search also in subcategories
  }

  if (mode != "find") {
    plotManager.LoadInputDataFiles(inputFiles);
    if (outputDir.empty() && mode != "interactive") {
      WARNING(R"(Please run "plot-config outputDir <PROJECT_NAME> </path/to/outputDir>" to set the ouptut directory.)");
    }
  }
  plotManager.ExtractPlotsFromFile(plotDefinitions, mode, plotNames, group, category);
  return 0;
}
