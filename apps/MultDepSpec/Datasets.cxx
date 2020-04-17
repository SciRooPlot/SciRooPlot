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

void MultDepSpec::DefineDatasetPlots(string& dataSet, PlotManager& plotEnv)
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
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName, "", "");
    myPlot.AddLegend(0.6, 0.7);
    myPlot.AddText(0.4, 0.91, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "sign_covMat_multDistUnfolded";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName, "", "");
    myPlot.AddLegend(0.6, 0.7);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "abs_covMat_multDistUnfolded";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddData(plotName, "", "");
    myPlot.AddLegend(0.6, 0.7);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "edgeContamPt";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logX");
    myPlot.AddData(plotName, "", "");
    myPlot.SetAxisRange("X", 0, 20);
    myPlot.SetAxisTitle("Y", "contamination");
    myPlot.AddLegend(0.3, 0.5);
    myPlot.AddText(0.41, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "edgeContamMult";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName, "", "");
    myPlot.SetAxisRange("X", 0, 20);
    myPlot.SetAxisTitle("Y", "contamination");
    myPlot.SetAxisTitle("X", "test");
    myPlot.AddLegend(0.3, 0.5);
    myPlot.AddText(0.41, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "triggerEff";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData("eventEfficiency", "", "efficiency of selected event to be measured");
    myPlot.AddData(plotName, "", "efficiency including event selection");
    myPlot.AddData("eventLossEff", "", "remaining");
    myPlot.SetAxisRange("Y", 0.4, 1.01);
    myPlot.SetAxisRange("X", 0, 10);
    myPlot.SetAxisTitle("Y", "efficiency");
    myPlot.AddLegend(0.3, 0.5);
    myPlot.AddText(0.41, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "chi2_multDistUnfolded";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddData("chi2_multDistUnfolded", dataSet, "data", kFullCross, kRed+3);
    myPlot.AddData("chi2_multDistUnfoldedMC", dataSet, "mc", kFullCross, kBlue+3);
    myPlot.AddData("chi2_multDistUnfoldedClosureTestFlat", dataSet, "mc flat prior", kFullCross, kGreen+3);
    myPlot.AddLegend(0.6, 0.7);
    myPlot.AddText(0.4, 0.91, datasetLable);
    myPlot.SetAxisRange("Y", 0.1, 1e6);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "chi2_multPtUnfoldedRaw_PtBin";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logY");
    vector<int> ptBins = {2, 3, 5, 7, 11, 13, 15, 17, 20, 30, 40, 50, 51};
    int i = 0;
    for(int ptBin : ptBins)
    {
      myPlot.AddData(plotName + "_" + std::to_string(ptBin), dataSet, GetPtString(ptBin), 0, 0, "hist");
      i++;
    }
    myPlot.AddLegend(0.5, 0.78);
    myPlot.AddText(0.3, 0.92, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "chi2_multPtUnfoldedRaw_MultBin";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logY");
    vector<int> multBins = {2, 6, 9, 11, 16, 21, 31, 41, 51, 61};
    int i = 0;
    for(int multBin : multBins)
    {
      myPlot.AddData(plotName + "_" + std::to_string(multBin), dataSet, "Nch = " + std::to_string(multBin-1), 0, 0, "hist");
      i++;
    }
    myPlot.AddLegend(0.6, 0.7);
    myPlot.AddText(0.4, 0.91, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "particleClosure";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName + "MCtruth", "", "MC truth", kOpenSquare, kGreen+2);
    myPlot.AddData(plotName + "MC", "", "MC unfolded", kOpenCircle, kRed+1);
    myPlot.AddData(plotName, "", "data unfolded", kFullCross, kBlue+3);
    myPlot.AddLegend(0.5, 0.7);
    myPlot.SetAxisRange("Y", 0.9, 1.6);
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisTitle("Y", "integrated / true #it{N}_{ch}");
    myPlot.AddText(0.6, 0.5, "particles per event in // measured spectra // over respective #it{N}_{ch}");
    myPlot.AddText(0.15, 0.91, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "particeContaminationVsNch";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.AddData("particleClosureMCmeasVsNch", "", "prim and sec", kOpenCross, kBlue+1);
    myPlot.AddData("particleClosureMCprimaries", "", "only prim", kFullCross, kBlack+1);
    myPlot.AddRatio("particleClosureMCmeasVsNch", "", "particleClosureMCprimaries", "", "sec contam");
    myPlot.SetAxisTitle("ratio", "all / prim");
    myPlot.SetAxisTitle("Y", "measured over true particles");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.6, 1.2);
    myPlot.SetAxisRange("ratio", 0.95, 1.1);
    myPlot.AddLegend(0.6, 0.6);
    myPlot.AddText(0.15, 0.91, datasetLable);
    myPlot.AddText(0.5, 0.4, "particles per event in // measured spectra // over respective #it{N}_{ch}");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  //
  {
    int ptBin = 28;
    
    { // -----------------------------------------------------------------------
      string plotName = "presentation_PtBin_measuredRawMC";
      Plot myPlot(plotName, dataSet);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddData(string("multPtMeasuredMC_PtBin_") + std::to_string(ptBin), "", "measured", kFullCross, kGreen+3);
      myPlot.AddLegend(0.2, 0.4);
      myPlot.SetAxisRange("Y", 1e-8, 1e-3);
      myPlot.AddText(0.4, 0.91, datasetLable);
      plotEnv.AddPlot(myPlot);
    } // -----------------------------------------------------------------------
    
    { // -----------------------------------------------------------------------
      string plotName = "presentation_PtBin_unfoldedRawMC";
      Plot myPlot(plotName, dataSet);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddData(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", "un-smeared", kFullCircle, kBlue+1);
      myPlot.AddLegend(0.2, 0.4);
      myPlot.SetAxisRange("Y", 1e-8, 1e-3);
      myPlot.AddText(0.4, 0.91, datasetLable);
      plotEnv.AddPlot(myPlot);
    } // -----------------------------------------------------------------------
    
    int multBin = 18;
    
    { // -----------------------------------------------------------------------
      string plotName = "presentation_MultBin_unfoldedRawMC";
      Plot myPlot(plotName, dataSet);
      myPlot.SetDrawingProperties("logY logX");
      //myPlot.AddData(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "unfolded", kFullCircle, kBlue+1);
      myPlot.AddData(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "un-smeared", kFullCircle, kBlue+1);
      myPlot.AddLegend(0.6, 0.8);
      myPlot.SetAxisRange("Y", 1e-12, 2);
      myPlot.SetAxisTitle("X", "#it{p}^{ meas}_{T}");
      myPlot.AddText(0.16, 0.3, datasetLable);
      plotEnv.AddPlot(myPlot);
    } // -----------------------------------------------------------------------
  }
  
  //--------------------------------------------------------------------------
  //---------------------- result plots --------------------------------------
  //--------------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensity";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddData("multDensityUnfolded", dataSet, "", kFullCircle, kBlue+1);
    myPlot.AddData("multDensityUnfolded_Syst", dataSet, "data", kFullCircle, kBlue+1, "boxes");
    myPlot.AddData("multDensityGeneratedMC", dataSet, "mc", kOpenCircle, kBlue+1);
    myPlot.AddRatio("multDensityUnfolded_Syst", "", "multDensityGeneratedMC", "", "");
    myPlot.AddLegend(0.2, 0.4);
    myPlot.SetAxisRange("X", 0, 130);
    myPlot.AddText(0.4, 0.91, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDists";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddData("multDistMeasured", dataSet, "measured");
    myPlot.AddData("multDistUnfolded", dataSet, "", kFullCircle, kBlue+1);
    myPlot.AddData("multDistUnfolded_Syst", dataSet, "unfolded", kFullCircle, kBlue+1, "boxes");
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.AddLegend(0.2, 0.4);
    myPlot.AddText(0.4, 0.91, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistsMC";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logY");
    myPlot.AddData("multDistMeasuredMC", dataSet, "measured");
    myPlot.AddData("multDistUnfoldedMC", dataSet, "unfolded");
    myPlot.AddData("multDistGeneratedMC", dataSet, "generated");
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.AddLegend(0.2, 0.4);
    myPlot.AddText(0.4, 0.91, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasured";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 50.0);
    myPlot.SetAxisRange("Z", 1e-10, 5);
    //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
    myPlot.SetAxisTitle("Z", "norm. yield");
    //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    myPlot.AddText(0.15, 0.8, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasuredRaw";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 50.0);
    //myPlot.SetAxisRange("Z", 1e-10, 5);
    //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T} (GeV/#it{c})");
    //myPlot.SetAxisTitle("Z", "norm. yield");
    myPlot.SetAxisTitle("Z", "raw yield");
    //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    myPlot.AddText(0.15, 0.8, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfolded";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 30.0);
    myPlot.SetAxisRange("Z", 1e-10, 0.5);
    //myPlot.SetAxisTitle("Z", "raw yield");
    //myPlot.SetAxisTitle("Z", "raw yield");abcde
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    //myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T} (GeV/#it{c})");
    //myPlot.SetAxisTitle("Z", "norm. yield");
    myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) [(GeV/#it{c})^{-2}]");
    myPlot.AddText(0.15, 0.5, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedNormalized";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 50.0);
    //myPlot.SetAxisRange("Z", 1e-10, 50);
    //myPlot.SetAxisTitle("Z", "raw yield");
    //myPlot.SetAxisTitle("Z", "raw yield");abcde
    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    //myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T} (GeV/#it{c})");
    //myPlot.SetAxisTitle("Z", "norm. yield");
    myPlot.SetAxisTitle("Z", "1/#it{N}_{evt}(#it{N}_{ch}) 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) [(GeV/#it{c})^{-2}]");
    //myPlot.AddText(0.15, 0.8, datasetLable);
    myPlot.AddText(0.3, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasuredMC";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 50.0);
    myPlot.SetAxisRange("Z", 1e-10, 5);
    //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
    myPlot.SetAxisTitle("Z", "norm. yield");
    //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    myPlot.AddText(0.15, 0.8, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtMeasuredRawMC";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 50.0);
    //myPlot.SetAxisRange("Z", 1e-10, 5);
    //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
    //myPlot.SetAxisTitle("Z", "norm. yield");
    myPlot.SetAxisTitle("Z", "raw yield");
    //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
    myPlot.AddText(0.15, 0.8, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtUnfoldedMC";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.15, 50.0);
    myPlot.SetAxisRange("Z", 1e-10, 5);
    //myPlot.SetAxisTitle("X", "#it{N}^{ true}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
    myPlot.SetAxisTitle("Z", "norm. yield");
    //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
    myPlot.AddText(0.15, 0.8, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtGeneratedMC";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logY");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    //myPlot.SetAxisRange("Y", 0.15, 50.0);
    //myPlot.SetAxisRange("Z", 1e-10, 5);
    //myPlot.SetAxisTitle("X", "#it{N}^{ true}_{ch}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
    myPlot.SetAxisTitle("Z", "norm. yield");
    //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
    myPlot.AddText(0.15, 0.8, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPt";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData("momentUnfolded1", "", "", kFullSquare, kBlue+1, "", multRange);
    myPlot.AddData("momentUnfolded1_Syst", "", "data", kFullSquare, kBlue+1, "boxes", multRange);
    myPlot.AddData("momentGeneratedMC1", "", "MC", kOpenCircle, kBlack, "", multRange);
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegend(0.5, 0.3);
    myPlot.AddText(0.16, 0.92, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData("varianceUnfolded", "", "", kFullSquare, kBlue+1);
    myPlot.AddData("varianceUnfolded_Syst", "", "unfolded data", kFullSquare, kBlue+1, "boxes");
    myPlot.AddData("varianceGeneratedMC", "", "MC", kOpenCircle, kBlack, "");
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0., 0.9);
    myPlot.AddLegend(0.2, 0.6);
    myPlot.AddText(0.16, 0.92, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  //--------------------------------------------------------------------------
  //---------------------- technical plots -----------------------------------
  //--------------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddData(plotName);
    myPlot.SetAxisRange("X", 0, 10);
    myPlot.SetAxisRange("Y", 0, 10);
    myPlot.AddText(0.3, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrixOrig";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    //myPlot.SetAxisRange("Y", 0, multRange);
    myPlot.AddText(0.3, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "unfoldingMatrix";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    //myPlot.SetAxisRange("Y", 0, multRange);
    myPlot.AddText(0.3, 0.3, datasetLable);
    //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    //myPlot.SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
    //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
    //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
    myPlot.SetAxisTitle("Z", "#it{S}(#it{N}_{acc} | #it{N}_{ch})");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddData(plotName);
    //myPlot.SetAxisRange("X", 0, multRange);
    //myPlot.SetAxisRange("Y", 0, multRange);
    myPlot.AddText(0.3, 0.3, datasetLable);
    //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
    //myPlot.SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
    //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
    //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
    myPlot.SetAxisTitle("Z", "#it{S}(#it{N}_{acc} | #it{N}_{ch})");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "ptResolutionMatrix";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logX logY logZ");
    //myPlot.SetAxisRange("X", 0.15, 50);
    //myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.SetAxisTitle("X", "#it{p}^{ meas}_{T}");
    myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
    myPlot.AddText(0.3, 0.3, datasetLable);
    myPlot.AddData(plotName);
    myPlot.SetAxisTitle("Z", "#it{S}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
    //myPlot.SetAxisTitle("Z", "#it{P}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "relPtResoFromCovData";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logX");
    myPlot.SetAxisRange("X", 0.15, 50);
    myPlot.SetAxisRange("Z", 2, 5e7);
    myPlot.SetAxisTitle("Z", "# Tracks");
    myPlot.AddText(0.3, 0.9, datasetLable);
    myPlot.AddData(plotName);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "relPtResoFromCovMC";
    Plot myPlot(plotName, dataSet);
    myPlot.SetDrawingProperties("logZ logX");
    myPlot.SetAxisRange("X", 0.15, 50);
    myPlot.SetAxisTitle("Z", "# Tracks");
    myPlot.AddText(0.3, 0.9, datasetLable);
    myPlot.AddData(plotName);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  //--------------------------------------------------------------------------
  //--------------------- QA histograms --------------------------------------
  //--------------------------------------------------------------------------
  
  // ---------------------- Efficiencies------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "eventEfficiency";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName);
    myPlot.SetAxisRange("Y", 0.5, 1.01);
    myPlot.SetAxisRange("X", 0, 10);
    myPlot.SetAxisTitle("Y", "efficiency");
    myPlot.AddText(0.41, 0.3, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "fakeEventContam";
    Plot myPlot(plotName, dataSet);
    
    myPlot.AddData(plotName);
    myPlot.SetAxisRange("X", 0, 10);
    myPlot.AddText(0.41, 0.92, datasetLable);
    myPlot.SetAxisTitle("Y", "contamination");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "efficiencyMC";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData("primTrackEff", dataSet, "", 0, kBlue);
    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisRange("X", 0.15, 50);
    myPlot.SetAxisRange("Y", 0.3, 0.9);
    myPlot.AddLegend(0.6, 0.4);
    myPlot.AddText(0.15, 0.9, datasetLable);
    myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
    myPlot.SetAxisTitle("Y", "efficiency x acceptance");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secContamMC";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName, dataSet, "", 0, kBlue);
    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisRange("X", 0.15, 50);
    myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
    myPlot.AddText(0.4, 0.9, datasetLable);
    myPlot.SetAxisTitle("Y", "secondary contamination");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secContamMC_mult";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData(plotName, dataSet, "", 0, kBlue);
    //myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisRange("Y", 0.0, 0.08);
    //myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
    myPlot.AddText(0.4, 0.9, datasetLable);
    myPlot.SetAxisTitle("Y", "secondary contamination");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "efficiencyMultPtRaw_MultDep";
    Plot myPlot(plotName, dataSet);
    vector<int> ptBins = {3, 5, 7, 11, 13, 15, 17};
    int i = 0;
    for(int ptBin : ptBins)
    {
      string name = "efficiencyMultPtRaw_PtBin_" + std::to_string(ptBin);
      myPlot.AddData(name, dataSet, GetPtString(ptBin));
      i++;
    }
    myPlot.SetAxisRange("X", 0., multRange);
    myPlot.SetAxisRange("Y", 0.25, 0.8);
    myPlot.AddLegend(0.16, 0.52);
    //myPlot.AddText(0.41, 0.3, datasetLable);
    myPlot.SetAxisTitle("Y", "acceptance x efficiency");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "efficiencyMultPtRaw_PtDep";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData("primTrackEff", dataSet, "integrated");
    vector<int> multBins = {2, 3, 5, 7, 9, 11};
    for(int multBin : multBins)
    {
      string multLable = "#it{N}_{ch} = " + std::to_string(multBin-1);
      string name = "efficiencyMultPtRaw_MultBin_" + std::to_string(multBin);
      myPlot.AddData(name, dataSet, multLable);
    }
    myPlot.SetDrawingProperties("logX");
    myPlot.AddLegend(0.22, 0.52);
    myPlot.AddText(0.41, 0.3, datasetLable);
    myPlot.SetAxisRange("Y", 0.2, 0.9);
    myPlot.SetAxisTitle("Y", "acceptance x efficiency");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secContamMultPtRaw_MultDep";
    Plot myPlot(plotName, dataSet);
    vector<int> ptBins = {3, 5, 7, 11, 13, 15, 17};
    int i = 0;
    for(int ptBin : ptBins)
    {
      string name = "secContamMultPtRaw_PtBin_" + std::to_string(ptBin);
      myPlot.AddData(name, dataSet, GetPtString(ptBin));
      i++;
    }
    myPlot.AddLegend(0.16, 0.92, "");
    myPlot.SetAxisRange("X", 0., multRange);
    myPlot.SetAxisRange("Y", 0, 0.12);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "secondariesMultPtRaw_PtDep";
    Plot myPlot(plotName, dataSet);
    myPlot.AddData("secContamMC", dataSet, "MC truth");
    myPlot.AddData("secContamMultPtRaw_full", dataSet, "inclusive");
    vector<int> multBins = {2, 3, 5, 7, 9, 11};
    for(int multBin : multBins)
    {
      string name = "secContamMultPtRaw_MultBin_" + std::to_string(multBin);
      myPlot.AddData(name, dataSet, std::to_string(multBin));
    }
    myPlot.SetDrawingProperties("logX");
    myPlot.AddLegend(0.7, 0.3);
    myPlot.SetAxisRange("Y", 0, 0.15);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanRelPtReso_DataVsMC";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetDrawingProperties("logX");
    myPlot.SetAxisTitle("Y", "< #sigma(#it{p}_{T}) / #it{p}_{T} >");
    myPlot.SetAxisRange("X", 0.15, 50);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddData("meanRelPtResoFromCovMC", "", "MC");
    myPlot.AddData("meanRelPtResoFromCovData", "", "data");
    myPlot.AddRatio("meanRelPtResoFromCovData", "", "meanRelPtResoFromCovMC");
    myPlot.SetAxisTitle("ratio", "data / MC");
    myPlot.AddLegend(0.4, 0.5, "");
    myPlot.AddText(0.3, 0.9, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  // ---------------------- Closure tests ------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMultDist";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.AddData("multDistGeneratedClosureTest", "", "generated (prior)", kFullSquare, kGray+1);
    myPlot.AddData("multDistUnfoldedClosureTest", "", "unfolded", kOpenCircle);
    myPlot.AddData("multDistMeasuredClosureTest", "", "measured");
    myPlot.AddRatio("multDistUnfoldedClosureTest", "", "multDistGeneratedClosureTest", "", "");
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("X", 0, multRange);
    //myPlot.SetAxisRange("Y", 0.9, 3e5);
    myPlot.AddLegend(0.6, 0.9);
    myPlot.AddText(0.16, 0.2, datasetLable);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMultDistFlat";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.AddData("multDistGeneratedClosureTest", "", "generated", kFullSquare, kGray+1);
    myPlot.AddData("multDistUnfoldedClosureTestFlat", "", "unfolded", kOpenCircle);
    myPlot.AddData("multDistMeasuredClosureTest", "", "measured");
    myPlot.AddData("multDistInitialClosureTestFlat", "", "prior", 0, 0, "hist");
    myPlot.AddRatio("multDistUnfoldedClosureTestFlat", "", "multDistGeneratedClosureTest", "", "");
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("X", 0, multRange);
    //myPlot.SetAxisRange("Y", 0.9, 3e5);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddLegend(0.7, 0.8);
    myPlot.AddText(0.16, 0.2, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestProjectionPt";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.SetDrawingProperties("logX logY");
    myPlot.SetAxisRange("X", 0.15, 50);
    myPlot.AddData("trueParticlesPt", "", "true");
    myPlot.AddData("unfoParticlesPt", "", "unfolded");
    myPlot.AddData("TEST", "", "TEST");
    myPlot.AddData("TESTDATA", "", "TESTDATA");
    myPlot.AddData("TESTDATAMEAS", "", "TESTDATAMEAS");
    myPlot.AddData("TESTMCMEAS", "", "TESTMCMEAS");
    myPlot.AddRatio("unfoParticlesPt", "", "trueParticlesPt");
    //myPlot.SetAxisRange("ratio", 0.9, 1.1);
    myPlot.SetAxisTitle("X", "#it{p}_{T} (GeV/#it{c})");
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.AddLegend(0.7, 0.85);
    myPlot.AddText(0.16, 0.2, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestProjectionMult";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.AddData("trueParticlesMult", "", "true");
    myPlot.AddData("unfoParticlesMult", "", "unfolded");
    myPlot.AddRatio("unfoParticlesMult", "", "trueParticlesMult");
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddLegend(0.7, 0.7);
    myPlot.AddText(0.16, 0.2, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMoment1";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.AddData("momentGeneratedMC1", "", "generated");
    myPlot.AddData("momentUnfoldedMC1", "", "unfolded");
    myPlot.AddData("momentReweightedMC1", "", "re-weighted");
    myPlot.AddRatio("momentUnfoldedMC1", "", "momentGeneratedMC1");
    myPlot.AddRatio("momentReweightedMC1", "", "momentGeneratedMC1");
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.AddLegend(0.6, 0.4);
    myPlot.AddText(0.16, 0.92, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMoment2";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.AddData("momentGeneratedMC2", "", "generated");
    myPlot.AddData("momentUnfoldedMC2", "", "unfolded");
    //myPlot.AddData("momentReweightedMC2", "", "re-weighted");
    myPlot.AddRatio("momentUnfoldedMC2", "", "momentGeneratedMC2");
    //myPlot.AddRatio("momentReweightedMC2", "", "momentGeneratedMC2");
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.2, 1.6);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.AddLegend(0.6, 0.4);
    myPlot.AddText(0.16, 0.92, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "closureTestMoment3";
    Plot myPlot(plotName, dataSet, "default ratio");
    myPlot.SetFigureCategory(categoryClosure);
    myPlot.AddData("momentGeneratedMC3", "", "generated");
    myPlot.AddData("momentUnfoldedMC3", "", "unfolded");
    //myPlot.AddData("momentReweightedMC3", "", "re-weighted");
    myPlot.AddRatio("momentUnfoldedMC3", "", "momentGeneratedMC3");
    //myPlot.AddRatio("momentReweightedMC3", "", "momentGeneratedMC3");
    myPlot.SetAxisRange("X", 0, multRange);
    myPlot.SetAxisRange("Y", 0.1, 7.0);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.SetAxisTitle("ratio", "ratio");
    myPlot.AddLegend(0.7, 0.3);
    myPlot.AddText(0.16, 0.92, datasetLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    for(int ptBin = 2; ptBin < 50; ptBin++){
      //if(!createBinWiseClosureTests) break;
      string plotName = "closureTestPtBin_" + std::to_string(ptBin);
      Plot myPlot(plotName, dataSet, "default ratio");
      myPlot.SetFigureCategory(categoryClosure + "/pT-Bins");
      myPlot.AddData(string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin), "", "", kFullSquare, kBlack);
      myPlot.AddData(string("multPtMeasuredMC_PtBin_") + std::to_string(ptBin), "", "measured", kFullCross, kGreen+3);
      myPlot.AddData(string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin), "", "generated", kFullSquare, kBlack);
      myPlot.AddData(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", "unfolded", kFullCircle, kBlue+1);
      myPlot.AddRatio(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin));
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.AddText(0.6, 0.65, GetPtString(ptBin));
      myPlot.AddLegend(0.7, 0.5);
      myPlot.AddText(0.42, 0.92, datasetLable);
      plotEnv.AddPlot(myPlot);
    }
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    for(int multBin = 2; multBin < 101; multBin++){
      //if(!createBinWiseClosureTests) break;
      string plotName = "closureTestMultBin_" + std::to_string(multBin);
      Plot myPlot(plotName, dataSet, "default ratio");
      myPlot.SetFigureCategory(categoryClosure + "/Nch-Bins");
      myPlot.SetDrawingProperties("logX logY");
      myPlot.AddData(string("multPtGeneratedMC_MultBin_") + std::to_string(multBin), "", "", kFullSquare, kBlack);
      myPlot.AddData(string("multPtMeasuredMC_MultBin_") + std::to_string(multBin), "", "measured", kFullCross, kGreen+3);
      myPlot.AddData(string("multPtGeneratedMC_MultBin_") + std::to_string(multBin), "", "generated", kFullSquare, kBlack);
      myPlot.AddData(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "unfolded", kFullCircle, kBlue+1);
      myPlot.AddRatio(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", string("multPtGeneratedMC_MultBin_") + std::to_string(multBin));
      myPlot.SetAxisRange("ratio", 0.9, 1.1);
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.AddText(0.72, 0.9, "#it{N}_{ch} = " + std::to_string(multBin-1));
      myPlot.AddLegend(0.7, 0.7);
      myPlot.AddText(0.16, 0.2, datasetLable);
      plotEnv.AddPlot(myPlot);
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
      Plot myPlot(plotName, dataSet, "default");
      myPlot.SetDrawingProperties("thick");
      myPlot.AddData(histName + "_RelSyst_total", "", "", 0, kWhite, "hist");
      myPlot.AddData(histName + "_RelSyst_DCAtoVertexZ", "", " 1", 0, colorsSyst[0], "hist");
      myPlot.AddData(histName + "_RelSyst_DCAtoVertexXYPtDep", "", " 2", 0, colorsSyst[1], "hist");
      myPlot.AddData(histName + "_RelSyst_RatioCrossedRowsOverFindableClustersTPC", "", " 3", 0, colorsSyst[2], "hist");
      myPlot.AddData(histName + "_RelSyst_FractionSharedClustersTPC", "", " 4", 0, colorsSyst[3], "hist");
      myPlot.AddData(histName + "_RelSyst_Maxchi2perTPCclu", "", " 5", 0, colorsSyst[4], "hist");
      myPlot.AddData(histName + "_RelSyst_Maxchi2perITSclu", "", " 6", 0, colorsSyst[5], "hist");
      myPlot.AddData(histName + "_RelSyst_ClusterReqITS", "", " 7", 0, colorsSyst[6], "hist");
      myPlot.AddData(histName + "_RelSyst_DeadzoneWidth", "", " 8", 0, colorsSyst[7], "hist");
      myPlot.AddData(histName + "_RelSyst_Ncrnclgeomlength", "", " 9", 0, colorsSyst[8], "hist");
      myPlot.AddData(histName + "_RelSyst_MaxChi2TPCConstrained", "", "10", 0, colorsSyst[9], "hist");
      myPlot.AddData(histName + "_RelSyst_nonClosure", "", "11", 0, colorsSyst[11], "hist");
      myPlot.AddData(histName + "_RelSyst_total", "", "total", 0, kBlack, "hist");
      //myPlot.AddData(histName + "_RelSyst_ZVertexSelection", "", "11", 0, colorsSyst[10], "hist");
      myPlot.AddText(0.39, 0.91, datasetLable);
      //        myPlot.AddLegend(0.15, 0.8, "", 2);
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("X", 0, 100);
      //myPlot.SetAxisRange("Y", 0, 0.06);
      plotEnv.AddPlot(myPlot);
    }
  } // -----------------------------------------------------------------------
}
