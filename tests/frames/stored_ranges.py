"""Ranges stored with an input (axis ranges, minimum and maximum) shape the frame only where the input defines the frame: given to
Pad::SetFrame or marked SetDefinesFrame(), for every data kind and every stored range; a range set on the pad axis wins over the stored
one. Drawn data that does not define the frame discards them silently: its frame is the one of the same data without stored ranges.
Projections and profiles do not apply stored ranges either and warn once for an axis that is integrated out (invisible in the result);
the input keeps its stored ranges for other plots.
Covers: e290323 (stored ranges kept by frames, discarded by data, projection warnings; ratio pads framed by stored dummies).
"""

import numpy as np
from srptest import DATA_1D, DATA_2D, DATA_3D, DATA_KINDS, Project, ROOT, S, base_of, cases, check, close, data_objects, describe, finish

STORED = {"x": "x axis range", "y": "minimum and maximum", "yaxis": "y axis range", "z": "minimum and maximum (2d)"}
# every kind drawn as the input object it is that can store a range: a TEfficiency stores none of its own (its histograms are not
# drawn); the projected kinds are covered by the projections below
KINDS = tuple(k for k in DATA_1D + DATA_2D + DATA_3D if not DATA_KINDS[k][2] & {"formula", "projected"} and DATA_KINDS[k][1] != "TEfficiency")
EXPLICIT = {"x": ("X", 2.0, 6.0), "y": ("Y", 0.5, 12.0), "z": ("Z", 1.0, 50.0)}  # x values: bin edges of every kind with binned axes


def explicit(case):
    """the range set on the pad axis holding the stored range (a binned y axis of 2d data: bin edges of every kind)"""
    if case["stored"] == "yaxis":
        return ("Y", 2.0, 6.0) if DATA_KINDS[case["kind"]][0] > 1 else EXPLICIT["y"]
    return EXPLICIT[case["stored"]]


def stored_kinds(kind):
    dim, _, props = DATA_KINDS[kind]
    if dim == 3:
        return ("x",) if "binned" in props else ()  # the range of a function is its own
    if dim == 2:
        return ("x", "yaxis", "z") if "binned" in props else ("z",)
    return ("x", "y", "yaxis") if "binned" in props else ("y",)


def with_stored(obj, kind, stored):
    """a copy of obj with one stored range"""
    obj = obj.Clone()
    if stored == "x":
        obj.GetXaxis().SetRangeUser(2.6, 7.4)
    elif stored == "yaxis":
        obj.GetYaxis().SetRangeUser(2.6 if kind == "hist2d" else 2.0, 7.4 if kind == "hist2d" else 8.0)
    else:
        obj.SetMinimum(2.0)
        obj.SetMaximum(8.0)
    return obj


p = Project("stored_ranges")
p.inputs()
plain = data_objects()
stored = {}
for kind in KINDS:
    for what in stored_kinds(kind):
        stored[f"{kind}_{what}"] = with_stored(plain[kind], kind, what)
p.source("stored", stored)

todo = cases(
    {"kind": list(KINDS), "stored": list(STORED), "role": ["frame", "defines", "data"], "explicit": [False, True]},
    valid=lambda c: c["stored"] in stored_kinds(c["kind"]) and not (c["explicit"] and c["role"] == "data"),
    named=[
        {"kind": "hist", "stored": "y", "role": "frame", "explicit": False, "why": "e290323"},
        {"kind": "hist", "stored": "yaxis", "role": "frame", "explicit": False, "why": "e290323 ratio pad framed by a dummy"},
        {"kind": "hist", "stored": "y", "role": "data", "explicit": False, "why": "e290323"},
        {"kind": "graph", "stored": "y", "role": "frame", "explicit": False, "why": "e290323"},
        {"kind": "hist2d", "stored": "z", "role": "frame", "explicit": False, "why": "e290323"},
    ],
)


def expected(case):
    """the stored range the frame must show: (pad axis, min, max)"""
    obj = stored[f"{case['kind']}_{case['stored']}"]
    if case["explicit"]:
        return explicit(case)
    if case["stored"] == "x" or (case["stored"] == "yaxis" and case["kind"] in DATA_2D + DATA_3D):
        axis = obj.GetXaxis() if case["stored"] == "x" else obj.GetYaxis()
        return ("X" if case["stored"] == "x" else "Y", axis.GetBinLowEdge(axis.GetFirst()), axis.GetBinUpEdge(axis.GetLast()))
    return ("Z" if case["stored"] == "z" else "Y", 2.0, 8.0)


for i, case in enumerate(todo):
    name = f"{case['kind']}_{case['stored']}"
    plot = S.Plot(f"c{i}", "stored", base_of(case["kind"]))
    if case["role"] == "frame":
        plot[1].SetFrame(name, "stored")
        plot[1].AddData(case["kind"], "inputs")
    elif case["role"] == "defines":
        plot[1].AddData(name, "stored").SetDefinesFrame()
    else:
        plot[1].AddData(name, "stored")
    if case["explicit"]:
        axis, low, high = explicit(case)
        plot[1][axis].SetRange(low, high)
    p.pm.AddPlot(plot)
for kind in KINDS:
    plot = S.Plot(f"ref_{kind}", "stored", base_of(kind))
    plot[1].AddData(kind, "inputs")
    p.pm.AddPlot(plot)

# projections: a stored y range of a 2d histogram (integrated by ProjectX, the axis kept by ProjectY) and of a THnSparse axis
p2 = ROOT.TH2D("p2", "", 10, 0.0, 10.0, 10, 0.0, 10.0)
for a in np.linspace(0.5, 9.5, 10):
    for b in np.linspace(0.5, 9.5, 10):
        p2.Fill(a, b)
p2.GetYaxis().SetRangeUser(0.0, 3.0)
nBins = np.array([10, 10, 10], dtype=np.int32)
sparse = ROOT.THnSparseD("sparse", "", 3, nBins, np.array([0.0, 0.0, 0.0]), np.array([10.0, 10.0, 10.0]))
for a in np.linspace(0.5, 9.5, 10):
    for b in np.linspace(0.5, 9.5, 10):
        sparse.Fill(np.array([a, b, b]))
sparse.GetAxis(2).SetRange(1, 3)
p.source("proj", {"p2": p2, "sparse": sparse})
PROJECTIONS = {  # plot: (definition, integral of the result, warning expected)
    "projx_integrated": (lambda pad: pad.AddData("p2", "proj").ProjectX(), 100, True),
    "projy_kept": (lambda pad: pad.AddData("p2", "proj").ProjectY(), 100, False),
    "projx_definition": (lambda pad: pad.AddData("p2", "proj").ProjectX(0.0, 5.0, True), 50, False),
    "profile_integrated": (lambda pad: pad.AddData("p2", "proj").ProfileX(), None, True),
    "sparse_integrated": (lambda pad: pad.AddData("sparse", "proj").Project([0]), 100, True),
    "sparse_definition": (lambda pad: pad.AddData("sparse", "proj").Project([0], [(2, 0.0, 10.0)], True), 100, False),
}
for name, (define, _, _) in PROJECTIONS.items():
    plot = S.Plot(name, "proj", "1d")
    define(plot[1])
    p.pm.AddPlot(plot)
plot = S.Plot("input_after", "proj", "2d")
plot[1].SetFrame("p2", "proj")
p.pm.AddPlot(plot)

run = p.plot("stored")


def ranges(name):
    """the drawn ranges of a pad: X, Y in axis values, Z the minimum and maximum of the frame; for 3D views the axis ranges of the frame"""
    frame, frameHist = run.frame(name, "stored"), run.frame_hist(name, "stored")
    if frame is None or not frameHist:
        return None
    if run.pad(name, "stored").GetView() or frameHist.GetDimension() == 3:
        axes = (frameHist.GetXaxis(), frameHist.GetYaxis(), frameHist.GetZaxis())
        return {a: (ax.GetBinLowEdge(ax.GetFirst()), ax.GetBinUpEdge(ax.GetLast())) for a, ax in zip("XYZ", axes)}
    frame["Z"] = (frameHist.GetMinimum(), frameHist.GetMaximum())
    return frame


for i, case in enumerate(todo):
    what = describe(case)
    got = ranges(f"c{i}")
    if got is None:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    if case["role"] == "data":
        reference = ranges(f"ref_{case['kind']}")
        axes = ("X", "Y") if DATA_KINDS[case["kind"]][0] == 1 else ("X", "Y", "Z")
        same = reference is not None and all(close(got[a][e], reference[a][e], 1e-6) for a in axes for e in (0, 1))
        check(f"drawn data discards stored ranges: {what}", same, f"frame {got}, without stored ranges {reference}")
    else:
        axis, low, high = expected(case)
        rule = "explicit range wins over the stored one" if case["explicit"] else "frame keeps the stored range"
        check(f"{rule}: {what}", close(got[axis][0], low, 1e-6) and close(got[axis][1], high, 1e-6), f"{axis} {got[axis][0]:.6g} to {got[axis][1]:.6g}, expected {low:.6g} to {high:.6g}")
check("stored ranges discarded silently", not run.warnings(), " | ".join(run.warnings()[:3]))
check("no errors", not run.errors(), " | ".join(run.errors()[:3]))

run = p.plot("proj")
warnings = run.warnings("is not applied to its")
for name, (_, integral, warns) in PROJECTIONS.items():
    objects = [o for o in run.objects(name, "proj") if o.InheritsFrom("TH1")]
    if not objects:
        check(f"projection made: {name}", False, " | ".join(run.errors()[:2]))
        continue
    if integral is not None:
        check(f"projection ignores the stored range: {name}", close(objects[0].Integral(), integral), f"integral {objects[0].Integral()}, expected {integral}")
    source = "sparse" if name.startswith("sparse") else "p2"
    count = sum(f"of {source}:proj" in line and f"plot '{name}'" in line for line in warnings) if warnings and "plot '" in warnings[0] else None
    if count is None:  # warnings name the data, not the plot: count them per input over the run below
        continue
    check(f"warning only for an integrated axis: {name}", count == (1 if warns else 0), " | ".join(warnings))
for source, axis, n in (("p2", "axis 1", 2), ("sparse", "axis 2", 1)):  # once per plot integrating the axis
    found = [line for line in warnings if f"of {source}:proj" in line and axis in line]
    check(f"one warning per plot integrating a stored range: {source} {axis}", len(found) == n, " | ".join(warnings))
check("no warning for an axis kept or a range from the definition", len(warnings) == 3, " | ".join(warnings))
frame = run.frame("input_after", "proj")
check("the input keeps its stored range after projections", frame is not None and close(frame["Y"][0], 0.0) and close(frame["Y"][1], 3.0), str(frame))
finish()
