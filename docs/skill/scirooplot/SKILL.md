---
name: scirooplot
description: Define, organize and generate ROOT plots with SciRooPlot (C++ or Python). Use when writing or changing a DefinePlots.cpp / DefinePlots.py program, calling the `plot` or `srp` command line tools, defining data sources, base plots, ratios, tree/CSV projections or drawing option aliases, or when a user mentions SciRooPlot.
---

# SciRooPlot

SciRooPlot is a C++/Python library on top of ROOT that separates *defining* plots from *drawing* them.
Describes SciRooPlot 3.3 (ROOT >= 6.36.04). Repository: https://github.com/SciRooPlot/SciRooPlot — docs: https://scirooplot.github.io/SciRooPlot/

## Mental model (read this first)

1. The user's program (`DefinePlots.cpp` or `DefinePlots.py`) declares data sources and plots and ends with `pm.SaveProject()`. It draws nothing; it writes `plots.info` and `dataSources.info` into `~/.SciRooPlot/<project>/` (or `$SCIROOPLOT_CONFIG_PATH/<project>/`).
2. The command line app `plot <group> <name> [<mode>]` reads these files, loads the data, draws the plots and shows or saves them. It rebuilds (CMake) and re-runs the program automatically when the sources changed.
3. Every plot belongs to a group (subgroups with `/`), builds on a *base plot* (layout, fonts, colours) and consists of pads `plot[1]`, `plot[2]`, …; `plot[0]` holds defaults for all pads. Everything is a fluent setter chain returning the modified object.
4. `plot` and `srp` act on the **currently selected project** (`srp select <project>`), not on the directory you are in.

## Install and project setup

```bash
brew tap SciRooPlot/scirooplot && brew install SciRooPlot && source "$(brew --prefix scirooplot)/share/scirooplot/env.sh"   # macOS/Linux
conda install conda-forge::scirooplot && source "${CONDA_PREFIX}/share/scirooplot/env.sh"
git clone https://github.com/SciRooPlot/SciRooPlot.git && cd SciRooPlot && ./scripts/install.sh   # from source (needs ROOT + Boost)
```
Put the `source .../env.sh` line into `.zshrc`/`.bashrc` (absolute path); it provides `srp`, `plot`, the Python module `SciRooPlot` and tab completion.

```bash
srp init-cpp <project> [<dir>]      # minimal C++ project (CMake), or init-py for Python; contains one plot: `plot myGroup myFirstPlot`
srp example-cpp <project> [<dir>]   # commented example plots with example data (also example-py), e.g. `plot spectra ptSpectrum`, `plot higgs .+`
srp add <project> <program> [<outdir>]   # register an existing program
srp select <project>                # make it the current project
srp set <project> outdir <dir>      # where files are saved (subfolders per group)
srp set <project> <key> <value>     # own variables, read in code via pm.GetProjectProperty("<key>")
srp projects | srp show | srp settings | srp help
srp verbosity (debug|log|info|warning|error|silent) ; srp plotmode (show|pdf|png|...) ; srp matchmode (exact|contains)
```

## The `plot` app

```bash
plot <group> <name> [<mode>]        # group and name are regular expressions (whole match by default)
plot myGroup .+                     # all plots of myGroup and its subgroups
plot 'myGroup$' .+                  # without subgroups;  plot myGroup/QA ctrl.*  selects a subgroup
plot thesis 'fig_(a|b)' pdf         # quote patterns with ( ) | in bash
```
Modes: `show` (interactive, default: keys `s`/`a` next/previous (circular), `d` moves the current plot into a window of its own, `q` closes a window, `x` all; the keys also work in the terminal, where `q` closes all; double-click a legend, text or shape to print its name and coordinates), `spread` (all matching plots at once, one window each), `list`, `print` (dump definition), `pdf`, `eps`, `ps`, `svg`, `png`, `jpg`, `gif` (`gif+<centiseconds>` animates all matching plots, played once; `gif+<centiseconds>++` loops endlessly, `gif+<centiseconds>++<repeats>` that many times; in the order they were defined: one plot per frame in a group of its own, e.g. a loop over `SetView` angles or multiplicity classes, see the example group `animations`), `html`, `json`, `xml`, `root`, `macro` (ROOT macro), `file` (canvases into a ROOT file), `data` (the drawn data into a ROOT file).

## Program skeleton

C++ (`using namespace SciRooPlot; using Data = Plot::Pad::Data; using LegendEntry = Plot::Pad::LegendEntry; using StatsBox = Plot::Pad::StatsBox;`):
```cpp
#include "SciRooPlot/PlotManager.h"
using namespace SciRooPlot;
int main() {
  PlotManager pm("myProject");                                  // project name = the one given to srp
  pm.AddDataSource("data", "/path/run1.root");                  // see "Data sources"
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d"));              // layouts used as third Plot argument
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d_ratio"));
  {
    Plot plot("spectrum", "paper/pt", "1d_ratio");              // name, group[/subgroup], base plot
    plot[1].AddData("hPt", "data", "data <mean[.2f]>").SetDrawOption(points).SetColor(kBlue + 1);
    plot[1].AddData("hPtMC", "mc", "simulation").SetDrawOption(curve);
    plot[2].AddRatio({"hPt", "data"}, {"hPtMC", "mc"}).SetIsCorrelated();
    plot[1].AddLegend();                                        // auto-placed, or AddLegend(0.6, 0.9) / AddLegend(top_right)
    plot[0]['X'].SetRange(0.5, 20.0).SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetLog();
    plot[2]['Y'].SetRange(0.5, 1.5).SetTitle("data / MC");
    pm.AddPlot(plot);
  }
  pm.SaveProject();
}
```
Python is the same API: `from SciRooPlot import *`, `PlotManager("myProject")`, `plot[1]['X']`, `pm.AddPlot(plot)`, `pm.SaveProject()` (see "C++ vs Python").

## Data sources

`pm.AddDataSource(name, input | [inputs])` — a data source is one data set made of its inputs:
- `"/path/a.root"` whole file; `"/path/a.root:dir/list"` a directory or TList inside it; `"/path/folder/"` all ROOT files below a directory; wildcards `AO2D_*.root:DF_*` (files and folders inside files); `${HOME}/...` environment variables; `"/path/table.csv"` (data name = file name without `.csv`).
- Within a data source the first input containing a requested name wins; trees and RNTuples found in several inputs are chained. Train/test samples etc. therefore belong in separate sources.
- The data name may be a path inside the input: `AddData("folder1/hist", "src")`. Sub-sources are allowed in `AddData` as `"src:folder"` (restricts the search to that folder) — the folder is part of the source, never of the name.
- Without a path, a name is searched in the folder of the data source first, then in its sub-folders/lists one after the other in stored order, each completely (depth-first); the first match counts. If a name exists more than once, give the path.
- `AddDataSource(name, TObject*)` or a vector of objects adds in-memory objects; they are copied into the project, so they may be deleted afterwards. A named list (TList, TObjArray, ...) or a TDirectory becomes a folder of the data source (`AddData("myList/myHist", "src")`, trees inside are copied with all entries); a nameless list (ROOT reports its class name) or an open TFile adds its contents directly. Folders of the same name are merged, an object of an existing name replaces it (with a warning).

## Plot, pads, axes

```cpp
Plot plot("name", "group/sub", "basePlot");  Plot copy(plot, "name2", "group2");   // copy keeps everything
plot.SetDimensions(710, 710[, fixAspect]).SetTransparent().AppendGroup("QA");
plot[0]...        // defaults for all pads (text font/size, margins, colour lists, ref function)
plot[1].SetPosition(x1,y1,x2,y2).SetMargins(t,b,l,r).SetFrameFill(1001, 10).SetPalette(kBird).SetView(theta, phi)   // 3D view in degrees (ROOT: 30, 30)
plot[1].SetDefaultMarkerColors({kBlack, kRed+1}).SetDefaultLineStyles({kSolid, kDashed})   // cycled over the data
// every colour argument takes a ROOT colour (kRed + 1), "#rrggbb" or {r, g, b} from 0 to 1 (Python: a tuple); transparency only with the alpha setters
plot[1].SetDefaultColors(gradient)   // {{"#000080", 0.0}, {kRed, 0.5}, {{1.0, 1.0, 0.0}, 1.0}}: (colour, position) or {r,g,b,position}, all in [0,1]; also SetPalette(gradient)
plot[1].SetRefFunc("1");              // reference line/function in a ratio pad (drawn first, black); SetRefFunc("") removes the one of the base plot
plot[1]['X'].SetTitle("#it{x}").SetRange(a, b).SetLog().SetGrid().SetNumDivisions(305).SetMaxDigits(3).SetNoExponent()
            .SetTitleFont(43).SetTitleSize(28).SetTitleOffset(1.1).SetLabelFont(43).SetLabelSize(24)   // sizes in pixels only with font 43; with the default font 42 they are fractions of the pad
            .SetOppositeTicks().SetTickOrientation("+-").SetTimeFormat("%H:%M")
            .SetHideClippedLabels(false) // the base plots drop a label whose text would reach beyond the pad (top label of a ratio pad at the shared edge); false keeps it
plot[1]['Y'].SetRoomAboveMax(0.3).SetRoomBelowMin(0.1)    // 30 % / 10 % of the frame free beyond everything drawn in the pad (any axis; error bars
                                                         // where ROOT counts them; log axes in decades); an explicit SetMinRange/SetMaxRange wins on its side
```
Pads count from 1; data in a pad from 1 (`plot[1](2)` or `plot[1].GetData(2)` modifies the second data later). The first data drawn defines the axis frame unless another one calls `.SetDefinesFrame()`; an input that should only give the frame (bin labels, a binning) is set with `plot[1].SetFrame("name", "source")`: it is not drawn and not numbered. A pad without data needs no dummy input: its X and Y ranges (`plot[1]['X'].SetRange(0.0, 10.0)`) give an empty frame.

Text sizes, three levels: for a whole class of plots change the base plot (`MakeBasePlot("1d", PanelLayout().SetTextSize(24).SetTitleSize(30))`, see below: the only way that keeps all panels consistent); for one plot set the pad (`plot[1].SetDefaultTextSize(s)`: labels, legends and texts of that pad; `plot[0]` for all pads) or the axis (`plot[1]['Y'].SetTitleSize(s).SetLabelSize(s)`); for one box `AddText(...).SetTextSize(s)`, `GetLegend(1).SetTextSize(s)`. With the default font 42 a size is a fraction of the pad, so the same number is smaller in a smaller pad; font 43 (`SetDefaultTextFont(43)`, `SetTitleFont(43)`, `SetTextFont(43)`) takes pixels of the plot.

Inset (a zoom inside the frame): an extra pad placed with `SetPosition` in fractions of the plot; pads are drawn in the order of their numbers, so the inset covers what is below it, and its text sizes are fractions of the small pad:
```cpp
plot[2].SetPosition(0.5, 0.5, 0.88, 0.88).SetMargins(0.05, 0.22, 0.3, 0.05).SetDefaultTextSize(0.09);
plot[2].AddData("diphoton/mgg_data", "higgs");                      // no label: no legend row
plot[2]['X'].SetRange(120, 130).SetNumDivisions(505).SetTitleSize(0.1).SetLabelSize(0.08).SetHideClippedLabels(false);
plot[2]['Y'].SetRange(1500, 2600).SetNoExponent().SetNumDivisions(505).SetTitleSize(0.1).SetLabelSize(0.08).SetTitleOffset(1.4).SetHideClippedLabels(false);
```

Base plots: `PlotManager::MakeBasePlot("<type>[_wide|_tall][_<cols>x<rows>][_ratio][_gap]")` with type `1d` or `2d` (colour scale), e.g. `1d`, `1d_ratio`, `2d`, `1d_wide`, `1d_2x1`, `1d_2x1_ratio`, `1d_2x2`, `2d_2x1`. Pads are numbered row by row; ratio pads follow their panels. Modify and re-add them (`auto bp = PlotManager::MakeBasePlot("1d"); bp[1]['X'].SetColor(kRed); pm.AddBasePlot(bp);`). Other text sizes, line widths or margins go in as the lengths of a `PanelLayout` in pixels: `PlotManager::MakeBasePlot("1d_ratio", PanelLayout().SetTextSize(24).SetTitleSize(30))` keeps them equal in all panels; setting `SetDefaultTextSize` on the pads of a base plot afterwards does not (each pad holds a size relative to its own dimensions, and pad 0 is overridden by them in multi-panel layouts). Other proportions go in as a `BasePlotShape`: `MakeBasePlot("1d_wide_ratio", PanelLayout(), BasePlotShape().SetAspectRatio(4.0 / 3.0).SetRatioHeight(0.25))`. Or build your own with `PanelLayout` (`Plot plot("name", PanelLayout(sizes, rows))`, panels `Panel(w, h)`, `Gap()`, `Empty()`, `Empty(w, h)`, `Panel().ZAxis()`; `.Span(columns, rows)` lets a cell cover several columns/rows, listed once in the row where it starts, e.g. `{{Empty(1, 0.2).Span(3)}, {Panel(), Panel(), Panel()}}` for a common legend strip).

## Data: adding, appearance, labels

```cpp
plot[1].AddData("hist", "src", "label");                    // histogram, graph, function, tree, csv — type is detected
plot[1].AddFunction("sin(x)/x", "label");  plot[1].AddPoints({x...}, {y...});  plot[1].AddPoints({x1, x2}, {y1, y2}, "label").SetDrawOption(line);  // a line as data
// shapes are not data (never in the axis ranges, not numbered, no default colours, no legend row), drawn after the data unless SetBehindData()/SetOnTop():
plot[1].AddVerticalLine(125.);  plot[1].AddHorizontalLine(1.);  plot[1].AddRegionX(120., 130.).SetBehindData();  plot[1].AddRegionY(y1, y2);
plot[1].AddLine({x1, y1}, {x2, y2});  plot[1].AddArrow({x1, y1}, {x2, y2}).SetArrowHead("|>").SetArrowSize(0.03);  plot[1].AddRegion({x1, y1}, {x2, y2});
plot[1].AddEllipse({x, y}, rx, ry).SetArc(0., 180.).SetRotation(30.);  plot[1].AddPolygon({{x1, y1}, {x2, y2}, {x3, y3}});  // line and fill setters as for data; SetUserCoordinates(false): fractions of the pad
plot[1].AddFunction("[](double* x, double* p) { return std::exp(-x[0] * x[1]); }");  // C++ lambda as text: x[0..2] set the dimension, no parameters
// any callable (with captures, also from python): TF1 f("f", callable, xmin, xmax, 0); f.SetNpx(n); pm.AddDataSource("src", &f); then AddData("f", "src")
plot[1].AddData(myTemplate("hist"), "label");               // Data template: Data().SetDrawOption(curve).SetColor(kRed) then template("name"[, "src"])
.SetDrawOption(points | "HIST C")     // alias (preferred) or raw ROOT option string
.SetColor(c).SetAlpha(a)           // all of marker, line, fill at once
.SetMarker(style, size, color).SetLine(style, width, color).SetFill(style, color)  // optional alpha last, default 1 (+ SetMarkerColor, SetLineStyle, ... SetFillAlpha)
.SetRangeX(a, b).SetMaxRangeX(b).SetRangeY(a, b)             // visible range of this data, independent of the axis
.SetLegend(2).SetLegendLabel("l").SetDefinesFrame().SetShowOverflowBins().SetContours(n).SetTextFormat("4.2f")
.SetKeepInputStyle()               // line, marker, fill stored with the input win over the pad defaults (what the data sets still wins)
.SetNpx(n[, ny])                   // points a function is drawn with (AddFunction: 1000 in 1d; inputs keep their own)
.SetShowStats("eMR", "title")   // statistics box of a histogram (letters of TStyle::SetOptStat; the optional title is its first line)
.SetStatsBox(StatsBox().SetAutoPlacement(top_left).SetFill(1001, kWhite))   // its placement and appearance: all setters of legend and text boxes
```
Labels may contain `<name> <title> <entries> <integral> <maximum> <minimum> <mean>` with printf formats: `"avg = <mean[.2f]>"` (graphs: `<entries>` points, `<maximum>`/`<minimum>` of y; `<integral>`, `<mean>`, `<maximum>`, `<minimum>` cover the drawn range, `<entries>` is the stored total), and from the definition `<source>`, `<projection>` (`x`, `y:x`, `prof. y:x`, weight in parentheses) and `<selection>` (filters joined with `&&`, entries range, projection ranges); histogram projections name the axes by the axis titles of the input (`x`, `y`, `z` where a title is empty), tree requests by the variables; an empty placeholder takes one separating space with it, so `"<name> <projection> <selection>"` never ends in a space. `"<name> <selection>"` tells apart requests differing only in the filter. ROOT TLatex syntax works in all texts (`#it{p}_{T}`, `#sqrt{s}`).

Without `SetDrawOption` the same rule holds for histograms and graphs: a fill set → `hist` (graph: `area`); a marker style or size set on the data, or non-zero errors → `points`; otherwise `hist` (graph: `line`); 2D histograms, graphs and functions → `colz` (one colour axis covering all 2D data of the pad). The legend row follows what is drawn.
Drawing option aliases (one appearance for every type; histograms and graphs are interchangeable, the data is converted when ROOT lacks the option):

| 1D alias | look | 2D alias | look |
|---|---|---|---|
| `points` | markers + y errors (default for graphs) | `col` / `colz` | colour map (z = colour axis) |
| `points_xerr`, `points_endcaps`, `points_xerr_endcaps` | + x errors / end caps | `box`, `text` | boxes, numbers |
| `points_line`, `points_text`, `points_arrows` | + line / values written / arrow ends (limits: one-sided errors) | `lego`, `lego_col(z)`, `lego_noborders` | lego |
| `line`, `curve` | polyline, smooth curve (no markers) | `surf`, `surf_col(z)`, `surf_fill(z)`, `surf_contours`, `surf_shaded` | surfaces |
| `hist`, `hist_open`, `bars`, `hbars` | steps, open steps, bars with gaps, horizontal bars | `cont`, `cont_col(z)`, `cont_fill(z)`, `cont_smooth(z)` | contours |
| `area`, `area_smooth` | filled to zero | `candle`, `candle_minmax`, `candle_mean`, `candle_notched`, `candle_points`, `candle_scatter`, `candle_meanline` | candle plots |
| `band`, `band_smooth` | filled error band | `violin`, `violin_minmax` | violin plots |
| `boxes`, `boxes_nomarkers`, `brackets` | error boxes, brackets | `col_polar`, `colz_polar`, `lego_polar`, `lego_col_polar`, `surf_polar`, `surf_col_polar` | polar coordinates (x angle, y radius) |
| `hist_clipped`, `hist_open_clipped`, `curve_clipped` | as `hist`, `hist_open`, `curve`, but cut at the frame instead of running along its edge (drawn as graph) | | |

The suffix always adds one feature (`_xerr`, `_endcaps`, `_smooth`, `_nomarkers`, `_clipped`; 2D: `_col` lines coloured by content, `_fill` filled, trailing `z` adds the colour axis). An alias a type cannot draw gives a WARNING and ROOT's default.
Functions (`TF1`): `points`, `line`, `curve`, `area`, `area_smooth` (a fill colours the area between curve and zero; `line`/`curve` drop it). 2D functions and 2D graphs: the `col`, `lego`, `surf`, `cont` aliases (no `box`, `text`, candles). 2D graphs only, drawn in 3D: `triangles`, `triangles_wire`, `triangles_col(z)`, `triangles_fill(z)` (Delaunay triangles), `points`, `points_col(z)` (markers coloured by z; error bars for TGraph2DErrors), `line`; such data must be the first in its pad or follow 3D data (lego, surf).
Polar: the `_polar` aliases take x as the angle (one full turn) and y as the radius; `col_polar`/`colz_polar` draw a round disc in a square frame with a radial axis below it (no cartesian axes, no cell outlines unless a line width is set). Polar graphs (`TGraphPolar`, angle in radians) are drawn on a polar grid filling the pad (use a square base plot such as `1d`); x/y ranges, divisions and label styles apply to angle/radius; shapes are not drawn in such a pad.

## Modifiers (applied in this order, before drawing)

`ScaleX/Y/Z(f)` → `RebinX/Y/Z(n)`, `RebinXY`, `RebinXYZ` → `DivideBinWidth()` → `Smooth(n)` → `Normalize([isDensity], [min, max])` (to integral) → `Cumulative([forward])` → `NormalizeToMaximum([min, max])`, `Scale(f)` → then the range cuts `SetRangeX/Y`.
With `min, max` the integral or maximum is taken only in that x range (the bins reaching into it; graphs: the points within it), e.g. `Normalize(false, 0.6, 0.9)` to compare shapes in a peak region while showing everything.
`Density()` = `DivideBinWidth().Normalize()`: a probability density from counts. `Normalize(true)` is only for data that already is a density (e.g. dN/dpT); `Density()` on such data would divide by the bin widths twice.
So `Normalize().Cumulative()` is a CDF and `Cumulative().NormalizeToMaximum()` ends at 1.0

Projections of histograms: `.ProjectX(yMin, yMax[, userCoords])`, `.ProjectY(...)`, `.ProfileX/Y(...)`, generic `.Project({dims}, {{axis, lo, hi}, ...}[, userCoords])` and `.Profile(...)` for n-dimensional inputs (dims in the order they become x, y, z of the result).

## Ratios

```cpp
plot[2].AddRatio({"num", "src"}, {"den", "src"}, "label").SetIsCorrelated();   // binomial errors for sub-samples
plot[2].AddRatio(templ("num"), Data("den", "src"));                           // from Data objects (ratio keeps the numerator's look)
plot[2].AddRatio(...).Both().Normalize().Result().SetColor(kRed);            // modifier scope: Numer() / Denom() / Both() / Result() (default)
```
Incompatible binnings are divided via spline interpolation. Modifiers and data selections on the ratio itself (`Result()` mode) are usually meaningless for Rebin/Project etc. and warn — write them explicitly with `Numer()`, `Denom()` or `Both()`.

## Trees and tables (TTree, RNTuple, CSV)

```cpp
.Project1D("pt")                                  // 100 automatic bins; {"pt", 200}, {"pt", 200, {0.1, 20.0}}, {"pt", {edges...}}
.Project2D({"pt", 50}, {"eta", 50})  .Project({dims...}[, "weight"])   // up to 3D
.Profile1D({"eta", 70}, "pt")  .Profile2D(x, y, "pt")                   // <pt> vs eta
.Scatter("x", "y"[, "ex", "ey"])  .Scatter(x, y, exl, exh, eyl, eyh)    // graph from columns
.Filter("abs(eta) < 0.8").Filter("z == 8")  .Define("r", "sqrt(x*x+y*y)")  .Entries(100) / .Entries(800, 1200)
.Join("scores")                                  // row by row, columns as 'scores.col' or 'col' if unique
.Join("truth/events", {}, "truth")               // tree in another folder, with alias
.Join({"events", "otherSource"}, {"run", "event"})   // from another data source, matched by up to two integer keys
```
Column expressions and filters are C++ expressions over the column/leaf names (evaluated with RDataFrame when the plot is generated). Without explicit bins, 100 bins over the observed range are used. RNTuples (ROOT's newer table format, e.g. written by uproot's `f["t"] = df`) work like trees, but cannot be joined and are chained only when they have the same path in every file.

## Legends and texts

```cpp
plot[1].AddLegend();  AddLegend(top_right);  AddLegend(0.5, 0.8[, "title"]);   // corners: best_corner, top_left, top_right, bottom_right, bottom_left
plot[1].GetLegend(2).SetNumColumns(2).SetDefaultLineWidth(5.0).SetNoBox().SetTransparent();
plot[1].AddData("band", "src", "unc.").SetLegendEntry(LegendEntry().SetSymbolElements("F"));   // style of a data's row (label stays on the data); symbol element letters: L line, P marker, F fill, E error bar
plot[1].GetLegend(1).AddEntry("custom").SetSymbolElements("L").SetLine(kSolid, 2.0, kRed);   // row of its own (AddEntry(): empty line); SetRefData(n): symbol of data n
plot[1].GetLegend(1).AddEntry("#bf{group}").BeforeData(4);   // placed relative to data n of the pad (as in plot[1](n)): AtTop(), BeforeData(n), AfterData(n); default: at the end
plot[1].AddText({"line one", "line two"});  AddText(top_left, "text");  AddText(0.2, 0.9, "text").SetTextFont(43).SetTextSize(20).SetTextAlign(13);   // font 43: size in pixels
```
Positions are pad coordinates (0–1) of the upper left corner, `.SetUserCoordinates()` switches to axis units. Legend symbols are derived from what is actually drawn; override with `SetSymbolElements`/`SetDefaultSymbolElements`.

## Practices to suggest (this is where SciRooPlot saves work)

**Define appearance once, reuse it everywhere.** Each kind of data (measured points, simulation, model, systematics) should look the same in every figure. A `Data` without a name is a template for one kind; calling it with a name gives a data object with all its settings, so plots only name the data and a change of the template changes every figure:
```cpp
const Data measured = Data().SetDataSource("data").SetMarker(kFullCircle, 1.0, kBlack);
const Data simulated = Data().SetDataSource("mc").SetDrawOption(hist_open).SetLine(kSolid, 3.0, kRed + 1);
plot[1].AddData(measured("ptSpec"), "data");               // name from the template's data source
plot[1].AddData(simulated("ptSpec", "otherMC"), "MC");     // or another data source
plot[2].AddRatio(measured("ptSpec"), simulated("ptSpec"));
```
Templates carry everything a `Data` has (options, colours, ranges, modifiers, projections, tree queries) and can be kept in a `map` keyed by an enum. `.SetLayout(tmpl)` / `.ApplyLayout(tmpl)` take over only the appearance of a template (drawing options, text format, ranges, marker, line, fill, contours) on an existing data; `ApplyLayout` only what is set in it. Put them (and label constants like the experiment name) at file scope of `DefinePlots`.

**Derive plots instead of repeating them.** `Plot variant(plot, "newName", "group")` copies a finished plot; add or change data and axes afterwards (`variant[1](2).SetColor(kRed)`). Generate plot families in loops — over centralities, cuts, runs, detector parts — with the loop variable in the name and group (`"raa_" + cent`, `AppendGroup("nuclearModification")`), so `plot heavyion/nuclearModification .+` makes the whole set.

**Put layout into base plots, not into plots.** Everything that is the same for a class of plots — dimensions, margins, fonts, colour sequence, axis conventions, ratio reference line — belongs into a base plot; the plots then only contain data, titles and ranges (data in a base plot is ignored with a warning). Start from the shipped ones (`MakeBasePlot("1d")`, `"1d_ratio"`, `"2d"`, `"1d_2x1"`, …; modify and `AddBasePlot` under the same or a new name) and keep `plot[0]` for per-plot defaults (`plot[0]['X'].SetRange(...)` acts on all pads).

**Own layouts with PanelLayout** when the grid of panels is not one of the shipped names: panels are placed in rows in units of the standard panel, the sizes (text, ticks, line widths, margins in pixels) are shared, so plots of different layouts look alike in a document:
```cpp
PanelLayout sizes;                                            // the defaults; or sizes.SetTextSize(31.52).SetTitleSize(39.4).SetLineWidth(5)
const vector<vector<Panel>> rows = {{Panel(), Panel()},       // two panels sharing their y axis
                                    {Panel(1, 0.37), Panel(1, 0.37)}};  // a ratio row below each
Plot layout("1d_2x1_ratio_mine", PanelLayout(sizes, rows));   // pads numbered row by row: 1 2 / 3 4
layout.SetTransparent();
layout[0].SetDefaultTextFont(42).SetDefaultColors({kBlack, kBlue + 1, kRed + 1});
layout[0]['X'].SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
for (uint8_t ratioPad : {3, 4}) layout[ratioPad].SetRefFunc("1");
pm.AddBasePlot(layout);
```
`Gap()` between panels makes them independent (own axes), `Empty()` leaves a cell for text, `Panel(w, h)` scales a panel, `Panel().ZAxis()` reserves room for a colour scale. The source of `PlotManager::MakeBasePlot` is the full worked example.

**Keep inputs out of the code.** Machine-specific paths go into project variables (`srp set <project> inputDir /path`, `pm.GetProjectProperty("inputDir")`) or environment variables in the data source strings (`${HOME}/...`); the data source names stay stable across machines. In-memory objects (a graph computed in the program) are registered with `pm.AddDataSource("glauber", &graph)` and used like file data.

**Let the plot show numbers.** Label placeholders (`"<mean[.2f]>"`, `"<entries>"`, `"<integral[.2e]>"`) and `points_text` save hand-copied values; `AddColorOverview("colors", "overview")` renders the colour sequence of the base plots with names and indices (`plot overview colors`), `srp colorwheel` shows all of ROOT's named colours.

## C++ vs Python

Identical names and chains. Differences:
- `from SciRooPlot import *` exports everything: classes, the drawing option aliases (`points`, `colz`, …), `box_placement_t` corners and ROOT's colour/style constants (`kRed`, `kFullCircle`, `kDashed`, `kBird`, …), so `kRed + 1` works.
- Braces become tuples/lists: `Project1D(("pt", 200, [0.1, 20.0]))`, `Project1D(("pt", [0.1, 0.5, 1.0]))`, `AddRatio(["num", "src"], ["den", "src"])`, `Join(["events", "otherSource"], ["run", "event"])`, colour lists `[kBlack, "#1f77b4", (0.2, 0.6, 0.3)]`, gradients `[("#0000ff", 0.0), (kRed, 1.0)]` or `[(0.0, 0.0, 1.0, 0.0), (1.0, 0.0, 0.0, 1.0)]`.
- A list `["name", "src"]` is accepted wherever a `Data` is expected (the Python form of the C++ braces); templates are called the same way: `tmpl("name", "src")`.
- Optional arguments may be left out as in C++; `std::optional` values are `None` when unset.
- No `SRC_DIR` macro: use `Path(__file__).resolve().parent`. The program is plain Python; `plot` runs it with the interpreter from the environment script.
- Python projects are created with `srp init-py` / `srp example-py` and otherwise behave identically.

## Pitfalls that cost time

- Nothing shows up / "Found no plots matching": the wrong project is selected (`srp projects`, `srp select`), or the program did not run `pm.SaveProject()`, or the group/name regex does not match whole names (`srp matchmode contains` relaxes this).
- `plot` uses the program registered for the project (`srp show`), not the file in the current directory.
- Data not found: check the data source name and that the object name (or path) exists in one of the inputs; `plot ... print` shows the definition, `srp verbosity debug` shows which inputs are opened. For sub-folders use `"src:folder"` or a path in the name, never both for the same folder.
- A data name appearing twice in a pad gets a unique suffix automatically; refer to data by index (`plot[1](2)`), not by name.
- Axis ranges live on the pad (`plot[1]['X'].SetRange`), data ranges on the data (`SetRangeX`); the first data (or `SetDefinesFrame`) decides the frame.
- Ranges, minimum and maximum stored with an input histogram or graph are ignored: set them in the plot definition. Exception: data marked `.SetDefinesFrame()` and the frame set with `SetFrame` keep them for the frame they draw (e.g. an empty dummy histogram with the y range stored); an explicit `plot[i]['Y'].SetRange` still wins. A projection or profile never applies stored ranges and warns when one sits on an axis it integrates out. Functions from inputs are drawn where they are defined, functions from `AddFunction` span the frame; `SetRangeX` overrides both (a function written from C++ code only has values within its own range).
- Fill colours are drawn for `hist`/`area`/`band`/`bars`; histograms drawn with `line`/`curve` are not filled even if a fill colour is set by the pad defaults.
- Line widths are whole pixels: ROOT stores them as integers, so a fractional width (`SetLine(s, 3.5, c)`, `SetLineWidth(2.5)`, `SetBorder(s, 1.5, c)`, `SetDefaultLineWidth`, `PanelLayout::SetLineWidth`) is cut to the integer below without a warning. Write whole numbers (`3.`); marker sizes, text sizes and alpha may be fractional.
- Rebin/Scale/Normalize on a ratio act on the result — use `.Both().Normalize()` etc. to act on the inputs.
- In C++ `AddData({"h", "src"}, "label")` is ambiguous (the brace list also converts to a `std::string`): write `AddData("h", "src", "label")` or `AddData(Data("h", "src"), "label")`. Without a label, `AddData({"h", "src"})` is fine.
- In bash, quote patterns containing `(`, `)` or `|`.
- Verify by generating: `plot <group> <name> png` and look at the file, or `plot ... list`. Warnings in the output name the plot and data.

## Where to look for more

- `reference/api.md` — every method of PlotManager, Plot, Pad, Axis, Data, Ratio, legends, texts and PanelLayout with its arguments (generated from the headers).
- `reference/drawing-options.md` — alias → ROOT option string for each data type (generated).
- `reference/cli.md` — `srp` and `plot` commands, modes and configuration files.
- https://scirooplot.github.io/SciRooPlot/api-changes.md — interface changes between versions, with what to change in existing plotting code; read it when user code was written for an older version.
- https://scirooplot.github.io/SciRooPlot/ — the web page: installation, tour, reference and example plots with their code; `srp example-cpp` / `srp example-py` create a runnable copy of it with example data.
