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
    myPlot.AddHisto(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    myPlot.AddHisto(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddHisto(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddHisto(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddHisto(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    myPlot.AddHisto(plotName + "_pPb_5TeV", "Fits", "p-Pb 5.02 TeV");
    myPlot.AddHisto(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    //      myPlot.AddHisto(plotName + "_XeXe_5TeV", "Fits", "Xe-Xe 5.44 TeV");
    myPlot.SetAxisRange("X", 0, 3000);
    myPlot.SetAxisRange("Y", 4, 11);
    myPlot.AddLegendBox(0.7, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "yieldFractions";
    Plot myPlot(plotName, plotGroup);
    //      myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddHisto(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddHisto(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddHisto(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    myPlot.AddHisto(plotName + "_pPb_5TeV", "Fits", "p-Pb 5.02 TeV");
    myPlot.AddHisto(plotName + "_PbPb_5TeV", "Fits", "Pb-Pb 5.02 TeV");
    //      myPlot.AddHisto(plotName + "_XeXe_5TeV", "Fits", "Xe-Xe 5.44 TeV");
    //      myPlot.SetAxisRange("Y", 4, 11);
    myPlot.AddLegendBox(0.13, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "yieldFractionsSelfNormalized";
    Plot myPlot(plotName, plotGroup);
    //      myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddHisto(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddHisto(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddHisto(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    //      myPlot.SetAxisRange("Y", 4, 11);
    myPlot.AddLegendBox(0.3, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedNormalized";
    Plot myPlot(plotName, plotGroup, "default");
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddHisto(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.SetAxisRange("X", 0, 60);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "momentUnfolded1SelfNormalized";
    Plot myPlot(plotName, plotGroup, "default");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto(plotName, "pp_13TeV", "13 TeV");
    myPlot.AddHisto(plotName, "pp_7TeV", "7 TeV");
    myPlot.AddHisto(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.AddHisto(plotName, "pp_2TeV", "2.76 TeV");
    myPlot.SetAxisRange("X", 0, 9);
    myPlot.SetAxisRange("Y", 0.7, 1.6);
    //      myPlot.AddLegendBox(0.4, 0.4);
    myPlot.AddTextBox(0.15, 0.91, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "momentUnfolded1SelfNormalizedWithPythia";
    Plot myPlot(plotName, plotGroup, "default");
    //myPlot.SetDrawingProperties("logX logY");
    string dataName = "momentUnfolded1SelfNormalized";
    string mcName = "momentGeneratedMC1SelfNormalized";
    myPlot.AddHisto(dataName, "pp_13TeV", "13 TeV");
    myPlot.AddHisto(mcName, "pp_13TeV", "", 0, -1, "band", 7);
    myPlot.AddHisto(dataName, "pp_7TeV", "7 TeV");
    myPlot.AddHisto(mcName, "pp_7TeV", "", 0, -1, "band", 7);
    myPlot.AddHisto(dataName, "pp_5TeV", "5.02 TeV");
    myPlot.AddHisto(mcName, "pp_5TeV", "", 0, -1, "band", 7);
    myPlot.AddHisto(dataName, "pp_2TeV", "2.76 TeV", 0, 0, "", 7);
    myPlot.AddHisto(mcName, "pp_2TeV", "", 0, -1, "band", 7);
    
    
    myPlot.SetAxisRange("X", 0, 9);
    myPlot.SetAxisRange("Y", 0.7, 1.6);
    myPlot.AddLegendBox(0.4, 0.4);
    myPlot.AddTextBox(0.15, 0.91, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    string plotName = "momentGeneratedMC1SelfNormalized";
    Plot myPlot(plotName, plotGroup, "default");
    myPlot.SetDrawingProperties("logX logY");
    myPlot.AddHisto(plotName, "pp_13TeV", "13 TeV");
    myPlot.AddHisto(plotName, "pp_7TeV", "7 TeV (Pythia6)");
    myPlot.AddHisto(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.AddHisto(plotName, "pp_2TeV", "2.76 TeV");
    myPlot.SetAxisRange("X", 0, 9);
    myPlot.SetAxisRange("Y", 0.7, 1.6);
    //      myPlot.AddLegendBox(0.4, 0.4);
    myPlot.AddTextBox(0.15, 0.91, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    string plotName = "inclusiveSpectrum";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY logX");
    myPlot.AddHisto(plotName, "PbPb_5TeV", "Pb-Pb 5.02 TeV");
    myPlot.AddHisto(plotName, "pp_13TeV", "13 TeV");
    myPlot.AddHisto(plotName, "pp_7TeV", "7 TeV");
    myPlot.AddHisto(plotName, "pp_5TeV", "5.02 TeV");
    myPlot.AddHisto(plotName, "pp_2TeV", "2.76 TeV");
    myPlot.AddLegendBox(0.7, 0.91);
    myPlot.AddRatio(plotName, "pp_7TeV", plotName, "pp_13TeV");
    myPlot.AddRatio(plotName, "pp_5TeV", plotName, "pp_13TeV");
    myPlot.AddRatio(plotName, "pp_2TeV", plotName, "pp_13TeV");
    myPlot.SetAxisTitle("Y", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{2}#it{N})/(d#it{p}_{T}d#it{#eta}) [(GeV/#it{c})^{-2}]");
    myPlot.AddTextBox(0.2, 0.25, energyLable);
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
    //      myPlot.AddRatio("multPtUnfolded", "pp_5TeV", "multPtUnfolded", "pp_13TeV");
    myPlot.SetAxisRange("Z", 0, 1.5);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "5 TeV / 13 TeV");
    //      myPlot.AddTextBox(0.3, 0.6, "5 TeV / 13 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtRatio_PbPbOverpPb";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logZ");
    myPlot.AddRatio("multPtUnfolded", "PbPb_5TeV", "multPtUnfolded", "pPb_5TeV");
    //      myPlot.SetAxisRange("Z", 1e-4, 5e-1);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.15, 20.0);
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Z", "Pb-Pb / p-Pb");
    //      myPlot.AddTextBox(0.3, 0.6, "5 TeV / 13 TeV");
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
    myPlot.AddTextBox(0.4, 0.4, "pp 2.76 TeV");
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
    myPlot.AddTextBox(0.4, 0.4, "pp 5 TeV");
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
    myPlot.AddTextBox(0.4, 0.4, "pp 7 TeV");
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
    myPlot.AddTextBox(0.4, 0.4, "pp 13 TeV");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMean";
    Plot myPlot(plotName, plotGroup);
    
    vector<int> multBins = {2, 9, 16, 26, 36};
    //vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddHisto(string("meanPt_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    myPlot.SetAxisRange("Y", 0.3, 1.0);
    myPlot.AddLegendBox(0.24, 0.91, "", false, 3);
    myPlot.AddTextBox(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMeanMC";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddHisto(string("meanPt_") + std::to_string(multBin), "Energyscan", string("#it{N}_{ch} = ") + std::to_string(multBin-1), 0, 0, "boxes");
      myPlot.AddHisto(string("meanPtMC_") + std::to_string(multBin), "Energyscan", "", 0, -1, "band");
      //        myPlot.AddHisto(string("meanPtMC_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    myPlot.SetAxisRange("Y", 0.3, 0.9);
    myPlot.AddLegendBox(0.15, 0.92, "", true, 2);
    myPlot.AddTextBox(0.15, 0.35, energyLable + " // MC: Pythia8 Monash13");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "energyScanMeanMCLog";
    Plot myPlot(plotName, plotGroup);
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddHisto(string("meanPtMC_Log_") + std::to_string(multBin), "Energyscan", "", 0, 0, "band");
      //        myPlot.AddHisto(string("meanPtMC_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
      myPlot.AddHisto(string("meanPt_Log_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, -1, "boxes");
    }
    myPlot.SetAxisRange("Y", 0.3, 1.0);
    myPlot.AddLegendBox(0.24, 0.91, "", 3);
    //myPlot.AddTextBox(0.4, 0.3, energyLable);
    myPlot.AddTextBox(0.4, 0.3, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVariance";
    Plot myPlot(plotName, plotGroup);
    
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddHisto(string("variance_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, 0, "boxes");
    }
    myPlot.SetAxisRange("Y", -0.4, 0.8);
    myPlot.AddLegendBox(0.24, 0.91);
    myPlot.AddTextBox(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVarianceMC";
    Plot myPlot(plotName, plotGroup);
    
    vector<int> multBins = {2, 9, 16, 26, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddHisto(string("variance_") + std::to_string(multBin), "Energyscan", string("#it{N}_{ch} = ") + std::to_string(multBin-1), 0, 0, "boxes");
      myPlot.AddHisto(string("varianceMC_") + std::to_string(multBin), "Energyscan", "", 0, -1, "band");
    }
    myPlot.SetAxisRange("Y", -0.4, 0.8);
    myPlot.AddLegendBox(0.24, 0.91, "",false, 2);
    myPlot.AddTextBox(0.15, 0.35, energyLable + " // MC: Pythia8 Monash13");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "energyScanVarianceMCLog";
    Plot myPlot(plotName, plotGroup);
    
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
    for(auto& multBin : multBins)
    {
      myPlot.AddHisto(string("varianceMC_Log_") + std::to_string(multBin), "Energyscan", "", 0, 0, "band");
      myPlot.AddHisto(string("variance_Log_") + std::to_string(multBin), "Energyscan", string("Nch = ") + std::to_string(multBin-1), 0, -1, "boxes");
    }
    myPlot.SetAxisRange("Y", -0.4, 1.4);
    myPlot.AddLegendBox(0.24, 0.91);
    //myPlot.AddTextBox(0.4, 0.3, energyLable);
    myPlot.AddTextBox(0.4, 0.3, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "meanPt";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", marker::pp_13, color::pp_13, "", range::pp_13);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "13 TeV", marker::pp_13, color::pp_13, "boxes", 60);
    //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_13TeV", "", kStar, kRed+1, "");
    myPlot.AddHisto("momentUnfolded1", "pp_7TeV", "", marker::pp_7, color::pp_7, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_7TeV", "7 TeV", marker::pp_7, color::pp_7, "boxes", 60);
    //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_7TeV", "", kStar, kMagenta+1, "");
    myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", marker::pp_5, color::pp_5, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "5.02 TeV", marker::pp_5, color::pp_5, "boxes", 60);
    //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_5TeV", "", kStar, kBlue+1, "");
    myPlot.AddHisto("momentUnfolded1", "pp_2TeV", "", marker::pp_2, color::pp_2, "", 40);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_2TeV", "2.76 TeV", marker::pp_2, color::pp_2, "boxes", 40);
    //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_2TeV", "", kStar, kGreen+3, "");
    
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "meanPt_pp_VS_PYTHIA";
    Plot myPlot(plotName, plotGroup, "default ratio");
    
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", kFullSquare, kRed+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes", 60);
    
    myPlot.AddHisto("meanPt_13TeV", "Simulations", "", kOpenSquare, kRed+1, "", 60);
    
    myPlot.AddHisto("momentUnfolded1", "pp_7TeV", "", kFullCircle, kMagenta+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes", 60);
    
    myPlot.AddHisto("meanPt_7TeV", "Simulations", "", kOpenCircle, kMagenta+1, "", 60);
    
    myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullCross, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 60);
    
    myPlot.AddHisto("meanPt_5.02TeV", "Simulations", "", kOpenCross, kBlue+1, "", 60);
    
    myPlot.AddHisto("momentUnfolded1", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 40);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 40);
    
    myPlot.AddHisto("meanPt_2.76TeV", "Simulations", "", kOpenDiamond, kGreen+3, "", 40);
    
    myPlot.AddRatio("meanPt_13TeV", "Simulations", "momentUnfolded1_Syst", "pp_13TeV", "", kOpenSquare, kRed+1, "", 60);
    myPlot.AddRatio("meanPt_7TeV", "Simulations", "momentUnfolded1_Syst", "pp_7TeV", "", kOpenCircle, kMagenta+1, "", 60);
    myPlot.AddRatio("meanPt_5.02TeV", "Simulations", "momentUnfolded1_Syst", "pp_5TeV", "", kOpenCross, kBlue+1, "", 60);
    myPlot.AddRatio("meanPt_2.76TeV", "Simulations", "momentUnfolded1_Syst", "pp_2TeV", "", kOpenDiamond, kGreen+3, "", 40);
    
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("ratio", "MC / data");
    myPlot.SetAxisRange("ratio", 0.94, 1.06);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.46, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, energyLable + " // MC: PYTHIA 8 Monash13");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "meanPt_pp_VS_EPOS";
    Plot myPlot(plotName, plotGroup, "default ratio");
    
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", kFullSquare, kRed+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes", 60);
    
    myPlot.AddHisto("meanPt_pp_EPOS-LHC_13TeV", "Simulations", "", kOpenSquare, kRed+1, "", 60);
    
    myPlot.AddHisto("momentUnfolded1", "pp_7TeV", "", kFullCircle, kMagenta+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes", 60);
    
    myPlot.AddHisto("meanPt_pp_EPOS-LHC_7TeV", "Simulations", "", kOpenCircle, kMagenta+1, "", 60);
    
    myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullCross, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 60);
    
    myPlot.AddHisto("meanPt_pp_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCross, kBlue+1, "", 60);
    
    myPlot.AddHisto("momentUnfolded1", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 40);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 40);
    
    myPlot.AddHisto("meanPt_pp_EPOS-LHC_2.76TeV", "Simulations", "", kOpenDiamond, kGreen+3, "", 40);
    
    myPlot.AddRatio("meanPt_pp_EPOS-LHC_13TeV", "Simulations", "momentUnfolded1_Syst", "pp_13TeV", "", kOpenSquare, kRed+1, "", 60);
    myPlot.AddRatio("meanPt_pp_EPOS-LHC_7TeV", "Simulations", "momentUnfolded1_Syst", "pp_7TeV", "", kOpenCircle, kMagenta+1, "", 60);
    myPlot.AddRatio("meanPt_pp_EPOS-LHC_5.02TeV", "Simulations", "momentUnfolded1_Syst", "pp_5TeV", "", kOpenCross, kBlue+1, "", 60);
    myPlot.AddRatio("meanPt_pp_EPOS-LHC_2.76TeV", "Simulations", "momentUnfolded1_Syst", "pp_2TeV", "", kOpenDiamond, kGreen+3, "", 40);
    
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.46, 0.85);
    myPlot.SetAxisTitle("ratio", "MC / data");
    myPlot.SetAxisRange("ratio", 0.94, 1.06);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, energyLable + " // MC: EPOS-LHC");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    string plotName = "variance";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.AddHisto("varianceUnfolded", "pp_13TeV", "", kFullSquare, kRed+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes", 60);
    //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_13TeV_old", "", kStar, kRed+1, "");
    myPlot.AddHisto("varianceUnfolded", "pp_7TeV", "", kFullCircle, kMagenta+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes", 60);
    //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_7TeV", "", kStar, kMagenta+1, "");
    myPlot.AddHisto("varianceUnfolded", "pp_5TeV", "", kFullCross, kBlue+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 60);
    //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_5TeV", "", kStar, kBlue+1, "");
    myPlot.AddHisto("varianceUnfolded", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 40);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 40);
    //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_2TeV", "", kStar, kGreen+3, "");
    
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.05, 0.75);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  
  { // -----------------------------------------------------------------------
    string plotName = "varianceMC";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.AddHisto("variance_2TeV", "Simulations", "2 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_2.5TeV", "Simulations", "2.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_3TeV", "Simulations", "3 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_3.5TeV", "Simulations", "3.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_4TeV", "Simulations", "4 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_4.5TeV", "Simulations", "4.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_5TeV", "Simulations", "5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_5.5TeV", "Simulations", "5.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_6TeV", "Simulations", "6 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_6.5TeV", "Simulations", "6.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_7TeV", "Simulations", "7 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_7.5TeV", "Simulations", "7.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_8TeV", "Simulations", "8 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_8.5TeV", "Simulations", "8.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_9TeV", "Simulations", "9 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_9.5TeV", "Simulations", "9.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_10TeV", "Simulations", "10 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_10.5TeV", "Simulations", "10.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_11TeV", "Simulations", "11 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_11.5TeV", "Simulations", "11.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_12TeV", "Simulations", "12 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_12.5TeV", "Simulations", "12.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_13TeV", "Simulations", "13 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_13.5TeV", "Simulations", "13.5 TeV", 0, 0, "band");
    myPlot.AddHisto("variance_14TeV", "Simulations", "14 TeV", 0, 0, "band");
    //myPlot.AddHisto("variance_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    //myPlot.AddHisto("varianceGeneratedMC", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
    //myPlot.AddHisto("variance_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
    //myPlot.AddHisto("varianceGeneratedMC", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
    myPlot.SetAxisRange("X", 0, 40);
    myPlot.SetAxisRange("Y", 0.05, 0.9);
    myPlot.AddLegendBox(0.13, 0.9, "",4);
    myPlot.AddTextBox(0.4, 0.3, energyLableMC);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "moment1MC";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.AddHisto("meanPt_2TeV", "Simulations", "2 TeV", 0, 0, "band");
    
    myPlot.AddHisto("meanPt_2.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_3TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_3.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_4TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_4.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_5.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_6TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_6.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_7TeV", "Simulations", "7 TeV", 0, 0, "band");
    myPlot.AddHisto("meanPt_7.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_8TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_8.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_9TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_9.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_10TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_10.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_11TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_11.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_12TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_12.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_13TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_13.5TeV", "Simulations", "", 0, 0, "band");
    myPlot.AddHisto("meanPt_14TeV", "Simulations", "14 TeV", 0, 0, "band");
    
    
    //myPlot.AddHisto("meanPt_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    //myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
    //myPlot.AddHisto("meanPt_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
    //myPlot.AddHisto("momentGeneratedMC1", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.35, 0.3, "",4);
    myPlot.AddTextBox(0.4, 0.5, energyLableMC);
    
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "moment1MCGenerated";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentGeneratedMC1", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
    myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "");
    myPlot.AddHisto("momentGeneratedMC1", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "");
    myPlot.AddHisto("momentGeneratedMC1", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.6, 0.5, "");
    myPlot.AddTextBox(0.15, 0.92, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "moment2";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded2", "pp_2TeV", "", kFullSquare, kGreen+3, "", 40);
    myPlot.AddHisto("momentUnfolded2_Syst", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "boxes", 40);
    myPlot.AddHisto("momentUnfolded2", "pp_5TeV", "", kFullSquare, kBlue+1);
    myPlot.AddHisto("momentUnfolded2_Syst", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "boxes");
    myPlot.AddHisto("momentUnfolded2", "pp_7TeV", "", kFullCircle, kMagenta+1);
    myPlot.AddHisto("momentUnfolded2_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddHisto("momentUnfolded2", "pp_13TeV", "", kFullCross, kRed+1);
    myPlot.AddHisto("momentUnfolded2_Syst", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "boxes");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.2, 1.5);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "moment3";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded3", "pp_2TeV", "", kFullSquare, kGreen+3, "", 40);
    myPlot.AddHisto("momentUnfolded3_Syst", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "boxes", 40);
    myPlot.AddHisto("momentUnfolded3", "pp_5TeV", "", kFullSquare, kBlue+1);
    myPlot.AddHisto("momentUnfolded3_Syst", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "boxes");
    myPlot.AddHisto("momentUnfolded3", "pp_7TeV", "", kFullCircle, kMagenta+1);
    myPlot.AddHisto("momentUnfolded3_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddHisto("momentUnfolded3", "pp_13TeV", "", kFullCross, kRed+1);
    myPlot.AddHisto("momentUnfolded3_Syst", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "boxes");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.1, 7.0);
    myPlot.AddLegendBox(0.6, 0.4, "");
    myPlot.AddTextBox(0.15, 0.92, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDensity";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDensityUnfolded", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 42);
    myPlot.AddHisto("multDensityUnfolded", "pp_13TeV", "", kFullSquare, kRed+1);
    myPlot.AddHisto("multDensityUnfolded_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes");
    //myPlot.AddHisto("multDensityUnfolded", "pp_7TeV", "", kFullCircle, kMagenta+1);
    //myPlot.AddHisto("multDensityUnfolded_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddHisto("multDist_pp_7TeV_Stat", "Publications", "", kFullCircle, kMagenta+1, "", 52);
    myPlot.AddHisto("multDist_pp_7TeV_Syst", "Publications", "7 TeV", kFullCircle, kMagenta+1, "boxes", 52);
    myPlot.AddHisto("multDensityUnfolded", "pp_5TeV", "", kFullCross, kBlue+1, "", 49);
    myPlot.AddHisto("multDensityUnfolded_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 49);
    myPlot.AddHisto("multDensityUnfolded", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 42);
    myPlot.AddHisto("multDensityUnfolded_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 42);
    myPlot.AddHisto("multDist_pp_0.9TeV_Stat", "Publications", "", kFullStar, kBlack, "", 33);
    myPlot.AddHisto("multDist_pp_0.9TeV_Syst", "Publications", "0.9 TeV", kFullStar, kBlack, "boxes", 33);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{P}(#it{N}_{ch})");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 1e-4, 3e-1);
    myPlot.AddLegendBox(0.18, 0.45);
    myPlot.AddTextBox(0.4, 0.9, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistKNO";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV", "", kOpenCross, kBlue+1);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV", "", kOpenSquare, kRed+1);
    myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_13TeV", "13 TeV", kOpenSquare, kRed+1, "boxes");
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_7TeV", "", kOpenCircle, kMagenta+1);
    myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_7TeV", "7 TeV", kOpenCircle, kMagenta+1, "boxes");
    //myPlot.AddHisto("multDist_pp_7TeVKNO_Stat", "Publications", "", kOpenCircle, kMagenta+1, "");
    //myPlot.AddHisto("multDist_pp_7TeVKNO_Syst", "Publications", "7 TeV", kOpenCircle, kMagenta+1, "boxes");
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV", "", kOpenCross, kBlue+1);
    myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_5TeV", "5.02 TeV", kOpenCross, kBlue+1, "boxes");
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_2TeV", "", kOpenDiamond, kGreen+3);
    myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_2TeV", "2.76 TeV", kOpenDiamond, kGreen+3, "boxes");
    myPlot.AddHisto("multDist_pp_0.9TeVKNO_Stat", "Publications", "", kOpenStar, kBlack, "");
    myPlot.AddHisto("multDist_pp_0.9TeVKNO_Syst", "Publications", "0.9 TeV", kOpenStar, kBlack, "boxes");
    myPlot.SetAxisRange("X", 0, 5);
    //myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddLegendBox(0.7, 0.9);
    myPlot.AddTextBox(0.15, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDistKNORatio";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV", "5.02 TeV", 0, kBlue+1);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_2TeV", "2.76 TeV", 0, kGreen+3);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_7TeV", "7 TeV", 0, kMagenta+1);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV", "13 TeV", 0, kRed+1);
    myPlot.AddRatio("multDistUnfoldedKNO", "pp_2TeV", "multDistUnfoldedKNO", "pp_5TeV", "", 0, kGreen+3);
    myPlot.AddRatio("multDistUnfoldedKNO", "pp_7TeV", "multDistUnfoldedKNO", "pp_5TeV", "", 0, kMagenta+1);
    myPlot.AddRatio("multDistUnfoldedKNO", "pp_13TeV", "multDistUnfoldedKNO", "pp_5TeV", "", 0, kRed+1);
    myPlot.SetAxisRange("ratio", 0.45, 1.55);
    myPlot.SetAxisTitle("ratio", "ratio to 5");
    myPlot.SetAxisRange("X", 0, 7);
    myPlot.AddLegendBox(0.7, 0.8);
    myPlot.AddTextBox(0.15, 0.3, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
}
