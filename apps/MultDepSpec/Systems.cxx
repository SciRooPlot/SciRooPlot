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

void MultDepSpec::DefineSystemPlots(PlotManager& plotEnv)
{
  string plotGroup = "system_comparison";
  string systemSizeLable = alice + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  string systemSizeLablePrel = alicePrel + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  
  // FIXME: put nch cent stuff in a separate graph
  vector<string> centrality = {"0-5%","5-10%","10-20%","20-30%","30-40%","40-50%","50-60%","60-70%","70-80%"};
  vector<double> nchCent = {2869.49, 2342.32, 1740.05, 1156.23, 750.512, 463.796, 265.249, 138.504, 64.0346};
  vector<double> nchCentErrors = {82.7027, 70.8452, 45.7133, 29.4205, 21.9877, 17.5017, 11.5956, 8.3419, 4.84894};
  

  string system_energy_lable = alice + newLine + chargedParticles + newLine + eta08 + ", " + ptRange;
  string pPb_lable = alice + newLine + chargedParticles + ", " + pPb + newLine + eta08 + ", " + ptRange;
  
  { // -----------------------------------------------------------------------
    // compare meanPt in pPb at 5.02 and 8.16 with all available models
    string plotName = "meanPt_pPb_models";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded1", pPb_5TeV[data].input, "5.02 TeV data",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pPb_5TeV[data].input, "5.02 TeV data",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    myPlot.AddHisto("meanPt", pPb_5TeV[angantyr].input, "5.02 TeV angantyr",
                    pPb_5TeV[angantyr].marker, pPb_5TeV[angantyr].color, "", pPb_5TeV[angantyr].mult);
    myPlot.AddHisto("meanPt", pPb_5TeV[epos_lhc].input, "5.02 TeV epos-lhc",
                    pPb_5TeV[epos_lhc].marker, pPb_5TeV[epos_lhc].color, "", pPb_5TeV[epos_lhc].mult);
    myPlot.AddGraph("meanPt", pPb_5TeV[epos].input, "5.02 TeV epos",
                    pPb_5TeV[epos].marker, pPb_5TeV[epos].color, "", pPb_5TeV[epos].mult);
    myPlot.AddHisto("momentUnfolded1", pPb_8TeV[data].input, "8.16 TeV data",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pPb_8TeV[data].input, "8.16 TeV data",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    myPlot.AddHisto("momentUnfoldedMC1", pPb_8TeV[epos_lhc].input, "8.16 TeV epos-lhc",
                    pPb_8TeV[epos_lhc].marker, pPb_8TeV[epos_lhc].color, "", pPb_8TeV[epos_lhc].mult);
    
    myPlot.SetAxisRange("X", 0, 150);
    myPlot.SetAxisRange("Y", 0.45, 0.98);
    myPlot.AddLegendBox(0.15, 0.92);
    myPlot.AddTextBox(0.3, 0.3, pPb_lable);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    plotEnv.AddPlot(myPlot);
   } // -----------------------------------------------------------------------

  
  { // -----------------------------------------------------------------------
    // compare meanPt in PbPb at 5.02 and XeXe at 5.44 with all available models
    string plotName = "meanPt_PbPb_XeXe_models";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded1", PbPb_5TeV[data].input, "PbPb 5.02 TeV data",
                    PbPb_5TeV[data].marker,PbPb_5TeV[data].color);
    myPlot.AddHisto("momentUnfolded1_Syst", PbPb_5TeV[data].input, "PbPb 5.02 TeV data",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes");
    myPlot.AddHisto("momentGeneratedMC1", PbPb_5TeV[hijing].input, "PbPb 5.02 TeV hijing",
                    PbPb_5TeV[hijing].marker, PbPb_5TeV[hijing].color);
    myPlot.AddHisto("meanPt", PbPb_5TeV[epos_lhc].input, "PbPb 5.02 TeV epos-lhc",
                    PbPb_5TeV[epos_lhc].marker, PbPb_5TeV[epos_lhc].color);
    myPlot.AddGraph("meanPt", PbPb_5TeV[epos].input, "PbPb 5.02 TeV epos",
                    PbPb_5TeV[epos].marker, PbPb_5TeV[epos].color);
    myPlot.AddHisto("momentUnfolded1", XeXe_5TeV[data].input, "XeXe 5.44 TeV data",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color);
    myPlot.AddHisto("momentUnfolded1_Syst", XeXe_5TeV[data].input, "XeXe 5.44 TeV data",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "boxes");
    myPlot.AddHisto("momentGeneratedMC1", XeXe_5TeV[hijing].input, "XeXe 5.44 TeV hijing",
                    XeXe_5TeV[hijing].marker, XeXe_5TeV[hijing].color);
    
    myPlot.SetDrawingProperties("logX");
    myPlot.AddLegendBox(0.15, 0.92);
    myPlot.SetAxisRange("X", 1, 3000);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisRange("Y", 0.45, 0.98);
    plotEnv.AddPlot(myPlot);
   } // -----------------------------------------------------------------------

  
  { // -----------------------------------------------------------------------
    // compare meanPt in all systems, data only
    string plotName = "meanPt_all";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddFrame("momentUnfolded1", PbPb_5TeV[data].input);
    
    //p-p
    myPlot.AddHisto("momentUnfolded1", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    //p-Pb
    myPlot.AddHisto("momentUnfolded1", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pPb_5TeV[data].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1", pPb_8TeV[data].input, "",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", pPb_8TeV[data].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    //Pb-Pb
    myPlot.AddHisto("momentUnfolded1", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", PbPb_5TeV[data].input, "Pb-Pb, 5.02 TeV",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    //Xe-Xe
    myPlot.AddHisto("momentUnfolded1", XeXe_5TeV[data].input, "",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    myPlot.AddHisto("momentUnfolded1_Syst", XeXe_5TeV[data].input, "Xe-Xe, 5.44 TeV",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);

    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisRange("X", 0.1, 4000);
    myPlot.SetAxisRange("Y", 0.45, 0.9);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    
    myPlot.AddLegendBox(0.14, 0.92);
    myPlot.AddTextBox(0.36, 0.3, system_energy_lable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    // compare variance in all systems, data only
    string plotName = "variance_all";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddFrame("varianceUnfolded", PbPb_5TeV[data].input);
    
    //p-p
    myPlot.AddHisto("varianceUnfolded", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    //p-Pb
    myPlot.AddHisto("varianceUnfolded", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", pPb_5TeV[data].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded", pPb_8TeV[data].input, "",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", pPb_8TeV[data].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    //Pb-Pb
    myPlot.AddHisto("varianceUnfolded", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", PbPb_5TeV[data].input, "Pb-Pb, 5.02 TeV",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    //Xe-Xe
    myPlot.AddHisto("varianceUnfolded", XeXe_5TeV[data].input, "",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    myPlot.AddHisto("varianceUnfolded_Syst", XeXe_5TeV[data].input, "Xe-Xe, 5.44 TeV",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    
    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisRange("X", 0.1, 4000);
    myPlot.SetAxisRange("Y", 0.05, 0.95);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    
    myPlot.AddLegendBox(0.14, 0.92);
    myPlot.AddTextBox(0.36, 0.3, system_energy_lable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multiplicity_all";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddFrame("multDensityUnfolded", PbPb_5TeV[data].input);

    myPlot.AddHisto("multDensityUnfolded", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", pp_7TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    //p-Pb
    myPlot.AddHisto("multDensityUnfolded", pPb_5TeV[data].input, "",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", pPb_5TeV[data].input, "p-Pb, 5.02 TeV",
                    pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded", pPb_8TeV[data].input, "",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "", pPb_8TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", pPb_8TeV[data].input, "p-Pb, 8.16 TeV",
                    pPb_8TeV[data].marker, pPb_8TeV[data].color, "boxes", pPb_8TeV[data].mult);
    //Pb-Pb
    myPlot.AddHisto("multDensityUnfolded", PbPb_5TeV[data].input, "",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", PbPb_5TeV[data].input, "Pb-Pb, 5.02 TeV",
                    PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
    //Xe-Xe
    myPlot.AddHisto("multDensityUnfolded", XeXe_5TeV[data].input, "",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    myPlot.AddHisto("multDensityUnfolded_Syst", XeXe_5TeV[data].input, "Xe-Xe, 5.44 TeV",
                    XeXe_5TeV[data].marker, XeXe_5TeV[data].color, "", XeXe_5TeV[data].mult);
    
    myPlot.SetDrawingProperties("logY logX");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddTextBox(0.12, 0.3, systemSizeLable);
    myPlot.AddLegendBox(0.6, 0.92);
    myPlot.SetAxisRange("Y", 1e-5, 1);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "meanPtGeometricalScaling";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddGraph("meanPtScaled_pp_2TeV", "GeometricalScaling", "", 0, 0, "", 2.14);
    myPlot.AddGraph("meanPtScaled_pp_2TeV_Syst", "GeometricalScaling", "pp, 2.76 TeV", -1, -1, "boxes", 2.14);
    myPlot.AddGraph("meanPtScaled_pp_5TeV", "GeometricalScaling", "", 0, 0, "", 2.8);
    myPlot.AddGraph("meanPtScaled_pp_5TeV_Syst", "GeometricalScaling", "pp, 5.02 TeV", -1, -1, "boxes", 2.8);
    myPlot.AddGraph("meanPtScaled_pp_7TeV", "GeometricalScaling", "", 0, 0, "", 2.8);
    myPlot.AddGraph("meanPtScaled_pp_7TeV_Syst", "GeometricalScaling", "pp, 7 TeV", -1, -1, "boxes", 2.8);
    myPlot.AddGraph("meanPtScaled_pp_13TeV", "GeometricalScaling", "", 0, 0, "", 3.3);
    myPlot.AddGraph("meanPtScaled_pp_13TeV_Syst", "GeometricalScaling", "pp, 13 TeV", -1, -1, "boxes", 3.3);
    myPlot.AddGraph("meanPtScaled_pPb_5TeV", "GeometricalScaling", "", 0, 0, "", 2.5);
    myPlot.AddGraph("meanPtScaled_pPb_5TeV_Syst", "GeometricalScaling", "p-Pb, 5.02 TeV", -1, -1, "boxes", 2.5);
    myPlot.AddGraph("meanPtScaled_pPb_8TeV", "GeometricalScaling", "", 0, 0, "", 2.6);
    myPlot.AddGraph("meanPtScaled_pPb_8TeV_Syst", "GeometricalScaling", "p-Pb, 8 TeV", -1, -1, "boxes", 2.6);
    myPlot.AddLegendBox(0.15, 0.9);
    myPlot.SetAxisRange("X", 0.7, 3.7);
    myPlot.SetAxisRange("Y", 0.48, 1.0);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_pPbOverpp";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddRatio("reducedMultPtpPb", "Fits", "multPtUnfoldedNormalized", "pp_5TeV"); // This does not work!!
    myPlot.SetAxisRange("Z", 0.0, 2.0);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "p-Pb / pp");
    myPlot.AddTextBox(0.2, 0.9, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_PbPbOverpp";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddRatio("reducedMultPtPbPb", "Fits", "multPtUnfoldedNormalized", "pp_5TeV");
    myPlot.SetAxisRange("Z", 0.0, 2.0);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "Pb-Pb / pp");
    myPlot.AddTextBox(0.2, 0.9, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPbPbXeXe";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("michaelRatio_XeXe", "Fits", "Xe-Xe", 0, kBlue+2);
    myPlot.AddHisto("michaelRatio_PbPb", "Fits", "Pb-Pb", 0, kBlack);
    myPlot.AddRatio("michaelRatio_XeXe", "Fits", "michaelRatio_PbPb", "Fits", "", 0, kBlue+2);
    myPlot.AddLegendBox(0.6, 0.9);
    myPlot.AddTextBox(0.3, 0.5, "1200 < #it{N}_{ch} < 2300");
    myPlot.SetAxisRange("ratio", 0.6, 1.1);
    myPlot.AddTextBox(0.18, 0.2, systemSizeLable);
    myPlot.SetAxisTitle("ratio", "extreme / nearby");
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPbPb";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("spectraExtreme_PbPb", "Fits", "#it{N}_{ch} > 2000", 0, kBlue+2);
    myPlot.AddHisto("spectraNearby_PbPb", "Fits", "700 < #it{N}_{ch} < 1200", 0, kBlack);
    myPlot.AddRatio("spectraExtreme_PbPb", "Fits", "spectraNearby_PbPb", "Fits", "", 0, kBlue+2);
    myPlot.AddLegendBox(0.6, 0.9);
    myPlot.AddTextBox(0.4, 0.5, "#bf{Pb-Pb}");
    myPlot.SetAxisRange("ratio", 1.3, 3.3);
    myPlot.AddTextBox(0.18, 0.2, systemSizeLable);
    myPlot.SetAxisTitle("ratio", "extreme / nearby");
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultXeXe";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("spectraExtreme_XeXe", "Fits", "#it{N}_{ch} > 1200", 0, kBlue+2);
    myPlot.AddHisto("spectraNearby_XeXe", "Fits", "500 < #it{N}_{ch} < 800", 0, kBlack);
    myPlot.AddRatio("spectraExtreme_XeXe", "Fits", "spectraNearby_XeXe", "Fits", "", 0, kBlue+2);
    myPlot.AddLegendBox(0.6, 0.9);
    myPlot.AddTextBox(0.4, 0.5, "#bf{Xe-Xe}");
    myPlot.AddTextBox(0.18, 0.2, systemSizeLable);
    myPlot.SetAxisTitle("ratio", "extreme / nearby");
    myPlot.SetAxisRange("ratio", 1.3, 3.3);
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPP";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("inclusiveSpectrum", "pp_5TeV", "", 0, kBlack);
    myPlot.AddHisto("spectraExtreme_pp", "Fits", "#it{N}_{ch} > 40", 0, kBlue+2);
    myPlot.AddHisto("inclusiveSpectrum", "pp_5TeV", "all #it{N}_{ch}", 0, kBlack);
    myPlot.AddRatio("spectraExtreme_pp", "Fits", "inclusiveSpectrum", "pp_5TeV", "", 0, kBlue+2);
    myPlot.AddLegendBox(0.6, 0.9);
    myPlot.AddTextBox(0.4, 0.5, "#bf{pp}");
    myPlot.AddTextBox(0.18, 0.2, systemSizeLable);
    myPlot.SetAxisTitle("ratio", "extreme/MB");
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyDepHighNchSpectra";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("spectraHighNch_pp_13TeV", "Fits", "", 0, 0);
    myPlot.AddHisto("spectraHighNch_pp_2TeV", "Fits", "", 0, 0);
    myPlot.AddHisto("spectraHighNch_pp_5TeV", "Fits", "", 0, 0);
    myPlot.AddHisto("spectraHighNch_pp_7TeV", "Fits", "", 0, 0);
    //myPlot.AddRatio("spectraHighNch_pp_2TeV", "Fits", "spectraHighNch_pp_13TeV", "Fits", "", 0);
    myPlot.AddRatio("spectraHighNch_pp_5TeV", "Fits", "spectraHighNch_pp_13TeV", "Fits", "", 0);
    myPlot.AddRatio("spectraHighNch_pp_7TeV", "Fits", "spectraHighNch_pp_13TeV", "Fits", "", 0);
    myPlot.AddLegendBox(0.6, 0.9);
    myPlot.AddTextBox(0.4, 0.5, "#bf{pp}");
    myPlot.AddTextBox(0.18, 0.2, systemSizeLable);
    myPlot.SetAxisTitle("ratio", "ratio to 13");
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisTitle("X", "#it{p}_{T}");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyExtremeMultPPb";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("inclusiveSpectrum", "pPb_5TeV", "", 0, kBlack);
    myPlot.AddHisto("spectraExtreme_pp", "Fits", "#it{N}_{ch} > 100", 0, kBlue+2);
    myPlot.AddHisto("inclusiveSpectrum", "pPb_5TeV", "all #it{N}_{ch}", 0, kBlack);
    myPlot.AddRatio("spectraExtreme_pp", "Fits", "inclusiveSpectrum", "pPb_5TeV", "", 0, kBlue+2);
    myPlot.AddLegendBox(0.6, 0.9);
    myPlot.AddTextBox(0.4, 0.5, "#bf{p-Pb}");
    myPlot.AddTextBox(0.18, 0.2, systemSizeLable);
    myPlot.SetAxisTitle("ratio", "extreme/MB");
    //      myPlot.SetAxisRange("ratio", 1.3, 3.3);
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "studyLowMult";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto("spectraNch2_PbPb", "Fits", "Pb-Pb, #it{N}_{ch} = 2", kFullSquare, kGreen+2);
    myPlot.AddHisto("spectraNch2_XeXe", "Fits", "Xe-Xe, #it{N}_{ch} = 2", kFullCircle, kGreen+2);
    myPlot.AddHisto("spectraNch1_PbPb", "Fits", "Pb-Pb, #it{N}_{ch} = 1", kFullSquare, kBlue+2);
    myPlot.AddHisto("spectraNch1_XeXe", "Fits", "Xe-Xe, #it{N}_{ch} = 1", kFullCircle, kBlue+2);
    myPlot.AddRatio("spectraNch1_PbPb", "Fits", "spectraNch1_XeXe", "Fits", "", kFullSquare, kBlue+2);
    myPlot.AddRatio("spectraNch2_PbPb", "Fits", "spectraNch2_XeXe", "Fits", "", kFullSquare, kGreen+2);
    myPlot.SetAxisRange("X", 0.15, 10);
    myPlot.SetAxisRange("ratio", 0.5, 1.5);
    myPlot.AddLegendBox(0.2, 0.4);
    //myPlot.AddTextBox(0.4, 0.3, "#bf{Xe-Xe}");
    myPlot.SetAxisTitle("ratio", "Pb / Xe");
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityComparisonPerNuclPair";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logX");
    myPlot.AddGraph("multDensityVsParticlesPerNucleon_XeXe_5TeV", "Fits", "Xe-Xe", kFullCircle, kGreen+2, "");
    myPlot.AddGraph("multDensityVsParticlesPerNucleon_PbPb_5TeV", "Fits", "Pb-Pb", kFullSquare, kRed+1, "");
    myPlot.AddLegendBox(0.7, 0.8);
    myPlot.AddTextBox(0.2, 0.3, systemSizeLable);
    myPlot.SetAxisRange("Y", 1e-4, 3e-2);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtPerNuclPair";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddGraph("meanPtVsParticlesPerNucleon_XeXe_5TeV", "Fits", "Xe-Xe", kFullCircle, kGreen+2, "");
    myPlot.AddGraph("meanPtVsParticlesPerNucleon_PbPb_5TeV", "Fits", "Pb-Pb", kFullSquare, kRed+1, "");
    //myPlot.SetAxisRange("Y", 1e-4, 3e-2);
    //      myPlot.SetAxisRange("Y", 0.48, 0.75);
    myPlot.AddLegendBox(0.7, 0.4);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtPerNuclPairRatio";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddGraph("meanPtVsParticlesPerNucleon_XeXe_5TeV", "Fits", "Xe-Xe", kFullCircle, kGreen+2, "");
    myPlot.AddGraph("meanPtVsParticlesPerNucleon_PbPb_5TeV", "Fits", "Pb-Pb", kFullSquare, kRed+1, "");
    myPlot.SetAxisRange("Y", 0.75, 1.05);
    myPlot.SetAxisRange("X", 0, 10);
    myPlot.AddLegendBox(0.7, 0.4);
    for(int i = 0; i < 9; i++)
    {
      //myPlot.AddTextBox(nchCent[i]/(2*208), 1, "#bf{|}", true);
    }
    myPlot.ChangePad(2);
    for(int i = 0; i < 9; i++)
    {
      //myPlot.AddTextBox(nchCent[i]/(2*208), 1, "#bf{|}", true);
    }
    myPlot.AddGraph("ratioMeanPtVsParticlesPerNucleon", "Fits", "Xe / Pb", kFullCircle, kGreen+2, "");
    myPlot.SetAxisRange("Y", 0.99, 1.01);
    myPlot.SetAxisTitle("Y", "Xe / Pb");
    myPlot.SetAxisRange("X", 0, 10);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1dNchdeta";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded1dNchDeta", "pp_5TeV", "pp", 0, kBlue+1);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "varianceFullRangeMC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logX");
    myPlot.AddHisto("varianceGeneratedMC", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("varianceGeneratedMC", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 50);
    myPlot.AddHisto("varianceGeneratedMC", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 85);
    myPlot.AddHisto("varianceGeneratedMC", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 3000);
    myPlot.SetAxisRange("X", 0.1, 4000);
    myPlot.SetAxisRange("Y", 0.05, 0.8);
    myPlot.AddLegendBox(0.15, 0.6);
    myPlot.AddTextBox(0.14, 0.92, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtpPb8Over5";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCross, kRed+1, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb, 5.02 TeV", kFullCross, kRed+1, "boxes");
    myPlot.AddHisto("momentUnfolded1", "pPb_8TeV", "", kFullStar, kBlue+1, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "pPb_8TeV", "p-Pb, 8.16 TeV", kFullStar, kBlue+1, "boxes");
    myPlot.AddRatio("momentUnfolded1_Syst", "pPb_8TeV", "momentUnfolded1_Syst", "pPb_5TeV", "", kFullStar, kBlue+1, "boxes");
    
    myPlot.AddHisto("meanPt_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCross, kRed+1, "", 130);
    myPlot.AddHisto("momentGeneratedMC1", "pPb_8TeV", "", kOpenStar, kBlue+1, "boxes");
    myPlot.AddRatio("momentGeneratedMC1", "pPb_8TeV", "meanPt_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenStar, kBlue+1, "boxes");
    
    myPlot.SetAxisRange("ratio", 0.9, 1.1);
    myPlot.SetAxisTitle("ratio", "8 / 5 TeV");
    myPlot.SetAxisRange("X", 0, 100);
    myPlot.SetAxisRange("Y", 0.45, 0.95);
    myPlot.AddLegendBox(0.2, 0.8, "");
    
    myPlot.AddTextBox(0.4, 0.3, alice + newLine + chargedParticles + newLine + eta08 + ", " + ptRange + newLine + "Open symbols: EPOS-LHC");
    myPlot.ChangePad(2);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    
    
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtpPbOverPbp";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("momentUnfolded1", "pPb_8TeV", "", kFullStar, kBlue+1, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "pPb_8TeV", "p-Pb, 8 TeV", kFullStar, kBlue+1, "boxes");
    myPlot.AddHisto("momentUnfolded1", "Pbp_8TeV", "", kFullStar, kGreen+1, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "Pbp_8TeV", "Pb-p, 8 TeV", kFullStar, kGreen+1, "boxes");
    myPlot.AddRatio("momentUnfolded1_Syst", "Pbp_8TeV", "momentUnfolded1_Syst", "pPb_8TeV", "", kFullStar, kGreen+1, "boxes");
    myPlot.SetAxisRange("ratio", 0.98, 1.02);
    myPlot.SetAxisTitle("ratio", "Pbp/pPb");
    myPlot.SetAxisRange("X", 0, 150);
    myPlot.SetAxisRange("Y", 0.45, 0.95);
    myPlot.AddLegendBox(0.4, 0.4, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtPbOverPb";
    Plot myPlot(plotName, plotGroup, "default ratio");
    //      myPlot.SetDrawingProperties("logX");
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3500);
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "2015", kFullCross, kRed+1, "boxes", 3500);
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV_2018", "", kFullStar, kGreen+2, "", 3500);
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV_2018", "2018", kFullStar, kGreen+2, "boxes", 3500);
    myPlot.AddRatio("momentUnfolded1_Syst", "PbPb_5TeV_2018", "momentUnfolded1_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 3500);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "2018/2015");
    myPlot.SetAxisRange("X", 0, 3400);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.2, systemSizeLable);
    myPlot.ChangePad(2);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    //     myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityPbOverPb";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3500);
    myPlot.AddHisto("multDensityUnfolded_Syst", "PbPb_5TeV", "2015", kFullCross, kRed+1, "boxes", 3500);
    myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV_2018", "", kFullStar, kGreen+2, "", 3500);
    myPlot.AddHisto("multDensityUnfolded_Syst", "PbPb_5TeV_2018", "2018", kFullStar, kGreen+2, "boxes", 3500);
    myPlot.AddRatio("multDensityUnfolded_Syst", "PbPb_5TeV_2018", "multDensityUnfolded_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 3500);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "2018/2015");
    myPlot.SetAxisRange("X", 0, 3400);
    myPlot.SetAxisRange("ratio", 0.9, 1.1);
    //          myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.2, systemSizeLable);
    myPlot.ChangePad(2);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    //     myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityPbpOverpPb";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDensityUnfolded", "pPb_8TeV", "", kFullCross, kRed+1, "", 3500);
    myPlot.AddHisto("multDensityUnfolded_Syst", "pPb_8TeV", "p-Pb", kFullCross, kRed+1, "boxes", 3500);
    myPlot.AddHisto("multDensityUnfolded", "Pbp_8TeV", "", kFullStar, kGreen+2, "", 3500);
    myPlot.AddHisto("multDensityUnfolded_Syst", "Pbp_8TeV", "Pb-p", kFullStar, kGreen+2, "boxes", 3500);
    myPlot.AddRatio("multDensityUnfolded_Syst", "Pbp_8TeV", "multDensityUnfolded_Syst", "pPb_8TeV", "", kFullStar, kGreen+2, "boxes", 3500);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "Pbp/pPb");
    myPlot.SetAxisRange("X", 0, 120);
    myPlot.SetAxisRange("ratio", 0.9, 1.1);
    //          myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.7, 0.9, "");
    myPlot.AddTextBox(0.4, 0.2, erg8TeV_NN);
    myPlot.ChangePad(2);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    //     myPlot.SetDrawingProperties("logX");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtXeOverPb";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 100);
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 100);
    myPlot.AddHisto("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 100);
    myPlot.AddHisto("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 100);
    myPlot.AddRatio("momentUnfolded1_Syst", "XeXe_5TeV", "momentUnfolded1_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 100);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "Xe / Pb");
    myPlot.SetAxisRange("X", 0, 100);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtXeOverPbFullRange";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 2000);
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 2000);
    myPlot.AddHisto("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 2000);
    myPlot.AddHisto("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 2000);
    myPlot.AddRatio("momentUnfolded1_Syst", "XeXe_5TeV", "momentUnfolded1_Syst", "PbPb_5TeV", "", kFullStar, kGreen+2, "boxes", 2000);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "Xe / Pb");
    myPlot.SetAxisRange("X", 0, 2000);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1UnfoldedVsReWeighted";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("momentReweighted1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentReweighted1_Syst", "pp_5TeV", "re-weighted", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullCross, kRed+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "unfolded", kFullCross, kRed+1, "boxes", 60);
    myPlot.AddRatio("momentUnfolded1_Syst", "pp_5TeV", "momentReweighted1_Syst", "pp_5TeV", "", kFullCross, kRed+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0., 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.SetAxisRange("ratio", 0.9, 1.1);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
}
