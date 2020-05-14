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

//****************************************************************************************
/**
 * Playground for testing.
 */
//****************************************************************************************
void MultDepSpec::DefineTestPlots(PlotManager& plotManager)
{
  string plotGroup = "test";

  { // -----------------------------------------------------------------------
    string plotName = "test1d";
    Plot plot(plotName, plotGroup, "1d");

    plot[1].AddData({"func", "dummyInput"}, "func").SetRangeX(0, 20).SetLineWidth(3.).SetLineColor(kBlue);

    plot[1].AddData({"momentUnfolded1", "pPb_5TeV"}, "pPb, 5 TeV")
    .SetOptions(hist)
    .SetRangeX(1, 60).SetColor(kGreen+1).SetLineWidth(3.).SetLineColor(kRed);

    plot[1].AddData({"graph", "dummyInput"}, "graph").SetOptions(points).SetMarkerStyle(kFullCircle).SetMarkerColor(kRed).SetMarkerSize(1.5).SetLine(kBlue, kSolid, 3.);

    plot[1].AddData({"momentUnfolded1", "pp_2TeV"}, "2 TeV")
    .SetMaxRangeX(40).SetMarkerColor(kRed);

    plot[1].AddData({"momentUnfolded1", "pp_5TeV"}, "5 TeV")
    .SetMaxRangeX(60);
    
    plot[1].AddData({"momentUnfolded1", "pp_7TeV"}, "7 TeV")
    .SetMaxRangeX(60);
    
    plot[1].AddData({"momentUnfolded1", "pp_13TeV"}, "13 TeV")
    .SetMaxRangeX(70);

    plot[1].SetRedrawAxes();
    plot[1].AddLegend();
    plot[1]["Y"].SetRange(0.40, 0.85);
    plot[1]["X"].SetMaxRange(70).SetTitle("Multiplicity");

    plotManager.AddPlot(plot);

  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "test1d_ratio";
    Plot plot(plotName, plotGroup, "1d_ratio");

    plot[1].AddData({"momentUnfolded1", "pp_5TeV"}, "5 TeV")
    .SetMarker(kRed, kFullCircle, 1.2).SetMaxRangeX(40);
    
    plot[1].AddData({"momentUnfolded1", "pp_13TeV"}, "13 TeV")
    .SetMarker(kBlack, kFullCircle, 1.2).SetMaxRangeX(70);

    plot[2].AddRatio({"momentUnfolded1", "pp_13TeV"}, {"momentUnfolded1", "pp_5TeV"}, "ratio").SetMarker(kRed, kFullCircle, 1.2).SetMaxRangeX(60);

    plot[1].AddLegend();

    plot[0]["X"].SetRange(20, 70);

    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  { // -----------------------------------------------------------------------
    string plotName = "test2d";
    Plot plot(plotName, plotGroup, "2d");
    plot[1].SetTitle("Hallo");
    plot[1].AddData({"multPtUnfolded", "pp_13TeV"})
    .SetOptions("COLZ")
    //.SetOptions("LEGO")
    ;
    plot[1]["Z"].SetRange(1e-4, 1e-2).SetLog();
    plot[1]["Y"].SetLog().SetRange(0.15, 5);

    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  // -----------------------------------------------------------------------
  // hard probes 2020 preliminaries:
  // -----------------------------------------------------------------------

  string systemSizeLablePrel = alicePrel + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
  string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;

   { // -----------------------------------------------------------------------
     // compare meanPt in all systems
     string plotName = "meanPt_systems";
     Plot plot(plotName, "preliminary", "1d");
     plot[1].AddFrame({"momentUnfolded1", PbPb_5TeV[data].input});
     
     //p-p
     plot[1].AddData({"momentUnfolded1", pp_5TeV[data].input})
     .SetMarkerStyle(kFullCross)
     .SetColor(pp_5TeV[data].color)
     .SetMaxRangeX(pp_5TeV[data].mult)
     ;
     plot[1].AddData({"momentUnfolded1_Syst", pp_5TeV[data].input}, "pp").SetOptions(boxes).SetFillStyle(0)
     .SetMarkerStyle(kFullCross)
     .SetColor(pp_5TeV[data].color)
     .SetMaxRangeX(pp_5TeV[data].mult)
     ;
     plot[1].AddData({"meanPt", pp_5TeV[epos_lhc].input})
     .SetMarkerStyle(pp_5TeV[epos_lhc].marker)
     .SetColor(pp_5TeV[epos_lhc].color)
     .SetMaxRangeX(pp_5TeV[epos_lhc].mult)
     .SetOptions(curve).SetLine(pp_5TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
     ;
     plot[1].AddData({"meanPt", pp_5TeV[pythia].input})
     .SetMarkerStyle(pp_5TeV[pythia].marker)
     .SetColor(pp_5TeV[pythia].color)
     .SetMaxRangeX(pp_5TeV[pythia].mult)
     .SetOptions(curve).SetLine(pp_5TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
     ;
     //p-Pb
     plot[1].AddData({"momentUnfolded1", pPb_5TeV[data].input})
     .SetMarkerStyle(pPb_5TeV[data].marker)
     .SetColor(pPb_5TeV[data].color)
     .SetMaxRangeX(pPb_5TeV[data].mult)
     ;
     plot[1].AddData({"momentUnfolded1_Syst", pPb_5TeV[data].input}, "p-Pb").SetOptions(boxes).SetFillStyle(0)
     .SetMarkerStyle(pPb_5TeV[data].marker)
     .SetColor(pPb_5TeV[data].color)
     .SetMaxRangeX(pPb_5TeV[data].mult)
     ;
     plot[1].AddData({"meanPt", pPb_5TeV[epos_lhc].input})
     .SetMarkerStyle(pPb_5TeV[epos_lhc].marker)
     .SetColor(pPb_5TeV[epos_lhc].color)
     .SetMaxRangeX(pPb_5TeV[epos_lhc].mult)
     .SetOptions(curve).SetLine(pPb_5TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
     ;
     plot[1].AddData({"meanPt", pPb_5TeV[angantyr].input})
     .SetMarkerStyle(pPb_5TeV[angantyr].marker)
     .SetColor(pPb_5TeV[angantyr].color)
     .SetMaxRangeX(pPb_5TeV[angantyr].mult)
     .SetOptions(curve).SetLine(pPb_5TeV[angantyr].color, kSolid, 3.).SetFillStyle(0)
     ;
     //Pb-Pb
     plot[1].AddData({"momentUnfolded1", PbPb_5TeV[data].input})
     .SetMarkerStyle(PbPb_5TeV[data].marker)
     .SetColor(PbPb_5TeV[data].color)
     .SetMaxRangeX(PbPb_5TeV[data].mult)
     ;
     plot[1].AddData({"momentUnfolded1_Syst", PbPb_5TeV[data].input}, "Pb-Pb").SetOptions(boxes).SetFillStyle(0)
     .SetMarkerStyle(PbPb_5TeV[data].marker)
     .SetColor(PbPb_5TeV[data].color)
     .SetMaxRangeX(PbPb_5TeV[data].mult)
     ;
     plot[1].AddData({"meanPt", PbPb_5TeV[epos].input})
     .SetMarkerStyle(PbPb_5TeV[epos].marker)
     .SetColor(PbPb_5TeV[epos].color)
     .SetMaxRangeX(PbPb_5TeV[epos].mult)
     //.SetMinRangeX(5.)
     .SetOptions(line).SetLine(PbPb_5TeV[epos].color, kDashed, 3.).SetFillStyle(0)
     ;
     plot[1].AddData({"meanPt", PbPb_5TeV[angantyr].input})
     .SetMarkerStyle(PbPb_5TeV[angantyr].marker)
     .SetColor(PbPb_5TeV[angantyr].color)
     .SetMaxRangeX(PbPb_5TeV[angantyr].mult)
     .SetOptions(curve).SetLine(PbPb_5TeV[angantyr].color, kSolid, 3.).SetFillStyle(0);

     plot[1].AddText(0.34, 0.3, systemSizeLablePrel);
     plot[1]["Y"].SetRange(0.45, 0.85);
     plot[1]["X"].SetTitle("#it{N}_{ch}");
     plot[1]["Y"].SetTitleOffset(1.7);
     plot[1].AddText(0.12, 0.93, "solid: //   Pythia8 (pp), //   Angantyr (p-Pb, Pb-Pb) // dashed: //   EOPOS-LHC (pp, p-Pb), //   EPOS3 (Pb-Pb)");

     Plot plotLin(plot, plotName + "_lin", "preliminary");

     plot[1].AddLegend(0.65, 0.9);

     plot[1].SetPadOptions("logX");
     plot[1]["X"].SetRange(0.1, 4000).SetLog();
     
     plotLin[1]["Y"].SetRange(0.45, 0.95);

     plotLin[1]["X"].SetMaxRange(100).SetLog(false);
     plotLin[1].AddLegend(0.65, 0.9);
     plotManager.AddPlot(plotLin);
     plotManager.AddPlot(plot);
   } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    // compare meanPt in all systems
    string plotName = "variance_systems";
    Plot plot(plotName, "preliminary", "1d");
    plot[1].AddFrame({"varianceUnfolded", PbPb_5TeV[data].input});
    
    //p-p
    plot[1].AddData({"varianceUnfolded", pp_5TeV[data].input})
    .SetMarkerStyle(kFullCross)
    .SetColor(pp_5TeV[data].color)
    .SetMaxRangeX(pp_5TeV[data].mult)
    ;
    plot[1].AddData({"varianceUnfolded_Syst", pp_5TeV[data].input}, "pp").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(kFullCross)
    .SetColor(pp_5TeV[data].color)
    .SetMaxRangeX(pp_5TeV[data].mult)
    ;
    plot[1].AddData({"variance", pp_5TeV[epos_lhc].input})
    .SetMarkerStyle(pp_5TeV[epos_lhc].marker)
    .SetColor(pp_5TeV[epos_lhc].color)
    .SetMaxRangeX(pp_5TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pp_5TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddData({"variance", pp_5TeV[pythia].input})
    .SetMarkerStyle(pp_5TeV[pythia].marker)
    .SetColor(pp_5TeV[pythia].color)
    .SetMaxRangeX(pp_5TeV[pythia].mult)
    .SetOptions(curve).SetLine(pp_5TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    //p-Pb
    plot[1].AddData({"varianceUnfolded", pPb_5TeV[data].input})
    .SetMarkerStyle(pPb_5TeV[data].marker)
    .SetColor(pPb_5TeV[data].color)
    .SetMaxRangeX(pPb_5TeV[data].mult)
    ;
    plot[1].AddData({"varianceUnfolded_Syst", pPb_5TeV[data].input}, "p-Pb").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(pPb_5TeV[data].marker)
    .SetColor(pPb_5TeV[data].color)
    .SetMaxRangeX(pPb_5TeV[data].mult)
    ;
    plot[1].AddData({"variance", pPb_5TeV[epos_lhc].input})
    .SetMarkerStyle(pPb_5TeV[epos_lhc].marker)
    .SetColor(pPb_5TeV[epos_lhc].color)
    .SetMaxRangeX(pPb_5TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pPb_5TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddData({"variance", pPb_5TeV[angantyr].input})
    .SetMarkerStyle(pPb_5TeV[angantyr].marker)
    .SetColor(pPb_5TeV[angantyr].color)
    .SetMaxRangeX(pPb_5TeV[angantyr].mult)
    .SetOptions(curve).SetLine(pPb_5TeV[angantyr].color, kSolid, 3.).SetFillStyle(0)
    ;
    //Pb-Pb
    plot[1].AddData({"varianceUnfolded", PbPb_5TeV[data].input})
    .SetMarkerStyle(PbPb_5TeV[data].marker)
    .SetColor(PbPb_5TeV[data].color)
    .SetMaxRangeX(PbPb_5TeV[data].mult)
    ;
    plot[1].AddData({"varianceUnfolded_Syst", PbPb_5TeV[data].input}, "Pb-Pb").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(PbPb_5TeV[data].marker)
    .SetColor(PbPb_5TeV[data].color)
    .SetMaxRangeX(PbPb_5TeV[data].mult)
    ;
    plot[1].AddData({"variance", PbPb_5TeV[angantyr].input})
    .SetMarkerStyle(PbPb_5TeV[angantyr].marker)
    .SetColor(PbPb_5TeV[angantyr].color)
    .SetMaxRangeX(PbPb_5TeV[angantyr].mult)
    .SetOptions(curve).SetLine(PbPb_5TeV[angantyr].color, kSolid, 3.).SetFillStyle(0);

    plot[1].AddText(0.34, 0.3, systemSizeLablePrel);
    plot[1]["Y"].SetRange(0.05, 0.8);
    plot[1]["X"].SetTitle("#it{N}_{ch}");
    plot[1]["Y"].SetTitleOffset(1.7);
    plot[1].AddText(0.12, 0.93, "solid: //   Pythia8 (pp), //   Angantyr (p-Pb, Pb-Pb) // dashed: //   EOPOS-LHC (pp, p-Pb), //   EPOS3 (Pb-Pb)");

    Plot plotLin(plot, plotName + "_lin", "preliminary");

    plot[1].AddLegend(0.65, 0.9);

    plot[1].SetPadOptions("logX");
    plot[1]["X"].SetRange(0.1, 4000).SetLog();
    
    //plotLin[1]["Y"].SetRange(0.45, 0.95);

    plotLin[1]["X"].SetMaxRange(100).SetLog(false);
    plotLin[1].AddLegend(0.65, 0.9);
    plotManager.AddPlot(plotLin);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  

  
  { // -----------------------------------------------------------------------
    // compare meanPt for different ernergies in pp
    string plotName = "meanPt_energies";
    Plot plot(plotName, "preliminary", "1d");

    double_t maxMult = 59;
    // pp, 13 TeV
    plot[1].AddData({"momentUnfolded1", pp_13TeV[data].input})
    .SetMarkerStyle(pp_13TeV[data].marker)
    .SetColor(pp_13TeV[data].color)
    .SetRangeX(1., maxMult)
    ;
    plot[1].AddData({"momentUnfolded1_Syst", pp_13TeV[data].input}, "13 TeV").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(pp_13TeV[data].marker)
    .SetColor(pp_13TeV[data].color)
    .SetRangeX(1., maxMult)
    ;
    plot[1].AddData({"meanPt", pp_13TeV[epos_lhc].input})
    .SetMarkerStyle(pp_13TeV[epos_lhc].marker)
    .SetColor(pp_13TeV[epos_lhc].color)
    .SetRangeX(1., maxMult)
    .SetOptions(curve).SetLine(pp_13TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddData({"meanPt", pp_13TeV[pythia].input})
    .SetMarkerStyle(pp_13TeV[pythia].marker)
    .SetColor(pp_13TeV[pythia].color)
    .SetRangeX(1., maxMult)
    .SetOptions(curve).SetLine(pp_13TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    // pp, 7 TeV
    plot[1].AddData({"momentUnfolded1", pp_7TeV[data].input})
    .SetMarkerStyle(pp_7TeV[data].marker)
    .SetColor(pp_7TeV[data].color)
    .SetRangeX(1., pp_7TeV[data].mult-1)
    ;
    plot[1].AddData({"momentUnfolded1_Syst", pp_7TeV[data].input}, "7 TeV").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(pp_7TeV[data].marker)
    .SetColor(pp_7TeV[data].color)
    .SetRangeX(1., pp_7TeV[data].mult-1)
    ;
    plot[1].AddData({"meanPt", pp_7TeV[epos_lhc].input})
    .SetMarkerStyle(pp_7TeV[epos_lhc].marker)
    .SetColor(pp_7TeV[epos_lhc].color)
    .SetRangeX(1., pp_7TeV[epos_lhc].mult-1)
    .SetOptions(curve).SetLine(pp_7TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddData({"meanPt", pp_7TeV[pythia].input})
    .SetMarkerStyle(pp_7TeV[pythia].marker)
    .SetColor(pp_7TeV[pythia].color)
    .SetRangeX(1., pp_7TeV[pythia].mult-1)
    .SetOptions(curve).SetLine(pp_7TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    // pp, 5 TeV
    plot[1].AddData({"momentUnfolded1", pp_5TeV[data].input})
    .SetMarkerStyle(pp_5TeV[data].marker)
    .SetColor(pp_5TeV[data].color)
    .SetRangeX(1., pp_5TeV[data].mult-1)
    ;
    plot[1].AddData({"momentUnfolded1_Syst", pp_5TeV[data].input}, "5.02 TeV").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(pp_5TeV[data].marker)
    .SetColor(pp_5TeV[data].color)
    .SetRangeX(1., pp_5TeV[data].mult-1)
    ;
    plot[1].AddData({"meanPt", pp_5TeV[epos_lhc].input})
    .SetMarkerStyle(pp_5TeV[epos_lhc].marker)
    .SetColor(pp_5TeV[epos_lhc].color)
    .SetRangeX(1., pp_5TeV[epos_lhc].mult-1)
    .SetOptions(curve).SetLine(pp_5TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddData({"meanPt", pp_5TeV[pythia].input})
    .SetMarkerStyle(pp_5TeV[pythia].marker)
    .SetColor(pp_5TeV[pythia].color)
    .SetRangeX(1., pp_5TeV[pythia].mult-1)
    .SetOptions(curve).SetLine(pp_5TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    // pp, 2 TeV
    plot[1].AddData({"momentUnfolded1", pp_2TeV[data].input})
    .SetMarkerStyle(pp_2TeV[data].marker)
    .SetColor(pp_2TeV[data].color)
    .SetRangeX(1., pp_2TeV[data].mult)
    ;
    plot[1].AddData({"momentUnfolded1_Syst", pp_2TeV[data].input}, "2.76 TeV").SetOptions(boxes).SetFillStyle(0)
    .SetMarkerStyle(pp_2TeV[data].marker)
    .SetColor(pp_2TeV[data].color)
    .SetRangeX(1., pp_2TeV[data].mult)
    ;
    plot[1].AddData({"meanPt", pp_2TeV[epos_lhc].input})
    .SetMarkerStyle(pp_2TeV[epos_lhc].marker)
    .SetColor(pp_2TeV[epos_lhc].color)
    .SetRangeX(1., pp_2TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pp_2TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddData({"meanPt", pp_2TeV[pythia].input})
    .SetMarkerStyle(pp_2TeV[pythia].marker)
    .SetColor(pp_2TeV[pythia].color)
    .SetRangeX(1., pp_2TeV[pythia].mult)
    .SetOptions(curve).SetLine(pp_2TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    
    plot[1]["X"].SetRange(0.1, 60);
    plot[1]["Y"].SetRange(0.45, 0.85);
    plot[1]["X"].SetTitle("#it{N}_{ch}");
    
    plot[1]["Y"].SetTitleOffset(1.7);

    
    plot[1].AddLegend(0.14, 0.92);
    plot[1].AddText(0.4, 0.45, "solid: Pythia8 Monash13 // dashed: EPOS-LHC");
    plot[1].AddText(0.34, 0.3, energyLablePrel);
    
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  
  
  
  { // -----------------------------------------------------------------------
    // compare meanPt for different ernergies in pp
    string plotName = "meanPt_energies_ratio";
    Plot plot(plotName, "preliminary", "1d");

    plot[1].SetRefFunc("1");

    plot[1].AddData({"momentUnfolded1_SystBand", pp_13TeV[data].input})
    .SetFill(pp_13TeV[data].color, 1001).SetFillOpacity(0.3)
    .SetRangeX(1., pp_13TeV[data].mult)
    .SetOptions(band)
    ;
    plot[1].AddData({"momentUnfolded1_SystBand", pp_7TeV[data].input})
    .SetFill(pp_7TeV[data].color, 1001).SetFillOpacity(0.3)
    .SetRangeX(1., pp_7TeV[data].mult)
    .SetOptions(band)
    ;
    plot[1].AddData({"momentUnfolded1_SystBand", pp_5TeV[data].input})
    .SetFill(pp_5TeV[data].color, 1001).SetFillOpacity(0.3)
    .SetRangeX(1., pp_5TeV[data].mult)
    .SetOptions(band)
    ;
    plot[1].AddData({"momentUnfolded1_SystBand", pp_2TeV[data].input})
    .SetFill(pp_2TeV[data].color, 1001).SetFillOpacity(0.3)
    .SetRangeX(1., pp_2TeV[data].mult)
    .SetOptions(band)
    ;
    
    // pp, 13 TeV
    plot[1].AddRatio({"meanPt", pp_13TeV[epos_lhc].input}, {"momentUnfolded1_Syst", pp_13TeV[data].input})
    .SetColor(pp_13TeV[epos_lhc].color)
    .SetRangeX(1., pp_13TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pp_13TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddRatio({"meanPt", pp_13TeV[pythia].input}, {"momentUnfolded1_Syst", pp_13TeV[data].input},  "13 TeV")
    .SetColor(pp_13TeV[pythia].color)
    .SetRangeX(1., pp_13TeV[pythia].mult)
    .SetOptions(curve).SetLine(pp_13TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    // pp, 7 TeV
    plot[1].AddRatio({"meanPt", pp_7TeV[epos_lhc].input}, {"momentUnfolded1_Syst", pp_7TeV[data].input})
    .SetColor(pp_7TeV[epos_lhc].color)
    .SetRangeX(1., pp_7TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pp_7TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddRatio({"meanPt", pp_7TeV[pythia].input}, {"momentUnfolded1_Syst", pp_7TeV[data].input},  "7 TeV")
    .SetColor(pp_7TeV[pythia].color)
    .SetRangeX(1., pp_7TeV[pythia].mult)
    .SetOptions(curve).SetLine(pp_7TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    // pp, 5 TeV
    plot[1].AddRatio({"meanPt", pp_5TeV[epos_lhc].input}, {"momentUnfolded1_Syst", pp_5TeV[data].input})
    .SetColor(pp_5TeV[epos_lhc].color)
    .SetRangeX(1., pp_5TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pp_5TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddRatio({"meanPt", pp_5TeV[pythia].input}, {"momentUnfolded1_Syst", pp_5TeV[data].input},  "5.02 TeV")
    .SetColor(pp_5TeV[pythia].color)
    .SetRangeX(1., pp_5TeV[pythia].mult)
    .SetOptions(curve).SetLine(pp_5TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;
    // pp, 2 TeV
    plot[1].AddRatio({"meanPt", pp_2TeV[epos_lhc].input}, {"momentUnfolded1_Syst", pp_2TeV[data].input})
    .SetColor(pp_2TeV[epos_lhc].color)
    .SetRangeX(1., pp_2TeV[epos_lhc].mult)
    .SetOptions(curve).SetLine(pp_2TeV[epos_lhc].color, kDashed, 3.).SetFillStyle(0)
    ;
    plot[1].AddRatio({"meanPt", pp_2TeV[pythia].input}, {"momentUnfolded1_Syst", pp_2TeV[data].input},  "2.76 TeV")
    .SetColor(pp_2TeV[pythia].color)
    .SetRangeX(1., pp_2TeV[pythia].mult)
    .SetOptions(curve).SetLine(pp_2TeV[pythia].color, kSolid, 3.).SetFillStyle(0)
    ;

    
    plot[1]["X"].SetRange(0.1, 60);
    plot[1]["Y"].SetRange(0.94, 1.06);
    plot[1]["X"].SetTitle("#it{N}_{ch}");
    plot[1]["Y"].SetTitle("model / data");

    plot[1]["Y"].SetTitleOffset(1.7);
    
    plot[1].AddLegend(0.18, 0.92);
    plot[1].AddText(0.44, 0.42, "solid: Pythia8 Monash13 // dashed: EPOS-LHC");
    plot[1].AddText(0.34, 0.3, energyLablePrel);
    
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
  /*

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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
   
   */
}
