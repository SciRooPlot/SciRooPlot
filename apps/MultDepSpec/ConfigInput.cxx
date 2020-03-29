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
  string dataSetFolder = MultDepSpec::gDatasetFolder;
  
  // create plotting environment
  PlotManager plotEnv;
  
  // Define analysis inputs
  vector<string> dataSets;
  dataSets.push_back("pp_2TeV");
  dataSets.push_back("pp_5TeV");
  dataSets.push_back("pp_7TeV");
  dataSets.push_back("pp_13TeV");
  dataSets.push_back("pPb_5TeV");
  dataSets.push_back("pPb_8TeV");
  dataSets.push_back("PbPb_5TeV");
  dataSets.push_back("XeXe_5TeV");
  for(string dataSet : dataSets){
    string folder = dataSetFolder + dataSet + "/";
    string inputFile = folder + dataSet + "_Results.root";
    string inputFileSyst = folder + dataSet + "_Syst.root";
    plotEnv.AddInputDataFiles(dataSet, {inputFile, inputFileSyst});
  }

  // Define simulation inputs
  string simIdentifier = "Simulations";
  vector<string> simFiles;
  simFiles.push_back(dataSetFolder + simIdentifier + "/PYTHIA_ANGANTYR_EPOS-LHC.root");
  simFiles.push_back(dataSetFolder + simIdentifier + "/EPOS.root");
  plotEnv.AddInputDataFiles(simIdentifier, simFiles);

  string energyDependenceIdentifer = "Energyscan";
  plotEnv.AddInputDataFiles(energyDependenceIdentifer, {dataSetFolder + energyDependenceIdentifer + "/energy_dependence.root"});

  string publicationsIdentifer = "Publications";
  plotEnv.AddInputDataFiles(publicationsIdentifer, {dataSetFolder + publicationsIdentifer + "/" + publicationsIdentifer + ".root"});

  string fitsIdentifer = "Fits";
  plotEnv.AddInputDataFiles(fitsIdentifer, {dataSetFolder + fitsIdentifer + "/" + fitsIdentifer + ".root"});

  string geomScalingIdentifier = "GeometricalScaling";
  plotEnv.AddInputDataFiles(geomScalingIdentifier, {dataSetFolder + geomScalingIdentifier + "/" + geomScalingIdentifier + ".root"});
  
  INFO("Saving input file paths in: '{}'.", MultDepSpec::gInputFilesConfig);
  gSystem->Exec((string("mkdir -p ") + MultDepSpec::gConfigFolder).c_str());
  plotEnv.DumpInputDataFiles(MultDepSpec::gInputFilesConfig);
  return 0;
}
