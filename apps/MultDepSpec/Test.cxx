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

void MultDepSpec::DefineTestPlots(PlotManager& plotEnv)
{
  string plotGroup = "test";

  
  
  { // -----------------------------------------------------------------------
    string plotName = "test";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetPadOptions("logY");
    myPlot.AddData("momentUnfolded1", "pp_5TeV");
    myPlot.ChangePad(2);
    myPlot.AddData("momentUnfolded1", "pp_5TeV");
    myPlot.SetPadOptions("logY");
    //myPlot.SetAxisRange("Y", 0.15, 50);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  // hard probes preliminaries:
  
  string systemSizeLablePrel = alicePrel + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;

   { // -----------------------------------------------------------------------
     // compare meanPt in all systems
     string plotName = "meanPt_systems";
     Plot myPlot(plotName, "preliminary");
     myPlot.AddFrame("momentUnfolded1", PbPb_5TeV[data].input);
     
     //p-p
     myPlot.AddData("momentUnfolded1", pp_5TeV[data].input, "",
                     pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
     myPlot.AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp",
                     pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
     myPlot.AddData("meanPt", pp_5TeV[epos_lhc].input, "",
                     pp_5TeV[epos_lhc].marker, pp_5TeV[epos_lhc].color, "band", pp_5TeV[epos_lhc].mult);
     myPlot.AddData("meanPt", pp_5TeV[pythia].input, "",
                     pp_5TeV[pythia].marker, pp_5TeV[pythia].color, "band", pp_5TeV[pythia].mult);
     //p-Pb
     myPlot.AddData("momentUnfolded1", pPb_5TeV[data].input, "",
                     pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
     myPlot.AddData("momentUnfolded1_Syst", pPb_5TeV[data].input, "p-Pb",
                     pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
     myPlot.AddData("meanPt", pPb_5TeV[epos_lhc].input, "",
                     pPb_5TeV[epos_lhc].marker, pPb_5TeV[epos_lhc].color, "curve smooth", pPb_5TeV[epos_lhc].mult);
     myPlot.AddData("meanPt", pPb_5TeV[angantyr].input, "",
                     pPb_5TeV[angantyr].marker, pPb_5TeV[angantyr].color, "curve smooth dotted", pPb_5TeV[angantyr].mult);
     //Pb-Pb
     myPlot.AddData("momentUnfolded1", PbPb_5TeV[data].input, "",
                     PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
     myPlot.AddData("momentUnfolded1_Syst", PbPb_5TeV[data].input, "Pb-Pb",
                     PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
     myPlot.AddData("meanPt", PbPb_5TeV[epos].input, "",
                     PbPb_5TeV[epos].marker, PbPb_5TeV[epos].color, "", PbPb_5TeV[epos].mult);
     myPlot.AddData("meanPt", PbPb_5TeV[angantyr].input, "",
                     PbPb_5TeV[angantyr].marker, PbPb_5TeV[angantyr].color, "curve smooth dotted", PbPb_5TeV[angantyr].mult);

     myPlot.SetPadOptions("logX");
     myPlot.SetAxisRange("X", 0.1, 4000);
     //myPlot.SetAxisRange("X", 0.1, 120);

     myPlot.SetAxisRange("Y", 0.45, 0.9);
     myPlot.SetAxisTitle("X", "#it{N}_{ch}");
     
     myPlot.AddLegend(0.14, 0.92);
     myPlot.AddText(0.36, 0.3, systemSizeLablePrel);
     plotEnv.AddPlot(myPlot);
   } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    // compare meanPt in pp, data only
    string plotName = "meanPt_energies";
    Plot myPlot(plotName, "preliminary");
    myPlot.AddFrame("momentUnfolded1", pp_2TeV[data].input);
    //p-p
    myPlot.AddData("momentUnfolded1", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    myPlot.AddData("momentUnfolded1_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    myPlot.AddData("meanPt", pp_2TeV[pythia].input, "",
                    pp_2TeV[pythia].marker, pp_2TeV[pythia].color, "curve dotted", pp_2TeV[pythia].mult);
    myPlot.AddData("meanPt", pp_2TeV[epos_lhc].input, "",
                    pp_2TeV[epos_lhc].marker, pp_2TeV[epos_lhc].color, "curve", pp_2TeV[epos_lhc].mult);

    myPlot.AddData("momentUnfolded1", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    myPlot.AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    myPlot.AddData("meanPt", pp_5TeV[pythia].input, "",
                    pp_5TeV[pythia].marker, pp_5TeV[pythia].color, "curve dotted", pp_5TeV[pythia].mult);
    myPlot.AddData("meanPt", pp_5TeV[epos_lhc].input, "",
                    pp_5TeV[epos_lhc].marker, pp_5TeV[epos_lhc].color, "curve dotted", pp_5TeV[epos_lhc].mult);

    myPlot.AddData("momentUnfolded1", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    myPlot.AddData("momentUnfolded1_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    myPlot.AddData("meanPt", pp_7TeV[pythia].input, "",
                    pp_7TeV[pythia].marker, pp_7TeV[pythia].color, "curve dotted", pp_7TeV[pythia].mult);
    myPlot.AddData("meanPt", pp_7TeV[epos_lhc].input, "",
                    pp_7TeV[epos_lhc].marker, pp_7TeV[epos_lhc].color, "curve", pp_7TeV[epos_lhc].mult);

    
    myPlot.AddData("momentUnfolded1", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    myPlot.AddData("momentUnfolded1_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    myPlot.AddData("meanPt", pp_13TeV[pythia].input, "",
                    pp_13TeV[pythia].marker, pp_13TeV[pythia].color, "curve dotted", pp_13TeV[pythia].mult);
    myPlot.AddData("meanPt", pp_13TeV[epos_lhc].input, "",
                    pp_13TeV[epos_lhc].marker, pp_13TeV[epos_lhc].color, "curve dotted", pp_13TeV[epos_lhc].mult);

    myPlot.SetAxisRange("X", 0.1, 70);
    myPlot.SetAxisRange("Y", 0.45, 0.9);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    
    myPlot.AddLegend(0.14, 0.92);
    myPlot.AddText(0.36, 0.3, energyLablePrel);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_2TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddData(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddData(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_7TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddData(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_13TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddData(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pPb_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddData(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_PbPb_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddData(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "selfNormalizedHighPtYield";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    myPlot.AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddLegend();
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "selfNormalizedHighPtYieldNormalizedNch";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("normalize");
    myPlot.AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV (<mean>)");
    myPlot.AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV (<mean>)");
    myPlot.AddData(plotName + "_pp_7TeV", "Fits", "7 TeV (<mean>)");
    myPlot.AddData(plotName + "_pp_13TeV", "Fits", "13 TeV (<mean>)");
    myPlot.AddText(0.5, 0.9, "6 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}");
    myPlot.AddLegend();
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasured13TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logY");
    myPlot.AddData("multDistMeasured", "pp_13TeV_old", "event selection before", kFullSquare, kRed+1);
    myPlot.AddData("multDistMeasuredMC", "pp_13TeV_old", "", kOpenSquare, kRed+1);
    myPlot.AddData("multDistMeasured", "pp_13TeV", "AilEventCuts and PhysSel plieup rejection", kFullCircle, kBlue+1);
    myPlot.AddData("multDistMeasuredMC", "pp_13TeV", "", kOpenCircle, kBlue+1);
    myPlot.AddLegend(0.3, 0.9);
    myPlot.SetAxisRange("X", 0, 60);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCMeas";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logZ");
    myPlot.AddData("scaleEffectMultMeas", "pp_13TeV_withDDC", "");
    myPlot.SetAxisTitle("Y", "#it{N}^{MC}_{acc}");
    myPlot.SetAxisTitle("X", "#it{N}^{scaled}_{acc}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0, 60);
    myPlot.AddLegend(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCTrue";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logZ");
    myPlot.AddData("scaleEffectMultTrue", "pp_13TeV_withDDC", "");
    myPlot.SetAxisTitle("Y", "#it{N}^{MC}_{ch}");
    myPlot.SetAxisTitle("X", "#it{N}^{scaled}_{ch}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0, 60);
    myPlot.AddLegend(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDist_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetPadOptions("logY");
    myPlot.AddData("multDistUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddData("multDistUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("multDistUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistUnfolded_Syst", "pp_13TeV", "multDistUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    //    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddText(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegend(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistKNO_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetPadOptions("logY");
    myPlot.AddData("multDistUnfoldedKNO_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddData("multDistUnfoldedKNO", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("multDistUnfoldedKNO", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistUnfoldedKNO_Syst", "pp_13TeV", "multDistUnfoldedKNO", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.AddText(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegend(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensity_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetPadOptions("logY");
    myPlot.AddData("multDensityUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddData("multDensityUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("multDensityUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDensityUnfolded_Syst", "pp_13TeV", "multDensityUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.AddText(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegend(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasuredMC_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetPadOptions("logY");
    myPlot.AddData("multDistMeasuredMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("multDistMeasuredMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistMeasuredMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.AddText(0.2, 0.25, "MC measured");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegend(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistGeneratedMC_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetPadOptions("logY");
    myPlot.AddData("multDistGeneratedMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("multDistGeneratedMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistGeneratedMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.AddText(0.2, 0.25, "MC truth");
    myPlot.AddLegend(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_noDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logZ");
    myPlot.AddData("multCorrelationMatrix", "pp_13TeV");
    myPlot.AddText(0.4, 0.25, "no data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_withDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logZ");
    myPlot.AddData("multCorrelationMatrix", "pp_13TeV_withDDC");
    myPlot.AddText(0.4, 0.25, "with data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Mean";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddData("mean", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    myPlot.AddData("mean", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    myPlot.SetAxisTitle("Y", "<#it{N}_{acc}>");
    myPlot.SetAxisRange("Y", 0, 45);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddRatio("mean", "pp_13TeV", "mean", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    myPlot.AddLegend(0.2, 0.9);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Sigma";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddData("sigma", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    myPlot.AddData("sigma", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    myPlot.SetAxisTitle("Y", "#sigma(#it{N}_{acc})");
    myPlot.SetAxisRange("Y", 0, 5);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.5, 1.5);
    myPlot.AddRatio("sigma", "pp_13TeV", "sigma", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    myPlot.AddLegend(0.2, 0.9);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_noDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logZ");
    myPlot.AddData("responseMatrix", "pp_13TeV");
    myPlot.AddText(0.4, 0.25, "no data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_withDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetPadOptions("logZ");
    myPlot.AddData("responseMatrix", "pp_13TeV_withDDC");
    myPlot.AddText(0.4, 0.25, "with data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddData("momentUnfolded1", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("momentUnfolded1_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddData("momentUnfolded1", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("momentUnfolded1_Syst", "pp_13TeV", "momentUnfolded1", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.SetAxisRange("ratio", 0.99, 1.01);
    myPlot.AddLegend(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddData("varianceUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddData("varianceUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddData("varianceUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("varianceUnfolded_Syst", "pp_13TeV", "varianceUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.05, 0.9);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddLegend(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
}
