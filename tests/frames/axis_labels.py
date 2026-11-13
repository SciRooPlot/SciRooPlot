"""Axis labels as painted:
- with Axis::SetHideClippedLabels (on in the base plots) no label is drawn cut at the edge of its pad: the labels at the edge shared by
  touching panels (the y labels of a ratio pad and its main pad, the x labels of panels side by side, a log decade) are left out, a label
  that fits into the margin of its pad stays; with the flag off ROOT draws them cut
- the primary divisions ROOT paints on an x axis are the ones set, also in narrow panels (THistPainter scales them with the pad width
  where the setting has a max digits part; SciRooPlot raises them to give back the setting), in a side panel drawn with hbars and with
  divisions set by the user; single pads and 3d views keep the setting
- Axis::SetLabelAngle turns the alphanumeric labels of an x axis by that angle as painted (ROOT's orientations or one per label); numeric
  axes, y axes and the labels of hbars warn and are left as they are; Data::SetTextAngle turns the values written by TEXT (0 to 90, with
  a warning beyond) and the labels of points_text, and warns for data that writes no values
Cases: a ratio layout (linear and log main pad) and panels side by side with labels at the shared edges × the flag on or off; layouts of
one, two and three panels, an hbars side panel, 3d views × the divisions set or not; every angle of ROOT's orientations and others.
Reference: the pixels of each pad painted alone at its edges; ROOT's scaling of the divisions; ROOT's angles of its label orientations.
Covers: labels cut at the edge, divisions of narrow panels, label and text angles, labels measured as written (the exponent taken out, log
decades as 10^{n}).
"""

import numpy as np

from srptest import ROOT, WORK, Project, S, cases, check, describe, finish

p = Project("axis_labels", bases=("1d", "1d_ratio", "1d_2x1", "1d_3x1", "2d", "2d_2x1"))
h = ROOT.TH1D("h", ";;", 50, 0.0, 10.0)
for b in range(1, 51):
    h.SetBinContent(b, 2500.0 + 30.0 * b)
    h.SetBinError(b, 10.0)
lab = ROOT.TH1D("lab", "", 4, 0.0, 4.0)
for b, name in enumerate(["a", "bb", "ccc", "dddd"], 1):
    lab.GetXaxis().SetBinLabel(b, name)
    lab.SetBinContent(b, b)
m = ROOT.TH2D("m", "", 10, -5.0, 5.0, 10, -5.0, 5.0)
for b in range(1, 11):
    for c in range(1, 11):
        m.SetBinContent(b, c, b + c)
# x ranges whose numbers are long (the shipped look writes x labels without exponent; four digits at ROOT's 510 divisions already touch),
# and short labels whose 0 the steps miss by rounding (-1 + 5 * 0.2, written as 0): (low, high, exponent wanted, labels run into each other)
WIDE = {
    "wide_1e12": (0.0, 1e12, False, True),
    "wide_ns": (0.0, 1e-9, False, True),
    "wide_offset": (1e6, 1e6 + 10.0, False, True),
    "wide_5000": (0.0, 5000.0, False, True),
    "wide_100": (0.0, 100.0, False, False),
    "signed_steps": (-1.0, 0.2, False, False),
    "wide_exponent": (0.0, 1e12, True, False),
    "time_day": (0.0, 86400.0, False, False),
}
# labels measured as TGaxis writes them (the exponent taken out, log decades as 10^{n}), not as the numbers they stand for: none of them
# is hidden as clipped or said to run into others: (low, high, exponent wanted, log x)
WRITTEN = {"exponent_2e6": (0.0, 2e6, True, False), "log_decades": (1.0, 1e6, False, True)}
wide = {}
for name, (low, high, _, _) in WIDE.items():
    wide[name] = ROOT.TH1D(name, "", 20, low, high)
    for b in range(1, 21):
        wide[name].SetBinContent(b, b)
for name, (low, high, _, log) in WRITTEN.items():
    edges = np.geomspace(low, high, 21) if log else np.linspace(low, high, 21)
    wide[name] = ROOT.TH1D(name, "", 20, edges)
    for b in range(1, 21):
        wide[name].SetBinContent(b, b)
wide["time_day"].GetXaxis().SetTimeDisplay(1)  # a time axis stored with the input: hourly labels, no power of ten to advise
wide["time_day"].GetXaxis().SetTimeFormat("%H:%M")
p.source("s", {"h": h, "lab": lab, "m": m, **wide})

# ---- labels at the edges of touching panels
CLIP = {  # layout: (base plot, the edges checked: (pad, side, whether ROOT draws a label there))
    "ratio": ("1d_ratio", [(1, "bottom", True), (2, "top", True)]),
    "ratio_log": ("1d_ratio", [(1, "bottom", False), (2, "top", True)]),  # ROOT writes no decade at the lower end of this log axis
    "side_by_side": ("1d_2x1", [(1, "right", True), (2, "left", True)]),
}
clipCases = cases({"layout": list(CLIP), "flag": ["on", "off"]}, always=lambda c: True)
for i, case in enumerate(clipCases):
    base, _ = CLIP[case["layout"]]
    plot = S.Plot(f"clip{i}", "clip", base)
    if base == "1d_ratio":
        plot[1].AddData("h", "s")
        plot[2].AddRatio(["h", "s"], ["h", "s"]).Numer().Scale(1.05)
        plot[1]["Y"].SetRange(*((10.0, 10000.0) if case["layout"] == "ratio_log" else (500.0, 4500.0)))  # a label at the bottom edge, one in the top margin
        plot[2]["Y"].SetRange(0.9, 1.15)  # labels at both edges
        plot[2]["Y"].SetTitle("")
        if case["layout"] == "ratio_log":
            plot[1]["Y"].SetLog()
        axes = [(1, "Y"), (2, "Y")]
    else:
        for pad in (1, 2):
            plot[pad].AddData("h", "s")
            plot[pad]["X"].SetRange(0.0, 10.0)  # labels at both ends
        axes = [(1, "X"), (2, "X")]
    if case["flag"] == "off":
        for pad, axis in axes:
            plot[pad][axis].SetHideClippedLabels(False)
    p.pm.AddPlot(plot)

# ---- primary divisions of x axes
DIVISIONS = {  # name: (base plot or layout, pads with the expected primary divisions painted, settings)
    "single": ("1d", {1: 10}),
    "two": ("1d_2x1", {1: 10, 2: 10}),
    "three_505": ("1d_3x1", {1: 5, 2: 5, 3: 5}),
    "hbars_side": ("side", {2: 10}),
    "hbars_side_503": ("side", {2: 3}),
    "legos": ("2d_2x1", {1: None, 2: None}),  # 3d views: the setting as it is
}
for name, (base, _) in DIVISIONS.items():
    if base == "side":
        plot = S.Plot(name, S.PanelLayout([[S.Panel(), S.Panel(0.25, 1.0)]]))
        plot.SetGroup("divisions")
        plot[1].AddData("m", "s").SetDrawOption(S.col)
        plot[2].AddData("h", "s").SetDrawOption(S.hbars)
        if name.endswith("503"):
            plot[2]["X"].SetNumDivisions(503)
    else:
        plot = S.Plot(name, "divisions", base)
        for pad in DIVISIONS[name][1]:
            plot[pad].AddData("m" if base == "2d_2x1" else "h", "s").SetDrawOption(S.lego if base == "2d_2x1" else S.points)
        if name == "three_505":
            plot[0]["X"].SetNumDivisions(505)
    p.pm.AddPlot(plot)

# ---- label and text angles
ANGLES = {0.0: 0.0, 90.0: 90.0, 20.0: 20.0, -20.0: 340.0, 45.0: 45.0, -45.0: 315.0, 70.0: 70.0}  # angle set: angle painted
for k, angle in enumerate(ANGLES):
    plot = S.Plot(f"angle{k}", "angles", "1d")
    plot[1].AddData("lab", "s").SetDrawOption(S.bars)
    plot[1]["X"].SetLabelAngle(angle)
    p.pm.AddPlot(plot)
for name, setup in (("numeric", lambda pl: (pl[1].AddData("h", "s"), pl[1]["X"].SetLabelAngle(45.0))), ("y_axis", lambda pl: (pl[1].AddData("lab", "s"), pl[1]["Y"].SetLabelAngle(45.0))), ("hbars", lambda pl: (pl[1].AddData("lab", "s").SetDrawOption(S.hbars), pl[1]["X"].SetLabelAngle(-45.0)))):
    plot = S.Plot(name, "angles_left", "1d")
    setup(plot)
    p.pm.AddPlot(plot)
TEXTS = {  # name: (data, option, angle set, option part expected)
    "text_2d": ("m", S.text, 45.0, "TEXT45"),
    "text_1d": ("lab", S.text, 30.0, "TEXT30"),
    "text_beyond": ("m", S.text, 120.0, "TEXT90"),
    "text_raw": ("m", "TEXT60", 30.0, "TEXT60"),
}
for name, (data, option, angle, _) in TEXTS.items():
    plot = S.Plot(name, "texts", "2d" if data == "m" else "1d")
    plot[1].AddData(data, "s").SetDrawOption(option).SetTextAngle(angle)
    p.pm.AddPlot(plot)
plot = S.Plot("points_text", "texts", "1d")
plot[1].AddData("lab", "s").SetDrawOption(S.points_text).SetTextAngle(90.0)
p.pm.AddPlot(plot)
plot = S.Plot("no_text", "texts", "1d")
plot[1].AddData("lab", "s").SetTextAngle(30.0)
p.pm.AddPlot(plot)
for name, (_, _, exponent, _) in WIDE.items():
    plot = S.Plot(name, "wide", "1d")
    plot[1].AddData(name, "s")
    if exponent:
        plot[1]["X"].SetNoExponent(False)
    p.pm.AddPlot(plot)
for name, (_, _, exponent, log) in WRITTEN.items():
    plot = S.Plot(name, "written", "1d")
    plot[1].AddData(name, "s")
    plot[1]["X"].SetNoExponent(not exponent)
    plot[1]["X"].SetLog(log)
    p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))
for name, (low, high, exponent, overlap) in WIDE.items():
    said = [m for m in run.messages(name, "wide") if "run into each other" in m]
    check(f"x labels from {low:g} to {high:g}{' with exponent' if exponent else ''}: {'warned once that they run into each other' if overlap else 'no warning'}", len(said) == (1 if overlap else 0), " | ".join(said))
for name, (low, high, exponent, log) in WRITTEN.items():
    padObj = run.pad(name, "written")
    axis = padObj.GetPrimitive("axis_hist_pad_1").GetXaxis() if padObj else None
    hidden = [(m.GetLabNum(), m.GetLabValue()) for m in axis.GetModifiedLabels()] if axis and axis.GetModifiedLabels() else []
    said = [m for m in run.messages(name, "written") if "run into each other" in m]
    what = f"x labels from {low:g} to {high:g}{' with exponent' if exponent else ''}{' on a log axis' if log else ''}"
    check(f"{what}: measured as written, none hidden as clipped, no warning", axis is not None and not hidden and not said, f"hidden {hidden}; {' | '.join(said)}")


def edge_pixels(name, group, padID, side):
    """the dark pixels of the labels of a pad painted alone, near one of its edges (closer than a third of the label size): a label
    centred at the edge leaves a cut glyph there, the next label is far from it"""
    canvas = run.canvas(name, group)
    ROOT.SetOwnership(canvas, True)
    for obj in canvas.GetListOfPrimitives():
        if obj.InheritsFrom("TPad") and obj.GetName() != f"Pad_{padID}":
            obj.GetListOfPrimitives().Clear()
    pad = canvas.GetPrimitive(f"Pad_{padID}")
    level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
    canvas.Print(f"{WORK}/edge.png")
    ROOT.gErrorIgnoreLevel = level
    image = ROOT.TASImage.Open(f"{WORK}/edge.png")
    W, H = image.GetWidth(), image.GetHeight()
    argb = np.frombuffer(image.GetArgbArray(), dtype=np.uint32, count=W * H).reshape(H, W)
    dark = (((argb >> 16) & 0xFF) < 128) & (((argb >> 8) & 0xFF) < 128) & ((argb & 0xFF) < 128)
    x0, x1 = int(round(pad.GetAbsXlowNDC() * W)), int(round((pad.GetAbsXlowNDC() + pad.GetAbsWNDC()) * W)) - 1
    y0, y1 = int(round((1.0 - pad.GetAbsYlowNDC() - pad.GetAbsHNDC()) * H)), int(round((1.0 - pad.GetAbsYlowNDC()) * H)) - 1
    frameLeft = x0 + int(pad.GetLeftMargin() * (x1 - x0)) - 8  # left of the ticks: where the y labels are
    frameBottom = y1 - int(pad.GetBottomMargin() * (y1 - y0)) + 8  # below the ticks: where the x labels are
    axis = pad.GetPrimitive(f"axis_hist_pad_{padID}")
    size = (axis.GetXaxis() if side in ("left", "right") else axis.GetYaxis()).GetLabelSize() * min(x1 - x0, y1 - y0)
    n = max(3, int(0.3 * size))
    band = {
        "top": dark[y0 : y0 + n, x0:frameLeft],
        "bottom": dark[y1 - n + 1 : y1 + 1, x0:frameLeft],
        "left": dark[frameBottom : y1 + 1, x0 : x0 + n],
        "right": dark[frameBottom : y1 + 1, x1 - n + 1 : x1 + 1],
    }[side]
    return int(band.sum())


for i, case in enumerate(clipCases):
    _, edges = CLIP[case["layout"]]
    if not run.canvas(f"clip{i}", "clip"):
        check(f"{describe(case)}: the plot is made", False)
        continue
    for padID, side, labelThere in edges:
        found = edge_pixels(f"clip{i}", "clip", padID, side)
        if case["flag"] == "on":
            check(f"{describe(case)}: no label is cut at the {side} edge of pad {padID}", found == 0, f"{found} pixels of a label at the edge")
        elif labelThere:
            check(f"{describe(case)}: the reference sees the label cut at the {side} edge of pad {padID} (ROOT without the flag)", found > 0, "no pixels at the edge")
for case in clipCases:
    if case["layout"] == "ratio" and case["flag"] == "on":
        i = clipCases.index(case)
        padObj = run.pad(f"clip{i}", "clip", 1)
        mods = padObj.GetPrimitive("axis_hist_pad_1").GetYaxis().GetModifiedLabels() if padObj else None
        check("ratio, main pad: the label in its top margin is kept (one label left out, at the bottom)", mods is not None and mods.GetSize() == 1, f"{[(m.GetLabNum(), m.GetLabValue()) for m in mods] if mods else mods}")


def horizontal_axis(name, padID):
    """the divisions setting of the axis painted horizontally (the histogram's y axis with HBAR) and the width of the pad"""
    padObj = run.pad(name, "divisions", padID)
    frame = padObj.GetPrimitive(f"axis_hist_pad_{padID}") if padObj else None
    if not frame:
        return None, None
    hbar = any(obj.GetName() == frame.GetName() and "HBAR" in opt.upper() for obj, opt in run.links(name, "divisions", padID))
    return (frame.GetYaxis() if hbar else frame.GetXaxis()).GetNdivisions(), padObj.GetAbsWNDC()


for name, (base, pads) in DIVISIONS.items():
    for padID, want in pads.items():
        nDiv, width = horizontal_axis(name, padID)
        if nDiv is None:
            check(f"{name} pad {padID}: drawn", False)
            continue
        if want is None:
            check(f"{name} pad {padID}: a 3d view keeps the setting", nDiv % 1000000 == 510, f"setting {nDiv}")
            continue
        painted = int(float(nDiv % 100) * width) if nDiv > 1000 else nDiv % 100  # THistPainter::PaintAxis
        check(f"{name} pad {padID}: {want} primary divisions painted", painted == want, f"setting {nDiv}, pad width {width:.3f}, painted {painted}")

for k, (angle, want) in enumerate(ANGLES.items()):
    padObj = run.pad(f"angle{k}", "angles")
    axis = padObj.GetPrimitive("axis_hist_pad_1").GetXaxis() if padObj else None
    if not axis:
        check(f"label angle {angle}: drawn", False)
        continue
    bits = {"kLabelsHori": 0.0, "kLabelsVert": 90.0, "kLabelsUp": 20.0, "kLabelsDown": 340.0}  # ROOT's orientations as TGaxis paints them
    fromBits = [a for b, a in bits.items() if axis.TestBit(getattr(ROOT.TAxis, b))]
    mods = axis.GetModifiedLabels()
    perLabel = sorted({round(mm.GetAngle() % 360.0, 6) for mm in mods}) if mods and mods.GetSize() else []
    painted = perLabel if perLabel else fromBits
    every = not perLabel or mods.GetSize() == lab.GetNbinsX()
    check(f"label angle {angle}: every label painted at {want}", painted == [want] and every, f"orientation bits {fromBits}, per label {perLabel}")
warnings = run.warnings()
for name, axis in (("numeric", "X"), ("y_axis", "Y"), ("hbars", "X")):
    check(f"label angle on {name}: warned, the labels left as they are", any(f"label angle of the {axis} axis" in w for w in run.messages(name, "angles_left")), " | ".join(run.messages(name, "angles_left")))
padObj = run.pad("hbars", "angles_left")
axis = padObj.GetPrimitive("axis_hist_pad_1").GetXaxis() if padObj else None
check("label angle on hbars: no label turned", axis is not None and not axis.GetModifiedLabels())

for name, (_, _, angle, want) in TEXTS.items():
    options = [option.upper() for obj, option in run.links(name, "texts") if obj.GetName().startswith("1:")]
    check(f"text angle {angle} ({name}): {want} in the drawing option", len(options) == 1 and want in options[0] and options[0].count("TEXT") == 1, f"{options}")
check("a text angle beyond 90: one warning", sum("text angle 120" in w for w in warnings) == 1, " | ".join(w for w in warnings if "text angle" in w))
padObj = run.pad("points_text", "texts")
latex = [t for t in padObj.GetListOfPrimitives() if t.IsA() == ROOT.TLatex.Class()] if padObj else []
check("points_text: the values turned by the text angle", latex and {t.GetTextAngle() for t in latex} == {90.0} and {t.GetTextAlign() for t in latex} == {12})
check("a text angle on data that writes no values: one warning", sum("has no effect" in w for w in run.messages("no_text", "texts")) == 1, " | ".join(run.messages("no_text", "texts")))
finish()
