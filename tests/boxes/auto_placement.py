"""Automatically placed boxes (legends, texts, statistics boxes, images) lie inside the frame (in a pad with polar graphs, which has no
frame: inside the pad; in a 3d view: inside the margins of the pad), do not overlap each other, and do not cover anything drawn for
the data, the polar grid or shapes unless the plot warns that there is not enough space (then they go on top of the data, still apart).
2d and 3d data count as a map: a box may cover it when there is no room beside it (the convex hull of what is drawn for the data, for a
3d view of its box), which in a 3d view is the rule (ROOT fits the box of the view into the frame).
Cases: every data kind of srptest with the drawing options of what it is drawn as that fill the frame differently (points, lines,
fills, bars, bands, error boxes, maps, 3d views; ROOT's option F of a graph, its points as a filled polygon), values crossing zero and
unsorted graphs; one legend, a legend and a text, a legend and three texts, a legend of two columns and a text; at the best corner and at
each corner; with a statistics box, a shape (a region over the height of the frame, or over the whole frame), an image placed before them,
other viewing angles of 3d views, or the boxes in the ratio pad below the data (the data divided by itself); on a frame ending at the
data or twice as wide; linear and log y.
Reference: the pixels painted for the data (the canvas painted with and without the data) inside each box, in the PNG of the plot.
Covers: functions drawn with a fill, histograms with an error option but no errors drawn as
steps, lines and bands of histograms through the bin centres, graphs without errors drawn with error boxes or bands, multi-error graphs
by their largest errors, the edges of bands between the cells of the placement, 2d graphs and functions drawn as maps, the frame of 3d
views of functions, boxes on top of each other when there is no room, and the named cases.
"""

import re

import numpy as np

from srptest import DATA_KINDS, ROOT, WORK, Project, S, add_data, axis_range, base_of, cases, check, describe, drawn_type, finish, has_variant, in_chunks, is_box, is_data, pixels, ratio_input, source_of

OPTIONS = {
    "hist1d": ["default", "hist", "area", "area_smooth", "bars", "hbars", "band", "boxes", "line"],
    "graph": ["default", "line", "curve", "area", "bars", "band", "boxes", "raw_F"],  # raw_F: ROOT's option F, the points as a filled polygon
    "function": ["default", "area"],
    "hist2d": ["default", "box", "cont", "lego"],
    "graph2d": ["default", "points", "lego"],
    "function2d": ["default", "cont", "surf"],
    "hist3d": ["default", "iso"],
    "function3d": ["default"],
}
BOXES = {  # the number of boxes; legend_columns: a legend of two columns with three more rows, and a text
    "legend": 1,
    "legend_text": 2,
    "legend_texts": 4,
    "legend_columns": 2,
}
EXTRAS = ("none", "stats", "region", "image", "full", "view", "ratio")  # full: a region over the whole frame, no room beside the data; view: other viewing angles; ratio: the boxes in the ratio pad below
PLACEMENTS = ("best_corner", "top_left", "top_right", "bottom_right", "bottom_left")
NO_SPACE = ("Could not find enough space", "is larger than the space inside the frame")


VARIANTS = ("normal", "negative", "unsorted")


def polar(kind):
    return "polar" in DATA_KINDS[kind][2]


def plain(case, *keep):
    """whether a case has the plain values in every dimension but those in keep"""
    plainValues = {"boxes": "legend_text", "placement": "best_corner", "extra": "none", "range": "data", "log": False, "variant": "normal"}
    return all(case[k] == v for k, v in plainValues.items() if k not in keep)


def valid(case):
    """every option of every kind with each dimension varied on its own, the box sets also on wide frames and log y"""
    kind = case["kind"]
    if case["option"] not in OPTIONS[drawn_type(kind)] or not has_variant(kind, case["variant"]):
        return False
    if polar(kind) and (case["range"] != "data" or case["extra"] in ("region", "stats", "full")):  # shapes need a cartesian frame
        return False
    if case["extra"] == "view" and DATA_KINDS[kind][0] == 1:
        return False
    if case["extra"] == "ratio" and (DATA_KINDS[kind][0] > 1 or polar(kind) or case["option"] == "hbars"):
        return False
    if case["log"] and (DATA_KINDS[kind][0] > 1 or polar(kind)):
        return False
    return plain(case, "boxes", "range", "log") or plain(case, "placement", "range") or plain(case, "extra", "range") or plain(case, "variant")


def named(kind, option, what, **values):
    case = {"kind": kind, "option": option, "boxes": "legend_text", "placement": "best_corner", "extra": "none", "range": "data", "log": False, "variant": "normal"}
    return {**case, **values, "why": what}


todo = cases(
    {
        "kind": list(DATA_KINDS),
        "option": sorted({o for options in OPTIONS.values() for o in options}),
        "boxes": list(BOXES),
        "placement": list(PLACEMENTS),
        "extra": list(EXTRAS),
        "range": ["data", "wide"],
        "log": [False, True],
        "variant": list(VARIANTS),
    },
    valid=valid,
    always=lambda c: plain(c, "range"),
    named=[
        named("hist", "hist", "filled steps, also on a frame below zero (the fill starts at zero)", variant="negative"),
        named("hist", "area_smooth", "filled smooth curve"),
        named("graph", "bars", "bar chart of a graph"),
        named("hist", "hbars", "horizontal bar chart"),
        named("graph_polar", "default", "four boxes around the polar grid", boxes="legend_texts"),
        named("hist_counts", "default", "error bars of a histogram without Sumw2"),
        named("graph", "default", "markers only: any point order", variant="unsorted"),
        named("hist", "default", "a region", extra="region"),
        named("hist", "default", "no room: on top of the data, apart", boxes="legend_texts", extra="full"),
        named("hist_var", "line", "a line through the centres of wide bins", placement="top_right"),
        named("hist", "band", "a band through the bin centres", placement="bottom_left"),
        named("graph_multi", "band", "the larger of two sets of errors", placement="top_right"),
        named("graph_multi", "boxes", "the larger of two sets of errors", placement="top_right"),
        named("hist2d", "lego", "3d view at other angles", boxes="legend", extra="view"),
        named("function2d", "cont", "an image beside contour lines, in a corner they leave free", extra="image"),
        named("hist3d", "default", "3d view at other angles", boxes="legend", extra="view"),
        named("tf2", "surf", "3d view at other angles", extra="view"),
        named("hist", "default", "boxes in the pad of a ratio", boxes="legend_texts", extra="ratio"),
    ],
)

# an image to place: a small filled square
image = f"{WORK}/logo.png"
logo = ROOT.TCanvas("logo", "", 100, 60)
logo.SetFillColor(ROOT.kAzure + 1)
level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
logo.Print(image)
ROOT.gErrorIgnoreLevel = level

p = Project("auto_placement", bases=("1d", "1d_ratio", "2d"))
p.inputs(VARIANTS)
for i, case in enumerate(todo):
    kind, placement = case["kind"], getattr(S, case["placement"])
    plot = S.Plot(f"c{i}", "boxes", "1d_ratio" if case["extra"] == "ratio" else base_of(kind))
    pad = plot[1]
    data = add_data(pad, kind, source_of(case["variant"]), case["variant"]).SetLegendLabel("data of a kind").SetFill(1001, S.kOrange + 7)
    if case["extra"] == "ratio":  # the kind by itself in the ratio pad, which holds the boxes
        pad = plot[2]
        data = pad.AddRatio(ratio_input(kind, source_of(case["variant"]), case["variant"]), ratio_input(kind, source_of(case["variant"]), case["variant"]))
        data.SetLegendLabel("ratio").SetFill(1001, S.kOrange + 7)
        if "formula" in DATA_KINDS[kind][2]:
            data.SetRangeX(*axis_range(case["variant"]))
    if case["option"].startswith("raw_"):
        data.SetDrawOption(case["option"][4:])
    elif case["option"] != "default":
        data.SetDrawOption(getattr(S, case["option"]))
    if case["extra"] == "stats":
        data.SetShowStats()
    elif case["extra"] == "region":
        pad.AddRegionX(6.0, 7.0)
    elif case["extra"] == "full":
        pad.AddRegionX(0.0, 20.0)
    elif case["extra"] == "view":
        pad.SetView(60.0, 10.0)
    elif case["extra"] == "image":
        pad.AddImage(placement, image).SetWidth(0.15)
    legend = pad.AddLegend(placement)
    if case["boxes"] == "legend_columns":
        legend.SetNumColumns(2)
        for row in ("second row", "third row", "fourth row"):
            legend.AddEntry(row)
    for n in range(1, BOXES[case["boxes"]]):
        pad.AddText(placement, ["text", "with two lines"] if n == 1 else f"text {n}")
    if case["range"] == "wide":
        pad["X"].SetRange(1.0, 19.0)
    if case["log"]:
        pad["Y"].SetLog()
    p.pm.AddPlot(plot)
# room left by the drawing as ROOT draws it: markers only (P, or E P without errors) draw no steps and no fill, although the histogram
# has a fill colour and style; its contents alternate between the bottom and the top of the frame, so steps would block every column
room = ROOT.TH1D("room", "", 20, 0.0, 20.0)
room.Sumw2()
for b in range(1, 21):
    room.SetBinContent(b, 1.0 if b % 2 else 9.5)
    room.SetBinError(b, 0.0)
p.source("room", {"room": room})
ROOM = {"markers": "P", "error_markers": "E P"}
for name, option in ROOM.items():
    plot = S.Plot(f"room_{name}", "boxes", "1d")
    plot[1].AddData("room", "room").SetDrawOption(option).SetFill(1001, S.kOrange + 7).SetMarkerSize(0.6)
    plot[1]["Y"].SetRange(0.0, 10.0)
    plot[1].AddLegend()
    plot[1].AddText(["text", "with two lines"])
    p.pm.AddPlot(plot)
run = p.plot("boxes")
for name, option in ROOM.items():
    warned = [line for line in run.messages(f"room_{name}", "boxes") if any(text in line for text in NO_SPACE)]
    check(f"markers only ({option}) leave room between them: no warning of too little space [mark only what root draws]", run.pad(f"room_{name}", "boxes") is not None and not warned, " | ".join(warned))


def rect(obj):
    """the box in NDC of its pad: (x1, y1, x2, y2)"""
    if obj.InheritsFrom("TPad"):
        return obj.GetXlowNDC(), obj.GetYlowNDC(), obj.GetXlowNDC() + obj.GetWNDC(), obj.GetYlowNDC() + obj.GetHNDC()
    return obj.GetX1NDC(), obj.GetY1NDC(), obj.GetX2NDC(), obj.GetY2NDC()


def compare(i, case):
    what = f"c{i} " + describe(case)
    canvas = run.canvas(f"c{i}", "boxes")
    if not canvas:
        return [(f"plot made: {what}", False, " | ".join(run.errors()[:3]))]
    padID = 2 if case["extra"] == "ratio" else 1
    pad = canvas.GetPrimitive(f"Pad_{padID}")
    W, H = canvas.GetWw(), canvas.GetWh()
    tol = (2.0 / (W * pad.GetAbsWNDC()), 2.0 / (H * pad.GetAbsHNDC()))  # two pixels in NDC of the pad
    boxes = [(obj.GetName(), rect(obj)) for obj in pad.GetListOfPrimitives() if is_box(obj)]
    if polar(case["kind"]):
        area = (0.0, 0.0, 1.0, 1.0)
    else:
        area = (pad.GetLeftMargin(), pad.GetBottomMargin(), 1.0 - pad.GetRightMargin(), 1.0 - pad.GetTopMargin())
    warned = any(text in line for line in run.messages(f"c{i}", "boxes") for text in NO_SPACE)
    results = []
    outside = [n for n, (x1, y1, x2, y2) in boxes if x1 < area[0] - tol[0] or y1 < area[1] - tol[1] or x2 > area[2] + tol[0] or y2 > area[3] + tol[1]]
    results.append((f"boxes inside the frame: {what}", not outside, ", ".join(f"{n} {rect_text(r)}" for n, r in boxes if n in outside) + f" frame {rect_text(area)}"))
    overlaps = [f"{a[0]} and {b[0]}" for k, a in enumerate(boxes) for b in boxes[k + 1 :] if overlap(a[1], b[1], tol)]
    results.append((f"boxes apart: {what}", not overlaps, ", ".join(overlaps)))
    frame, _ = pixels(run.canvas(f"c{i}", "boxes"), lambda o: not is_box(o) and not is_data(o), padID)
    drawn, toPixel = pixels(run.canvas(f"c{i}", "boxes"), lambda o: not is_box(o), padID)
    data = drawn != frame
    # a 3d view: what may be covered when there is no room beside it is the outline of its box (painted with the frame)
    frameHist = run.frame_hist(f"c{i}", "boxes", padID)
    if frameHist and (frameHist.GetDimension() == 3 or re.search("LEGO|SURF", run.frame_option(f"c{i}", "boxes", padID) or "")):
        left, top = toPixel(area[0], area[3])
        right, bottom = toPixel(area[2], area[1])
        inside = np.zeros_like(data)
        inside[int(top) : int(bottom), int(left) : int(right)] = True
        outline = (frame != frame[int(top) + 5, int(left) + 5]) & inside
    else:
        outline = data
    inPixels = {}
    for name, (x1, y1, x2, y2) in boxes:
        c1, r1 = toPixel(x1, y2)
        c2, r2 = toPixel(x2, y1)
        inPixels[name] = (int(c1), int(r1), int(c2), int(r2))
    covering = []
    for name, (c1, r1, c2, r2) in inPixels.items():
        covered = int(data[r1 + 2 : r2 - 1, c1 + 2 : c2 - 1].sum())
        if covered and not (DATA_KINDS[case["kind"]][0] > 1 and not room_beside(outline, name, inPixels, toPixel, area)):
            covering.append(f"{name} covers {covered} pixels")
    results.append((f"boxes off the data unless warned: {what}", warned or not covering, "; ".join(covering)))
    if case.get("why") == "four boxes around the polar grid":
        results.append((f"room for four boxes around the polar grid: {what}", not warned, " | ".join(run.messages(f"c{i}", "boxes"))))
    return results


def room_beside(outline, name, inPixels, toPixel, area):
    """Whether a box would fit beside 2d or 3d data (a map, a 3d view) and the other boxes: somewhere in the frame (without a margin for
    the ticks) a box larger by a cell of the placement grid (10 pixels) on each side touches neither them nor the convex hull of the
    pixels drawn for the data (for a 3d view: of its box)."""
    blocked = hull(outline)
    for other, (c1, r1, c2, r2) in inPixels.items():
        if other != name:
            blocked[max(r1, 0) : r2 + 1, max(c1, 0) : c2 + 1] = True
    c1, r1, c2, r2 = inPixels[name]
    width, height = c2 - c1 + 20, r2 - r1 + 20
    left, top = toPixel(area[0], area[3])
    right, bottom = toPixel(area[2], area[1])
    margin = int(0.03 * (right - left))
    region = blocked[int(top) + margin : int(bottom) - margin, int(left) + margin : int(right) - margin]
    if region.shape[0] < height or region.shape[1] < width:
        return False
    table = np.zeros((region.shape[0] + 1, region.shape[1] + 1), dtype=np.int64)
    table[1:, 1:] = region.cumsum(0).cumsum(1)
    counts = table[height:, width:] - table[:-height, width:] - table[height:, :-width] + table[:-height, :-width]
    return bool((counts == 0).any())


def hull(mask):
    """the convex hull of the set pixels of a mask, filled"""
    rows = np.nonzero(mask.any(axis=1))[0]
    filled = np.zeros_like(mask, dtype=bool)
    if len(rows) == 0:
        return filled
    points = sorted({(int(np.nonzero(mask[r])[0][k]), int(r)) for r in rows for k in (0, -1)})
    chain = []
    for sequence in (points, points[::-1]):
        start = len(chain)
        for pt in sequence:
            while len(chain) >= start + 2 and (chain[-1][0] - chain[-2][0]) * (pt[1] - chain[-2][1]) - (chain[-1][1] - chain[-2][1]) * (pt[0] - chain[-2][0]) <= 0:
                chain.pop()
            chain.append(pt)
        chain.pop()
    for r in range(rows[0], rows[-1] + 1):
        xs = []
        for (xa, ya), (xb, yb) in zip(chain, chain[1:] + chain[:1]):
            if min(ya, yb) <= r <= max(ya, yb):
                xs += [xa, xb] if ya == yb else [xa + (r - ya) * (xb - xa) / (yb - ya)]
        if xs:
            filled[r, int(min(xs)) : int(max(xs)) + 1] = True
    return filled


def rect_text(r):
    return "(" + ", ".join(f"{v:.3f}" for v in r) + ")"


def overlap(a, b, tol):
    return a[0] < b[2] - tol[0] and b[0] < a[2] - tol[0] and a[1] < b[3] - tol[1] and b[1] < a[3] - tol[1]


check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
in_chunks(todo, compare)
finish()
