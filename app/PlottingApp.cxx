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
#include "Helpers.h"

using namespace PlottingFramework;
namespace po = boost::program_options;

// This program is intended to generate plots from plotDefinitions saved in xml files
int main(int argc, char* argv[])
{
  // set default values
  string configFolder = (gSystem->Getenv("__PLOTTING_CONFIG_DIR"))
                          ? expand_path("${__PLOTTING_CONFIG_DIR}/")
                          : "plotting_config/";

  string outputFolder = (gSystem->Getenv("__PLOTTING_OUTPUT_DIR"))
                          ? expand_path("${__PLOTTING_OUTPUT_DIR}/")
                          : "plotting_output/";

  string inputFiles = configFolder + "inputFiles.XML";
  string plotDefinitions = configFolder + "plotDefinitions.XML";

  string mode;
  string figureGroups;
  string plotNames;

  // handle user inputs
  try {
    po::options_description options("Configuration options");
    options.add_options()("help", "Show this help message.")(
      "inputFiles", po::value<string>(),
      "Location of config file containing the input file paths.")("plotDefinitions", po::value<string>(),
                                                                  "Location of config file containing the plot definitions.")("outputFolder", po::value<string>(), "Folder where output files should be saved.");

    po::options_description arguments("Positional arguments");
    arguments.add_options()("mode", po::value<string>(), "mode")(
      "figureGroups", po::value<string>(), "figure group")("plotNames", po::value<string>(), "plot name")("arguments", po::value<vector<string>>(), "arguments");

    po::positional_options_description
      pos; // this needs to be synchronous with the arguments options_description
    pos.add("figureGroups", 1);
    pos.add("plotNames", 1);
    pos.add("mode", 1);
    pos.add("arguments", -1);

    po::variables_map vm;
    po::options_description cmdline_options;
    cmdline_options.add(options).add(arguments);
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      PRINT("");
      PRINT("Usage:");
      PRINT(
        "  ./plot "
        "'<figureGroupRegex[/figureCategoryRegex]>'  '<plotNameRegex>' <find|interactive|pdf|eps|png|macro|file> \n");
      PRINT("You can use any standard regular expressions like 'begin.*end', 'begin[a,b,c]end', '(apple|banana)', etc.");
      PRINT("Regular expressions, need to be embraced in single quotes.");
      PRINT("To enable auto-completion on and global availability,");
      PRINT("add 'source /plotting/framework/location/.plotrc' to your .bashrc or .bash_aliases.");
      PRINT(
        "Locations of the configuration file containing the input file paths and the output "
        "directory");
      PRINT(
        "can be steered via the env variables __PLOTTING_CONFIG_DIR and __PLOTTING_OUTPUT_DIR.");
      PRINT("Alternatively the following command line options can be used:");
      PRINT("");
      cout << options << endl;
      return 0;
    }
    if (vm.count("inputFiles")) {
      inputFiles = vm["inputFiles"].as<string>();
    }
    if (vm.count("plotDefinitions")) {
      plotDefinitions = vm["plotDefinitions"].as<string>();
    }
    if (vm.count("outputFolder")) {
      outputFolder = vm["outputFolder"].as<string>();
    }
    if (vm.count("mode")) {
      mode = vm["mode"].as<string>();
    }
    if (vm.count("figureGroups")) {
      figureGroups = vm["figureGroups"].as<string>();
    }
    if (vm.count("plotNames")) {
      plotNames = vm["plotNames"].as<string>();
    }
    if (vm.count("arguments")) {
      PRINT("Found additional arguments:");
      for (auto& argument : vm["arguments"].as<vector<string>>()) {
        PRINT("   {}", argument);
      }
    }
  } catch (std::exception& e) {
    ERROR(R"(Exception "{}"! Exiting.)", e.what());
    return 1;
  } catch (...) {
    ERROR("Exception of unknown type! Exiting.");
    return 1;
  }

  if (mode.empty()) mode = "interactive";

  // check if specified input files exist
  if (!file_exists(expand_path(inputFiles))) {
    ERROR(R"(File "{}" does not exists! Exiting.)", inputFiles);
    return 1;
  }
  if (!file_exists(expand_path(plotDefinitions))) {
    ERROR(R"(File "{}" does not exists! Exiting.)", plotDefinitions);
    return 1;
  }

  if (plotNames.empty()) {
    ERROR("No plots were specified.");
    return 1;
  }

  // create plotting environment
  PlotManager plotManager;
  plotManager.SetOutputDirectory(outputFolder);
  if (mode == "find") {
    plotManager.ExtractPlotsFromFile(plotDefinitions, figureGroups, plotNames, mode);
    return 0;
  } else {
    plotManager.LoadInputDataFiles(inputFiles);
    plotManager.ExtractPlotsFromFile(plotDefinitions, figureGroups, plotNames, mode);
    return 0;
  }
}
