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

void MultDepSpec::DefineEnergyPlots(PlotManager& plotManager)
{
  /*
  string plotGroup = "energy_comparison";
  string energyLable = alice + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  string energyLableMC = "#bf{Pythia8 Monash13}" + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  
  { // -----------------------------------------------------------------------
    string plotName = "inverseSlope";
    Plot plot(plotName, plotGroup);
    plot[1].AddData(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    plot[1].AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    plot[1].AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    plot[1].AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    plot[1].AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    plot[1].AddData(plotName + "_pPb_5TeV", "Fits", "p-Pb 5.02 TeV");
    plot[1].AddData(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    //      plot[1].AddData(plotName + "_XeXe_5TeV", "Fits", "Xe-Xe 5.44 TeV");
    plot[1].SetAxisRange("X", 0, 3000);
    plot[1].SetAxisRange("Y", 4, 11);
    plot[1].AddLegend(0.7, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "yieldFractions";
    Plot plot(plotName, plotGroup);
    //plot[1].SetPadOptions("logX logY");
    plot[1].AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    plot[1].AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    plot[1].AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    plot[1].AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    plot[1].AddData(plotName + "_pPb_5TeV", "Fits", "p-Pb 5.02 TeV");
    plot[1].AddData(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    //plot[1].AddData(plotName + "_XeXe_5TeV", "Fits", "Xe-Xe 5.44 TeV");
    //plot[1].SetAxisRange("Y", 4, 11);
    plot[1].AddLegend(0.13, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "yieldFractionsSelfNormalized";
    Plot plot(plotName, plotGroup);
    //plot[1].SetPadOptions("logX logY");
    plot[1].AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    plot[1].AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    plot[1].AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    plot[1].AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    //plot[1].SetAxisRange("Y", 4, 11);
    plot[1].AddLegend(0.3, 0.9, "");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedNormalized";
    Plot plot(plotName, plotGroup, "default");
    plot[1].SetPadOptions("logY logZ");
    plot[1].AddData(plotName, "pp_5TeV", "5.02 TeV");
    plot[1].SetAxisRange("X", 0, 60);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "momentUnfolded1SelfNormalized";
    Plot plot(plotName, plotGroup, "default");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData(plotName, "pp_13TeV", "13 TeV");
    plot[1].AddData(plotName, "pp_7TeV", "7 TeV");
    plot[1].AddData(plotName, "pp_5TeV", "5.02 TeV");
    plot[1].AddData(plotName, "pp_2TeV", "2.76 TeV");
    plot[1].SetAxisRange("X", 0, 9);
    plot[1].SetAxisRange("Y", 0.7, 1.6);
    //plot[1].AddLegend(0.4, 0.4);
    plot[1].AddText(0.15, 0.91, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "momentUnfolded1SelfNormalizedWithPythia";
    Plot plot(plotName, plotGroup, "default");
    //plot[1].SetPadOptions("logX logY");
    string dataName = "momentUnfolded1SelfNormalized";
    string mcName = "momentGeneratedMC1SelfNormalized";
    plot[1].AddData(dataName, "pp_13TeV", "13 TeV");
    plot[1].AddData(mcName, "pp_13TeV", "", 0, -1, "band", 7);
    plot[1].AddData(dataName, "pp_7TeV", "7 TeV");
    plot[1].AddData(mcName, "pp_7TeV", "", 0, -1, "band", 7);
    plot[1].AddData(dataName, "pp_5TeV", "5.02 TeV");
    plot[1].AddData(mcName, "pp_5TeV", "", 0, -1, "band", 7);
    plot[1].AddData(dataName, "pp_2TeV", "2.76 TeV", 0, 0, "", 7);
    plot[1].AddData(mcName, "pp_2TeV", "", 0, -1, "band", 7);
    plot[1].SetAxisRange("X", 0, 9);
    plot[1].SetAxisRange("Y", 0.7, 1.6);
    plot[1].AddLegend(0.4, 0.4);
    plot[1].AddText(0.15, 0.91, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "momentGeneratedMC1SelfNormalized";
    Plot plot(plotName, plotGroup, "default");
    plot[1].SetPadOptions("logX logY");
    plot[1].AddData(plotName, "pp_13TeV", "13 TeV");
    plot[1].AddData(plotName, "pp_7TeV", "7 TeV (Pythia6)");
    plot[1].AddData(plotName, "pp_5TeV", "5.02 TeV");
    plot[1].AddData(plotName, "pp_2TeV", "2.76 TeV");
    plot[1].SetAxisRange("X", 0, 9);
    plot[1].SetAxisRange("Y", 0.7, 1.6);
    //plot[1].AddLegend(0.4, 0.4);
    plot[1].AddText(0.15, 0.91, energyLableMC);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "inclusiveSpectrum";
    Plot plot(plotName, plotGroup, "default ratio");
    plot[1].SetPadOptions("logY logX");
    plot[1].AddData(plotName, "PbPb_5TeV", "Pb-Pb 5.02 TeV");
    plot[1].AddData(plotName, "pp_13TeV", "13 TeV");
    plot[1].AddData(plotName, "pp_7TeV", "7 TeV");
    plot[1].AddData(plotName, "pp_5TeV", "5.02 TeV");
    plot[1].AddData(plotName, "pp_2TeV", "2.76 TeV");
    plot[1].AddLegend(0.7, 0.91);
    plot[2].AddRatio(plotName, "pp_7TeV", plotName, "pp_13TeV");
    plot[2].AddRatio(plotName, "pp_5TeV", plotName, "pp_13TeV");
    plot[2].AddRatio(plotName, "pp_2TeV", plotName, "pp_13TeV");
    plot[1].SetAxisTitle("Y", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{2}#it{N})/(d#it{p}_{T}d#it{#eta}) [(GeV/#it{c})^{-2}]");
    plot[1].AddText(0.2, 0.25, energyLable);
    plot[2].SetPadOptions("logX");
    plot[2].SetAxisTitle("Y", "ratio to 13");
    plot[1].SetAxisTitle("X", "#it{p}_{T} (GeV/#it{c})");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_5Over13";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY");
    plot[1].AddRatio("multPtUnfoldedNormalized", "pp_5TeV", "multPtUnfoldedNormalized", "pp_13TeV");
    //plot[2].AddRatio("multPtUnfolded", "pp_5TeV", "multPtUnfolded", "pp_13TeV");
    plot[1].SetAxisRange("Z", 0, 1.5);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "5 TeV / 13 TeV");
    //plot[1].AddText(0.3, 0.6, "5 TeV / 13 TeV");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_PbPbOverpPb";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY logZ");
    plot[1].AddRatio("multPtUnfolded", "PbPb_5TeV", "multPtUnfolded", "pPb_5TeV");
    //plot[1].SetAxisRange("Z", 1e-4, 5e-1);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "Pb-Pb / p-Pb");
    //plot[1].AddText(0.3, 0.6, "5 TeV / 13 TeV");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_2.76TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY logZ");
    plot[1].AddRatio("multPtUnfolded", "pp_2TeV", "multPt_2.76TeV", "Simulations");
    plot[1].SetAxisRange("Z", 0.5, 2.0);
    plot[1].SetAxisRange("X", 0, 40);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    plot[1].AddText(0.4, 0.4, "pp 2.76 TeV");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_5TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY logZ");
    plot[1].AddRatio("multPtUnfolded", "pp_5TeV", "multPt_5.02TeV", "Simulations");
    plot[1].SetAxisRange("Z", 0.5, 2.0);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    plot[1].AddText(0.4, 0.4, "pp 5 TeV");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_7TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY logZ");
    plot[1].AddRatio("multPtUnfolded", "pp_7TeV", "multPt_7TeV", "Simulations");
    plot[1].SetAxisRange("Z", 0.5, 2.0);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    plot[1].AddText(0.4, 0.4, "pp 7 TeV");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_13TeV";
    Plot plot(plotName, plotGroup);
    plot[1].SetPadOptions("logY logZ");
    plot[1].AddRatio("multPtUnfolded", "pp_13TeV", "multPt_13TeV", "Simulations");
    plot[1].SetAxisRange("Z", 0.5, 2.0);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.15, 20.0);
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    plot[1].SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    plot[1].AddText(0.4, 0.4, "pp 13 TeV");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMean";
    Plot plot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    //vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      plot[1].AddData(string("meanPt_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    plot[1].SetAxisRange("Y", 0.3, 1.0);
    plot[1].AddLegend(0.24, 0.91, "", false, 3);
    plot[1].AddText(0.4, 0.3, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMeanMC";
    Plot plot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      plot[1].AddData(string("meanPt_") + std::to_string(multBin), "Energyscan", string("#it{N}_{ch} = ") + std::to_string(multBin-1), 0, 0, "boxes");
      plot[1].AddData(string("meanPtMC_") + std::to_string(multBin), "Energyscan", "", 0, -1, "band");
      //        plot[1].AddData(string("meanPtMC_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    plot[1].SetAxisRange("Y", 0.3, 0.9);
    plot[1].AddLegend(0.15, 0.92, "", true, 2);
    plot[1].AddText(0.15, 0.35, energyLable + " // MC: Pythia8 Monash13");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMeanMCLog";
    Plot plot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      plot[1].AddData(string("meanPtMC_Log_") + std::to_string(multBin), "Energyscan", "", 0, 0, "band");
      //        plot[1].AddData(string("meanPtMC_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
      plot[1].AddData(string("meanPt_Log_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, -1, "boxes");
    }
    plot[1].SetAxisRange("Y", 0.3, 1.0);
    plot[1].AddLegend(0.24, 0.91, "", 3);
    //plot[1].AddText(0.4, 0.3, energyLable);
    plot[1].AddText(0.4, 0.3, energyLableMC);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVariance";
    Plot plot(plotName, plotGroup);
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      plot[1].AddData(string("variance_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    plot[1].SetAxisRange("Y", -0.4, 0.8);
    plot[1].AddLegend(0.24, 0.91);
    plot[1].AddText(0.4, 0.3, energyLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVarianceMC";
    Plot plot(plotName, plotGroup);
    
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      plot[1].AddData(string("variance_") + std::to_string(multBin), "Energyscan", string("#it{N}_{ch} = ") + std::to_string(multBin-1), 0, 0, "boxes");
      plot[1].AddData(string("varianceMC_") + std::to_string(multBin), "Energyscan", "", 0, -1, "band");
    }
    plot[1].SetAxisRange("Y", -0.4, 0.8);
    plot[1].AddLegend(0.24, 0.91, "",false, 2);
    plot[1].AddText(0.15, 0.35, energyLable + " // MC: Pythia8 Monash13");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVarianceMCLog";
    Plot plot(plotName, plotGroup);
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      plot[1].AddData(string("varianceMC_Log_") + std::to_string(multBin), "Energyscan", "", 0, 0, "band");
      plot[1].AddData(string("variance_Log_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, -1, "boxes");
    }
    plot[1].SetAxisRange("Y", -0.4, 1.4);
    plot[1].AddLegend(0.24, 0.91);
    //plot[1].AddText(0.4, 0.3, energyLable);
    plot[1].AddText(0.4, 0.3, energyLableMC);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "varianceMC";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("variance_2TeV", "Simulations", "2 TeV", 0, 0, "band");
    plot[1].AddData("variance_2.5TeV", "Simulations", "2.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_3TeV", "Simulations", "3 TeV", 0, 0, "band");
    plot[1].AddData("variance_3.5TeV", "Simulations", "3.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_4TeV", "Simulations", "4 TeV", 0, 0, "band");
    plot[1].AddData("variance_4.5TeV", "Simulations", "4.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_5TeV", "Simulations", "5 TeV", 0, 0, "band");
    plot[1].AddData("variance_5.5TeV", "Simulations", "5.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_6TeV", "Simulations", "6 TeV", 0, 0, "band");
    plot[1].AddData("variance_6.5TeV", "Simulations", "6.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_7TeV", "Simulations", "7 TeV", 0, 0, "band");
    plot[1].AddData("variance_7.5TeV", "Simulations", "7.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_8TeV", "Simulations", "8 TeV", 0, 0, "band");
    plot[1].AddData("variance_8.5TeV", "Simulations", "8.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_9TeV", "Simulations", "9 TeV", 0, 0, "band");
    plot[1].AddData("variance_9.5TeV", "Simulations", "9.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_10TeV", "Simulations", "10 TeV", 0, 0, "band");
    plot[1].AddData("variance_10.5TeV", "Simulations", "10.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_11TeV", "Simulations", "11 TeV", 0, 0, "band");
    plot[1].AddData("variance_11.5TeV", "Simulations", "11.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_12TeV", "Simulations", "12 TeV", 0, 0, "band");
    plot[1].AddData("variance_12.5TeV", "Simulations", "12.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_13TeV", "Simulations", "13 TeV", 0, 0, "band");
    plot[1].AddData("variance_13.5TeV", "Simulations", "13.5 TeV", 0, 0, "band");
    plot[1].AddData("variance_14TeV", "Simulations", "14 TeV", 0, 0, "band");
    //plot[1].AddData("variance_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    //plot[1].AddData("varianceGeneratedMC", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
    //plot[1].AddData("variance_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
    //plot[1].AddData("varianceGeneratedMC", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
    plot[1].SetAxisRange("X", 0, 40);
    plot[1].SetAxisRange("Y", 0.05, 0.9);
    plot[1].AddLegend(0.13, 0.9, "",4);
    plot[1].AddText(0.4, 0.3, energyLableMC);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1MC";
    Plot plot(plotName, plotGroup);
    plot[1].AddData("meanPt_2TeV", "Simulations", "2 TeV", 0, 0, "band");
    plot[1].AddData("meanPt_2.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_3TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_3.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_4TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_4.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_5.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_6TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_6.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_7TeV", "Simulations", "7 TeV", 0, 0, "band");
    plot[1].AddData("meanPt_7.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_8TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_8.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_9TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_9.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_10TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_10.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_11TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_11.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_12TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_12.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_13TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_13.5TeV", "Simulations", "", 0, 0, "band");
    plot[1].AddData("meanPt_14TeV", "Simulations", "14 TeV", 0, 0, "band");
    //plot[1].AddData("meanPt_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    //plot[1].AddData("momentGeneratedMC1", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
    //plot[1].AddData("meanPt_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
    //plot[1].AddData("momentGeneratedMC1", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.35, 0.3, "",4);
    plot[1].AddText(0.4, 0.5, energyLableMC);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
   */
}
