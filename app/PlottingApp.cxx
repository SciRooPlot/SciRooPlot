// Plotting Framework
//
// Copyright (C) 2019-2021  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see docs/Credits
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
#include "HelperFunctions.h"

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

  string inputFilesConfig = configFolder + "inputFiles.XML";
  string plotDefConfig = configFolder + "plotDefinitions.XML";

  string mode;
  string figureGroups;
  string plotNames;

  // handle user inputs
  try {
    po::options_description options("Configuration options");
    options.add_options()("help", "Show this help message.")(
      "inputFilesConfig", po::value<string>(),
      "Location of config file containing the input file paths.")(
      "plotDefConfig", po::value<string>(),
      "Location of config file containing the plot definitions.")(
      "outputFolder", po::value<string>(), "Folder where output files should be saved.");

    po::options_description arguments("Positional arguments");
    arguments.add_options()("mode", po::value<string>(), "mode")(
      "figureGroups", po::value<string>(), "figure group")(
      "plotNames", po::value<string>(), "plot name")("arguments", po::value<vector<string>>(),
                                                     "arguments");

    po::positional_options_description
      pos; // this needs to be synchronous with the arguments options_description
    pos.add("figureGroups", 1);
    pos.add("plotNames", 1);
    pos.add("mode", 1);
    pos.add("arguments", -1);

    po::variables_map vm;
    po::options_description cmdline_options;
    cmdline_options.add(options).add(arguments);
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(pos).run(),
              vm);
    po::notify(vm);

    if (vm.count("help")) {
      PRINT("");
      PRINT("Usage:");
      PRINT(
        "  ./plot "
        "'<figureGroupRegex[:figureCategoryRegex]>'  '<plotNameRegex>' <find|interactive|pdf|eps|png|macro|file> \n");
      PRINT("You can use any standard regular expressions like 'begin.*end' or 'begin[a,b,c]end'.");
      PRINT(
        "Multiple figureGroups and plotNames can be specified separated by blank space: 'plotA "
        "plotB'.");
      PRINT(
        "When using regular expressions or multiple entries, it is required to embrace this in "
        "quotes.");
      PRINT("The use of blank spaces and colons in the regular expressions is not supported.");
      PRINT("You can also have a quick look into input identifiers or .root files:");
      PRINT("  ./plot browse '<inputIdentifier|rootfile[:fileSubPath]>'  '<dataName>'\n");
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
    if (vm.count("inputFilesConfig")) {
      inputFilesConfig = vm["inputFilesConfig"].as<string>();
    }
    if (vm.count("plotDefConfig")) {
      plotDefConfig = vm["plotDefConfig"].as<string>();
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
  if (!file_exists(expand_path(inputFilesConfig))) {
    ERROR(R"(File "{}" does not exists! Exiting.)", inputFilesConfig);
    return 1;
  }
  if (!file_exists(expand_path(plotDefConfig))) {
    ERROR(R"(File "{}" does not exists! Exiting.)", plotDefConfig);
    return 1;
  }

  if (plotNames.empty()) {
    ERROR("No plots were specified.");
    return 1;
  }

  // create plotting environment
  PlotManager plotManager;
  plotManager.SetOutputDirectory(outputFolder);
  INFO(R"(Reading plot definitions from "{}".)", plotDefConfig);

  vector<string> figureGroupsVector = split_string(figureGroups, ' ');
  vector<string> plotNamesVector = split_string(plotNames, ' ');
  if (mode == "find") {
    PRINT_SEPARATOR;
    plotManager.ExtractPlotsFromFile(plotDefConfig, figureGroupsVector, plotNamesVector, mode);
    PRINT_SEPARATOR;
    return 0;
  } else if (mode == "browse") { // directly plot histograms from input identifier or file
    string inputIdentifier = figureGroupsVector[0];
    if (inputIdentifier.find(".root") == string::npos) {
      plotManager.LoadInputDataFiles(inputFilesConfig);
    } else {
      plotManager.AddInputDataFiles("browse", {inputIdentifier});
      inputIdentifier = "browse";
    }
    Plot plot("plot", inputIdentifier);
    for (auto& dataName : plotNamesVector)
      plot[1].AddData(dataName, inputIdentifier, "<name>");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
    plotManager.CreatePlots(inputIdentifier, "", {}, "interactive");
  } else {
    INFO(R"(Reading input files from "{}".)", inputFilesConfig);
    plotManager.LoadInputDataFiles(inputFilesConfig);
    plotManager.ExtractPlotsFromFile(plotDefConfig, figureGroupsVector, plotNamesVector, mode);
    return 0;
  }
}
