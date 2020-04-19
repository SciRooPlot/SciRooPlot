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

void MultDepSpec::DefineTestPlots(PlotManager& plotManager)
{
  string plotGroup = "test";

  
  { // -----------------------------------------------------------------------
    string plotName = "test2";
    Plot plot(plotName, plotGroup, "default");
    //plot[1].SetPadOptions("logY");
    plot[1].AddData("func", "InputTest").SetViewRangeXHigh(35).SetColor(kRed+1);
    plot[1].AddData("momentUnfolded1", "pp_5TeV").SetViewRangeXHigh(50).SetMarker(kOpenSquare);
    plot[1].AddData("meanPt", "Models:epos");
    plot[1].AddData("momentUnfolded1", "pp_13TeV");
    //plot[2].AddRatio("momentUnfolded1", "pp_13TeV", "momentUnfolded1", "pp_5TeV");
    //plot[2].SetAxisRange("Y", 0.9, 1.1);
    plot[1].SetAxisTitle("X", "Mult");
    plot[1].SetAxisTitle("Y", "Something");
    plot[1].SetAxisRange("X", 10, 70);

    //plot[2].AddData("momentUnfolded1", "pp_5TeV").Divide("momentUnfolded1", "pp_5TeV").SetDivideMethod("binomial");
    //plot[1].AddData("momentUnfolded1", "pp_5TeV") / ("momentUnfolded1", "pp_5TeV");

    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "test";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData("multPtUnfolded", "pp_13TeV").SetViewRangeXHigh(60);
    plot[1].AddData("multPtUnfolded", "pp_5TeV").SetViewRangeXHigh(50).SetViewRangeXLow(40);
    plot[1].SetAxisTitle("X", "My X axis title");
    plot[1].SetAxisTitle("Y", "My Y axis title");
    plot[1].SetAxisTitle("Z", "My Z axis title");
    plot[1].SetAxisRange("X", 10, 70);
    plot[1].SetAxisRange("Y", .15, 5);
    plot[1].SetAxisRange("Z", 1e-4, 1e-2);

    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  // hard probes preliminaries:
  
  string systemSizeLablePrel = alicePrel + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;

   { // -----------------------------------------------------------------------
     // compare meanPt in all systems
     string plotName = "meanPt_systems";
     Plot plot(plotName, "preliminary");
     plot[1].AddFrame("momentUnfolded1", PbPb_5TeV[data].input);
     
     //p-p
     plot[1].AddData("momentUnfolded1", pp_5TeV[data].input, "",
                     pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
     plot[1].AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp",
                     pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
     plot[1].AddData("meanPt", pp_5TeV[epos_lhc].input, "",
                     pp_5TeV[epos_lhc].marker, pp_5TeV[epos_lhc].color, "band", pp_5TeV[epos_lhc].mult);
     plot[1].AddData("meanPt", pp_5TeV[pythia].input, "",
                     pp_5TeV[pythia].marker, pp_5TeV[pythia].color, "band", pp_5TeV[pythia].mult);
     //p-Pb
     plot[1].AddData("momentUnfolded1", pPb_5TeV[data].input, "",
                     pPb_5TeV[data].marker, pPb_5TeV[data].color, "", pPb_5TeV[data].mult);
     plot[1].AddData("momentUnfolded1_Syst", pPb_5TeV[data].input, "p-Pb",
                     pPb_5TeV[data].marker, pPb_5TeV[data].color, "boxes", pPb_5TeV[data].mult);
     plot[1].AddData("meanPt", pPb_5TeV[epos_lhc].input, "",
                     pPb_5TeV[epos_lhc].marker, pPb_5TeV[epos_lhc].color, "curve smooth", pPb_5TeV[epos_lhc].mult);
     plot[1].AddData("meanPt", pPb_5TeV[angantyr].input, "",
                     pPb_5TeV[angantyr].marker, pPb_5TeV[angantyr].color, "curve smooth dotted", pPb_5TeV[angantyr].mult);
     //Pb-Pb
     plot[1].AddData("momentUnfolded1", PbPb_5TeV[data].input, "",
                     PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "", PbPb_5TeV[data].mult);
     plot[1].AddData("momentUnfolded1_Syst", PbPb_5TeV[data].input, "Pb-Pb",
                     PbPb_5TeV[data].marker, PbPb_5TeV[data].color, "boxes", PbPb_5TeV[data].mult);
     plot[1].AddData("meanPt", PbPb_5TeV[epos].input, "",
                     PbPb_5TeV[epos].marker, PbPb_5TeV[epos].color, "", PbPb_5TeV[epos].mult);
     plot[1].AddData("meanPt", PbPb_5TeV[angantyr].input, "",
                     PbPb_5TeV[angantyr].marker, PbPb_5TeV[angantyr].color, "curve smooth dotted", PbPb_5TeV[angantyr].mult);

     plot[1].SetPadOptions("logX");
     plot[1].SetAxisRange("X", 0.1, 4000);
     //plot[1].SetAxisRange("X", 0.1, 120);

     plot[1].SetAxisRange("Y", 0.45, 0.9);
     plot[1].SetAxisTitle("X", "#it{N}_{ch}");
     
     plot[1].AddLegend(0.14, 0.92);
     plot[1].AddText(0.36, 0.3, systemSizeLablePrel);
     plotManager.AddPlot(plot);
   } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    // compare meanPt in pp, data only
    string plotName = "meanPt_energies";
    Plot plot(plotName, "preliminary");
    plot[1].AddFrame("momentUnfolded1", pp_2TeV[data].input);
    //p-p
    plot[1].AddData("momentUnfolded1", pp_2TeV[data].input, "",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "", pp_2TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_2TeV[data].input, "pp, 2.76 TeV",
                    pp_2TeV[data].marker, pp_2TeV[data].color, "boxes", pp_2TeV[data].mult);
    plot[1].AddData("meanPt", pp_2TeV[pythia].input, "",
                    pp_2TeV[pythia].marker, pp_2TeV[pythia].color, "curve dotted", pp_2TeV[pythia].mult);
    plot[1].AddData("meanPt", pp_2TeV[epos_lhc].input, "",
                    pp_2TeV[epos_lhc].marker, pp_2TeV[epos_lhc].color, "curve", pp_2TeV[epos_lhc].mult);

    plot[1].AddData("momentUnfolded1", pp_5TeV[data].input, "",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "", pp_5TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_5TeV[data].input, "pp, 5.02 TeV",
                    pp_5TeV[data].marker, pp_5TeV[data].color, "boxes", pp_5TeV[data].mult);
    plot[1].AddData("meanPt", pp_5TeV[pythia].input, "",
                    pp_5TeV[pythia].marker, pp_5TeV[pythia].color, "curve dotted", pp_5TeV[pythia].mult);
    plot[1].AddData("meanPt", pp_5TeV[epos_lhc].input, "",
                    pp_5TeV[epos_lhc].marker, pp_5TeV[epos_lhc].color, "curve dotted", pp_5TeV[epos_lhc].mult);

    plot[1].AddData("momentUnfolded1", pp_7TeV[data].input, "",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "", pp_7TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_7TeV[data].input, "pp, 7 TeV",
                    pp_7TeV[data].marker, pp_7TeV[data].color, "boxes", pp_7TeV[data].mult);
    plot[1].AddData("meanPt", pp_7TeV[pythia].input, "",
                    pp_7TeV[pythia].marker, pp_7TeV[pythia].color, "curve dotted", pp_7TeV[pythia].mult);
    plot[1].AddData("meanPt", pp_7TeV[epos_lhc].input, "",
                    pp_7TeV[epos_lhc].marker, pp_7TeV[epos_lhc].color, "curve", pp_7TeV[epos_lhc].mult);

    
    plot[1].AddData("momentUnfolded1", pp_13TeV[data].input, "",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "", pp_13TeV[data].mult);
    plot[1].AddData("momentUnfolded1_Syst", pp_13TeV[data].input, "pp, 13 TeV",
                    pp_13TeV[data].marker, pp_13TeV[data].color, "boxes", pp_13TeV[data].mult);
    plot[1].AddData("meanPt", pp_13TeV[pythia].input, "",
                    pp_13TeV[pythia].marker, pp_13TeV[pythia].color, "curve dotted", pp_13TeV[pythia].mult);
    plot[1].AddData("meanPt", pp_13TeV[epos_lhc].input, "",
                    pp_13TeV[epos_lhc].marker, pp_13TeV[epos_lhc].color, "curve dotted", pp_13TeV[epos_lhc].mult);

    plot[1].SetAxisRange("X", 0.1, 70);
    plot[1].SetAxisRange("Y", 0.45, 0.9);
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    
    plot[1].AddLegend(0.14, 0.92);
    plot[1].AddText(0.36, 0.3, energyLablePrel);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_2TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].AddData(plotName, "Fits");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_5TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].AddData(plotName, "Fits");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_7TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].AddData(plotName, "Fits");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_13TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].AddData(plotName, "Fits");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pPb_5TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].AddData(plotName, "Fits");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_PbPb_5TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].AddData(plotName, "Fits");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "selfNormalizedHighPtYield";
    Plot plot(plotName, plotGroup);
    plot[1].AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    plot[1].AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    plot[1].AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    plot[1].AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "selfNormalizedHighPtYieldNormalizedNch";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("normalize");
    plot[1].AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV (<mean>)");
    plot[1].AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV (<mean>)");
    plot[1].AddData(plotName + "_pp_7TeV", "Fits", "7 TeV (<mean>)");
    plot[1].AddData(plotName + "_pp_13TeV", "Fits", "13 TeV (<mean>)");
    plot[1].AddText(0.5, 0.9, "6 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasured13TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistMeasured", "pp_13TeV_old", "event selection before", kFullSquare, kRed+1);
    plot[1].AddData("multDistMeasuredMC", "pp_13TeV_old", "", kOpenSquare, kRed+1);
    plot[1].AddData("multDistMeasured", "pp_13TeV", "AilEventCuts and PhysSel plieup rejection", kFullCircle, kBlue+1);
    plot[1].AddData("multDistMeasuredMC", "pp_13TeV", "", kOpenCircle, kBlue+1);
    plot[1].AddLegend(0.3, 0.9);
    plot[1].SetAxisRange("X", 0, 60);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCMeas";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData("scaleEffectMultMeas", "pp_13TeV_withDDC", "");
    plot[1].SetAxisTitle("Y", "#it{N}^{MC}_{acc}");
    plot[1].SetAxisTitle("X", "#it{N}^{scaled}_{acc}");
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0, 60);
    plot[1].AddLegend(0.2, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCTrue";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData("scaleEffectMultTrue", "pp_13TeV_withDDC", "");
    plot[1].SetAxisTitle("Y", "#it{N}^{MC}_{ch}");
    plot[1].SetAxisTitle("X", "#it{N}^{scaled}_{ch}");
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0, 60);
    plot[1].AddLegend(0.2, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDist_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].AddData("multDistUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("multDistUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("multDistUnfolded_Syst", "pp_13TeV", "multDistUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].SetAxisRange("X", 0, 60);
    //    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddText(0.2, 0.25, "data unfolded");
    plot[2].SetAxisRange("Y", 0.8, 1.2);
    plot[1].AddLegend(0.4, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistKNO_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistUnfoldedKNO_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].AddData("multDistUnfoldedKNO", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("multDistUnfoldedKNO", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("multDistUnfoldedKNO_Syst", "pp_13TeV", "multDistUnfoldedKNO", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].AddText(0.2, 0.25, "data unfolded");
    plot[2].SetAxisRange("Y", 0.8, 1.2);
    plot[1].AddLegend(0.4, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensity_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDensityUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].AddData("multDensityUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("multDensityUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("multDensityUnfolded_Syst", "pp_13TeV", "multDensityUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].AddText(0.2, 0.25, "data unfolded");
    plot[2].SetAxisRange("Y", 0.8, 1.2);
    plot[1].AddLegend(0.4, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasuredMC_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistMeasuredMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("multDistMeasuredMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("multDistMeasuredMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].AddText(0.2, 0.25, "MC measured");
    plot[2].SetAxisRange("Y", 0.8, 1.2);
    plot[1].AddLegend(0.4, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistGeneratedMC_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistGeneratedMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("multDistGeneratedMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("multDistGeneratedMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].AddText(0.2, 0.25, "MC truth");
    plot[1].AddLegend(0.4, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_noDDC";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData("multCorrelationMatrix", "pp_13TeV");
    plot[1].AddText(0.4, 0.25, "no data-driven corrections");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_withDDC";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData("multCorrelationMatrix", "pp_13TeV_withDDC");
    plot[1].AddText(0.4, 0.25, "with data-driven corrections");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Mean";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("mean", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    plot[1].AddData("mean", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    plot[1].SetAxisTitle("Y", "<#it{N}_{acc}>");
    plot[1].SetAxisRange("Y", 0, 45);
    plot[1].SetAxisRange("X", 0, 60);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].AddRatio("mean", "pp_13TeV", "mean", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    plot[1].AddLegend(0.2, 0.9);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Sigma";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("sigma", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    plot[1].AddData("sigma", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    plot[1].SetAxisTitle("Y", "#sigma(#it{N}_{acc})");
    plot[1].SetAxisRange("Y", 0, 5);
    plot[1].SetAxisRange("X", 0, 60);
    plot[2].SetAxisRange("Y", 0.5, 1.5);
    plot[2].AddRatio("sigma", "pp_13TeV", "sigma", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    plot[1].AddLegend(0.2, 0.9);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_noDDC";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData("responseMatrix", "pp_13TeV");
    plot[1].AddText(0.4, 0.25, "no data-driven corrections");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_withDDC";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData("responseMatrix", "pp_13TeV_withDDC");
    plot[1].AddText(0.4, 0.25, "with data-driven corrections");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("momentUnfolded1", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("momentUnfolded1_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].AddData("momentUnfolded1", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("momentUnfolded1_Syst", "pp_13TeV", "momentUnfolded1", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[2].SetAxisRange("Y", 0.99, 1.01);
    plot[1].AddLegend(0.2, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance_withDDC";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].AddData("varianceUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    plot[1].AddData("varianceUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].AddData("varianceUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    plot[2].AddRatio("varianceUnfolded_Syst", "pp_13TeV", "varianceUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.05, 0.9);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[1].AddLegend(0.2, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
}
