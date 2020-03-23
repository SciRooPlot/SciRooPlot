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


#include "MultDepSpec.h"

int main(int argc, char *argv[])
{
  string configDir = "~/Desktop/PlottingFramework/config";
  string inputFilesConfig = configDir + "/inputFiles.XML";
  string plotDefConfig = configDir + "/plotDefinitions.XML";
  string outputFilesBasePath = "~/Desktop/testPlots";
  
  // create plotting environment
  PlotManager plotEnv;
  plotEnv.SetOutputDirectory(outputFilesBasePath);
  plotEnv.SetUseUniquePlotNames(false);
  
  vector<string> dataSets;
  dataSets.push_back("pp_2TeV");
  dataSets.push_back("pp_5TeV");
  dataSets.push_back("pp_7TeV");
  dataSets.push_back("pp_13TeV");
  dataSets.push_back("pPb_5TeV");
  dataSets.push_back("pPb_8TeV");
  dataSets.push_back("PbPb_5TeV");
  dataSets.push_back("XeXe_5TeV");
  dataSets.push_back("Fits");
  dataSets.push_back("Simulations");
  dataSets.push_back("Energyscan");
  
  // generate config file containing locations of the input files or load it
  if(argc > 1 && string(argv[1]) == "updateInputFiles")
  {
    for(string dataSet : dataSets){
      string folder = "~/Desktop/AliMultDepSpec/Datasets/" + dataSet + "/";
      string inputFile = folder + dataSet + "_Results.root";
      string inputFileSyst = folder + dataSet + "_Syst.root";
      
      if(dataSet == "Publications" || dataSet == "Simulations" || dataSet == "Energyscan" || dataSet == "Fits"){
        plotEnv.AddInputDataFiles(dataSet, {inputFile});
      }else{
        plotEnv.AddInputDataFiles(dataSet, {inputFile, inputFileSyst});
      }
    }
    plotEnv.DumpInputDataFiles(inputFilesConfig);
    cout << "Updated input files: '" << inputFilesConfig << "'." << endl;
    return 0;
  }
  else{
    plotEnv.LoadInputDataFiles(inputFilesConfig);
  }
  
  // create dataset specific plots:
  for(string dataSet : dataSets)
  {
    if(dataSet == "Publications" || dataSet == "Simulations"|| dataSet == "Energyscan" || dataSet == "Fits") continue;
    MultDepSpec::DefineDatasetPlots(dataSet, plotEnv);
  }
  
  MultDepSpec::DefineTestPlots(plotEnv);
  MultDepSpec::DefineSystemPlots(plotEnv);
  MultDepSpec::DefineEnergyPlots(plotEnv);
  //MultDepSpec::DefinePublicationPlots(plotEnv);
  
  plotEnv.DumpPlots(plotDefConfig);
  return 0;
}
