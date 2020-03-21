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


// plotting app
int main(int argc, char *argv[]) {

  // generate
  
  // workflow:
  // use your own program to generate plot templates and dump them to files
  
  // read in (selected) plot definitions from file and generate plots
  //
  
  
  // usage:
  // if you have it in files already
  // ./plot {help, find, interactive, pdf, } [figureGroups] [plotNames]
  // keyword: all, any
  // --file=myPlots.xml
  // or put plotDefinitions.xml and filePath.xml in config file?

  // whatever needs to be created
  // ./plot generate myPlots.cxx dump?
  // --file=myPlots.cxx
  // path to executable?
  
  // dump input files
  // dump plots
  // sessions

  string outputPath = "~/Desktop/testPlots";
  string inputFileConfig = "/Users/mkrueger/Desktop/PlottingFramework/config/inputFiles";
  string plotConfig = "/Users/mkrueger/Desktop/PlottingFramework/config/plotDefinitions";
  string outputFileName = "myPlots.root";

  // create plotting environment
  PlotManager plotEnv;
  plotEnv.SetOutputDirectory(outputPath);
  plotEnv.SetUseUniquePlotNames(false);

  
  vector<string> dataSets;
  if(argc > 1 && (string(argv[1]) == "help" || string(argv[1]) == "--h")){
    cout << endl;
    cout << "Update plotDefinitions with output from current main program:" << endl << endl;
    cout << "  ./plot" << endl << endl;
    cout << "Update plotDefinitions with output for specific dataset from current main program:" << endl << endl;
    cout << "  ./plot dataset" << endl << endl;
    cout << "Create plots:" << endl << endl;
    cout << "  ./plot [figureGroup,figureGroup2 | all] [plotName,plotName2 | all] [interactive | pdf | eps | bitmap | file]" << endl << endl;
    cout << "Find plots defined in plotDefinitions file:" << endl << endl;
    cout << "  ./plot find [plotNameRegexp] [inputIdentifierRegexp]" << endl << endl;
    cout << endl;
    return 0;
  }
  if(argc > 1 && (string(argv[1]) == "find")){
    string plotNameRegex = "";
    string inputIdentifierRegexp = "";
    if(argc > 2) plotNameRegex = argv[2];
    if(argc > 3) inputIdentifierRegexp = argv[3];
    plotEnv.ListPlotsDefinedInFile(plotConfig, plotNameRegex, inputIdentifierRegexp);
    return 0;
  }
  else if(argc > 2){
    // plot only specific plots stored in the plotConfig file
    plotEnv.LoadInputDataFiles(inputFileConfig);

    string inputIdentifierString = argv[1];
    std::istringstream inputIdentifierStringStream(inputIdentifierString);
    vector<string> inputIdentifiers;
    string tempName;
    while(std::getline(inputIdentifierStringStream, tempName, ',')) {
        inputIdentifiers.push_back(tempName);
    }

    string fileNameString = argv[2];
    std::istringstream fileNameStringStream(fileNameString);
    vector<string> fileNames;
    while(std::getline(fileNameStringStream, tempName, ',')) {
        fileNames.push_back(tempName);
    }
    if(fileNameString == "all") fileNames = {};

    string outputMode = "interactive";
    if(argc > 3 && argv[3]) outputMode = argv[3];
    
    for(auto& inputIdentifier : inputIdentifiers){
      plotEnv.CreatePlotsFromFile(plotConfig, inputIdentifier, fileNames, outputMode);
    }
    return 0;
  }
  else if(argc > 1){
    // update plots defined in file for specific dataset
    plotEnv.LoadPlots(plotConfig);
    if(!(string(argv[1]) == "none")) dataSets.push_back(argv[1]); // none loads no dataset and therefore only overrides the combined plots
  }
  else
  {
    // compiled my plots library
    cout << "bla"<<endl;
  }

}
