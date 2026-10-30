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
 * The GNU General Public License can be found here: <https://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#include "SciRooPlot/Logger.h"
#include "SciRooPlot/PanelLayout.h"
#include "SciRooPlot/Plot.h"
#include "SciRooPlot/PlotManager.h"

#include <TDirectory.h>
#include <TEnum.h>
#include <TEnumConstant.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>
#include <TTree.h>

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <any>
#include <array>
#include <deque>
#include <functional>
#include <math.h>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "util/Binning.h"

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
using StatsBox = Pad::StatsBox;
using LegendBox = Pad::LegendBox;
using LegendEntry = Pad::LegendEntry;
using Shape = Pad::Shape;
constexpr auto ref_int = py::return_value_policy::reference_internal;

void exportLogging(py::module_& m);
void exportDrawOptions(py::module_& m);
void exportRootConstants(py::module_& m);
void exportPythonDataInterfaces(py::module_& m);
void exportPlotManager(py::module_& m);
void exportPanelLayout(py::module_& m);
void exportPlot(py::module_& m);
void exportPad(py::module_& m);
void exportAxis(py::module_& m);
void exportData(py::module_& m);
void exportRatio(py::module_& m);
void exportTextBox(py::module_& m);
void exportStatsBox(py::module_& m);
void exportLegendBox(py::module_& m);
void exportLegendEntry(py::module_& m);
void exportShape(py::module_& m);

// custom caster for Data::data_dim_t: accepts "var", (var, nBins), (var, edges), (var, nBins, range)
namespace pybind11::detail
{
template <>
struct type_caster<Data::data_dim_t> {
  using T = Data::data_dim_t;
  PYBIND11_TYPE_CASTER(T, const_name("str | tuple"));

  bool load(handle src, bool)
  {
    namespace py = pybind11;
    try {
      if (py::isinstance<py::str>(src)) {
        value = T(src.cast<std::string>());
        return true;
      }
      if (!py::isinstance<py::sequence>(src)) return false;
      auto seq = py::reinterpret_borrow<py::sequence>(src);
      const auto size = seq.size();
      if (size < 1 || size > 3) return false;
      auto var = seq[0].cast<std::string>();
      if (size == 1) {
        value = T(var);
      } else if (size == 2) {
        try {
          value = T(var, seq[1].cast<int32_t>());
        } catch (const py::cast_error&) {
          value = T(var, seq[1].cast<std::vector<double_t>>());
        }
      } else {
        value = T(var, seq[1].cast<int32_t>(), seq[2].cast<std::vector<double_t>>());
      }
      return true;
    } catch (const py::cast_error&) {
      return false;
    } catch (const py::error_already_set&) {
      return false;
    }
  }

  static handle cast(const T& src, return_value_policy, handle)
  {
    return py::make_tuple(src.var, src.nBins, src.edges).release();
  }
};

// custom caster for color_t: a ROOT colour index (kRed + 1), "#rrggbb" or (r, g, b) with components from 0 to 1
template <>
struct type_caster<color_t> {
  PYBIND11_TYPE_CASTER(color_t, const_name("int | str | tuple[float, float, float]"));

  bool load(handle src, bool)
  {
    namespace py = pybind11;
    // an invalid hex string or out-of-range components throw (a ValueError with the reason) instead of failing the match
    if (py::isinstance<py::str>(src)) {
      value = color_t(src.cast<std::string>());
      return true;
    }
    try {
      if (py::isinstance<py::int_>(src) && !py::isinstance<py::bool_>(src)) {
        value = color_t(src.cast<int16_t>());
        return true;
      }
      if (!py::isinstance<py::sequence>(src)) return false;
      auto seq = py::reinterpret_borrow<py::sequence>(src);
      if (seq.size() != 3) return false;
      const std::array<double_t, 3> rgb{seq[0].cast<double_t>(), seq[1].cast<double_t>(), seq[2].cast<double_t>()};
      value = color_t(rgb[0], rgb[1], rgb[2]);
      return true;
    } catch (const py::cast_error&) {
      return false;
    }
  }

  static handle cast(const color_t& src, return_value_policy, handle)
  {
    if (auto index = src.GetIndex()) return py::int_(*index).release();
    return py::str(src.ToString()).release();
  }
};
}  // namespace pybind11::detail

PYBIND11_MODULE(SciRooPlot, m)
{
  m.doc() = R"pbdoc(
    SciRooPlot - ROOT-based C++ and Python framework to define, organize and generate plots

    This module provides Python bindings for the SciRooPlot C++ library, allowing
    users to programmatically define and generate publication-ready plots from Python.

    Example usage:
        from SciRooPlot import *

        pm = PlotManager()
        pm.AddDataSource("dataSource", "/path/to/file.root")

        plot = Plot("name", "group")
        plot[1].AddData("histo", "dataSource", "my label")

        pm.AddPlot(plot)
        pm.GeneratePlots(mode="pdf")
  )pbdoc";

  // add path to root lib to be independent of user environment
  py::module_ sys = py::module_::import("sys");
  py::list path = sys.attr("path");
  path.insert(0, SCIROOPLOT_ROOT_PYTHON_DIR);

  // a class is registered before the classes whose signatures use it, so they show its Python name
  exportLogging(m);
  exportDrawOptions(m);
  exportRootConstants(m);
  exportPythonDataInterfaces(m);
  exportLegendEntry(m);
  exportLegendBox(m);
  exportTextBox(m);
  exportStatsBox(m);
  exportShape(m);
  exportData(m);
  exportRatio(m);
  exportAxis(m);
  exportPad(m);
  exportPanelLayout(m);
  exportPlot(m);
  exportPlotManager(m);
}

void exportLogging(py::module_& m)
{
  // the functions of the logger namespace of C++: SciRooPlot.logger.info("...")
  auto loggerModule = m.def_submodule("logger", "messages in the style of SciRooPlot, shown depending on the verbosity (print: always)");
  loggerModule.def("print", [](std::string_view s) { logger::print("{}", s); }, arg("message"));
  loggerModule.def("info", [](std::string_view s) { logger::info("{}", s); }, arg("message"));
  loggerModule.def("log", [](std::string_view s) { logger::log("{}", s); }, arg("message"));
  loggerModule.def("debug", [](std::string_view s) { logger::debug("{}", s); }, arg("message"));
  loggerModule.def("warning", [](std::string_view s) { logger::warning("{}", s); }, arg("message"));
  loggerModule.def("error", [](std::string_view s) { logger::error("{}", s); }, arg("message"));
}
void exportDrawOptions(py::module_& m)
{
#define OPT(name, ...) .value(#name, draw_option_t::name)
  py::enum_<draw_option_t>(m, "draw_option_t") DRAW_OPTIONS.export_values();
#undef OPT
#define PLACEMENT(name) .value(#name, box_placement_t::name)
  py::enum_<box_placement_t>(m, "box_placement_t") BOX_PLACEMENTS.export_values();
#undef PLACEMENT
}

void exportRootConstants(py::module_& m)
{
  gSystem->Load("libGraf");
  for (auto enumName : {"EColor", "ELineStyle", "EMarkerStyle", "EFillStyle", "EColorPalette", "ETextAlign"}) {
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
    .def(py::init<const string&>(), arg("projectName") = "")
    // one entry point for file names and ROOT objects: separate overloads let pybind11 try to convert a
    // ROOT object into a list of strings first, which fails with a TypeError for objects like TGraph
    .def("AddDataSource", [](PlotManager& self, const std::string& dataSource, py::object input, bool replace) {
      // py::handle, not py::object/py::module_: a static py::object gets decref'd at program exit, possibly after Python has already shut down, which crashes. release() leaks it on purpose instead -- fine since it's a permanent singleton.
      static py::handle ROOT = py::module_::import("ROOT").release();
      static py::handle TObjectClass = py::object(ROOT.attr("TObject")).release();
      auto toTObject = [](py::handle obj) {
        py::object addressof = ROOT.attr("addressof");
        return reinterpret_cast<TObject*>(addressof(obj).cast<std::uintptr_t>());
      };
      if (py::isinstance<py::str>(input)) {
        self.AddDataSource(dataSource, input.cast<std::string>(), replace);
      } else if (py::isinstance(input, TObjectClass)) {
        self.AddDataSource(dataSource, toTObject(input), replace);
      } else if (py::isinstance<py::list>(input) || py::isinstance<py::tuple>(input)) {
        std::vector<std::string> files;
        std::vector<TObject*> objects;
        for (auto item : input) {
          if (py::isinstance<py::str>(item)) {
            files.push_back(item.cast<std::string>());
          } else if (py::isinstance(item, TObjectClass)) {
            objects.push_back(toTObject(item));
          } else {
            throw py::type_error("Inputs must be file names or ROOT TObjects (e.g. histograms, graphs, or trees).");
          }
        }
        if (!files.empty() && !objects.empty()) {
          throw py::type_error("A data source cannot mix file names and ROOT objects.");
        }
        if (!objects.empty()) {
          self.AddDataSource(dataSource, objects, replace);
        } else {
          self.AddDataSource(dataSource, files, replace);
        }
      } else {
        throw py::type_error("Inputs must be a file name, a ROOT TObject, or a list of either.");
      } }, py::arg("dataSource"), py::arg("inputs"), py::arg("replace") = false)
    .def("SaveDataSources", &PlotManager::SaveDataSources, arg("file") = py::none())
    .def("LoadDataSources", &PlotManager::LoadDataSources, arg("file") = py::none(), arg("replace") = false)
    .def("AddPlot", &PlotManager::AddPlot, arg("plot"))
    .def("AddBasePlot", &PlotManager::AddBasePlot, arg("basePlot"))
    .def("AddColorOverview", &PlotManager::AddColorOverview, arg("name"), arg("group"), arg("colors") = vector<int32_t>{})
    .def("ListPlots", &PlotManager::ListPlots)
    .def("ClearDataBuffer", &PlotManager::ClearDataBuffer)
    .def("ClearCanvasRegistry", &PlotManager::ClearCanvasRegistry)
    .def("LoadPlots", &PlotManager::LoadPlots, arg("name") = ".+", arg("group") = ".+", arg("file") = py::none())
    .def("SavePlots", &PlotManager::SavePlots, arg("name") = ".+", arg("group") = ".+", arg("file") = py::none())
    .def("GeneratePlots", &PlotManager::GeneratePlots, arg("mode") = "show", arg("name") = ".+", arg("group") = ".+")
    .def("SetOutputDirectory", &PlotManager::SetOutputDirectory, arg("path"))
    .def("SetPlotsRootFile", &PlotManager::SetPlotsRootFile, arg("fileName"))
    .def("SetDataRootFile", &PlotManager::SetDataRootFile, arg("fileName"))
    .def("SaveProject", &PlotManager::SaveProject)
    .def("GetProjectProperty", &PlotManager::GetProjectProperty, arg("property"))
    .def_static("MakeBasePlot", &PlotManager::MakeBasePlot, arg("name") = "1d", py::arg_v("sizes", PanelLayout(), "PanelLayout()"), py::arg_v("shape", BasePlotShape(), "BasePlotShape()"));
}

void exportPanelLayout(py::module_& m)
{
  py::class_<Panel>(m, "Panel", "One axis frame of a layout, sizes in units of the standard panel.")
    .def(py::init<double_t, double_t>(), arg("width") = 1., arg("height") = 1.)
    .def("ZAxis", &Panel::ZAxis, ref_int)
    .def("Span", &Panel::Span, arg("columns"), arg("rows") = 1, ref_int);
  m.def("Gap", &Gap, "Separates independent panels: both sides keep their axis labels and titles.");
  m.def("Empty", py::overload_cast<>(&Empty), "A cell of the grid without axis frame, its remaining space becomes a pad for texts.");
  m.def("Empty", py::overload_cast<double_t, double_t>(&Empty), arg("width"), arg("height"), "An empty cell with a size of its own, e.g. a strip for a common legend.");
  m.def("ZAxis", py::overload_cast<>(&ZAxis), "A cell holding the colour scale of the z group of the panel to its left.");
  m.def("ZAxis", py::overload_cast<uint8_t>(&ZAxis), arg("padID"), "A cell holding the colour scale of the z group of the given pad.");
  py::class_<PanelLayout>(m, "PanelLayout", "Arrangement of panels (rows from top to bottom) and the lengths of the standard plot in pixels.")
    .def(py::init<vector<vector<Panel>>>(), py::arg_v("rows", vector<vector<Panel>>{{Panel()}}, "[[Panel()]]"))
    .def(py::init<const PanelLayout&, vector<vector<Panel>>>(), arg("sizes"), arg("rows"))
    .def("SetPanelSize", &PanelLayout::SetPanelSize, arg("pixel"), ref_int)
    .def("SetMargins", &PanelLayout::SetMargins, arg("top"), arg("bottom"), arg("left"), arg("right"), ref_int)
    .def("SetZAxisMargin", &PanelLayout::SetZAxisMargin, arg("pixel"), ref_int)
    .def("SetZAxisOffset", &PanelLayout::SetZAxisOffset, arg("pixel"), ref_int)
    .def("SetZAxisWidth", &PanelLayout::SetZAxisWidth, arg("pixel"), ref_int)
    .def("SetPanelSpacing", &PanelLayout::SetPanelSpacing, arg("pixel"), ref_int)
    .def("SetTextSize", &PanelLayout::SetTextSize, arg("pixel"), ref_int)
    .def("SetTitleSize", &PanelLayout::SetTitleSize, arg("pixel"), ref_int)
    .def("SetTickLength", &PanelLayout::SetTickLength, arg("pixel"), ref_int)
    .def("SetLabelOffset", &PanelLayout::SetLabelOffset, arg("pixel"), ref_int)
    .def("SetXTitleOffset", &PanelLayout::SetXTitleOffset, arg("pixel"), ref_int)
    .def("SetYTitleOffset", &PanelLayout::SetYTitleOffset, arg("pixel"), ref_int)
    .def("SetZTitleOffset", &PanelLayout::SetZTitleOffset, arg("pixel"), ref_int)
    .def("SetLineWidth", &PanelLayout::SetLineWidth, arg("width"), ref_int)
    .def("SetMarkerSize", &PanelLayout::SetMarkerSize, arg("size"), ref_int)
    .def("SetCommonTitles", &PanelLayout::SetCommonTitles, arg("common") = true, ref_int);
  py::class_<BasePlotShape>(m, "BasePlotShape", "Proportions of the panels of PlotManager.MakeBasePlot, in units of the standard panel.")
    .def(py::init<>())
    .def("SetAspectRatio", &BasePlotShape::SetAspectRatio, arg("ratio"), ref_int)
    .def("SetRatioHeight", &BasePlotShape::SetRatioHeight, arg("height"), ref_int);
}

// the setter groups of Plot.h (MarkerSetters, LineSetters, ...) for the classes that have them; through a lambda per setter,
// so that each class returns itself to Python (a Ratio its own type, not the Data the forwarded setters of Data would give)
template <class Self, class... Options>
void defMarkerSetters(py::class_<Self, Options...>& cls)
{
  cls
    .def("SetMarker", [](Self& self, int16_t style, float_t size, color_t color, float_t alpha) -> Self& { return self.SetMarker(style, size, color, alpha); }, arg("style"), arg("size"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetMarkerColor", [](Self& self, color_t color) -> Self& { return self.SetMarkerColor(color); }, arg("color"), ref_int)
    .def("SetMarkerAlpha", [](Self& self, float_t alpha) -> Self& { return self.SetMarkerAlpha(alpha); }, arg("alpha"), ref_int)
    .def("SetMarkerStyle", [](Self& self, int16_t style) -> Self& { return self.SetMarkerStyle(style); }, arg("style"), ref_int)
    .def("SetMarkerSize", [](Self& self, float_t size) -> Self& { return self.SetMarkerSize(size); }, arg("size"), ref_int);
}
template <class Self, class... Options>
void defLineSetters(py::class_<Self, Options...>& cls)
{
  cls
    .def("SetLine", [](Self& self, int16_t style, float_t width, color_t color, float_t alpha) -> Self& { return self.SetLine(style, width, color, alpha); }, arg("style"), arg("width"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetLineColor", [](Self& self, color_t color) -> Self& { return self.SetLineColor(color); }, arg("color"), ref_int)
    .def("SetLineAlpha", [](Self& self, float_t alpha) -> Self& { return self.SetLineAlpha(alpha); }, arg("alpha"), ref_int)
    .def("SetLineStyle", [](Self& self, int16_t style) -> Self& { return self.SetLineStyle(style); }, arg("style"), ref_int)
    .def("SetLineWidth", [](Self& self, float_t width) -> Self& { return self.SetLineWidth(width); }, arg("width"), ref_int);
}
template <class Self, class... Options>
void defFillSetters(py::class_<Self, Options...>& cls)
{
  cls
    .def("SetFill", [](Self& self, int16_t style, color_t color, float_t alpha) -> Self& { return self.SetFill(style, color, alpha); }, arg("style"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetFillColor", [](Self& self, color_t color) -> Self& { return self.SetFillColor(color); }, arg("color"), ref_int)
    .def("SetFillStyle", [](Self& self, int16_t style) -> Self& { return self.SetFillStyle(style); }, arg("style"), ref_int)
    .def("SetFillAlpha", [](Self& self, float_t alpha) -> Self& { return self.SetFillAlpha(alpha); }, arg("alpha"), ref_int);
}
template <class Self, class... Options>
void defTextSetters(py::class_<Self, Options...>& cls)
{
  cls
    .def("SetText", [](Self& self, int16_t font, float_t size, color_t color, float_t alpha) -> Self& { return self.SetText(font, size, color, alpha); }, arg("font"), arg("size"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetTextColor", [](Self& self, color_t color) -> Self& { return self.SetTextColor(color); }, arg("color"), ref_int)
    .def("SetTextAlpha", [](Self& self, float_t alpha) -> Self& { return self.SetTextAlpha(alpha); }, arg("alpha"), ref_int)
    .def("SetTextFont", [](Self& self, int16_t font) -> Self& { return self.SetTextFont(font); }, arg("font"), ref_int)
    .def("SetTextSize", [](Self& self, float_t size) -> Self& { return self.SetTextSize(size); }, arg("size"), ref_int);
}
template <class Self, class... Options>
void defBorderSetters(py::class_<Self, Options...>& cls)
{
  cls
    .def("SetBorder", [](Self& self, int16_t style, float_t width, color_t color, float_t alpha) -> Self& { return self.SetBorder(style, width, color, alpha); }, arg("style"), arg("width"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetBorderColor", [](Self& self, color_t color) -> Self& { return self.SetBorderColor(color); }, arg("color"), ref_int)
    .def("SetBorderAlpha", [](Self& self, float_t alpha) -> Self& { return self.SetBorderAlpha(alpha); }, arg("alpha"), ref_int)
    .def("SetBorderStyle", [](Self& self, int16_t style) -> Self& { return self.SetBorderStyle(style); }, arg("style"), ref_int)
    .def("SetBorderWidth", [](Self& self, float_t width) -> Self& { return self.SetBorderWidth(width); }, arg("width"), ref_int);
}

void exportPlot(py::module_& m)
{
  py::class_<Plot> plot(m, "Plot");
  plot
    .def(py::init<const string&, const string&>(), arg("name"), arg("group"))
    .def(py::init<const string&, const string&, const optional<string>&>(), arg("name"), arg("group") = "", arg("basePlot") = nullopt)
    .def(py::init<const string&, const PanelLayout&>(), arg("name"), arg("layout"))
    .def(py::init<const Plot&, const std::string&, const optional<string>&>(), py::arg("other"), py::arg("name"), py::arg("group") = nullopt)
    .def("Print", py::overload_cast<>(&Plot::Print, py::const_))
    .def("__getitem__", [](Plot& self, uint8_t padID) -> Pad& { return self[padID]; }, ref_int)
    .def("__iadd__", [](Plot& self, const Plot& other) -> Plot& { self += other; return self; }, py::is_operator())
    .def("__add__", [](const Plot& a, const Plot& b) -> Plot { return a + b; }, py::is_operator())
    .def("GetPad", &Plot::GetPad, arg("padID"), ref_int)
    .def("SetName", &Plot::SetName, arg("name"), ref_int)
    .def("SetGroup", &Plot::SetGroup, arg("group"), ref_int)
    .def("AppendGroup", &Plot::AppendGroup, arg("subgroup"), ref_int)
    .def("SetBasePlot", &Plot::SetBasePlot, arg("basePlot"), ref_int)
    .def("SetDimensions", &Plot::SetDimensions, arg("width"), arg("height"), arg("fixAspectRatio") = false, ref_int)
    .def("SetWidth", &Plot::SetWidth, arg("width"), ref_int)
    .def("SetHeight", &Plot::SetHeight, arg("height"), ref_int)
    .def("SetFixAspectRatio", &Plot::SetFixAspectRatio, arg("fixAspectRatio") = true, ref_int)
    .def("ShareAxis", &Plot::ShareAxis, arg("axis"), arg("pads"), ref_int);
  defFillSetters(plot);
  plot
    .def("SetTransparent", &Plot::SetTransparent, ref_int);
}

void exportPad(py::module_& m)
{
  py::class_<Pad> pad(m, "Pad");
  pad
    .def("Print", &Pad::Print)
    .def("AddData", overload_cast<const string&, const string&, const optional<string>&>(&Pad::AddData), arg("name"), arg("dataSource"), arg("label") = nullopt, ref_int)
    .def("AddData", overload_cast<const Data&, const optional<string>&>(&Pad::AddData), arg("data"), arg("label") = nullopt, ref_int)
    .def("AddFunction", &Pad::AddFunction, arg("function"), arg("label") = nullopt, ref_int)
    .def("AddPoints", overload_cast<const vector<double_t>&, const vector<double_t>&, const optional<string>&>(&Pad::AddPoints), arg("x"), arg("y"), arg("label") = nullopt, ref_int)
    .def("AddPoints", overload_cast<const vector<pair<double_t, double_t>>&, const optional<string>&>(&Pad::AddPoints), arg("positions"), arg("label") = nullopt, ref_int)
    .def("SetPosition", &Pad::SetPosition, arg("xlow"), arg("ylow"), arg("xup"), arg("yup"), ref_int)
    .def("SetMargins", &Pad::SetMargins, arg("top"), arg("bottom"), arg("left"), arg("right"), ref_int)
    .def("SetLeftMargin", &Pad::SetLeftMargin, arg("left"), ref_int)
    .def("SetRightMargin", &Pad::SetRightMargin, arg("right"), ref_int)
    .def("SetTopMargin", &Pad::SetTopMargin, arg("top"), ref_int)
    .def("SetBottomMargin", &Pad::SetBottomMargin, arg("bottom"), ref_int)
    .def("SetView", &Pad::SetView, arg("theta"), arg("phi"), ref_int)
    .def("SetPalette", overload_cast<int32_t>(&Pad::SetPalette), arg("palette"), ref_int)
    .def("SetPalette", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetPalette), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetPalette", overload_cast<const vector<pair<color_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetPalette), arg("gradient"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultTextSize", &Pad::SetDefaultTextSize, arg("size"), ref_int)
    .def("SetDefaultTextColor", &Pad::SetDefaultTextColor, arg("color"), ref_int)
    .def("SetDefaultTextAlpha", &Pad::SetDefaultTextAlpha, arg("alpha"), ref_int)
    .def("SetDefaultTextFont", &Pad::SetDefaultTextFont, arg("font"), ref_int)
    .def("SetDefaultAlpha", &Pad::SetDefaultAlpha, arg("alpha"), ref_int)
    .def("SetDefaultColors", overload_cast<const vector<color_t>&>(&Pad::SetDefaultColors), arg("colors"), ref_int)
    .def("SetDefaultColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultColors", overload_cast<const vector<pair<color_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultColors), arg("gradient"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultMarkerAlpha", &Pad::SetDefaultMarkerAlpha, arg("alpha"), ref_int)
    .def("SetDefaultMarkerSize", &Pad::SetDefaultMarkerSize, arg("size"), ref_int)
    .def("SetDefaultMarkerColors", overload_cast<const vector<color_t>&>(&Pad::SetDefaultMarkerColors), arg("colors"), ref_int)
    .def("SetDefaultMarkerColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultMarkerColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultMarkerColors", overload_cast<const vector<pair<color_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultMarkerColors), arg("gradient"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultMarkerStyles", &Pad::SetDefaultMarkerStyles, arg("styles"), ref_int)
    .def("SetDefaultLineAlpha", &Pad::SetDefaultLineAlpha, arg("alpha"), ref_int)
    .def("SetDefaultLineWidth", &Pad::SetDefaultLineWidth, arg("width"), ref_int)
    .def("SetDefaultLineColors", overload_cast<const vector<color_t>&>(&Pad::SetDefaultLineColors), arg("colors"), ref_int)
    .def("SetDefaultLineColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultLineColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultLineColors", overload_cast<const vector<pair<color_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultLineColors), arg("gradient"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultLineStyles", &Pad::SetDefaultLineStyles, arg("styles"), ref_int)
    .def("SetDefaultFillAlpha", &Pad::SetDefaultFillAlpha, arg("alpha"), ref_int)
    .def("SetDefaultFillColors", overload_cast<const vector<color_t>&>(&Pad::SetDefaultFillColors), arg("colors"), ref_int)
    .def("SetDefaultFillColors", overload_cast<const vector<tuple<float_t, float_t, float_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultFillColors), arg("rgbEndpoints"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultFillColors", overload_cast<const vector<pair<color_t, float_t>>&, optional<float_t>, optional<int32_t>>(&Pad::SetDefaultFillColors), arg("gradient"), arg("alpha") = nullopt, arg("nColors") = nullopt, ref_int)
    .def("SetDefaultFillStyles", &Pad::SetDefaultFillStyles, arg("styles"), ref_int)
    .def("SetDefaultCandleBoxRange", &Pad::SetDefaultCandleBoxRange, arg("candleOption"), ref_int)
    .def("SetDefaultCandleWhiskerRange", &Pad::SetDefaultCandleWhiskerRange, arg("candleOption"), ref_int);
  defFillSetters(pad);
  pad
    .def("SetTransparent", &Pad::SetTransparent, ref_int)
    .def("SetFrameFill", &Pad::SetFrameFill, arg("style"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetFrameFillColor", &Pad::SetFrameFillColor, arg("color"), ref_int)
    .def("SetFrameFillStyle", &Pad::SetFrameFillStyle, arg("style"), ref_int)
    .def("SetFrameFillAlpha", &Pad::SetFrameFillAlpha, arg("alpha"), ref_int)
    .def("SetFrameBorder", &Pad::SetFrameBorder, arg("style"), arg("width"), arg("color"), arg("alpha") = 1.f, ref_int)
    .def("SetFrameBorderColor", &Pad::SetFrameBorderColor, arg("color"), ref_int)
    .def("SetFrameBorderAlpha", &Pad::SetFrameBorderAlpha, arg("alpha"), ref_int)
    .def("SetFrameBorderStyle", &Pad::SetFrameBorderStyle, arg("style"), ref_int)
    .def("SetFrameBorderWidth", &Pad::SetFrameBorderWidth, arg("width"), ref_int)
    .def("SetFrameTransparent", &Pad::SetFrameTransparent, ref_int)
    .def("SetRedrawAxes", &Pad::SetRedrawAxes, arg("redraw") = true, ref_int)
    .def("__getitem__", [](Pad& self, const char axis) -> Axis& { return self[axis]; }, ref_int)
    .def("GetAxis", &Pad::GetAxis, arg("axis"), ref_int)
    .def("AddRatio", overload_cast<const string&, const string&, const string&, const string&, const optional<string>&>(&Pad::AddRatio), arg("numeratorName"), arg("numeratorDataSource"), arg("denominatorName"), arg("denominatorDataSource"), arg("label") = nullopt, ref_int)
    .def("AddRatio", overload_cast<const Data&, const Data&, const optional<string>&>(&Pad::AddRatio), arg("numerator"), arg("denominator"), arg("label") = nullopt, ref_int)
    .def("AddRatio", overload_cast<const Ratio&, const optional<string>&>(&Pad::AddRatio), arg("ratio"), arg("label") = nullopt, ref_int)
    .def("AddText", overload_cast<double_t, double_t, const string&>(&Pad::AddText), arg("xPos"), arg("yPos"), arg("text"), ref_int)
    .def("AddText", overload_cast<double_t, double_t, const vector<string>&>(&Pad::AddText), arg("xPos"), arg("yPos"), arg("lines"), ref_int)
    .def("AddText", overload_cast<const string&>(&Pad::AddText), arg("text"), ref_int)
    .def("AddText", overload_cast<const vector<string>&>(&Pad::AddText), arg("lines"), ref_int)
    .def("AddText", overload_cast<box_placement_t, const string&>(&Pad::AddText), arg("placement"), arg("text"), ref_int)
    .def("AddText", overload_cast<box_placement_t, const vector<string>&>(&Pad::AddText), arg("placement"), arg("lines"), ref_int)
    .def("AddLegend", overload_cast<double_t, double_t, const optional<string>&>(&Pad::AddLegend), arg("xPos"), arg("yPos"), arg("title") = nullopt, ref_int)
    .def("AddLegend", overload_cast<box_placement_t, const optional<string>&>(&Pad::AddLegend), arg("placement"), arg("title") = nullopt, ref_int)
    .def("AddLegend", overload_cast<const optional<string>&>(&Pad::AddLegend), arg("title") = nullopt, ref_int)
    .def("GetText", &Pad::GetText, arg("textID"), ref_int)
    .def("GetShape", &Pad::GetShape, arg("shapeID"), ref_int)
    .def("AddLine", &Pad::AddLine, arg("pos1"), arg("pos2"), ref_int)
    .def("AddArrow", &Pad::AddArrow, arg("pos1"), arg("pos2"), ref_int)
    .def("AddVerticalLine", &Pad::AddVerticalLine, arg("x"), ref_int)
    .def("AddHorizontalLine", &Pad::AddHorizontalLine, arg("y"), ref_int)
    .def("AddRegion", &Pad::AddRegion, arg("pos1"), arg("pos2"), ref_int)
    .def("AddRegionX", &Pad::AddRegionX, arg("x1"), arg("x2"), ref_int)
    .def("AddRegionY", &Pad::AddRegionY, arg("y1"), arg("y2"), ref_int)
    .def("AddEllipse", &Pad::AddEllipse, arg("center"), arg("radiusX"), arg("radiusY"), ref_int)
    .def("AddPolygon", &Pad::AddPolygon, arg("points"), ref_int)
    .def("GetLegend", &Pad::GetLegend, arg("legendID"), ref_int)
    .def("SetRefFunc", &Pad::SetRefFunc, arg("refFunc"), ref_int)
    .def("SetFrame", overload_cast<const string&, const string&>(&Pad::SetFrame), arg("name"), arg("dataSource"), ref_int)
    .def("SetFrame", overload_cast<const Data&>(&Pad::SetFrame), arg("data"), ref_int)
    .def("__call__", [](Pad& self, uint8_t dataID) -> Data& { return self(dataID); }, ref_int)
    .def("GetData", overload_cast<uint8_t>(&Pad::GetData), arg("dataID"), ref_int);
}

void exportData(py::module_& m)
{
  py::class_<Data> data(m, "Data");
  data
    .def(py::init<>())
    .def(py::init<const string&, const string&, const optional<string>&>(), arg("name"), arg("dataSource"), arg("label") = nullopt)
    .def(py::init([](const py::list& nameAndSource) {
           if (nameAndSource.size() != 2 || !py::isinstance<py::str>(nameAndSource[0]) || !py::isinstance<py::str>(nameAndSource[1])) {
             throw py::type_error("Data can only be created from a list [name, dataSource].");
           }
           return Data(nameAndSource[0].cast<string>(), nameAndSource[1].cast<string>());
         }),
         arg("nameAndSource"))
    .def("__call__", [](const Data& self, const string& name, const optional<string>& dataSource) { return self(name, dataSource); }, arg("name"), arg("dataSource") = nullopt)
    .def("Print", &Data::Print)
    .def("GetDataSource", &Data::GetDataSource)
    .def("SetDataSource", &Data::SetDataSource, arg("dataSource"), ref_int)
    .def("SetLayout", &Data::SetLayout, arg("dataLayout"), ref_int)
    .def("ApplyLayout", &Data::ApplyLayout, arg("dataLayout"), ref_int)
    .def("SetRangeX", overload_cast<double_t, double_t>(&Data::SetRangeX), arg("min"), arg("max"), ref_int)
    .def("SetRangeX", overload_cast<>(&Data::SetRangeX), ref_int)
    .def("SetMinRangeX", &Data::SetMinRangeX, arg("min"), ref_int)
    .def("SetMaxRangeX", &Data::SetMaxRangeX, arg("max"), ref_int)
    .def("SetRangeY", overload_cast<double_t, double_t>(&Data::SetRangeY), arg("min"), arg("max"), ref_int)
    .def("SetRangeY", overload_cast<>(&Data::SetRangeY), ref_int)
    .def("SetMinRangeY", &Data::SetMinRangeY, arg("min"), ref_int)
    .def("SetMaxRangeY", &Data::SetMaxRangeY, arg("max"), ref_int)
    .def("SetShowOverflowBins", &Data::SetShowOverflowBins, arg("showOverflowBins") = true, ref_int)
    .def("SetLegendLabel", &Data::SetLegendLabel, arg("legendLabel"), ref_int)
    .def("SetLegend", &Data::SetLegend, arg("legendID"), ref_int)
    .def("SetLegendEntry", &Data::SetLegendEntry, arg("legendEntry"), ref_int)
    .def("SetDrawOption", overload_cast<const string&>(&Data::SetDrawOption), arg("drawOption"), ref_int)
    .def("SetDrawOption", overload_cast<draw_option_t>(&Data::SetDrawOption), arg("optionAlias"), ref_int)
    .def("SetDrawOption", overload_cast<>(&Data::SetDrawOption), ref_int)
    .def("SetTextFormat", &Data::SetTextFormat, arg("textFormat"), ref_int)
    .def("SetTextAngle", &Data::SetTextAngle, arg("angle"), ref_int)
    .def("SetShowStats", &Data::SetShowStats, arg("format") = "emr", arg("title") = nullopt, ref_int)
    .def("SetStatsBox", &Data::SetStatsBox, arg("statsBox"), ref_int)
    .def("SetColor", &Data::SetColor, arg("color"), ref_int)
    .def("SetAlpha", &Data::SetAlpha, arg("alpha"), ref_int);
  defMarkerSetters(data);
  defLineSetters(data);
  defFillSetters(data);
  data
    .def("SetDefinesFrame", &Data::SetDefinesFrame, ref_int)
    .def("SetKeepInputStyle", &Data::SetKeepInputStyle, arg("keep") = true, ref_int)
    .def("SetContours", overload_cast<const vector<double>&>(&Data::SetContours), arg("contours"), ref_int)
    .def("SetContours", overload_cast<int32_t>(&Data::SetContours), arg("nContours"), ref_int)
    .def("SetNpx", &Data::SetNpx, arg("nx"), arg("ny") = nullopt, ref_int)
    .def("Normalize", &Data::Normalize, arg("isDensity") = false, arg("min") = nullopt, arg("max") = nullopt, ref_int)
    .def("NormalizeToMaximum", &Data::NormalizeToMaximum, arg("min") = nullopt, arg("max") = nullopt, ref_int)
    .def("Scale", &Data::Scale, arg("scaleFactor"), ref_int)
    .def("DivideBinWidth", &Data::DivideBinWidth, arg("divideBinWidth") = true, ref_int)
    .def("Density", &Data::Density, ref_int)
    .def("RebinX", &Data::RebinX, arg("nGroup"), ref_int)
    .def("RebinY", &Data::RebinY, arg("nGroup"), ref_int)
    .def("RebinZ", &Data::RebinZ, arg("nGroup"), ref_int)
    .def("RebinXY", &Data::RebinXY, arg("nGroupX"), arg("nGroupY"), ref_int)
    .def("RebinXYZ", &Data::RebinXYZ, arg("nGroupX"), arg("nGroupY"), arg("nGroupZ"), ref_int)
    .def("ScaleX", &Data::ScaleX, arg("scaleFactor"), ref_int)
    .def("ScaleY", &Data::ScaleY, arg("scaleFactor"), ref_int)
    .def("ScaleZ", &Data::ScaleZ, arg("scaleFactor"), ref_int)
    .def("Smooth", &Data::Smooth, arg("nIterSmooth") = 1, ref_int)
    .def("Cumulative", &Data::Cumulative, arg("forward") = true, ref_int)
    .def("Project", overload_cast<const vector<uint8_t>&, const vector<tuple<uint8_t, double_t, double_t>>&, optional<bool>>(&Data::Project), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectX", &Data::ProjectX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectY", &Data::ProjectY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Profile", overload_cast<const vector<uint8_t>&, const vector<tuple<uint8_t, double_t, double_t>>&, optional<bool>>(&Data::Profile), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
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
    .def("Join", [](Data& self, const string& tree, const vector<string>& keys, const string& alias) -> Data& { return self.Join(tree, keys, alias); }, arg("tree"), arg("keys") = vector<string>{}, arg("alias") = "", ref_int)
    .def("Join", [](Data& self, const std::array<string, 2>& tree, const vector<string>& keys, const string& alias) -> Data& { return self.Join({tree[0], tree[1]}, keys, alias); }, arg("tree"), arg("keys") = vector<string>{}, arg("alias") = "", ref_int)
    .def("Entries", overload_cast<uint32_t>(&Data::Entries), arg("nEntries"), ref_int)
    .def("Entries", overload_cast<uint32_t, uint32_t>(&Data::Entries), arg("entryMin"), arg("entryMax"), ref_int);
  // [name, dataSource] lists can be passed wherever data is expected, e.g. AddRatio(["h", "data"], ["h", "mc"])
  py::implicitly_convertible<py::list, Data>();
}

void exportRatio(py::module_& m)
{
  py::class_<Ratio, Data> ratio(m, "Ratio");
  ratio
    .def(py::init<const Data&, const Data&>(), arg("numerator"), arg("denominator"))
    .def(py::init<const string&, const string&, const string&, const string&, const optional<string>&>(), arg("name"), arg("dataSource"), arg("denomName"), arg("denomDataSource"), arg("label"))
    .def("Print", &Ratio::Print)
    .def("SetIsCorrelated", &Ratio::SetIsCorrelated, arg("isCorrelated") = true, ref_int)
    .def("GetDataSource", [](const Ratio& self) -> const string& { return self.GetDataSource(); })
    .def("SetDataSource", [](Ratio& self, const string& dataSource) -> Ratio& { return self.SetDataSource(dataSource); }, arg("dataSource"), ref_int)
    .def("SetLayout", [](Ratio& self, const Data& d) -> Ratio& { return self.SetLayout(d); }, arg("dataLayout"), ref_int)
    .def("ApplyLayout", [](Ratio& self, const Data& d) -> Ratio& { return self.ApplyLayout(d); }, arg("dataLayout"), ref_int)
    .def("SetRangeX", [](Ratio& self, double_t min, double_t max) -> Ratio& { return self.SetRangeX(min, max); }, arg("min"), arg("max"), ref_int)
    .def("SetRangeX", [](Ratio& self) -> Ratio& { return self.SetRangeX(); }, ref_int)
    .def("SetMaxRangeX", [](Ratio& self, double_t max) -> Ratio& { return self.SetMaxRangeX(max); }, arg("max"), ref_int)
    .def("SetMinRangeX", [](Ratio& self, double_t min) -> Ratio& { return self.SetMinRangeX(min); }, arg("min"), ref_int)
    .def("SetRangeY", [](Ratio& self, double_t min, double_t max) -> Ratio& { return self.SetRangeY(min, max); }, arg("min"), arg("max"), ref_int)
    .def("SetRangeY", [](Ratio& self) -> Ratio& { return self.SetRangeY(); }, ref_int)
    .def("SetMaxRangeY", [](Ratio& self, double_t max) -> Ratio& { return self.SetMaxRangeY(max); }, arg("max"), ref_int)
    .def("SetMinRangeY", [](Ratio& self, double_t min) -> Ratio& { return self.SetMinRangeY(min); }, arg("min"), ref_int)
    .def("SetShowOverflowBins", [](Ratio& self, bool v = true) -> Ratio& { return self.SetShowOverflowBins(v); }, arg("showOverflowBins") = true, ref_int)
    .def("SetLegendLabel", [](Ratio& self, const string& s) -> Ratio& { return self.SetLegendLabel(s); }, arg("legendLabel"), ref_int)
    .def("SetLegend", [](Ratio& self, uint8_t id) -> Ratio& { return self.SetLegend(id); }, arg("legendID"), ref_int)
    .def("SetLegendEntry", [](Ratio& self, const LegendEntry& entry) -> Ratio& { return self.SetLegendEntry(entry); }, arg("legendEntry"), ref_int)
    .def("SetDrawOption", [](Ratio& self, const string& s) -> Ratio& { return self.SetDrawOption(s); }, arg("drawOption"), ref_int)
    .def("SetDrawOption", [](Ratio& self, draw_option_t o) -> Ratio& { return self.SetDrawOption(o); }, arg("optionAlias"), ref_int)
    .def("SetDrawOption", [](Ratio& self) -> Ratio& { return self.SetDrawOption(); }, ref_int)
    .def("SetTextFormat", [](Ratio& self, const string& s) -> Ratio& { return self.SetTextFormat(s); }, arg("textFormat"), ref_int)
    .def("SetTextAngle", [](Ratio& self, float_t angle) -> Ratio& { return self.SetTextAngle(angle); }, arg("angle"), ref_int)
    .def("SetShowStats", [](Ratio& self, const string& f, const optional<string>& t) -> Ratio& { return self.SetShowStats(f, t); }, arg("format") = "emr", arg("title") = nullopt, ref_int)
    .def("SetStatsBox", [](Ratio& self, const StatsBox& box) -> Ratio& { return self.SetStatsBox(box); }, arg("statsBox"), ref_int)
    .def("SetColor", [](Ratio& self, color_t c) -> Ratio& { return self.SetColor(c); }, arg("color"), ref_int)
    .def("SetAlpha", [](Ratio& self, float_t a) -> Ratio& { return self.SetAlpha(a); }, arg("alpha"), ref_int);
  defMarkerSetters(ratio);
  defLineSetters(ratio);
  defFillSetters(ratio);
  ratio
    .def("SetDefinesFrame", [](Ratio& self) -> Ratio& { return self.SetDefinesFrame(); }, ref_int)
    .def("SetKeepInputStyle", [](Ratio& self, bool v = true) -> Ratio& { return self.SetKeepInputStyle(v); }, arg("keep") = true, ref_int)
    .def("SetContours", [](Ratio& self, const vector<double>& v) -> Ratio& { return self.SetContours(v); }, arg("contours"), ref_int)
    .def("SetContours", [](Ratio& self, int32_t n) -> Ratio& { return self.SetContours(n); }, arg("nContours"), ref_int)
    .def("Normalize", [](Ratio& self, bool b, optional<double_t> min, optional<double_t> max) -> Ratio& { return self.Normalize(b, min, max); }, arg("isDensity") = false, arg("min") = nullopt, arg("max") = nullopt, ref_int)
    .def("NormalizeToMaximum", [](Ratio& self, optional<double_t> min, optional<double_t> max) -> Ratio& { return self.NormalizeToMaximum(min, max); }, arg("min") = nullopt, arg("max") = nullopt, ref_int)
    .def("Scale", [](Ratio& self, double_t v) -> Ratio& { return self.Scale(v); }, arg("scaleFactor"), ref_int)
    .def("DivideBinWidth", [](Ratio& self, bool b) -> Ratio& { return self.DivideBinWidth(b); }, arg("divideBinWidth") = true, ref_int)
    .def("Density", [](Ratio& self) -> Ratio& { return self.Density(); }, ref_int)
    .def("RebinX", [](Ratio& self, uint16_t n) -> Ratio& { return self.RebinX(n); }, arg("nGroup"), ref_int)
    .def("RebinY", [](Ratio& self, uint16_t n) -> Ratio& { return self.RebinY(n); }, arg("nGroup"), ref_int)
    .def("RebinZ", [](Ratio& self, uint16_t n) -> Ratio& { return self.RebinZ(n); }, arg("nGroup"), ref_int)
    .def("RebinXY", [](Ratio& self, uint16_t x, uint16_t y) -> Ratio& { return self.RebinXY(x, y); }, arg("nGroupX"), arg("nGroupY"), ref_int)
    .def("RebinXYZ", [](Ratio& self, uint16_t x, uint16_t y, uint16_t z) -> Ratio& { return self.RebinXYZ(x, y, z); }, arg("nGroupX"), arg("nGroupY"), arg("nGroupZ"), ref_int)
    .def("ScaleX", [](Ratio& self, double_t v) -> Ratio& { return self.ScaleX(v); }, arg("scaleFactor"), ref_int)
    .def("ScaleY", [](Ratio& self, double_t v) -> Ratio& { return self.ScaleY(v); }, arg("scaleFactor"), ref_int)
    .def("ScaleZ", [](Ratio& self, double_t v) -> Ratio& { return self.ScaleZ(v); }, arg("scaleFactor"), ref_int)
    .def("Smooth", [](Ratio& self, uint16_t n = 1) -> Ratio& { return self.Smooth(n); }, arg("nIterSmooth") = 1, ref_int)
    .def("Cumulative", [](Ratio& self, bool b = true) -> Ratio& { return self.Cumulative(b); }, arg("forward") = true, ref_int)
    .def("Numer", &Ratio::Numer, ref_int)
    .def("Denom", &Ratio::Denom, ref_int)
    .def("Both", &Ratio::Both, ref_int)
    .def("Result", &Ratio::Result, ref_int)
    .def("Project", overload_cast<const vector<uint8_t>&, const vector<tuple<uint8_t, double_t, double_t>>&, optional<bool>>(&Ratio::Project), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectX", &Ratio::ProjectX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProjectY", &Ratio::ProjectY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Profile", overload_cast<const vector<uint8_t>&, const vector<tuple<uint8_t, double_t, double_t>>&, optional<bool>>(&Ratio::Profile), arg("dims"), arg("ranges") = vector<tuple<uint8_t, double_t, double_t>>{}, arg("isUserCoord") = nullopt, ref_int)
    .def("ProfileX", &Ratio::ProfileX, arg("startY") = 0, arg("endY") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("ProfileY", &Ratio::ProfileY, arg("startX") = 0, arg("endX") = -1, arg("isUserCoord") = nullopt, ref_int)
    .def("Project", overload_cast<const vector<Data::data_dim_t>&, optional<string>>(&Ratio::Project), arg("dataDims"), arg("weight") = nullopt, ref_int)
    .def("Project1D", &Ratio::Project1D, arg("x"), arg("weight") = nullopt, ref_int)
    .def("Project2D", &Ratio::Project2D, arg("x"), arg("y"), arg("weight") = nullopt, ref_int)
    .def("Profile", overload_cast<const vector<Data::data_dim_t>&, const string&, optional<string>>(&Ratio::Profile), arg("dataDims"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Profile1D", &Ratio::Profile1D, arg("x"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Profile2D", &Ratio::Profile2D, arg("x"), arg("y"), arg("profile"), arg("weight") = nullopt, ref_int)
    .def("Scatter", overload_cast<const string&, const string&>(&Ratio::Scatter), arg("x"), arg("y"), ref_int)
    .def("Scatter", overload_cast<const string&, const string&, const string&, const string&>(&Ratio::Scatter), arg("x"), arg("y"), arg("xErr"), arg("yErr"), ref_int)
    .def("Scatter", overload_cast<const string&, const string&, const string&, const string&, const string&, const string&>(&Ratio::Scatter), arg("x"), arg("y"), arg("xErrLow"), arg("xErrHigh"), arg("yErrLow"), arg("yErrHigh"), ref_int)
    .def("Define", &Ratio::Define, arg("key"), arg("value"), ref_int)
    .def("Filter", &Ratio::Filter, arg("filter"), ref_int)
    .def("Join", [](Ratio& self, const string& tree, const vector<string>& keys, const string& alias) -> Ratio& { return self.Join(tree, keys, alias); }, arg("tree"), arg("keys") = vector<string>{}, arg("alias") = "", ref_int)
    .def("Join", [](Ratio& self, const std::array<string, 2>& tree, const vector<string>& keys, const string& alias) -> Ratio& { return self.Join({tree[0], tree[1]}, keys, alias); }, arg("tree"), arg("keys") = vector<string>{}, arg("alias") = "", ref_int)
    .def("Entries", overload_cast<uint32_t>(&Ratio::Entries), arg("nEntries"), ref_int)
    .def("Entries", overload_cast<uint32_t, uint32_t>(&Ratio::Entries), arg("entryMin"), arg("entryMax"), ref_int);
  // only here, after Ratio is registered
  py::class_<Data>(m.attr("Data")).def("AsRatio", &Data::AsRatio, ref_int);
}

void exportAxis(py::module_& m)
{
  py::class_<Axis>(m, "Axis")
    .def("Print", &Axis::Print)
    .def("SetTitle", &Axis::SetTitle, arg("title"), ref_int)
    .def("SetRange", &Axis::SetRange, arg("min"), arg("max"), ref_int)
    .def("SetMaxRange", &Axis::SetMaxRange, arg("max"), ref_int)
    .def("SetMinRange", &Axis::SetMinRange, arg("min"), ref_int)
    .def("SetRoomAboveMax", &Axis::SetRoomAboveMax, arg("frameFraction"), ref_int)
    .def("SetRoomBelowMin", &Axis::SetRoomBelowMin, arg("frameFraction"), ref_int)
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
    .def("SetLabelAngle", &Axis::SetLabelAngle, arg("angle"), ref_int)
    .def("SetTitleCenter", &Axis::SetTitleCenter, arg("center") = true, ref_int)
    .def("SetLabelCenter", &Axis::SetLabelCenter, arg("center") = true, ref_int)
    .def("SetLog", &Axis::SetLog, arg("isLog") = true, ref_int)
    .def("SetGrid", &Axis::SetGrid, arg("isGrid") = true, ref_int)
    .def("SetOppositeTicks", &Axis::SetOppositeTicks, arg("isOppositeTicks") = true, ref_int)
    .def("SetNoExponent", &Axis::SetNoExponent, arg("isNoExponent") = true, ref_int)
    .def("SetHideClippedLabels", &Axis::SetHideClippedLabels, arg("hide") = true, ref_int)
    .def("SetColorScaleOffset", &Axis::SetColorScaleOffset, arg("offset"), ref_int)
    .def("SetColorScaleWidth", &Axis::SetColorScaleWidth, arg("width"), ref_int)
    .def("SetTimeFormat", &Axis::SetTimeFormat, arg("timeFormat"), ref_int)
    .def("SetTickOrientation", &Axis::SetTickOrientation, arg("tickOrientation"), ref_int);
}

void exportTextBox(py::module_& m)
{
  py::class_<TextBox> textBox(m, "TextBox");
  textBox
    .def("Print", &TextBox::Print)
    .def("SetText", py::overload_cast<const std::string&>(&TextBox::SetText), arg("text"), ref_int)
    .def("SetText", py::overload_cast<const vector<string>&>(&TextBox::SetText), arg("lines"), ref_int)
    .def("SetTextAlign", &TextBox::SetTextAlign, arg("align"), ref_int)
    .def("SetPosition", &TextBox::SetPosition, arg("x"), arg("y"), ref_int)
    .def("SetSize", &TextBox::SetSize, arg("width"), arg("height"), ref_int)
    .def("SetUserCoordinates", &TextBox::SetUserCoordinates, arg("isUserCoord") = true, ref_int)
    .def("SetAutoPlacement", &TextBox::SetAutoPlacement, arg("placement") = best_corner, ref_int);
  defBorderSetters(textBox);
  defTextSetters(textBox);
  defFillSetters(textBox);
  textBox
    .def("SetTransparent", &TextBox::SetTransparent, ref_int)
    .def("SetNoBox", &TextBox::SetNoBox, ref_int)
    .def("SetMargin", &TextBox::SetMargin, arg("margin"), ref_int)
    .def("SetLineSpacing", &TextBox::SetLineSpacing, arg("lineSpacing"), ref_int);
}

void exportStatsBox(py::module_& m)
{
  py::class_<StatsBox> statsBox(m, "StatsBox");
  statsBox
    .def(py::init<>())
    .def("Print", &StatsBox::Print)
    .def("SetPosition", &StatsBox::SetPosition, arg("x"), arg("y"), ref_int)
    .def("SetSize", &StatsBox::SetSize, arg("width"), arg("height"), ref_int)
    .def("SetUserCoordinates", &StatsBox::SetUserCoordinates, arg("isUserCoord") = true, ref_int)
    .def("SetAutoPlacement", &StatsBox::SetAutoPlacement, arg("placement") = best_corner, ref_int);
  defBorderSetters(statsBox);
  defTextSetters(statsBox);
  defFillSetters(statsBox);
  statsBox
    .def("SetTransparent", &StatsBox::SetTransparent, ref_int)
    .def("SetNoBox", &StatsBox::SetNoBox, ref_int)
    .def("SetMargin", &StatsBox::SetMargin, arg("margin"), ref_int)
    .def("SetLineSpacing", &StatsBox::SetLineSpacing, arg("lineSpacing"), ref_int);
}

void exportLegendBox(py::module_& m)
{
  py::class_<LegendBox> legendBox(m, "LegendBox");
  legendBox
    .def("Print", &LegendBox::Print)
    .def("AddEntry", &LegendBox::AddEntry, arg("label") = "", ref_int)
    .def("AddDataEntry", &LegendBox::AddDataEntry, arg("padID"), arg("dataID"), ref_int)
    .def("SetTitle", &LegendBox::SetTitle, arg("title"), ref_int)
    .def("SetNumColumns", &LegendBox::SetNumColumns, arg("numColumns"), ref_int)
    .def("SetDefaultSymbolElements", &LegendBox::SetDefaultSymbolElements, arg("symbolElements"), ref_int)
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
    .def("SetAutoPlacement", &LegendBox::SetAutoPlacement, arg("placement") = best_corner, ref_int);
  defBorderSetters(legendBox);
  defTextSetters(legendBox);
  defFillSetters(legendBox);
  legendBox
    .def("SetTransparent", &LegendBox::SetTransparent, ref_int)
    .def("SetNoBox", &LegendBox::SetNoBox, ref_int)
    .def("SetMargin", &LegendBox::SetMargin, arg("margin"), ref_int)
    .def("SetLineSpacing", &LegendBox::SetLineSpacing, arg("lineSpacing"), ref_int);
}

void exportLegendEntry(py::module_& m)
{
  py::class_<LegendEntry> legendEntry(m, "LegendEntry");
  legendEntry
    .def(py::init<>())
    .def("Print", &LegendEntry::Print)
    .def("SetLabel", &LegendEntry::SetLabel, arg("label"), ref_int)
    .def("SetRefData", &LegendEntry::SetRefData, arg("refDataID"), ref_int)
    .def("SetSymbolElements", &LegendEntry::SetSymbolElements, arg("symbolElements"), ref_int)
    .def("AtTop", &LegendEntry::AtTop, ref_int)
    .def("BeforeData", &LegendEntry::BeforeData, arg("dataID"), ref_int)
    .def("AfterData", &LegendEntry::AfterData, arg("dataID"), ref_int)
    .def("SetColor", &LegendEntry::SetColor, arg("color"), ref_int)
    .def("SetAlpha", &LegendEntry::SetAlpha, arg("alpha"), ref_int);
  defMarkerSetters(legendEntry);
  defLineSetters(legendEntry);
  defFillSetters(legendEntry);
  defTextSetters(legendEntry);
}

void exportShape(py::module_& m)
{
  py::class_<Shape> shape(m, "Shape", "A line, arrow, region, ellipse or polygon in a pad that is not data (Pad.AddLine, AddArrow, AddRegion, ...).");
  shape
    .def("Print", &Shape::Print)
    .def("SetUserCoordinates", &Shape::SetUserCoordinates, arg("isUserCoord") = true, ref_int)
    .def("SetBehindData", &Shape::SetBehindData, ref_int)
    .def("SetOnTop", &Shape::SetOnTop, ref_int)
    .def("SetArrowHead", &Shape::SetArrowHead, arg("style"), ref_int)
    .def("SetArrowSize", &Shape::SetArrowSize, arg("size"), ref_int)
    .def("SetArc", &Shape::SetArc, arg("phiMin"), arg("phiMax"), ref_int)
    .def("SetRotation", &Shape::SetRotation, arg("theta"), ref_int);
  defLineSetters(shape);
  defFillSetters(shape);
}

void exportPythonDataInterfaces(py::module_& m)
{
  auto bind_owned = [](std::uintptr_t addr, const char* className) {
    py::module_ ROOT = py::module_::import("ROOT");
    py::object BindObject = ROOT.attr("BindObject");
    py::object SetOwnership = ROOT.attr("SetOwnership");
    py::object result = BindObject(addr, ROOT.attr(className));
    SetOwnership(result, true);
    return result;
  };

  // the numbers of an argument: a numpy array is read in place (any numeric type), any other sequence is converted;
  // numpy is only used when the program has loaded it, so plain lists work without numpy installed
  struct Values {
    py::array_t<double, py::array::c_style | py::array::forcecast> array;
    vector<double> list;
    const double* data() const { return array ? array.data() : list.data(); }
    size_t size() const { return array ? static_cast<size_t>(array.size()) : list.size(); }
  };
  auto values = [](py::handle input, const string& what) {
    Values v;
    if (py::module_::import("sys").attr("modules").contains("numpy")) {
      v.array = py::array_t<double, py::array::c_style | py::array::forcecast>::ensure(input);
      if (!v.array || v.array.ndim() != 1) {
        throw py::type_error("The " + what + " must be a one-dimensional sequence of numbers.");
      }
    } else {
      try {
        v.list = input.cast<vector<double>>();
      } catch (const py::cast_error&) {
        throw py::type_error("The " + what + " must be a sequence of numbers.");
      }
    }
    return v;
  };

  auto weightsFor = [values](py::handle weights, size_t n) -> optional<Values> {
    if (weights.is_none()) return nullopt;
    Values w = values(weights, "weights");
    if (w.size() != n) {
      throw py::value_error("The weights must have as many entries as the values.");
    }
    return w;
  };

  // bins as for numpy.histogram: a number of bins or the bin edges
  auto binning = [values](py::handle bins, const string& what) -> std::variant<int32_t, vector<double>> {
    if (py::isinstance<py::sequence>(bins) && !py::isinstance<py::str>(bins)) {
      const Values v = values(bins, what);
      vector<double> edges(v.data(), v.data() + v.size());
      if (edges.size() < 2 || std::adjacent_find(edges.begin(), edges.end(), std::greater_equal<>()) != edges.end()) {
        throw py::value_error("The " + what + " must be at least two increasing bin edges.");
      }
      return edges;
    }
    int32_t n = 0;
    try {
      n = bins.cast<int32_t>();
    } catch (const py::cast_error&) {
      throw py::type_error("The " + what + " must be a number of bins or a sequence of bin edges.");
    }
    if (n < 1) {
      throw py::value_error("The " + what + " must be at least 1.");
    }
    return n;
  };

  // the range of a number of bins: as given, or from the values
  auto rangeFor = [](const optional<pair<double, double>>& range, const Values& v, const string& what) {
    if (range) {
      if (!(range->first < range->second)) {
        throw py::value_error("The " + what + " range must have a lower end below the upper end.");
      }
      return *range;
    }
    if (v.size() == 0) {
      throw py::value_error("No " + what + " values to take the range from: give a range or bin edges.");
    }
    auto [minIt, maxIt] = std::minmax_element(v.data(), v.data() + v.size());
    return util::auto_axis_range(*minIt, *maxIt);
  };

  auto graph = [bind_owned, values](const string& name, py::object xValues, py::object yValues, py::object xErrors, py::object yErrors) {
    const Values x = values(xValues, "x values");
    const Values y = values(yValues, "y values");
    const auto n = x.size();
    if (y.size() != n) {
      throw py::value_error("The x and y values must have the same length.");
    }
    auto errors = [&](py::handle e, const string& what) -> optional<Values> {
      if (e.is_none()) return nullopt;
      Values v = values(e, what);
      if (v.size() != n) {
        throw py::value_error("The " + what + " must have as many entries as the points.");
      }
      return v;
    };
    const auto xerr = errors(xErrors, "x errors (xerr)");
    const auto yerr = errors(yErrors, "y errors (yerr)");
    auto* g = new TGraphErrors(static_cast<int>(n), x.data(), y.data(), xerr ? xerr->data() : nullptr, yerr ? yerr->data() : nullptr);
    g->SetName(name.data());
    return bind_owned(reinterpret_cast<std::uintptr_t>(g), "TGraphErrors");
  };

  auto histo = [=](const string& name, py::object xValues, py::object bins, py::object range, py::object weights) {
    const Values x = values(xValues, "values");
    const auto w = weightsFor(weights, x.size());
    const auto spec = binning(bins, "bins");
    TH1D* h = nullptr;
    if (const auto* edges = std::get_if<vector<double>>(&spec)) {
      if (!range.is_none()) logger::warning("Histogram {}: the range is not used, since bin edges are given.", name);
      h = new TH1D(name.data(), "", static_cast<int>(edges->size()) - 1, edges->data());
    } else {
      const auto [min, max] = rangeFor(range.is_none() ? nullopt : optional(range.cast<pair<double, double>>()), x, "x");
      h = new TH1D(name.data(), "", std::get<int32_t>(spec), min, max);
    }
    h->SetDirectory(nullptr);
    if (w) h->Sumw2();
    if (x.size()) h->FillN(static_cast<int>(x.size()), x.data(), w ? w->data() : nullptr);
    return bind_owned(reinterpret_cast<std::uintptr_t>(h), "TH1D");
  };

  auto histo2d = [=](const string& name, py::object xValues, py::object yValues, py::object bins, py::object range, py::object weights) {
    const Values x = values(xValues, "x values");
    const Values y = values(yValues, "y values");
    if (x.size() != y.size()) {
      throw py::value_error("The x and y values must have the same length.");
    }
    const auto w = weightsFor(weights, x.size());
    // bins: 50 x 50 by default, one number for both axes, or one entry per axis (a number of bins or the edges)
    py::object binsX = py::int_(50);
    py::object binsY = py::int_(50);
    if (!bins.is_none()) {
      if (py::isinstance<py::sequence>(bins) && !py::isinstance<py::str>(bins)) {
        auto perAxis = py::reinterpret_borrow<py::sequence>(bins);
        if (perAxis.size() != 2) {
          throw py::value_error("The bins must be a number, (nx, ny) or (x edges, y edges).");
        }
        binsX = perAxis[0];
        binsY = perAxis[1];
      } else {
        binsX = binsY = bins;
      }
    }
    optional<pair<double, double>> rangeX, rangeY;
    if (!range.is_none()) {
      std::tie(rangeX, rangeY) = range.cast<pair<pair<double, double>, pair<double, double>>>();
    }
    const auto specX = binning(binsX, "x bins");
    const auto specY = binning(binsY, "y bins");
    TH2D* h = nullptr;
    if (std::holds_alternative<int32_t>(specX) && std::holds_alternative<int32_t>(specY)) {
      const auto [xMin, xMax] = rangeFor(rangeX, x, "x");
      const auto [yMin, yMax] = rangeFor(rangeY, y, "y");
      h = new TH2D(name.data(), "", std::get<int32_t>(specX), xMin, xMax, std::get<int32_t>(specY), yMin, yMax);
    } else {
      // an axis given by a number of bins gets evenly spaced edges
      auto edgesOf = [&](const std::variant<int32_t, vector<double>>& spec, const optional<pair<double, double>>& axisRange, const Values& v, const string& axis) {
        if (const auto* edges = std::get_if<vector<double>>(&spec)) {
          if (axisRange) logger::warning("Histogram {}: the {} range is not used, since {} bin edges are given.", name, axis, axis);
          return *edges;
        }
        const auto [min, max] = rangeFor(axisRange, v, axis);
        const int32_t n = std::get<int32_t>(spec);
        vector<double> edges(n + 1);
        for (int32_t i = 0; i <= n; ++i) {
          edges[i] = min + (max - min) * i / n;
        }
        return edges;
      };
      const vector<double> edgesX = edgesOf(specX, rangeX, x, "x");
      const vector<double> edgesY = edgesOf(specY, rangeY, y, "y");
      h = new TH2D(name.data(), "", static_cast<int>(edgesX.size()) - 1, edgesX.data(), static_cast<int>(edgesY.size()) - 1, edgesY.data());
    }
    h->SetDirectory(nullptr);
    if (w) h->Sumw2();
    if (x.size()) h->FillN(static_cast<int>(x.size()), x.data(), y.data(), w ? w->data() : nullptr);
    return bind_owned(reinterpret_cast<std::uintptr_t>(h), "TH2D");
  };

  // a table as a tree, from a dict of arrays, a numpy structured array or a pandas DataFrame (anything with columns and item access),
  // or from a 2D array (rows x columns, as pandas reads it) with the names given in columns:
  // numbers and bools keep their type, strings become std::string branches, other columns are skipped with a warning
  auto table = [bind_owned](const string& name, py::object data, py::object columnNames) {
    py::module_ np;
    try {
      np = py::module_::import("numpy");
    } catch (const py::error_already_set&) {
      throw std::runtime_error("table() needs numpy.");
    }
    vector<pair<string, py::array>> columns;
    auto add = [&](py::handle key, py::handle column) {
      columns.emplace_back(py::str(key).cast<string>(), np.attr("asarray")(column).cast<py::array>());
    };
    const bool isDict = py::isinstance<py::dict>(data);
    const bool isStructured = py::hasattr(data, "dtype") && py::hasattr(data.attr("dtype"), "names") && !data.attr("dtype").attr("names").is_none();
    const bool isFrame = py::hasattr(data, "columns");
    if (!columnNames.is_none()) {
      if (isDict || isStructured || isFrame) {
        throw py::type_error("columns= names the columns of an array without names; a dict, DataFrame or structured array has its own.");
      }
      vector<string> names;
      try {
        names = columnNames.cast<vector<string>>();
      } catch (const py::cast_error&) {
        throw py::type_error("columns= must be a list of names.");
      }
      auto matrix = np.attr("asarray")(data).cast<py::array>();
      if (matrix.ndim() == 1 && names.size() == 1) {
        columns.emplace_back(names[0], matrix);
      } else if (matrix.ndim() != 2) {
        throw py::value_error("columns= needs a 2D array (rows x columns), or a 1D array and one name.");
      } else if (static_cast<size_t>(matrix.shape(1)) != names.size()) {
        throw py::value_error("The array has " + std::to_string(matrix.shape(1)) + " columns, but " + std::to_string(names.size()) + " names are given.");
      } else {
        py::object byColumn = matrix.attr("T");
        for (size_t i = 0; i < names.size(); ++i) {
          add(py::str(names[i]), byColumn[py::int_(i)]);
        }
      }
    } else if (isDict) {
      for (auto [key, column] : py::reinterpret_borrow<py::dict>(data))
        add(key, column);
    } else if (isStructured) {
      for (auto key : data.attr("dtype").attr("names"))
        add(key, data[key]);
    } else if (isFrame) {
      for (auto key : data.attr("columns"))
        add(key, data[key]);
    } else {
      throw py::type_error("A table must be a dict of arrays, a numpy structured array or a pandas DataFrame, or a 2D array with columns=[names].");
    }

    auto tree = std::make_unique<TTree>(name.data(), "");
    tree->SetDirectory(nullptr);
    std::deque<std::any> buffers;  // what the branches read when filling (a deque keeps the addresses)
    vector<std::function<void(py::ssize_t)>> setters;
    std::set<string> names;
    py::ssize_t nRows = -1;
    auto skip = [&](const string& column, const string& reason) {
      logger::warning("Table {}: column '{}' is skipped ({}).", name, column, reason);
    };
    for (auto& entry : columns) {
      const auto& column = entry.first;  // plain references: lambdas cannot capture structured bindings in C++17
      const auto& array = entry.second;
      if (column.empty() || column.find_first_of("/[]:") != string::npos) {
        skip(column, "a branch name cannot be empty or contain / [ ] :");
        continue;
      }
      if (!names.insert(column).second) {
        skip(column, "a column of the same name comes first");
        continue;
      }
      if (array.ndim() != 1) {
        skip(column, "only one value per row is supported");
        continue;
      }
      if (nRows < 0) {
        nRows = array.shape(0);
      } else if (array.shape(0) != nRows) {
        throw py::value_error("All columns of a table must have the same length.");
      }
      auto numeric = [&](auto zero, const char* leafType) {
        using T = decltype(zero);
        auto typed = py::array_t<T, py::array::forcecast>::ensure(array);
        auto& buffer = std::any_cast<T&>(buffers.emplace_back(zero));
        tree->Branch(column.data(), &buffer, (column + "/" + leafType).data());
        setters.emplace_back([view = typed.template unchecked<1>(), typed, &buffer](py::ssize_t i) { buffer = view(i); });
      };
      // kind and size read from Python: older pybind11 reads them from the descriptor struct, whose layout changed with numpy 2
      const char kind = array.dtype().attr("kind").cast<string>()[0];
      const auto bytes = array.dtype().attr("itemsize").cast<int32_t>();
      if (kind == 'f' && bytes == 8) {
        numeric(double_t{}, "D");
      } else if (kind == 'f' && bytes <= 4) {
        numeric(float_t{}, "F");
      } else if (kind == 'i' && bytes == 1) {
        numeric(int8_t{}, "B");
      } else if (kind == 'i' && bytes == 2) {
        numeric(int16_t{}, "S");
      } else if (kind == 'i' && bytes == 4) {
        numeric(int32_t{}, "I");
      } else if (kind == 'i' && bytes == 8) {
        numeric(int64_t{}, "L");
      } else if (kind == 'u' && bytes == 1) {
        numeric(uint8_t{}, "b");
      } else if (kind == 'u' && bytes == 2) {
        numeric(uint16_t{}, "s");
      } else if (kind == 'u' && bytes == 4) {
        numeric(uint32_t{}, "i");
      } else if (kind == 'u' && bytes == 8) {
        numeric(uint64_t{}, "l");
      } else if (kind == 'b') {
        numeric(bool{}, "O");
      } else if (kind == 'U' || kind == 'S' || kind == 'O') {
        py::list items = array.attr("tolist")();
        const bool strings = std::all_of(items.begin(), items.end(), [](py::handle item) { return py::isinstance<py::str>(item) || py::isinstance<py::bytes>(item); });
        if (!strings) {
          skip(column, "its values are neither numbers of one type nor all strings; convert it, e.g. with astype()");
          continue;
        }
        auto& buffer = std::any_cast<string&>(buffers.emplace_back(string{}));
        tree->Branch(column.data(), &buffer);
        setters.emplace_back([items, &buffer](py::ssize_t i) { buffer = items[static_cast<size_t>(i)].cast<string>(); });
      } else {
        skip(column, "its type " + py::str(array.dtype()).cast<string>() + " is not supported");
      }
    }
    if (setters.empty()) {
      throw py::value_error("Table " + name + " has no column that can be stored.");
    }
    for (py::ssize_t i = 0; i < nRows; ++i) {
      for (auto& set : setters)
        set(i);
      tree->Fill();
    }
    tree->ResetBranchAddresses();  // the buffers end here
    return bind_owned(reinterpret_cast<std::uintptr_t>(tree.release()), "TTree");
  };

  m.def("graph", graph, "A TGraphErrors through the points (x, y), with optional errors, like matplotlib's errorbar.",
        py::arg("name"), py::arg("x"), py::arg("y"), py::arg("xerr") = py::none(), py::arg("yerr") = py::none());
  m.def("histo", histo,
        "A TH1D filled with the values, like numpy.histogram: bins is a number (with the range taken from the values if not given) or the bin edges; "
        "with weights the errors come from the sum of the squared weights.",
        py::arg("name"), py::arg("values"), py::arg("bins") = 100, py::arg("range") = py::none(), py::arg("weights") = py::none());
  m.def("histo2d", histo2d,
        "A TH2D filled with the pairs (x, y), like numpy.histogram2d: bins is one number for both axes, (nx, ny) or (x edges, y edges), "
        "where a number and edges can be mixed; range is ((x0, x1), (y0, y1)); optional weights.",
        py::arg("name"), py::arg("x"), py::arg("y"), py::arg("bins") = py::none(), py::arg("range") = py::none(), py::arg("weights") = py::none());
  m.def("table", table,
        "A tree with one branch per column of a dict of arrays, a numpy structured array or a pandas DataFrame, or of a 2D array (rows x columns) "
        "with the names in columns, to be used with Project1D, Filter, Define: "
        "numbers and bools keep their type, strings become std::string, other columns are skipped with a warning.",
        py::arg("name"), py::arg("data"), py::arg("columns") = py::none());
}
