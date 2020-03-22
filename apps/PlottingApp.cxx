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

using PlottingFramework::PlotManager;
using PlottingFramework::Plot;
namespace po = boost::program_options;

// Helper function to split comma separated argument strings
vector<string> splitArguments(string argString, char deliminator = ',')
{
  vector<string> arguments;
  string currArg;
  std::istringstream argStream(argString);
  while(std::getline(argStream, currArg, deliminator)) {
      arguments.push_back(currArg);
  }
  return arguments;
}


// This program is intended to generate plots from plotDefinitions saved in xml files
int main(int argc, char *argv[])
{
  // FIXME: put this stuff in config file
  string outputPath = "~/Desktop/testPlots";
  string inputFilesConfig = "/Users/mkrueger/Desktop/PlottingFramework/config/inputFiles.XML";
  string plotDefConfig = "/Users/mkrueger/Desktop/PlottingFramework/config/plotDefinitions.XML";
  string outputFileName = "myPlots.root";

  string mode;
  string figureGroups;
  string plotNames;
  
  try {
    po::options_description options("Configuration options");
    options.add_options()
    ("help", "Show this help message.")
    ("inputFilesConfig", po::value<string>(), "Location of config file containing the input file paths.")
    ("plotDefConfig", po::value<string>(), "Location of config file containing the plot definitions.");
    
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
      cout << endl;
      cout << "Find plots defined in plotDefinitions file:" << endl << endl;
      cout << "  ./plot find  <plotNameRegexp> <figureGroupRegexp>" << endl << endl;
      cout << "Generate plots defined in plotDefinitions file:" << endl << endl;
      cout << "  ./plot <interactive|pdf|eps|bitmap|file> <figureGroup,figureGroup2|all> <plotName,plotName2|all>" << endl << endl;
      cout << options << "\n";

      return 0;
    }
    if (vm.count("inputFilesConfig")) {
        inputFilesConfig = vm["inputFilesConfig"].as<string>();
    }
    if (vm.count("plotDefConfig")) {
        plotDefConfig = vm["plotDefConfig"].as<string>();
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
    catch(std::exception& e) {
      std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
      std::cerr << "Exception of unknown type!\n";
    return 1;
  }


  // create plotting environment
  PlotManager plotEnv;
  plotEnv.SetOutputDirectory(outputPath);
  plotEnv.SetUseUniquePlotNames(false);

  if(mode == "find"){
    // TODO: make this find multiple things as well
    // TODO: put exact match on top of search results
    plotEnv.ListPlotsDefinedInFile(plotDefConfig, plotNames, figureGroups);
    cout << endl;
    return 0;
  }
  else{
    // plot only specific plots stored in the plotConfig file
    plotEnv.LoadInputDataFiles(inputFilesConfig);
    vector<string> figureGroupsVector = splitArguments(figureGroups);
    vector<string> plotNamesVector  = splitArguments(plotNames);
    if(plotNames == "all") plotNamesVector = {};
    //if(figureGroups == "all") figureGroupsVector = {}; //TODO: add this feature

    for(auto& inputIdentifier : figureGroupsVector){
      plotEnv.CreatePlotsFromFile(plotDefConfig, inputIdentifier, plotNamesVector, mode);
    }
    return 0;
  }
}
