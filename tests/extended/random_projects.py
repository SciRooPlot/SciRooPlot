"""Any combination of the interface makes its plots, written in C++ as in Python: random plot definitions (every shipped base plot, data of
every kind and variant in its panels, ratios in ratio and pull panels, tree requests, legends, texts and shapes, and the setters of every
object with their test values, chained at random: srptest.random_definition).
- the C++ program compiles and saves the same definitions as the Python program;
- in every output mode (file, data, pdf, png, svg; macro with a display) every plot is made, the plot program ends normally and reports
  no error of SciRooPlot or ROOT, no crash and nothing of a sanitizer (the weekly sanitizer job runs this with the sanitizer build).
Warnings are allowed: random combinations warn about what does not fit (a range of the wrong axis, a modifier for another kind of data).
"""

import os
import shutil

import srptest as S
from srptest import check, finish

seeds, nPlots = ((1, 2, 3, 4, 5, 6), 60) if S.FULL else ((1,), 40)
SANITIZERS = ("AddressSanitizer", "LeakSanitizer", "runtime error:")
modes = ["file", "data", "pdf", "png", "svg"] + (["macro"] if S.display() else [])
if "macro" not in modes:
    print("no display: mode macro is left out")
compiler = shutil.which("cmake") and (shutil.which("c++") or shutil.which("g++") or shutil.which("clang++"))
if not compiler:
    print("no cmake or C++ compiler: the C++ programs are left out")

folder = os.path.join(S.WORK, "inputs")
os.makedirs(folder)
sources = S.random_inputs(folder)
for seed in seeds:
    project = f"random{seed}"
    py, cpp = S.random_definition(seed, nPlots, sources, project)
    work = os.path.join(S.WORK, project)
    os.makedirs(work)
    program = os.path.join(work, "DefinePlots.py")
    with open(program, "w") as f:
        f.write(py)
    out = S.python(py, cwd=work)
    check(f"seed {seed}: the Python program runs", out.rc == 0, out.tail(1500))
    info = os.path.join(S.WORK, "config", project, "plots.info")
    if compiler:
        built, build, _ = S.cpp_program(os.path.join(work, "cpp"), cpp.replace(f'PlotManager pm("{project}")', f'PlotManager pm("{project}_cpp")'))
        check(f"seed {seed}: the C++ program compiles", built is not None, build.tail(1500))
        if built:
            ran = S.run(built, cwd=work)
            check(f"seed {seed}: the C++ program runs", ran.rc == 0, ran.tail(1500))
            cppInfo = os.path.join(S.WORK, "config", f"{project}_cpp", "plots.info")
            a, b = (S.info_plots(path) if os.path.exists(path) else {} for path in (info, cppInfo))
            differ = []
            for plot in sorted(set(a) | set(b)):
                x, y = a.get(plot) or {}, b.get(plot) or {}
                differ += [f"{plot} {key}: {x.get(key)} (Python) | {y.get(key)} (C++)" for key in sorted(set(x) | set(y)) if x.get(key) != y.get(key)]
            check(f"seed {seed}: C++ saves the definitions Python saves", a and not differ, "; ".join(differ[:3])[:1500])
    S.run(S.SRP, "add", project, program, os.path.join(work, "out"))
    S.run(S.SRP, "select", project)
    for mode in modes:
        ran = S.run(S.PLOT, ".+", ".+", mode, timeout=3600)
        made = len([line for line in ran.text.splitlines() if "Created plot" in line])
        check(f"seed {seed}, mode {mode}: every plot made", made == nPlots, f"{made} of {nPlots}; {' | '.join(ran.errors()[:3])}")
        errors = [line for line in ran.text.splitlines() if line.startswith("[ ERR") or "Error in <" in line or "Fatal in <" in line or "*** Break" in line or any(s in line for s in SANITIZERS)]
        check(f"seed {seed}, mode {mode}: the plot program ends normally without errors", ran.rc == 0 and not errors, f"exit status {ran.rc}: {' | '.join(errors[:5])}"[:1500])
finish()
