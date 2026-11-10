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

The data is created when a test runs, in `tests/work/` of the build directory; nothing is installed.
The `cli` tests run `srp`, `plot` and the shell functions of `env.sh` of the build directory, in bash and in zsh where it is installed;
their C++ projects need cmake and a compiler.
The `output` tests that need a display (mode `macro`, screenshots of mode `show`) use the caller's `DISPLAY` or start Xvfb themselves;
the macros are run with the `root` executable, the screenshots taken with ImageMagick's `import` (that part is left out without it).
