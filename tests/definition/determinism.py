"""The same definition gives the same plots: nothing depends on the order of hashing, on the run or on the other plots of a run.

The plots: every data kind in the variants normal and negative in its base plot, each with an rgb colour, a legend and a text placed
automatically, half of them with a gradient of default colours; a ratio, a request of a tree read with weights and a filter, a 1d grid of
four pads sharing their axes. References:
- the definition program run in two processes of their own (other seeds of Python's hashing) saves the same file, byte for byte, as the
  test process does;
- a plot run twice draws every plot the same: the same primitives in the same order, with the same values, positions, sizes, styles,
  colours (as rgb and alpha) and pad ranges, the same palette;
- a plot drawn among only half of the plots (every other one) is drawn as among all of them.
Covers: the same file from processes with other hash seeds, a plot drawn among half of the plots.
"""

import os
import re

import numpy as np

import srptest as S

p = S.Project("determinism", bases=())
p.inputs(("normal", "negative"))
rows = 200
S.write_tree(p.path("trees.root"), {"x": np.linspace(0.5, 9.5, rows), "y": np.linspace(0.0, 4.0, rows), "w": np.linspace(1.0, 2.0, rows)})
inputs = {"inputs": p.path("inputs.root"), "inputs_negative": p.path("inputs_negative.root"), "trees": p.path("trees.root")}

# the definition as a program of its own (SciRooPlot only)
lines = [f"pm.AddBasePlot(PlotManager.MakeBasePlot({base!r}))" for base in ("1d", "2d", "1d_ratio", "1d_2x2")]
names = []
for kind in S.DATA_KINDS:
    for variant in ("normal", "negative"):
        if not S.has_variant(kind, variant):
            continue
        name = f"p{len(names):03d}"
        names.append(name)
        source = S.source_of(variant)
        if kind in S.FORMULAS or kind == "function2d":
            add = f"AddFunction({S.formula(kind, variant)!r}, 'f').SetRangeX(*{S.axis_range(variant)!r})" + (f".SetRangeY(*{S.axis_range(variant)!r})" if kind == "function2d" else "")
        else:
            add = f"AddData({kind!r}, {source!r}, 'data')" + (f".Project({[0] if S.DATA_KINDS[kind][0] == 1 else [0, 1]!r})" if "projected" in S.DATA_KINDS[kind][2] else "")
        lines += [
            f"p = Plot({name!r}, 'g', {S.base_of(kind)!r})",
            f"p[1].{add}.SetColor((0.2, 0.45, 0.7))",
            f"p[1].AddData('hist', {source!r}, 'second')" if S.DATA_KINDS[kind][0] == 1 else "",
            "p[1].AddLegend()",
            "p[1].AddText('<name> <entries>')",
            "p[0].SetDefaultColors([((0.9, 0.1, 0.1), 0.), ((0.1, 0.1, 0.9), 1.)])" if len(names) % 2 else "",
            "pm.AddPlot(p)",
        ]
extras = {
    "ratio": "p = Plot('{name}', 'g', '1d_ratio')\np[1].AddData('hist', 'inputs')\np[1].AddData('hist_var', 'inputs')\np[2].AddRatio('hist', 'inputs', 'hist_var', 'inputs')\np[1].AddLegend()\npm.AddPlot(p)",
    "request": "p = Plot('{name}', 'g', '1d')\np[1].AddData('t', 'trees').Project1D(('x', 20, (0., 10.)), 'w').Filter('y > 1')\np[1].AddData('t', 'trees').Profile1D(('x', 10), 'y')\npm.AddPlot(p)",
    "grid": "p = Plot('{name}', 'g', '1d_2x2')\n" + "".join(f"p[{i}].AddData('{kind}', 'inputs')\n" for i, kind in enumerate(("hist", "graph_errors", "profile", "tf1"), 1)) + "pm.AddPlot(p)",
}
for what, code in extras.items():
    name = f"p{len(names):03d}"
    names.append(name)
    lines.append(code.format(name=name))
definition = "\n".join(line for line in lines if line)
sources = "".join(f"pm.AddDataSource({source!r}, {path!r})\n" for source, path in inputs.items())

namespace = dict(vars(S.S), pm=p.pm)
exec(sources.replace("pm.AddDataSource", "pm.AddDataSource") + definition, namespace)  # noqa: S102  the plots of the test
own = os.path.join(S.WORK, "own.info")
p.pm.SavePlots(file=own)
with open(own, "rb") as f:
    reference = f.read()
for seed in ("1", "2"):
    out = os.path.join(S.WORK, f"seed{seed}.info")
    program = f"from SciRooPlot import *\npm = PlotManager('determinism_{seed}')\n{sources}{definition}\npm.SavePlots(file={out!r})\n"
    result = S.python(program, env={"PYTHONHASHSEED": seed})
    saved = open(out, "rb").read() if os.path.exists(out) else b""
    S.check(f"the definition run in a process of its own (hash seed {seed}) saves the same file", result.rc == 0 and saved == reference, result.tail() if result.rc else "the files differ")


first = p.plot("g")
S.check("the plots are made without errors", not first.errors(), " | ".join(first.errors()[:3]))
reference = {name: S.fingerprint(first, name, "g") for name in names}
S.check("every plot is made", all(reference.values()), str([n for n, f in reference.items() if not f]))
again = p.plot("g")
for name in names:
    S.check(f"plot {name} is drawn the same by a second run", not S.fingerprint_diff(reference[name], S.fingerprint(again, name, "g")), S.fingerprint_diff(reference[name], S.fingerprint(again, name, "g")))
for half, pattern in (("odd", r"p\d\d[13579]"), ("even", r"p\d\d[02468]")):
    run = p.plot("g", pattern)
    for name in names:
        if re.fullmatch(pattern, name):
            diff = S.fingerprint_diff(reference[name], S.fingerprint(run, name, "g"))
            S.check(f"plot {name} drawn among the {half} plots only is drawn as among all", not diff, diff)
S.finish()
