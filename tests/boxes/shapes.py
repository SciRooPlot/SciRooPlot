"""Shapes (lines, arrows, vertical and horizontal lines, regions, ellipses, polygons) are drawn as ROOT's TLine, TArrow, TBox, TEllipse and
TPolyLine in their layer: behind the data (right after the frame), after the data (the default) or on top of the boxes. They are not data:
they leave the frame, the numbering, the default colours and the legend of the data as they are. Lines and regions over a whole axis span
the final frame, also a log one; regions and polygons are cut at the frame; a shape without its default look gets ROOT's line in black,
regions and polygons a translucent fill, ellipses no fill, arrows a head in the colour of their line; set styles are drawn as set.
Cases: every kind × every layer × a histogram, a graph, a map and a log axis giving the frame × the default or a set look, and the named
cases (lines at the limits of a log axis, positions a log axis cannot show, shapes of a base plot and of pad 0, arrow heads, arcs).
Reference: the primitives of the pad and the same plot without the shape.
Covers: the commit adding shapes (backlog 28 of the old harness), f49b65a (a line at the limit of a log axis), and the named cases.
"""

import math

from srptest import ROOT, Project, S, cases, check, close, describe, finish

KINDS = {  # kind: drawn class
    "line": "TLine",
    "arrow": "TArrow",
    "vertical_line": "TLine",
    "horizontal_line": "TLine",
    "region": "TBox",
    "region_x": "TBox",
    "region_y": "TBox",
    "ellipse": "TEllipse",
    "polygon": "TPolyLine",
}
LAYERS = ("behind", "default", "top")
FRAMES = ("hist", "graph", "map", "log")


def add_shape(pad, kind, frame):
    """the shape, inside the frame of every frame kind (x 2 to 8, y 2 to 8; a map: 0.2 to 0.8)"""
    s = 0.1 if frame == "map" else 1.0
    return {
        "line": lambda: pad.AddLine((2 * s, 3 * s), (7 * s, 6 * s)),
        "arrow": lambda: pad.AddArrow((6 * s, 7 * s), (4 * s, 5 * s)),
        "vertical_line": lambda: pad.AddVerticalLine(5 * s),
        "horizontal_line": lambda: pad.AddHorizontalLine(4 * s),
        "region": lambda: pad.AddRegion((-5 * s, 3 * s), (3 * s, 50 * s)),  # beyond the frame on the left and on top: cut
        "region_x": lambda: pad.AddRegionX(6 * s, 7 * s),
        "region_y": lambda: pad.AddRegionY(2 * s, 2.5 * s),
        "ellipse": lambda: pad.AddEllipse((5 * s, 5 * s), 1 * s, 0.5 * s),
        "polygon": lambda: pad.AddPolygon([(6 * s, 2 * s), (12 * s, 3 * s), (7 * s, 4 * s)]),  # beyond the frame on the right: cut
    }[kind]()


todo = cases({"kind": list(KINDS), "layer": list(LAYERS), "frame": list(FRAMES), "look": ["default", "set"]})

p = Project("shapes")
h = ROOT.TH1D("h", "", 10, 0.0, 10.0)
m = ROOT.TH2D("m", "", 10, 0.0, 1.0, 10, 0.0, 1.0)
g = ROOT.TGraphErrors(9)
for b in range(1, 11):
    h.SetBinContent(b, b)
    h.SetBinError(b, 0.1)
    for c in range(1, 11):
        m.SetBinContent(b, c, b + c)
for k in range(9):
    g.SetPoint(k, k + 1.0, k + 1.0)
    g.SetPointError(k, 0.0, 0.1)
p.source("s", {"h": h, "m": m, "g": g})


def frame_plot(name, group, frame):
    plot = S.Plot(name, group, "2d" if frame == "map" else "1d")
    pad = plot[1]
    pad.AddData({"hist": "h", "log": "h", "graph": "g", "map": "m"}[frame], "s", "data")
    if frame == "map":
        pad["X"].SetRange(0.1, 0.9)
        pad["Y"].SetRange(0.1, 0.9)
    else:
        pad["X"].SetRange(1.0, 9.0)
        pad["Y"].SetRange(1.0, 9.0)
    if frame == "log":
        pad["Y"].SetLog()
    pad.AddLegend(0.05, 0.95).SetPadCoordinates()
    return plot


for frame in FRAMES:
    p.pm.AddPlot(frame_plot(f"plain_{frame}", "shapes", frame))
for i, case in enumerate(todo):
    plot = frame_plot(f"s{i}", "shapes", case["frame"])
    shape = add_shape(plot[1], case["kind"], case["frame"])
    if case["layer"] == "behind":
        shape.SetBehindData()
    elif case["layer"] == "top":
        shape.SetOnTop()
    if case["look"] == "set":
        shape.SetLine(ROOT.kDashed, 3.0, ROOT.kRed + 1).SetFill(3004, ROOT.kBlue + 1)
    p.pm.AddPlot(plot)

# named cases
plot = S.Plot("log_limits", "named", "1d")
plot[1].AddData("h", "s")
plot[1]["Y"].SetLog()
plot[1]["Y"].SetRange(0.2, 200.0)
plot[1].AddHorizontalLine(0.2)
plot[1].AddHorizontalLine(200.0)
p.pm.AddPlot(plot)
plot = S.Plot("log_not_positive", "named", "1d")
plot[1].AddData("h", "s")
plot[1]["Y"].SetLog()
plot[1].AddRegion((2.0, -5.0), (3.0, 5.0))  # below zero: starts at the frame
plot[1].AddHorizontalLine(-1.0)  # cannot be shown: left out, warned
plot[1].AddPolygon([(4.0, 0.01), (6.0, 0.01), (5.0, 5.0)])  # below the frame: cut at its lower edge
plot[1].AddPolygon([(7.0, -1.0), (8.0, -1.0), (7.5, 5.0)])  # a point the axis cannot show: left out, warned
p.pm.AddPlot(plot)
base = S.PlotManager.MakeBasePlot("1d")
base.SetName("1d_with_shape")
base[1].AddHorizontalLine(5.0)
p.pm.AddBasePlot(base)
plot = S.Plot("from_base", "named", "1d_with_shape")
plot[1].AddData("h", "s")
p.pm.AddPlot(plot)
plot = S.Plot("pad0", "named", "1d")
plot[1].AddData("h", "s")
plot[0].AddVerticalLine(5.0)
p.pm.AddPlot(plot)
plot = S.Plot("styles", "named", "1d")
plot[1].AddData("h", "s")
plot[1].AddArrow((2.0, 2.0), (4.0, 4.0)).SetArrowHead("<|>").SetArrowSize(0.05).SetLine(ROOT.kSolid, 3.0, ROOT.kBlue + 1)
plot[1].AddEllipse((5.0, 5.0), 1.0, 2.0).SetArc(0.0, 180.0).SetRotation(30.0)
plot[1].AddRegionX(6.0, 7.0).SetLine(ROOT.kDashed, 1.0, ROOT.kBlack)
p.pm.AddPlot(plot)
plot = S.Plot("polygon_too_few", "named", "1d")
plot[1].AddData("h", "s")
plot[1].AddPolygon([(2.0, 2.0), (3.0, 3.0)])
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))
SHAPE_CLASSES = ("TLine", "TArrow", "TBox", "TEllipse", "TPolyLine")


def shapes_of(prims, plain):
    """the shapes among the primitives of a pad: not the lines of the frame of a map, which the plain plot has too"""

    def key(o):
        return (o.ClassName(), *(round(getattr(o, f)(), 9) for f in ("GetX1", "GetY1", "GetX2", "GetY2"))) if o.ClassName() == "TLine" else None

    frameLines = {key(o) for o in plain if key(o)}
    return [k for k, o in enumerate(prims) if o.ClassName() in SHAPE_CLASSES and key(o) not in frameLines]


for i, case in enumerate(todo):
    name, kind, frame = f"s{i}", case["kind"], case["frame"]
    what = describe(case)
    padObj = run.pad(name, "shapes")
    plain = run.pad(f"plain_{frame}", "shapes")
    if not padObj or not plain:
        check(f"{what}: the plot is made", False, " | ".join(run.messages(name, "shapes")))
        continue
    prims = list(padObj.GetListOfPrimitives())
    found = shapes_of(prims, list(plain.GetListOfPrimitives()))
    cls = KINDS[kind]
    drawn = [prims[k] for k in found]
    check(f"{what}: drawn as one {cls}", [o.ClassName() for o in drawn] == [cls], f"{[o.ClassName() for o in drawn]}")
    if len(drawn) != 1:
        continue
    shape, where = drawn[0], found[0]
    names = [o.GetName() for o in prims]
    frameHist = names.index("axis_hist_pad_1") if "axis_hist_pad_1" in names else -1
    data = [k for k, n in enumerate(names) if n[:1].isdigit()]
    boxes = [k for k, o in enumerate(prims) if o.InheritsFrom("TLegend")]
    layer = case["layer"]
    if layer == "behind":
        ok = frameHist < where < min(data)
    elif layer == "default":
        ok = max(data) < where < min(boxes)
    else:
        ok = where > max(boxes)
    check(f"{what}: drawn in its layer", ok and data and boxes, f"order {[(k, o.ClassName(), o.GetName()) for k, o in enumerate(prims)]}")
    frameOf = lambda q: (q.GetUxmin(), q.GetUxmax(), q.GetUymin(), q.GetUymax())  # noqa: E731
    check(f"{what}: the frame is that of the plot without the shape", all(close(a, b) for a, b in zip(frameOf(padObj), frameOf(plain))), f"{frameOf(padObj)} vs {frameOf(plain)}")
    dataHere, dataPlain = [prims[k] for k in data], [o for o in plain.GetListOfPrimitives() if o.GetName()[:1].isdigit()]
    same = len(dataHere) == 1 and dataHere[0].GetName().startswith("1:") and dataHere[0].GetLineColor() == dataPlain[0].GetLineColor()
    check(f"{what}: the data keeps its number and colour", same, f"{[o.GetName() for o in dataHere]}")
    legend = prims[boxes[0]] if boxes else None
    check(f"{what}: the legend has only the row of the data", legend is not None and legend.GetListOfPrimitives().GetSize() == 1)
    # spanning the final frame (pad coordinates: log10 on a log axis), cut at it
    x1, x2, y1, y2 = frameOf(padObj)
    toPad = lambda v, logAxis: math.log10(v) if logAxis and v > 0 else v  # noqa: E731
    logY = padObj.GetLogy()
    if kind in ("vertical_line", "region_x"):
        check(f"{what}: over the height of the frame", close(toPad(shape.GetY1(), logY), y1, 1e-6) and close(toPad(shape.GetY2(), logY), y2, 1e-6), f"{shape.GetY1()}, {shape.GetY2()} vs {y1}, {y2}")
    if kind in ("horizontal_line", "region_y"):
        check(f"{what}: over the width of the frame", close(shape.GetX1(), x1, 1e-6) and close(shape.GetX2(), x2, 1e-6), f"{shape.GetX1()}, {shape.GetX2()} vs {x1}, {x2}")
    if kind == "region":
        check(f"{what}: cut at the frame", close(shape.GetX1(), x1, 1e-6) and close(toPad(shape.GetY2(), logY), y2, 1e-6), f"x1 {shape.GetX1()} y2 {shape.GetY2()} vs {x1}, {y2}")
    if kind == "polygon":
        xs = [shape.GetX()[k] for k in range(shape.GetN())]
        check(f"{what}: cut at the frame and closed", max(xs) <= x2 + 1e-9 and any(close(x, x2, 1e-6) for x in xs) and xs[0] == xs[-1], f"{xs} frame up to {x2}")
    # the look
    if case["look"] == "set":
        line = (shape.GetLineStyle(), shape.GetLineWidth(), shape.GetLineColor()) == (ROOT.kDashed, 3, ROOT.kRed + 1)
        filled = kind in ("region", "region_x", "region_y", "polygon", "ellipse")
        fill = not filled or (shape.GetFillStyle(), shape.GetFillColor()) == (3004, ROOT.kBlue + 1)
        check(f"{what}: drawn with the line and fill set", line and fill, f"line {(shape.GetLineStyle(), shape.GetLineWidth(), shape.GetLineColor())}" + (f" fill {(shape.GetFillStyle(), shape.GetFillColor())}" if filled else ""))
    elif kind in ("region", "region_x", "region_y", "polygon"):
        alpha = ROOT.gROOT.GetColor(shape.GetFillColor()).GetAlpha()
        check(f"{what}: a translucent fill", shape.GetFillStyle() == 1001 and alpha < 0.5, f"style {shape.GetFillStyle()} alpha {alpha}")
    elif kind == "ellipse":
        check(f"{what}: open, a black line", shape.GetFillStyle() == 0 and shape.GetLineColor() == ROOT.kBlack, f"fill {shape.GetFillStyle()} line {shape.GetLineColor()}")
    elif kind == "arrow":
        check(f"{what}: a black line with a head in its colour", shape.GetLineColor() == ROOT.kBlack and shape.GetFillColor() == shape.GetLineColor() and shape.GetOption() == "|>", f"{shape.GetLineColor()} {shape.GetFillColor()} {shape.GetOption()!r}")
    else:
        check(f"{what}: a black line", shape.GetLineColor() == ROOT.kBlack, f"{shape.GetLineColor()}")

prims = lambda name: list(run.pad(name, "named").GetListOfPrimitives()) if run.pad(name, "named") else []  # noqa: E731
of = lambda name, cls: [o for o in prims(name) if o.ClassName() == cls]  # noqa: E731
padObj = run.pad("log_limits", "named")
lines = of("log_limits", "TLine")
check("lines at both limits of a log axis are drawn", len(lines) == 2 and not any("outside of the frame" in m for m in run.messages("log_limits", "named")), f"{len(lines)} lines")
padObj = run.pad("log_not_positive", "named")
regions, polys = of("log_not_positive", "TBox"), of("log_not_positive", "TPolyLine")
y1 = 10 ** padObj.GetUymin() if padObj else None
check("log y: a region below zero starts at the frame", len(regions) == 1 and close(regions[0].GetY1(), y1, 1e-6) and close(regions[0].GetY2(), 5.0), f"{[(b.GetY1(), b.GetY2()) for b in regions]} frame from {y1}")
check("log y: a polygon below zero is cut at the lower edge of the frame", len(polys) == 1 and close(min(polys[0].GetY()[k] for k in range(polys[0].GetN())), y1, 1e-6))
messages = run.messages("log_not_positive", "named")
check("log y: a line the axis cannot show is left out, warned", not of("log_not_positive", "TLine") and any("Shape 2 (horizontal_line) of pad 1 is not drawn" in m for m in messages), " | ".join(messages))
check("log y: a polygon with a point the axis cannot show is left out, warned", any("Shape 4 (polygon) of pad 1 is not drawn: a point is not positive" in m for m in messages), " | ".join(messages))
check("a shape of the base plot is drawn", len([o for o in of("from_base", "TLine") if close(o.GetY1(), 5.0)]) == 1)
check("shapes of pad 0 are not drawn, warned", not of("pad0", "TLine") and any("shapes of pad 0 are not drawn" in m for m in run.warnings()), " | ".join(run.warnings()))
arrows, ellipses, boxes = of("styles", "TArrow"), of("styles", "TEllipse"), of("styles", "TBox")
check("an arrow with its head, size and line", arrows and arrows[0].GetOption() == "<|>" and close(arrows[0].GetArrowSize(), 0.05, 1e-6) and arrows[0].GetLineColor() == ROOT.kBlue + 1 and arrows[0].GetLineWidth() == 3)
check("an ellipse with its arc and rotation", ellipses and (ellipses[0].GetPhimin(), ellipses[0].GetPhimax(), ellipses[0].GetTheta()) == (0.0, 180.0, 30.0))
lnk = run.pad("styles", "named").GetListOfPrimitives().FirstLink() if run.pad("styles", "named") else None
options = []
while lnk:
    if lnk.GetObject().ClassName() == "TBox":
        options.append(str(lnk.GetOption()))
    lnk = lnk.Next()
check("a region with a line set gets its outline", boxes and boxes[0].GetLineStyle() == ROOT.kDashed and options == ["l"], f"{options}")
check("a polygon of fewer than three points is not drawn, reported", not of("polygon_too_few", "TPolyLine") and any("polygon" in m.lower() and "point" in m for m in run.lines("WARN") + run.lines("ERR")), " | ".join(run.lines("WARN")))
finish()
