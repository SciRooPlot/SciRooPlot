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

void MultDepSpec::DefineSystemPlots(PlotManager& plotManager)
{
  string plotGroup = "system_comparison";
  string systemSizeLable = alice + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  string systemSizeLablePrel = alicePrel + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  
  string energyLable = alice + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  string energyLableMC = "#bf{Pythia8 Monash13}" + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;

  
  // FIXME: put nch cent stuff in a separate graph
  vector<string> centrality = {"0-5%","5-10%","10-20%","20-30%","30-40%","40-50%","50-60%","60-70%","70-80%"};
  vector<double> nchCent = {2869.49, 2342.32, 1740.05, 1156.23, 750.512, 463.796, 265.249, 138.504, 64.0346};
  vector<double> nchCentErrors = {82.7027, 70.8452, 45.7133, 29.4205, 21.9877, 17.5017, 11.5956, 8.3419, 4.84894};
  

  string system_energy_lable = alice + newLine + chargedParticles + newLine + eta08 + ", " + ptRange;
  string pPb_lable = alice + newLine + chargedParticles + ", " + pPb + newLine + eta08 + ", " + ptRange;
  
  { // -----------------------------------------------------------------------
    // compare meanPt in pPb at 5.02 and 8.16 with all available models
    string plotName = "meanPt_pPb_models";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("momentUnfolded1", pPb_5TeV[data].input, "5.02 TeV data",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pPb_5TeV[data].input, "5.02 TeV data",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    plot[1].AddData("meanPt", pPb_5TeV[angantyr].input, "5.02 TeV angantyr",
                    pPb_5TeV[angantyr].marker, pPb_5TeV[angantyr].color, "", pPb_5TeV[angantyr].mult);
    plot[1].AddData("meanPt", pPb_5TeV[epos_lhc].input, "5.02 TeV epos-lhc",
                    pPb_5TeV[epos_lhc].marker, pPb_5TeV[epos_lhc].color, "", pPb_5TeV[epos_lhc].mult);
    plot[1].AddData("meanPt", pPb_5TeV[epos].input, "5.02 TeV epos",
                    pPb_5TeV[epos].marker, pPb_5TeV[epos].color, "", pPb_5TeV[epos].mult);
    plot[1].AddData("momentUnfolded1", pPb_8TeV[data].input, "8.16 TeV data",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pPb_8TeV[data].input, "8.16 TeV data",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    plot[1].AddData("momentUnfoldedMC1", pPb_8TeV[epos_lhc].input, "8.16 TeV epos-lhc",
                    pPb_8TeV[epos_lhc].marker, pPb_8TeV[epos_lhc].color, "", pPb_8TeV[epos_lhc].mult);
    
    plot[1].SetAxisRange("X", 0, 150);
    plot[1].SetAxisRange("Y", 0.45, 0.98);
    plot[1].AddLegend(0.15, 0.92);
    plot[1].AddText(0.3, 0.3, pPb_lable);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plotManager.AddPlot(plot);
   } // -----------------------------------------------------------------------

  
  { // -----------------------------------------------------------------------
    // compare meanPt in PbPb at 5.02 and XeXe at 5.44 with all available models
    string plotName = "meanPt_PbPb_XeXe_models";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("momentUnfolded1", PbPb_5TeV[data].input, "PbPb 5.02 TeV data",
                    PbPb_5TeV[data].marker,PbPb_5TeV[data].color);
    plot[1].AddData("momentUnfolded1_Syst", PbPb_5TeV[data].input, "PbPb 5.02 TeV data",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes");
    plot[1].AddData("momentGeneratedMC1", PbPb_5TeV[hijing].input, "PbPb 5.02 TeV hijing",
                    PbPb_5TeV[hijing].marker, PbPb_5TeV[hijing].color);
    plot[1].AddData("meanPt", PbPb_5TeV[epos_lhc].input, "PbPb 5.02 TeV epos-lhc",
                    PbPb_5TeV[epos_lhc].marker, PbPb_5TeV[epos_lhc].color);
    plot[1].AddData("meanPt", PbPb_5TeV[epos].input, "PbPb 5.02 TeV epos",
                    PbPb_5TeV[epos].marker, PbPb_5TeV[epos].color);
    plot[1].AddData("momentUnfolded1", XeXe_5TeV[data].input, "XeXe 5.44 TeV data",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color);
    plot[1].AddData("momentUnfolded1_Syst", XeXe_5TeV[data].input, "XeXe 5.44 TeV data",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "boxes");
    plot[1].AddData("momentGeneratedMC1", XeXe_5TeV[hijing].input, "XeXe 5.44 TeV hijing",
                    XeXe_5TeV[hijing].marker, XeXe_5TeV[hijing].color);
    
    plot[1].SetPadOptions("logX");
    plot[1].AddLegend(0.15, 0.92);
    plot[1].SetAxisRange("X", 1, 3000);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisRange("Y", 0.45, 0.98);
    plotManager.AddPlot(plot);
   } // -----------------------------------------------------------------------

  
  { // -----------------------------------------------------------------------
    // compare meanPt in all systems, data only
    string plotName = "meanPt_all";
    Plot plot(plotName, plotGroup);
    plot[1].AddFrame("momentUnfolded1", PbPb_5TeV[data].input);
    
    //p-p
    plot[1].AddData("momentUnfolded1", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    //p-Pb
    plot[1].AddData("momentUnfolded1", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pPb_5TeV[data].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pPb_8TeV[data].input, "",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pPb_8TeV[data].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    //Pb-Pb
    plot[1].AddData("momentUnfolded1", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", PbPb_5TeV[data].input, "Pb-Pb, 5.02 TeV",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    //Xe-Xe
    plot[1].AddData("momentUnfolded1", XeXe_5TeV[data].input, "",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", XeXe_5TeV[data].input, "Xe-Xe, 5.44 TeV",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);

    plot[1].SetPadOptions("logX");
    plot[1].SetAxisRange("X", 0.1, 4000);
    plot[1].SetAxisRange("Y", 0.45, 0.9);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    
    plot[1].AddLegend(0.14, 0.92);
    plot[1].AddText(0.36, 0.3, system_energy_lable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    // compare meanPt in all systems, data only
    string plotName = "meanPt_epos";
    Plot plot(plotName, plotGroup);
    plot[1].AddFrame("momentUnfolded1", PbPb_5TeV[data].input);
    
    //p-p
    plot[1].AddData("momentUnfolded1", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("meanPt", pp_5TeV[epos].input, "",
                    pp_5TeV[epos].marker, pp_5TeV[epos].color);
    //p-Pb
    plot[1].AddData("momentUnfolded1", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pPb_5TeV[data].input, "p-Pb",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    plot[1].AddData("meanPt", pPb_5TeV[epos].input, "",
                    pPb_5TeV[epos].marker, pPb_5TeV[epos].color, "", pPb_5TeV[epos].mult);
    //Pb-Pb
    plot[1].AddData("momentUnfolded1", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", PbPb_5TeV[data].input, "Pb-Pb",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    plot[1].AddData("meanPt", PbPb_5TeV[epos].input, "",
                    PbPb_5TeV[epos].marker, PbPb_5TeV[epos].color);

    plot[1].SetPadOptions("logX");
    plot[1].SetAxisRange("X", 0.1, 4000);
    //plot[1].SetAxisRange("X", 0.1, 120);
    plot[1].SetAxisRange("Y", 0.45, 0.9);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    
    plot[1].AddLegend(0.14, 0.92);
    plot[1].AddText(0.36, 0.3, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

   { // -----------------------------------------------------------------------
     // compare meanPt in all systems, data only
     string plotName = "meanPt_epos_lhc";
     Plot plot(plotName, plotGroup);
     plot[1].AddFrame("momentUnfolded1", PbPb_5TeV[data].input);
     
     //p-p
     plot[1].AddData("momentUnfolded1", pp_5TeV[data].input, "",
                     pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
     plot[1].AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp",
                     pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
     plot[1].AddData("meanPt", pp_5TeV[epos_lhc].input, "",
                     pp_5TeV[epos_lhc].marker, pp_5TeV[epos_lhc].color);
     //p-Pb
     plot[1].AddData("momentUnfolded1", pPb_5TeV[data].input, "",
                     pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
     plot[1].AddData("momentUnfolded1_Syst", pPb_5TeV[data].input, "p-Pb",
                     pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
     plot[1].AddData("meanPt", pPb_5TeV[epos_lhc].input, "",
                     pPb_5TeV[epos_lhc].marker, pPb_5TeV[epos_lhc].color, "", pPb_5TeV[epos_lhc].mult);
     //Pb-Pb
     plot[1].AddData("momentUnfolded1", PbPb_5TeV[data].input, "",
                     PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
     plot[1].AddData("momentUnfolded1_Syst", PbPb_5TeV[data].input, "Pb-Pb",
                     PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
     plot[1].AddData("meanPt", PbPb_5TeV[epos_lhc].input, "",
                     PbPb_5TeV[epos_lhc].marker, PbPb_5TeV[epos_lhc].color, "", PbPb_5TeV[data].mult);

     plot[1].SetPadOptions("logX");
     plot[1].SetAxisRange("X", 0.1, 4000);
     //plot[1].SetAxisRange("X", 0.1, 120);

     plot[1].SetAxisRange("Y", 0.45, 0.9);
     plot[1].SetAxisTitle("X", "#it{N}_{ch}");
     
     plot[1].AddLegend(0.14, 0.92);
     plot[1].AddText(0.36, 0.3, systemSizeLable);
     plotManager.AddPlot(plot);
   } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    // compare meanPt in pp, data only
    string plotName = "meanPt_pp";
    Plot plot(plotName, plotGroup);
    //p-p
    plot[1].AddData("momentUnfolded1", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("momentUnfolded1", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);

    plot[1].SetAxisRange("X", 0.1, 70);
    plot[1].SetAxisRange("Y", 0.45, 0.9);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    
    plot[1].AddLegend(0.14, 0.92);
    plot[1].AddText(0.36, 0.3, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    // compare variance in all systems, data only
    string plotName = "variance_all";
    Plot plot(plotName, plotGroup);
    plot[1].AddFrame("varianceUnfolded", PbPb_5TeV[data].input);
    
    //p-p
    plot[1].AddData("varianceUnfolded", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("varianceUnfolded", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("varianceUnfolded", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("varianceUnfolded", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    //p-Pb
    plot[1].AddData("varianceUnfolded", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", pPb_5TeV[data].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    plot[1].AddData("varianceUnfolded", pPb_8TeV[data].input, "",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", pPb_8TeV[data].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    //Pb-Pb
    plot[1].AddData("varianceUnfolded", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", PbPb_5TeV[data].input, "Pb-Pb, 5.02 TeV",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    //Xe-Xe
    plot[1].AddData("varianceUnfolded", XeXe_5TeV[data].input, "",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    plot[1].AddData("varianceUnfolded_Syst", XeXe_5TeV[data].input, "Xe-Xe, 5.44 TeV",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    
    plot[1].SetPadOptions("logX");
    plot[1].SetAxisRange("X", 0.1, 4000);
    plot[1].SetAxisRange("Y", 0.05, 0.95);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    
    plot[1].AddLegend(0.14, 0.92);
    plot[1].AddText(0.36, 0.3, system_energy_lable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multiplicity_all";
    Plot plot(plotName, plotGroup);
    plot[1].AddFrame("multDensityUnfolded", PbPb_5TeV[data].input);

    plot[1].AddData("multDensityUnfolded", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_7TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    //p-Pb
    plot[1].AddData("multDensityUnfolded", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pPb_5TeV[data].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pPb_8TeV[data].input, "",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pPb_8TeV[data].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    //Pb-Pb
    plot[1].AddData("multDensityUnfolded", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", PbPb_5TeV[data].input, "Pb-Pb, 5.02 TeV",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    //Xe-Xe
    plot[1].AddData("multDensityUnfolded", XeXe_5TeV[data].input, "",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", XeXe_5TeV[data].input, "Xe-Xe, 5.44 TeV",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    
    plot[1].SetPadOptions("logY logX");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].AddText(0.12, 0.3, systemSizeLable);
    plot[1].AddLegend(0.6, 0.92);
    plot[1].SetAxisRange("Y", 1e-5, 1);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  
  { // -----------------------------------------------------------------------
    string plotName = "multiplicity_pp";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    
    plot[1].AddData("multDensityUnfolded", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("multDensityUnfolded", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("multDensityUnfolded_Syst", pp_7TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);

    plot[1].AddData("multDist_pp_7TeV_Stat", "Publications", "", kFullCircle, kMagenta+1, "", 52);
    plot[1].AddData("multDist_pp_7TeV_Syst", "Publications", "7 TeV", kFullCircle, kMagenta+1, "boxes", 52);
    plot[1].AddData("multDist_pp_0.9TeV_Stat", "Publications", "", kFullStar, kBlack, "", 33);
    plot[1].AddData("multDist_pp_0.9TeV_Syst", "Publications", "0.9 TeV", kFullStar, kBlack, "boxes", 33);

    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{P}(#it{N}_{ch})");
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 1e-4, 3e-1);
    plot[1].AddLegend(0.18, 0.45);
    plot[1].AddText(0.4, 0.9, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multiplicity_KNO_pp";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    
    plot[1].AddData("multDistUnfoldedKNO", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("multDistUnfoldedKNO_Syst", pp_7TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);

    plot[1].AddData("multDist_pp_7TeVKNO_Stat", "Publications", "", kOpenCircle, kMagenta+1, "");
    plot[1].AddData("multDist_pp_7TeVKNO_Syst", "Publications", "7 TeV", kOpenCircle, kMagenta+1, "boxes");
    plot[1].AddData("multDist_pp_0.9TeVKNO_Stat", "Publications", "", kOpenStar, kBlack, "");
    plot[1].AddData("multDist_pp_0.9TeVKNO_Syst", "Publications", "0.9 TeV", kOpenStar, kBlack, "boxes");
    plot[1].SetAxisRange("X", 0, 5);
    //plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].AddLegend(0.7, 0.9);
    plot[1].AddText(0.15, 0.3, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  
   
  { // -----------------------------------------------------------------------
    string plotName = "meanPt_geom_scaling";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("meanPtScaled", pp_2TeV[geom_scaling].input, "",
                    pp_2TeV[geom_scaling].marker, pp_2TeV[geom_scaling].color, "", 2.14);
    plot[1].AddData("meanPtScaled_Syst", pp_2TeV[geom_scaling].input, "pp, 2.76 TeV",
                    pp_2TeV[geom_scaling].marker, pp_2TeV[geom_scaling].color, "boxes", 2.14);
    plot[1].AddData("meanPtScaled", pp_5TeV[geom_scaling].input, "",
                    pp_5TeV[geom_scaling].marker, pp_5TeV[geom_scaling].color, "", 2.8);
    plot[1].AddData("meanPtScaled_Syst", pp_5TeV[geom_scaling].input, "pp, 5.02 TeV",
                    pp_5TeV[geom_scaling].marker, pp_5TeV[geom_scaling].color, "boxes", 2.8);
    plot[1].AddData("meanPtScaled", pp_7TeV[geom_scaling].input, "",
                    pp_7TeV[geom_scaling].marker, pp_7TeV[geom_scaling].color, "", 2.8);
    plot[1].AddData("meanPtScaled_Syst", pp_7TeV[geom_scaling].input, "pp, 7 TeV",
                    pp_7TeV[geom_scaling].marker, pp_7TeV[geom_scaling].color, "boxes", 2.8);
    plot[1].AddData("meanPtScaled", pp_13TeV[geom_scaling].input, "",
                    pp_13TeV[geom_scaling].marker, pp_13TeV[geom_scaling].color, "", 3.3);
    plot[1].AddData("meanPtScaled_Syst", pp_13TeV[geom_scaling].input, "pp, 13 TeV",
                    pp_13TeV[geom_scaling].marker, pp_13TeV[geom_scaling].color, "boxes", 3.3);
    plot[1].AddData("meanPtScaled", pPb_5TeV[geom_scaling].input, "",
                    pPb_5TeV[geom_scaling].marker, pPb_5TeV[geom_scaling].color, "", 2.5);
    plot[1].AddData("meanPtScaled_Syst", pPb_5TeV[geom_scaling].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[geom_scaling].marker, pPb_5TeV[geom_scaling].color, "boxes", 2.5);
    plot[1].AddData("meanPtScaled", pPb_8TeV[geom_scaling].input, "",
                    pPb_8TeV[geom_scaling].marker, pPb_8TeV[geom_scaling].color, "", 2.6);
    plot[1].AddData("meanPtScaled_Syst", pPb_8TeV[geom_scaling].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[geom_scaling].marker, pPb_8TeV[geom_scaling].color, "boxes", 2.6);
    plot[1].AddLegend(0.15, 0.9);
    plot[1].SetAxisRange("X", 0.7, 3.7);
    plot[1].SetAxisRange("Y", 0.48, 1.0);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_pPbOverpp";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].AddRatio("reducedMultPtpPb", "Fits", "multPtUnfoldedNormalized", "pp_5TeV"); // This does not work!!
    plot[1].SetAxisRange("Z", 0.0, 2.0);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "p-Pb / pp");
    plot[1].AddText(0.2, 0.9, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_PbPbOverpp";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].AddRatio("reducedMultPtPbPb", "Fits", "multPtUnfoldedNormalized", "pp_5TeV");
    plot[1].SetAxisRange("Z", 0.0, 2.0);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "Pb-Pb / pp");
    plot[1].AddText(0.2, 0.9, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPbPbXeXe";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("michaelRatio_XeXe", "Fits", "Xe-Xe", 0, kBlue+2);
    plot[1].AddData("michaelRatio_PbPb", "Fits", "Pb-Pb", 0, kBlack);
    plot[2].AddRatio("michaelRatio_XeXe", "Fits", "michaelRatio_PbPb", "Fits", "", 0, kBlue+2);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.3, 0.5, "1200 < #it{N}_{ch} < 2300");
    plot[2].SetAxisRange("Y", 0.6, 1.1);
    plot[1].AddText(0.18, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("Y", "extreme / nearby");
    plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPbPb";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("spectraExtreme_PbPb", "Fits", "#it{N}_{ch} > 2000", 0, kBlue+2);
    plot[1].AddData("spectraNearby_PbPb", "Fits", "700 < #it{N}_{ch} < 1200", 0, kBlack);
    plot[2].AddRatio("spectraExtreme_PbPb", "Fits", "spectraNearby_PbPb", "Fits", "", 0, kBlue+2);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.4, 0.5, "#bf{Pb-Pb}");
    plot[2].SetAxisRange("Y", 1.3, 3.3);
    plot[1].AddText(0.18, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("Y", "extreme / nearby");
    plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultXeXe";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("spectraExtreme_XeXe", "Fits", "#it{N}_{ch} > 1200", 0, kBlue+2);
    plot[1].AddData("spectraNearby_XeXe", "Fits", "500 < #it{N}_{ch} < 800", 0, kBlack);
    plot[2].AddRatio("spectraExtreme_XeXe", "Fits", "spectraNearby_XeXe", "Fits", "", 0, kBlue+2);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.4, 0.5, "#bf{Xe-Xe}");
    plot[1].AddText(0.18, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("Y", "extreme / nearby");
    plot[2].SetAxisRange("Y", 1.3, 3.3);
    plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPP";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("inclusiveSpectrum", "pp_5TeV", "", 0, kBlack);
    plot[1].AddData("spectraExtreme_pp", "Fits", "#it{N}_{ch} > 40", 0, kBlue+2);
    plot[1].AddData("inclusiveSpectrum", "pp_5TeV", "all #it{N}_{ch}", 0, kBlack);
    plot[2].AddRatio("spectraExtreme_pp", "Fits", "inclusiveSpectrum", "pp_5TeV", "", 0, kBlue+2);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.4, 0.5, "#bf{pp}");
    plot[1].AddText(0.18, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("Y", "extreme/MB");
    plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyDepHighNchSpectra";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("spectraHighNch_pp_13TeV", "Fits", "", 0, 0);
    plot[1].AddData("spectraHighNch_pp_2TeV", "Fits", "", 0, 0);
    plot[1].AddData("spectraHighNch_pp_5TeV", "Fits", "", 0, 0);
    plot[1].AddData("spectraHighNch_pp_7TeV", "Fits", "", 0, 0);
    //plot[2].AddRatio("spectraHighNch_pp_2TeV", "Fits", "spectraHighNch_pp_13TeV", "Fits", "", 0);
    plot[2].AddRatio("spectraHighNch_pp_5TeV", "Fits", "spectraHighNch_pp_13TeV", "Fits", "", 0);
    plot[2].AddRatio("spectraHighNch_pp_7TeV", "Fits", "spectraHighNch_pp_13TeV", "Fits", "", 0);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.4, 0.5, "#bf{pp}");
    plot[1].AddText(0.18, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("Y", "ratio to 13");
    plot[2].SetPadOptions("logX");
    plot[1].SetAxisTitle("X", "#it{p}_{T}");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPPb";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("inclusiveSpectrum", "pPb_5TeV", "", 0, kBlack);
    plot[1].AddData("spectraExtreme_pp", "Fits", "#it{N}_{ch} > 100", 0, kBlue+2);
    plot[1].AddData("inclusiveSpectrum", "pPb_5TeV", "all #it{N}_{ch}", 0, kBlack);
    plot[2].AddRatio("spectraExtreme_pp", "Fits", "inclusiveSpectrum", "pPb_5TeV", "", 0, kBlue+2);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.4, 0.5, "#bf{p-Pb}");
    plot[1].AddText(0.18, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("Y", "extreme/MB");
    //      plot[2].SetAxisRange("Y", 1.3, 3.3);
    plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyLowMult";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData("spectraNch2_PbPb", "Fits", "Pb-Pb, #it{N}_{ch} = 2", kFullSquare, kGreen+2);
    plot[1].AddData("spectraNch2_XeXe", "Fits", "Xe-Xe, #it{N}_{ch} = 2", kFullCircle, kGreen+2);
    plot[1].AddData("spectraNch1_PbPb", "Fits", "Pb-Pb, #it{N}_{ch} = 1", kFullSquare, kBlue+2);
    plot[1].AddData("spectraNch1_XeXe", "Fits", "Xe-Xe, #it{N}_{ch} = 1", kFullCircle, kBlue+2);
    plot[2].AddRatio("spectraNch1_PbPb", "Fits", "spectraNch1_XeXe", "Fits", "", kFullSquare, kBlue+2);
    plot[2].AddRatio("spectraNch2_PbPb", "Fits", "spectraNch2_XeXe", "Fits", "", kFullSquare, kGreen+2);
    plot[1].SetAxisRange("X", 0.15, 10);
    plot[2].SetAxisRange("Y", 0.5, 1.5);
    plot[1].AddLegend(0.2, 0.4);
    //plot[1].AddText(0.4, 0.3, "#bf{Xe-Xe}");
    plot[2].SetAxisTitle("Y", "Pb / Xe");
    plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityComparisonPerNuclPair";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY logX");
    plot[1].AddData("multDensityVsParticlesPerNucleon_XeXe_5TeV", "Fits", "Xe-Xe", kFullCircle, kGreen+2, "");
    plot[1].AddData("multDensityVsParticlesPerNucleon_PbPb_5TeV", "Fits", "Pb-Pb", kFullSquare, kRed+1, "");
    plot[1].AddLegend(0.7, 0.8);
    plot[1].AddText(0.2, 0.3, systemSizeLable);
    plot[1].SetAxisRange("Y", 1e-4, 3e-2);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtPerNuclPair";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("meanPtVsParticlesPerNucleon_XeXe_5TeV", "Fits", "Xe-Xe", kFullCircle, kGreen+2, "");
    plot[1].AddData("meanPtVsParticlesPerNucleon_PbPb_5TeV", "Fits", "Pb-Pb", kFullSquare, kRed+1, "");
    //plot[1].SetAxisRange("Y", 1e-4, 3e-2);
    //      plot[1].SetAxisRange("Y", 0.48, 0.75);
    plot[1].AddLegend(0.7, 0.4);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtPerNuclPairRatio";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("meanPtVsParticlesPerNucleon_XeXe_5TeV", "Fits", "Xe-Xe", kFullCircle, kGreen+2, "");
    plot[1].AddData("meanPtVsParticlesPerNucleon_PbPb_5TeV", "Fits", "Pb-Pb", kFullSquare, kRed+1, "");
    plot[1].SetAxisRange("Y", 0.75, 1.05);
    plot[1].SetAxisRange("X", 0, 10);
    plot[1].AddLegend(0.7, 0.4);
    for(int i = 0; i < 9; i++)
    {
      //plot[1].AddText(nchCent[i]/(2*208), 1, "#bf{|}", true);
    }
    for(int i = 0; i < 9; i++)
    {
      //plot[2].AddText(nchCent[i]/(2*208), 1, "#bf{|}", true);
    }
    plot[2].AddData("ratioMeanPtVsParticlesPerNucleon", "Fits", "Xe / Pb", kFullCircle, kGreen+2, "");
    plot[2].SetAxisRange("Y", 0.99, 1.01);
    plot[2].SetAxisTitle("Y", "Xe / Pb");
    plot[2].SetAxisRange("X", 0, 10);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1dNchdeta";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("momentUnfolded1dNchDeta", "pp_5TeV", "pp", 0, kBlue+1);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "varianceFullRangeMC";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logX");
    plot[1].AddData("varianceGeneratedMC", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    plot[1].AddData("varianceGeneratedMC", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 50);
    plot[1].AddData("varianceGeneratedMC", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 85);
    plot[1].AddData("varianceGeneratedMC", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 3000);
    plot[1].SetAxisRange("X", 0.1, 4000);
    plot[1].SetAxisRange("Y", 0.05, 0.8);
    plot[1].AddLegend(0.15, 0.6);
    plot[1].AddText(0.14, 0.92, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtpPb8Over5";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("momentUnfolded1", "pPb_5TeV", "", kFullCross, kRed+1, "");
    plot[1].AddData("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb, 5.02 TeV", kFullCross, kRed+1, "boxes");
    plot[1].AddData("momentUnfolded1", "pPb_8TeV", "", kFullStar, kBlue+1, "");
    plot[1].AddData("momentUnfolded1_Syst", "pPb_8TeV", "p-Pb, 8.16 TeV", kFullStar, kBlue+1, "boxes");
    plot[2].AddRatio("momentUnfolded1_Syst", "pPb_8TeV", "momentUnfolded1_Syst", "pPb_5TeV", "", kFullStar, kBlue+1, "boxes");
    
    plot[1].AddData("meanPt_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCross, kRed+1, "", 130);
    plot[1].AddData("momentGeneratedMC1", "pPb_8TeV", "", kOpenStar, kBlue+1, "boxes");
    plot[2].AddRatio("momentGeneratedMC1", "pPb_8TeV", "meanPt_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenStar, kBlue+1, "boxes");
    
    plot[2].SetAxisRange("Y", 0.9, 1.1);
    plot[2].SetAxisTitle("Y", "8 / 5 TeV");
    plot[1].SetAxisRange("X", 0, 100);
    plot[1].SetAxisRange("Y", 0.45, 0.95);
    plot[1].AddLegend(0.2, 0.8, "");
    
    plot[1].AddText(0.4, 0.3, alice + newLine + chargedParticles + newLine + eta08 + ", " + ptRange + newLine + "Open symbols: EPOS-LHC");
    plot[2].SetAxisTitle("X", "#it{N}_{ch}");
    
    
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtpPbOverPbp";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("momentUnfolded1", "pPb_8TeV", "", kFullStar, kBlue+1, "");
    plot[1].AddData("momentUnfolded1_Syst", "pPb_8TeV", "p-Pb, 8 TeV", kFullStar, kBlue+1, "boxes");
    plot[1].AddData("momentUnfolded1", "Pbp_8TeV", "", kFullStar, kGreen+1, "");
    plot[1].AddData("momentUnfolded1_Syst", "Pbp_8TeV", "Pb-p, 8 TeV", kFullStar, kGreen+1, "boxes");
    plot[2].AddRatio("momentUnfolded1_Syst", "Pbp_8TeV", "momentUnfolded1_Syst", "pPb_8TeV", "", kFullStar, kGreen+1, "boxes");
    plot[2].SetAxisRange("Y", 0.98, 1.02);
    plot[2].SetAxisTitle("Y", "Pbp/pPb");
    plot[1].SetAxisRange("X", 0, 150);
    plot[1].SetAxisRange("Y", 0.45, 0.95);
    plot[1].AddLegend(0.4, 0.4, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtPbOverPb";
    Plot plot(plotName, plotGroup, "default ratio");
    //      plot[1].SetPadOptions("logX");
    plot[1].AddData("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3500);
    plot[1].AddData("momentUnfolded1_Syst", "PbPb_5TeV", "2015", kFullCross, kRed+1, "boxes", 3500);
    plot[1].AddData("momentUnfolded1", "PbPb_5TeV_2018", "", kFullStar, kGreen+2, "", 3500);
    plot[1].AddData("momentUnfolded1_Syst", "PbPb_5TeV_2018", "2018", kFullStar, kGreen+2, "boxes", 3500);
    plot[2].AddRatio("momentUnfolded1_Syst", "PbPb_5TeV_2018", "momentUnfolded1_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 3500);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "2018/2015");
    plot[1].SetAxisRange("X", 0, 3400);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.2, 0.9, "");
    plot[1].AddText(0.4, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("X", "#it{N}_{ch}");
    //     plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityPbOverPb";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3500);
    plot[1].AddData("multDensityUnfolded_Syst", "PbPb_5TeV", "2015", kFullCross, kRed+1, "boxes", 3500);
    plot[1].AddData("multDensityUnfolded", "PbPb_5TeV_2018", "", kFullStar, kGreen+2, "", 3500);
    plot[1].AddData("multDensityUnfolded_Syst", "PbPb_5TeV_2018", "2018", kFullStar, kGreen+2, "boxes", 3500);
    plot[2].AddRatio("multDensityUnfolded_Syst", "PbPb_5TeV_2018", "multDensityUnfolded_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 3500);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "2018/2015");
    plot[1].SetAxisRange("X", 0, 3400);
    plot[2].SetAxisRange("Y", 0.9, 1.1);
    //          plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.2, 0.9, "");
    plot[1].AddText(0.4, 0.2, systemSizeLable);
    plot[2].SetAxisTitle("X", "#it{N}_{ch}");
    //     plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityPbpOverpPb";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDensityUnfolded", "pPb_8TeV", "", kFullCross, kRed+1, "", 3500);
    plot[1].AddData("multDensityUnfolded_Syst", "pPb_8TeV", "p-Pb", kFullCross, kRed+1, "boxes", 3500);
    plot[1].AddData("multDensityUnfolded", "Pbp_8TeV", "", kFullStar, kGreen+2, "", 3500);
    plot[1].AddData("multDensityUnfolded_Syst", "Pbp_8TeV", "Pb-p", kFullStar, kGreen+2, "boxes", 3500);
    plot[2].AddRatio("multDensityUnfolded_Syst", "Pbp_8TeV", "multDensityUnfolded_Syst", "pPb_8TeV", "", kFullStar, kGreen+2, "boxes", 3500);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "Pbp/pPb");
    plot[1].SetAxisRange("X", 0, 120);
    plot[2].SetAxisRange("Y", 0.9, 1.1);
    //          plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.7, 0.9, "");
    plot[1].AddText(0.4, 0.2, erg8TeV_NN);
    plot[2].SetAxisTitle("X", "#it{N}_{ch}");
    //     plot[2].SetPadOptions("logX");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtXeOverPb";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 100);
    plot[1].AddData("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 100);
    plot[1].AddData("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 100);
    plot[1].AddData("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 100);
    plot[2].AddRatio("momentUnfolded1_Syst", "XeXe_5TeV", "momentUnfolded1_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 100);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "Xe / Pb");
    plot[1].SetAxisRange("X", 0, 100);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.2, 0.9, "");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].AddText(0.4, 0.3, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtXeOverPbFullRange";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 2000);
    plot[1].AddData("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 2000);
    plot[1].AddData("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 2000);
    plot[1].AddData("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 2000);
    plot[2].AddRatio("momentUnfolded1_Syst", "XeXe_5TeV", "momentUnfolded1_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 2000);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "Xe / Pb");
    plot[1].SetAxisRange("X", 0, 2000);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.2, 0.9, "");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].AddText(0.4, 0.3, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1UnfoldedVsReWeighted";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("momentReweighted1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("momentReweighted1_Syst", "pp_5TeV", "re-weighted", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].AddData("momentUnfolded1", "pp_5TeV", "", kFullCross, kRed+1, "", 60);
    plot[1].AddData("momentUnfolded1_Syst", "pp_5TeV", "unfolded", kFullCross, kRed+1, "boxes", 60);
    plot[2].AddRatio("momentUnfolded1_Syst", "pp_5TeV", "momentReweighted1_Syst", "pp_5TeV", "", kFullCross, kRed+1, "boxes", 60);
    plot[1].SetAxisRange("X", 0., 60);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[2].SetAxisRange("Y", 0.9, 1.1);
    plot[1].AddLegend(0.2, 0.9, "");
    plot[1].AddText(0.4, 0.3, systemSizeLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
}
