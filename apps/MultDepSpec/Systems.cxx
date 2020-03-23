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
  vector<string> centrality = {"0-5%","5-10%","10-20%","20-30%","30-40%","40-50%","50-60%","60-70%","70-80%"};
  vector<double> nchCent = {2869.49, 2342.32, 1740.05, 1156.23, 750.512, 463.796, 265.249, 138.504, 64.0346};
  vector<double> nchCentErrors = {82.7027, 70.8452, 45.7133, 29.4205, 21.9877, 17.5017, 11.5956, 8.3419, 4.84894};
  
  bool drawcent = false;
  
  
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
    string plotName = "meanPtFullRangeLin";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
    myPlot.AddHisto("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    //myPlot.AddLegendBox(0.2, 0.8);
    myPlot.SetAxisRange("Y", 0.69, 0.74);
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "meanPtFullRange";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetFigureCategory("mostImportantPlots");
    myPlot.SetDrawingProperties("logX");
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    
    //p-p
    //myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    //myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    //myPlot.AddHisto("meanPt_pp_EPOS-LHC_5.02TeV", "Simulations", "", kOpenSquare, kBlue+1, "", 60);
    
    //p-Pb
    myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 150);
    myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 150);
    myPlot.AddHisto("meanPt_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCircle, kMagenta+1, "", 150);
    myPlot.AddGraph("meanPt_pPb_EPOS_5.02TeV", "Simulations", "", kOpenSquare, kMagenta+1, "", 150);
    
    //myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "", kOpenCircle, kMagenta+1, "", 110);//DPMJET
    //myPlot.AddHisto("meanPt_pPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kMagenta+1, "", 110);
    
    //myPlot.AddHisto("momentUnfolded1", "pPb_8TeV", "", kFullCircle, kYellow+1, "", 100);
    //myPlot.AddHisto("momentUnfolded1_Syst", "pPb_8TeV", "pPb, 8.16 TeV", kFullCircle, kYellow+1, "boxes", 100);
    
    
    //Pb-Pb
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 3000);
    myPlot.AddHisto("meanPt_PbPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCross, kRed+1, "", 3000);
    myPlot.AddGraph("meanPt_PbPb_EPOS_5.02TeV", "Simulations", "", kOpenSquare, kRed+1, "", 3000);
    //myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "", kOpenCross, kRed+1, "", 3000);//HIJING
    //myPlot.AddHisto("meanPt_PbPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kRed+1, "", 3000);
    
    //Xe-Xe
    //myPlot.AddHisto("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 2000);
    //myPlot.AddHisto("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "", 2000);
    //myPlot.AddHisto("momentGeneratedMC1", "XeXe_5TeV", "HIJING", kFullStar, kGreen+1, "", 2000);
    
    
    //myPlot.AddGraph("meanPtPbPb_5TeV", "Publications", "Pb-Pb published", kFullCross, kGreen+2);
    myPlot.SetAxisRange("X", 0.1, 4000);
    myPlot.SetAxisRange("Y", 0.35, 0.85);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    
    myPlot.AddLegendBox(0.15, 0.85);
    //myPlot.AddTextBox(0.36, 0.4, systemSizeLable + " // Open symbols: EPOS-LHC // Stars Pythia Angantyr");
    myPlot.AddTextBox(0.36, 0.4, systemSizeLable);
    
    if(drawcent){
      for(int i = 0; i < 9; i++)
      {
        for(int j = 0; j < 6; j++)
        {
          //myPlot.AddTextBox(nchCent[i], 0.705 + 0.01*j, "#bf{|}", true);
        }
        myPlot.AddTextBox(nchCent[i], 0.69 - 0.02*i, string("#bf{") + centrality[i] + string("}"), true);
      }
    }
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "varianceFullRange";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.SetDrawingProperties("logX");
    myPlot.AddHisto("varianceUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("varianceUnfolded", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    //myPlot.AddHisto("variance_pp_EPOS-LHC_5.02TeV", "Simulations", "", kOpenSquare, kBlue+1, "", 60);
    myPlot.AddHisto("varianceUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
    myPlot.AddHisto("varianceUnfolded_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
    //myPlot.AddHisto("variance_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCircle, kMagenta+1, "", 100);
    //myPlot.AddHisto("variance_pPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kMagenta+1, "", 100);
    myPlot.AddHisto("varianceUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("varianceUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 3000);
    //myPlot.AddHisto("variance_PbPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCircle, kRed+1, "", 3000);
    //myPlot.AddHisto("variance_PbPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kRed+1, "", 3000);
    
    myPlot.AddHisto("varianceUnfolded", "pPb_8TeV", "", kFullCircle, kYellow+1, "", 90);
    myPlot.AddHisto("varianceUnfolded_Syst", "pPb_8TeV", "p-Pb, 8.16 TeV", kFullCircle, kYellow+1, "boxes", 90);
    
    myPlot.AddHisto("varianceUnfolded", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 1100);
    myPlot.AddHisto("varianceUnfolded_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 1100);
    
    myPlot.SetAxisRange("X", 0.1, 4000);
    myPlot.SetAxisRange("Y", 0.001, 0.8);
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    
    myPlot.AddLegendBox(0.15, 0.9);
    //myPlot.AddTextBox(0.15, 0.9, "Open symbols: EPOS-LHC // Stars Pythia Angantyr");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    
    if(drawcent){
      for(int i = 0; i < 9; i++)
      {
        for(int j = 0; j < 6; j++)
        {
          myPlot.AddTextBox(nchCent[i], 0.36 + 0.01*j, "#bf{|}", true);
        }
        myPlot.AddTextBox(nchCent[i], 0.46 + 0.025*i, string("#bf{") + centrality[i] + string("}"), true);
      }
    }
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1FullRangeMC";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.SetDrawingProperties("logX");
    myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 110);
    myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 3000);
    myPlot.SetAxisRange("X", 0.1, 4000);
    myPlot.SetAxisRange("Y", 0.4, 0.85);
    myPlot.AddLegendBox(0.6, 0.8);
    myPlot.AddTextBox(0.4, 0.29, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDensityComparison";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logX");
    myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "");
    myPlot.AddHisto("multDensityUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
    myPlot.AddHisto("multDensityUnfolded", "XeXe_5TeV", "", kFullStar, kGreen+2, "");
    myPlot.AddHisto("multDensityUnfolded_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes");
    myPlot.AddTextBox(0.2, 0.3, systemSizeLable);
    myPlot.AddLegendBox(0.7, 0.8);
    myPlot.SetAxisRange("Y", 1e-4, 3e-2);
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
    string plotName = "multDensityFullRange";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY logX");
    myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("multDensityUnfolded", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDensityUnfolded_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDensityUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 120);
    myPlot.AddHisto("multDensityUnfolded_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 120);
    myPlot.AddHisto("multDensityGeneratedMC", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 120);
    myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
    myPlot.AddHisto("multDensityUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 3000);
    myPlot.AddHisto("multDensityUnfolded", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 2000);
    myPlot.AddHisto("multDensityUnfolded_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 2000);
    
    for(int i = 0; i < 9; i++)
    {
      for(int j = 0; j < 6; j++)
      {
        myPlot.AddTextBox(nchCent[i], 1e-4 - 0.00001*j, "#bf{|}", true);
      }
      //        myPlot.AddTextBox(nchCent[i], 1e-3 + 0.001*i, string("#bf{") + centrality[i] + string("}"), true);
    }
    myPlot.SetAxisRange("X", 0.1, 3000);
    myPlot.SetAxisRange("Y", 0.00001, 0.15);
    myPlot.AddLegendBox(0.2, 0.5);
    myPlot.AddTextBox(0.35, 0.3, systemSizeLable);
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
    string plotName = "meanPt";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 120);
    
    // pp
    //myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    //myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    //myPlot.AddHisto("meanPt_pp_EPOS-LHC_5.02TeV", "Simulations", "", kOpenSquare, kBlue+1, "", 60);
    
    // pPb
    myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 150);
    myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 150);
    //myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "DPMJET", kOpenCircle, kMagenta+1, "", 120);
    myPlot.AddHisto("meanPt_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCircle, kMagenta+1, "", 150);
    myPlot.AddGraph("meanPt_pPb_EPOS_5.02TeV", "Simulations", "", kOpenSquare, kMagenta+1, "", 150);
    //myPlot.AddHisto("meanPt_pPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kMagenta+1, "", 120);
    
    // PbPb
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "");
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
    //myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "HIJING", kFullCross, kRed+1, "");
    myPlot.AddHisto("meanPt_PbPb_EPOS-LHC_5.02TeV", "Simulations", "", kFullCross, kRed+1, "");
    myPlot.AddGraph("meanPt_PbPb_EPOS_5.02TeV", "Simulations", "", kOpenSquare, kRed+1, "");
    //myPlot.AddHisto("meanPt_PbPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kRed+1, "");
    
    // XeXe
    //myPlot.AddHisto("momentUnfolded1", "XeXe_5TeV", "", kFullStar, kGreen+2, "");
    //myPlot.AddHisto("momentUnfolded1_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes");
    //myPlot.AddHisto("momentGeneratedMC1", "XeXe_5TeV", "HIJING", kFullStar, kGreen+1, "");
    
    // additional stuff
    //myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV_LowIR", "", kFullCross, kOrange+1, "");
    //myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV_LowIR", "PbPb low IR", kFullCross, kOrange+1, "boxes");
    
    //myPlot.AddHisto("momentUnfolded1", "pPb_8TeV", "", kFullCircle, kYellow+1, "", 100);
    //myPlot.AddHisto("momentUnfolded1_Syst", "pPb_8TeV", "pPb, 8.16 TeV", kFullCircle, kYellow+1, "boxes", 100);
    
    //myPlot.AddHisto("momentUnfolded1", "Pbp_8TeV", "", kFullCircle, kYellow+2, "", 100);
    //myPlot.AddHisto("momentUnfolded1_Syst", "Pbp_8TeV", "Pbp, 8 TeV", kFullCircle, kYellow+2, "boxes", 100);
    
    //myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV_2018", "", kFullCross, kBlack, "");
    //myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV_2018", "PbPb 2018", kFullCross, kBlack, "boxes");
    
    myPlot.SetAxisRange("X", -0.5, 300);
    myPlot.SetAxisRange("Y", 0.4, 0.95);
    myPlot.AddLegendBox(0.63, 0.92, "");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddTextBox(0.35, 0.4, systemSizeLable);
    //myPlot.AddTextBox(0.35, 0.4, systemSizeLable + " // Open symbols: EPOS-LHC // Stars: Pythia Angantyr");
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
    string plotName = "moment1WithMC";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "", 0, kBlue+1, "band", 60);
    myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
    myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
    myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "", 0, kMagenta+1, "band");
    myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 100);
    myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 100);
    myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "", 0, kRed+1, "band");
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("varianceUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
    myPlot.AddHisto("varianceUnfolded", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("varianceUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
    myPlot.AddHisto("varianceUnfolded_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
    //myPlot.AddHisto("variance_pPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCircle, kMagenta+1, "", 100);
    //myPlot.AddHisto("variance_pPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kMagenta+1, "", 100);
    myPlot.AddHisto("varianceUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 200);
    myPlot.AddHisto("varianceUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 200);
    //myPlot.AddHisto("variance_PbPb_EPOS-LHC_5.02TeV", "Simulations", "", kOpenCircle, kRed+1, "", 200);
    //myPlot.AddHisto("variance_PbPb_Angantyr_5.02TeV", "Simulations", "", kFullStar, kRed+1, "", 200);
    myPlot.AddHisto("varianceUnfolded", "pPb_8TeV", "", kFullCircle, kYellow+1, "", 90);
    myPlot.AddHisto("varianceUnfolded_Syst", "pPb_8TeV", "p-Pb, 8.16 TeV", kFullCircle, kYellow+1, "boxes", 90);
    myPlot.AddHisto("varianceUnfolded", "XeXe_5TeV", "", kFullStar, kGreen+2, "", 200);
    myPlot.AddHisto("varianceUnfolded_Syst", "XeXe_5TeV", "Xe-Xe", kFullStar, kGreen+2, "boxes", 200);
    myPlot.SetAxisRange("Y", 0.05, 0.8);
    myPlot.AddLegendBox(0.6, 0.9, "");
    myPlot.SetAxisTitle("X", "#it{N}_{ch}");
    myPlot.AddTextBox(0.35, 0.3, systemSizeLable);
    //myPlot.AddTextBox(0.53, 0.72, "Open symbols: EPOS-LHC // Stars Pythia Angantyr");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "varianceMC";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("varianceGeneratedMC", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("varianceGeneratedMC", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 100);
    myPlot.AddHisto("varianceGeneratedMC", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 100);
    myPlot.SetAxisRange("Y", 0.05, 0.8);
    myPlot.AddLegendBox(0.2, 0.9, "");
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
  { // -----------------------------------------------------------------------
    string plotName = "moment2";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded2", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded2_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("momentUnfolded2", "pPb_5TeV", "", kFullCircle, kMagenta+1);
    myPlot.AddHisto("momentUnfolded2_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddHisto("momentUnfolded2", "PbPb_5TeV", "", kFullCross, kRed+1);
    myPlot.AddHisto("momentUnfolded2_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
    myPlot.AddHisto("momentUnfolded2_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
    myPlot.SetAxisRange("Y", 0.2, 1.5);
    myPlot.AddLegendBox(0.2, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment3";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfolded3", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded3_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("momentUnfolded3", "pPb_5TeV", "", kFullCircle, kMagenta+1);
    myPlot.AddHisto("momentUnfolded3_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddHisto("momentUnfolded3", "PbPb_5TeV", "", kFullCross, kRed+1);
    myPlot.AddHisto("momentUnfolded3_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
    myPlot.SetAxisRange("Y", 0.2, 5);
    myPlot.AddLegendBox(0.6, 0.4, "");
    myPlot.AddTextBox(0.15, 0.9, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1MC";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentUnfoldedMC1", "pp_5TeV", "pp", 0, kBlue+1);
    myPlot.AddHisto("momentUnfoldedMC1", "pPb_5TeV", "p-Pb", 0, kMagenta+1);
    myPlot.AddHisto("momentUnfoldedMC1", "PbPb_5TeV", "Pb-Pb", 0, kRed+1);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.6, 0.4, "");
    myPlot.AddTextBox(0.15, 0.9, systemSizeLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "moment1MCtruth";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "pp (Pythia8)", 0, kBlue+1, "", 60);
    myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "p-Pb (DPMJET)", 0, kMagenta+1, "", 100);
    myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "Pb-Pb (HIJING)", 0, kRed+1, "", 100);
    myPlot.AddLegendBox(0.15, 0.9, "");
    myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
}
