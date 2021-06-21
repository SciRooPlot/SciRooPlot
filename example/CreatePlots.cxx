// PlottingFramework
//
// Copyright (C) 2019-2021  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see doc/CONTRIBUTORS.md
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

#include "PlotManager.h"
using namespace PlottingFramework;

// helper that creates some good looking plot templates you can base your plots on
void DefinePlotTemplates(PlotManager& plotManager);

// ======= minimal working example ==========
// 0. source the ".plotrc" script in the main directory of the PlottingFramework repository (best put this also it in your .bashrc, .bash_profile, or similar)
// 1. copy directory containing this example to some place on your system (outside of the PlottingFramework repository)
// 2. generate files used in this example by running 'root -l generateMWE.C'
// 3. create build folder in this directory: 'mkdir build; cd build'
// 4. cmake ../
// 5. make
// 6. ./create
// 7. play around with the features described in README.md

int main(int argc, char* argv[])
{
  bool saveInputDataFiles = true;
  // create plotting environment
  PlotManager plotManager;
  DefinePlotTemplates(plotManager);

  if (saveInputDataFiles) {
    plotManager.AddInputDataFiles("measurements", {"../InputFiles/file1.root", "../InputFiles/file2.root"});
    plotManager.AddInputDataFiles("theory", {"../InputFiles/file3.root"});
    plotManager.DumpInputDataFiles("inputFilesConfig.XML");
  } else {
    plotManager.LoadInputDataFiles("inputFilesConfig.XML");
  }

  { // -----------------------------------------------------------------------
    string plotName = "plot1";
    Plot plot(plotName, "test", "1d");
    plot[1].AddData("hist3", "measurements", "gaus");
    plot[1].AddData("hist1", "measurements", "anothergaus");
    plot[1].AddData("hist7", "theory", "prediction");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "plot2";
    Plot plot(plotName, "test", "1d");
    plot[1].AddData("graph1", "measurements", "some graph").SetOptions(points);
    plot[1].AddData("func1", "measurements", "some function");
    plot[1].AddLegend(0.5, 0.3);
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  // create all the plots that were defined in interactive mode
  plotManager.CreatePlots("", "", {}, "interactive");

  // create all the plots that were defined as pdf
  //plotManager.SetOutputDirectory("/some/path/on/system/");
  //plotManager.CreatePlots("", "", {}, "pdf");

  return 0;
}

void DefinePlotTemplates(PlotManager& plotManager)
{
  vector<int16_t> goodColors = {kBlack, kBlue + 1, kRed + 1, kYellow + 1,
                                kMagenta - 4, kGreen + 3, kOrange + 1, kViolet - 3, kCyan + 2, kPink + 3, kTeal - 7, kMagenta + 1, kPink + 8, kCyan - 6, kMagenta, kRed + 2, kGreen + 2,
                                kOrange + 2, kMagenta + 2, kYellow + 3, kGray + 2, kBlue + 2, kYellow + 2, kRed, kBlue, kMagenta + 3, kGreen + 4, 28, 8, 15, 17, 12};

  { // -----------------------------------------------------------------------
    // template 1d
    Plot templatePlot("1d", "TEMPLATES");
    templatePlot.SetDimensions(710, 710, true);
    templatePlot.SetTransparent();
    templatePlot[0].SetDefaultLineColors(goodColors);
    templatePlot[0].SetDefaultMarkerColors(goodColors);
    templatePlot[0].SetDefaultFillColors(goodColors);
    templatePlot[0].SetDefaultFillStyles({0});
    templatePlot[0].SetDefaultMarkerStyles({kFullCircle});
    templatePlot[0].SetDefaultLineStyles({kSolid});
    templatePlot[0].SetDefaultTextFont(43);
    templatePlot[0].SetDefaultTextSize(24);
    templatePlot[0].SetDefaultMarkerSize(1.);
    templatePlot[0].SetDefaultLineWidth(3.);
    templatePlot[0].SetTransparent();
    templatePlot[0].SetMargins(0.07, 0.14, 0.12, 0.07);
    templatePlot[0]['X'].SetTitleOffset(1.).SetTitleFont(43).SetTitleSize(34).SetOppositeTicks().SetMaxDigits(3);
    templatePlot[0]['Y'].SetTitleOffset(1.2).SetTitleFont(43).SetTitleSize(34).SetOppositeTicks().SetMaxDigits(3);
    templatePlot[1].SetPosition(0., 0., 1., 1.);
    //templatePlot[1].AddText(0.433, 0.98, "my Pad Title").SetTextColor(kRed);
    plotManager.AddPlotTemplate(templatePlot);
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    // template 1d ratio
    Plot templatePlot("1d_ratio", "TEMPLATES");
    templatePlot.SetDimensions(710, 710, true);
    templatePlot.SetTransparent();
    templatePlot[0].SetDefaultLineColors(goodColors);
    templatePlot[0].SetDefaultMarkerColors(goodColors);
    templatePlot[0].SetDefaultFillColors(goodColors);
    templatePlot[0].SetDefaultFillStyles({0});
    templatePlot[0].SetDefaultMarkerStyles({kFullCircle});
    templatePlot[0].SetDefaultLineStyles({kSolid});
    templatePlot[0].SetDefaultMarkerSize(1.);
    templatePlot[0].SetDefaultLineWidth(3.);
    templatePlot[0].SetDefaultTextFont(43);
    templatePlot[0].SetDefaultTextSize(24);
    templatePlot[0].SetTransparent();
    templatePlot[0]['X'].SetTitleOffset(1.1).SetOppositeTicks().SetMaxDigits(3);
    templatePlot[0]['Y'].SetTitleOffset(1.4).SetOppositeTicks().SetMaxDigits(3);
    templatePlot[1].SetPosition(0., 0.28, 1., 1.);
    templatePlot[1].SetMargins(0.05, 0.0, 0.14, 0.05);
    templatePlot[1]['X'].SetTitleSize(0.).SetLabelSize(0.);
    templatePlot[1]['Y'].SetTitleOffset(1.5);
    templatePlot[2].SetPosition(0., 0., 1., 0.28);
    templatePlot[2].SetMargins(0.015, 0.4, 0.14, 0.05);
    templatePlot[2].SetRefFunc("1");
    templatePlot[2]['X'].SetTickLength(0.06).SetTitleOffset(1.1).SetTitleSize(34).SetOppositeTicks();
    templatePlot[2]['Y'].SetNumDivisions(305).SetTitleOffset(1.5).SetTitleSize(34).SetTitleCenter();
    plotManager.AddPlotTemplate(templatePlot);
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    // template 2d
    Plot templatePlot("2d", "TEMPLATES");
    templatePlot.SetDimensions(710, 710, true);
    templatePlot.SetTransparent();
    templatePlot[0].SetDefaultDrawingOptionHist2d(colz);
    templatePlot[0].SetDefaultTextFont(43);
    templatePlot[0].SetDefaultTextSize(24);
    templatePlot[0].SetTransparent();
    templatePlot[0].SetMargins(0.12 - 0.05, 0.12 + 0.02, 0.12, 0.12 + 0.06);
    templatePlot[0]['X'].SetTitleOffset(1.1).SetMaxDigits(3);
    templatePlot[0]['Y'].SetTitleOffset(1.1).SetMaxDigits(3);
    templatePlot[0]['Z'].SetTitleOffset(1.6).SetMaxDigits(3);
    templatePlot[0].SetRedrawAxes();
    templatePlot[1].SetPosition(0., 0., 1., 1.);
    plotManager.AddPlotTemplate(templatePlot);
  } // -----------------------------------------------------------------------
}
