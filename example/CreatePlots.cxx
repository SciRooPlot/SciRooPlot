// PlottingFramework
//
// Copyright (C) 2019-2022  Mario Krüger
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
#include "Logging.h"

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
    // NB.: in order to allow running your program from everywhere on your computer it is better NOT to use relative paths
    plotManager.DumpInputDataFiles("inputFiles.XML");
  } else {
    plotManager.LoadInputDataFiles("inputFiles.XML");
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

  // OPTION A:
  // directly create all the plots that were defined before
  plotManager.CreatePlots("", "", {}, "interactive");

  // you can also store them as pdf (for more options see README.md)
  // plotManager.SetOutputDirectory("/some/path/on/system/");
  // plotManager.CreatePlots("", "", {}, "pdf");

  // OPTION B (recommended way):
  // dump the plot definitions to file and then use the plotting app to create the plots:
  plotManager.DumpPlots("plotDefinitions.XML");

  string exampleName = "myPlots";
  INFO("");
  INFO("Run once the following commands to configure the plotting app ({} can be replaced by an arbitrary name):", exampleName);
  INFO("  plot-config add {} plotDefinitions /path/to/plotDefinitions.XML", exampleName);
  INFO("  plot-config add {} inputFiles /path/to/inputFiles.XML", exampleName);
  INFO("  plot-config add {} executable /path/to/executable/that/dumps/plots/definitions/create", exampleName);
  INFO("  plot-config add {} outputDir /where/the/output/plots/should/go", exampleName);
  INFO("");

  /*
  // in your program you can make use of the logging macros of the framework (requires to include Logging.h)
  LOG("Hello World.");
  // the strings and arguments are passed to fmt::format, so you can use all of its formatting features
  double someNumber = 42.424242424242;
  LOG("Logging magic number {}.", someNumber);
  DEBUG("With digits {:.2f}", someNumber);
  WARNING("my warning");
  ERROR("my error");
   */

  return 0;
}

void DefinePlotTemplates(PlotManager& plotManager)
{
  vector<int16_t> goodColors = {kBlack, kBlue + 1, kRed + 1, kYellow + 1,
                                kMagenta - 4, kGreen + 3, kOrange + 1,
                                kViolet - 3, kCyan + 2, kPink + 3, kTeal - 7,
                                kMagenta + 1, kPink + 8, kCyan - 6,
                                kMagenta, kRed + 2, kGreen + 2,
                                kOrange + 2, kMagenta + 2, kYellow + 3,
                                kGray + 2, kBlue + 2, kYellow + 2,
                                kRed, kBlue, kMagenta + 3,
                                kGreen + 4, 28, 8, 15, 17, 12};
  { // -----------------------------------------------------------------------
    Plot plotTemplate("1d", "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(710, 710, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultMarkerColors(goodColors);
    plotTemplate[0].SetDefaultLineColors(goodColors);
    plotTemplate[0].SetDefaultFillColors(goodColors);
    plotTemplate[0].SetDefaultFillStyles({0});
    plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
    plotTemplate[0].SetDefaultLineStyles({kSolid});
    plotTemplate[0].SetDefaultTextFont(43);
    plotTemplate[0].SetDefaultTextSize(24);
    plotTemplate[0].SetDefaultMarkerSize(1.);
    plotTemplate[0].SetDefaultLineWidth(3.);
    plotTemplate[0].SetDefaultDrawingOptionGraph(points);
    plotTemplate[0].SetTransparent();
    plotTemplate[0].SetMargins(0.07, 0.14, 0.12, 0.07);
    plotTemplate[0]['X'].SetTitleSize(28).SetTitleOffset(1.1).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
    plotTemplate[0]['Y'].SetTitleSize(28).SetTitleOffset(1.5).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[1].SetPosition(0., 0., 1., 1.);
    plotManager.AddPlotTemplate(plotTemplate);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    Plot plotTemplate("1d_ratio", "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(710, 710, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultLineColors(goodColors);
    plotTemplate[0].SetDefaultMarkerColors(goodColors);
    plotTemplate[0].SetDefaultFillColors(goodColors);
    plotTemplate[0].SetDefaultFillStyles({0});
    plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
    plotTemplate[0].SetDefaultLineStyles({kSolid});
    plotTemplate[0].SetDefaultMarkerSize(1.);
    plotTemplate[0].SetDefaultLineWidth(3.);
    plotTemplate[0].SetDefaultDrawingOptionGraph(points);
    plotTemplate[0].SetDefaultTextFont(43);
    plotTemplate[0].SetDefaultTextSize(24);
    plotTemplate[0].SetTransparent();
    plotTemplate[0]['X'].SetTitleSize(28).SetTitleOffset(1.1).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[0]['Y'].SetTitleSize(28).SetTitleOffset(1.5).SetOppositeTicks().SetMaxDigits(3);
    plotTemplate[1]['X'].SetTitleSize(0.).SetLabelSize(0.);
    plotTemplate[1].SetPosition(0., 0.28, 1., 1.);
    plotTemplate[1].SetMargins(0.05, 0.0, 0.14, 0.05);
    plotTemplate[2].SetPosition(0., 0., 1., 0.28);
    plotTemplate[2].SetMargins(0.015, 0.4, 0.14, 0.05);
    plotTemplate[2].SetRefFunc("1");
    plotTemplate[2]['X'].SetTickLength(0.06).SetNoExponent();
    plotTemplate[2]['Y'].SetNumDivisions(305).SetTitleCenter();
    plotManager.AddPlotTemplate(plotTemplate);
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    Plot plotTemplate("2d", "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(710, 710, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultDrawingOptionHist2d(colz);
    plotTemplate[0].SetDefaultTextFont(43);
    plotTemplate[0].SetDefaultTextSize(24);
    plotTemplate[0].SetTransparent();
    plotTemplate[0].SetMargins(0.07, 0.14, 0.12, 0.18);
    plotTemplate[0]['X'].SetTitleSize(28).SetTitleOffset(1.2).SetMaxDigits(3);
    plotTemplate[0]['Y'].SetTitleSize(28).SetTitleOffset(1.2).SetMaxDigits(3);
    plotTemplate[0]['Z'].SetTitleSize(28).SetTitleOffset(1.6).SetMaxDigits(3);
    plotTemplate[0].SetRedrawAxes();
    plotTemplate[1].SetPosition(0., 0., 1., 1.);
    plotManager.AddPlotTemplate(plotTemplate);
  } // -----------------------------------------------------------------------
}
