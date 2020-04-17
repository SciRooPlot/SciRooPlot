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

void MultDepSpec::DefinePublicationPlots(PlotManager& plotEnv)
{
  string publicationLable = alice + ", " + chargedParticles +  newLine + eta03 + ", " + ptRange10GeV + newLine + "#it{N}_{ch} cut: #it{p}_{T} > 0 GeV/#it{c}";
  string plotGroup = "system_comparison";
  { // -----------------------------------------------------------------------
    string plotName = "publication";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logX");
    myPlot.AddData("meanPt_PbPb_2.76TeV_Stat", "Publications", "", kFullCross, kWhite);
    myPlot.AddData("meanPt_pp_7TeV_Stat", "Publications", "", kFullSquare, kBlue+1);
    myPlot.AddData("meanPt_pp_7TeV_Syst", "Publications", "pp #sqrt{s} = 7 TeV", kFullSquare, kBlue+1, "boxes");
    myPlot.AddData("meanPt_pPb_5.02TeV_Stat", "Publications", "", kFullCircle, kMagenta+1);
    myPlot.AddData("meanPt_pPb_5.02TeV_Syst", "Publications", "p-Pb #sqrt{#it{s}_{NN}} = 5.02 TeV", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddData("meanPt_PbPb_2.76TeV_Stat", "Publications", "", kFullCross, kRed+1);
    myPlot.AddData("meanPt_PbPb_2.76TeV_Syst", "Publications", "Pb-Pb  #sqrt{#it{s}_{NN}} = 2.76 TeV", kFullCross, kRed+1, "boxes");
    myPlot.SetAxisRange("X", 0, 100);
    myPlot.SetAxisRange("Y", 0.45, 0.92);
    //      myPlot.AddLegendBox(0.5, 0.92, "");
    //      myPlot.AddTextBox(0.3, 0.3, publicationLable);
    myPlot.AddLegendBox(0.15, 0.7, "");
    myPlot.AddTextBox(0.15, 0.92, publicationLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "publicationdNchdEta";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddData("meanPt_PbPb_2.76TeV_dNchdEta_Stat", "Publications", "", kFullCross, kWhite);
    myPlot.AddData("meanPt_pp_7TeV_dNchdEta_Stat", "Publications", "", kFullSquare, kBlue+1);
    myPlot.AddData("meanPt_pp_7TeV_dNchdEta_Syst", "Publications", "pp #sqrt{s} = 7 TeV", kFullSquare, kBlue+1, "boxes");
    myPlot.AddData("meanPt_pPb_5.02TeV_dNchdEta_Stat", "Publications", "", kFullCircle, kMagenta+1);
    myPlot.AddData("meanPt_pPb_5.02TeV_dNchdEta_Syst", "Publications", "p-Pb #sqrt{#it{s}_{NN}} = 5.02 TeV", kFullCircle, kMagenta+1, "boxes");
    myPlot.AddData("meanPt_PbPb_2.76TeV_dNchdEta_Stat", "Publications", "", kFullCross, kRed+1);
    myPlot.AddData("meanPt_PbPb_2.76TeV_dNchdEta_Syst", "Publications", "Pb-Pb #sqrt{#it{s}_{NN}} = 2.76 TeV", kFullCross, kRed+1, "boxes");
    myPlot.AddData("momentUnfolded1dNchDeta", "pp_5TeV", "pp", kOpenSquare, kBlue+1);
    myPlot.AddData("momentUnfolded1dNchDeta", "pPb_5TeV", "pPb", kOpenCircle, kMagenta+1);
    myPlot.AddData("momentUnfolded1dNchDeta", "PbPb_5TeV", "PbPb", kOpenCross, kRed+1);
    //      myPlot.CropHisto("momentUnfolded1", "pp_5TeV", "X", 0, 40)
    myPlot.SetAxisRange("Y", 0.45, 0.92);
    myPlot.AddLegendBox(0.55, 0.3, "");
    myPlot.AddTextBox(0.5, 0.92, publicationLable);
    //  myPlot3.SetDrawingProperties("thick");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDistsComparisonPub";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddData("multDist_pp_7TeV_Stat", "Publications", "", kFullSquare, kMagenta+1);
    myPlot.AddData("multDist_pp_7TeV_Syst", "Publications", "preliminary (ALICE-PUBLIC-2013-001)", kFullSquare, kMagenta+1, "boxes");
    myPlot.AddData("multDensityUnfolded", "pp_7TeV", "", kOpenCircle, kBlue+1);
    myPlot.AddData("multDensityUnfolded_Syst", "pp_7TeV", "this work", kOpenCircle, kBlue+1, "boxes");
    myPlot.AddData("multDensityGeneratedMC", "pp_7TeV", "Pythia6", kOpenCross, kGray+2);
    //      myPlot.AddData("multDensityGeneratedMC", "Simulations", "Pythia8", kOpenCross, kGray+2);
    myPlot.AddRatio("multDensityUnfolded_Syst", "pp_7TeV", "multDist_pp_7TeV_Syst", "Publications", "", kOpenCircle, kBlue+1, "boxes");
    //      myPlot.AddRatio("multDensityUnfolded", "pp_7TeV", "multDist_pp_7TeV_Stat", "Publications");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.75, 1.25);
    myPlot.AddLegendBox(0.2, 0.25);
    string energyLable = alice + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
    myPlot.AddTextBox(0.42, 0.92, energyLable);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
}
