"""A range set on a pad axis gives a frame that ends at the requested values, for every data kind, linear and log axes, ranges
inside the data, beyond it (the frame is widened: SciRooPlot's rule, unlike ROOT's clamping) and set on one side only. Inside the
bins of a histogram the frame shows whole bins (ROOT): it ends at the edges of the bins holding the requested values. Graphs and
functions have no bins: their frame ends exactly at the values.
Also: no histogram bin outside the frame is drawn, and widening x around all the data leaves the automatic y range as it is. Values of 1d
histograms (bins that are not empty) and graphs (points) within the x range of the frame but above or below it are cut off: a hint says how
many of how many, and no hint is given where none are (values at a range end are on the frame, also on a log axis: no hint, and no
warning that a graph of such values lies entirely beyond the frame). A formula of
the plot definition without a range of its own that gives the frame spans the pad range (it has no range but ROOT's default 0 to 1): it is
drawn across the whole frame, and the automatic y range (colour scale) is that of the drawn function, also of a function drawn
with many samples (SetNpx).
1d data filling less than a tenth of the height of a linear y axis (a range set far too wide) gets a hint that it looks flat; not with the
automatic range, a range a little wider, a log axis or a map in the pad.
Covers: range beyond the axis, exact ends on graph frames, log y headroom, bins outside the frame, formulas spanning the pad range."""

import math
import re

from srptest import BINNED_AXES, DATA_1D, DATA_2D, DATA_KINDS, ROOT, Project, S, add_data, cases, check, close, describe, finish, formula, own

X_RANGES = {"inside": (2.3, 7.7), "beyond": (0.5, 14.0), "above": (1.0, 13.7), "below": (0.5, 10.0), "exact": (1.0, 10.0), "min_only": (3.0, None), "max_only": (None, 7.0)}
Y_RANGES = {"inside": (2.0, 6.0), "beyond": (0.5, 20.0), "min_only": (2.0, None), "max_only": (None, 30.0)}
SCALES = {"lin": (), "logx": ("X",), "logy": ("Y",), "logxy": ("X", "Y")}
CONTAINS_DATA = ("beyond", "above", "below", "exact")  # x ranges around all the data

p = Project("user_ranges")
p.inputs()
todo = cases(
    {"kind": DATA_1D + DATA_2D, "scale": list(SCALES), "axis": ["X", "Y"], "range": list(X_RANGES)},
    # a lower end above all values (efficiencies end below 1) is an empty range: whole axis with a warning, see robust_frames.py
    valid=lambda c: c["range"] in (X_RANGES if c["axis"] == "X" else Y_RANGES) and not (c["kind"] in DATA_2D and "y" in c["scale"]) and not (DATA_KINDS[c["kind"]][1] == "TEfficiency" and c["axis"] == "Y" and c["range"] == "min_only"),
    named=[
        {"kind": "hist", "scale": "lin", "axis": "X", "range": "above", "why": "a range ending at the axis edge does not overshoot"},
        {"kind": "graph", "scale": "logxy", "axis": "X", "range": "inside", "why": "graph frame on log x and y"},
        {"kind": "graph", "scale": "lin", "axis": "X", "range": "min_only", "why": "graph frame with only a lower range"},
        {"kind": "hist", "scale": "logy", "axis": "X", "range": "beyond", "why": "log y headroom"},
        {"kind": "hist_var", "scale": "logx", "axis": "X", "range": "inside", "why": "lines stay inside the frame"},
        {"kind": "hist2d", "scale": "lin", "axis": "Y", "range": "beyond", "why": "a range ending at the axis edge does not overshoot"},
        {"kind": "function", "scale": "lin", "axis": "X", "range": "inside", "why": "frame ended on the sampling grid"},
        {"kind": "function2d", "scale": "lin", "axis": "Y", "range": "inside", "why": "frame ended on the sampling grid"},
    ],
)


def define(name, case, withRange=True):
    plot = S.Plot(name, "ranges", "2d" if case["kind"] in DATA_2D else "1d")
    add_data(plot[1], case["kind"])
    for axis in SCALES[case["scale"]]:
        plot[1][axis].SetLog()
    if withRange:
        low, high = (X_RANGES if case["axis"] == "X" else Y_RANGES)[case["range"]]
        axis = plot[1][case["axis"]]
        if low is not None and high is not None:
            axis.SetRange(low, high)
        elif low is not None:
            axis.SetMinRange(low)
        else:
            axis.SetMaxRange(high)
    p.pm.AddPlot(plot)


references = set()
for i, case in enumerate(todo):
    define(f"c{i}", case)
    references.add((case["kind"], case["scale"]))
for kind, scale in references:
    define(f"ref_{kind}_{scale}", {"kind": kind, "scale": scale}, withRange=False)
# formulas without a range of their own, pad ranges beyond and inside ROOT's default range 0 to 1, on linear and log axes
FORMULA_CASES = [(kind, x, log) for kind in ("function", "function2d") for x in ((2.3, 13.7), (0.2, 0.7)) for log in (False, True)]
for i, (kind, x, log) in enumerate(FORMULA_CASES):
    plot = S.Plot(f"formula{i}", "ranges", "2d" if kind in DATA_2D else "1d")
    plot[1].AddFunction(formula(kind))
    plot[1]["X"].SetRange(*x).SetLog(log)
    if kind in DATA_2D:
        plot[1]["Y"].SetRange(*x)
    p.pm.AddPlot(plot)
# a narrow peak drawn with many samples (SetNpx): the frame comes from the samples it is drawn with
plot = S.Plot("narrow_peak", "ranges", "1d")
plot[1].AddFunction("TMath::Gaus(x, 0.5003, 0.0002)").SetNpx(10000)
p.pm.AddPlot(plot)
# contents 1 to 10, a y range ending at two of them: on log y, 10^log10(8) comes back below 8
steps = own(ROOT.TH1D("steps", "", 10, 0, 10))
for b in range(1, 11):
    steps.SetBinContent(b, b)
p.pm.AddDataSource("steps", steps)
EDGE_RANGES = [(log, r) for log in (False, True) for r in ((1, 8), (5, 10))]
# graphs whose points all lie at one end of the y range: drawn on the frame edge, so not entirely beyond the frame
for i, (log, r) in enumerate(EDGE_RANGES):
    for end, value in (("low", r[0]), ("high", r[1])):
        graph = own(ROOT.TGraph(3))
        graph.SetName(f"{end}{i}")
        for k in range(3):
            graph.SetPoint(k, 2 + 3 * k, value)
        p.pm.AddDataSource("steps", graph)
for i, (log, r) in enumerate(EDGE_RANGES):
    plot = S.Plot(f"edge{i}", "ranges", "1d")
    plot[1].AddData("steps", "steps")
    plot[1].AddData(f"low{i}", "steps")
    plot[1].AddData(f"high{i}", "steps")
    plot[1]["Y"].SetLog(log).SetRange(*r)
    p.pm.AddPlot(plot)
# data squeezed by a y range set far too wide (under a tenth of its height: a hint); the automatic range, a range a little wider, a pad
# with a map and a log axis none, nor data without spread (a constant without errors) or a function filling the range beside the data
SQUEEZED = {"squeezed": True, "snug": False, "automatic": False, "squeezed_log": False, "squeezed_map": False, "constant": False, "with_function": False}
for name, yRange, log, overMap in (("squeezed", (0, 200), False, False), ("snug", (0, 14), False, False), ("automatic", None, False, False), ("squeezed_log", (1, 1e6), True, False), ("squeezed_map", (0, 200), False, True), ("constant", None, False, False), ("with_function", (0, 200), False, False)):
    plot = S.Plot(name, "ranges", "1d")
    if overMap:
        plot[1].AddData("disc_map", "steps").SetDrawOption(S.colz)
    plot[1].AddData("constant" if name == "constant" else "steps", "steps")
    if name == "with_function":
        plot[1].AddFunction("2*x*x").SetRangeX(0, 10)
    if yRange:
        plot[1]["Y"].SetRange(*yRange).SetLog(log)
    p.pm.AddPlot(plot)
discMap = own(ROOT.TH2D("disc_map", "", 10, 0, 10, 10, 0, 200))
for b in range(1, 101):
    discMap.SetBinContent(b, 1.0)
p.pm.AddDataSource("steps", discMap)
constant = own(ROOT.TH1D("constant", "", 10, 0, 10))
for b in range(1, 11):
    constant.SetBinContent(b, 1.0)
    constant.SetBinError(b, 0.0)
p.pm.AddDataSource("steps", constant)
run = p.plot("ranges")

for i, case in enumerate(todo):
    what = describe(case)
    frame = run.frame(f"c{i}", "ranges")
    if frame is None:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    low, high = (X_RANGES if case["axis"] == "X" else Y_RANGES)[case["range"]]
    got = frame[case["axis"]]
    frameHist = run.pad(f"c{i}", "ranges").GetPrimitive("axis_hist_pad_1")
    binnedAxis = case["axis"] == "X" or case["kind"] in DATA_2D
    if binnedAxis and case["kind"] in BINNED_AXES:
        axis = frameHist.GetXaxis() if case["axis"] == "X" else frameHist.GetYaxis()
        if low is not None and axis.GetXmin() < low < axis.GetXmax():
            low = axis.GetBinLowEdge(axis.FindFixBin(low))
        if high is not None and axis.GetXmin() < high < axis.GetXmax() and not close(axis.GetBinLowEdge(axis.FindFixBin(high)), high, 1e-6):
            high = axis.GetBinUpEdge(axis.FindFixBin(high))
    wanted = [(end, value) for end, value in ((0, low), (1, high)) if value is not None]
    check(f"frame ends at the requested range: {what}", all(close(got[end], value) for end, value in wanted), f"frame {got[0]:.6g} to {got[1]:.6g}")

    if case["kind"] in BINNED_AXES:  # polygons are clipped to the frame by ROOT
        x0, x1 = frame["X"]
        for obj in run.objects(f"c{i}", "ranges"):
            if obj.InheritsFrom("TH1"):
                axis = obj.GetXaxis()
                first, last = axis.GetBinLowEdge(axis.GetFirst()), axis.GetBinUpEdge(axis.GetLast())
                check(f"no bin drawn outside the frame: {what}", first >= x0 * (1 - 1e-9) - 1e-12 and last <= x1 * (1 + 1e-9) + 1e-12, f"bins {first:.6g} to {last:.6g}, frame {x0:.6g} to {x1:.6g}")

    if case["kind"] not in DATA_2D:
        (x0, x1), (y0, y1) = frame["X"], frame["Y"]
        logY = "Y" in SCALES[case["scale"]]
        values = []
        for obj in run.objects(f"c{i}", "ranges"):
            if obj.InheritsFrom("TH1") and obj.GetDimension() == 1:
                values += [(obj.GetBinCenter(b), obj.GetBinContent(b)) for b in range(1, obj.GetNbinsX() + 1) if obj.GetBinContent(b) != 0]
            elif obj.InheritsFrom("TGraph") and not obj.InheritsFrom("TGraph2D") and obj.GetFillStyle() == 0:
                values += [(obj.GetPointX(k), obj.GetPointY(k)) for k in range(obj.GetN())]
        values = [y for x, y in values if x0 <= x <= x1 and math.isfinite(y) and not (logY and y <= 0)]
        cut = sum(y > y1 or y < y0 for y in values)
        said = [m for m in run.messages(f"c{i}", "ranges") if "within the x range of the frame of pad 1 lie" in m]
        counts = [tuple(int(n) for n in re.search(r": (\d+) of the (\d+) ", m).groups()) for m in said]
        if cut and cut < len(values):
            check(f"values cut off at the frame: a hint with their number: {what}", counts == [(cut, len(values))], f"{cut} of {len(values)}; " + " | ".join(said))
        elif not cut:
            check(f"no value cut off at the frame: no hint: {what}", not said, " | ".join(said))

    if case["axis"] == "X" and case["range"] in CONTAINS_DATA and case["kind"] not in DATA_2D:
        reference = run.frame(f"ref_{case['kind']}_{case['scale']}", "ranges")
        if reference:
            same = close(frame["Y"][0], reference["Y"][0], 1e-6) and close(frame["Y"][1], reference["Y"][1], 1e-6)
            check(f"automatic y range unchanged by an x range around all data: {what}", same, f"y {frame['Y'][0]:.6g} to {frame['Y'][1]:.6g}, without x range {reference['Y'][0]:.6g} to {reference['Y'][1]:.6g}")

for i, (kind, x, log) in enumerate(FORMULA_CASES):
    what = f"{kind} without range, pad range {x[0]:g} to {x[1]:g}" + " on log x" * log
    frame = run.frame(f"formula{i}", "ranges")
    funcs = [obj for obj in run.objects(f"formula{i}", "ranges") if obj.InheritsFrom("TF1")]
    if frame is None or not funcs:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    func = funcs[0]
    check(f"frame ends at the pad range: {what}", close(frame["X"][0], x[0]) and close(frame["X"][1], x[1]), f"frame {frame['X'][0]:.6g} to {frame['X'][1]:.6g}")
    spans = close(func.GetXmin(), x[0]) and close(func.GetXmax(), x[1])
    if kind in DATA_2D:
        spans = spans and close(func.GetYmin(), x[0]) and close(func.GetYmax(), x[1])
    check(f"function drawn across the frame: {what}", spans, f"function x {func.GetXmin():.6g} to {func.GetXmax():.6g}")
    # the automatic value range is that of the function within the frame (y axis for 1d, colour scale for 2d)
    steps = [x[0] + (x[1] - x[0]) * k / 200 for k in range(201)]
    if kind in DATA_2D:
        reference = ROOT.TF2(f"v{i}", formula(kind))
        values = [reference.Eval(a, b) for a in steps[::10] for b in steps[::10]]
        frameHist = run.frame_hist(f"formula{i}", "ranges")
        low, high = frameHist.GetMinimum(), frameHist.GetMaximum()
    else:
        reference = ROOT.TF1(f"v{i}", formula(kind))
        values = [reference.Eval(a) for a in steps]
        low, high = frame["Y"]
    span = max(values) - min(values)
    if kind in DATA_2D:  # the colour scale is that of the cell centres: inside the values, close to their span
        covered = low >= min(values) - 1e-6 * span and high <= max(values) + 1e-6 * span and high - low > 0.9 * span
    else:
        covered = low <= min(values) + 1e-6 * span and high >= max(values) - 1e-6 * span
    check(f"automatic value range is that of the drawn function: {what}", covered, f"range {low:.6g} to {high:.6g}, function {min(values):.6g} to {max(values):.6g}")

for i, (log, r) in enumerate(EDGE_RANGES):
    said = [m for m in run.messages(f"edge{i}", "ranges") if "within the x range of the frame of pad 1 lie" in m]
    want = "2 of the 10 bins" if r == (1, 8) else "4 of the 10 bins"
    check(f"values at the ends of a y range {r[0]} to {r[1]} are not cut off" + " (log y)" * log, len(said) == 1 and want in said[0], " | ".join(said))
    beyond = [m for m in run.messages(f"edge{i}", "ranges") if "lies entirely" in m]
    check(f"graphs at the ends of a y range {r[0]} to {r[1]} are not beyond the frame" + " (log y)" * log, not beyond, " | ".join(beyond))

frame = run.frame("narrow_peak", "ranges")
funcs = [obj for obj in run.objects("narrow_peak", "ranges") if obj.InheritsFrom("TF1")]
peak = 0.99 * funcs[0].GetMaximum() if funcs else None  # (the frame of 1000 samples reached 0.64)
check("a function sampled at SetNpx points gives the frame: its peak inside", bool(frame) and peak is not None and frame["Y"][1] >= peak, f"frame up to {frame['Y'][1] if frame else None}, peak {peak}")
for name, want in SQUEEZED.items():
    said = [m for m in run.messages(name, "ranges") if "look flat" in m]
    check(f"data under a tenth of the height of a linear y axis of 1d data gets a hint, other data none: {name}", len(said) == int(want), " | ".join(said))
check("no errors from the run", not run.errors(), " | ".join(run.errors()[:3]))
finish()
