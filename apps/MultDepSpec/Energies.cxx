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

void MultDepSpec::DefineEnergyPlots(PlotManager& plotEnv)
{
  string plotGroup = "energy_comparison";
  string energyLable = alice + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  string energyLableMC = "#bf{Pythia8 Monash13}" + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
  
  { // -----------------------------------------------------------------------
    string plotName = "inverseSlope";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddData(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    myPlot.AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    myPlot.AddData(plotName + "_pPb_5TeV", "Fits", "p-Pb 5.02 TeV");
    myPlot.AddData(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    //      myPlot.AddData(plotName + "_XeXe_5TeV", "Fits", "Xe-Xe 5.44 TeV");
    myPlot.SetAxisRange("X", 0, 3000);
    myPlot.SetAxisRange("Y", 4, 11);
    myPlot.AddLegend(0.7, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "yieldFractions";
    Plot myPlot(plotName, plotGroup);
    //myPlot.SetDrawingProperties("logX logY");
    myPlot.AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    myPlot.AddData(plotName + "_pPb_5TeV", "Fits", "p-Pb 5.02 TeV");
    myPlot.AddData(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    //myPlot.AddData(plotName + "_XeXe_5TeV", "Fits", "Xe-Xe 5.44 TeV");
    //myPlot.SetAxisRange("Y", 4, 11);
    myPlot.AddLegend(0.13, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "yieldFractionsSelfNormalized";
    Plot myPlot(plotName, plotGroup);
    //myPlot.SetDrawingProperties("logX logY");
    myPlot.AddData(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddData(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddData(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddData(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    //myPlot.SetAxisRange("Y", 4, 11);
    myPlot.AddLegend(0.3, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedNormalized";
    Plot myPlot(plotName, plotGroup, "default");
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddData(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.SetAxisRange("X", 0, 60);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "momentUnfolded1SelfNormalized";
    Plot myPlot(plotName, plotGroup, "default");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddData(plotName, "pp_13TeV", "13 TeV");
    myPlot.AddData(plotName, "pp_7TeV", "7 TeV");
    myPlot.AddData(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.AddData(plotName, "pp_2TeV", "2.76 TeV");
    myPlot.SetAxisRange("X", 0, 9);
    myPlot.SetAxisRange("Y", 0.7, 1.6);
    //myPlot.AddLegend(0.4, 0.4);
    myPlot.AddText(0.15, 0.91, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "momentUnfolded1SelfNormalizedWithPythia";
    Plot myPlot(plotName, plotGroup, "default");
    //myPlot.SetDrawingProperties("logX logY");
    string dataName = "momentUnfolded1SelfNormalized";
    string mcName = "momentGeneratedMC1SelfNormalized";
    myPlot.AddData(dataName, "pp_13TeV", "13 TeV");
    myPlot.AddData(mcName, "pp_13TeV", "", 0, -1, "band", 7);
    myPlot.AddData(dataName, "pp_7TeV", "7 TeV");
    myPlot.AddData(mcName, "pp_7TeV", "", 0, -1, "band", 7);
    myPlot.AddData(dataName, "pp_5TeV", "5.02 TeV");
    myPlot.AddData(mcName, "pp_5TeV", "", 0, -1, "band", 7);
    myPlot.AddData(dataName, "pp_2TeV", "2.76 TeV", 0, 0, "", 7);
    myPlot.AddData(mcName, "pp_2TeV", "", 0, -1, "band", 7);
    myPlot.SetAxisRange("X", 0, 9);
    myPlot.SetAxisRange("Y", 0.7, 1.6);
    myPlot.AddLegend(0.4, 0.4);
    myPlot.AddText(0.15, 0.91, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "momentGeneratedMC1SelfNormalized";
    Plot myPlot(plotName, plotGroup, "default");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddData(plotName, "pp_13TeV", "13 TeV");
    myPlot.AddData(plotName, "pp_7TeV", "7 TeV (Pythia6)");
    myPlot.AddData(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.AddData(plotName, "pp_2TeV", "2.76 TeV");
    myPlot.SetAxisRange("X", 0, 9);
    myPlot.SetAxisRange("Y", 0.7, 1.6);
    //myPlot.AddLegend(0.4, 0.4);
    myPlot.AddText(0.15, 0.91, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "inclusiveSpectrum";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY logX");
    myPlot.AddData(plotName, "PbPb_5TeV", "Pb-Pb 5.02 TeV");
    myPlot.AddData(plotName, "pp_13TeV", "13 TeV");
    myPlot.AddData(plotName, "pp_7TeV", "7 TeV");
    myPlot.AddData(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.AddData(plotName, "pp_2TeV", "2.76 TeV");
    myPlot.AddLegend(0.7, 0.91);
    myPlot.AddRatio(plotName, "pp_7TeV", plotName, "pp_13TeV");
    myPlot.AddRatio(plotName, "pp_5TeV", plotName, "pp_13TeV");
    myPlot.AddRatio(plotName, "pp_2TeV", plotName, "pp_13TeV");
    myPlot.SetAxisTitle("Y", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{2}#it{N})/(d#it{p}_{T}d#it{#eta}) [(GeV/#it{c})^{-2}]");
    myPlot.AddText(0.2, 0.25, energyLable);
    myPlot.ChangePad(2);
    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisTitle("ratio", "ratio to 13");
    myPlot.SetAxisTitle("X", "#it{p}_{T} (GeV/#it{c})");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_5Over13";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddRatio("multPtUnfoldedNormalized", "pp_5TeV", "multPtUnfoldedNormalized", "pp_13TeV");
    //myPlot.AddRatio("multPtUnfolded", "pp_5TeV", "multPtUnfolded", "pp_13TeV");
    myPlot.SetAxisRange("Z", 0, 1.5);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "5 TeV / 13 TeV");
    //myPlot.AddText(0.3, 0.6, "5 TeV / 13 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_PbPbOverpPb";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddRatio("multPtUnfolded", "PbPb_5TeV", "multPtUnfolded", "pPb_5TeV");
    //myPlot.SetAxisRange("Z", 1e-4, 5e-1);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "Pb-Pb / p-Pb");
    //myPlot.AddText(0.3, 0.6, "5 TeV / 13 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_2.76TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddRatio("multPtUnfolded", "pp_2TeV", "multPt_2.76TeV", "Simulations");
    myPlot.SetAxisRange("Z", 0.5, 2.0);
    myPlot.SetAxisRange("X", 0, 40);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    myPlot.AddText(0.4, 0.4, "pp 2.76 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddRatio("multPtUnfolded", "pp_5TeV", "multPt_5.02TeV", "Simulations");
    myPlot.SetAxisRange("Z", 0.5, 2.0);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    myPlot.AddText(0.4, 0.4, "pp 5 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_7TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddRatio("multPtUnfolded", "pp_7TeV", "multPt_7TeV", "Simulations");
    myPlot.SetAxisRange("Z", 0.5, 2.0);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    myPlot.AddText(0.4, 0.4, "pp 7 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatioToPythia_13TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddRatio("multPtUnfolded", "pp_13TeV", "multPt_13TeV", "Simulations");
    myPlot.SetAxisRange("Z", 0.5, 2.0);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "ratio to Pythia8 Monash13");
    myPlot.AddText(0.4, 0.4, "pp 13 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMean";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    //vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddData(string("meanPt_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    myPlot.SetAxisRange("Y", 0.3, 1.0);
    myPlot.AddLegend(0.24, 0.91, "", false, 3);
    myPlot.AddText(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMeanMC";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddData(string("meanPt_") + std::to_string(multBin), "Energyscan", string("#it{N}_{ch} = ") + std::to_string(multBin-1), 0, 0, "boxes");
      myPlot.AddData(string("meanPtMC_") + std::to_string(multBin), "Energyscan", "", 0, -1, "band");
      //        myPlot.AddData(string("meanPtMC_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    myPlot.SetAxisRange("Y", 0.3, 0.9);
    myPlot.AddLegend(0.15, 0.92, "", true, 2);
    myPlot.AddText(0.15, 0.35, energyLable + " // MC: Pythia8 Monash13");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMeanMCLog";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddData(string("meanPtMC_Log_") + std::to_string(multBin), "Energyscan", "", 0, 0, "band");
      //        myPlot.AddData(string("meanPtMC_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
      myPlot.AddData(string("meanPt_Log_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, -1, "boxes");
    }
    myPlot.SetAxisRange("Y", 0.3, 1.0);
    myPlot.AddLegend(0.24, 0.91, "", 3);
    //myPlot.AddText(0.4, 0.3, energyLable);
    myPlot.AddText(0.4, 0.3, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVariance";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddData(string("variance_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    myPlot.SetAxisRange("Y", -0.4, 0.8);
    myPlot.AddLegend(0.24, 0.91);
    myPlot.AddText(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVarianceMC";
    Plot myPlot(plotName, plotGroup);
    
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddData(string("variance_") + std::to_string(multBin), "Energyscan", string("#it{N}_{ch} = ") + std::to_string(multBin-1), 0, 0, "boxes");
      myPlot.AddData(string("varianceMC_") + std::to_string(multBin), "Energyscan", "", 0, -1, "band");
    }
    myPlot.SetAxisRange("Y", -0.4, 0.8);
    myPlot.AddLegend(0.24, 0.91, "",false, 2);
    myPlot.AddText(0.15, 0.35, energyLable + " // MC: Pythia8 Monash13");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVarianceMCLog";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddData(string("varianceMC_Log_") + std::to_string(multBin), "Energyscan", "", 0, 0, "band");
      myPlot.AddData(string("variance_Log_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, -1, "boxes");
    }
    myPlot.SetAxisRange("Y", -0.4, 1.4);
    myPlot.AddLegend(0.24, 0.91);
    //myPlot.AddText(0.4, 0.3, energyLable);
    myPlot.AddText(0.4, 0.3, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "varianceMC";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddData("variance_2TeV", "Simulations", "2 TeV", 0, 0, "band");
    myPlot.AddData("variance_2.5TeV", "Simulations", "2.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_3TeV", "Simulations", "3 TeV", 0, 0, "band");
    myPlot.AddData("variance_3.5TeV", "Simulations", "3.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_4TeV", "Simulations", "4 TeV", 0, 0, "band");
    myPlot.AddData("variance_4.5TeV", "Simulations", "4.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_5TeV", "Simulations", "5 TeV", 0, 0, "band");
    myPlot.AddData("variance_5.5TeV", "Simulations", "5.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_6TeV", "Simulations", "6 TeV", 0, 0, "band");
    myPlot.AddData("variance_6.5TeV", "Simulations", "6.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_7TeV", "Simulations", "7 TeV", 0, 0, "band");
    myPlot.AddData("variance_7.5TeV", "Simulations", "7.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_8TeV", "Simulations", "8 TeV", 0, 0, "band");
    myPlot.AddData("variance_8.5TeV", "Simulations", "8.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_9TeV", "Simulations", "9 TeV", 0, 0, "band");
    myPlot.AddData("variance_9.5TeV", "Simulations", "9.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_10TeV", "Simulations", "10 TeV", 0, 0, "band");
    myPlot.AddData("variance_10.5TeV", "Simulations", "10.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_11TeV", "Simulations", "11 TeV", 0, 0, "band");
    myPlot.AddData("variance_11.5TeV", "Simulations", "11.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_12TeV", "Simulations", "12 TeV", 0, 0, "band");
    myPlot.AddData("variance_12.5TeV", "Simulations", "12.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_13TeV", "Simulations", "13 TeV", 0, 0, "band");
    myPlot.AddData("variance_13.5TeV", "Simulations", "13.5 TeV", 0, 0, "band");
    myPlot.AddData("variance_14TeV", "Simulations", "14 TeV", 0, 0, "band");
    //myPlot.AddData("variance_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    //myPlot.AddData("varianceGeneratedMC", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
    //myPlot.AddData("variance_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
    //myPlot.AddData("varianceGeneratedMC", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
    myPlot.SetAxisRange("X", 0, 40);
    myPlot.SetAxisRange("Y", 0.05, 0.9);
    myPlot.AddLegend(0.13, 0.9, "",4);
    myPlot.AddText(0.4, 0.3, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1MC";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddData("meanPt_2TeV", "Simulations", "2 TeV", 0, 0, "band");
    myPlot.AddData("meanPt_2.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_3TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_3.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_4TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_4.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_5.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_6TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_6.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_7TeV", "Simulations", "7 TeV", 0, 0, "band");
    myPlot.AddData("meanPt_7.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_8TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_8.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_9TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_9.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_10TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_10.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_11TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_11.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_12TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_12.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_13TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_13.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddData("meanPt_14TeV", "Simulations", "14 TeV", 0, 0, "band");
    //myPlot.AddData("meanPt_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    //myPlot.AddData("momentGeneratedMC1", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
    //myPlot.AddData("meanPt_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
    //myPlot.AddData("momentGeneratedMC1", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegend(0.35, 0.3, "",4);
    myPlot.AddText(0.4, 0.5, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
}
