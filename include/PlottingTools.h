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

#ifndef PlotGenerator_h
#define PlotGenerator_h

#include "PlottingFramework.h"
#include "PlotStyle.h"
#include "Plot.h"
using namespace PlottingFramework;
namespace PlottingFramework {

//****************************************************************************************
/**
 * Namespace that contains functionality to generate generate plots using the ROOT framework.
 */
//****************************************************************************************
namespace PlottingTools
{
// main interface
shared_ptr<TCanvas> GeneratePlot(Plot& plot, PlotStyle& plotStyle, TObjArray* availableData);

// internal helpers
template<typename T>
T* GetDataClone(string dataName, TObjArray* availableData);
optional<data_ptr_t> GetDataClone(string dataName, TObjArray* availableData);
TLegend* MakeLegend(shared_ptr<Plot::LegendBox> legendBox, TPad* pad, TObjArray& legendEntries, vector<string> legendTitles, vector<string>& errorStyles);
TPaveText* MakeText(shared_ptr<Plot::TextBox> textBox);
void CutHistogram(TH1* hist, double cutoff, double cutoffLow);
void CutGraph(TGraph* graph, double cutoff, double cutoffLow);
TGraph* DivideTSpline(TGraph* numerator, TGraph* denominator);
TH1* DivideTSpline(TH1* numerator, TH1* denominator);
} // end namespace PlottingTools
} // end namespace PlottingFramework
#endif /* PlotGenerator_h */
