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

void MultDepSpec::DefineDatasetPlots(string& dataSet, PlotManager& plotManager)
{
  string erg = "0 TeV";
  string colSys = "?-?";
  string plotName;
  int multRange = 60;
  
  if(dataSet.find("pp") != string::npos) colSys = pp;
  if(dataSet.find("pPb") != string::npos) {colSys = pPb;}// multRange = range::pPb_5;}
  if(dataSet.find("PbPb") != string::npos) {colSys = PbPb;}// multRange = range::PbPb_5;}
  if(dataSet.find("XeXe") != string::npos) {colSys = XeXe; erg = erg544TeV_NN;}// multRange = range::XeXe_5;}
  
  if(dataSet.find("pp_2TeV") != string::npos) {erg = erg2TeV;}// multRange = range::pp_2;}
  if(dataSet.find("pp_7TeV") != string::npos) erg = erg7TeV;
  if(dataSet.find("pp_13TeV") != string::npos) {erg = erg13TeV;}// multRange = range::pp_13;}
  if(dataSet.find("pp_5TeV") != string::npos) erg = erg5TeV;
  if(dataSet.find("pPb_5TeV") != string::npos) erg = erg5TeV_NN;
  if(dataSet.find("pPb_8TeV") != string::npos) erg = erg8TeV_NN;
  if(dataSet.find("PbPb_5TeV") != string::npos) erg = erg5TeV_NN;
  
  const string datasetLable = alice + newLine + chargedParticles + ", " + colSys + ", " + erg + newLine + eta08 + ", " + ptRange;
  const string datasetLablePrel = alicePrel + newLine + chargedParticles + ", " + colSys + ", " + erg + newLine + eta08 + ", " + ptRange;
  
  
  
  
  const string categoryQA = "QA-Plots";
  const string categoryClosure = categoryQA + "/MC_Closure_Tests";
  const string categorySystematics = "Systematic_Uncertainties";
  
  { // -----------------------------------------------------------------------
    string plotName = "covMat_multDistUnfolded";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName, "", "");
    plot[1].AddLegend(0.6, 0.7);
    plot[1].AddText(0.4, 0.91, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "sign_covMat_multDistUnfolded";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName, "", "");
    plot[1].AddLegend(0.6, 0.7);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "abs_covMat_multDistUnfolded";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData(plotName, "", "");
    plot[1].AddLegend(0.6, 0.7);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "edgeContamPt";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logX");
    plot[1].AddData(plotName, "", "");
    plot[1].SetAxisRange("X", 0, 20);
    plot[1].SetAxisTitle("Y", "contamination");
    plot[1].AddLegend(0.3, 0.5);
    plot[1].AddText(0.41, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "edgeContamMult";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName, "", "");
    plot[1].SetAxisRange("X", 0, 20);
    plot[1].SetAxisTitle("Y", "contamination");
    plot[1].SetAxisTitle("X", "test");
    plot[1].AddLegend(0.3, 0.5);
    plot[1].AddText(0.41, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "triggerEff";
    Plot plot(plotName, dataSet);
    plot[1].AddData("eventEfficiency", "", "efficiency of selected event to be measured");
    plot[1].AddData(plotName, "", "efficiency including event selection");
    plot[1].AddData("eventLossEff", "", "remaining");
    plot[1].SetAxisRange("Y", 0.4, 1.01);
    plot[1].SetAxisRange("X", 0, 10);
    plot[1].SetAxisTitle("Y", "efficiency");
    plot[1].AddLegend(0.3, 0.5);
    plot[1].AddText(0.41, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "chi2_multDistUnfolded";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logY");
    plot[1].AddData("chi2_multDistUnfolded", dataSet, "data", kFullCross, kRed+3);
    plot[1].AddData("chi2_multDistUnfoldedMC", dataSet, "mc", kFullCross, kBlue+3);
    plot[1].AddData("chi2_multDistUnfoldedClosureTestFlat", dataSet, "mc flat prior", kFullCross, kGreen+3);
    plot[1].AddLegend(0.6, 0.7);
    plot[1].AddText(0.4, 0.91, datasetLable);
    plot[1].SetAxisRange("Y", 0.1, 1e6);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "chi2_multPtUnfoldedRaw_PtBin";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logY");
    vector<int> ptBins = {2, 3, 5, 7, 11, 13, 15, 17, 20, 30, 40, 50, 51};
    int i = 0;
    for(int ptBin : ptBins)
    {
      plot[1].AddData(plotName + "_" + std::to_string(ptBin), dataSet, GetPtString(ptBin), 0, 0, "hist");
      i++;
    }
    plot[1].AddLegend(0.5, 0.78);
    plot[1].AddText(0.3, 0.92, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "chi2_multPtUnfoldedRaw_MultBin";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logY");
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 31, 41, 51, 61};
    int i = 0;
    for(int multBin : multBins)
    {
      plot[1].AddData(plotName + "_" + std::to_string(multBin), dataSet, "Nch = " + std::to_string(multBin-1), 0, 0, "hist");
      i++;
    }
    plot[1].AddLegend(0.6, 0.7);
    plot[1].AddText(0.4, 0.91, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "particleClosure";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName + "MCtruth", "", "MC truth", kOpenSquare, kGreen+2);
    plot[1].AddData(plotName + "MC", "", "MC unfolded", kOpenCircle, kRed+1);
    plot[1].AddData(plotName, "", "data unfolded", kFullCross, kBlue+3);
    plot[1].AddLegend(0.5, 0.7);
    plot[1].SetAxisRange("Y", 0.9, 1.6);
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisTitle("Y", "integrated / true #it{N}_{ch}");
    plot[1].AddText(0.6, 0.5, "particles per event in // measured spectra // over respective #it{N}_{ch}");
    plot[1].AddText(0.15, 0.91, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "particeContaminationVsNch";
    Plot plot(plotName, dataSet, "default ratio");
    plot[1].AddData("particleClosureMCmeasVsNch", "", "prim and sec", kOpenCross, kBlue+1);
    plot[1].AddData("particleClosureMCprimaries", "", "only prim", kFullCross, kBlack+1);
    plot[2].AddRatio("particleClosureMCmeasVsNch", "", "particleClosureMCprimaries", "", "sec contam");
    plot[2].SetAxisTitle("Y", "all / prim");
    plot[1].SetAxisTitle("Y", "measured over true particles");
    plot[1].SetAxisRange("X", 0, 60);
    plot[1].SetAxisRange("Y", 0.6, 1.2);
    plot[2].SetAxisRange("Y", 0.95, 1.1);
    plot[1].AddLegend(0.6, 0.6);
    plot[1].AddText(0.15, 0.91, datasetLable);
    plot[1].AddText(0.5, 0.4, "particles per event in // measured spectra // over respective #it{N}_{ch}");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  //
  {
    int ptBin = 28;
    
    { // -----------------------------------------------------------------------
      string plotName = "presentation_PtBin_measuredRawMC";
      Plot plot(plotName, dataSet);
      plot[1].SetPadOptions("logY");
      plot[1].AddData(string("multPtMeasuredMC_PtBin_") + std::to_string(ptBin), "", "measured", kFullCross, kGreen+3);
      plot[1].AddLegend(0.2, 0.4);
      plot[1].SetAxisRange("Y", 1e-8, 1e-3);
      plot[1].AddText(0.4, 0.91, datasetLable);
      plotManager.AddPlot(plot);
    } // -----------------------------------------------------------------------
    
    { // -----------------------------------------------------------------------
      string plotName = "presentation_PtBin_unfoldedRawMC";
      Plot plot(plotName, dataSet);
      plot[1].SetPadOptions("logY");
      plot[1].AddData(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", "un-smeared", kFullCircle, kBlue+1);
      plot[1].AddLegend(0.2, 0.4);
      plot[1].SetAxisRange("Y", 1e-8, 1e-3);
      plot[1].AddText(0.4, 0.91, datasetLable);
      plotManager.AddPlot(plot);
    } // -----------------------------------------------------------------------
    
    int multBin = 18;
    
    { // -----------------------------------------------------------------------
      string plotName = "presentation_MultBin_unfoldedRawMC";
      Plot plot(plotName, dataSet);
      plot[1].SetPadOptions("logY logX");
      //plot[1].AddData(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "unfolded", kFullCircle, kBlue+1);
      plot[1].AddData(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "un-smeared", kFullCircle, kBlue+1);
      plot[1].AddLegend(0.6, 0.8);
      plot[1].SetAxisRange("Y", 1e-12, 2);
      plot[1].SetAxisTitle("X", "#it{p}^{ meas}_{T}");
      plot[1].AddText(0.16, 0.3, datasetLable);
      plotManager.AddPlot(plot);
    } // -----------------------------------------------------------------------
  }
  
  //--------------------------------------------------------------------------
  //---------------------- result plots --------------------------------------
  //--------------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensity";
    Plot plot(plotName, dataSet, "default ratio");
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDensityUnfolded", dataSet, "", kFullCircle, kBlue+1);
    plot[1].AddData("multDensityUnfolded_Syst", dataSet, "data", kFullCircle, kBlue+1, "boxes");
    plot[1].AddData("multDensityGeneratedMC", dataSet, "mc", kOpenCircle, kBlue+1);
    plot[2].AddRatio("multDensityUnfolded_Syst", "", "multDensityGeneratedMC", "", "");
    plot[1].AddLegend(0.2, 0.4);
    plot[1].SetAxisRange("X", 0, 130);
    plot[1].AddText(0.4, 0.91, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDists";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistMeasured", dataSet, "measured");
    plot[1].AddData("multDistUnfolded", dataSet, "", kFullCircle, kBlue+1);
    plot[1].AddData("multDistUnfolded_Syst", dataSet, "unfolded", kFullCircle, kBlue+1, "boxes");
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].AddLegend(0.2, 0.4);
    plot[1].AddText(0.4, 0.91, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistsMC";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logY");
    plot[1].AddData("multDistMeasuredMC", dataSet, "measured");
    plot[1].AddData("multDistUnfoldedMC", dataSet, "unfolded");
    plot[1].AddData("multDistGeneratedMC", dataSet, "generated");
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].AddLegend(0.2, 0.4);
    plot[1].AddText(0.4, 0.91, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasured";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 50.0);
    plot[1].SetAxisRange("Z", 1e-10, 5);
    //plot[1].SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
    plot[1].SetAxisTitle("Z", "norm. yield");
    //plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    plot[1].AddText(0.15, 0.8, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasuredRaw";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 50.0);
    //plot[1].SetAxisRange("Z", 1e-10, 5);
    //plot[1].SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ meas}_{T} (GeV/#it{c})");
    //plot[1].SetAxisTitle("Z", "norm. yield");
    plot[1].SetAxisTitle("Z", "raw yield");
    //plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    plot[1].AddText(0.15, 0.8, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfolded";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 30.0);
    plot[1].SetAxisRange("Z", 1e-10, 0.5);
    //plot[1].SetAxisTitle("Z", "raw yield");
    //plot[1].SetAxisTitle("Z", "raw yield");abcde
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    //plot[1].SetAxisTitle("Y", "#it{p}^{ true}_{T} (GeV/#it{c})");
    //plot[1].SetAxisTitle("Z", "norm. yield");
    plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) [(GeV/#it{c})^{-2}]");
    plot[1].AddText(0.15, 0.5, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedNormalized";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 50.0);
    //plot[1].SetAxisRange("Z", 1e-10, 50);
    //plot[1].SetAxisTitle("Z", "raw yield");
    //plot[1].SetAxisTitle("Z", "raw yield");abcde
    plot[1].SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    plot[1].SetAxisTitle("X", "#it{N}_{ch}");
    //plot[1].SetAxisTitle("Y", "#it{p}^{ true}_{T} (GeV/#it{c})");
    //plot[1].SetAxisTitle("Z", "norm. yield");
    plot[1].SetAxisTitle("Z", "1/#it{N}_{evt}(#it{N}_{ch}) 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) [(GeV/#it{c})^{-2}]");
    //plot[1].AddText(0.15, 0.8, datasetLable);
    plot[1].AddText(0.3, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasuredMC";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 50.0);
    plot[1].SetAxisRange("Z", 1e-10, 5);
    //plot[1].SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
    plot[1].SetAxisTitle("Z", "norm. yield");
    //plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    plot[1].AddText(0.15, 0.8, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasuredRawMC";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 50.0);
    //plot[1].SetAxisRange("Z", 1e-10, 5);
    //plot[1].SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
    //plot[1].SetAxisTitle("Z", "norm. yield");
    plot[1].SetAxisTitle("Z", "raw yield");
    //plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    plot[1].AddText(0.15, 0.8, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedMC";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.15, 50.0);
    plot[1].SetAxisRange("Z", 1e-10, 5);
    //plot[1].SetAxisTitle("X", "#it{N}^{ true}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ true}_{T}");
    plot[1].SetAxisTitle("Z", "norm. yield");
    //plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
    plot[1].AddText(0.15, 0.8, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtGeneratedMC";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logY");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    //plot[1].SetAxisRange("Y", 0.15, 50.0);
    //plot[1].SetAxisRange("Z", 1e-10, 5);
    //plot[1].SetAxisTitle("X", "#it{N}^{ true}_{ch}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ true}_{T}");
    plot[1].SetAxisTitle("Z", "norm. yield");
    //plot[1].SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
    plot[1].AddText(0.15, 0.8, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPt";
    Plot plot(plotName, dataSet);
    plot[1].AddData("momentUnfolded1", "", "", kFullSquare, kBlue+1, "", multRange);
    plot[1].AddData("momentUnfolded1_Syst", "", "data", kFullSquare, kBlue+1, "boxes", multRange);
    plot[1].AddData("momentGeneratedMC1", "", "MC", kOpenCircle, kBlack, "", multRange);
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[1].AddLegend(0.5, 0.3);
    plot[1].AddText(0.16, 0.92, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance";
    Plot plot(plotName, dataSet);
    plot[1].AddData("varianceUnfolded", "", "", kFullSquare, kBlue+1);
    plot[1].AddData("varianceUnfolded_Syst", "", "unfolded data", kFullSquare, kBlue+1, "boxes");
    plot[1].AddData("varianceGeneratedMC", "", "MC", kOpenCircle, kBlack, "");
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0., 0.9);
    plot[1].AddLegend(0.2, 0.6);
    plot[1].AddText(0.16, 0.92, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  //--------------------------------------------------------------------------
  //---------------------- technical plots -----------------------------------
  //--------------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData(plotName);
    plot[1].SetAxisRange("X", 0, 10);
    plot[1].SetAxisRange("Y", 0, 10);
    plot[1].AddText(0.3, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrixOrig";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    //plot[1].SetAxisRange("Y", 0, multRange);
    plot[1].AddText(0.3, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "unfoldingMatrix";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    //plot[1].SetAxisRange("Y", 0, multRange);
    plot[1].AddText(0.3, 0.3, datasetLable);
    //plot[1].SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    //plot[1].SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
    //plot[1].SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
    //plot[1].SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
    plot[1].SetAxisTitle("Z", "#it{S}(#it{N}_{acc} | #it{N}_{ch})");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ");
    plot[1].AddData(plotName);
    //plot[1].SetAxisRange("X", 0, multRange);
    //plot[1].SetAxisRange("Y", 0, multRange);
    plot[1].AddText(0.3, 0.3, datasetLable);
    //plot[1].SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    //plot[1].SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
    //plot[1].SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
    //plot[1].SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
    plot[1].SetAxisTitle("Z", "#it{S}(#it{N}_{acc} | #it{N}_{ch})");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "ptResolutionMatrix";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logX logY logZ");
    //plot[1].SetAxisRange("X", 0.15, 50);
    //plot[1].SetAxisRange("Y", 0.15, 50);
    plot[1].SetAxisTitle("X", "#it{p}^{ meas}_{T}");
    plot[1].SetAxisTitle("Y", "#it{p}^{ true}_{T}");
    plot[1].AddText(0.3, 0.3, datasetLable);
    plot[1].AddData(plotName);
    plot[1].SetAxisTitle("Z", "#it{S}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
    //plot[1].SetAxisTitle("Z", "#it{P}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "relPtResoFromCovData";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logX");
    plot[1].SetAxisRange("X", 0.15, 50);
    plot[1].SetAxisRange("Z", 2, 5e7);
    plot[1].SetAxisTitle("Z", "# Tracks");
    plot[1].AddText(0.3, 0.9, datasetLable);
    plot[1].AddData(plotName);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "relPtResoFromCovMC";
    Plot plot(plotName, dataSet);
    plot[1].SetPadOptions("logZ logX");
    plot[1].SetAxisRange("X", 0.15, 50);
    plot[1].SetAxisTitle("Z", "# Tracks");
    plot[1].AddText(0.3, 0.9, datasetLable);
    plot[1].AddData(plotName);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  //--------------------------------------------------------------------------
  //--------------------- QA histograms --------------------------------------
  //--------------------------------------------------------------------------
  
  // ---------------------- Efficiencies------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "eventEfficiency";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName);
    plot[1].SetAxisRange("Y", 0.5, 1.01);
    plot[1].SetAxisRange("X", 0, 10);
    plot[1].SetAxisTitle("Y", "efficiency");
    plot[1].AddText(0.41, 0.3, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "fakeEventContam";
    Plot plot(plotName, dataSet);
    
    plot[1].AddData(plotName);
    plot[1].SetAxisRange("X", 0, 10);
    plot[1].AddText(0.41, 0.92, datasetLable);
    plot[1].SetAxisTitle("Y", "contamination");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "efficiencyMC";
    Plot plot(plotName, dataSet);
    plot[1].AddData("primTrackEff", dataSet, "", 0, kBlue);
    plot[1].SetPadOptions("logX");
    plot[1].SetAxisRange("X", 0.15, 50);
    plot[1].SetAxisRange("Y", 0.3, 0.9);
    plot[1].AddLegend(0.6, 0.4);
    plot[1].AddText(0.15, 0.9, datasetLable);
    plot[1].SetAxisTitle("X", "#it{p}^{ true}_{T}");
    plot[1].SetAxisTitle("Y", "efficiency x acceptance");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secContamMC";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName, dataSet, "", 0, kBlue);
    plot[1].SetPadOptions("logX");
    plot[1].SetAxisRange("X", 0.15, 50);
    plot[1].SetAxisTitle("X", "#it{p}^{ true}_{T}");
    plot[1].AddText(0.4, 0.9, datasetLable);
    plot[1].SetAxisTitle("Y", "secondary contamination");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secContamMC_mult";
    Plot plot(plotName, dataSet);
    plot[1].AddData(plotName, dataSet, "", 0, kBlue);
    //plot[1].SetPadOptions("logX");
    plot[1].SetAxisRange("Y", 0.0, 0.08);
    //plot[1].SetAxisTitle("X", "#it{p}^{ true}_{T}");
    plot[1].AddText(0.4, 0.9, datasetLable);
    plot[1].SetAxisTitle("Y", "secondary contamination");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "efficiencyMultPtRaw_MultDep";
    Plot plot(plotName, dataSet);
    vector<int> ptBins = {3, 5, 7, 11, 13, 15, 17};
    int i = 0;
    for(int ptBin : ptBins)
    {
      string name = "efficiencyMultPtRaw_PtBin_" + std::to_string(ptBin);
      plot[1].AddData(name, dataSet, GetPtString(ptBin));
      i++;
    }
    plot[1].SetAxisRange("X", 0., multRange);
    plot[1].SetAxisRange("Y", 0.25, 0.8);
    plot[1].AddLegend(0.16, 0.52);
    //plot[1].AddText(0.41, 0.3, datasetLable);
    plot[1].SetAxisTitle("Y", "acceptance x efficiency");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "efficiencyMultPtRaw_PtDep";
    Plot plot(plotName, dataSet);
    plot[1].AddData("primTrackEff", dataSet, "integrated");
    vector<int> multBins = {2, 3, 5, 7, 9, 11};
    for(int multBin : multBins)
    {
      string multLable = "#it{N}_{ch} = " + std::to_string(multBin-1);
      string name = "efficiencyMultPtRaw_MultBin_" + std::to_string(multBin);
      plot[1].AddData(name, dataSet, multLable);
    }
    plot[1].SetPadOptions("logX");
    plot[1].AddLegend(0.22, 0.52);
    plot[1].AddText(0.41, 0.3, datasetLable);
    plot[1].SetAxisRange("Y", 0.2, 0.9);
    plot[1].SetAxisTitle("Y", "acceptance x efficiency");
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secContamMultPtRaw_MultDep";
    Plot plot(plotName, dataSet);
    vector<int> ptBins = {3, 5, 7, 11, 13, 15, 17};
    int i = 0;
    for(int ptBin : ptBins)
    {
      string name = "secContamMultPtRaw_PtBin_" + std::to_string(ptBin);
      plot[1].AddData(name, dataSet, GetPtString(ptBin));
      i++;
    }
    plot[1].AddLegend(0.16, 0.92, "");
    plot[1].SetAxisRange("X", 0., multRange);
    plot[1].SetAxisRange("Y", 0, 0.12);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secondariesMultPtRaw_PtDep";
    Plot plot(plotName, dataSet);
    plot[1].AddData("secContamMC", dataSet, "MC truth");
    plot[1].AddData("secContamMultPtRaw_full", dataSet, "inclusive");
    vector<int> multBins = {2, 3, 5, 7, 9, 11};
    for(int multBin : multBins)
    {
      string name = "secContamMultPtRaw_MultBin_" + std::to_string(multBin);
      plot[1].AddData(name, dataSet, std::to_string(multBin));
    }
    plot[1].SetPadOptions("logX");
    plot[1].AddLegend(0.7, 0.3);
    plot[1].SetAxisRange("Y", 0, 0.15);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanRelPtReso_DataVsMC";
    Plot plot(plotName, dataSet, "default ratio");
    plot[1].SetPadOptions("logX");
    plot[1].SetAxisTitle("Y", "< #sigma(#it{p}_{T}) / #it{p}_{T} >");
    plot[1].SetAxisRange("X", 0.15, 50);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[1].AddData("meanRelPtResoFromCovMC", "", "MC");
    plot[1].AddData("meanRelPtResoFromCovData", "", "data");
    plot[2].AddRatio("meanRelPtResoFromCovData", "", "meanRelPtResoFromCovMC");
    plot[2].SetAxisTitle("Y", "data / MC");
    plot[1].AddLegend(0.4, 0.5, "");
    plot[1].AddText(0.3, 0.9, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  // ---------------------- Closure tests ------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMultDist";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].AddData("multDistGeneratedClosureTest", "", "generated (prior)", kFullSquare, kGray+1);
    plot[1].AddData("multDistUnfoldedClosureTest", "", "unfolded", kOpenCircle);
    plot[1].AddData("multDistMeasuredClosureTest", "", "measured");
    plot[2].AddRatio("multDistUnfoldedClosureTest", "", "multDistGeneratedClosureTest", "", "");
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("X", 0, multRange);
    //plot[1].SetAxisRange("Y", 0.9, 3e5);
    plot[1].AddLegend(0.6, 0.9);
    plot[1].AddText(0.16, 0.2, datasetLable);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMultDistFlat";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].AddData("multDistGeneratedClosureTest", "", "generated", kFullSquare, kGray+1);
    plot[1].AddData("multDistUnfoldedClosureTestFlat", "", "unfolded", kOpenCircle);
    plot[1].AddData("multDistMeasuredClosureTest", "", "measured");
    plot[1].AddData("multDistInitialClosureTestFlat", "", "prior", 0, 0, "hist");
    plot[2].AddRatio("multDistUnfoldedClosureTestFlat", "", "multDistGeneratedClosureTest", "", "");
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].SetPadOptions("logY");
    plot[1].SetAxisRange("X", 0, multRange);
    //plot[1].SetAxisRange("Y", 0.9, 3e5);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[1].AddLegend(0.7, 0.8);
    plot[1].AddText(0.16, 0.2, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestProjectionPt";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].SetPadOptions("logX logY");
    plot[1].SetAxisRange("X", 0.15, 50);
    plot[1].AddData("trueParticlesPt", "", "true");
    plot[1].AddData("unfoParticlesPt", "", "unfolded");
    plot[1].AddData("TEST", "", "TEST");
    plot[1].AddData("TESTDATA", "", "TESTDATA");
    plot[1].AddData("TESTDATAMEAS", "", "TESTDATAMEAS");
    plot[1].AddData("TESTMCMEAS", "", "TESTMCMEAS");
    plot[2].AddRatio("unfoParticlesPt", "", "trueParticlesPt");
    //plot[2].SetAxisRange("Y", 0.9, 1.1);
    plot[1].SetAxisTitle("X", "#it{p}_{T} (GeV/#it{c})");
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].AddLegend(0.7, 0.85);
    plot[1].AddText(0.16, 0.2, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestProjectionMult";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].AddData("trueParticlesMult", "", "true");
    plot[1].AddData("unfoParticlesMult", "", "unfolded");
    plot[2].AddRatio("unfoParticlesMult", "", "trueParticlesMult");
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].SetPadOptions("logY");
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[1].AddLegend(0.7, 0.7);
    plot[1].AddText(0.16, 0.2, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMoment1";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].AddData("momentGeneratedMC1", "", "generated");
    plot[1].AddData("momentUnfoldedMC1", "", "unfolded");
    plot[1].AddData("momentReweightedMC1", "", "re-weighted");
    plot[2].AddRatio("momentUnfoldedMC1", "", "momentGeneratedMC1");
    plot[2].AddRatio("momentReweightedMC1", "", "momentGeneratedMC1");
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.45, 0.85);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].AddLegend(0.6, 0.4);
    plot[1].AddText(0.16, 0.92, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMoment2";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].AddData("momentGeneratedMC2", "", "generated");
    plot[1].AddData("momentUnfoldedMC2", "", "unfolded");
    //plot[1].AddData("momentReweightedMC2", "", "re-weighted");
    plot[2].AddRatio("momentUnfoldedMC2", "", "momentGeneratedMC2");
    //plot[2].AddRatio("momentReweightedMC2", "", "momentGeneratedMC2");
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.2, 1.6);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].AddLegend(0.6, 0.4);
    plot[1].AddText(0.16, 0.92, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMoment3";
    Plot plot(plotName, dataSet, "default ratio");
    plot.SetFigureCategory(categoryClosure);
    plot[1].AddData("momentGeneratedMC3", "", "generated");
    plot[1].AddData("momentUnfoldedMC3", "", "unfolded");
    //plot[1].AddData("momentReweightedMC3", "", "re-weighted");
    plot[2].AddRatio("momentUnfoldedMC3", "", "momentGeneratedMC3");
    //plot[2].AddRatio("momentReweightedMC3", "", "momentGeneratedMC3");
    plot[1].SetAxisRange("X", 0, multRange);
    plot[1].SetAxisRange("Y", 0.1, 7.0);
    plot[2].SetAxisRange("Y", 0.95, 1.05);
    plot[2].SetAxisTitle("Y", "ratio");
    plot[1].AddLegend(0.7, 0.3);
    plot[1].AddText(0.16, 0.92, datasetLable);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    for(int ptBin = 2; ptBin < 50; ptBin++){
      //if(!createBinWiseClosureTests) break;
      string plotName = "closureTestPtBin_" + std::to_string(ptBin);
      Plot plot(plotName, dataSet, "default ratio");
      plot.SetFigureCategory(categoryClosure + "/pT-Bins");
      plot[1].AddData(string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin), "", "", kFullSquare, kBlack);
      plot[1].AddData(string("multPtMeasuredMC_PtBin_") + std::to_string(ptBin), "", "measured", kFullCross, kGreen+3);
      plot[1].AddData(string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin), "", "generated", kFullSquare, kBlack);
      plot[1].AddData(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", "unfolded", kFullCircle, kBlue+1);
      plot[2].AddRatio(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin));
      plot[1].SetAxisRange("X", 0, multRange);
      plot[2].SetAxisRange("Y", 0.95, 1.05);
      plot[2].SetAxisTitle("Y", "ratio");
      plot[1].AddText(0.6, 0.65, GetPtString(ptBin));
      plot[1].AddLegend(0.7, 0.5);
      plot[1].AddText(0.42, 0.92, datasetLable);
      plotManager.AddPlot(plot);
    }
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    for(int multBin = 2; multBin < 101; multBin++){
      //if(!createBinWiseClosureTests) break;
      string plotName = "closureTestMultBin_" + std::to_string(multBin);
      Plot plot(plotName, dataSet, "default ratio");
      plot.SetFigureCategory(categoryClosure + "/Nch-Bins");
      plot[1].SetPadOptions("logX logY");
      plot[1].AddData(string("multPtGeneratedMC_MultBin_") + std::to_string(multBin), "", "", kFullSquare, kBlack);
      plot[1].AddData(string("multPtMeasuredMC_MultBin_") + std::to_string(multBin), "", "measured", kFullCross, kGreen+3);
      plot[1].AddData(string("multPtGeneratedMC_MultBin_") + std::to_string(multBin), "", "generated", kFullSquare, kBlack);
      plot[1].AddData(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "unfolded", kFullCircle, kBlue+1);
      plot[2].AddRatio(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", string("multPtGeneratedMC_MultBin_") + std::to_string(multBin));
      plot[2].SetAxisRange("Y", 0.9, 1.1);
      plot[2].SetAxisTitle("Y", "ratio");
      plot[1].AddText(0.72, 0.9, "#it{N}_{ch} = " + std::to_string(multBin-1));
      plot[1].AddLegend(0.7, 0.7);
      plot[1].AddText(0.16, 0.2, datasetLable);
      plotManager.AddPlot(plot);
    }
  } // -----------------------------------------------------------------------
  
  //--------------------------------------------------------------------------
  //----------------------- Systematics --------------------------------------
  //--------------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    vector<string> histNames = {"multDistUnfolded", "momentUnfolded1", "varianceUnfolded"};//, "momentUnfolded2", "momentUnfolded3", "momentReweighted1", "momentReweighted2", "momentReweighted3"};
    Short_t colorsSyst[14] = {kPink+8, kGreen+2, 28, kOrange+2 ,8 , kCyan-6, kMagenta+3, kRed, kBlue ,15, kGreen+4, kMagenta, 17, 12};
    for(string& histName : histNames)
    {
      string plotName = histName + "_SystContrib";
      Plot plot(plotName, dataSet, "default");
      plot[1].SetPadOptions("thick");
      plot[1].AddData(histName + "_RelSyst_total", "", "", 0, kWhite, "hist");
      plot[1].AddData(histName + "_RelSyst_DCAtoVertexZ", "", " 1", 0, colorsSyst[0], "hist");
      plot[1].AddData(histName + "_RelSyst_DCAtoVertexXYPtDep", "", " 2", 0, colorsSyst[1], "hist");
      plot[1].AddData(histName + "_RelSyst_RatioCrossedRowsOverFindableClustersTPC", "", " 3", 0, colorsSyst[2], "hist");
      plot[1].AddData(histName + "_RelSyst_FractionSharedClustersTPC", "", " 4", 0, colorsSyst[3], "hist");
      plot[1].AddData(histName + "_RelSyst_Maxchi2perTPCclu", "", " 5", 0, colorsSyst[4], "hist");
      plot[1].AddData(histName + "_RelSyst_Maxchi2perITSclu", "", " 6", 0, colorsSyst[5], "hist");
      plot[1].AddData(histName + "_RelSyst_ClusterReqITS", "", " 7", 0, colorsSyst[6], "hist");
      plot[1].AddData(histName + "_RelSyst_DeadzoneWidth", "", " 8", 0, colorsSyst[7], "hist");
      plot[1].AddData(histName + "_RelSyst_Ncrnclgeomlength", "", " 9", 0, colorsSyst[8], "hist");
      plot[1].AddData(histName + "_RelSyst_MaxChi2TPCConstrained", "", "10", 0, colorsSyst[9], "hist");
      plot[1].AddData(histName + "_RelSyst_nonClosure", "", "11", 0, colorsSyst[11], "hist");
      plot[1].AddData(histName + "_RelSyst_total", "", "total", 0, kBlack, "hist");
      //plot[1].AddData(histName + "_RelSyst_ZVertexSelection", "", "11", 0, colorsSyst[10], "hist");
      plot[1].AddText(0.39, 0.91, datasetLable);
      //        plot[1].AddLegend(0.15, 0.8, "", 2);
      plot[1].SetAxisRange("X", 0, multRange);
      plot[1].SetAxisRange("X", 0, 100);
      //plot[1].SetAxisRange("Y", 0, 0.06);
      plotManager.AddPlot(plot);
    }
  } // -----------------------------------------------------------------------
}
