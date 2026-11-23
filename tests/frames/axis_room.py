"""The room above the maximum and below the minimum of an axis (Axis::SetRoomAboveMax, Axis::SetRoomBelowMin) is that fraction of the
frame left free beyond everything drawn in the pad, for every data kind drawn flat, on every axis of the pad (x and y; the z of a map),
linear and log. Where it cannot apply it is ignored with a warning: a range set on that side, a z axis on a pad without one, room
below and above adding up to the whole frame, a binned axis with bin labels.
Cases: every 1d and 2d data kind (not polar) × axis X, Y, Z (maps only) × room above, below or both × linear or log axis, and the
ignored settings.
Reference: the extent of the drawn objects read back (bin edges, contents with the errors drawn, points with their errors, functions
sampled at their number of points) against the pad ranges, in log10 on log axes.
Covers: room beyond histograms, polygon histograms, graphs, 2d graphs, 1d and 2d functions on every axis, and the warnings of room
that is ignored.
"""

import math

from srptest import DATA_1D, DATA_2D, DATA_KINDS, Project, S, add_data, cases, check, describe, finish

ABOVE, BELOW = 0.2, 0.15
KINDS = DATA_1D + DATA_2D
todo = cases(
    {"kind": list(KINDS), "axis": ["X", "Y", "Z"], "side": ["above", "below", "both"], "scale": ["linear", "log"]},
    valid=lambda c: (c["axis"] != "Z" or DATA_KINDS[c["kind"]][0] == 2) and not (c["axis"] == "X" and "labels" in DATA_KINDS[c["kind"]][2]),
    named=[
        {"kind": "hist2poly", "axis": "Z", "side": "both", "scale": "linear", "why": "the contents of polygon bins inside the frame"},
        {"kind": "hist2poly", "axis": "X", "side": "above", "scale": "linear", "why": "the extent of polygon bins along x"},
        {"kind": "graph2d_errors", "axis": "Z", "side": "above", "scale": "linear", "why": "2d graph points with their z errors"},
        {"kind": "graph2d", "axis": "Y", "side": "below", "scale": "linear", "why": "the extent of 2d graph points along y"},
        {"kind": "tf2", "axis": "Z", "side": "both", "scale": "log", "why": "a 2d function sampled inside the frame"},
        {"kind": "tf2", "axis": "X", "side": "above", "scale": "linear", "why": "the range of a 2d function along x"},
        {"kind": "graph_asymm", "axis": "Y", "side": "both", "scale": "log", "why": "asymmetric errors on a log axis"},
    ],
)
# settings that cannot apply: (kind, setting of the pad, the warning)
IGNORED = {
    "range below": ("hist", lambda pad: (pad["Y"].SetRoomBelowMin(0.1), pad["Y"].SetMinRange(0.5)), "The room below the minimum (0.1) of the Y axis of pad 1 is ignored: its minimum is set explicitly"),
    "range above": ("graph", lambda pad: (pad["X"].SetRoomAboveMax(0.1), pad["X"].SetMaxRange(12.0)), "The room above the maximum (0.1) of the X axis of pad 1 is ignored: its maximum is set explicitly"),
    "z on a 1d pad": ("hist", lambda pad: pad["Z"].SetRoomAboveMax(0.1), "The room on the Z axis of pad 1 is ignored: the pad has no z axis"),
    "the whole frame": ("hist2d", lambda pad: (pad["Z"].SetRoomAboveMax(0.6), pad["Z"].SetRoomBelowMin(0.4)), "The room on the Z axis of pad 1 is ignored: room below and above add up to 1 of the frame"),
    "bin labels": ("hist_labels", lambda pad: pad["X"].SetRoomAboveMax(0.1), "The room on the X axis of pad 1 is ignored: an axis with bin labels cannot be widened"),
}

p = Project("axis_room")
p.inputs()
for i, case in enumerate(todo):
    dim = DATA_KINDS[case["kind"]][0]
    plot = S.Plot(f"r{i}", "room", "2d" if dim == 2 else "1d")
    pad = plot[1]
    add_data(pad, case["kind"])
    axis = pad[case["axis"]]
    if case["side"] in ("above", "both"):
        axis.SetRoomAboveMax(ABOVE)
    if case["side"] in ("below", "both"):
        axis.SetRoomBelowMin(BELOW)
    if case["scale"] == "log":
        axis.SetLog()
    p.pm.AddPlot(plot)
for name, (kind, setting, _) in IGNORED.items():
    plot = S.Plot(name.replace(" ", "_"), "ignored", "2d" if DATA_KINDS[kind][0] == 2 else "1d")
    add_data(plot[1], kind)
    setting(plot[1])
    p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def extent(drawn, axis, frame, log, flat):
    """lowest and highest value of the drawn data along an axis, (None, None) if nothing is drawn there"""
    values = []

    def add(low, value, high):
        if not math.isfinite(value) or (log and value <= 0.0):
            return
        values.append((value if log and low <= 0.0 else low, high))

    def inside(low, high, ax):
        return high >= frame[ax][0] and low <= frame[ax][1]

    for obj, opt in drawn:
        if obj.InheritsFrom("TH2Poly"):
            for b in obj.GetBins():
                if axis == "X":
                    add(b.GetXMin(), 0.5 * (b.GetXMin() + b.GetXMax()), b.GetXMax())
                elif axis == "Y":
                    add(b.GetYMin(), 0.5 * (b.GetYMin() + b.GetYMax()), b.GetYMax())
                elif inside(b.GetXMin(), b.GetXMax(), "X") and inside(b.GetYMin(), b.GetYMax(), "Y"):
                    add(b.GetContent(), b.GetContent(), b.GetContent())
        elif obj.InheritsFrom("TH1") and obj.GetDimension() == 1:
            ax = obj.GetXaxis()
            errors = "E" in opt.replace("TEXT", "")
            for b in range(1, ax.GetNbins() + 1):
                low, up = ax.GetBinLowEdge(b), ax.GetBinUpEdge(b)
                if axis == "X":
                    add(low, ax.GetBinCenter(b), up)
                elif inside(low, up, "X"):
                    c, e = obj.GetBinContent(b), obj.GetBinError(b) if errors else 0.0
                    add(c - e, c, c + e)
        elif obj.InheritsFrom("TH2"):
            xa, ya = obj.GetXaxis(), obj.GetYaxis()
            if axis == "X":
                add(xa.GetBinLowEdge(1), xa.GetBinCenter(1), xa.GetBinUpEdge(xa.GetNbins()))
            elif axis == "Y":
                add(ya.GetBinLowEdge(1), ya.GetBinCenter(1), ya.GetBinUpEdge(ya.GetNbins()))
            else:
                for bx in range(1, xa.GetNbins() + 1):
                    for by in range(1, ya.GetNbins() + 1):
                        if inside(xa.GetBinLowEdge(bx), xa.GetBinUpEdge(bx), "X") and inside(ya.GetBinLowEdge(by), ya.GetBinUpEdge(by), "Y"):
                            c = obj.GetBinContent(bx, by)
                            add(c, c, c)
        elif obj.InheritsFrom("TGraph2D"):
            for k in range(obj.GetN()):
                x, y, z = obj.GetX()[k], obj.GetY()[k], obj.GetZ()[k]
                if axis == "X":
                    add(x, x, x)
                elif axis == "Y":
                    add(y, y, y)
                elif inside(x, x, "X") and inside(y, y, "Y"):
                    e = max(obj.GetErrorZ(k), 0.0)
                    add(z - e, z, z + e)
        elif obj.InheritsFrom("TGraph"):
            for k in range(obj.GetN()):
                x, y = obj.GetPointX(k), obj.GetPointY(k)
                if axis == "X":
                    add(x - max(obj.GetErrorXlow(k), 0.0), x, x + max(obj.GetErrorXhigh(k), 0.0))
                elif not flat or inside(x, x, "X"):
                    add(y - max(obj.GetErrorYlow(k), 0.0), y, y + max(obj.GetErrorYhigh(k), 0.0))
        elif obj.InheritsFrom("TF2"):
            if axis == "X":
                add(obj.GetXmin(), obj.GetXmin(), obj.GetXmax())
            elif axis == "Y":
                add(obj.GetYmin(), obj.GetYmin(), obj.GetYmax())
            else:
                x0, x1 = max(obj.GetXmin(), frame["X"][0]), min(obj.GetXmax(), frame["X"][1])
                y0, y1 = max(obj.GetYmin(), frame["Y"][0]), min(obj.GetYmax(), frame["Y"][1])
                nx, ny = max(obj.GetNpx(), 2), max(obj.GetNpy(), 2)
                for a in range(nx):
                    for b in range(ny):
                        v = obj.Eval(x0 + (x1 - x0) * a / (nx - 1), y0 + (y1 - y0) * b / (ny - 1))
                        add(v, v, v)
        elif obj.InheritsFrom("TF1"):
            if axis == "X":
                add(obj.GetXmin(), obj.GetXmin(), obj.GetXmax())
            else:
                x0, x1 = max(obj.GetXmin(), frame["X"][0]), min(obj.GetXmax(), frame["X"][1])
                n = max(obj.GetNpx(), 2)
                for a in range(n):
                    v = obj.Eval(x0 + (x1 - x0) * a / (n - 1))
                    add(v, v, v)
    if not values:
        return None, None
    return min(v[0] for v in values), max(v[1] for v in values)


for i, case in enumerate(todo):
    what = describe(case)
    name, ax = f"r{i}", case["axis"]
    padObj, frameHist, drawn = run.pad(name, "room"), run.frame_hist(name, "room"), run.drawn(name, "room")
    if not padObj or not frameHist or not drawn:
        check(f"{what}: drawn", False, " | ".join(run.messages(name, "room")))
        continue
    frame = run.frame(name, "room")
    flat = DATA_KINDS[case["kind"]][0] == 1
    frame["Z"] = (frameHist.GetMinimum(), frameHist.GetMaximum())
    log = {"X": padObj.GetLogx(), "Y": padObj.GetLogy(), "Z": padObj.GetLogz()}[ax]
    low, high = extent(drawn, ax, frame, log, flat)
    if low is None:
        check(f"{what}: data drawn on the axis", False)
        continue
    t = math.log10 if log else (lambda v: v)
    lo, hi = t(frame[ax][0]), t(frame[ax][1])
    width = hi - lo
    detail = f"frame {frame[ax]}, data {low} to {high}"
    if case["side"] in ("above", "both"):
        check(f"{what}: {ABOVE:g} of the frame free above everything drawn", abs((hi - t(high)) / width - ABOVE) < 1e-6, detail)
    else:
        check(f"{what}: everything drawn below the top of the frame", hi >= t(high) - 1e-9 * abs(width), detail)
    if case["side"] in ("below", "both"):
        check(f"{what}: {BELOW:g} of the frame free below everything drawn", abs((t(low) - lo) / width - BELOW) < 1e-6, detail)
    else:
        check(f"{what}: everything drawn above the bottom of the frame", lo <= t(low) + 1e-9 * abs(width), detail)

for name, (kind, _, warning) in IGNORED.items():
    said = [m for m in run.messages(name.replace(" ", "_"), "ignored") if warning in m]
    check(f"room ignored ({name}): one warning", len(said) == 1, " | ".join(run.messages(name.replace(" ", "_"), "ignored")[:3]))
finish()
