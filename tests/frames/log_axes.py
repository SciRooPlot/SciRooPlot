"""A log axis shows the data the way ROOT shows it, for every data kind, on every log axis it has, with values crossing zero, a single
value and coordinates at and below zero (variant crossing), without a range and with ranges inside, beyond and below the data:
- an automatic frame is ROOT's frame of the same data drawn alone, except where ROOT cannot show the data: bins (of a histogram, or of
  the frame histogram of 2d data) below zero are left out from the first bin edge above zero on (ROOT starts at 0.001 x the maximum and
  draws the bins with negative edges left of the frame), a graph never hides a positive point (where ROOT starts a log x axis at
  0.001 x the maximum, it starts at 0.9 x the smallest positive point), and a graph with values at or below zero on log y starts at half
  its smallest positive value inside the frame (ROOT: 0.001 x the maximum)
- the y range of a histogram comes from the bins shown in the frame, as ROOT does for the same bins drawn alone, that of a graph with
  an x range from the points drawn into it (their x error bars reaching into it, on log x above zero), as ROOT does for those points
  alone; that of a function contains every value shown, at most a decade beyond it (on log y only its maximum where it crosses zero);
  a log z axis colours every positive value shown, or starts at ROOT's floor min(1, 0.001 x maximum) where there are values at or below
  zero (2d functions as drawn, sampled as the frame), and is off with a warning where 2d data shows no positive value
- a positive lower range below the data widens the frame to exactly that value, also below bins that end at or cross zero; a lower range
  at or below zero is reported and ignored: the frame is the one with only the upper end of the range set
Covers: 6251b3b (lowest bins of log z not drawn), 57036b4 (log y minimum from bins and points outside the frame), d89603e (graph on log x
lost its small-x points), d15c937 (widening below the first positive bin edge), f934095 (log x frame below zero ends at the last bin),
f49b65a (lower range at or below zero warns), the y range of a graph from the points shown, the z range of a 2d graph read before it is
deleted, the positive TH2Poly bins of a widened frame, a function drawn with the samples of the frame.
"""

import math

from srptest import DATA_KINDS, ROOT, Project, S, add_data, axis_range, base_of, cases, check, close, describe, finish, has_variant, source_of

KINDS = tuple(k for k, (_, _, props) in DATA_KINDS.items() if "polar" not in props)
VARIANTS = ("normal", "negative", "single", "crossing")
LOGS = {1: ("X", "Y", "XY"), 2: ("X", "Y", "Z", "XYZ"), 3: ("X", "Y", "Z")}
# ranges of the X axis, from the range (lo, hi) the data covers
RANGES = {
    "none": None,
    "inside": lambda lo, hi: (lo + 1.3, lo + 6.7),
    "wide": lambda lo, hi: (1e-4, hi + 30.0),  # positive, below all data, and beyond it
    "nonpositive_inside": lambda lo, hi: (lo - 6.0, lo + 6.7),
    "nonpositive_end": lambda lo, hi: (lo - 6.0, hi),
    "nonpositive_wide": lambda lo, hi: (lo - 6.0, hi + 30.0),
    "zero": lambda lo, hi: (0.0, lo + 6.7),
}
IGNORED_MIN = ("nonpositive_inside", "nonpositive_end", "nonpositive_wide", "zero")  # lower ends a log x axis cannot show


def dim(kind):
    return DATA_KINDS[kind][0]


def valid(c):
    if not has_variant(c["kind"], c["variant"]) or c["log"] not in LOGS[dim(c["kind"])]:
        return False
    if dim(c["kind"]) == 3:  # 3d views: the log flags only
        return c["range"] == "none"
    return c["range"] not in IGNORED_MIN or "X" in c["log"]


p = Project("log_axes")
p.inputs(VARIANTS)
# Mario's case of 6251b3b: rows of ones next to a maximum of 2e4 on log z
ones = ROOT.TH2D("ones", "", 20, 0.0, 1.0, 20, 0.0, 1.0)
for i in range(1, 21):
    ones.SetBinContent(i, 10, 2e4 * i / 20)
    ones.SetBinContent(i, 3, 1.0)
    ones.SetBinContent(i, 5, 2.0)
p.source("ones", {"ones": ones})
plot = S.Plot("ones", "log", "2d")
plot[1].AddData("ones", "ones")
plot[1]["Z"].SetLog()
p.pm.AddPlot(plot)

todo = cases(
    {"kind": list(KINDS), "variant": list(VARIANTS), "log": ["X", "Y", "Z", "XY", "XYZ"], "range": list(RANGES)},
    valid=valid,
    named=[
        {"kind": "hist", "variant": "normal", "log": "Y", "range": "inside", "why": "57036b4"},
        {"kind": "graph", "variant": "negative", "log": "Y", "range": "inside", "why": "57036b4"},
        {"kind": "graph_errors", "variant": "crossing", "log": "X", "range": "none", "why": "d89603e"},
        {"kind": "hist_var", "variant": "crossing", "log": "X", "range": "wide", "why": "d15c937"},
        {"kind": "hist", "variant": "crossing", "log": "X", "range": "wide", "why": "d15c937"},
        {"kind": "hist", "variant": "normal", "log": "X", "range": "nonpositive_end", "why": "f934095"},
        {"kind": "hist", "variant": "crossing", "log": "X", "range": "zero", "why": "f49b65a"},
        {"kind": "graph_multi", "variant": "normal", "log": "X", "range": "inside", "why": "y range of a graph from the points shown"},
        {"kind": "graph_asymm", "variant": "crossing", "log": "XY", "range": "inside", "why": "y range of a graph from the points shown"},
        {"kind": "graph2d_errors", "variant": "normal", "log": "Z", "range": "wide", "why": "z range of a 2d graph read before it is deleted"},
        {"kind": "hist2poly", "variant": "single", "log": "Z", "range": "wide", "why": "positive TH2Poly bins of a widened frame"},
        {"kind": "hist2poly", "variant": "single", "log": "Z", "range": "inside", "why": "positive TH2Poly bins of a widened frame"},
        {"kind": "tf2", "variant": "crossing", "log": "XYZ", "range": "none", "why": "function drawn with the samples of the frame"},
        {"kind": "function2d", "variant": "normal", "log": "Z", "range": "inside", "why": "function drawn with the samples of the frame"},
    ],
    always=lambda c: c["range"] == "none" or (c["range"] in IGNORED_MIN and c["variant"] == "crossing" and c["log"] == "X"),
)


def define(name, case, rangeX):
    plot = S.Plot(name, "log", base_of(case["kind"]))
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    for axis in case["log"]:
        plot[1][axis].SetLog()
    if rangeX:
        low, high = rangeX
        (plot[1]["X"].SetRange(low, high) if low is not None else plot[1]["X"].SetMaxRange(high))
    p.pm.AddPlot(plot)


for i, case in enumerate(todo):
    rangeX = RANGES[case["range"]](*axis_range(case["variant"])) if RANGES[case["range"]] else None
    define(f"c{i}", case, rangeX)
    if case["range"] in IGNORED_MIN:  # the reference: only the upper end
        define(f"c{i}_max", case, (None, rangeX[1]))
run = p.plot("log")

canvas = ROOT.TCanvas("reference", "", 800, 600)


def root_frame(obj, opt, logs, shown=None):
    """the frame ROOT draws for obj alone with option opt on the log axes (a histogram with only the bins inside shown, a pair of x values)"""
    canvas.Clear()
    canvas.SetLogx("X" in logs)
    canvas.SetLogy("Y" in logs and dim_of(obj) == 1)
    canvas.SetLogz("Z" in logs)
    ref = obj.Clone(f"{obj.GetName()}_ref")
    if ref.InheritsFrom("TH1"):
        axis = ref.GetXaxis()
        if shown:
            eps = 1e-9 * (axis.GetXmax() - axis.GetXmin())
            first = max(1, axis.FindFixBin(max(shown[0], axis.GetXmin()) + eps))
            last = min(axis.GetNbins(), axis.FindFixBin(min(shown[1], axis.GetXmax()) - eps))
            axis.SetRange(first, last)
        else:
            axis.SetRange()
    elif ref.InheritsFrom("TGraph"):
        opt = "A " + opt
    ref.Draw(opt)
    canvas.Print(p.path("reference.svg"))
    x = (canvas.GetUxmin(), canvas.GetUxmax())
    y = (canvas.GetUymin(), canvas.GetUymax())
    return {"X": tuple(10**v for v in x) if canvas.GetLogx() else x, "Y": tuple(10**v for v in y) if canvas.GetLogy() else y}


def dim_of(obj):
    if obj.InheritsFrom("TH3") or obj.InheritsFrom("TF3"):
        return 3
    if obj.InheritsFrom("TH2") or obj.InheritsFrom("TGraph2D") or obj.InheritsFrom("TF2"):
        return 2
    return 1


def shown_values(obj, frame):
    """the values of obj shown in the frame (central values; histograms: the bins with their centre inside, polygons reaching into it)"""
    (x0, x1), (y0, y1) = frame["X"], frame["Y"]

    def inX(x):
        return x0 <= x <= x1

    def inY(y):
        return y0 <= y <= y1

    if obj.InheritsFrom("TH2Poly"):
        return [b.GetContent() for b in obj.GetBins() if b.GetXMax() >= x0 and b.GetXMin() <= x1 and b.GetYMax() >= y0 and b.GetYMin() <= y1]
    if obj.InheritsFrom("TH2"):
        ax, ay = obj.GetXaxis(), obj.GetYaxis()
        return [obj.GetBinContent(i, j) for i in range(1, ax.GetNbins() + 1) for j in range(1, ay.GetNbins() + 1) if inX(ax.GetBinCenter(i)) and inY(ay.GetBinCenter(j))]
    if obj.InheritsFrom("TH1"):
        ax = obj.GetXaxis()
        return [obj.GetBinContent(i) for i in range(1, ax.GetNbins() + 1) if inX(ax.GetBinCenter(i))]
    if obj.InheritsFrom("TGraph2D"):
        return [obj.GetZ()[i] for i in range(obj.GetN()) if inX(obj.GetX()[i]) and inY(obj.GetY()[i])]
    if obj.InheritsFrom("TGraph"):
        return [obj.GetPointY(i) for i in shown_points(obj, frame["X"])]
    n = 60 if obj.InheritsFrom("TF2") else 200
    xs = [max(x0, obj.GetXmin()) + (min(x1, obj.GetXmax()) - max(x0, obj.GetXmin())) * (k + 0.5) / n for k in range(n)]
    if obj.InheritsFrom("TF2"):
        ys = [max(y0, obj.GetYmin()) + (min(y1, obj.GetYmax()) - max(y0, obj.GetYmin())) * (k + 0.5) / n for k in range(n)]
        return [obj.Eval(x, y) for x in xs for y in ys]
    return [obj.Eval(x) for x in xs]


def shown_points(graph, rangeX, logX=False):
    """the points of a graph drawn into an x range: those whose x error bar reaches into it (on a log axis above zero)"""
    shown = []
    for i in range(graph.GetN()):
        low, high = graph.GetPointX(i) - max(graph.GetErrorXlow(i), 0.0), graph.GetPointX(i) + max(graph.GetErrorXhigh(i), 0.0)
        if high >= rangeX[0] and low <= rangeX[1] and not (logX and high <= 0):
            shown.append(i)
    return shown


def points_alone(graph, keep):
    """a copy of a graph with only the points keep (a TGraphMultiErrors made anew: ROOT's RemovePoint leaves its y errors in place)"""
    if graph.InheritsFrom("TGraphMultiErrors"):
        copy = ROOT.TGraphMultiErrors(len(keep), graph.GetNYErrors())
        for n, i in enumerate(keep):
            copy.SetPoint(n, graph.GetPointX(i), graph.GetPointY(i))
            copy.SetPointEX(n, graph.GetErrorXlow(i), graph.GetErrorXhigh(i))
            for e in range(graph.GetNYErrors()):
                copy.SetPointEY(n, e, graph.GetEYlow(e)[i], graph.GetEYhigh(e)[i])
        return copy
    copy = graph.Clone(f"{graph.GetName()}_shown")
    for i in reversed(range(graph.GetN())):
        if i not in keep:
            copy.RemovePoint(i)
    return copy


def positive_x_low(graph):
    """the smallest positive lower end of the points of a graph: x minus its error, or x where that is not positive"""
    lows = []
    for i in range(graph.GetN()):
        x = graph.GetPointX(i)
        low = x - max(graph.GetErrorXlow(i), 0.0)
        if low > 0:
            lows.append(low)
        elif x > 0:
            lows.append(x)
    return min(lows) if lows else None


def first_positive_edge(axis):
    """the first bin edge above zero of an axis whose bins start below zero, else None"""
    if axis.GetXmin() >= 0:
        return None
    return next((axis.GetBinLowEdge(b) for b in range(1, axis.GetNbins() + 1) if axis.GetBinLowEdge(b) > 0), None)


def same(a, b, rel=1e-6):
    return close(a[0], b[0], rel) and close(a[1], b[1], rel)


def fmt(r):
    return f"{r[0]:.6g} to {r[1]:.6g}"


nIgnored = 0
for i, case in enumerate(todo):
    what = describe(case)
    pad = run.pad(f"c{i}", "log")
    if pad is None:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    logs = case["log"]
    flags = {"X": pad.GetLogx(), "Y": pad.GetLogy(), "Z": pad.GetLogz()}
    if dim(case["kind"]) == 3:
        check(f"log scale kept: {what}", all(flags[a] for a in logs), str(flags))
        continue
    frame = run.frame(f"c{i}", "log")
    obj, opt = run.drawn(f"c{i}", "log")[0]
    lo, hi = axis_range(case["variant"])
    rangeX = RANGES[case["range"]](lo, hi) if RANGES[case["range"]] else None
    # the values shown (2d functions: coloured cell by cell from ROOT's samples)
    values = [v for v in shown_values(obj.GetHistogram() if obj.InheritsFrom("TF2") else obj, frame) if math.isfinite(v)]
    positive = [v for v in values if v > 0]
    if dim_of(obj) == 2 and "Z" in logs and not positive:  # nothing to show on log z: off, with a warning
        warned = any("Log scale of Z axis in pad 1 ignored: the data has no positive values." in m for m in run.messages(f"c{i}", "log"))
        check(f"log z of 2d data without positive values shown is off with a warning: {what}", not flags["Z"] and warned, f"log z {flags['Z']}, warned {warned}")
        logs = logs.replace("Z", "")
    check(f"log scale kept: {what}", all(flags[a] for a in logs), str(flags))
    for a in logs:
        if a in frame:
            check(f"positive {a} range on the log axis: {what}", 0 < frame[a][0] < frame[a][1], fmt(frame[a]))

    # the x axis (and y of 2d data)
    if rangeX and rangeX[0] <= 0 and "X" in logs:  # (also a range inside data crossing zero)
        nIgnored += 1
    if case["range"] in IGNORED_MIN:
        reference = run.frame(f"c{i}_max", "log")
        check(
            f"a lower range at or below zero on log x is ignored: {what}",
            reference is not None and same(frame["X"], reference["X"]) and same(frame["Y"], reference["Y"]),
            f"x {fmt(frame['X'])}, y {fmt(frame['Y'])}; with only the upper end: x {fmt(reference['X'])}, y {fmt(reference['Y'])}" if reference else "no reference",
        )
    elif case["range"] == "wide" and "X" in logs:
        check(f"a positive lower range below the data widens log x to it: {what}", same(frame["X"], rangeX), fmt(frame["X"]))
    elif case["range"] == "none":
        ref = root_frame(obj, opt, logs)
        fh = run.frame_hist(f"c{i}", "log")
        for a in ("X", "Y") if dim_of(obj) == 2 else ("X",):
            want, rule = ref[a], "as ROOT draws it"
            if a in logs and obj.InheritsFrom("TGraph"):
                if close(ref[a][0], 0.001 * ref[a][1], 1e-6) and positive_x_low(obj):  # ROOT's fallback hides the small points
                    want, rule = (0.9 * positive_x_low(obj), ref[a][1]), "from 0.9 x the smallest positive point"
            elif a in logs:
                edge = first_positive_edge(fh.GetXaxis() if a == "X" else fh.GetYaxis())
                if edge:
                    want, rule = (edge, frame[a][1]), "from the first bin edge above zero"
            check(f"{a} frame {rule}: {what}", same(frame[a], want), f"{fmt(frame[a])}, ROOT {fmt(ref[a])}")

    # the value axis
    if dim_of(obj) == 1:
        y = frame["Y"]
        if obj.InheritsFrom("TH1"):
            ref = root_frame(obj, opt, logs, frame["X"])
            check(f"y range from the bins shown, as ROOT draws them: {what}", same(y, ref["Y"]), f"{fmt(y)}, ROOT {fmt(ref['Y'])}")
        elif obj.InheritsFrom("TGraph"):
            # the y frame of the points drawn into the x range, as ROOT draws them alone (without a range all points, as ROOT draws the
            # graph); where ROOT starts log y at 0.001 x the maximum (values at or below zero), half the smallest positive value shown
            shown = shown_points(obj, rangeX, "X" in logs) if rangeX else []
            ref = root_frame(points_alone(obj, shown) if shown else obj, opt, logs)
            fallback = "Y" in logs and close(ref["Y"][0], 0.001 * ref["Y"][1], 1e-6)
            if case["range"] not in IGNORED_MIN and not fallback:
                check(f"y frame of the points shown as ROOT draws them: {what}", same(y, ref["Y"]), f"{fmt(y)}, ROOT {fmt(ref['Y'])}")
            if fallback and positive:
                check(f"log y of a graph with values at or below zero starts at half its smallest positive value shown: {what}", close(y[0], 0.5 * min(positive), 1e-6), f"{fmt(y)}, smallest positive {min(positive):.6g}")
            if values:
                shownValues = positive if "Y" in logs else values
                check(f"every point shown is inside the y range: {what}", not shownValues or (y[0] <= min(shownValues) and y[1] >= max(shownValues)), f"{fmt(y)}, values {min(shownValues):.6g} to {max(shownValues):.6g}")
        elif positive or ("Y" not in logs and values):
            vmin, vmax = min(values), max(values)
            if "Y" not in logs:
                inside = y[0] <= vmin + 1e-3 * (vmax - vmin) and y[1] >= vmax - 1e-3 * (vmax - vmin)
            elif vmin > 0:
                inside = y[0] <= vmin * (1 + 1e-3) and y[0] >= 0.1 * vmin and vmax * (1 - 1e-3) <= y[1] <= 10 * vmax
            else:  # a curve crossing zero has no smallest positive value
                inside = vmax * (1 - 1e-3) <= y[1] <= 10 * vmax
            check(f"y range of a function holds every value shown, at most a decade beyond it: {what}", inside, f"{fmt(y)}, values {vmin:.6g} to {vmax:.6g}")
    elif "Z" in logs:
        fh = run.frame_hist(f"c{i}", "log")
        z = (fh.GetMinimum(), fh.GetMaximum())
        floor = min(1.0, 0.001 * z[1])
        if min(values) > 0:
            check(f"log z colours every value shown: {what}", z[0] <= min(values) * (1 + 1e-9) and z[1] >= max(values) * (1 - 1e-9), f"z {fmt(z)}, values {min(values):.6g} to {max(values):.6g}")
        else:
            check(f"log z of values at or below zero starts at ROOT's floor: {what}", close(z[0], floor, 1e-6) and z[1] >= max(values) * (1 - 1e-9), f"z {fmt(z)}, values up to {max(values):.6g}, floor {floor:.6g}")

fh = run.frame_hist("ones", "log")
check("log z colours the ones next to a maximum of 2e4 [6251b3b]", bool(fh) and 0 < fh.GetMinimum() <= 1.0, f"z from {fh.GetMinimum()}" if fh else "plot not made")
warned = run.warnings("is not positive and cannot be shown on a log scale")
check("each lower range at or below zero on log x is reported", len(warned) == nIgnored, f"{len(warned)} warnings for {nIgnored} cases")
errors = run.errors()
check("no errors from the run", not errors, " | ".join(errors[:3]))
finish()
