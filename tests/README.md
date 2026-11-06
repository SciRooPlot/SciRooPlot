# Tests

Configure with `-DBUILD_TESTING=ON` (needs the Python bindings), build, then run from the build directory:

```
ctest --test-dir build -j4                 all tests
ctest --test-dir build -L frames           the tests of one folder
ctest --test-dir build -R user_ranges      one test
SRP_TESTS_FULL=1 ctest --test-dir build    the full case lists (as the weekly CI run)
```

A failing test prints its failed checks with `--output-on-failure`. A test file can also run on its own:
`SRP_PREFIX=build PYTHONPATH=tests python3 tests/frames/user_ranges.py`.

## Layout

One folder per feature: `data/`, `drawing/`, `frames/`, `boxes/`, `layouts/`, `colours/`, `definition/`, `inputs/`, `cli/`, `output/`.
Each `.py` file in them is a test and gets its folder as CTest label. Each file covers one guarantee (its name says which) for a whole
area: every data kind, axis kind, option … against an independent reference (plain ROOT, an invariant, the requested values).
A bug found later adds a case to the list of the file that should have caught it, named after the commit that fixed it.

`srptest.py` holds what the tests share: an isolated project per test file, the inputs of every data kind, case lists, reading the
drawn objects back, and the PASS/FAIL bookkeeping. A test exits 0 if all checks pass, 1 if one fails and 77 if it cannot run (skipped).

The data is created when a test runs, in `tests/work/` of the build directory; nothing is installed.
The `cli` tests run `srp`, `plot` and the shell functions of `env.sh` of the build directory, in bash and in zsh where it is installed;
their C++ projects need cmake and a compiler.
The `output` tests that need a display (mode `macro`, screenshots of mode `show`) use the caller's `DISPLAY` or start Xvfb themselves;
the macros are run with the `root` executable, the screenshots taken with ImageMagick's `import` (that part is left out without it).
