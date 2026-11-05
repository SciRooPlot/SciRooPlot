"""A range set on a pad axis gives a frame that ends at the requested values, for every data kind, linear and log axes, ranges
inside the data, beyond it (the frame is widened: SciRooPlot's rule, unlike ROOT's clamping) and set on one side only. Inside the
bins of a histogram the frame shows whole bins (ROOT): it ends at the edges of the bins holding the requested values. Graphs and
functions have no bins: their frame ends exactly at the values.
Also: no histogram bin outside the frame is drawn, and widening x around all the data leaves the automatic y range as it is.
Covers: 7c2f879 (range beyond the axis), 08f2af5 (exact ends on graph frames, log y headroom), 5b47ca3 (bins outside the frame)."""

from srptest import BINNED_AXES, DATA_1D, DATA_2D, DATA_KINDS, Project, S, add_data, cases, check, close, describe, finish

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
        {"kind": "hist", "scale": "lin", "axis": "X", "range": "above", "why": "7c2f879"},
        {"kind": "graph", "scale": "logxy", "axis": "X", "range": "inside", "why": "08f2af5"},
        {"kind": "graph", "scale": "lin", "axis": "X", "range": "min_only", "why": "08f2af5"},
        {"kind": "hist", "scale": "logy", "axis": "X", "range": "beyond", "why": "08f2af5 log y headroom"},
        {"kind": "hist_var", "scale": "logx", "axis": "X", "range": "inside", "why": "5b47ca3"},
        {"kind": "hist2d", "scale": "lin", "axis": "Y", "range": "beyond", "why": "7c2f879"},
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

    if case["axis"] == "X" and case["range"] in CONTAINS_DATA and case["kind"] not in DATA_2D:
        reference = run.frame(f"ref_{case['kind']}_{case['scale']}", "ranges")
        if reference:
            same = close(frame["Y"][0], reference["Y"][0], 1e-6) and close(frame["Y"][1], reference["Y"][1], 1e-6)
            check(f"automatic y range unchanged by an x range around all data: {what}", same, f"y {frame['Y'][0]:.6g} to {frame['Y'][1]:.6g}, without x range {reference['Y'][0]:.6g} to {reference['Y'][1]:.6g}")

check("no errors from the run", not run.errors(), " | ".join(run.errors()[:3]))
finish()
