// PlottingFramework
//
// Copyright (C) 2019-2021  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see doc/CONTRIBUTORS.md
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "PlottingFramework.h"
#include "PlotManager.h"
#include "Plot.h"
#include "Logging.h"

#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "Helpers.h"

using namespace PlottingFramework;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{

  string configFileName = (gSystem->Getenv("__PLOTTING_CONFIG_FILE"))
                            ? expand_path("${__PLOTTING_CONFIG_FILE}")
                            : "~/.plotconfig.xml";
  configFileName = expand_path(configFileName);

  string inputFiles;
  string plotDefinitions;
  string outputDir;

  ptree activeConfigTree;
  if (file_exists(configFileName)) {
    using boost::property_tree::read_xml;
    ptree configTree;
    read_xml(configFileName, configTree, boost::property_tree::xml_parser::trim_whitespace);
    string activeSettings;
    if (auto activated = configTree.get_child_optional("activated")) {
      activeSettings = activated.get().data();
    }
    if (auto curConfig = configTree.get_child_optional(activeSettings)) {
      auto tree = curConfig.get();
      if (auto property = tree.get_child_optional("plotDefinitions")) {
        plotDefinitions = property->get_value<string>();
      }
      if (auto property = tree.get_child_optional("inputFiles")) {
        inputFiles = property->get_value<string>();
      }
      if (auto property = tree.get_child_optional("outputDir")) {
        outputDir = property->get_value<string>();
      }
    }
  } else {
    ERROR("Plotting app was not configured. Please run plot-config ...");
    return 1;
  }

  // check if specified input files exist
  if (!file_exists(expand_path(inputFiles))) {
    ERROR(R"(File "{}" does not exists! Exiting.)", inputFiles);
    return 1;
  }
  if (!file_exists(expand_path(plotDefinitions))) {
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
  }
  plotManager.ExtractPlotsFromFile(plotDefinitions, mode, plotNames, group, category);
  return 0;
}
