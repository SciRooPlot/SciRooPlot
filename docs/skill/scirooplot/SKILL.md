---
name: scirooplot
description: Define, organize and generate ROOT plots with SciRooPlot (C++ or Python). Use when writing or changing a DefinePlots.cpp / DefinePlots.py program, calling the `plot` or `srp` command line tools, defining data sources, base plots, ratios, tree/CSV projections or drawing option aliases, or when a user mentions SciRooPlot.
---

# SciRooPlot

SciRooPlot is a C++/Python library on top of ROOT that separates *defining* plots from *drawing* them.
Describes SciRooPlot 3.3 (ROOT >= 6.34). Repository: https://github.com/SciRooPlot/SciRooPlot — docs: https://scirooplot.github.io/SciRooPlot/

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
srp init-cpp <project> [<dir>]      # minimal C++ project (CMake), or init-py for Python
srp example-cpp <project> [<dir>]   # many commented example plots with example data (also example-py)
srp add <project> <program> [<outdir>]   # register an existing program
srp select <project>                # make it the current project
srp set <project> outdir <dir>      # where files are saved (subfolders per group)
srp set <project> <key> <value>     # own variables, read in code via pm.GetProjectProperty("<key>")
srp projects | srp show | srp settings | srp help
srp verbosity (debug|log|info|warning|error) ; srp plotmode (show|pdf|png|...) ; srp matchmode (exact|contains)
```

## The `plot` app

```bash
plot <group> <name> [<mode>]        # group and name are regular expressions (whole match by default)
plot myGroup .+                     # all plots of myGroup and its subgroups
plot 'myGroup$' .+                  # without subgroups;  plot myGroup/QA ctrl.*  selects a subgroup
plot myGroup                        # name omitted = all plots of the group
plot thesis 'fig_(a|b)' pdf         # quote patterns with ( ) | in bash
```
Modes: `show` (interactive, default: keys `s`/`a` next/previous, `q` quit, double-click a legend to print its coordinates), `list`, `print` (dump definition), `pdf`, `eps`, `ps`, `svg`, `png`, `jpg`, `gif` (`gif+<centiseconds>` animates all matching plots), `html`, `json`, `xml`, `root`, `macro` (ROOT macro), `file` (canvases into a ROOT file), `data` (the drawn data into a ROOT file).

## Program skeleton

C++ (`using namespace SciRooPlot; using Data = Plot::Pad::Data;`):
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
    plot[1].AddData("hPt", "data", "data <mean[.2f]>").SetOptions(points).SetColor(kBlue + 1);
    plot[1].AddData("hPtMC", "mc", "simulation").SetOptions(curve);
    plot[2].AddRatio({"hPt", "data"}, {"hPtMC", "mc"}).SetIsCorrelated();
    plot[1].AddLegend();                                        // auto-placed, or AddLegend(0.6, 0.9) / AddLegend(top_right)
    plot[0]['X'].SetRange(0.5, 20.).SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})");
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
- Within a data source the first input containing a requested name wins; trees found in several inputs are chained. Train/test samples etc. therefore belong into separate sources.
- The data name may be a path inside the input: `AddData("folder1/hist", "src")`. Sub-sources are allowed in `AddData` as `"src:folder"` (restricts the search to that folder) — the folder is part of the source, never of the name.
- `AddDataSource(name, TObject*)` or a vector of objects adds in-memory objects.

## Plot, pads, axes

```cpp
Plot plot("name", "group/sub", "basePlot");  Plot copy(plot, "name2", "group2");   // copy keeps everything
plot.SetDimensions(710, 710[, fixAspect]).SetTransparent().AppendGroup("QA");
plot[0]...        // defaults for all pads (text font/size, margins, colour lists, drawing options, ref function)
plot[1].SetPosition(x1,y1,x2,y2).SetMargins(t,b,l,r).SetFrameFill(10, 1001).SetPalette(kBird).SetView(theta, phi)
plot[1].SetDefaultMarkerColors({kBlack, kRed+1}).SetDefaultLineStyles({kSolid, kDashed})   // cycled over the data
plot[1].SetDefaultColors(rgbGradient)   // {{r,g,b,position},...} with r,g,b,position in [0,1]; also SetPalette(gradient)
plot[1].SetDefaultDrawingOptionHist(hist).SetDefaultDrawingOptionGraph(points).SetDefaultDrawingOptionHist2d(colz)
plot[1].SetRefFunc("1");              // reference line/function in a ratio pad (drawn first, black)
plot[1]['X'].SetTitle("#it{x}").SetRange(a, b).SetLog().SetGrid().SetNumDivisions(305).SetMaxDigits(3).SetNoExponent()
            .SetTitleSize(28).SetTitleOffset(1.1).SetLabelSize(24).SetOppositeTicks().SetTickOrientation("+-").SetTimeFormat("%H:%M")
```
Pads count from 1; data in a pad from 1 (`plot[1](2)` or `plot[1].GetData(2)` modifies the second data later). The first data drawn defines the axis frame unless another one calls `.SetDefinesFrame()`.

Base plots: `PlotManager::MakeBasePlot("<type>[_wide|_tall][_<cols>x<rows>][_ratio][_gap]")` with type `1d` or `2d` (colour scale), e.g. `1d`, `1d_ratio`, `2d`, `1d_wide`, `1d_2x1`, `1d_2x1_ratio`, `1d_2x2`, `2d_2x1`. Pads are numbered row by row; ratio pads follow their panels. Modify and re-add them (`auto bp = PlotManager::MakeBasePlot("1d"); bp[1]['X'].SetColor(kRed); pm.AddBasePlot(bp);`) or build your own with `PanelLayout` (`Plot plot("name", PanelLayout(sizes, rows))`, panels `Panel(w, h)`, `Gap()`, `Empty()`, `Panel().ZAxis()`).

## Data: adding, appearance, labels

```cpp
plot[1].AddData("hist", "src", "label");                    // histogram, graph, function, tree, csv — type is detected
plot[1].AddFunction("sin(x)/x", "label");  plot[1].AddPoints({x...}, {y...});  plot[1].AddLine({x1,y1}, {x2,y2});
plot[1].AddData(myTemplate("hist"), "label");               // Data template: Data().SetOptions(curve).SetColor(kRed) then template("name"[, "src"])
.SetOptions(points | "HIST C")     // alias (preferred) or raw ROOT option string
.SetColor(c).SetAlpha(a)           // all of marker, line, fill at once
.SetMarker(color, style, size).SetLine(color, style, width).SetFill(color, style)  (+ SetMarkerColor, SetLineStyle, ... SetFillAlpha)
.SetRangeX(a, b).SetMaxRangeX(b).SetRangeY(a, b)             // visible range of this data, independent of the axis
.SetScaleMinimum(f).SetScaleMaximum(f)                      // widen the automatic axis range
.SetLegend(2).SetLegendLabel("l").SetDefinesFrame().SetShowOverflowBins().SetContours(n).SetTextFormat("4.2f")
```
Labels may contain `<name> <title> <entries> <integral> <maximum> <minimum> <mean>` with printf formats: `"avg = <mean[.2f]>"`. ROOT TLatex syntax works in all texts (`#it{p}_{T}`, `#sqrt{s}`).

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
| `boxes`, `boxes_nomarkers`, `brackets` | error boxes, brackets | | |

The suffix always adds one feature (`_xerr`, `_endcaps`, `_smooth`, `_nomarkers`; 2D: `_col` lines coloured by content, `_fill` filled, trailing `z` adds the colour axis). An alias a type cannot draw gives a WARNING and ROOT's default.

## Modifiers (applied in this order, before drawing)

`ScaleX/Y/Z(f)` → `RebinX/Y/Z(n)`, `RebinXY`, `RebinXYZ` → `DivideBinWidth()` → `Smooth(n)` → `Normalize([toBinWidth])` (to integral) → `Cumulative([forward])` → `NormalizeToMaximum()`, `Scale(f)` → then the range cuts `SetRangeX/Y`.
So `Normalize().Cumulative()` is a CDF and `Cumulative().NormalizeToMaximum()` ends at 1.

Projections of histograms: `.ProjectX(yMin, yMax[, userCoords])`, `.ProjectY(...)`, `.ProfileX/Y(...)`, generic `.Project({dims}, {{axis, lo, hi}, ...}[, userCoords])` and `.Profile(...)` for n-dimensional inputs (dims in the order they become x, y, z of the result).

## Ratios

```cpp
plot[2].AddRatio({"num", "src"}, {"den", "src"}, "label").SetIsCorrelated();   // Bayesian errors for sub-samples
plot[2].AddRatio(templ("num"), Data("den", "src"));                           // from Data objects (ratio keeps the numerator's look)
plot[2].AddRatio(...).Both().Normalize().Result().SetColor(kRed);            // modifier scope: Numer() / Denom() / Both() / Result() (default)
```
Incompatible binnings are divided via spline interpolation. Modifiers and data selections on the ratio itself (`Result()` mode) are usually meaningless for Rebin/Project etc. and warn — write them explicitly with `Numer()`, `Denom()` or `Both()`.

## Trees and tables (ROOT trees, CSV)

```cpp
.Project1D("pt")                                  // 100 automatic bins; {"pt", 200}, {"pt", 200, {0.1, 20.}}, {"pt", {edges...}}
.Project2D({"pt", 50}, {"eta", 50})  .Project({dims...}[, "weight"])   // up to 3D
.Profile1D({"eta", 70}, "pt")  .Profile2D(x, y, "pt")                   // <pt> vs eta
.Scatter("x", "y"[, "ex", "ey"])  .Scatter(x, y, exl, exh, eyl, eyh)    // graph from columns
.Filter("abs(eta) < 0.8").Filter("z == 8")  .Define("r", "sqrt(x*x+y*y)")  .Entries(100) / .Entries(800, 1200)
.Join("scores")                                  // row by row, columns as 'scores.col' or 'col' if unique
.Join("truth/events", {}, "truth")               // tree in another folder, with alias
.Join({"events", "otherSource"}, {"run", "event"})   // from another data source, matched by up to two integer keys
```
Column expressions and filters are C++ expressions over the column/leaf names (evaluated with RDataFrame when the plot is generated). Without explicit bins, 100 bins over the observed range are used.

## Legends and texts

```cpp
plot[1].AddLegend();  AddLegend(top_right);  AddLegend(0.5, 0.8[, "title"]);   // corners: best_corner, top_left, top_right, bottom_right, bottom_left
plot[1].GetLegend(2).SetNumColumns(2).SetDefaultLineWidth(5.).SetNoBox().SetTransparent();
plot[1].GetLegend(1).GetEntry(1).SetLabel("custom").SetDrawStyle("L").SetLineColor(kRed);   // draw style letters: L line, P marker, F fill, E error bar
plot[1].AddText("line one // line two");  AddText(top_left, "text");  AddText(0.2, 0.9, "text").SetTextSize(20).SetTextAlign(13);
```
Positions are pad coordinates (0–1) of the upper left corner, `.SetUserCoordinates()` switches to axis units. Legend symbols are derived from what is actually drawn; override with `SetDrawStyle`/`SetDefaultDrawStyle`.

## C++ vs Python

Identical names and chains. Differences:
- `from SciRooPlot import *` exports everything: classes, the drawing option aliases (`points`, `colz`, …), `box_placement_t` corners and ROOT's colour/style constants (`kRed`, `kFullCircle`, `kDashed`, `kBird`, …), so `kRed + 1` works.
- Braces become tuples/lists: `Project1D(("pt", 200, [0.1, 20.0]))`, `Project1D(("pt", [0.1, 0.5, 1.0]))`, `AddRatio(["num", "src"], ["den", "src"])`, `Join(["events", "otherSource"], ["run", "event"])`, colour lists `[kBlack, kRed + 1]`, gradients `[(0., 0., 1., 0.), (1., 0., 0., 1.)]`.
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
- Fill colours are drawn for `hist`/`area`/`band`/`bars`; histograms drawn with `line`/`curve` are not filled even if a fill colour is set by the pad defaults.
- Rebin/Scale/Normalize on a ratio act on the result — use `.Both().Normalize()` etc. to act on the inputs.
- In C++ the constructors of `Plot`, `Data`, `PlotManager` are explicit: write `Data("h", "src")`, not `{"h", "src"}` where a `Data` is expected (brace lists are only accepted by `AddRatio`).
- In bash, quote patterns containing `(`, `)` or `|`.
- Verify by generating: `plot <group> <name> png` and look at the file, or `plot ... list`. Warnings in the output name the plot and data.

## Where to look for more

- `reference/api.md` — every method of PlotManager, Plot, Pad, Axis, Data, Ratio, legends, texts and PanelLayout with its arguments (generated from the headers).
- `reference/drawing-options.md` — alias → ROOT option string for each data type (generated).
- `reference/cli.md` — `srp` and `plot` commands, modes and configuration files.
- https://scirooplot.github.io/SciRooPlot/ — README with the commented example walkthrough; `srp example-cpp` / `srp example-py` create a runnable copy of it with example data.
