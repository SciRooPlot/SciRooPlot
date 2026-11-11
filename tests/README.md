# Tests

Configure with `-DBUILD_TESTING=ON` (needs the Python bindings), build, then run from the build directory:

```
ctest --test-dir build -j4                 all tests
ctest --test-dir build -L frames           the tests of one folder
ctest --test-dir build -R user_ranges      one test
SRP_TESTS_FULL=1 ctest --test-dir build    the full case lists (as the weekly CI run)
```

The long tests of `extended/` are registered with `-DSCIROOPLOT_EXTENDED_TESTS=ON` and run with `ctest --test-dir build -L extended`
(the weekly CI runs them with the full lists, and with the sanitizer build).

A failing test prints its failed checks with `--output-on-failure`. A test file can also run on its own:
`SRP_PREFIX=build PYTHONPATH=tests python3 tests/frames/user_ranges.py`.

## Layout

One folder per feature: `data/`, `drawing/`, `frames/`, `boxes/`, `layouts/`, `colours/`, `definition/`, `inputs/`, `cli/`, `output/`.
Each `.py` file in them is a test and gets its folder as CTest label. Each file covers one guarantee (its name says which) for a whole
area: every data kind, axis kind, option … against an independent reference (plain ROOT, an invariant, the requested values).
A bug found later adds a case to the list of the file that should have caught it, with a short text saying what it checks.

`srptest.py` holds what the tests share: an isolated project per test file, the inputs of every data kind, case lists, reading the
drawn objects back, and the PASS/FAIL bookkeeping. A test exits 0 if all checks pass, 1 if one fails and 77 if it cannot run (skipped).

`extended/` holds tests too long for every change: random plot definitions over the whole interface in C++ and Python and every output
mode (`random_projects.py`), random nests of containers with colliding names against the search rule (`random_containers.py`), plots
made again and again in one process without leaving ROOT objects behind (`repeated_runs.py`).

`tools/` is never run by CTest: what two builds draw compared pixel by pixel (`compare/outputs.py`), how fast they plot
(`compare/speed.py`), the memory of one process plotting again and again (`compare/memory.py`), how often a plot run calls a function
(`compare/calls.py`, with gdb), the windows of modes show and spread driven on a virtual display (`interactive/windows.py`), and raw ROOT
options as SciRooPlot draws them next to plain ROOT (`raw_options.py`). Each says in its docstring how to run it.

## Writing a test

- A test is mostly a case list (`cases(dims, valid, named, always)`) and a check against a reference that does not come from the code
  under test. Take the data kinds and variants from `DATA_KINDS` and `VARIANTS`, so that a kind added there is covered everywhere.
- The fast run takes every value of every dimension once and some pairs of values, plus the `always` and named cases; `SRP_TESTS_FULL=1`
  takes the whole list. A list of all dimensions crossed can be far too large: vary each dimension on its own around plain values
  (`valid`), or take each pair of kinds once. A combination that matters but the fast run may not pick belongs in `named`.
- One project and one `plot` run per file where possible (a run costs seconds, hundreds of plots in it little more). An exception while
  painting loses only that plot, a crash the rest of the run. Thousands of plots: `Chunks` spreads them over runs of 300.
- A test reading or painting thousands of canvases in one process runs out of memory (ROOT and its interpreter keep some for every
  canvas read and every `TExec` run): wrap the reading in `Run.held()`, paint in forked chunks (`in_chunks`).
- Plain ROOT as reference: draw a clone of the drawn object alone on a `TCanvas` with the same log flags and `Print` it (`TCanvas::Update`
  crashes in batch PyROOT); reset what ROOT sets when painting (PLC/PMC/PFC) on the clone first.
- Drawn objects are not the input: flows are cleared (unless `SetShowOverflowBins`), graphs drawn as points lose their x errors,
  points that are not finite and empty graphs are not drawn. Each `Run.canvas`/`pad`/`drawn` call reads a new copy from the file.
- Colours are compared by rgb and alpha (`rgba`), not by index: ROOT gives an rgb colour the index of an existing colour close to it.
- Messages: warnings name the data, not the plot. Take the messages of one plot with `Run.messages(name, group)`, or count them per data
  name as a whole word. Messages of the definition come from the C++ logger and Python out of order: match them by content. `plot`
  loads the whole `plots.info` before it says which plots it makes: load a saved plot alone to get its loading messages.
- `AddPlot` stores a copy: set up the data before adding the plot. Objects handed over in memory are found by their own name (a
  nameless object is skipped), objects in a file by their key. Plot names cannot contain `/`.
- A program run in a process of its own must not import `srptest` (importing it empties the work folder of the test): write it as plain
  SciRooPlot code. Histograms it keeps after closing their file need `TH1::AddDirectory(false)`.
- Read back from a multi-panel plot: a pad that draws nothing is not in the canvas (give every checked pad data or a text that fits
  inside it).
- A finding is fixed, or the guarantee is narrowed in the docstring with the reason: no known-failure marks. Named cases, check titles
  and the "Covers:" line say in words what they check. A new or rewritten check must fail on the code without the fix it covers.
- To find the plot behind a ROOT error: run `<build>/bin/plot <group> '<name regex>' file` with the configuration of the test
  (`tests/work/<folder>.<test>/config` of the build directory as `SCIROOPLOT_CONFIG_PATH`) on halves of the names, then the plot alone under
  `gdb -batch -ex "break Error" -ex run -ex bt --args …`.

The data is created when a test runs, in `tests/work/` of the build directory; nothing is installed.
The `cli` tests run `srp`, `plot` and the shell functions of `env.sh` of the build directory, in bash and in zsh where it is installed;
their C++ projects need cmake and a compiler.
The `output` tests that need a display (mode `macro`, screenshots of mode `show`) use the caller's `DISPLAY` or start Xvfb themselves;
the macros are run with the `root` executable, the screenshots taken with ImageMagick's `import` (that part is left out without it).
