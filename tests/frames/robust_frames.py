"""Every data kind in every variant (values crossing zero, a NaN or infinite value, all zeros, no entries, a single bin or point, values
near the largest float, one value everywhere, a function NaN over part of its range) gives a plot with a finite frame and no ROOT errors,
on linear and log scales of its value axis: a log axis keeps a positive range (or is switched off with a warning when there is nothing
positive), the colour scale of a map has two different ends unless every content is zero. Data with a NaN or infinite value is reported
with a warning naming it, as data with contents beyond the largest float (ROOT finds axis ranges only below it) and a function NaN in
part of its range. A plot that ROOT throws an exception on while painting is reported and loses only itself (an axis too narrow for ROOT
with that reason). A TGraphMultiErrors without y error sets (ROOT crashes on it) is drawn as a TGraph, with a warning.
Covers: crashes on NaN points of TGraphMultiErrors and on empty 2d graphs, misplaced errors of
2d graphs after removing NaN points, the whole run lost to one plot ROOT could not paint, a map of one value painted in the lowest
colour (its scale had equal ends), a function NaN in part of its range drawn as an empty plot (NaN frame), a function NaN at one point
(a removable singularity) taken for one ROOT cannot draw.
"""

import math

from srptest import DATA_KINDS, ROOT, VARIANTS, Project, S, add_data, base_of, cases, check, describe, finish, formula, has_variant, source_of

p = Project("robust_frames")
p.inputs(VARIANTS)
todo = cases(
    {"kind": list(DATA_KINDS), "variant": list(VARIANTS), "log": [False, True]},
    valid=lambda c: has_variant(c["kind"], c["variant"]),
)


def value_axis(kind):
    return {1: "Y", 2: "Z", 3: "C"}[DATA_KINDS[kind][0]]


# contents beyond ROOT's float range on a log colour scale: ROOT's TGaxis throws while painting the scale (length_error)
unpaintable = ROOT.TH2D("unpaintable", "", 4, 0.0, 4.0, 4, 0.0, 4.0)
for b in range(1, 5):
    unpaintable.SetBinContent(b, b, 1e200 * b)
p.source("unpaintable", {"unpaintable": unpaintable})
plot = S.Plot("a_unpaintable", "robust", "2d")  # defined first: the plots after it must still be made
plot[1].AddData("unpaintable", "unpaintable")
plot[1]["Z"].SetLog()
p.pm.AddPlot(plot)

# a removable singularity (one point where the formula is NaN, sin(x)/x at 0, hit by a sample): ROOT draws over it
plot = S.Plot("a_removable", "robust", "1d")
plot[1].AddFunction("sin(x)/x").SetRangeX(-10.0, 10.0)
p.pm.AddPlot(plot)

# an axis spanning less than about 1e-196 (ROOT's axis throws) and a TGraphMultiErrors without y error sets (ROOT crashes making its histogram)
tiny = ROOT.TH1D("tiny", "", 10, 1e-300, 2e-300)
tiny.Fill(1.5e-300)
noErrorSets = ROOT.TGraphMultiErrors("noy", "", 4, 0)
for i in range(4):
    noErrorSets.SetPoint(i, i, 2.0 * i)
p.source("rootbugs", {"tiny": tiny, "noy": noErrorSets})
for name in ("tiny", "noy"):
    plot = S.Plot(f"a_{name}", "robust", "1d")
    plot[1].AddData(name, "rootbugs")
    p.pm.AddPlot(plot)

for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "robust", base_of(case["kind"]))
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    if case["log"]:
        plot[1][value_axis(case["kind"])].SetLog()
    p.pm.AddPlot(plot)
run = p.plot("robust")
log = run.log.splitlines()
for i, case in enumerate(todo):
    what = describe(case)
    pad = run.pad(f"c{i}", "robust")
    if pad is None:
        check(f"plot made: {what}", False, " | ".join(line for line in run.errors() if f"c{i} " in line or "Break" in line)[:300])
        continue
    ranges = [pad.GetUxmin(), pad.GetUxmax(), pad.GetUymin(), pad.GetUymax()]
    frame = run.frame_hist(f"c{i}", "robust")
    if frame and DATA_KINDS[case["kind"]][0] > 1:
        ranges += [frame.GetMinimum(), frame.GetMaximum()]
    # (a TF3 is drawn as surfaces in a 3d view: its frame has no contents to colour)
    nothingToColour = case["variant"] in ("zeros", "empty") or DATA_KINDS[case["kind"]][1] == "TF3"
    ordered = all(ranges[k] < ranges[k + 1] or (k == 4 and nothingToColour and ranges[k] == ranges[k + 1]) for k in range(0, len(ranges), 2))
    check(f"finite frame: {what}", all(math.isfinite(v) for v in ranges) and ordered, str(ranges))
    if case["log"] and frame and DATA_KINDS[case["kind"]][0] == 2:
        if pad.GetLogz():
            check(f"positive range on the log scale: {what}", 0 < frame.GetMinimum() < frame.GetMaximum(), f"{frame.GetMinimum()} to {frame.GetMaximum()}")
        else:
            ignored = [line for line in log if "Log scale of Z axis in pad 1 ignored" in line]
            check(f"log scale without positive values ignored with a warning: {what}", bool(ignored), "log scale off without warning")
unpaintableErrors = [line for line in run.errors() if "a_unpaintable" in line]
check("a plot ROOT cannot paint is made or reported", bool(run.pad("a_unpaintable", "robust")) or any("could not be painted" in line for line in unpaintableErrors), str(unpaintableErrors))
check("contents beyond the largest float are reported by name", any(line.split("]", 1)[-1].split()[0] == "unpaintable:unpaintable" and "beyond the largest float" in line for line in run.warnings()), " | ".join(run.warnings()[:3]))
tinyErrors = [line for line in run.errors() if "a_tiny" in line]
check("an axis too narrow for ROOT: the plot is lost with the reason", not run.pad("a_tiny", "robust") and any("ROOT cannot draw one of its axes" in line for line in tinyErrors), str(tinyErrors))
noErrorSetsDrawn = run.objects("a_noy", "robust")
check("a TGraphMultiErrors without y error sets is drawn as a TGraph", [o.ClassName() for o in noErrorSetsDrawn] == ["TGraph"], str([o.ClassName() for o in noErrorSetsDrawn]))
check("a TGraphMultiErrors without y error sets is reported", bool(run.warnings("rootbugs:noy is a TGraphMultiErrors without y errors")), " | ".join(run.warnings()[:3]))
errors = [line for line in run.errors() if "a_unpaintable" not in line and "a_tiny" not in line]
check("no ROOT errors", not errors, " | ".join(errors[:5]))
undefined = sorted({case["kind"] for case in todo if case["variant"] == "undefined"})
unreportedUndefined = [kind for kind in undefined if not any("is NaN in parts of" in line and (f"Function {kind}:" in line or formula(kind, "undefined") in line) for line in run.warnings())]
check("a function NaN in part of its range is reported by name", not unreportedUndefined, ", ".join(unreportedUndefined))
removable = run.pad("a_removable", "robust")
check(
    "a function NaN at one point only is drawn over its whole range without a warning",
    removable and math.isclose(removable.GetUxmin(), -10.0) and math.isclose(removable.GetUxmax(), 10.0) and not any("sin(x)/x" in line for line in run.warnings()),
    " | ".join(line for line in run.warnings() if "sin(x)/x" in line),
)
kinds = {case["kind"] for case in todo if case["variant"] in ("nan", "inf")}
unreported = sorted(kind for kind in kinds if not any(line.split("]", 1)[-1].split()[0].split(":")[0] == kind and "NaN" in line for line in run.warnings()))
check("data with a NaN or infinite value is reported by name", not unreported, ", ".join(unreported))
finish()
