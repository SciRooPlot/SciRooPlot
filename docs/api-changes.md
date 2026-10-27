# Interface changes between versions

What changed for users between released versions of SciRooPlot, and what to change in existing plotting code, projects and scripts.

**How to migrate:** find the section that starts at your version and apply it and every section above it, from the bottom up. Each section lists the changes relative to the release before it. Where a later section renames something an earlier one introduced, applying the sections in order still gives the current name.

**Kinds**
- *renamed*, *removed*, *signature*: code stops compiling or fails at run time; search for the old name.
- *default*, *behaviour*: code still compiles and runs, but the plots or results differ. Check these by looking at the output.
- *input validation*: input that used to be accepted is now rejected (an exception, or an error and the call is ignored).
- *file format*: stored project files or output files changed; usually re-running the plotting program is enough.
- *CLI*: the `srp` and `plot` commands. *build*: installation, CMake and dependencies.

Names are given for C++; the Python name is the same unless stated. Saved definitions (`plots.info`, `dataSources.info`) are not converted: after updating, re-run the plotting program of each project once. `plot` re-runs a Python program only if a `.py` file is newer than `plots.info`, so touch the program or delete `plots.info`.

## 3.3.3 → 4.0.0

### Build, includes and logging

| Old | New | Kind | Action |
|---|---|---|---|
| ROOT ≥ 6.16, Boost ≥ 1.65, fmt ≥ 6.1.2 | ROOT ≥ 6.36.04, Boost ≥ 1.74, fmt ≥ 8.0.0 (fetched if older), pybind11 ≥ 2.9 (fetched) | build | Upgrade ROOT and Boost if older. |
| Python bindings built for any Python 3 found | Built only for the Python version of ROOT; otherwise disabled with a warning | build | Install the development package of ROOT's Python. |
| Configuring without a prefix installed to `/usr/local` | Installs to `<source>/install` | build | Pass `-DCMAKE_INSTALL_PREFIX` if you relied on the system prefix. |
| `add_plotting_executable` copied `INSTALL_RPATH` and include directories from the SciRooPlot target | Uses `INSTALL_RPATH_USE_LINK_PATH`; includes come from linking `SciRooPlot::SciRooPlot` | build | Re-configure existing user projects (delete `CMakeCache.txt`). |
| Installed `share/scirooplot/user/` and `share/scirooplot/TestFiles/` | `share/scirooplot/templates/{skeleton,example}`, `templates/CMakeLists.txt`, `ExampleData/` | build | Point code copied from the old template at your own data or at `ExampleData`. |
| `#include "SciRooPlot/Logging.h"` | `#include "SciRooPlot/Logger.h"` | renamed | Change the include. |
| `#include "SciRooPlot/Helpers.h"` (`split_string`, `str_contains`, `expand_path`, `RegexMatcher`, …) | not installed any more | removed | Drop the include and replace the helpers with your own code. |
| `#include "SciRooPlot/PlotPainter.h"` (`defaultDrawingOptions_Hist/Graph/Hist2d`) | not installed any more | removed | Drop the include. |
| Macros `DEBUG`, `INFO`, `LOG`, `WARNING`, `ERROR`, `PRINT`, `PRINT_INLINE`, `PRINT_SEPARATOR`, `SUPPRESS_STDERR`, `HERE`, `CHECK_TYPE`; Python `INFO()`, `PRINT()`, … | `SciRooPlot::logger::debug/info/log/warning/error/print/print_inline/print_separator`; Python `logger.info()`, `logger.print()`, … | removed | Replace `INFO(...)` by `logger::info(...)` (Python `logger.info(...)`) etc. |
| `logger::suppress_stderr(bool)` | — | removed | Remove the calls. |
| `logger::print`, `logger::print_inline` shown only at verbosity ≥ info, `print` partly to stderr | Always shown, all to stdout | behaviour | Use `logger::info` for output that should follow the verbosity. |

### Plot definitions: renamed, removed, changed signatures

| Old | New | Kind | Action |
|---|---|---|---|
| `Data::NormalizeToMaximum(bool normMaximum = true)` | `Data::NormalizeToMaximum(std::optional<double_t> min = {}, std::optional<double_t> max = {})` | signature | Remove the bool argument: `NormalizeToMaximum(true)` still compiles but now means `min = 1.0`, and `NormalizeToMaximum(false)` no longer switches it off. Same for `Ratio`. |
| `Data::Normalize(bool scaleBinWidth = false)`; Python keyword `scaleBinWidth` | `Data::Normalize(bool isDensity = false, std::optional<double_t> min = {}, std::optional<double_t> max = {})`; Python keyword `isDensity` | signature | Python: rename the keyword to `isDensity=`. |
| `Plot::SetPaintColorWheel(bool)` | `srp colorwheel [<file>]` | removed | Show the wheel with `srp colorwheel` (or save it to a file) instead of a plot. |
| `Ratio::SetDivideNormalized(bool scaleBinWidth = false)` | `Ratio::Both().Normalize(isDensity)` | removed | Replace by `.Both().Normalize(<same bool>)` and continue with `.Result()` before modifiers meant for the ratio. |
| `Pad::SetDefaultDrawingOptionGraph/Hist/Hist2d(drawing_options_t)`, `Pad::GetDefaultDrawingOption*()` | — | removed | Set `SetDrawOption(alias)` on the data where needed (see the default rule below). |
| `Data::SetDefinesFrame(bool dontDraw = false)` | `Data::SetDefinesFrame()` (the data is drawn); a frame that is not drawn: `Pad::SetFrame(name, dataSource)` or `Pad::SetFrame(const Data&)`, or no input at all (X and Y ranges of a pad without data) | signature | Replace `AddData(…).SetDefinesFrame(true)` by `plot[i].SetFrame(…)`, or drop the dummy input and set `plot[i]['X'].SetRange(…)` and `['Y'].SetRange(…)`. Same for `Ratio`. |
| `Data::UnsetRangeX()`, `Data::UnsetRangeY()` | `Data::SetRangeX()`, `Data::SetRangeY()` (no arguments) | renamed | Replace the calls. Same for `Ratio`. |
| `Plot::Pad::LegendBox::LegendEntry` | `Plot::Pad::LegendEntry` | renamed | Update the qualified type name. |
| `Data::SetOptions(const std::string&)`, `Data::SetOptions(drawing_options_t)`, `Data::UnsetOptions()`, enum `drawing_options_t`; Python keyword `options` | `Data::SetDrawOption(const std::string&)`, `Data::SetDrawOption(draw_option_t)`, `Data::SetDrawOption()` (no argument: unset), enum `draw_option_t`; Python keyword `drawOption` | renamed | Replace the names; `UnsetOptions()` becomes `SetDrawOption()`. Same for `Ratio`. |
| `LegendEntry::SetDrawStyle(const std::string&)`, `LegendBox::SetDefaultDrawStyle(const std::string&)` | `LegendEntry::SetSymbolElements(const std::string&)`, `LegendBox::SetDefaultSymbolElements(const std::string&)` | renamed | Replace the names (the letters `L`, `P`, `F`, `E` are unchanged). |
| `LegendBox::GetEntry(uint8_t entryID)` | `Data::SetLegendEntry(const LegendEntry&)` for the row of a data item; `LegendBox::AddEntry(const std::string& label = "")` for extra rows (position via `AtTop()`, `BeforeData()`, `AfterData()`) | removed | Style data rows with `plot[p](i).SetLegendEntry(LegendEntry().Set…)`, add extra rows with `AddEntry`. |
| `TextBox::SetTextAlign(const std::string& align)` (`"left"`, `"center"`, `"right"`) | `TextBox::SetTextAlign(int16_t align)` (ROOT `ETextAlign`, e.g. `kHAlignCenter`; also exported in Python) | signature | Replace the strings by `kHAlignLeft`, `kHAlignCenter`, `kHAlignRight`. |
| Text lines separated by `" // "` in `AddText` / `SetText` | A list of lines (`AddText({"a", "b"})`, Python `["a", "b"]`) or `"\n"`; `" // "` is printed as it is | behaviour | Replace `" // "` by a list or `\n`. |
| `Axis::SetNumDivisions(int32_t)` | `Axis::SetNumDivisions(int16_t)` | signature | Pass values up to 32767; an `int` variable now narrows. |
| `PlotManager::MakeBasePlot(const std::string& name = "1d", double_t screenResolution = 100)`; Python keyword `screenResolution` | `MakeBasePlot(const std::string& name = "1d", const PanelLayout& sizes = PanelLayout(), const BasePlotShape& shape = BasePlotShape())` | signature | Drop the resolution argument; the screen size is a user setting now (`srp screenscale`, `srp bitmapscale`). |
| Base plot name `"1d_3panels"` | `"1d_3x1"`; names follow `<1d\|2d>[_wide\|_tall][_<c>x<r>][_ratio][_gap]` | removed | Use `"1d_3x1"`. |
| Python `AddDataSource(..., inputFiles=…)` / `inputFile=` / `inputData=` | One overload with keyword `inputs`; file names and ROOT objects can't be mixed in one list (`TypeError`) | signature | Python: rename the keyword to `inputs=`. |
| A string and an alias set on the same data: the string won | Setting one clears the other; a layout applied with `ApplyLayout` replaces both when it has either | behaviour | none (only code setting both is affected). |
| `Pad::AddData(const std::string& name, const Data& settings)`, `Pad::AddRatio(name, const Data& settings, name, ...)` and `Pad::AddRatio(name, dataSource, name, const Data& settings)` | — | removed | Use `AddData(settings("name"))` and `AddRatio(numSettings("name"), denSettings("name"))`. |
| Drawing option aliases `hist_no_borders`, `bar`, `hbar`, `area_curve`, `area_line`, `boxes_only`, `legoz`, `lego_no_borders`, `surf1`, `surf1z`, `surf2`, `surf2z`, `surf3`, `surf4`, `contz`, `cont1z`, `cont4z`, `candle1` … `candle7` | `hist_open`, `bars`, `hbars`, `area_smooth`, `area`, `boxes_nomarkers`, `lego_colz`, `lego_noborders`, `surf_col`, `surf_colz`, `surf_fill`, `surf_fillz`, `surf_contours`, `surf_shaded`, `cont_fillz`, `cont_colz`, `cont_smoothz`, `candle_minmax`, `candle`, `candle_mean`, `candle_notched`, `candle_points`, `candle_scatter`, `candle_meanline` | renamed | Replace the alias names (in that order, `candle1` → `candle_minmax` … `candle7` → `candle_meanline`). |
| Aliases `fit`, `stars`, `hbar_no_borders`, `hbar1` … `hbar4`, `box1`, `surf3z`, `surf7`, `surf7z` | — | removed | Use the ROOT strings, e.g. `SetDrawOption("*H")`, `"HBAR1"`, `"BOX1"`, `"SURF7Z"`; drop `fit`. |
| Alias `area` (hist `HIST F`, graph `X CF`), `text` (hist `TEXT`) | `area`: hist `HIST LF2`, graph filled down to zero; `text`: `HIST TEXT` | behaviour | none (different look). |

### Plot definitions: changed defaults and behaviour

| Old | New | Kind | Action |
|---|---|---|---|
| Without `SetDrawOption`: 1d histograms used ROOT's default (error bars incl. x bars if they have errors), graphs were drawn as `points` (base-plot default), TGraph2D/TF2 with ROOT's default | One rule: a fill set → `hist` (graphs `area`); else a marker set or errors → `points`; else `hist` (graphs `line`); 2d → `colz` | default | Graphs without y errors and without a marker style are now lines: add `.SetDrawOption(points)`. Histograms with `SetFill…` are now filled. Use `points_xerr` for x error bars. |
| `points`, `points_endcaps`, `points_line` drew the x errors of graphs and histograms | x errors only with `points_xerr` and `points_xerr_endcaps` | default | Use the `_xerr` aliases where x errors are wanted. |
| The `Z` option letter was removed for all but the first item, so graphs got end caps | `Z` removed only for 2d/3d types; graph error bars have no end caps | default | Use `points_endcaps` for end caps. |
| `band`, `band_smooth` on histograms also drew markers | No markers | default | Use `boxes` if you want markers. |
| `AddColorOverview(name, group)` without colours drew ROOT's colour wheel; cells labelled by index | The colours of the base plots; cells labelled by name and index | default | `srp colorwheel` for the wheel. |
| Base plots showed axis labels cut at the pad edge | `MakeBasePlot` sets `SetHideClippedLabels()` on X and Y | default | `plot[0]['X'].SetHideClippedLabels(false)` (same for Y) restores them. |
| Colour scale of base plots 0.005 and 0.045 of the pad width away from and wide (thicker in wide 2D panels) | 5.12 and 46.08 pixels in every panel (`PanelLayout::SetZAxisOffset`, `SetZAxisWidth`; per plot `Axis::SetColorScaleOffset`, `SetColorScaleWidth` on Z) | default | Only `2d_wide` and own layouts with wide 2D panels look different; square and tall panels are unchanged. |
| A reference function without colour (`SetRefFunc("1")`) took the first default colour, so data started at the second | Reference functions are black and solid unless set and take no default colour | default | Set data colours explicitly if you relied on the shift. |
| Candle whisker range: ROOT's default for the first plot, 0.75 for later plots in the same run | Always 1.0 unless set | default | `SetDefaultCandleWhiskerRange(0.75)` restores it. |
| Automatic binning of tree/table projections (no edges given): range `min*(1∓0.01)` to `max*(1±0.01)`; Python `histo()`/`histo2d()` used the exact min and max | Range padded by 1% of the span; integer columns get integer-wide bins centred on the integers | default | Give explicit edges to keep the old binning. |
| Line width scale for PDF, EPS, PS output: ROOT's `LineScalePS` (3) | Matched to the canvas size (about 2.9 for the default plots) | default | none (slightly different line widths in vector output). |
| Axis ranges, minimum and maximum stored with input histograms and graphs were used | Discarded, except for data marked `SetDefinesFrame()` | behaviour | Call `SetDefinesFrame()` on that data or set the ranges on the axes. |
| `Normalize()` on graphs divided by `TGraph::Integral()` (polygon area) | Divides by the sum of the y values | behaviour | Check normalized graphs. |
| `Normalize()` with `Cumulative()`: normalized after the cumulative sum | Normalized before it (a CDF ends at 1) | behaviour | Check plots using both. |
| `Ratio` modifiers (`Normalize`, `Scale`, `Rebin*`, `Smooth`, `Cumulative`, …) always acted on the ratio | After `Numer()`, `Denom()` or `Both()` they act on those inputs before the division; `Result()` (the default) acts on the ratio | behaviour | Insert `.Result()` before modifiers meant for the ratio that follow a `Numer()`/`Denom()`/`Both()`. |
| `Ratio` selections (`Project*`, `Profile*`, `Scatter`, `Define`, `Filter`, `Entries`) without `Numer()`/`Denom()`/`Both()` applied silently to the numerator | Warns, applies to the numerator and switches the mode to `Numer()` for the rest of the chain | behaviour | Write `Numer()`, `Denom()` or `Both()` before selections, `Result()` before modifiers meant for the ratio. |
| Projections with ranges in user coordinates included the next bin when a range ended exactly on a bin edge | A bin is included only if it overlaps the range | behaviour | Check projected yields. |
| A tree found in several files of a data source was read from the first file only | Chained over all files of the data source that contain it | behaviour | Restrict the data source (`file.root:folder` or fewer inputs) if one file was meant. |
| Inputs of a data source were searched in alphabetical order | Searched in the order they were added (directory contents alphabetically) | behaviour | Order the inputs deliberately or use unique object paths. |
| `GeneratePlots`, `LoadPlots`, `SavePlots` with a group matched the group exactly | Subgroups are included | behaviour | End the group pattern with `$` to exclude subgroups. |
| Legends and texts without a position were placed by ROOT (`TPad::PlaceBox`) | Placed at the best free frame corner; filled areas and error bars count as occupied | behaviour | Give a placement (`AddLegend(top_right)`) or a position where it matters. |
| Legend symbol `EP` by default (`L` for C/L/HIST options) | Derived from what is drawn (filled → `F`, markers without errors → `P`, …) | behaviour | `SetLegendEntry(LegendEntry().SetSymbolElements(...))` forces the symbol. |
| Data added to a base plot was drawn | Ignored with a warning | behaviour | Move the data into the plots. |
| A plot whose base plot is missing was drawn without it | Not generated | behaviour | Register every base plot your plots use. |
| A pad without data showed nothing ("Nothing to be drawn") | With X and Y ranges set, it shows an empty frame spanning them (a 2d frame if Z has a range too) | behaviour | none. |
| Colour gradients counted data that is not drawn | Only data that takes a default colour counts | behaviour | none (gradient steps may shift). |
| Every alias on a `TF1`, `TF2` or `TGraph2D` gave a warning and ROOT's default | Functions: `points`, `line`, `curve`, `area`, `area_smooth`; 2D functions and 2D graphs: the `col`, `lego`, `surf` and `cont` aliases; 2D graphs also `triangles`, `triangles_wire`, `triangles_col(z)`, `triangles_fill(z)`, `points`, `points_col(z)`, `line` (new aliases) | behaviour | none. |
| A `TGraph2D` drawn with `TRI1`/`TRI2` showed a large wedge, with `P`/`PCOL`/`LINE` as first data a flat colour map or nothing; its colour scale ended at the highest interpolated value | Drawn in an empty 3D frame; the z range of a `TGraph2D` includes its points (with errors), as in ROOT; `TRI`/`P`/`LINE` data after a 2D frame is not drawn (warning) | behaviour | Give a 2D graph that is drawn in 3D the first place in the pad, or a lego/surf frame before it. |
| Legend row of a filled function: `L` unless the option contained `FC` | `F` when it has a fill (as drawn), `P` for an option with markers | behaviour | `SetLegendEntry(LegendEntry().SetSymbolElements("L"))` for the old row. |
| Non-integer line widths were truncated by ROOT | Rounded, with a warning | behaviour | Pass integer widths. |
| The `PlotManager` constructor left implicit multi-threading on and `gErrorIgnoreLevel = kWarning` for the rest of the program | Set only during `GeneratePlots` and restored afterwards | behaviour | Call `ROOT::EnableImplicitMT()` yourself if your code relied on it. |
| A relative `SetOutputDirectory("out")` was resolved where `plot` runs | Made absolute against the current directory when defined | behaviour | Use absolute paths or run the program from the intended directory. |

### Input validation

| Old | New | Kind | Action |
|---|---|---|---|
| `Plot(name, …)` and `Plot(other, name, group)` accepted an empty name | Throw `std::invalid_argument` (Python `ValueError`) | input validation | Give every plot a name. |
| Group names taken as given | Leading, trailing and doubled `/` removed, with a warning | input validation | Use clean group paths. |
| `Project`/`Profile` without dimensions, tree projections with nBins ≤ 0 and no edges | Throw at definition | input validation | Fix these requests. |
| `Entries(0)`, `Entries(min, max)` with `max ≤ min` | Error, call ignored | input validation | Fix the entry ranges. |
| `AddFunction` formulas with free parameters (`gaus`, `pol2`, `[0]`) were drawn with all parameters 0 | Rejected with an error | input validation | Write the parameter values into the formula. |
| `plot += plot`, `pad += pad` | Throw | input validation | none |

### Files

| Old | New | Kind | Action |
|---|---|---|---|
| `plots.info`: aliases stored as numbers, `default_drawing_option_*`, `ENTRY_<n>` legend rows, ratio key `scale_bin_width_norm_division`, text alignment as a string | Aliases and placements by name, legend rows as repeated `ENTRY`, ratio modifiers with `numer_`/`denom_` prefixes; old keys are ignored with warnings | file format | Re-run the plotting program. |
| `plots.info` data key `dont_draw` | Pad key `FRAME` (a data entry); `dont_draw` is ignored | file format | Re-run the plotting program. |
| `plots.info` keys `drawing_options`, `drawing_option_alias`, `draw_style`, `default_draw_style`, pad key `options` | `draw_option`, `draw_option_alias`, `symbol_elements`, `default_symbol_elements`; the old keys are ignored | file format | Re-run the plotting program. |
| `dataSources.info` entries with key `FILE` | Written with key `INPUT`, inputs kept as given and in order | file format | none (re-run to rewrite; `srp stats` counts only `INPUT`). |
| `Data.root` (`data` mode): names split at `:` and `/`, old suffixes of tree requests, unsorted | Only the data path becomes directories (a `src:dir` source adds subdirectories), new request suffixes (`{dim=…@…}`), sorted | file format | Update scripts that read `Data.root` keys. |
| `Plots.root`: a `user_colors` canvas; a plot named like a subgroup clashed with it | No `user_colors` (colours are defined inside each canvas); a clashing plot is stored as `<name>_plot`; a histogram repeated in a pad is stored as `<name>_<n>` | file format | Update scripts that expect `user_colors` or the old names. |
| `UserData.root`: an in-memory `TList`/`TDirectory` added as one object | A named list or directory becomes a folder of the data source | file format | none (objects are still found by name). |

### Command line

| Old | New | Kind | Action |
|---|---|---|---|
| `srp remove`, `srp reset` only unregistered projects | Also delete each project's configuration directory (`plots.info`, `dataSources.info`; never user code or output) | CLI | Re-run the plotting program to recreate them; back them up before `srp reset` if needed. |
| `srp add <p> <prog>` on an existing project replaced its program; `srp add <p>` without a program did nothing; `srp set`/`get`/`show` on an unknown project (`set` created it) | All fail with exit code 1 | CLI | Use `srp set <p> program …` to change a program; `srp add` for new projects. |
| `srp` and `plot` returned 0 on most errors; any `srp plotmode X` was accepted | Exit code 1 on errors and unknown commands; `plotmode` is validated; `plot` rejects more than 3 arguments and invalid group regexes | CLI | Quote regex arguments (`plot g '.+'`); check scripts relying on exit codes. |
| `srp print` listed object names | Lists key names (what you pass to `AddData`) and branch types | CLI | Update scripts that parse the output. |
| `srp init-cpp`, `srp init-py` copied the full example | Copy a minimal skeleton; `srp example-cpp`, `srp example-py` copy the commented example with example data; both fail if the project exists | CLI | Use `srp example-cpp`/`example-py` for the old content. |
| `plot` ran `cmake -S .. -B .` and a build on every call; Python programs were re-run only if the main file changed | Builds only if `CMakeCache.txt` is next to the program; Python is re-run if any `.py` file (up to two levels deep) is newer | CLI | Keep the C++ program in its CMake build directory. |
| `srp edit`, `srp cd` without a project: error | Use the current project | CLI | none |
| – | `srp colorwheel [<file>]`: ROOT's colour wheel in a window, or saved to a file | CLI | none |
| – | `srp out [<project>]`: opens the output folder of the project (default: the current one) in the file manager | CLI | none |

## 3.2.4 → 3.3.3

3.2.4 is the first version distributed through conda and Homebrew.

### Build and includes

| Old | New | Kind | Action |
|---|---|---|---|
| `#include "Plot.h"`, `"PlotManager.h"`, `"Config.h"`, `"Logging.h"`, `"Helpers.h"`, `"PlotPainter.h"` | `#include "SciRooPlot/Plot.h"` etc. | build | Prefix every SciRooPlot include with `SciRooPlot/`; delete stale headers in `<prefix>/include/` of an old installation. |
| `<prefix>/share/scirooplot/scirooplot-env.sh` | `<prefix>/share/scirooplot/env.sh` | CLI | Change the `source` line in `~/.bashrc` / `~/.zshrc`. |
| CMake ≥ 3.15 | CMake ≥ 3.27 | build | Use CMake 3.27 or newer. |
| `str_contains(str, substr, bool reverseSearch)` (Helpers.h) | `str_contains(str, substr)`, `str_ends_with`, `str_starts_with` | signature | Drop the third argument; use `str_ends_with` for suffix checks. |

### Plot definitions

| Old | New | Kind | Action |
|---|---|---|---|
| `Plot::Clone() const` | — | removed | Copy instead: `Plot b = a;` (now a deep copy). |
| Copying a `Plot`/`Pad` shared its data, legends and texts with the original | Deep copy | behaviour | none, unless edits of the copy were meant to change the original. |
| Base-plot merging (`Plot::operator+=`, base plots): the plot's pad data replaced the base pad's data | Base-pad data kept, plot data appended | behaviour | Remove data from base plots or expect it to be drawn. |
| `Data::SetLegend(n)` in plots with a base plot counted the base plot's legends first | `n` counts only the plot's own legends; an invalid index drops the entry with a warning | behaviour | Re-index `SetLegend(n)` relative to the plot's own `AddLegend` calls. |
| Several `Data::SetDefinesFrame()` in a pad: the first won | The last one wins | behaviour | Keep one per pad. |
| `Data::SetScaleMinimum(f)`, `SetScaleMaximum(f)` were applied once per axis (effectively `f³` for 1d) | Applied once | behaviour | Re-tune the factors (`f³` reproduces the old result). |
| `Data::Scale(f)` with `f ≤ 0` was applied (`Scale(-1)` flipped the sign) | Ignored with a warning | behaviour | Flip signs another way. |
| `Data::NormalizeToMaximum()` ignored for graphs | Applied | behaviour | Remove it from graph data if it was there by accident. |
| `Data::Smooth()` applied to profiles | Ignored with a warning | behaviour | none |
| TH2 projections and profiles with several ranges used only the last range | Every range applies to its own axis | behaviour | Check projections with several ranges. |
| `LegendBox::GetEntry(0)` created entry 0 | Throws (entries start at 1) | input validation | Use IDs from 1. |
| Invalid axis names and indices exited the program | Throw (Python `ValueError`, `IndexError`, `RuntimeError`) | behaviour | none |
| Plot and group names with any characters (only `.` in groups and whitespace exited) | Only `A-Z a-z 0-9 _ -` (and `/` in groups); the constructor throws, `SetName`/`SetGroup`/`AppendGroup` log an error and keep the old value | input validation | Rename plots and groups, e.g. replace dots. |
| Data source names with any characters | Only `A-Z a-z 0-9 _ -`; otherwise the source is not added (also when loaded from `dataSources.info`) | input validation | Rename data sources. |
| Project and property names with any characters | Only `A-Z a-z 0-9 _ -`; otherwise nothing is stored | input validation | Rename projects with `srp rename`. |

### Files and command line

| Old | New | Kind | Action |
|---|---|---|---|
| `plots.info` keys `text_style` (boxes), `marker_width` (legend entries); numbers of drawing option aliases | `text_font`, `marker_size`; aliases after `hbar` shifted by one | file format | Re-run the plotting program. |
| `UserData.root` (objects from `AddDataSource(name, TObject*)`) appended across runs | Recreated by each run; without a project a temporary file deleted with the `PlotManager` | file format | Add all in-memory objects in every run. |
| Interactive browsing: stepping back past the first plot exited the program | `GeneratePlots` returns | behaviour | Guard code that must not run after browsing. |
| Consecutive `GeneratePlots("gif")` calls wrote into one GIF | Each call starts a new GIF | behaviour | Make a multi-frame GIF in a single call. |
| `srp rename <project> <dummy> <name>` | `srp rename <project> <name>` | CLI | Drop the extra argument. |
| `srp color <bad>`, `srp verbosity <bad>` did nothing, exit code 0 | Error, exit code 1 | CLI | Use valid values. |
| `srp unset` created missing projects | Fails for unknown projects; unsetting the last property removes the project | CLI | Use `srp set` to change a project you want to keep. |
