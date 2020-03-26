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
namespace MultDepSpec{

const double range::minPt = 0.5;
const double range::maxPt = 50.;
const int range::minMult = 0;
const int range::maxMult = 100;

const string pp_5TeV::input::analysis = "pp_5TeV";
const string pp_5TeV::input::pythia = "Simulations:pythia/pp_5TeV";
const string pp_5TeV::input::epos_lhc = "Simulations:epos_lhc/pp_5TeV";
const string pp_5TeV::input::epos = "Simulations:epos/pp_5TeV"; // not yet available

}

int main(int argc, char *argv[])
{
  // create plotting environment
  PlotManager plotEnv;
  plotEnv.LoadInputDataFiles(MultDepSpec::gInputFilesConfig);

  // by default create plots for all available datasets
  vector<string> dataSets;
  dataSets.push_back("pp_2TeV");
  dataSets.push_back("pp_5TeV");
  dataSets.push_back("pp_7TeV");
  dataSets.push_back("pp_13TeV");
  dataSets.push_back("pPb_5TeV");
  dataSets.push_back("pPb_8TeV");
  dataSets.push_back("PbPb_5TeV");
  dataSets.push_back("XeXe_5TeV");

  // ... and all additional plots
  bool createTestPlots = true;
  bool createEnergyPlots = true;
  bool createSystemPlots = true;
  
  // user can override
  if(argc > 1){
    createTestPlots = false;
    createEnergyPlots = false;
    createSystemPlots = false;
    dataSets.clear();
    for(auto argID = 1; argID < argc; argID++)
    {
      string argument = argv[argID];
      if(argument == "test") createTestPlots = true;
      else if(argument == "energy") createEnergyPlots = true;
      else if(argument == "system") createSystemPlots = true;
      else dataSets.push_back(argv[argID]);
    }
  }

  // create dataset specific plots:
  for(string dataSet : dataSets)
  {
    MultDepSpec::DefineDatasetPlots(dataSet, plotEnv);
  }
  
  if(createTestPlots) MultDepSpec::DefineTestPlots(plotEnv);
  if(createEnergyPlots) MultDepSpec::DefineEnergyPlots(plotEnv);
  if(createSystemPlots) MultDepSpec::DefineSystemPlots(plotEnv);
  //MultDepSpec::DefinePublicationPlots(plotEnv);
  
  plotEnv.DumpPlots(MultDepSpec::gPlotDefConfig);
  LOGF("-- wrote plot definitions to %s", MultDepSpec::gPlotDefConfig.c_str());
  return 0;
}
