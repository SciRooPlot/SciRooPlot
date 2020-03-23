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
    string plotName = "dummyPlot";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddGraph("dummy", "CSVTest", "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  
  { // -----------------------------------------------------------------------
    string plotName = "readFile";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto("myHist15", "", "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_2TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddHisto(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddHisto(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_7TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddHisto(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pp_13TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddHisto(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_pPb_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddHisto(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multPtScaled_PbPb_5TeV";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logY");
    myPlot.SetAxisRange("Y", 0.15, 50);
    myPlot.AddHisto(plotName, "Fits");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "selfNormalizedHighPtYield";
    Plot myPlot(plotName, plotGroup);
    myPlot.AddHisto(plotName + "_pp_2TeV", "Fits", "2.76 TeV");
    myPlot.AddHisto(plotName + "_pp_5TeV", "Fits", "5.02 TeV");
    myPlot.AddHisto(plotName + "_pp_7TeV", "Fits", "7 TeV");
    myPlot.AddHisto(plotName + "_pp_13TeV", "Fits", "13 TeV");
    myPlot.AddLegendBox();
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "selfNormalizedHighPtYieldNormalizedNch";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("normalize");
    myPlot.AddHisto(plotName + "_pp_2TeV", "Fits", "2.76 TeV (<mean>)");
    myPlot.AddHisto(plotName + "_pp_5TeV", "Fits", "5.02 TeV (<mean>)");
    myPlot.AddHisto(plotName + "_pp_7TeV", "Fits", "7 TeV (<mean>)");
    myPlot.AddHisto(plotName + "_pp_13TeV", "Fits", "13 TeV (<mean>)");
    myPlot.AddTextBox(0.5, 0.9, "6 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}");
    myPlot.AddLegendBox();
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasured13TeV";
    Plot myPlot(plotName, plotGroup);
    
    myPlot.SetDrawingProperties("logY");
    //  myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("multDistMeasured", "pp_13TeV_old", "event selection before", kFullSquare, kRed+1);
    myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV_old", "", kOpenSquare, kRed+1);
    myPlot.AddHisto("multDistMeasured", "pp_13TeV", "AilEventCuts and PhysSel plieup rejection", kFullCircle, kBlue+1);
    myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV", "", kOpenCircle, kBlue+1);
    myPlot.AddLegendBox(0.3, 0.9);
    myPlot.SetAxisRange("X", 0, 60);
    //  myPlot.SetAxisRange("Y", 0.1, 1e6);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCMeas";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("scaleEffectMultMeas", "pp_13TeV_withDDC", "");
    myPlot.SetAxisTitle("Y", "#it{N}^{MC}_{acc}");
    myPlot.SetAxisTitle("X", "#it{N}^{scaled}_{acc}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0, 60);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCTrue";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("scaleEffectMultTrue", "pp_13TeV_withDDC", "");
    myPlot.SetAxisTitle("Y", "#it{N}^{MC}_{ch}");
    myPlot.SetAxisTitle("X", "#it{N}^{scaled}_{ch}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0, 60);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDist_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDistUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistUnfolded_Syst", "pp_13TeV", "multDistUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    //    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDistKNO_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistUnfoldedKNO_Syst", "pp_13TeV", "multDistUnfoldedKNO", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    //    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDensity_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDensityUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDensityUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDensityUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDensityUnfolded_Syst", "pp_13TeV", "multDensityUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    //    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasuredMC_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistMeasuredMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    myPlot.SetAxisRange("X", 0, 60);
    //    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "MC measured");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multDistGeneratedMC_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistGeneratedMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistGeneratedMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistGeneratedMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    myPlot.SetAxisRange("X", 0, 60);
    //    myPlot.SetAxisRange("Y", 0.45, 0.85);
    //    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddTextBox(0.2, 0.25, "MC truth");
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_noDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("multCorrelationMatrix", "pp_13TeV");
    myPlot.AddTextBox(0.4, 0.25, "no data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_withDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("multCorrelationMatrix", "pp_13TeV_withDDC");
    myPlot.AddTextBox(0.4, 0.25, "with data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Mean";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("mean", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    myPlot.AddHisto("mean", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    myPlot.SetAxisTitle("Y", "<#it{N}_{acc}>");
    myPlot.SetAxisRange("Y", 0, 45);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddRatio("mean", "pp_13TeV", "mean", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    myPlot.AddLegendBox(0.2, 0.9);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Sigma";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("sigma", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    myPlot.AddHisto("sigma", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    myPlot.SetAxisTitle("Y", "#sigma(#it{N}_{acc})");
    myPlot.SetAxisRange("Y", 0, 5);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.5, 1.5);
    myPlot.AddRatio("sigma", "pp_13TeV", "sigma", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    myPlot.AddLegendBox(0.2, 0.9);
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_noDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("responseMatrix", "pp_13TeV");
    myPlot.AddTextBox(0.4, 0.25, "no data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_withDDC";
    Plot myPlot(plotName, plotGroup);
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("responseMatrix", "pp_13TeV_withDDC");
    myPlot.AddTextBox(0.4, 0.25, "with data-driven corrections");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "moment1_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("momentUnfolded1_Syst", "pp_13TeV", "momentUnfolded1", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.SetAxisRange("ratio", 0.99, 1.01);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance_withDDC";
    Plot myPlot(plotName, plotGroup, "default ratio");
    myPlot.AddHisto("varianceUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("varianceUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("varianceUnfolded_Syst", "pp_13TeV", "varianceUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.05, 0.9);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.AddPlot(myPlot);
  } // -----------------------------------------------------------------------
  
}
