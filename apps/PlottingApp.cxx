// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
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
#include <sys/stat.h>

using PlottingFramework::PlotManager;
using PlottingFramework::Plot;
namespace po = boost::program_options;

// Helper function to split comma separated argument strings
vector<string> splitArguments(string argString, char deliminator = ' ')
{
  vector<string> arguments;
  string currArg;
  std::istringstream argStream(argString);
  while(std::getline(argStream, currArg, deliminator)) {
    arguments.push_back(currArg);
  }
  return arguments;
}
// Helper function to check if specified file is available on the system
inline bool fileExists(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


// This program is intended to generate plots from plotDefinitions saved in xml files
int main(int argc, char *argv[])
{
  // set default values
  string configFolder = (gSystem->Getenv("PLOTTING_CONFIG_FOLDER")) ?
  gSystem->ExpandPathName("${PLOTTING_CONFIG_FOLDER}/") : "plotting_config/";
  
  string outputFolder = (gSystem->Getenv("PLOTTING_OUTPUT_FOLDER")) ?
  gSystem->ExpandPathName("${PLOTTING_OUTPUT_FOLDER}/") : "plotting_output/";

  string inputFilesConfig = configFolder + "inputFiles.XML";
  string plotDefConfig = configFolder + "plotDefinitions.XML";

  string mode;
  string figureGroups;
  string plotNames;
  
  // handle user inputs
  try
  {
    po::options_description options("Configuration options");
    options.add_options()
    ("help", "Show this help message.")
    ("inputFilesConfig", po::value<string>(), "Location of config file containing the input file paths.")
    ("plotDefConfig", po::value<string>(), "Location of config file containing the plot definitions.")
    ("outputFolder", po::value<string>(), "Folder where output files should be saved.")
    ;
    
    po::options_description arguments("Positional arguments");
    arguments.add_options()
    ("mode", po::value<string>(), "mode")
    ("figureGroups", po::value<string>(), "figure group")
    ("plotNames", po::value<string>(), "plot name")
    ("arguments", po::value< vector<string> >(), "arguments");
    
    po::positional_options_description pos; // this needs to be synchronous with the arguments options_description
    pos.add("mode", 1);
    pos.add("figureGroups", 1);
    pos.add("plotNames", 1);
    pos.add("arguments", -1);
    
    po::variables_map vm;
    po::options_description cmdline_options;
    cmdline_options.add(options).add(arguments);
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(pos).run(), vm);
    po::notify(vm);
    
    if (vm.count("help")) {
      PRINT("");
      PRINT("Usage:");
      PRINT("  ./plot <find|interactive|pdf|eps|bitmap|file> '<figureGroupRegex[:figureCategoryRegex]>'  '<plotNameRegex>'\n");
      PRINT("You can use any standard regular expressions like 'begin.*end' or 'begin[a,b,c]end'.");
      PRINT("An exhaustive list of such expressions can be found here:");
      PRINT("-> http://www.cplusplus.com/reference/regex/ECMAScript/");
      PRINT("Multiple figureGroups and plotNames can be specified separated by blank space: 'plotA plotB'.");
      PRINT("When using regular expressions or multiple entries, it is required to embrace this in quotes.");
      PRINT("The use of blank spaces and colons in the regular expressions is not supported.");
      PRINT("Locations of the configuration file containing the input file paths and the output directory");
      PRINT("can be steered via the env variables PLOTTING_CONFIG_FOLDER and PLOTTING_OUTPUT_FOLDER.")
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
      cout << "Found additional arguments:" << endl;
      for(auto& argument : vm["arguments"].as< vector<string> >())
      {
        cout << "   " << argument << endl;
      }
    }
  }
  catch(std::exception& e)
  {
    ERROR("Exception \"{}\"! Exiting.", e.what());
    return 1;
  }
  catch(...)
  {
    ERROR("Exception of unknown type! Exiting.");
    return 1;
  }
  
  // check if specified input files exist
  if(!fileExists(gSystem->ExpandPathName(inputFilesConfig.c_str())))
  {
    ERROR("File \"{}\" does not exists! Exiting.", inputFilesConfig);
    return 1;
  }
  if(!fileExists(gSystem->ExpandPathName(inputFilesConfig.c_str())))
  {
    ERROR("File \"{}\" does not exists! Exiting.", plotDefConfig);
    return 1;
  }

  
  
  // create plotting environment
  PlotManager plotEnv;
  plotEnv.SetOutputDirectory(outputFolder);
  INFO("Reading plot definitions from {}.", plotDefConfig);

  vector<string> figureGroupsVector = splitArguments(figureGroups);
  vector<string> plotNamesVector  = splitArguments(plotNames);
  if(mode == "find"){
    PRINT_SEPARATOR;
    plotEnv.ExtractPlotsFromFile(plotDefConfig, figureGroupsVector, plotNamesVector, mode);
    PRINT_SEPARATOR;
    return 0;
  }
  else{
    INFO("Reading input files from {}.", inputFilesConfig);
    plotEnv.LoadInputDataFiles(inputFilesConfig);
    plotEnv.ExtractPlotsFromFile(plotDefConfig, figureGroupsVector, plotNamesVector, mode);
    return 0;
  }
}
