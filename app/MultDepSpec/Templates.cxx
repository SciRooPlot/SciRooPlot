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
 * Defining default layouts for the plots.
 */
//****************************************************************************************
void MultDepSpec::DefinePlotTemplates(PlotManager& plotManager)
{
  { // -----------------------------------------------------------------------
    // template 1d
    Plot templatePlot("1d", "TEMPLATES");
    templatePlot.SetDimensions(710, 710, true);
    templatePlot[0].SetDefaultTextFont(43);
    templatePlot[0].SetDefaultTextSize(24);
    templatePlot[0].SetDefaultMarkerSize(1.2);
    templatePlot[0].SetDefaultLineWidth(2.);
    //templatePlot[0].SetDefaultMarkerStyles({kFullCircle, kOpenCircle});
    templatePlot[0].SetTransparent();
    templatePlot[0].SetMargins(0.07, 0.14, 0.12, 0.07);
    templatePlot[0]["X"].SetTitleOffset(1.1);
    templatePlot[0]["Y"].SetTitleOffset(1.4);
    templatePlot[0]["Z"].SetTitleOffset(1.);
    templatePlot[1].SetPosition(0., 0., 1., 1.);
    plotManager.AddPlotTemplate(templatePlot);
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    // template 1d ratio
    Plot templatePlot("1d_ratio", "TEMPLATES");
    templatePlot.SetDimensions(710, 710, true);
    templatePlot[0].SetDefaultTextFont(43);
    templatePlot[0].SetDefaultTextSize(24);
    templatePlot[0].SetTransparent();
    templatePlot[0]["X"].SetTitleOffset(1.1);
    templatePlot[0]["Y"].SetTitleOffset(1.4);
    templatePlot[1].SetPosition(0., 0.28, 1., 1.);
    templatePlot[1].SetMargins(0.05, 0.0, 0.14, 0.05);
    templatePlot[1]["X"].SetTitleOffset(3.1);
    templatePlot[1]["Y"].SetTitleOffset(1.5);
    templatePlot[2].SetPosition(0., 0., 1., 0.28);
    templatePlot[2].SetMargins(0.015, 0.4, 0.14, 0.05);
    templatePlot[2].SetRefFunc("1");
    templatePlot[2]["X"].SetTickLength(0.06).SetTitleOffset(4.1);
    templatePlot[2]["Y"].SetNumDivisions(305).SetTitleOffset(1.5).SetTitleCenter();
    plotManager.AddPlotTemplate(templatePlot);
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    // template 2d
    Plot templatePlot("2d", "TEMPLATES");
    templatePlot.SetDimensions(710, 710, true);
    templatePlot[0].SetDefaultTextFont(43);
    templatePlot[0].SetDefaultTextSize(24);
    templatePlot[0].SetTransparent();
    templatePlot[0].SetMargins(0.12-0.05, 0.12+0.02, 0.12, 0.12+0.06);
    templatePlot[0]["X"].SetTitleOffset(1.1);
    templatePlot[0]["Y"].SetTitleOffset(1.1);
    templatePlot[0]["Z"].SetTitleOffset(1.6);
    templatePlot[0].SetRedrawAxes();
    templatePlot[1].SetPosition(0., 0., 1., 1.);
    plotManager.AddPlotTemplate(templatePlot);
  } // -----------------------------------------------------------------------
  
}
