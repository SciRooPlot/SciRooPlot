// SciRooPlot
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

using namespace SciRooPlot;

// helper that creates some good looking plot templates you can base your plots on
void DefinePlotTemplates(PlotManager& plotManager);

// ======= minimal working example ==========
// 0. source the ".plotrc" script in the main directory of the SciRooPlot repository (best put this also it in your .bashrc, .bash_profile, or similar)
// 1. copy directory containing this example to some place on your system (outside of the SciRooPlot repository)
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
  // create default plot templates
  DefinePlotTemplates(plotManager);

  if (saveInputDataFiles) {
    // plotManager.AddInputDataFiles("INPUT_REF_NAME", {"PATH_TO_ROOT_FILE", "OR_PATH_TO_TDIRECTORY_OR_TLIST_INSIDE_ROOT_FILE"});
    // NB.: in order to allow running your program from everywhere on your computer it is better NOT to use relative paths
    plotManager.DumpInputDataFiles("inputFiles.XML");
  } else {
    plotManager.LoadInputDataFiles("inputFiles.XML");
  }

  { // -----------------------------------------------------------------------
    // string plotName = "plot1";
    // Plot plot(plotName, "test", "1d");
    // plot[1].AddData("h1", "INPUT_REF_NAME", "LABEL_FOR_POSSIBLE_LEGEND");
    // plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  // OPTION A:
  // directly create all the plots that were defined before
  // plotManager.CreatePlots("", "", {}, "interactive");

  // you can also store them as pdf (for more options see README.md)
  // plotManager.SetOutputDirectory("/some/path/on/system/");
  // plotManager.CreatePlots("", "", {}, "pdf");

  // OPTION B (recommended way):
  // dump the plot definitions to file and then use the plotting app to create the plots:
  // plotManager.DumpPlots("plotDefinitions.XML");

  /*
  string exampleName = "myPlots";
  INFO("");
  INFO("Run once the following commands to configure the plotting app ({} can be replaced by an arbitrary name):", exampleName);
  INFO("  plot-config add {} plotDefinitions /path/to/plotDefinitions.XML", exampleName);
  INFO("  plot-config add {} inputFiles /path/to/inputFiles.XML", exampleName);
  INFO("  plot-config add {} executable /path/to/executable/that/dumps/plots/definitions/create", exampleName);
  INFO("  plot-config add {} outputDir /where/the/output/plots/should/go", exampleName);
  INFO("");

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
  // get default layout settings and apply specific modifications on top (if needed)
  // the definition of these default plots can be found in PlotManager.cxx
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("1d");
    // modify something...
    plotManager.AddPlotTemplate(plotTemplate);
  }
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("1d_ratio");
    plotManager.AddPlotTemplate(plotTemplate);
  }
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("2d");
    plotManager.AddPlotTemplate(plotTemplate);
  }
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("1d_3panels");
    plotManager.AddPlotTemplate(plotTemplate);
  }
  // or create an individual plot layout
  { // -----------------------------------------------------------------------
    Plot plotTemplate("myOwnLayout", "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(710, 710, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultMarkerColors({kRed, kBlue, kOrange});
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
    plotTemplate[1].SetFrameBorderColor(kRed);
    plotTemplate[1].SetFrameBorderWidth(5.);
    plotTemplate[1]['X'].SetColor(kBlue);
    plotTemplate[1]['Y'].SetColor(kBlue);
    plotManager.AddPlotTemplate(plotTemplate);
  } // -----------------------------------------------------------------------
}
