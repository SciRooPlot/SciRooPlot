/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot -------------------------------------
 * Copyright (c) 2019-2026 Mario Krüger
 * Contact: mario.kruger@cern.ch
 * For a full list of contributors please see doc/CONTRIBUTORS.md.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation in version 3 (or later) of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "PlotManager.h"
#include "Plot.h"

#include <math.h>
#include <optional>
#include <utility>
#include <tuple>
#include <vector>
#include <string>
#include <numeric>

#include "TSystem.h"
#include "TEnum.h"
#include "TEnumConstant.h"
#include "TGraphErrors.h"
#include "TH1.h"
#include "TH2.h"
#include "TDirectory.h"

namespace py = pybind11;
using namespace SciRooPlot;

using py::arg;
using py::overload_cast;
using std::nullopt;
using std::optional;
using std::pair;
using std::string;
using std::tuple;
using std::vector;
using Pad = Plot::Pad;
using Data = Pad::Data;
using Ratio = Pad::Ratio;
using Axis = Pad::Axis;
using TextBox = Pad::TextBox;
using LegendBox = Pad::LegendBox;
using LegendEntry = LegendBox::LegendEntry;
constexpr auto ref_int = py::return_value_policy::reference_internal;

void exportDrawingOptions(py::module_& m);
void exportRootConstants(py::module_& m);
void exportPythonDataInterfaces(py::module_& m);
void exportPlotManager(py::module_& m);
void exportPlot(py::module_& m);
void exportPad(py::module_& m);
void exportAxis(py::module_& m);
void exportData(py::module_& m);
void exportRatio(py::module_& m);
void exportTextBox(py::module_& m);
void exportLegendBox(py::module_& m);
void exportLegendEntry(py::module_& m);

PYBIND11_MODULE(SciRooPlot, m)
{
  m.doc() = R"pbdoc(
    SciRooPlot - A command-line plotting app to manage and visualize scientific data

    This module provides Python bindings for the SciRooPlot C++ library, allowing
    users to programmatically define and generate publication-ready plots from Python.

    Example usage:
        from SciRooPlot import *

        pm = PlotManager()
        pm.AddDataset("dataset", ["/path/to/file.root"])

        plot = Plot("name", "group")
        plot[1].AddData("histo", "dataset", "my label")

        pm.AddPlot(plot)
        pm.CreatePlots(mode="pdf")
  )pbdoc";

  // add path to root lib to be independent of user environment
  py::module_ sys = py::module_::import("sys");
  py::list path = sys.attr("path");
  path.insert(0, SCIROOPLOT_ROOT_PYTHON_DIR);

  exportDrawingOptions(m);
  exportRootConstants(m);
  exportPythonDataInterfaces(m);
  exportLegendEntry(m);
  exportLegendBox(m);
  exportTextBox(m);
  exportData(m);
  exportRatio(m);
  exportAxis(m);
  exportPad(m);
  exportPlot(m);
  exportPlotManager(m);
}

void exportDrawingOptions(py::module_& m)
{
#define OPT(name, ...) .value(#name, drawing_options_t::name)
  py::enum_<drawing_options_t>(m, "drawing_options_t") DRAWING_OPTIONS.export_values();
#undef OPT
}

void exportRootConstants(py::module_& m)
{
  gSystem->Load("libGraf");
  for (auto enumName : {"EColor", "ELineStyle", "EMarkerStyle", "EFillStyle", "EColorPalette"}) {
    TEnum* e = TEnum::GetEnum(enumName);
    if (!e) continue;
    TIter next(e->GetConstants());
    while (auto* obj = next()) {
      auto* c = static_cast<TEnumConstant*>(obj);
      m.attr(c->GetName()) = py::int_(c->GetValue());
    }
  }
}

void exportPlotManager(py::module_& m)
{
  py::class_<PlotManager>(m, "PlotManager")
    .def(py::init<>())
    .def("AddDataset", overload_cast<const string&, const vector<string>&>(&PlotManager::AddDataset), arg("dataset"), arg("inputFiles"))
    .def("AddDataset", overload_cast<const string&, const string&>(&PlotManager::AddDataset), arg("dataset"), arg("inputFile"))
    .def("AddDataset", [](PlotManager& self, const std::string& dataset, py::list objs) { py::module_ ROOT = py::module_::import("ROOT"); py::object addressof = ROOT.attr("addressof"); std::vector<TObject*> v; for (auto o : objs) v.push_back(reinterpret_cast<TObject*>(addressof(o).cast<std::uintptr_t>())); self.AddDataset(dataset, v); }, py::arg("dataset"), py::arg("inputData"))
    .def("AddDataset", [](PlotManager& self, const std::string& dataset, py::object obj) { py::module_ ROOT = py::module_::import("ROOT"); py::object addressof = ROOT.attr("addressof"); auto ptr = reinterpret_cast<TObject*>(addressof(obj).cast<std::uintptr_t>()); self.AddDataset(dataset, ptr); }, py::arg("dataset"), py::arg("inputData"))
    .def("SaveDatasets", &PlotManager::SaveDatasets, arg("configFile"))
    .def("LoadDatasets", &PlotManager::LoadDatasets, arg("configFile"))
    .def("AddPlot", &PlotManager::AddPlot, arg("plot"))
    .def("AddPlotTemplate", &PlotManager::AddPlotTemplate, arg("plotTemplate"))
    .def("AddColorPlot", &PlotManager::AddColorPlot, arg("name"), arg("group"), arg("colors") = vector<int32_t>{})
    .def("ListPlots", &PlotManager::ListPlots)
    .def("LoadPlots", &PlotManager::LoadPlots, arg("file"), arg("group") = ".*", arg("name") = ".*")
    .def("SavePlots", &PlotManager::SavePlots, arg("file"), arg("group") = "", arg("names") = vector<string>{})
    .def("CreatePlots", &PlotManager::CreatePlots, arg("mode") = "pdf", arg("group") = "", arg("names") = vector<string>())
    .def("SetOutputDirectory", &PlotManager::SetOutputDirectory, arg("path"))
    .def("SaveProject", &PlotManager::SaveProject, arg("projectName"))
    .def_static("GetPlotTemplate", overload_cast<const string&, double_t>(&PlotManager::GetPlotTemplate), arg("plotTemplateName") = "1d", arg("screenResolution") = 100)
    .def_static("GetProjectSettings", &PlotManager::GetProjectSettings, arg("projectName") = "");
}

void exportPlot(py::module_& m)
{
  py::class_<Plot>(m, "Plot")
    .def(py::init<const string&, const string&>(), arg("name"), arg("group"))
    .def(py::init<const string&, const string&, const optional<string>&>(), arg("name"), arg("group"), arg("plotTemplateName") = nullopt)
    //.def("Print", &Plot::Print)
    .def("__getitem__", [](Plot& self, uint8_t padID) -> Pad& { return self[padID]; }, ref_int)
    .def("__iadd__", [](Plot& self, const Plot& other) -> Plot& { self += other; return self; }, py::is_operator())
    .def("__add__", [](const Plot& a, const Plot& b) -> Plot { return a + b; }, py::is_operator())
    .def("GetPad", &Plot::GetPad, arg("padID"), ref_int)
    .def("AppendGroup", &Plot::AppendGroup, arg("subgroup"))
    .def("SetPlotTemplateName", &Plot::SetPlotTemplateName, arg("plotTemplateName"))
    .def("SetDimensions", &Plot::SetDimensions, arg("width"), arg("height"), arg("fixAspectRatio") = false)
    .def("SetWidth", &Plot::SetWidth, arg("width"))
    .def("SetHeight", &Plot::SetHeight, arg("height"))
    .def("SetFixAspectRatio", &Plot::SetFixAspectRatio, arg("fixAspectRatio") = true)
    .def("SetPaintColorWheel", &Plot::SetPaintColorWheel, arg("paintColorWheel") = true)
    .def("SetFill", &Plot::SetFill, arg("color"), arg("style") = nullopt, arg("alpha") = nullopt)
    .def("SetFillColor", &Plot::SetFillColor, arg("color"))
    .def("SetFillStyle", &Plot::SetFillStyle, arg("style"))
    .def("SetFillAlpha", &Plot::SetFillAlpha, arg("alpha"))
    .def("SetTransparent", &Plot::SetTransparent);
}

void exportPad(py::module_& m)
{
  py::class_<Pad>(m, "Pad")
    .def("Print", &Pad::Print)
    .def("AddData", overload_cast<const string&, const string&, const optional<string>&>(&Pad::AddData), arg("name"), arg("dataset"), arg("label") = nullopt, ref_int)
    .def("AddData", overload_cast<const string&, const Data&, const optional<string>&>(&Pad::AddData), arg("name"), arg("dataTemplate"), arg("label") = nullopt, ref_int)
    .def("AddFunction", &Pad::AddFunction, arg("function"), arg("label") = nullopt, ref_int)
    .def("AddPoints", overload_cast<vector<double_t>, vector<double_t>, const optional<string>&>(&Pad::AddPoints), arg("x"), arg("y"), arg("label") = nullopt, ref_int)
    .def("AddPoints", overload_cast<vector<pair<double_t, double_t>>, const optional<string>&>(&Pad::AddPoints), arg("positions"), arg("label") = nullopt, ref_int)
    .def("AddLine", &Pad::AddLine, arg("pos1"), arg("pos2"), arg("label") = nullopt, ref_int)
    .def("SetPosition", &Pad::SetPosition, arg("xlow"), arg("ylow"), arg("xup"), arg("yup"), ref_int)
    .def("SetMargins", &Pad::SetMargins, arg("top"), arg("bottom"), arg("left"), arg("right"), ref_int)
    .def("SetLeftMargin", &Pad::SetLeftMargin, arg("left"), ref_int)
    .def("SetRightMargin", &Pad::SetRightMargin, arg("right"), ref_int)
    .def("SetTopMargin", &Pad::SetTopMargin, arg("top"), ref_int)
    .def("SetBottomMargin", &Pad::SetBottomMargin, arg("bottom"), ref_int)
    .def("SetView", &Pad::SetView, arg("theta"), arg("phi"), ref_int)
    .def("SetPalette", overload_cast<int32_t>(&Pad::SetPalette), arg("palette"), ref_int)
    .def("SetPalette", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetPalette), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultTextSize", &Pad::SetDefaultTextSize, arg("size"), ref_int)
    .def("SetDefaultTextColor", &Pad::SetDefaultTextColor, arg("color"), ref_int)
    .def("SetDefaultTextAlpha", &Pad::SetDefaultTextAlpha, arg("alpha"), ref_int)
    .def("SetDefaultAlpha", &Pad::SetDefaultAlpha, arg("alpha"), ref_int)
    .def("SetDefaultColors", overload_cast<const vector<int16_t>&>(&Pad::SetDefaultColors), arg("colors"), ref_int)
    .def("SetDefaultColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultMarkerAlpha", &Pad::SetDefaultMarkerAlpha, arg("alpha"), ref_int)
    .def("SetDefaultMarkerSize", &Pad::SetDefaultMarkerSize, arg("size"), ref_int)
    .def("SetDefaultMarkerColors", overload_cast<const vector<int16_t>&>(&Pad::SetDefaultMarkerColors), arg("colors"), ref_int)
    .def("SetDefaultMarkerColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultMarkerColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultMarkerStyles", &Pad::SetDefaultMarkerStyles, arg("styles"), ref_int)
    .def("SetDefaultLineAlpha", &Pad::SetDefaultLineAlpha, arg("alpha"), ref_int)
    .def("SetDefaultLineWidth", &Pad::SetDefaultLineWidth, arg("width"), ref_int)
    .def("SetDefaultLineColors", overload_cast<const vector<int16_t>&>(&Pad::SetDefaultLineColors), arg("colors"), ref_int)
    .def("SetDefaultLineColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultLineColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultLineStyles", &Pad::SetDefaultLineStyles, arg("styles"), ref_int)
    .def("SetDefaultFillAlpha", &Pad::SetDefaultFillAlpha, arg("alpha"), ref_int)
    .def("SetDefaultFillColors", overload_cast<const vector<int16_t>&>(&Pad::SetDefaultFillColors), arg("colors"), ref_int)
    .def("SetDefaultFillColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultFillColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultFillStyles", &Pad::SetDefaultFillStyles, arg("styles"), ref_int)
    .def("SetDefaultDrawingOptionGraph", &Pad::SetDefaultDrawingOptionGraph, arg("drawingOption"), ref_int)
    .def("SetDefaultDrawingOptionHist", &Pad::SetDefaultDrawingOptionHist, arg("drawingOption"), ref_int)
    .def("SetDefaultDrawingOptionHist2d", &Pad::SetDefaultDrawingOptionHist2d, arg("drawingOption"), ref_int)
    .def("SetDefaultCandleBoxRange", &Pad::SetDefaultCandleBoxRange, arg("candleOption"), ref_int)
    .def("SetDefaultCandleWhiskerRange", &Pad::SetDefaultCandleWhiskerRange, arg("candleOption"), ref_int)
    .def("SetFill", &Pad::SetFill, arg("color"), arg("style") = nullopt, arg("alpha") = nullopt, ref_int)
    .def("SetFillColor", &Pad::SetFillColor, arg("color"), ref_int)
    .def("SetFillStyle", &Pad::SetFillStyle, arg("style"), ref_int)
    .def("SetFillAlpha", &Pad::SetFillAlpha, arg("alpha"), ref_int)
    .def("SetTransparent", &Pad::SetTransparent, ref_int)
    .def("SetFrameFill", &Pad::SetFrameFill, arg("color"), arg("style") = nullopt, arg("alpha") = nullopt, ref_int)
    .def("SetFrameFillColor", &Pad::SetFrameFillColor, arg("color"), ref_int)
    .def("SetFrameFillStyle", &Pad::SetFrameFillStyle, arg("style"), ref_int)
    .def("SetFrameFillAlpha", &Pad::SetFrameFillAlpha, arg("alpha"), ref_int)
    .def("SetFrameBorder", &Pad::SetFrameBorder, arg("color"), arg("style") = nullopt, arg("width") = nullopt, arg("alpha") = nullopt, ref_int)
    .def("SetFrameBorderColor", &Pad::SetFrameBorderColor, arg("color"), ref_int)
    .def("SetFrameBorderAlpha", &Pad::SetFrameBorderAlpha, arg("alpha"), ref_int)
    .def("SetFrameBorderStyle", &Pad::SetFrameBorderStyle, arg("style"), ref_int)
    .def("SetFrameBorderWidth", &Pad::SetFrameBorderWidth, arg("width"), ref_int)
    .def("SetFrameTransparent", &Pad::SetFrameTransparent, ref_int)
    .def("SetRedrawAxes", &Pad::SetRedrawAxes, arg("redraw") = true, ref_int)
    .def("__getitem__", [](Pad& self, const char axis) -> Axis& { return self[axis]; }, ref_int)
    .def("GetAxis", &Pad::GetAxis, arg("axis"), ref_int)
    .def("AddRatio", overload_cast<const string&, const string&, const string&, const string&, const optional<string>&>(&Pad::AddRatio), arg("numeratorName"), arg("numeratorDataset"), arg("denominatorName"), arg("denominatorDataset"), arg("label") = nullopt, ref_int)
    .def("AddRatio", overload_cast<const string&, const Data&, const string&, const string&, const optional<string>&>(&Pad::AddRatio), arg("numeratorName"), arg("numeratorLayout"), arg("denominatorName"), arg("denominatorDataset"), arg("label") = nullopt, ref_int)
    .def("AddRatio", overload_cast<const string&, const Data&, const string&, const Data&, const optional<string>&>(&Pad::AddRatio), arg("numeratorName"), arg("numeratorLayout"), arg("denominatorName"), arg("denominatorLayout"), arg("label") = nullopt, ref_int)
    .def("AddRatio", overload_cast<const string&, const string&, const string&, const Data&, const optional<string>&>(&Pad::AddRatio), arg("numeratorName"), arg("numeratorDataset"), arg("denominatorName"), arg("denominatorLayout"), arg("label") = nullopt, ref_int)
    .def("AddText", overload_cast<double_t, double_t, const string&>(&Pad::AddText), arg("xPos"), arg("yPos"), arg("text"), ref_int)
    .def("AddText", overload_cast<const string&>(&Pad::AddText), arg("text"), ref_int)
    .def("AddLegend", overload_cast<double_t, double_t, const optional<string>&>(&Pad::AddLegend), arg("xPos"), arg("yPos"), arg("title") = nullopt, ref_int)
    .def("AddLegend", overload_cast<const optional<string>&>(&Pad::AddLegend), arg("title") = nullopt, ref_int)
    .def("GetText", &Pad::GetText, arg("textID"), ref_int)
    .def("GetLegend", &Pad::GetLegend, arg("legendID"), ref_int)
    .def("SetRefFunc", &Pad::SetRefFunc, arg("refFunc"), ref_int)
    .def("__call__", [](Pad& self, uint8_t dataID) -> Data& { return self(dataID); }, ref_int)
    .def("GetData", overload_cast<uint8_t>(&Pad::GetData), arg("dataID"), ref_int);
}

void exportData(py::module_& m)
{
  py::class_<Data>(m, "Data")
    .def(py::init<>())
    .def(py::init<const string&, const string&, const optional<string>&>(), arg("name"), arg("dataset"), arg("label") = nullopt)
    .def("Print", &Data::Print)
    .def("AsRatio", &Data::AsRatio, ref_int)
    .def("GetDataset", &Data::GetDataset)
    .def("SetDataset", &Data::SetDataset, arg("dataset"), ref_int)
    .def("SetLayout", &Data::SetLayout, arg("dataLayout"), ref_int)
    .def("ApplyLayout", &Data::ApplyLayout, arg("dataLayout"), ref_int)
    .def("SetRangeX", &Data::SetRangeX, arg("min"), arg("max"), ref_int)
    .def("SetMinRangeX", &Data::SetMinRangeX, arg("min"), ref_int)
    .def("SetMaxRangeX", &Data::SetMaxRangeX, arg("max"), ref_int)
    .def("UnsetRangeX", &Data::UnsetRangeX, ref_int)
    .def("SetRangeY", &Data::SetRangeY, arg("min"), arg("max"), ref_int)
    .def("SetMinRangeY", &Data::SetMinRangeY, arg("min"), ref_int)
    .def("SetMaxRangeY", &Data::SetMaxRangeY, arg("max"), ref_int)
    .def("UnsetRangeY", &Data::UnsetRangeY, ref_int)
    .def("SetScaleMinimum", &Data::SetScaleMinimum, arg("scaleFactor"), ref_int)
    .def("SetScaleMaximum", &Data::SetScaleMaximum, arg("scaleFactor"), ref_int)
    .def("SetShowOverflowBins", &Data::SetShowOverflowBins, arg("showOverflowBins") = true, ref_int)
    .def("SetLegendLabel", &Data::SetLegendLabel, arg("legendLabel"), ref_int)
    .def("SetLegend", &Data::SetLegend, arg("legendID"), ref_int)
    .def("SetOptions", overload_cast<const string&>(&Data::SetOptions), arg("options"), ref_int)
    .def("SetOptions", overload_cast<drawing_options_t>(&Data::SetOptions), arg("optionAlias"), ref_int)
    .def("UnsetOptions", &Data::UnsetOptions, ref_int)
    .def("SetTextFormat", &Data::SetTextFormat, arg("textFormat"), ref_int)
    .def("SetColor", &Data::SetColor, arg("color"), ref_int)
    .def("SetAlpha", &Data::SetAlpha, arg("alpha"), ref_int)
    .def("SetMarker", &Data::SetMarker, arg("color"), arg("style"), arg("size"), arg("alpha") = nullopt, ref_int)
    .def("SetMarkerColor", &Data::SetMarkerColor, arg("color"), ref_int)
    .def("SetMarkerAlpha", &Data::SetMarkerAlpha, arg("alpha"), ref_int)
    .def("SetMarkerStyle", &Data::SetMarkerStyle, arg("style"), ref_int)
    .def("SetMarkerSize", &Data::SetMarkerSize, arg("size"), ref_int)
    .def("SetLine", &Data::SetLine, arg("color"), arg("style"), arg("width"), arg("alpha") = nullopt, ref_int)
    .def("SetLineColor", &Data::SetLineColor, arg("color"), ref_int)
    .def("SetLineAlpha", &Data::SetLineAlpha, arg("alpha"), ref_int)
    .def("SetLineStyle", &Data::SetLineStyle, arg("style"), ref_int)
    .def("SetLineWidth", &Data::SetLineWidth, arg("width"), ref_int)
    .def("SetFill", &Data::SetFill, arg("color"), arg("style"), arg("alpha") = nullopt, ref_int)
    .def("SetFillColor", &Data::SetFillColor, arg("color"), ref_int)
    .def("SetFillStyle", &Data::SetFillStyle, arg("style"), ref_int)
    .def("SetFillAlpha", &Data::SetFillAlpha, arg("alpha"), ref_int)
    .def("SetDefinesFrame", &Data::SetDefinesFrame, arg("dontDraw") = false, ref_int)
    .def("SetContours", overload_cast<const vector<double>&>(&Data::SetContours), arg("contours"), ref_int)
    .def("SetContours", overload_cast<int32_t>(&Data::SetContours), arg("nContours"), ref_int)
    .def("Normalize", &Data::Normalize, arg("scaleBinWidth") = false, ref_int)
    .def("NormalizeToMaximum", &Data::NormalizeToMaximum, arg("normMaximum") = true, ref_int)
    .def("Scale", &Data::Scale, arg("scaleFactor"), ref_int)
    .def("DivideBinWidth", &Data::DivideBinWidth, arg("divideBinWidth") = true, ref_int)
    .def("RebinX", &Data::RebinX, arg("nGroup"), ref_int)
    .def("RebinY", &Data::RebinY, arg("nGroup"), ref_int)
    .def("RebinXY", &Data::RebinXY, arg("nGroupX"), arg("nGroupY"), ref_int)
    .def("Smooth", &Data::Smooth, arg("nIterSmooth") = 1, ref_int)
    .def("Project", overload_cast<vector<uint8_t>, vector<tuple<uint8_t, double_t, double_t>>, optional<bool>>(&Data::Project), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectX", &Data::ProjectX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectY", &Data::ProjectY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Profile", overload_cast<vector<uint8_t>, vector<tuple<uint8_t, double_t, double_t>>, optional<bool>>(&Data::Profile), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProfileX", &Data::ProfileX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProfileY", &Data::ProfileY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Project", overload_cast<const vector<Data::data_dim_t>&, optional<string>>(&Data::Project), arg("dataDims"), arg("weight") = nullopt, ref_int)
    .def("Project1D", &Data::Project1D, arg("x"), arg("weight") = nullopt, ref_int)
    .def("Project2D", &Data::Project2D, arg("x"), arg("y"), arg("weight") = nullopt, ref_int)
    .def("Profile", overload_cast<const vector<Data::data_dim_t>&, const string&, optional<string>>(&Data::Profile), arg("dataDims"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Profile1D", &Data::Profile1D, arg("x"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Profile2D", &Data::Profile2D, arg("x"), arg("y"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Scatter", overload_cast<const string&, const string&>(&Data::Scatter), arg("x"), arg("y"), ref_int)
    .def("Scatter", overload_cast<const string&, const string&, const string&, const string&>(&Data::Scatter), arg("x"), arg("y"), arg("xErr"), arg("yErr"), ref_int)
    .def("Scatter", overload_cast<const string&, const string&, const string&, const string&, const string&, const string&>(&Data::Scatter), arg("x"), arg("y"), arg("xErrLow"), arg("xErrHigh"), arg("yErrLow"), arg("yErrHigh"), ref_int)
    .def("Define", &Data::Define, arg("key"), arg("value"), ref_int)
    .def("Filter", &Data::Filter, arg("filter"), ref_int)
    .def("Entries", overload_cast<uint32_t>(&Data::Entries), arg("nEntries"), ref_int)
    .def("Entries", overload_cast<uint32_t, uint32_t>(&Data::Entries), arg("entryMin"), arg("entryMax"), ref_int);
}

void exportRatio(py::module_& m)
{
  py::class_<Ratio, Data>(m, "Ratio")
    .def("Print", &Ratio::Print)
    .def("SetIsCorrelated", &Ratio::SetIsCorrelated, arg("isCorrelated") = true, ref_int)
    .def("SetDivideNormalized", &Ratio::SetDivideNormalized, arg("scaleBinWidth") = false, ref_int)
    .def("GetDataset", [](const Ratio& self) -> const string& { return self.GetDataset(); })
    .def("SetDataset", [](Ratio& self, const string& dataset) -> Ratio& { return self.SetDataset(dataset); }, arg("dataset"), ref_int)
    .def("SetLayout", [](Ratio& self, const Data& d) -> Ratio& { return self.SetLayout(d); }, arg("dataLayout"), ref_int)
    .def("ApplyLayout", [](Ratio& self, const Data& d) -> Ratio& { return self.ApplyLayout(d); }, arg("dataLayout"), ref_int)
    .def("SetRangeX", [](Ratio& self, double_t min, double_t max) -> Ratio& { return self.SetRangeX(min, max); }, arg("min"), arg("max"), ref_int)
    .def("SetMaxRangeX", [](Ratio& self, double_t max) -> Ratio& { return self.SetMaxRangeX(max); }, arg("max"), ref_int)
    .def("SetMinRangeX", [](Ratio& self, double_t min) -> Ratio& { return self.SetMinRangeX(min); }, arg("min"), ref_int)
    .def("UnsetRangeX", [](Ratio& self) -> Ratio& { return self.UnsetRangeX(); }, ref_int)
    .def("SetRangeY", [](Ratio& self, double_t min, double_t max) -> Ratio& { return self.SetRangeY(min, max); }, arg("min"), arg("max"), ref_int)
    .def("SetMaxRangeY", [](Ratio& self, double_t max) -> Ratio& { return self.SetMaxRangeY(max); }, arg("max"), ref_int)
    .def("SetMinRangeY", [](Ratio& self, double_t min) -> Ratio& { return self.SetMinRangeY(min); }, arg("min"), ref_int)
    .def("UnsetRangeY", [](Ratio& self) -> Ratio& { return self.UnsetRangeY(); }, ref_int)
    .def("SetScaleMinimum", [](Ratio& self, double_t v) -> Ratio& { return self.SetScaleMinimum(v); }, arg("scaleFactor"), ref_int)
    .def("SetScaleMaximum", [](Ratio& self, double_t v) -> Ratio& { return self.SetScaleMaximum(v); }, arg("scaleFactor"), ref_int)
    .def("SetShowOverflowBins", [](Ratio& self, bool v = true) -> Ratio& { return self.SetShowOverflowBins(v); }, arg("showOverflowBins") = true, ref_int)
    .def("SetLegendLabel", [](Ratio& self, const string& s) -> Ratio& { return self.SetLegendLabel(s); }, arg("legendLabel"), ref_int)
    .def("SetLegend", [](Ratio& self, uint8_t id) -> Ratio& { return self.SetLegend(id); }, arg("legendID"), ref_int)
    .def("SetOptions", [](Ratio& self, const string& s) -> Ratio& { return self.SetOptions(s); }, arg("options"), ref_int)
    .def("UnsetOptions", [](Ratio& self) -> Ratio& { return self.UnsetOptions(); }, ref_int)
    .def("SetTextFormat", [](Ratio& self, const string& s) -> Ratio& { return self.SetTextFormat(s); }, arg("textFormat"), ref_int)
    .def("SetColor", [](Ratio& self, int16_t c) -> Ratio& { return self.SetColor(c); }, arg("color"), ref_int)
    .def("SetAlpha", [](Ratio& self, float_t a) -> Ratio& { return self.SetAlpha(a); }, arg("alpha"), ref_int)
    .def("SetMarker", [](Ratio& self, int16_t c, int16_t s, float_t sz, optional<float_t> a) -> Ratio& { return self.SetMarker(c, s, sz, a); }, arg("color"), arg("style"), arg("size"), arg("alpha") = nullopt, ref_int)
    .def("SetMarkerColor", [](Ratio& self, int16_t c) -> Ratio& { return self.SetMarkerColor(c); }, arg("color"), ref_int)
    .def("SetMarkerAlpha", [](Ratio& self, float_t a) -> Ratio& { return self.SetMarkerAlpha(a); }, arg("alpha"), ref_int)
    .def("SetMarkerStyle", [](Ratio& self, int16_t s) -> Ratio& { return self.SetMarkerStyle(s); }, arg("style"), ref_int)
    .def("SetMarkerSize", [](Ratio& self, float_t s) -> Ratio& { return self.SetMarkerSize(s); }, arg("size"), ref_int)
    .def("SetLine", [](Ratio& self, int16_t c, int16_t s, float_t w, optional<float_t> a) -> Ratio& { return self.SetLine(c, s, w, a); }, arg("color"), arg("style"), arg("width"), arg("alpha") = nullopt, ref_int)
    .def("SetLineColor", [](Ratio& self, int16_t c) -> Ratio& { return self.SetLineColor(c); }, arg("color"), ref_int)
    .def("SetLineAlpha", [](Ratio& self, float_t a) -> Ratio& { return self.SetLineAlpha(a); }, arg("alpha"), ref_int)
    .def("SetLineStyle", [](Ratio& self, int16_t s) -> Ratio& { return self.SetLineStyle(s); }, arg("style"), ref_int)
    .def("SetLineWidth", [](Ratio& self, float_t w) -> Ratio& { return self.SetLineWidth(w); }, arg("width"), ref_int)
    .def("SetFill", [](Ratio& self, int16_t c, int16_t s, optional<float_t> a) -> Ratio& { return self.SetFill(c, s, a); }, arg("color"), arg("style"), arg("alpha") = nullopt, ref_int)
    .def("SetFillColor", [](Ratio& self, int16_t c) -> Ratio& { return self.SetFillColor(c); }, arg("color"), ref_int)
    .def("SetFillStyle", [](Ratio& self, int16_t s) -> Ratio& { return self.SetFillStyle(s); }, arg("style"), ref_int)
    .def("SetFillAlpha", [](Ratio& self, float_t a) -> Ratio& { return self.SetFillAlpha(a); }, arg("alpha"), ref_int)
    .def("SetDefinesFrame", [](Ratio& self, bool v = false) -> Ratio& { return self.SetDefinesFrame(v); }, arg("dontDraw") = false, ref_int)
    .def("SetContours", [](Ratio& self, const vector<double>& v) -> Ratio& { return self.SetContours(v); }, arg("contours"), ref_int)
    .def("SetContours", [](Ratio& self, int32_t n) -> Ratio& { return self.SetContours(n); }, arg("nContours"), ref_int)
    .def("Normalize", [](Ratio& self, bool b) -> Ratio& { return self.Normalize(b); }, arg("scaleBinWidth") = false, ref_int)
    .def("NormalizeToMaximum", [](Ratio& self, bool b) -> Ratio& { return self.NormalizeToMaximum(b); }, arg("normMaximum") = true, ref_int)
    .def("Scale", [](Ratio& self, double_t v) -> Ratio& { return self.Scale(v); }, arg("scaleFactor"), ref_int)
    .def("DivideBinWidth", [](Ratio& self, bool b) -> Ratio& { return self.DivideBinWidth(b); }, arg("divideBinWidth") = true, ref_int)
    .def("RebinX", [](Ratio& self, uint16_t n) -> Ratio& { return self.RebinX(n); }, arg("nGroup"), ref_int)
    .def("RebinY", [](Ratio& self, uint16_t n) -> Ratio& { return self.RebinY(n); }, arg("nGroup"), ref_int)
    .def("RebinXY", [](Ratio& self, uint16_t x, uint16_t y) -> Ratio& { return self.RebinXY(x, y); }, arg("nGroupX"), arg("nGroupY"), ref_int)
    .def("Smooth", [](Ratio& self, uint16_t n = 1) -> Ratio& { return self.Smooth(n); }, arg("nIterSmooth") = 1, ref_int)
    .def("Numer", &Ratio::Numer, ref_int)
    .def("Denom", &Ratio::Denom, ref_int)
    .def("Project", overload_cast<vector<uint8_t>, vector<tuple<uint8_t, double_t, double_t>>, optional<bool>>(&Ratio::Project), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectX", &Ratio::ProjectX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectY", &Ratio::ProjectY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Profile", overload_cast<vector<uint8_t>, vector<tuple<uint8_t, double_t, double_t>>, optional<bool>>(&Ratio::Profile), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProfileX", &Ratio::ProfileX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProfileY", &Ratio::ProfileY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Project", overload_cast<const vector<Ratio::data_dim_t>&, optional<string>>(&Ratio::Project), arg("dataDims"), arg("weight") = nullopt, ref_int)
    .def("Project1D", &Ratio::Project1D, arg("x"), arg("weight") = nullopt, ref_int)
    .def("Project2D", &Ratio::Project2D, arg("x"), arg("y"), arg("weight") = nullopt, ref_int)
    .def("Profile", overload_cast<const vector<Ratio::data_dim_t>&, const string&, optional<string>>(&Ratio::Profile), arg("dataDims"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Profile1D", &Ratio::Profile1D, arg("x"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Profile2D", &Ratio::Profile2D, arg("x"), arg("y"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Scatter", overload_cast<const string&, const string&>(&Ratio::Scatter), arg("x"), arg("y"), ref_int)
    .def("Scatter", overload_cast<const string&, const string&, const string&, const string&>(&Ratio::Scatter), arg("x"), arg("y"), arg("xErr"), arg("yErr"), ref_int)
    .def("Scatter", overload_cast<const string&, const string&, const string&, const string&, const string&, const string&>(&Ratio::Scatter), arg("x"), arg("y"), arg("xErrLow"), arg("xErrHigh"), arg("yErrLow"), arg("yErrHigh"), ref_int)
    .def("Define", &Ratio::Define, arg("key"), arg("value"), ref_int)
    .def("Filter", &Ratio::Filter, arg("filter"), ref_int)
    .def("Entries", overload_cast<uint32_t>(&Ratio::Entries), arg("nEntries"), ref_int)
    .def("Entries", overload_cast<uint32_t, uint32_t>(&Ratio::Entries), arg("entryMin"), arg("entryMax"), ref_int);
}

void exportAxis(py::module_& m)
{
  py::class_<Axis>(m, "Axis")
    .def("Print", &Axis::Print)
    .def("SetTitle", &Axis::SetTitle, arg("title"), ref_int)
    .def("SetRange", &Axis::SetRange, arg("min"), arg("max"), ref_int)
    .def("SetMaxRange", &Axis::SetMaxRange, arg("max"), ref_int)
    .def("SetMinRange", &Axis::SetMinRange, arg("min"), ref_int)
    .def("SetColor", &Axis::SetColor, arg("color"), ref_int)
    .def("SetAxisColor", &Axis::SetAxisColor, arg("color"), ref_int)
    .def("SetAxisAlpha", &Axis::SetAxisAlpha, arg("alpha"), ref_int)
    .def("SetNumDivisions", &Axis::SetNumDivisions, arg("numDivisions"), ref_int)
    .def("SetMaxDigits", &Axis::SetMaxDigits, arg("maxDigits"), ref_int)
    .def("SetTickLength", &Axis::SetTickLength, arg("tickLength"), ref_int)
    .def("SetTitleFont", &Axis::SetTitleFont, arg("font"), ref_int)
    .def("SetLabelFont", &Axis::SetLabelFont, arg("font"), ref_int)
    .def("SetTitleSize", &Axis::SetTitleSize, arg("size"), ref_int)
    .def("SetLabelSize", &Axis::SetLabelSize, arg("size"), ref_int)
    .def("SetTitleColor", &Axis::SetTitleColor, arg("color"), ref_int)
    .def("SetLabelColor", &Axis::SetLabelColor, arg("color"), ref_int)
    .def("SetTitleAlpha", &Axis::SetTitleAlpha, arg("alpha"), ref_int)
    .def("SetLabelAlpha", &Axis::SetLabelAlpha, arg("alpha"), ref_int)
    .def("SetTitleOffset", &Axis::SetTitleOffset, arg("offset"), ref_int)
    .def("SetLabelOffset", &Axis::SetLabelOffset, arg("offset"), ref_int)
    .def("SetTitleCenter", &Axis::SetTitleCenter, arg("center") = true, ref_int)
    .def("SetLabelCenter", &Axis::SetLabelCenter, arg("center") = true, ref_int)
    .def("SetLog", &Axis::SetLog, arg("isLog") = true, ref_int)
    .def("SetGrid", &Axis::SetGrid, arg("isGrid") = true, ref_int)
    .def("SetOppositeTicks", &Axis::SetOppositeTicks, arg("isOppositeTicks") = true, ref_int)
    .def("SetNoExponent", &Axis::SetNoExponent, arg("isNoExponent") = true, ref_int)
    .def("SetTimeFormat", &Axis::SetTimeFormat, arg("timeFormat"), ref_int)
    .def("SetTickOrientation", &Axis::SetTickOrientation, arg("tickOrientation"), ref_int);
}

void exportTextBox(py::module_& m)
{
  py::class_<TextBox>(m, "TextBox")
    .def("Print", &TextBox::Print)
    .def("SetText", &TextBox::SetText, arg("text"), ref_int)
    .def("SetPosition", &TextBox::SetPosition, arg("x"), arg("y"), ref_int)
    .def("SetSize", &TextBox::SetSize, arg("width"), arg("height"), ref_int)
    .def("SetUserCoordinates", &TextBox::SetUserCoordinates, arg("isUserCoord") = true, ref_int)
    .def("SetAutoPlacement", &TextBox::SetAutoPlacement, ref_int)
    .def("SetBorder", &TextBox::SetBorder, arg("color"), arg("style"), arg("width"), arg("alpha") = nullopt, ref_int)
    .def("SetBorderColor", &TextBox::SetBorderColor, arg("color"), ref_int)
    .def("SetBorderAlpha", &TextBox::SetBorderAlpha, arg("alpha"), ref_int)
    .def("SetBorderStyle", &TextBox::SetBorderStyle, arg("style"), ref_int)
    .def("SetBorderWidth", &TextBox::SetBorderWidth, arg("width"), ref_int)
    //.def("SetText", overload_cast<int16_t, int16_t, float_t, optional<float_t>>(&TextBox::SetText), arg("color"), arg("font"), arg("size"), arg("alpha") = nullopt, ref_int)
    .def("SetTextColor", &TextBox::SetTextColor, arg("color"), ref_int)
    .def("SetTextAlpha", &TextBox::SetTextAlpha, arg("alpha"), ref_int)
    .def("SetTextFont", &TextBox::SetTextFont, arg("font"), ref_int)
    .def("SetTextSize", &TextBox::SetTextSize, arg("size"), ref_int)
    .def("SetFill", &TextBox::SetFill, arg("color"), arg("style"), arg("alpha"), ref_int)
    .def("SetFillColor", &TextBox::SetFillColor, arg("color"), ref_int)
    .def("SetFillStyle", &TextBox::SetFillStyle, arg("style"), ref_int)
    .def("SetFillAlpha", &TextBox::SetFillAlpha, arg("alpha"), ref_int)
    .def("SetTransparent", &TextBox::SetTransparent, ref_int)
    .def("SetNoBox", &TextBox::SetNoBox, ref_int)
    .def("SetMargin", &TextBox::SetMargin, arg("margin"), ref_int)
    .def("SetLineSpacing", &TextBox::SetLineSpacing, arg("lineSpacing"), ref_int);
}

void exportLegendBox(py::module_& m)
{
  py::class_<LegendBox>(m, "LegendBox")
    .def("Print", &LegendBox::Print)
    .def("GetEntry", &LegendBox::GetEntry, arg("entryID"), ref_int)
    .def("SetTitle", &LegendBox::SetTitle, arg("title"), ref_int)
    .def("SetNumColumns", &LegendBox::SetNumColumns, arg("numColumns"), ref_int)
    .def("SetDefaultDrawStyle", &LegendBox::SetDefaultDrawStyle, arg("drawStyle"), ref_int)
    .def("SetDefaultColor", &LegendBox::SetDefaultColor, arg("color"), ref_int)
    .def("SetDefaultAlpha", &LegendBox::SetDefaultAlpha, arg("alpha"), ref_int)
    .def("SetDefaultLineColor", &LegendBox::SetDefaultLineColor, arg("color"), ref_int)
    .def("SetDefaultLineAlpha", &LegendBox::SetDefaultLineAlpha, arg("alpha"), ref_int)
    .def("SetDefaultLineStyle", &LegendBox::SetDefaultLineStyle, arg("style"), ref_int)
    .def("SetDefaultLineWidth", &LegendBox::SetDefaultLineWidth, arg("width"), ref_int)
    .def("SetDefaultMarkerColor", &LegendBox::SetDefaultMarkerColor, arg("color"), ref_int)
    .def("SetDefaultMarkerAlpha", &LegendBox::SetDefaultMarkerAlpha, arg("alpha"), ref_int)
    .def("SetDefaultMarkerStyle", &LegendBox::SetDefaultMarkerStyle, arg("style"), ref_int)
    .def("SetDefaultMarkerSize", &LegendBox::SetDefaultMarkerSize, arg("size"), ref_int)
    .def("SetDefaultFillColor", &LegendBox::SetDefaultFillColor, arg("color"), ref_int)
    .def("SetDefaultFillAlpha", &LegendBox::SetDefaultFillAlpha, arg("alpha"), ref_int)
    .def("SetDefaultFillStyle", &LegendBox::SetDefaultFillStyle, arg("style"), ref_int)
    .def("SetSymbolColScale", &LegendBox::SetSymbolColScale, arg("scale"), ref_int)
    .def("SetPosition", &LegendBox::SetPosition, arg("x"), arg("y"), ref_int)
    .def("SetSize", &LegendBox::SetSize, arg("width"), arg("height"), ref_int)
    .def("SetUserCoordinates", &LegendBox::SetUserCoordinates, arg("isUserCoord") = true, ref_int)
    .def("SetAutoPlacement", &LegendBox::SetAutoPlacement, ref_int)
    .def("SetBorder", &LegendBox::SetBorder, arg("color"), arg("style"), arg("width"), arg("alpha") = nullopt, ref_int)
    .def("SetBorderColor", &LegendBox::SetBorderColor, arg("color"), ref_int)
    .def("SetBorderAlpha", &LegendBox::SetBorderAlpha, arg("alpha"), ref_int)
    .def("SetBorderStyle", &LegendBox::SetBorderStyle, arg("style"), ref_int)
    .def("SetBorderWidth", &LegendBox::SetBorderWidth, arg("width"), ref_int)
    //.def("SetText", &LegendBox::SetText, arg("color"), arg("font"), arg("size"), arg("alpha"), ref_int)
    .def("SetTextColor", &LegendBox::SetTextColor, arg("color"), ref_int)
    .def("SetTextAlpha", &LegendBox::SetTextAlpha, arg("alpha"), ref_int)
    .def("SetTextFont", &LegendBox::SetTextFont, arg("font"), ref_int)
    .def("SetTextSize", &LegendBox::SetTextSize, arg("size"), ref_int)
    .def("SetFill", &LegendBox::SetFill, arg("color"), arg("style"), arg("alpha"), ref_int)
    .def("SetFillColor", &LegendBox::SetFillColor, arg("color"), ref_int)
    .def("SetFillStyle", &LegendBox::SetFillStyle, arg("style"), ref_int)
    .def("SetFillAlpha", &LegendBox::SetFillAlpha, arg("alpha"), ref_int)
    .def("SetTransparent", &LegendBox::SetTransparent, ref_int)
    .def("SetNoBox", &LegendBox::SetNoBox, ref_int)
    .def("SetMargin", &LegendBox::SetMargin, arg("margin"), ref_int)
    .def("SetLineSpacing", &LegendBox::SetLineSpacing, arg("lineSpacing"), ref_int);
}

void exportLegendEntry(py::module_& m)
{
  py::class_<LegendEntry>(m, "LegendEntry")
    .def("Print", &LegendEntry::Print)
    .def("SetLabel", &LegendEntry::SetLabel, arg("label"), ref_int)
    .def("SetRefData", &LegendEntry::SetRefData, arg("refDataID"), ref_int)
    .def("SetDrawStyle", &LegendEntry::SetDrawStyle, arg("drawStyle"), ref_int)
    .def("SetColor", &LegendEntry::SetColor, arg("color"), ref_int)
    .def("SetAlpha", &LegendEntry::SetAlpha, arg("alpha"), ref_int)
    .def("SetMarkerColor", &LegendEntry::SetMarkerColor, arg("color"), ref_int)
    .def("SetMarkerAlpha", &LegendEntry::SetMarkerAlpha, arg("alpha"), ref_int)
    .def("SetMarkerStyle", &LegendEntry::SetMarkerStyle, arg("style"), ref_int)
    .def("SetMarkerSize", &LegendEntry::SetMarkerSize, arg("size"), ref_int)
    .def("SetLineColor", &LegendEntry::SetLineColor, arg("color"), ref_int)
    .def("SetLineAlpha", &LegendEntry::SetLineAlpha, arg("alpha"), ref_int)
    .def("SetLineStyle", &LegendEntry::SetLineStyle, arg("style"), ref_int)
    .def("SetLineWidth", &LegendEntry::SetLineWidth, arg("width"), ref_int)
    .def("SetFillColor", &LegendEntry::SetFillColor, arg("color"), ref_int)
    .def("SetFillAlpha", &LegendEntry::SetFillAlpha, arg("alpha"), ref_int)
    .def("SetFillStyle", &LegendEntry::SetFillStyle, arg("style"), ref_int)
    .def("SetTextColor", &LegendEntry::SetTextColor, arg("color"), ref_int)
    .def("SetTextAlpha", &LegendEntry::SetTextAlpha, arg("alpha"), ref_int)
    .def("SetTextFont", &LegendEntry::SetTextFont, arg("font"), ref_int)
    .def("SetTextSize", &LegendEntry::SetTextSize, arg("size"), ref_int);
}

void exportPythonDataInterfaces(py::module_& m)
{
  auto graph = [](const std::string& name, const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& xerr, const std::vector<double>& yerr) {
    const auto n = x.size();
    static py::module_ ROOT = py::module_::import("ROOT");
    static py::object BindObject = ROOT.attr("BindObject");
    static py::object TGraphErrorsClass = ROOT.attr("TGraphErrors");

    if (y.size() != n)
      throw std::runtime_error("x and y must have same length");
    if (!xerr.empty() && xerr.size() != n)
      throw std::runtime_error("xerr has wrong size");
    if (!yerr.empty() && yerr.size() != n)
      throw std::runtime_error("yerr has wrong size");

    auto* g = new TGraphErrors(n, x.data(), y.data(), xerr.empty() ? nullptr : xerr.data(), yerr.empty() ? nullptr : yerr.data());
    g->SetName(name.data());
    return BindObject(reinterpret_cast<std::uintptr_t>(g), TGraphErrorsClass);
  };

  auto histo = [&](const std::string& name, const std::vector<double>& v, int bins, py::object range) {
    static py::module_ ROOT = py::module_::import("ROOT");
    static py::object BindObject = ROOT.attr("BindObject");
    static py::object TH1DClass = ROOT.attr("TH1D");

    if (v.empty())
      throw std::runtime_error("empty input");

    double xmin, xmax;

    if (range.is_none()) {
      auto [minIt, maxIt] = std::minmax_element(v.begin(), v.end());
      xmin = *minIt;
      xmax = *maxIt;
    } else {
      auto r = range.cast<std::pair<double, double>>();
      xmin = r.first;
      xmax = r.second;
    }

    auto* h = new TH1D(name.data(), "", bins, xmin, xmax);

    for (double x : v)
      h->Fill(x);

    return BindObject(reinterpret_cast<std::uintptr_t>(h), TH1DClass);
  };

  auto histo2d = [&](const std::string& name, const std::vector<double>& x, const std::vector<double>& y, py::object bins, py::object range) {
    static py::module_ ROOT = py::module_::import("ROOT");
    static py::object BindObject = ROOT.attr("BindObject");
    static py::object TH2DClass = ROOT.attr("TH2D");

    if (x.size() != y.size())
      throw std::runtime_error("x and y must have same length");

    int nx = 50, ny = 50;
    double xmin, xmax, ymin, ymax;

    auto [minxIt, maxxIt] = std::minmax_element(x.begin(), x.end());
    auto [minyIt, maxyIt] = std::minmax_element(y.begin(), y.end());
    xmin = *minxIt;
    xmax = *maxxIt;
    ymin = *minyIt;
    ymax = *maxyIt;

    if (!bins.is_none()) {
      auto b = bins.cast<std::pair<int, int>>();
      nx = b.first;
      ny = b.second;
    }

    if (!range.is_none()) {
      auto r = range.cast<std::pair<std::pair<double, double>, std::pair<double, double>>>();
      xmin = r.first.first;
      xmax = r.first.second;
      ymin = r.second.first;
      ymax = r.second.second;
    }

    auto* h = new TH2D(name.c_str(), "", nx, xmin, xmax, ny, ymin, ymax);

    for (size_t i = 0; i < x.size(); ++i)
      h->Fill(x[i], y[i]);

    h->SetName(name.c_str());

    return BindObject(reinterpret_cast<std::uintptr_t>(h), TH2DClass);
  };

  m.def("graph", graph, py::arg("name"), py::arg("x"), py::arg("y"), py::arg("xerr") = std::vector<double>{}, py::arg("yerr") = std::vector<double>{});
  m.def("histo", histo, py::arg("name"), py::arg("values"), py::arg("bins") = 100, py::arg("range") = py::none());
  m.def("histo2d", histo2d, py::arg("name"), py::arg("x"), py::arg("y"), py::arg("bins") = py::none(), py::arg("range") = py::none());
}
