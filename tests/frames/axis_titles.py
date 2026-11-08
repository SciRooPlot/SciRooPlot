"""The title of a colour scale sits next to its widest label, the same gap in pixels for any labels (short, long, written with a power of
ten taken out, small values), also for a 3d histogram's scale ('C') and in a ColorScale() cell; so does a y title drawn once for touching
panels (ROOT places the y title of a single pad itself); an offset set for it (Z or C SetTitleOffset > 0) puts it at that distance
instead. The x and y titles of a 3d view are centred at offset 1.5 unless set.
Cases: every 2d data kind and the 3d histograms drawn with their colour scale × contents of short labels, long ones, ones with a power of
ten taken out and small ones × in a pad or with a ColorScale() cell; y axes of such labels in panels above each other; and the named cases.
Reference: the pixels of the painted pad: the gap between the labels and the title right of a colour scale and left of a y axis, against
the gap of the same plot with short labels.
Covers: titles of colour scales and 3d views placed by their widest label, the power of ten TGaxis takes out of the
labels (estimated other than TGaxis does it: titles on top of the labels or far from them).
"""

import numpy as np

from srptest import DATA_KINDS, ROOT, WORK, Project, S, add_data, cases, check, close, describe, finish

KINDS = tuple(k for k, (dim, cls, props) in DATA_KINDS.items() if dim == 2 and not cls.startswith("TF") or dim == 3 and "binned" in props)
LABELS = {"short": None, "long": (0.0012345, 123.456), "exponent": (0.0, 123456.0), "small": (0.00012, 0.00016)}  # ranges of the value axis
todo = cases({"kind": list(KINDS), "labels": list(LABELS), "where": ["pad", "cell"]})

p = Project("axis_titles", bases=("2d", "1d_1x2"))
p.inputs()
for i, case in enumerate(todo):
    kind = case["kind"]
    if case["where"] == "cell":
        plot = S.Plot(f"t{i}", S.PanelLayout([[S.Panel(), S.ColorScale()]]))
        plot.SetGroup("titles")
    else:
        plot = S.Plot(f"t{i}", "titles", "2d")
    pad = plot[1]
    add_data(pad, kind).SetDrawOption(S.colz)
    value = "C" if DATA_KINDS[kind][0] == 3 else "Z"
    if LABELS[case["labels"]]:
        pad[value].SetRange(*LABELS[case["labels"]])
    pad[value].SetTitle("the colour scale")
    pad["Y"].SetTitle("the y axis")
    p.pm.AddPlot(plot)
plot = S.Plot("reference", "named", "2d")  # the gap of a plain 2d plot: y labels 1 to 10
plot[1].AddData("hist2d", "inputs").SetDrawOption(S.colz)
plot[1]["Y"].SetTitle("the y axis")
plot[1]["Z"].SetTitle("the colour scale")
p.pm.AddPlot(plot)
for label, yRange in LABELS.items():  # one y title for two panels above each other
    plot = S.Plot(f"y_{label}", "named", "1d_1x2")
    for pad in (1, 2):
        plot[pad].AddData("hist", "inputs")
        plot[pad]["Y"].SetTitle("the y axis")
        if yRange:
            plot[pad]["Y"].SetRange(*yRange)
    p.pm.AddPlot(plot)
plot = S.Plot("offset", "named", "2d")
plot[1].AddData("hist2d", "inputs").SetDrawOption(S.colz)
plot[1]["Z"].SetTitle("the colour scale").SetTitleOffset(1.8)
p.pm.AddPlot(plot)
plot = S.Plot("view", "named", "2d")
plot[1].AddData("hist2d", "inputs").SetDrawOption(S.lego)
plot[1]["X"].SetTitle("x")
plot[1]["Y"].SetTitle("y")
p.pm.AddPlot(plot)
plot = S.Plot("view_offset", "named", "2d")
plot[1].AddData("hist2d", "inputs").SetDrawOption(S.lego)
plot[1]["X"].SetTitle("x").SetTitleOffset(2.5)
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def painted(name, group):
    canvas = run.canvas(name, group)
    ROOT.SetOwnership(canvas, True)
    level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
    canvas.Print(f"{WORK}/titles.png")
    ROOT.gErrorIgnoreLevel = level
    image = ROOT.TASImage.Open(f"{WORK}/titles.png")
    W, H = image.GetWidth(), image.GetHeight()
    argb = np.frombuffer(image.GetArgbArray(), dtype=np.uint32, count=W * H).reshape(H, W)
    return (((argb >> 16) & 0xFF) < 100) & (((argb >> 8) & 0xFF) < 100) & ((argb & 0xFF) < 100), W, H


def runs(columns):
    """the runs of columns holding dark pixels: [(first, last), ...]"""
    found, start = [], None
    for k, dark in enumerate(columns):
        if dark and start is None:
            start = k
        elif not dark and start is not None:
            found.append((start, k - 1))
            start = None
    if start is not None:
        found.append((start, len(columns) - 1))
    return found


def gaps(name, group, scalePad, lowestPad=None):
    """the gap in pixels between the y labels and the y title, and between the labels and the title of the colour scale; in the upper half
    of the frame of pad 1 (the titles are aligned at its top), or down to the frame bottom of lowestPad (a title for several panels)"""
    dark, W, H = painted(name, group)
    pad = run.pad(name, group, 1)
    canvasX = lambda padObj, x: int(round((padObj.GetAbsXlowNDC() + x * padObj.GetAbsWNDC()) * W))  # noqa: E731
    frameBottom = lambda padObj: int(round((1.0 - padObj.GetAbsYlowNDC() - padObj.GetAbsHNDC() * padObj.GetBottomMargin()) * H))  # noqa: E731
    top = int(round((1.0 - pad.GetAbsYlowNDC() - pad.GetAbsHNDC() * (1.0 - pad.GetTopMargin())) * H))
    bottom = frameBottom(run.pad(name, group, lowestPad)) if lowestPad else (top + frameBottom(pad)) // 2
    left = runs(dark[top:bottom, canvasX(pad, 0.0) : canvasX(pad, pad.GetLeftMargin()) - 12].any(axis=0))  # title, labels (left of the ticks)
    yGap = left[1][0] - left[0][1] - 1 if len(left) >= 2 else None  # the title is the outermost run, the labels (digit by digit) follow
    scale = run.pad(name, group, scalePad)
    frame = scale.GetPrimitive(f"axis_hist_pad_{scalePad}") or next((o for o in scale.GetListOfPrimitives() if o.InheritsFrom("TH1")), None)
    palette = frame.GetListOfFunctions().FindObject("palette") if frame else None
    if not palette:
        palette = next((o for o in scale.GetListOfPrimitives() if o.InheritsFrom("TPaletteAxis")), None)
    if not palette:
        return yGap, None
    right = runs(dark[top:bottom, canvasX(scale, palette.GetX2NDC()) + 12 : canvasX(scale, 1.0)].any(axis=0))  # labels, title (right of the ticks)
    zGap = right[-1][0] - right[-2][1] - 1 if len(right) >= 2 else None  # the title is the outermost run
    return yGap, zGap


_, refGap = gaps("reference", "named", 1)
check("the reference: a gap between the labels and the title of the colour scale", refGap is not None and refGap > 5, f"{refGap}")
for i, case in enumerate(todo):
    what = describe(case)
    if not run.canvas(f"t{i}", "titles"):
        check(f"{what}: drawn", False, " | ".join(run.messages(f"t{i}", "titles")))
        continue
    _, zGap = gaps(f"t{i}", "titles", 2 if case["where"] == "cell" else 1)
    check(f"{what}: the title of the colour scale as far from its widest label as with short labels", zGap is not None and refGap is not None and abs(zGap - refGap) <= 2, f"gap {zGap}, with short labels {refGap} pixels")
yRef, _ = gaps("y_short", "named", 1, 2)
for label, yRange in LABELS.items():
    if yRange:
        yGap, _ = gaps(f"y_{label}", "named", 1, 2)
        check(f"one y title for panels with {label} labels: as far from the widest label as with short labels", yGap is not None and yRef is not None and abs(yGap - yRef) <= 2, f"gap {yGap}, with short labels {yRef} pixels")

frame = run.frame_hist("offset", "named")
palette = frame.GetListOfFunctions().FindObject("palette") if frame else None
check("an offset set for the title of the colour scale is kept", palette is not None and close(palette.GetAxis().GetTitleOffset(), 1.8, 1e-6), f"{palette.GetAxis().GetTitleOffset() if palette else None}")
frame = run.frame_hist("view", "named")
check(
    "a 3d view: the x and y titles centred at offset 1.5",
    frame is not None and all(close(a.GetTitleOffset(), 1.5, 1e-6) and a.GetCenterTitle() for a in (frame.GetXaxis(), frame.GetYaxis())),
    f"{[(a.GetTitleOffset(), a.GetCenterTitle()) for a in (frame.GetXaxis(), frame.GetYaxis())] if frame else None}",
)
frame = run.frame_hist("view_offset", "named")
check("a 3d view: an offset set wins", frame is not None and close(frame.GetXaxis().GetTitleOffset(), 2.5, 1e-6), f"{frame.GetXaxis().GetTitleOffset() if frame else None}")
finish()
