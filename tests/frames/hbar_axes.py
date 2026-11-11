"""A pad whose frame is drawn with horizontal bars (hbars or ROOT's HBAR: ROOT draws the histogram's x axis vertically) keeps the meaning of its axes:
'X' is the horizontal axis (the contents), 'Y' the vertical one (the bins): their ranges, titles and label sizes land on the axis drawn
there; the room above the maximum is room right of the bars, which start at the left edge of the frame; a log horizontal axis starts at
the smallest positive content of the bins shown; axes drawn again on top (SetRedrawAxes) keep the look of the axis drawn at each place;
a side panel of a map shares the map's vertical range, with its labels hidden on the shared axis only.
Cases: every 1d histogram kind drawn with hbars and HBAR × no setting, an x range, a y range inside the bins, room above the maximum, a log
x axis, titles and label sizes, and the named cases.
Reference: the pad ranges and the axes of the frame histogram.
Covers: hbars axes, log minimum from the bins in range, centred titles of the redrawn
axes, user ranges on the axis drawn there, and the named cases.
"""

from srptest import DATA_1D, DATA_KINDS, ROOT, Project, S, add_data, cases, check, close, data_objects, describe, finish

KINDS = tuple(k for k in DATA_1D if "binned" in DATA_KINDS[k][2] and DATA_KINDS[k][1] != "TEfficiency")  # a TEfficiency is drawn as a graph
SETTINGS = ("none", "x_range", "y_range", "room", "log", "looks")
OPTIONS = {"hbars": S.hbars, "raw_HBAR": "HBAR"}  # the alias and ROOT's option as it is
todo = cases(
    {"kind": list(KINDS), "option": list(OPTIONS), "setting": list(SETTINGS)},
    named=[{"kind": "hist_var", "option": "hbars", "setting": "y_range", "why": "a range between the variable bins ends at their edges"}],
)

p = Project("hbar_axes")
p.inputs()
inputs = data_objects()
for i, case in enumerate(todo):
    plot = S.Plot(f"h{i}", "hbars", "1d")
    pad = plot[1]
    add_data(pad, case["kind"]).SetDrawOption(OPTIONS[case["option"]])
    setting = case["setting"]
    if setting == "x_range":
        pad["X"].SetRange(0.5, 20.0)
    elif setting == "y_range":
        pad["Y"].SetRange(3.0, 7.0)
    elif setting == "room":
        pad["X"].SetRoomAboveMax(0.25)
    elif setting == "log":
        pad["X"].SetLog()
    elif setting == "looks":
        pad["X"].SetTitle("contents horizontal").SetLabelSize(0.031)
        pad["Y"].SetTitle("bins vertical").SetLabelSize(0.047)
    p.pm.AddPlot(plot)

m = ROOT.TH2D("m", ";x;y", 12, -6.0, 6.0, 12, -6.0, 6.0)
h = ROOT.TH1D("hs", ";y;counts", 12, -6.0, 6.0)
for b in range(1, 13):
    h.SetBinContent(b, 10.0 + b)
    for c in range(1, 13):
        m.SetBinContent(b, c, b + c)
p.source("s", {"m": m, "hs": h})
side = S.Plot("side", S.PanelLayout([[S.Panel(), S.Panel(0.4, 1.0)]]))
side.SetGroup("named")
side[1].AddData("m", "s").SetDrawOption(S.col)
side[2].AddData("hs", "s").SetDrawOption(S.hbars)
side[2].SetRedrawAxes()
side[2]["X"].SetTitle("counts").SetTitleCenter()
p.pm.AddPlot(side)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def contents(obj):
    ax = obj.GetXaxis()
    return [(ax.GetBinLowEdge(b), ax.GetBinUpEdge(b), obj.GetBinContent(b)) for b in range(1, ax.GetNbins() + 1)]


for i, case in enumerate(todo):
    what = describe(case)
    padObj = run.pad(f"h{i}", "hbars")
    frame = run.frame_hist(f"h{i}", "hbars")
    drawn = run.objects(f"h{i}", "hbars")
    if not padObj or not frame or not drawn:
        check(f"{what}: drawn", False, " | ".join(run.messages(f"h{i}", "hbars")))
        continue
    f = run.frame(f"h{i}", "hbars")
    bins = contents(drawn[0])
    setting = case["setting"]
    vertical = (frame.GetXaxis().GetBinLowEdge(frame.GetXaxis().GetFirst()), frame.GetXaxis().GetBinUpEdge(frame.GetXaxis().GetLast()))
    check(f"{what}: the bins on the vertical axis", close(f["Y"][0], vertical[0], 1e-9) and close(f["Y"][1], vertical[1], 1e-9), f"pad y {f['Y']}, bins {vertical}")
    if setting == "y_range":
        # the frame ends at the bin edges enclosing the range (variable bins: 2.7 and 7.5), as for every binned axis
        axis = drawn[0].GetXaxis()
        low, high = 3.0, 7.0
        if not close(axis.GetBinLowEdge(axis.FindFixBin(low)), low, 1e-6):
            low = axis.GetBinLowEdge(axis.FindFixBin(low))
        if not close(axis.GetBinLowEdge(axis.FindFixBin(high)), high, 1e-6):
            high = axis.GetBinUpEdge(axis.FindFixBin(high))
        check(f"{what}: the y range on the bins (vertical), at the bin edges enclosing it", close(f["Y"][0], low, 1e-9) and close(f["Y"][1], high, 1e-9), f"{f['Y']}, expected ({low}, {high})")
    if setting == "x_range":
        check(f"{what}: the x range on the contents (horizontal)", close(f["X"][0], 0.5, 1e-9) and close(f["X"][1], 20.0, 1e-9), f"{f['X']}")
    shown = [c for lo, hi, c in bins if lo >= f["Y"][0] - 1e-9 and hi <= f["Y"][1] + 1e-9]
    if setting == "log":
        positive = [c for c in shown if c > 0]
        check(f"{what}: a log horizontal axis from the smallest positive content shown", padObj.GetLogx() and positive and f["X"][0] <= min(positive) and f["X"][0] > 0.0 and f["X"][0] > 0.05 * min(positive), f"from {f['X'][0]}, smallest {min(positive) if positive else None}")
    elif setting == "room":
        largest = max(shown)
        check(f"{what}: a quarter of the width free right of the bars", close((f["X"][1] - largest) / (f["X"][1] - f["X"][0]), 0.25, 1e-6), f"frame {f['X']}, largest {largest}")
    elif setting != "x_range":
        check(f"{what}: the contents fit horizontally", f["X"][0] <= min(min(shown), 0.0) + 1e-9 and f["X"][1] >= max(shown) - 1e-9, f"frame {f['X']}, contents {min(shown)} to {max(shown)}")
    if not padObj.GetLogx():
        check(f"{what}: the bars start at the left edge of the frame", close(drawn[0].GetMinimum(), f["X"][0], 1e-9), f"minimum {drawn[0].GetMinimum()}, frame from {f['X'][0]}")
    if setting == "looks":
        horizontal, verticalAxis = frame.GetYaxis(), frame.GetXaxis()  # HBAR: the histogram's y axis is drawn horizontally
        ok = horizontal.GetTitle() == "contents horizontal" and close(horizontal.GetLabelSize(), 0.031, 1e-6) and verticalAxis.GetTitle() == "bins vertical" and close(verticalAxis.GetLabelSize(), 0.047, 1e-6)
        check(f"{what}: titles and label sizes on the axes drawn at each place", ok, f"horizontal {horizontal.GetTitle()!r} {horizontal.GetLabelSize()}, vertical {verticalAxis.GetTitle()!r} {verticalAxis.GetLabelSize()}")

mapPad, sidePad = run.pad("side", "named", 1), run.pad("side", "named", 2)
sideFrame = run.frame_hist("side", "named", 2)
if mapPad and sidePad and sideFrame:
    check("a side panel of a map: the map's vertical range", (round(sidePad.GetUymin(), 9), round(sidePad.GetUymax(), 9)) == (round(mapPad.GetUymin(), 9), round(mapPad.GetUymax(), 9)), f"{(sidePad.GetUymin(), sidePad.GetUymax())}")
    check("a side panel of a map: labels hidden on the shared vertical axis, kept on the count axis", sideFrame.GetXaxis().GetLabelSize() == 0.0 and sideFrame.GetYaxis().GetLabelSize() > 0.0, f"{(sideFrame.GetXaxis().GetLabelSize(), sideFrame.GetYaxis().GetLabelSize())}")
    copies = [o for o in sidePad.GetListOfPrimitives() if o.InheritsFrom("TH1") and o.GetXaxis().GetTitle() == "counts"]
    check("axes drawn again on top keep the centred title of the axis drawn there", copies and all(o.GetXaxis().TestBit(ROOT.TAxis.kCenterTitle) for o in copies), f"{len(copies)} copies")
else:
    check("the side panel of a map is drawn", False, " | ".join(run.messages("side", "named")))
finish()
