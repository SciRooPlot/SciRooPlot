---
name: scirooplot
description: Define, organize and generate ROOT plots with SciRooPlot (C++ or Python). Use when writing or changing a DefinePlots.cpp / DefinePlots.py program, calling the `plot` or `srp` command line tools, defining data sources, base plots, ratios, tree/RNTuple/table projections or drawing option aliases, or when a user mentions SciRooPlot.
---

# SciRooPlot

SciRooPlot is a C++/Python library on top of ROOT that separates *defining* plots from *drawing* them.
Describes SciRooPlot 4.0.0 (ROOT >= 6.36.04). Repository: https://github.com/SciRooPlot/SciRooPlot — docs: https://scirooplot.github.io/SciRooPlot/

**Version check** (once per session, where you can run commands): the first line of `srp info` names the installed version.
- Older than the version above: tell the user to run `srp update` (afterwards, run each Python plotting program once); until then, code written from this skill may use what the installed version lacks.
- Newer: this skill is outdated. Tell the user to import the current one, https://scirooplot.github.io/SciRooPlot/skill/scirooplot.skill, and meanwhile take the interface from https://scirooplot.github.io/SciRooPlot/skill/scirooplot/reference/api.md and the changes from https://scirooplot.github.io/SciRooPlot/api-changes.md.

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
srp projects [<project>] | srp settings | srp help
srp verbosity (debug|log|info|warning|error|silent) ; srp plotmode (show|pdf|png|...) ; srp matchmode (exact|contains)
```

## The `plot` app

```bash
plot <group> <name> [<mode>]        # group and name are regular expressions (whole match by default)
plot myGroup .+                     # all plots of myGroup and its subgroups
plot 'myGroup$' .+                  # without subgroups;  plot myGroup/QA ctrl.*  selects a subgroup
plot thesis 'fig_(a|b)' pdf         # quote patterns with ( ) | in bash
```
Modes: `show` (interactive, the default unless `srp plotmode` sets another; `s`/`a` next/previous, `q` closes; double-click a legend, text, image or shape to print its name and coordinates), `spread` (one window per plot), `list`, `print` (dump definition), `pdf`, `eps`, `ps`, `svg`, `png`, `jpg`, `gif` (all matching plots as one animation in the order they were defined, one plot per frame in a group of its own, e.g. a loop over `SetView` angles, see the example group `animations`; `gif+8++` 8 centiseconds per frame, looping endlessly), `html` (ROOT 6.42 or newer), `json`, `xml`, `root`, `macro` (ROOT macro), `file` (canvases into a ROOT file), `data` (the input objects as read). All keys, gif timing and file naming: `reference/cli.md`.

## Program skeleton

C++ (`using namespace SciRooPlot; using std::vector; using std::string; using Data = Plot::Pad::Data; using LegendEntry = Plot::Pad::LegendEntry; using StatsBox = Plot::Pad::StatsBox;`):
```cpp
#include "SciRooPlot/PlotManager.h"
using namespace SciRooPlot;
int main() {
  PlotManager pm("myProject");                                  // project name = the one given to srp
  pm.AddDataSource("data", "/path/run1.root");                  // see "Data sources"
  pm.AddDataSource("mc", "/path/simulation.root");
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d"));              // layouts used as third Plot argument
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d_ratio"));
  {
    Plot plot("spectrum", "paper/pt", "1d_ratio");              // name, group[/subgroup], base plot
    plot[1].AddData("hPt", "data", "data <mean[.2f]>").SetDrawOption(points).SetColor(kBlue + 1);
    plot[1].AddData("hPtMC", "mc", "simulation").SetDrawOption(curve);
    plot[2].AddRatio({"hPt", "data"}, {"hPtMC", "mc"}).SetCorrelated();
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
Scripts can make plots without the `plot` app: `pm.SetOutputDirectory("/path/out")` (a registered project has one already), then `pm.GeneratePlots("pdf", "name", "group")` (a mode of `plot` other than `list`, then the name and the group pattern: the reverse order of `plot <group> <name>`) on the plots added so far, `pm.LoadPlots(name, group)` to read saved definitions back first, `pm.ListPlots()`.

## Data sources

`pm.AddDataSource(name, input | [inputs])` — a data source is one data set made of its inputs:
- `"/path/a.root"` whole file; `"/path/a.root:dir/list"` a directory or TList inside it; `"/path/folder/"` all ROOT and table files below a directory; wildcards `AO2D_*.root:DF_*` (files and folders inside files); `${HOME}/...` environment variables; `"/path/table.csv"` (also `.tsv`, `.dat`, `.txt`, `.tab`, delimiter detected; data name = file name without the ending).
- Within a data source the first input containing a requested name wins; trees and RNTuples found in several inputs are chained. The files of a directory or wildcard are taken alphabetically (`run10.root` before `run2.root`). Train/test samples etc. therefore belong in separate sources.
- The data name may be a path inside the input: `AddData("folder1/hist", "src")`. A stored `THStack`/`TMultiGraph` is such a folder of its histograms/graphs (`"myStack/hPion"`; `srp print file.root` lists the names). Sub-sources are allowed in `AddData` as `"src:folder"` (restricts the search to that folder) — the folder is part of the source, never of the name.
- Without a path, a name is searched in the folder of the data source first, then in its sub-folders/lists one after the other in stored order, each completely (depth-first); the first match counts. If a name exists more than once, give the path.
- `AddDataSource(name, TObject*)` or a vector of objects adds in-memory objects; they are copied into the project, so they may be deleted afterwards. A named list (TList, TObjArray, ...) or a TDirectory becomes a folder of the data source (`AddData("myList/myHist", "src")`, trees inside are copied with all entries); a nameless list (ROOT reports its class name) or an open TFile adds its contents directly. Folders of the same name are merged, an object of an existing name replaces it (with a warning).
- Calling `AddDataSource` again with the same name appends the new inputs; `AddDataSource(name, inputs, true)` replaces them.

## Plot, pads, axes

```cpp
Plot plot("name", "group/sub", "basePlot");  Plot copy(plot, "name2", "group2");   // copy keeps everything
plot.SetDimensions(710, 710[, fixAspect]).SetTransparent().AppendGroup("QA");
plot[0]...        // defaults for all pads (text font/size, margins, colour lists, ref function); a pad's own value wins
plot[1].SetPosition(x1,y1,x2,y2).SetMargins(t,b,l,r).SetFrameFill(1001, 10).SetPalette(kBird).SetView(theta, phi)   // 3D view in degrees (ROOT: 30, 30)
plot[1].SetDefaultMarkerColors({kBlack, kRed+1}).SetDefaultLineStyles({kSolid, kDashed})   // cycled over the data
// without a colour list data takes kBlack, kP10Blue, kP10Red, kP10Violet, kP8Orange, kP10Ash, kP10Brown, kTeal + 3 (every plot; fills only with a fill style set)
// dashed/dotted lines on curves of hundreds of points come out solid in png (ROOT restarts the pattern per segment; pdf is fine): fewer points or a solid line in another colour
// every colour argument takes a ROOT colour (kRed + 1), "#rrggbb" or {r, g, b} from 0 to 1 (Python: a tuple); transparency only with the alpha setters
plot[1].SetDefaultColors(gradient)   // {{"#000080", 0.0}, {kRed, 0.5}, {{1.0, 1.0, 0.0}, 1.0}}: (colour, position) or {r,g,b,position}, all in [0,1], positions not decreasing (end colours beyond the first/last); also SetPalette(gradient)
plot[1].SetRefFunc("1");              // reference line/function in a ratio pad (drawn first, black); SetRefFunc("") removes the one of the base plot
plot[1]['X'].SetTitle("#it{x}").SetRange(a, b).SetLog().SetGrid().SetNumDivisions(305).SetMaxDigits(3).SetNoExponent()
            .SetTitleFont(43).SetTitleSize(28).SetTitleOffset(1.1).SetLabelFont(43).SetLabelSize(24)   // sizes in pixels only with font 43; with the default font 42 they are fractions of the pad
            .SetOppositeTicks().SetTickOrientation("+-").SetTimeFormat("%H:%M")   // ticks every fixed number of seconds: not on calendar years; for long ranges store decimal years on a plain axis
            .SetHideClippedLabels(false) // the base plots drop a label whose text would reach beyond the pad (top label of a ratio pad at the shared edge); false keeps it
            .SetLabelOffset(0.01).SetLabelAngle(45.).SetTitleCenter().SetTickLength(0.03).SetAxisColor(kGray + 2)   // label angle: alphanumeric bin labels of x only
plot[1]['Z'].SetColorScaleOffset(0.01).SetColorScaleWidth(0.05)   // colour scale beside the frame, fractions of the pad width ('C' for a TH3); room for it: PanelLayout().SetColorScaleMargin(pixel)
plot[1].SetRedrawAxes()               // axes (ticks) drawn again on top of the data, e.g. over filled areas
plot[1]['Y'].SetRoomAboveMax(0.3).SetRoomBelowMin(0.1)    // 30 % / 10 % of the frame free beyond everything drawn in the pad (any axis; error bars
                                                         // where ROOT counts them; log axes in decades); an explicit SetMinRange/SetMaxRange wins on its side
```
Pads count from 1; data in a pad from 1 (`plot[1](2)` or `plot[1].GetData(2)` modifies the second data later). The first data drawn defines the axis frame unless another one calls `.SetDefinesFrame()`; an input that should only give the frame (bin labels, a binning) is set with `plot[1].SetFrame("name", "source")`: it is not drawn and not numbered. A pad without data needs no dummy input: its X and Y ranges (`plot[1]['X'].SetRange(0.0, 10.0)`) give an empty frame.

Text sizes, three levels: for a whole class of plots change the base plot (`MakeBasePlot("1d", PanelLayout().SetTextSize(24).SetTitleSize(30))`, see below: the only way that keeps all panels consistent); for one plot set the pad (`plot[1].SetDefaultTextSize(s)`: labels, legends and texts of that pad; `plot[0]` for all pads, except in base plots with several panels (`1d_ratio`, `1d_2x1`, …), whose pads carry their own text sizes, margins and tick lengths, which win over `plot[0]`: set those per pad) or the axis (`plot[1]['Y'].SetTitleSize(s).SetLabelSize(s)`); for one box `AddText(...).SetTextSize(s)`, `GetLegend(1).SetTextSize(s)`. With the default font 42 a size is a fraction of the pad, so the same number is smaller in a smaller pad; font 43 (`SetDefaultTextFont(43)`, `SetTitleFont(43)`, `SetTextFont(43)`) takes pixels of the plot.

Inset (a zoom inside the frame): an extra pad placed with `SetPosition` in fractions of the plot; pads are drawn in the order of their numbers, so the inset covers what is below it, and its text sizes are fractions of the small pad:
```cpp
plot[2].SetPosition(0.5, 0.5, 0.88, 0.88).SetMargins(0.05, 0.22, 0.3, 0.05).SetDefaultTextSize(0.09);
plot[2].AddData("diphoton/mgg_data", "higgs");                      // no label: no legend row
plot[2]['X'].SetRange(120, 130).SetNumDivisions(505).SetTitleSize(0.1).SetLabelSize(0.08).SetHideClippedLabels(false);
plot[2]['Y'].SetRange(1500, 2600).SetNoExponent().SetNumDivisions(505).SetTitleSize(0.1).SetLabelSize(0.08).SetTitleOffset(1.4).SetHideClippedLabels(false);
```

Base plots: `PlotManager::MakeBasePlot("<type>[_wide|_tall][_<cols>x<rows>][_ratio][_pull][_gap]")` with type `1d` or `2d` (colour scale), e.g. `1d`, `1d_ratio`, `1d_ratio_pull`, `2d`, `1d_wide`, `1d_2x1`, `1d_2x1_ratio`, `1d_2x2`, `2d_2x1`. Pads are numbered row by row; ratio pads (a line at 1) and pull pads (a line at 0, below the ratio) follow their panels; `2d_<cols>x<rows>` grids share one colour scale in a cell of its own after the panels of the first row (`2d_2x2`: panels 1, 2, 4, 5, scale 3; `_gap` keeps a scale per panel). Modify and re-add them (`auto bp = PlotManager::MakeBasePlot("1d"); bp[1]['X'].SetColor(kRed); pm.AddBasePlot(bp);`). Other text sizes, line widths or margins go in as the lengths of a `PanelLayout` in pixels: `PlotManager::MakeBasePlot("1d_ratio", PanelLayout().SetTextSize(24).SetTitleSize(30))` keeps them equal in all panels; setting `SetDefaultTextSize` on the pads of a base plot afterwards does not (each pad holds a size relative to its own dimensions, and pad 0 is overridden by them in multi-panel layouts). Other proportions go in as a `BasePlotShape`: `MakeBasePlot("1d_wide_ratio", PanelLayout(), BasePlotShape().SetAspectRatio(4.0 / 3.0).SetRatioHeight(0.25))` (`SetPullHeight` for pull panels). Or build your own with `PanelLayout` (see "Own layouts with PanelLayout" below). Touching panels share their axes (x within a column, y within a row: range, log, time format; without a range the union of their automatic ranges) and a row/column of them gets its outer title once (`PanelLayout().SetCommonTitles(false)` keeps one per panel); other pads share with `plot.ShareAxis('Y', {1, 2})`, `ShareAxis('Y', {})` undoes the groups of an axis; `ShareAxis('Z', {…})` gives 2D panels one colour range (shown once by a `ColorScale()` cell of the layout).

## Data: adding, appearance, labels

```cpp
plot[1].AddData("hist", "src", "label");                    // histogram, graph, function, TEfficiency, tree, RNTuple, table — type is detected
// also TH2Poly, TProfile2D/TProfile3D and THn/THnSparse (n dimensions: select with .Project/.Profile, see Modifiers)
plot[1].AddFunction("sin(x)/x", "label");  plot[1].AddPoints({x...}, {y...});  plot[1].AddPoints({x1, x2}, {y1, y2}, "label").SetDrawOption(line);  // a line as data
// shapes are not data (never in the axis ranges, not numbered, no default colours, no legend row), drawn after the data unless SetBehindData()/SetOnTop():
plot[1].AddVerticalLine(125.);  plot[1].AddHorizontalLine(1.);  plot[1].AddRegionX(120., 130.).SetBehindData();  plot[1].AddRegionY(y1, y2);
plot[1].AddLine({x1, y1}, {x2, y2});  plot[1].AddArrow({x1, y1}, {x2, y2}).SetArrowHead("|>").SetArrowSize(0.03);  plot[1].AddRegion({x1, y1}, {x2, y2});
plot[1].AddEllipse({x, y}, rx, ry).SetArc(0., 180.).SetRotation(30.);  plot[1].AddPolygon({{x1, y1}, {x2, y2}, {x3, y3}});  // line and fill setters as for data; SetPadCoordinates()/SetFrameCoordinates(): fractions of the pad or the frame instead of axis values
plot[1].AddFunction("[](double* x, double* p) { return std::exp(-x[0] * x[1]); }");  // C++ lambda as text: x[0..2] set the dimension, no parameters
// any callable (with captures, also from python): TF1 f("f", callable, xmin, xmax, 0); f.SetNpx(n); pm.AddDataSource("src", &f); then AddData("f", "src")
plot[1].AddData(myTemplate("hist"), "label");               // Data template: Data().SetDrawOption(curve).SetColor(kRed) then template("name"[, "src"])
.SetDrawOption(points | "HIST C")     // alias (preferred) or raw ROOT option string
.SetColor(c).SetAlpha(a)           // all of marker, line, fill at once; the setters below set only their part, the rest takes the data's default colour
.SetMarker(style, size, color).SetLine(style, width, color).SetFill(style, color)  // optional alpha last, default 1 (+ SetMarkerColor, SetLineStyle, ... SetFillAlpha)
.SetRangeX(a, b).SetMaxRangeX(b).SetRangeY(a, b)             // visible range of this data, independent of the axis
.SetLegend(2).SetLegendLabel("l").SetDefinesFrame().SetShowOverflowBins().SetContours(n).SetTextFormat("4.2f")
.SetKeepInputStyle()               // line, marker, fill stored with the input win over the pad defaults (what the data sets still wins)
.SetNpx(n[, ny])                   // points a function is drawn with (AddFunction: 1000 in 1d; inputs keep their own)
.SetTextAngle(45.)                 // angle of the values written by text / points_text
.SetShowStats("eMR", "title")   // statistics box of a histogram (letters of TStyle::SetOptStat; the optional title is its first line)
.SetStatsBox(StatsBox().SetAutoPlacement(top_left).SetFill(1001, kWhite))   // its placement and appearance: all setters of legend and text boxes
```
Labels may contain `<name> <title> <entries> <integral> <maximum> <minimum> <mean> <stddev>` with printf formats: `"avg = <mean[.2f]>"` (graphs: `<entries>` points, `<maximum>`/`<minimum>` of y, of z for 2D graphs, of the points inside the frame; `<integral>`, `<mean>`, `<stddev>`, `<maximum>`, `<minimum>` cover the drawn range, `<entries>` is the stored total), and from the definition `<source>`, `<projection>` (`x`, `y:x`, `prof. y:x`, weight in parentheses) and `<selection>` (filters joined with `&&`, entries range, projection ranges); histogram projections name the axes by the axis titles of the input (`x`, `y`, `z` where a title is empty), tree requests by the variables; an empty placeholder takes one separating space with it, so `"<name> <projection> <selection>"` never ends in a space. `"<name> <selection>"` tells apart requests differing only in the filter. Texts name their data by number: `AddText("#sigma = <stddev[.1f]:2> mm")` (data 2 of the pad), `<stddev[.1f]:1@4>` (data 1 of pad 4). ROOT TLatex syntax works in all texts (`#it{p}_{T}`, `#sqrt{s}`).

Without `SetDrawOption` the same rule holds for histograms and graphs: a fill set → `hist` (graph: `area`); a marker style or size set on the data, non-zero errors or a graph of a single point → `points`; otherwise `hist` (graph: `line`); 2D histograms, graphs and functions → `colz` (one colour axis covering all 2D data of the pad). The legend row follows what is drawn.
Drawing option aliases (one appearance for every type; histograms and graphs are interchangeable, the data is converted when ROOT lacks the option):

| 1D alias | look | 2D alias | look |
|---|---|---|---|
| `points` | markers + y errors | `col` / `colz`, `col_clamped` / `colz_clamped` | colour map (z = colour axis); cells below the z minimum left out / in the lowest colour |
| `points_xerr`, `points_endcaps`, `points_xerr_endcaps` | + x errors / end caps | `box`, `text`, `colz_text` | boxes, numbers, colour map with numbers |
| `points_line`, `points_text`, `points_arrows` | + line / values written / arrow ends (limits: one-sided errors) | `lego`, `lego_col(z)`, `lego_noborders` | lego |
| `line`, `curve` | polyline, smooth curve (no markers) | `surf`, `surf_col(z)`, `surf_fill(z)`, `surf_contours`, `surf_shaded` | surfaces |
| `hist`, `hist_open`, `bars`, `hbars` | steps, open steps, bars with gaps, horizontal bars | `cont`, `cont_col(z)`, `cont_fill(z)`, `cont_smooth(z)` | contours |
| `area`, `area_smooth` | filled to zero | `candle`, `candle_minmax`, `candle_mean`, `candle_notched`, `candle_points`, `candle_scatter`, `candle_meanline` | candle plots |
| `band`, `band_smooth` | filled error band | `violin`, `violin_minmax` | violin plots |
| `boxes`, `boxes_nomarkers`, `brackets` | error boxes, brackets | `col_polar`, `colz_polar`, `lego_polar`, `lego_col_polar`, `surf_polar`, `surf_col_polar` | polar coordinates (x angle, y radius) |
| `hist_clipped`, `hist_open_clipped`, `curve_clipped` | as `hist`, `hist_open`, `curve`, but cut at the frame instead of running along its edge (drawn as graph) | | |

Axes of a pad are named by where they are drawn: with `hbars` the bins run along `'Y'` (vertical) and the contents along `'X'` (horizontal), for ranges, titles, room and shared axes alike.

The suffix always adds one feature (`_xerr`, `_endcaps`, `_smooth`, `_nomarkers`, `_clipped`; 2D: `_col` lines coloured by content, `_fill` filled, trailing `z` adds the colour axis). An alias a type cannot draw gives a WARNING and ROOT's default.
Functions (`TF1`): `points`, `line`, `curve`, `area`, `area_smooth` (a fill colours the area between curve and zero; `line`/`curve` drop it). 2D functions and 2D graphs: the `col`, `lego`, `surf`, `cont` aliases (no `box`, `text`, `colz_text`, candles). 2D graphs only, drawn in 3D: `triangles`, `triangles_wire`, `triangles_col(z)`, `triangles_fill(z)` (Delaunay triangles), `points`, `points_col(z)` (markers coloured by z; error bars for TGraph2DErrors), `line`; such data must be the first in its pad or follow 3D data (lego, surf).
3D histograms (`TH3`, default `colz`): `box` (box volume follows the content), `col`/`colz` (equal boxes coloured by content), `iso` (iso surface at the mean content); `Z` is the third binned axis, the colour scale of the contents is the axis `'C'` (`plot[1]['C'].SetRange(a, b).SetTitle("counts")`, only there; linear only, ROOT colours the boxes linearly; `SetContours(n)` sets its number of colour levels as in 2D). 3D functions (`TF3`): `iso` (also the default), the surface f(x, y, z) = 0 (write f − c for the value c; white faces, mesh in the data colour; a fill colour fills the faces).
Polar: the `_polar` aliases take x as the angle (one full turn) and y as the radius; `col_polar`/`colz_polar` draw a round disc in a square frame with a radial axis below it (no cartesian axes, no cell outlines unless a line width is set). Polar graphs (`TGraphPolar`, angle in radians) are drawn on a polar grid filling the pad (use a square base plot such as `1d`); x/y ranges, divisions and label styles apply to angle/radius; the raw option of the first polar graph sets the grid as in ROOT (`D`/`G` degrees/grads, `N` no angle labels, `O` angle labels along the radius); shapes are not drawn in such a pad; automatic legends and texts go into its corners, beside the grid and its labels.

## Modifiers (applied in this order, before drawing)

`ScaleX/Y/Z(f)` (on the value axis, i.e. `ScaleY` of 1D data and `ScaleZ` of 2D data, they act as `Scale(f)` at the end) → `RebinX/Y/Z(n)`, `RebinXY`, `RebinXYZ` → `DivideBinWidth()` → `Smooth(n)` → `Normalize([isDensity], [min, max])` (to integral) → `Cumulative([forward])` → `NormalizeToMaximum([min, max])`, `Scale(f)` → then the range cuts `SetRangeX/Y`.
With `min, max` the integral or maximum is taken only in that x range (the bins reaching into it; graphs: the points within it), e.g. `Normalize(false, 0.6, 0.9)` to compare shapes in a peak region while showing everything.
`Density()` = `DivideBinWidth().Normalize()`: a probability density from counts. `Normalize(true)` is only for data that already is a density (e.g. dN/dpT); `Density()` on such data would divide by the bin widths twice.
So `Normalize().Cumulative()` is a CDF and `Cumulative().NormalizeToMaximum()` ends at 1.0

Projections of histograms: `.ProjectX(yMin, yMax[, userCoords])`, `.ProjectY(...)`, `.ProfileX/Y(...)`, generic `.Project({dims}, {{axis, lo, hi}, ...}[, userCoords])` and `.Profile(...)` for n-dimensional inputs (dims in the order they become x, y, z of the result). They follow ROOT's conventions per class: a profile over the whole range leaves out the under- and overflow of the axis it averages over (`.ProfileX(0, n + 1)`, n its number of bins, takes them in), while a projection counts the flows of the axes it integrates out; the entries (stats box) are ROOT's: every fill for a THn, the effective entries in range for a TH2 or TH3; a THn or THnSparse has no profiles (ROOT has none: refused with an error).

## Ratios

```cpp
plot[2].AddRatio({"num", "src"}, {"den", "src"}, "label").SetCorrelated();   // binomial errors for sub-samples
plot[2].AddRatio(templ("num"), Data("den", "src"));                           // from Data objects (ratio keeps the numerator's look)
plot[2].AddRatio({"num", "src"}, {"den"});                                   // denominator without data source: the numerator's
plot[2].AddRatio(...).Both().Normalize().Result().SetColor(kRed);            // modifier scope: Numer() / Denom() / Both() / Result() (default)
plot[2].AddRatio({"data", "src"}, Function("1250*exp(-x)"));                // by a formula of the plot definition (= {"1250*exp(-x)", kUserFunctions})
plot[2].AddRatio({"model", "src"}, {"data", "src"});                         // model / data: a function numerator is evaluated at the data points, errors from the data
```
Incompatible binnings are divided via spline interpolation (a histogram through its filled bins: inside an empty bin of the denominator the ratio has no value); a profile is divided as the histogram of its means (after its `Numer()`/`Denom()` modifiers). Modifiers in `Result()` mode (the default) act on the ratio itself; Rebin, Cumulative, DivideBinWidth and Normalize there warn. Data selections (projections, profiles, `Scatter`, `Define`, `Filter`, `Entries`) always act on the inputs: in `Result()` mode they are taken as `Numer()` with a warning — write `Numer()`, `Denom()` or `Both()` explicitly.

## Trees and tables (TTree, RNTuple, CSV)

```cpp
.Project1D("pt")                                  // 100 automatic bins (integer columns: bins of whole-number width); {"pt", 200}, {"pt", 200, {0.1, 20.0}}, {"pt", {edges...}}
.Project2D({"pt", 50}, {"eta", 50})  .Project({dims...}[, "weight"])   // up to 3D
.Profile1D({"eta", 70}, "pt")  .Profile2D(x, y, "pt")                   // <pt> vs eta
.Scatter("x", "y"[, "ex", "ey"])  .Scatter(x, y, exl, exh, eyl, eyh)    // graph from columns
.Scatter("x", "y", "z")                                                  // points in 3D: a 2D graph (TGraph2D; scalar columns), e.g. points_colz or triangles
.Filter("abs(eta) < 0.8").Filter("z == 8")  .Define("r", "sqrt(x*x+y*y)")  .Entries(100) / .Entries(800, 1200)
.Join("scores")                                  // row by row, columns as 'scores.col' or 'col' if unique
.Join("truth/events", {}, "truth")               // tree in another folder, with alias
.Join({"events", "otherSource"}, {"run", "event"})   // from another data source, matched by up to two integer keys
```
Column expressions and filters are C++ expressions over the column/leaf names (evaluated with RDataFrame when the plot is generated). Without explicit bins, 100 bins over the observed range are used (integer columns: bins of whole-number width). RNTuples (ROOT's newer table format, e.g. written by uproot's `f["t"] = df`) work like trees, but cannot be joined and are chained only when they have the same path in every file.

## Legends and texts

```cpp
plot[1].AddLegend();  AddLegend(top_right);  AddLegend(0.5, 0.8[, "title"]);   // corners: best_corner, top_left, top_right, bottom_right, bottom_left
plot[1].GetLegend(2).SetNumColumns(2).SetColumnSpacing(1.5).SetOrderTopDown().SetDefaultLineWidth(5.0).SetNoBox().SetTransparent();   // spacing in text heights; top down: fill a column first
plot[1].AddData("band", "src", "unc.").GetLegendEntry().SetSymbolElements("F");   // style of a data's row (label stays on the data; SetLegendEntry(entry) applies a prepared one); symbol element letters: L line, P marker, F fill, E error bar
plot[1].GetLegend(1).AddEntry("custom").SetSymbolElements("L").SetLine(kSolid, 2.0, kRed);   // row of its own (AddEntry(): empty line); SetRefData(n): symbol of data n
plot[1].AddData("line", "src", "model").GetLegendEntry().AddSymbolOf(1);   // the row also shows the symbol of data 1 (e.g. its band) in that data's look; AddSymbolOf({1, 4}) for several
plot[1].GetLegend(1).AddEntry("#bf{group}").BeforeData(4);   // placed relative to data n of the pad (as in plot[1](n)): AtTop(), BeforeData(n), AfterData(n); default: at the end
plot[1](1).SetLegendEntry(LegendEntry().AfterData(2));       // the row of a data item placed the same way (a model drawn under the data, listed after it)
plot[1].AddText({"line one", "line two"});  AddText(top_left, "text");  AddText(0.2, 0.9, "text").SetTextFont(43).SetTextSize(20).SetTextAlign(13);   // font 43: size in pixels
plot[1].AddImage(0.2, 0.7, "/path/logo.png").SetWidth(0.3);  AddImage(top_right, "logo.png");   // default width 0.2 of the frame, height from the image; in pdf files only with ROOT >= 6.42
```
Positions are the upper left corner as fractions of the frame ((0, 0) its lower left, (1, 1) its upper right corner; negative or above 1: outside of it; in an empty layout cell fractions of the cell), the same place in every panel of a multi-panel layout (also `SetSize`); `.SetPadCoordinates()` switches to fractions of the pad, `.SetUserCoordinates()` to axis units; for all boxes of a plot: `plot[0].SetDefaultBoxCoordinates(pad_coordinates)` (shapes: `SetDefaultShapeCoordinates`). Box geometry (legends, texts, stats boxes): `.SetSize(w, h)` instead of the measured size, `.SetMargin(m)`, `.SetLineSpacing(s)` (in text heights), `.SetBorder(style, width, color)`; `GetLegend(1).SetSymbolColScale(f)` widens or narrows the symbol column. Legend symbols are derived from what is actually drawn; override with `SetSymbolElements`/`SetDefaultSymbolElements`.

## Practices to suggest (this is where SciRooPlot saves work)

**Define appearance once, reuse it everywhere.** Each kind of data (measured points, simulation, model, systematics) should look the same in every figure. A `Data` without a name is a template for one kind; calling it with a name gives a data object with all its settings, so plots only name the data and a change of the template changes every figure:
```cpp
const Data measured = Data().SetDataSource("data").SetMarker(kFullCircle, 1.0, kBlack);
const Data simulated = Data().SetDataSource("mc").SetDrawOption(hist_open).SetLine(kSolid, 3.0, kRed + 1);
plot[1].AddData(measured("ptSpec"), "data");               // name from the template's data source
plot[1].AddData(simulated("ptSpec", "otherMC"), "MC");     // or another data source
plot[2].AddRatio(measured("ptSpec"), simulated("ptSpec"));
```
Templates carry everything a `Data` has (options, colours, ranges, modifiers, projections, tree queries) and can be kept in a `map` keyed by an enum. `.SetLayout(tmpl)` / `.ApplyLayout(tmpl)` take over only the appearance of a template (drawing options, text format, ranges, marker, line, fill, contours) on an existing data; `ApplyLayout` only what is set in it. A `Ratio` called with a name gives data with the ratio's appearance, without its denominator (use it with `AddData`, or as numerator or denominator of `AddRatio`). Put them (and label constants like the experiment name) at file scope of `DefinePlots`.

**Derive plots instead of repeating them.** `Plot variant(plot, "newName", "group")` copies a finished plot; add or change data and axes afterwards (`variant[1](2).SetColor(kRed)`). Generate plot families in loops — over centralities, cuts, runs, detector parts — with the loop variable in the name and group (`"raa_" + cent`, `AppendGroup("raaPerCentrality")`), so `plot heavyion/raaPerCentrality .+` makes the whole set.

**Put layout into base plots, not into plots.** Everything that is the same for a class of plots — dimensions, margins, fonts, colour sequence, axis conventions, ratio reference line — belongs into a base plot; the plots then only contain data, titles and ranges (data in a base plot is ignored with a warning). Start from the shipped ones (`MakeBasePlot("1d")`, `"1d_ratio"`, `"2d"`, `"1d_2x1"`, …; modify and `AddBasePlot` under the same or a new name) and keep `plot[0]` for per-plot defaults (`plot[0]['X'].SetRange(...)` acts on all pads).

**Own layouts with PanelLayout** when the grid of panels is not one of the shipped names: panels are placed in rows in units of the standard panel, the sizes (text, ticks, line widths, margins in pixels) are shared, so plots of different layouts look alike in a document:
```cpp
PanelLayout sizes;                                            // the defaults; or sizes.SetTextSize(31.52).SetTitleSize(39.4).SetLineWidth(5)
const vector<vector<Panel>> rows = {{Panel(), Panel()},       // two panels sharing their y axis
                                    {Panel(1, 0.37), Panel(1, 0.37)}};  // a ratio row below each
Plot layout("1d_2x1_ratio_mine", PanelLayout(sizes, rows));   // pads numbered row by row: 1 2 / 3 4, with the look of the shipped base plots
layout[0].SetDefaultColors({kBlack, kBlue + 1, kRed + 1});    // change any of the look with the ordinary setters
for (uint8_t ratioPad : {3, 4}) layout[ratioPad].SetRefFunc("1");
pm.AddBasePlot(layout);
```
Cells: `Panel(w, h)` scales a panel; `Gap()` between panels makes them independent (own axes); `Empty()` / `Empty(w, h)` leaves a pad for text; `Hole()` a blank cell without pad number (e.g. a corner beside a ratio panel); `Panel().ColorScale()` reserves room for the panel's own colour scale, while a `ColorScale()` cell holds one scale for the z group of the panel left of it (`ColorScale(padID)`: of that panel). `.Span(columns, rows)` lets a cell cover several columns/rows, listed once in the row where it starts, e.g. `{{Empty(1, 0.2).Span(3)}, {Panel(), Panel(), Panel()}}` for a common legend strip, rows of it filled with `legend.AddDataEntry(padID, dataID)`. `.SpacingAbove(px)`/`.SpacingLeft(px)` sets a touching cell off from the cells above/left of it, for the whole boundary, instead of `SetPanelSpacing` (e.g. `Panel(1, 0.37).SpacingAbove(12)` for a ratio panel). The source of `PlotManager::MakeBasePlot` is the full worked example.

**Keep inputs out of the code.** Machine-specific paths go into project variables (`srp set <project> inputDir /path`, `pm.GetProjectProperty("inputDir")`) or environment variables in the data source strings (`${HOME}/...`); the data source names stay stable across machines. In-memory objects (a graph computed in the program) are registered with `pm.AddDataSource("glauber", &graph)` and used like file data.

**Let the plot show numbers.** Label placeholders (`"<mean[.2f]>"`, `"<entries>"`, `"<integral[.2e]>"`) and `points_text` save hand-copied values; `AddColorOverview("colors", "overview")` renders SciRooPlot's default colour sequence with names and indices (`AddColorOverview("colors", "overview", "1d_mine", 1)`: the colours that pad of a base plot gives its data) (`plot overview colors`), `srp colorwheel` shows all of ROOT's named colours.

## C++ vs Python

Identical names and chains. Differences:
- `from SciRooPlot import *` exports everything: classes, the drawing option aliases (`points`, `colz`, …), `box_placement_t` corners and ROOT's colour/style constants (`kRed`, `kFullCircle`, `kDashed`, `kBird`, …), so `kRed + 1` works.
- Braces become tuples/lists: `Project1D(("pt", 200, [0.1, 20.0]))`, `Project1D(("pt", [0.1, 0.5, 1.0]))`, `AddRatio(["num", "src"], ["den", "src"])`, `Join(["events", "otherSource"], ["run", "event"])`, colour lists `[kBlack, "#1f77b4", (0.2, 0.6, 0.3)]`, gradients `[("#0000ff", 0.0), (kRed, 1.0)]` or `[(0.0, 0.0, 1.0, 0.0), (1.0, 0.0, 0.0, 1.0)]`.
- A list `["name", "src"]` is accepted wherever a `Data` is expected (the Python form of the C++ braces); templates are called the same way: `tmpl("name", "src")`.
- Optional arguments may be left out as in C++; `std::optional` values are `None` when unset.
- No `SRC_DIR` macro: use `Path(__file__).resolve().parent`. The program is plain Python; `plot` runs it with the interpreter from the environment script.
- Python projects are created with `srp init-py` / `srp example-py` and otherwise behave identically.
- numpy/pandas data in memory (Python only): `histo(name, values, bins=100, range=None, weights=None)`, `histo2d(name, x, y, bins=None, range=None, weights=None)`, `graph(name, x, y, xerr=None, yerr=None)` and `table(name, data, columns=None)` (dict of arrays, structured array, DataFrame) return ROOT objects for `pm.AddDataSource("src", [...])`; tables work with `Project1D`/`Filter`/`Define`.
- Text, legend and image boxes are made only with `AddText`/`AddLegend`/`AddImage`; `StatsBox()` and `LegendEntry()` take no arguments, as in C++ (use their setters).

## Pitfalls that cost time

- bash: `plot` is an alias that turns off file-name expansion for its patterns; after `||`, `!` or `time` it needs braces (`make || { plot g .+; }`, `time { plot g p png; }`), otherwise it runs with the wrong condition or unprotected patterns.
- Nothing shows up / "Found no plots matching": the wrong project is selected (`srp projects`, `srp select`), or the program did not run `pm.SaveProject()`, or the group/name regex does not match whole names (`srp matchmode contains` relaxes this).
- `plot` uses the program registered for the project (`srp projects @current`), not the file in the current directory.
- Data not found: check the data source name and that the object name (or path) exists in one of the inputs; `plot ... print` shows the definition; the error of `plot` names the missing data and its data source. For sub-folders use `"src:folder"` or a path in the name, never both for the same folder.
- The same data may be added several times to a pad; refer to data by its number (`plot[1](2)`), there is no lookup by name.
- Axis ranges live on the pad (`plot[1]['X'].SetRange`), data ranges on the data (`SetRangeX`); the first data (or `SetDefinesFrame`) decides the frame.
- Ranges, minimum and maximum stored with an input histogram or graph are ignored: set them in the plot definition. Exception: data marked `.SetDefinesFrame()` and the frame set with `SetFrame` keep them for the frame they draw (e.g. an empty dummy histogram with the y range stored); an explicit `plot[i]['Y'].SetRange` still wins. A projection or profile never applies stored ranges and warns when one sits on an axis it integrates out. Functions from inputs are drawn where they are defined, functions from `AddFunction` span the frame; `SetRangeX` overrides both (a function written from C++ code only has values within its own range).
- Fill colours are drawn for `hist`/`area`/`band`/`bars`; histograms drawn with `line`/`curve` are not filled even if a fill colour is set by the pad defaults.
- Line widths are whole pixels: ROOT stores them as integers, so a fractional width (`SetLine(s, 3.5, c)`, `SetLineWidth(2.5)`, `SetBorder(s, 1.5, c)`, `SetDefaultLineWidth`, `PanelLayout::SetLineWidth`) is rounded to the nearest integer, with a warning. Write whole numbers (`3.`); marker sizes, text sizes and alpha may be fractional. Exclusion contours: a graph drawn `line`/`curve` with width 100 × zone + width (`SetLine(kSolid, 1503., kBlack).SetFill(3354, kBlack)`) hatches a zone of width 15 on one side in its fill, as in ROOT (negative width: the other side).
- Rebin/Scale/Normalize on a ratio act on the result — use `.Both().Normalize()` etc. to act on the inputs.
- In C++ `AddData({"h", "src"}, "label")` is ambiguous (the brace list also converts to a `std::string`): write `AddData("h", "src", "label")` or `AddData(Data("h", "src"), "label")`. Without a label, `AddData({"h", "src"})` is fine.
- Input paths must be absolute (or start with an environment variable such as `${HOME}`): a relative path is skipped with a warning.
- `from SciRooPlot import *` also brings the drawing option aliases (`line`, `text`, `box`, `col`, …) and the helpers `graph`, `histo`, `table`: a user variable of the same name hides them.
- In bash, quote patterns containing `(`, `)` or `|`.
- Verify by generating: `plot <group> <name> png` and look at the file, or `plot ... list`. Warnings in the output name the plot and data.

## Where to look for more

- `reference/api.md` — every method of PlotManager, Plot, Pad, Axis, Data, Ratio, legends, texts and PanelLayout with its arguments (generated from the headers).
- `reference/drawing-options.md` — alias → ROOT option string for each data type (generated).
- `reference/cli.md` — `srp` and `plot` commands, modes and configuration files.
- https://scirooplot.github.io/SciRooPlot/api-changes.md — interface changes between versions, with what to change in existing plotting code; read it when user code was written for an older version.
- https://scirooplot.github.io/SciRooPlot/ — the web page: installation, tour, reference and example plots with their code; `srp example-cpp` / `srp example-py` create a runnable copy of it with example data.
