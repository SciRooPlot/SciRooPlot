"""Boxes (legends, texts, statistics boxes, images) and shapes (lines, arrows, vertical and horizontal lines, regions, ellipses,
polygons) are drawn at the position given in their coordinate system: fractions of the pad (SetPadCoordinates, the default of boxes),
fractions of the frame (SetFrameCoordinates: (0, 0) its lower left, (1, 1) its upper right corner; in a 3d view the pad within its
margins) or axis values (SetUserCoordinates, the default of shapes); a pad sets them for all its boxes and shapes that set none
(SetDefaultBoxCoordinates, SetDefaultShapeCoordinates), plot[0] for all pads, a pad's own default before the one of plot[0]. In frame coordinates the size set for a box (SetSize, SetWidth)
and the radii of an ellipse are fractions of the frame too, so a box or shape sits at the same place relative to the frame in every
panel of a layout, whose pads differ in size; a region in frame or user coordinates is cut at the frame, one in pad coordinates is not.
Cases: a single panel, a grid of touching panels (pads with and without margins), the main and ratio pad of 1d_ratio, log axes, a 3d
view (boxes only); each coordinate system, the built-in defaults and the defaults of the pads.
Reference: the frame of each drawn pad (its axis range within the pad, the margins for a 3d view) and the coordinates of the drawn objects.
Covers: the commit of this file's first version (boxes in a multi-panel layout moved against the frame from pad to pad).
"""

import math

from srptest import ROOT, WORK, Project, S, cases, check, describe, finish

TOL = 1e-6

# given positions: fractions (pad and frame coordinates) and axis values (user coordinates, the pad ranges set below)
FRACTION = {
    "legend": (0.15, 0.85),
    "text": (0.45, 0.85, 0.3, 0.2),  # x, y, width, height
    "stats": (0.55, 0.45, 0.25, 0.15),
    "image": (0.2, 0.45, 0.15),  # x, y, width
    "line": (0.2, 0.2, 0.4, 0.3),
    "arrow": (0.3, 0.6, 0.4, 0.7),
    "vertical_line": (0.7,),
    "horizontal_line": (0.65,),
    "region": (0.1, 0.1, 0.2, 0.2),
    "region_x": (0.8, 0.9),
    "region_y": (0.3, 0.35),
    "ellipse": (0.5, 0.5, 0.1, 0.05),
    "polygon": (0.6, 0.1, 0.7, 0.1, 0.65, 0.2),
    "region_cut": (-0.1, 0.4, 0.15, 1.2),  # beyond the frame: cut in frame coordinates, kept in pad coordinates
}
VALUE = {  # x from 1 to 9, y from 1 to 100 (also on log axes)
    "legend": (2.0, 80.0),
    "text": (5.0, 80.0, 0.3, 0.2),
    "stats": (6.0, 40.0, 0.25, 0.15),
    "image": (2.0, 40.0, 0.15),
    "line": (2.0, 10.0, 4.0, 30.0),
    "arrow": (3.0, 50.0, 4.0, 60.0),
    "vertical_line": (7.0,),
    "horizontal_line": (65.0,),
    "region": (1.5, 10.0, 2.5, 20.0),
    "region_x": (8.0, 8.5),
    "region_y": (30.0, 35.0),
    "ellipse": (5.0, 50.0, 0.5, 5.0),
    "polygon": (6.0, 10.0, 7.0, 10.0, 6.5, 20.0),
    "region_cut": (0.5, 40.0, 2.0, 120.0),  # beyond the frame: cut
}
SHAPES = ("line", "arrow", "vertical_line", "horizontal_line", "region", "region_x", "region_y", "ellipse", "polygon", "region_cut")
LAYOUTS = ("single", "grid", "ratio", "log", "view")
SYSTEMS = ("default", "pad", "frame", "user", "pad_defaults")  # pad_defaults: plot[0] sets frame for boxes, pad for shapes; pad 2 its own user for boxes


def valid(case):
    return case["layout"] != "view" or case["system"] in ("pad", "frame", "pad_defaults")  # a 3d view has no axis values in the pad


todo = cases({"layout": LAYOUTS, "system": SYSTEMS}, valid, always=lambda case: True)

image = f"{WORK}/logo.png"
logo = ROOT.TCanvas("logo", "", 100, 60)
logo.SetFillColor(ROOT.kAzure + 1)
level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
logo.Print(image)
ROOT.gErrorIgnoreLevel = level

p = Project("coordinate_systems", bases=("1d", "1d_ratio", "2d"))
h1 = ROOT.TH1D("h1", "", 40, 1.0, 9.0)
h2 = ROOT.TH2D("h2", "", 10, 0.0, 1.0, 10, 0.0, 1.0)
for i in range(1, 41):
    h1.SetBinContent(i, 5.0 + 50.0 * math.exp(-0.5 * ((h1.GetBinCenter(i) - 4.0) / 1.0) ** 2))
for i in range(1, 11):
    for j in range(1, 11):
        h2.SetBinContent(i, j, i + j)
p.source("src", {"h1": h1, "h2": h2})


def pads_of(layout):
    return {"grid": range(1, 7), "ratio": (1, 2)}.get(layout, (1,))


def setter(obj, kind, system, padID):
    """sets the coordinate system of the case (none for the defaults); returns the one the object ends up with"""
    if system in ("pad", "frame", "user"):
        getattr(obj, {"pad": "SetPadCoordinates", "frame": "SetFrameCoordinates", "user": "SetUserCoordinates"}[system])()
    return effective(kind, system, padID)


def effective(kind, system, padID):
    """the coordinate system an object ends up with: as set, or the default of its pad, of plot[0] or the built-in one (boxes: pad, shapes: user)"""
    if system == "pad_defaults":
        return "pad" if kind in SHAPES else ("user" if padID == 2 else "frame")
    if system != "default":
        return system
    return "user" if kind in SHAPES else "pad"


def given(kind, system, padID):
    return VALUE[kind] if effective(kind, system, padID) == "user" else FRACTION[kind]


expected = {}  # plot name: {kind: system}
for i, case in enumerate(todo):
    layout, system = case["layout"], case["system"]
    if layout == "grid":
        plot = S.Plot(f"c{i}", S.PanelLayout([[S.Panel()] * 3] * 2))
        plot.SetGroup("coords")
    else:
        plot = S.Plot(f"c{i}", "coords", {"ratio": "1d_ratio", "view": "2d"}.get(layout, "1d"))
    if system == "pad_defaults":
        plot[0].SetDefaultBoxCoordinates(S.frame_coordinates).SetDefaultShapeCoordinates(S.pad_coordinates)
        if 2 in pads_of(layout):
            plot[2].SetDefaultBoxCoordinates(S.user_coordinates)
    systems = {}
    for padID in pads_of(layout):
        pad = plot[padID]
        if layout == "view":
            pad.AddData("h2", "src", "data").SetDrawOption(S.lego).SetShowStats("e")
        else:
            pad.AddData("h1", "src", "data").SetShowStats("e")
            pad["X"].SetRange(1.0, 9.0)
            pad["Y"].SetRange(1.0, 100.0)
            if layout == "log":
                pad["X"].SetLog()
                pad["Y"].SetLog()
        objects = {
            "legend": pad.AddLegend(*given("legend", system, padID)[:2]),
            "text": pad.AddText(*given("text", system, padID)[:2], ["text", "two lines"]),
            "image": pad.AddImage(*given("image", system, padID)[:2], image),
        }
        stats = S.StatsBox()
        stats.SetPosition(*given("stats", system, padID)[:2])
        objects["stats"] = stats
        if layout != "view":
            g = lambda kind: given(kind, system, padID)  # noqa: E731
            objects["line"] = pad.AddLine(g("line")[:2], g("line")[2:])
            objects["arrow"] = pad.AddArrow(g("arrow")[:2], g("arrow")[2:])
            objects["vertical_line"] = pad.AddVerticalLine(*g("vertical_line"))
            objects["horizontal_line"] = pad.AddHorizontalLine(*g("horizontal_line"))
            objects["region"] = pad.AddRegion(g("region")[:2], g("region")[2:])
            objects["region_x"] = pad.AddRegionX(*g("region_x"))
            objects["region_y"] = pad.AddRegionY(*g("region_y"))
            objects["ellipse"] = pad.AddEllipse(g("ellipse")[:2], *g("ellipse")[2:])
            pts = g("polygon")
            objects["polygon"] = pad.AddPolygon([(pts[k], pts[k + 1]) for k in range(0, len(pts), 2)])
            objects["region_cut"] = pad.AddRegion(g("region_cut")[:2], g("region_cut")[2:])
        for kind, obj in objects.items():
            systems[(padID, kind)] = setter(obj, kind, system, padID)
        objects["text"].SetSize(*given("text", system, padID)[2:])
        objects["image"].SetWidth(given("image", system, padID)[2])
        objects["stats"].SetSize(*given("stats", system, padID)[2:])
        pad(1).SetStatsBox(objects["stats"])
    expected[f"c{i}"] = systems
    p.pm.AddPlot(plot)

run = p.plot("coords")
check("the plots are drawn without errors", not run.errors(), " | ".join(run.errors()[:3]))


def frame_of(padObj, view):
    """the frame as fractions of the pad: x1, y1, x2, y2"""
    if view:
        return padObj.GetLeftMargin(), padObj.GetBottomMargin(), 1.0 - padObj.GetRightMargin(), 1.0 - padObj.GetTopMargin()
    w, h = padObj.GetX2() - padObj.GetX1(), padObj.GetY2() - padObj.GetY1()
    return ((padObj.GetUxmin() - padObj.GetX1()) / w, (padObj.GetUymin() - padObj.GetY1()) / h, (padObj.GetUxmax() - padObj.GetX1()) / w, (padObj.GetUymax() - padObj.GetY1()) / h)


for i, case in enumerate(todo):
    name, layout = f"c{i}", case["layout"]
    systems = expected[name]
    for padID in pads_of(layout):
        padObj = run.pad(name, "coords", padID)
        where = f"{describe(case)} pad {padID}"
        if not padObj:
            check(f"{where}: the pad is drawn", False)
            continue
        f = frame_of(padObj, layout == "view")
        logx, logy = padObj.GetLogx(), padObj.GetLogy()
        w, h = padObj.GetX2() - padObj.GetX1(), padObj.GetY2() - padObj.GetY1()

        def toPad(value, isX):  # axis value -> fraction of the pad
            v = math.log10(value) if (logx if isX else logy) else value
            return (v - padObj.GetX1()) / w if isX else (v - padObj.GetY1()) / h

        def inSystem(ndc, isX, system):  # fraction of the pad -> the coordinate system
            if system == "frame":
                return (ndc - f[0]) / (f[2] - f[0]) if isX else (ndc - f[1]) / (f[3] - f[1])
            if system == "user":
                v = (padObj.GetX1() + ndc * w) if isX else (padObj.GetY1() + ndc * h)
                return 10.0**v if (logx if isX else logy) else v
            return ndc

        def length(ndc, isX, system):  # a size in fractions of the pad -> in the coordinate system (pad fractions for user)
            return ndc / ((f[2] - f[0]) if isX else (f[3] - f[1])) if system == "frame" else ndc

        def same(found, want, label):
            ok = all(abs(a - b) <= TOL * max(1.0, abs(b)) for a, b in zip(found, want))
            check(f"{where}: the {label} is drawn where it was put", ok, f"found {tuple(round(v, 6) for v in found)}, given {want}")

        prims = list(padObj.GetListOfPrimitives())
        legend = next((o for o in prims if o.GetName().startswith("LegendBox_")), None)
        text = next((o for o in prims if o.GetName().startswith("TextBox_")), None)
        stats = next((o for o in prims if o.GetName().startswith("stats_")), None)
        img = next((o for o in prims if o.GetName().startswith("ImageBox_")), None)
        for label, box in (("legend", legend), ("text", text), ("stats", stats)):
            s = systems[(padID, label)]
            if not box:
                check(f"{where}: the {label} is drawn", False)
                continue
            found = [inSystem(box.GetX1NDC(), True, s), inSystem(box.GetY2NDC(), False, s)]
            want = list(given(label, s, padID)[:2])
            if label in ("text", "stats"):
                found += [length(box.GetX2NDC() - box.GetX1NDC(), True, s), length(box.GetY2NDC() - box.GetY1NDC(), False, s)]
                want += list(given(label, s, padID)[2:])
            same(found, tuple(want), label)
        s = systems[(padID, "image")]
        if img:
            found = (inSystem(img.GetXlowNDC(), True, s), inSystem(img.GetYlowNDC() + img.GetHNDC(), False, s), length(img.GetWNDC(), True, s))
            same(found, given("image", s, padID), "image")
        else:
            check(f"{where}: the image is drawn", False)
        if layout == "view":
            continue

        # shapes: drawn in axis values, in the order they were added (an arrow is a TLine too)
        lines = [o for o in prims if o.ClassName() in ("TLine", "TArrow")]
        regions = [o for o in prims if o.ClassName() == "TBox"]
        ellipse = next((o for o in prims if o.ClassName() == "TEllipse"), None)
        polygon = next((o for o in prims if o.ClassName() == "TPolyLine"), None)
        s = systems[(padID, "line")]

        def point(x, y):
            return inSystem(toPad(x, True), True, s), inSystem(toPad(y, False), False, s)

        if len(lines) != 4 or len(regions) != 4 or not ellipse or not polygon:
            check(f"{where}: every shape is drawn", False, f"{len(lines)} lines, {len(regions)} regions")
            continue
        for label, obj in (("line", lines[0]), ("arrow", lines[1])):
            same(point(obj.GetX1(), obj.GetY1()) + point(obj.GetX2(), obj.GetY2()), given(label, s, padID), label)
        same((point(lines[2].GetX1(), 1.0)[0],), given("vertical_line", s, padID), "vertical line")
        same((point(1.0, lines[3].GetY1())[1],), given("horizontal_line", s, padID), "horizontal line")
        frameEdges = point(padObj.GetUxmin() if not logx else 10 ** padObj.GetUxmin(), padObj.GetUymin() if not logy else 10 ** padObj.GetUymin()) + point(padObj.GetUxmax() if not logx else 10 ** padObj.GetUxmax(), padObj.GetUymax() if not logy else 10 ** padObj.GetUymax())
        check(f"{where}: a vertical line spans the frame", abs(point(1.0, lines[2].GetY1())[1] - frameEdges[1]) < TOL and abs(point(1.0, lines[2].GetY2())[1] - frameEdges[3]) < TOL)
        box = lambda o: point(o.GetX1(), o.GetY1()) + point(o.GetX2(), o.GetY2())  # noqa: E731
        same(box(regions[0]), given("region", s, padID), "region")
        same(box(regions[1])[0::2], given("region_x", s, padID), "region in x")
        same(box(regions[2])[1::2], given("region_y", s, padID), "region in y")
        cut = list(given("region_cut", s, padID))
        if s != "pad":  # cut at the frame
            cut = [max(cut[0], frameEdges[0]), max(cut[1], frameEdges[1]), min(cut[2], frameEdges[2]), min(cut[3], frameEdges[3])]
        same(box(regions[3]), tuple(cut), f"region beyond the frame ({'kept' if s == 'pad' else 'cut'})")
        cx, cy = point(ellipse.GetX1(), ellipse.GetY1())
        if s == "user":
            radii = (ellipse.GetR1(), ellipse.GetR2())
        else:
            radii = (length(ellipse.GetR1() / w, True, s), length(ellipse.GetR2() / h, False, s))
        if s != "user" or not (logx or logy):  # TEllipse does not convert its radii on log axes (warned)
            same((cx, cy) + radii, given("ellipse", s, padID), "ellipse")
        pts = []
        for k in range(polygon.GetN() - 1):  # the last point closes it
            pts += point(polygon.GetX()[k], polygon.GetY()[k])
        same(tuple(pts), given("polygon", s, padID), "polygon")

finish()
