"""The geometry of plots made from a PanelLayout or a shipped base plot is the one its lengths describe: every axis frame of Panel(w, h) is
w x h standard panels (638 pixels), a cell spanning columns or rows covers them exactly with what lies between, frames of a column share
their left and right edges and frames of a row their top and bottom edges, touching panels meet (or are the panel spacing apart, globally
or per boundary), a gap leaves the margins of both sides, the outer frames are the margins away from the canvas edges, a panel with its
own colour scale has the colour-scale margin on its right and a ColorScale() cell is that wide, all edges are whole pixels. The pads of
a plot cover no frame of another one, each panel's pad holds its frame, and the pads are numbered in reading order without gaps and holes.
The model of these rules (below) is computed from the documented lengths; it is checked against every name of the base-plot grammar
<type>[_wide|_tall][_<columns>x<rows>][_ratio][_pull][_gap] (with BasePlotShape and the lengths of a PanelLayout), against generated
layouts (grids with touching panels, gaps or spaces, wide and tall panels, ratio rows, own colour scales, colour-scale cells, spanning
cells, strips and holes) and against the layouts of past bugs. Invalid arrangements throw naming the problem; unknown base-plot names
and 2d base plots with ratio or pull panels are refused with an error.
Covers: spanning cells, BasePlotShape, colour scales in pixels, pull panels, holes, spacing per
boundary.
"""

import math
import re

from srptest import Chunks, Project, S, cases, check, describe, finish, python

# the lengths of the layout (PanelLayout.h), in pixels
DEFAULT = {"panel": 638.0, "top": 32.0, "bottom": 118.0, "left": 118.0, "right": 32.0, "scale": 268.0, "spacing": 0.0}
SHAPE = {"aspect": 1.618, "ratio": 0.37, "pull": 0.37}


# ---------------------------------------------------------------------------------------------
# cells of a layout, described independently of SciRooPlot's classes
# ---------------------------------------------------------------------------------------------
def panel(w=1.0, h=1.0, scale=False, span=(1, 1), above=None, left=None):
    return {"kind": "panel", "w": w, "h": h, "scale": scale, "span": span, "above": above, "left": left}


def gap():
    return {"kind": "gap"}


def empty(w=None, h=None, span=(1, 1), hole=False):
    return {"kind": "hole" if hole else "empty", "w": w, "h": h, "span": span}


def scale_cell(of=0, span=(1, 1)):
    return {"kind": "scale", "of": of, "span": span}


def build(cell):
    """the SciRooPlot cell of a description"""
    kind = cell["kind"]
    if kind == "gap":
        return S.Gap()
    if kind == "scale":
        made = S.ColorScale(cell["of"]) if cell["of"] else S.ColorScale()
    elif kind in ("empty", "hole"):
        make = S.Hole if kind == "hole" else S.Empty
        made = make(cell["w"], cell["h"]) if cell["w"] is not None else make()
    else:
        made = S.Panel(cell["w"], cell["h"])
        if cell["scale"]:
            made.ColorScale()
        if cell["above"] is not None:
            made.SpacingAbove(cell["above"])
        if cell["left"] is not None:
            made.SpacingLeft(cell["left"])
    if cell["span"] != (1, 1):
        made.Span(*cell["span"])
    return made


def layout_of(rows, lengths):
    layout = S.PanelLayout([[build(cell) for cell in row] for row in rows])
    if lengths.get("panel") != DEFAULT["panel"]:
        layout.SetPanelSize(lengths["panel"])
    if any(lengths[k] != DEFAULT[k] for k in ("top", "bottom", "left", "right")):
        layout.SetMargins(lengths["top"], lengths["bottom"], lengths["left"], lengths["right"])
    if lengths["scale"] != DEFAULT["scale"]:
        layout.SetColorScaleMargin(lengths["scale"])
    if lengths["spacing"] != DEFAULT["spacing"]:
        layout.SetPanelSpacing(lengths["spacing"])
    return layout


# ---------------------------------------------------------------------------------------------
# the model: canvas size, frames and pad numbers of a layout from its lengths
# ---------------------------------------------------------------------------------------------
def rounded(pixel):
    """to whole pixels, halves away from zero"""
    return math.floor(pixel + 0.5)


def model(rows, lengths):
    """{"canvas": (w, h), "pads": {pad: (kind, (left, right, top, bottom) of its frame, or of its column for a colour-scale cell)}}"""
    owner, cells, gapCols, gapRows = {}, [], set(), set()
    r = 0
    for row in rows:
        if all(cell["kind"] == "gap" for cell in row):
            gapRows.add(r - 1)
            continue
        c = 0
        for cell in row:
            if cell["kind"] == "gap":
                gapCols.add(c - 1)
                continue
            while (r, c) in owner:
                c += 1
            cols, nrows = cell["span"]
            cells.append((cell, r, c))
            for rr in range(r, r + nrows):
                for cc in range(c, c + cols):
                    owner[(rr, cc)] = len(cells) - 1
            c += cols
        r += 1
    nRows, nCols = r, 1 + max(c for _, c in owner)
    P = lengths["panel"]
    width, height = [0.0] * nCols, [0.0] * nRows
    scaleCol, ownScale = [False] * nCols, [False] * nCols
    for cell, r, c in cells:
        cols, nrows = cell["span"]
        if cell["kind"] == "scale":
            scaleCol[c] = True
            width[c] = lengths["scale"]
        if cell["kind"] == "panel" and cell["scale"]:
            ownScale[c + cols - 1] = True
    for cell, r, c in cells:
        cols, nrows = cell["span"]
        sized = cell["kind"] == "panel" or (cell["kind"] in ("empty", "hole") and cell["w"] is not None)
        if sized and cols == 1 and not scaleCol[c]:
            width[c] = cell["w"] * P
        if sized and nrows == 1:
            height[r] = cell["h"] * P
    # the space between columns c and c + 1 and between rows (the parts before and after the boundary)
    colSpacing, rowSpacing = {}, {}
    for cell, r, c in cells:
        if cell["kind"] == "panel" and cell["left"] is not None:
            colSpacing[c - 1] = cell["left"]
        if cell["kind"] == "panel" and cell["above"] is not None:
            rowSpacing[r - 1] = cell["above"]
    right = [lengths["scale"] if ownScale[c] else lengths["right"] for c in range(nCols)]
    colSep = []
    for c in range(nCols - 1):
        if c in gapCols:
            colSep.append(right[c] + lengths["left"])
        elif scaleCol[c + 1]:
            colSep.append(lengths["scale"] if ownScale[c] else 0.0)
        elif scaleCol[c]:
            colSep.append(lengths["left"])
        elif ownScale[c]:
            colSep.append(lengths["scale"] + lengths["left"])
        else:
            colSep.append(colSpacing.get(c, lengths["spacing"]))
    rowSep = [lengths["bottom"] + lengths["top"] if r in gapRows else rowSpacing.get(r, lengths["spacing"]) for r in range(nRows - 1)]

    def edges(start, sizes, seps):
        lows, highs, pos = [], [], start
        for i, size in enumerate(sizes):
            pos = rounded(pos)
            lows.append(pos)
            pos += rounded(size)
            highs.append(pos)
            if i < len(seps):
                pos += seps[i]
        return lows, highs, pos

    lefts, rights, x = edges(lengths["left"], width, colSep)
    tops, bottoms, y = edges(lengths["top"], height, rowSep)
    canvas = (rounded(x + (0.0 if scaleCol[-1] else right[-1])), rounded(y + lengths["bottom"]))
    pads, number = {}, 0
    for cell, r, c in cells:
        if cell["kind"] == "hole":
            continue
        number += 1
        cols, nrows = cell["span"]
        pads[number] = (cell["kind"], (lefts[c], rights[c + cols - 1], tops[r], bottoms[r + nrows - 1]))
    return {"canvas": canvas, "pads": pads}


# ---------------------------------------------------------------------------------------------
# the shipped base plots, as the grammar describes them (PlotManager::MakeBasePlot)
# ---------------------------------------------------------------------------------------------
GRAMMAR = re.compile(r"^(1d|2d)(?:_(wide|tall))?(?:_(\d+)x(\d+))?(_ratio)?(_pull)?(_gap)?$")


def base_rows(name, shape):
    """the cells of a base plot: a grid of main panels (2d: with colour scale, a grid without gaps one shared scale in a cell after the
    first row spanning all rows), below each a ratio and/or pull panel, gaps between the panels"""
    typ, form, cols, nrows, ratio, pull, gaps = GRAMMAR.match(name).groups()
    cols, nrows = int(cols or 1), int(nrows or 1)
    w = shape["aspect"] if form == "wide" else 1.0
    h = shape["aspect"] if form == "tall" else 1.0
    below = ([shape["ratio"]] if ratio else []) + ([shape["pull"]] if pull else [])
    shared = typ == "2d" and cols * nrows > 1 and not gaps
    rows = []
    for r in range(nrows):
        if r > 0 and gaps:
            rows.append([gap()])
        main = []
        for c in range(cols):
            if c > 0 and gaps:
                main.append(gap())
            main.append(panel(w, h, scale=typ == "2d" and not shared))
        if shared and r == 0:
            main.append(scale_cell(span=(1, nrows)))
        rows.append(main)
        for bh in below:
            row = []
            for c in range(cols):
                if c > 0 and gaps:
                    row.append(gap())
                row.append(panel(w, bh))
            rows.append(row)
    return rows


# ---------------------------------------------------------------------------------------------
# the cases
# ---------------------------------------------------------------------------------------------
NAMES = [typ + form + grid + below + gaps for typ in ("1d", "2d") for form in ("", "_wide", "_tall") for grid in ("", "_2x1", "_1x2", "_2x2", "_3x1", "_1x3") for below in ("", "_ratio", "_pull", "_ratio_pull") for gaps in ("", "_gap") if not (typ == "2d" and below)]
SHAPED = {  # base plots with other proportions or lengths
    "1d_wide_ratio": ({"aspect": 1.25, "ratio": 0.25, "pull": 0.37}, {}),
    "1d_tall_ratio_pull": ({"aspect": 1.4, "ratio": 0.3, "pull": 0.2}, {}),
    "1d_2x2_ratio_gap": (SHAPE, {"panel": 400.0, "top": 20.0, "bottom": 90.0, "left": 100.0, "right": 15.0}),
    "2d_2x1": (SHAPE, {"panel": 500.0, "scale": 200.0}),
    "2d_1x2_gap": (SHAPE, {"scale": 180.0, "left": 90.0}),
}
REFUSED = ["3d", "1d_0x1", "1d_x2", "2d_2x", "1d_ratio_wide", "1d_pull_ratio", "1d_gap_ratio", "1d_1000x1", "2d_ratio", "2d_pull", "2d_2x1_ratio_pull", "1D"]

# generated layouts: a grid of panels with the separation of its columns and rows, the shape of its panels and an extra
generated = cases(
    {
        "grid": ["1x1", "2x1", "1x2", "2x2", "3x2"],
        "columns": ["touch", "gap", "spacing", "global"],
        "rows": ["touch", "gap", "spacing", "global"],
        "shape": ["square", "wide", "tall"],
        "extra": ["none", "ratio", "own_scale", "scale_cell", "named_scale", "span_top", "strip", "hole_corner", "lengths"],
    },
    valid=lambda c: (c["grid"][0] != "1" or c["columns"] == "touch")
    and (c["grid"][-1] != "1" or c["rows"] == "touch")
    and not (c["extra"] in ("scale_cell", "named_scale") and c["columns"] == "gap")
    and not (c["extra"] == "own_scale" and c["columns"] == "spacing")  # panels with own colour scales do not touch
    and not (c["extra"] == "span_top" and c["grid"][0] == "1")
    and not (c["extra"] == "hole_corner" and c["grid"] == "1x1"),
)


def generated_rows(case):
    cols, nrows = (int(n) for n in case["grid"].split("x"))
    w, h = {"square": (1.0, 1.0), "wide": (1.618, 1.0), "tall": (1.0, 1.4)}[case["shape"]]
    extra = case["extra"]
    rows = []
    if extra == "strip":
        rows.append([empty(w, 0.2, span=(cols, 1))])
    if extra == "span_top":  # a gap row below it: touching panels of another width share no axis (warned)
        rows += [[panel(w, h, span=(cols, 1))], [gap()]]
    for r in range(nrows):
        if r > 0 and case["rows"] == "gap":
            rows.append([gap()])
        row = []
        for c in range(cols):
            if c > 0 and case["columns"] == "gap":
                row.append(gap())
            last = r == nrows - 1 and c == cols - 1
            if extra == "hole_corner" and last:
                row.append(empty(w, h, hole=True))
                continue
            cell = panel(w, h, scale=extra == "own_scale")
            if c > 0 and case["columns"] == "spacing":
                cell["left"] = 12.0
            if r > 0 and case["rows"] == "spacing" and c == 0:
                cell["above"] = 7.0  # given by one cell, holding for the whole boundary
            row.append(cell)
        if extra == "scale_cell" and r == 0:
            row.append(scale_cell(span=(1, nrows)))
        rows.append(row)
        if extra == "ratio":
            rows.append([panel(w, 0.37) for _ in range(cols)] if case["columns"] != "gap" else [x for c in range(cols) for x in ([gap()] if c else []) + [panel(w, 0.37)]])
    if extra == "named_scale":  # in front of the panels, spanning all rows, naming the first panel
        rows[0].insert(0, scale_cell(of=2, span=(1, sum(not all(cell["kind"] == "gap" for cell in row) for row in rows))))
    return rows


def generated_lengths(case):
    lengths = dict(DEFAULT)
    if case["columns"] == "global" or case["rows"] == "global":
        lengths["spacing"] = 20.0
    if case["extra"] == "lengths":
        lengths.update({"panel": 450.0, "top": 10.0, "bottom": 100.0, "left": 95.0, "right": 25.0, "scale": 210.0})
    return lengths


EXPLICIT = {  # layouts of past bugs and of the documentation
    "legend strip over three panels": [[empty(1.0, 0.2, span=(3, 1))], [panel(), panel(), panel()]],
    "wide panel above two after a gap row": [[panel(1.0, 0.75, span=(2, 1))], [gap()], [panel(), panel()]],
    "tall panel spanning two rows": [[panel(span=(1, 2)), gap(), panel(1.0, 0.62)], [panel(1.0, 0.38)]],
    "map with projections, corner hole": [[panel(1.0, 0.37), empty(hole=True, span=(2, 1))], [panel(), panel(0.37, 1.0), scale_cell(of=2)]],
    "map with projections, corner empty": [[panel(1.0, 0.37), empty(span=(2, 1))], [panel(), panel(0.37, 1.0), scale_cell(of=3)]],
    "ratio below, panel right, hole": [[panel(), panel(0.37, 1.0)], [panel(1.0, 0.37), empty(hole=True)]],
    "sized hole between panels": [[panel(), empty(0.3, 1.0, hole=True), panel()]],
    "sized empty between panels": [[panel(), empty(0.3, 1.0), panel()]],
    "ratio set off from its panel": [[panel()], [panel(1.0, 0.37, above=12.0)]],
    "two ratios set off": [[panel(), panel()], [panel(1.0, 0.37, above=12.0), panel(1.0, 0.37)]],
    "projection set off from its map": [[panel(), panel(0.37, 1.0, left=10.0), scale_cell(of=1)]],
    "no space at one boundary, global elsewhere": [[panel(), panel()], [panel(1.0, 0.37, above=0.0), panel(1.0, 0.37, above=0.0)]],
    "scale cells per row": [[panel(), panel(), scale_cell()], [panel(), panel(), scale_cell()]],
    "own scale before a scale cell": [[panel(scale=True), scale_cell()]],
    "own scales with a gap": [[panel(scale=True), gap(), panel(scale=True)]],
    "gap next to a cell from above": [[panel(span=(1, 2)), gap(), panel()], [panel()]],
}
EXPLICIT_LENGTHS = {"no space at one boundary, global elsewhere": dict(DEFAULT, spacing=20.0)}

INVALID = {  # rows: the part of the message naming the problem
    "a cell beyond the last row": ([[S.Panel().Span(1, 2), S.Panel()]], "spans below the last row"),
    "a row covering too many columns": ([[S.Panel(), S.Panel()], [S.Panel().Span(3)]], "covers 3 columns"),
    "a row covering too few columns": ([[S.Panel(), S.Panel(), S.Panel()], [S.Panel().Span(2)]], "covers 2 columns"),
    "gaps at other places": ([[S.Panel(), S.Gap(), S.Panel()], [S.Panel(), S.Panel()]], "not at the same places"),
    "a column without width": ([[S.Panel().Span(2)], [S.Empty(), S.Panel()]], "has no width"),
    "a row without height": ([[S.Panel().Span(1, 2), S.Panel()], [S.Empty()]], "has no height"),
    "panels of different widths in a column": ([[S.Panel(1.0, 1.0)], [S.Panel(2.0, 1.0)]], "different widths"),
    "panels of different heights in a row": ([[S.Panel(1.0, 1.0), S.Panel(1.0, 2.0)]], "different heights"),
    "a gap at the start of a row": ([[S.Gap(), S.Panel()]], "gap at its beginning"),
    "a row ending with a gap": ([[S.Panel(), S.Gap()]], "ends with a gap"),
    "a gap row first": ([[S.Gap()], [S.Panel()]], "first row must contain panels"),
    "two gap rows": ([[S.Panel()], [S.Gap()], [S.Gap()], [S.Panel()]], "two gap rows"),
    "a gap row last": ([[S.Panel()], [S.Gap()]], "last row must contain panels"),
    "an empty row": ([[S.Panel()], []], "is empty"),
    "a cell overlapping one from above": ([[S.Panel().Span(2, 2)], [S.Panel(), S.Panel()]], "covers 4 columns"),
    "a colour-scale cell spanning columns": ([[S.Panel(), S.ColorScale().Span(2)]], "spans one column"),
    "a panel in a colour-scale column": ([[S.Panel(), S.ColorScale()], [S.Panel(), S.Panel()]], "holds a ColorScale() cell and a panel"),
    "a colour-scale cell without width": ([[S.Panel(), S.ColorScale()]], "SetColorScaleMargin is 0"),
    "space above the first row": ([[S.Panel().SpacingAbove(5)]], "no cell on that side"),
    "space left of the first column": ([[S.Panel().SpacingLeft(5), S.Panel()]], "no cell on that side"),
    "two spaces on one boundary": ([[S.Panel(), S.Panel()], [S.Panel(1, 0.37).SpacingAbove(5), S.Panel(1, 0.37).SpacingAbove(6)]], "another cell along the same boundary"),
    "space after a gap column": ([[S.Panel(), S.Gap(), S.Panel().SpacingLeft(5)]], "a gap or a colour scale"),
    "space after a gap row": ([[S.Panel()], [S.Gap()], [S.Panel().SpacingAbove(5)]], "a gap lies above"),
    "space left of a colour-scale cell": ([[S.Panel(), S.ColorScale().SpacingLeft(5)]], "a gap or a colour scale"),
    "more than 255 pads": ([[S.Panel()] * 256], "more than 255 pads"),
}
INVALID_SETTERS = {  # setters refusing a value when called
    "a panel of size 0": (lambda: S.Panel(0.0, 1.0), "positive and finite"),
    "a span of 0 columns": (lambda: S.Panel().Span(0), "at least one column"),
    "a negative space": (lambda: S.Panel().SpacingAbove(-1.0), "zero or positive"),
    "ColorScale(0)": (lambda: S.ColorScale(0), "counted from 1"),
}
# lengths of the whole layout refusing a value: each warned about when set and ignored, the value set before stays (as for plots)
INVALID_LENGTHS = {
    "SetPanelSize": [(0.0,), (-1.0,), (float("nan"),)],
    "SetMargins": [(-1.0, float("inf"), -2.0, float("nan"))],
    "SetTextSize": [(0.0,), (-1.0,), (float("inf"),)],
    "SetTickLength": [(float("inf"),), (float("nan"),)],
    "SetLineWidth": [(-1.0,), (float("nan"),)],
}

# ---------------------------------------------------------------------------------------------
# the plots: data in every panel (2d data where the layout has colour scales), a text in every empty cell
# ---------------------------------------------------------------------------------------------
p = Project("geometry", bases=())
p.inputs()
chunks = Chunks("geometry")
plots = {}  # plot name: (description, rows, lengths)


def fill(plot, rows, lengths):
    scales = any(cell["kind"] == "scale" or (cell["kind"] == "panel" and cell["scale"]) for row in rows for cell in row)
    for padID, (kind, _) in model(rows, lengths)["pads"].items():
        if kind == "panel":
            plot[padID].AddData("hist2d" if scales else "hist", "inputs")
        elif kind == "empty":
            plot[padID].AddText(0.25, 0.75, "text").SetSize(0.5, 0.5)  # inside the cell, however narrow


for name in NAMES:
    p.pm.AddBasePlot(S.PlotManager.MakeBasePlot(name))
    plot = S.Plot(f"base_{name}", chunks.add(f"base_{name}"), name)
    rows = base_rows(name, SHAPE)
    fill(plot, rows, DEFAULT)
    p.pm.AddPlot(plot)
    plots[f"base_{name}"] = (f"base plot {name}", rows, DEFAULT)
for name, (shape, given) in SHAPED.items():
    lengths = dict(DEFAULT, **given)
    sizes = S.PanelLayout()
    if "panel" in given:
        sizes.SetPanelSize(given["panel"])
    if any(k in given for k in ("top", "bottom", "left", "right")):
        sizes.SetMargins(lengths["top"], lengths["bottom"], lengths["left"], lengths["right"])
    if "scale" in given:
        sizes.SetColorScaleMargin(given["scale"])
    base = S.PlotManager.MakeBasePlot(name, sizes, S.BasePlotShape().SetAspectRatio(shape["aspect"]).SetRatioHeight(shape["ratio"]).SetPullHeight(shape["pull"]))
    base.SetName(f"shaped_{name}")
    p.pm.AddBasePlot(base)
    plot = S.Plot(f"shaped_{name}", chunks.add(f"shaped_{name}"), f"shaped_{name}")
    rows = base_rows(name, shape)
    fill(plot, rows, lengths)
    p.pm.AddPlot(plot)
    plots[f"shaped_{name}"] = (f"base plot {name} with {shape} {given}", rows, lengths)
ignored = S.BasePlotShape()  # values that are not positive and finite are ignored (warned when set, see below)
for bad in (0.0, -1.0, float("nan"), float("inf")):
    ignored.SetAspectRatio(bad).SetRatioHeight(bad).SetPullHeight(bad)
base = S.PlotManager.MakeBasePlot("1d_wide_ratio_pull", S.PanelLayout(), ignored)
base.SetName("shaped_ignored")
p.pm.AddBasePlot(base)
plot = S.Plot("shaped_ignored", chunks.add("shaped_ignored"), "shaped_ignored")
fill(plot, base_rows("1d_wide_ratio_pull", SHAPE), DEFAULT)
p.pm.AddPlot(plot)
plots["shaped_ignored"] = ("base plot 1d_wide_ratio_pull with shapes that are not positive and finite", base_rows("1d_wide_ratio_pull", SHAPE), DEFAULT)
sizes = S.PanelLayout()
for setter, calls in INVALID_LENGTHS.items():
    for args in calls:
        getattr(sizes, setter)(*args)
base = S.PlotManager.MakeBasePlot("1d", sizes)
base.SetName("lengths_ignored")
p.pm.AddBasePlot(base)
plot = S.Plot("lengths_ignored", chunks.add("lengths_ignored"), "lengths_ignored")
fill(plot, base_rows("1d", SHAPE), DEFAULT)
p.pm.AddPlot(plot)
plots["lengths_ignored"] = ("base plot 1d with layout lengths that are not valid", base_rows("1d", SHAPE), DEFAULT)
layouts = [(f"layout {describe(case)}", generated_rows(case), generated_lengths(case)) for case in generated]
layouts += [(f"layout of {what}", rows, EXPLICIT_LENGTHS.get(what, DEFAULT)) for what, rows in EXPLICIT.items()]
for i, (what, rows, lengths) in enumerate(layouts):
    try:
        plot = S.Plot(f"layout{i}", layout_of(rows, lengths))
    except Exception as e:  # noqa: BLE001 (the message is the finding)
        check(f"layout accepted: {what}", False, str(e))
        continue
    plot.SetGroup(chunks.add(f"layout{i}"))
    fill(plot, rows, lengths)
    p.pm.AddPlot(plot)
    plots[f"layout{i}"] = (what, rows, lengths)

for what, (rows, message) in INVALID.items():
    try:
        S.Plot("invalid", S.PanelLayout(rows) if what != "a colour-scale cell without width" else S.PanelLayout(rows).SetColorScaleMargin(0.0))
        check(f"invalid layout throws: {what}", False, "no exception")
    except Exception as e:  # noqa: BLE001
        check(f"invalid layout throws: {what}", message in str(e), str(e))
for what, (call, message) in INVALID_SETTERS.items():
    try:
        call()
        check(f"invalid value throws: {what}", False, "no exception")
    except Exception as e:  # noqa: BLE001
        check(f"invalid value throws: {what}", message in str(e), str(e))
shapes = python("import SciRooPlot as S\nshape = S.BasePlotShape()\nfor value in (0.0, -1.0, float('nan'), float('inf')):\n    shape.SetAspectRatio(value).SetRatioHeight(value).SetPullHeight(value)\nS.PlotManager.MakeBasePlot('1d_wide_ratio_pull', S.PanelLayout(), shape)\n")
for setter in ("SetAspectRatio", "SetRatioHeight", "SetPullHeight"):
    warned = [line for line in shapes.warnings() if f"BasePlotShape::{setter}(" in line and "is ignored" in line]
    check(f"BasePlotShape::{setter} with 0, a negative, NaN or infinite value warns and keeps its value", len(warned) == 4 and shapes.rc == 0, shapes.tail())
code = "".join(f"sizes.{setter}({', '.join(f'float({str(a)!r})' for a in args)})\n" for setter, calls in INVALID_LENGTHS.items() for args in calls)
lengths = python("import SciRooPlot as S\nsizes = S.PanelLayout()\n" + code)
for setter, calls in INVALID_LENGTHS.items():
    warned = [line for line in lengths.warnings() if f"PanelLayout::{setter}" in line and "is ignored: it is" in line]
    check(f"PanelLayout::{setter} with an invalid length warns and keeps its value", len(warned) == sum(map(len, calls)) and lengths.rc == 0, lengths.tail())
refused = python("import SciRooPlot as S\n" + "".join(f"S.PlotManager.MakeBasePlot({name!r})\n" for name in REFUSED))
for name in REFUSED:
    lines = [line for line in refused.errors() if f"base plot {name}:" in line or f"Base plot {name} " in line]
    check(f"base plot name refused with an error: {name}", len(lines) == 1, refused.tail())

run = chunks.plot(p)
check("the plot runs wrote their files", not run.unwritten(), " | ".join(run.unwritten()))
errors = run.errors()
check("no errors", not errors, " | ".join(errors[:5]))
check("no warnings", not run.lines("WARN"), " | ".join(run.lines("WARN")[:5]))
for name, (what, rows, lengths) in plots.items():
    want = model(rows, lengths)
    canvas = run.canvas(name)
    if not canvas:
        check(f"plot made: {what}", False, "not in the output")
        continue
    W, H = canvas.GetWw(), canvas.GetWh()
    check(f"canvas size: {what}", (W, H) == want["canvas"], f"{W} x {H}, expected {want['canvas'][0]} x {want['canvas'][1]}")
    found = {int(o.GetName()[4:]): o for o in canvas.GetListOfPrimitives() if o.InheritsFrom("TPad") and o.GetName().startswith("Pad_")}
    expectedPads = {i for i, (kind, _) in want["pads"].items()}
    check(f"pads numbered in reading order: {what}", set(found) == expectedPads, f"pads {sorted(found)}, expected {sorted(expectedPads)}")
    rects, frames = {}, {}
    for padID, pad in found.items():
        x0, y1 = pad.GetXlowNDC() * W, (1 - pad.GetYlowNDC()) * H  # y from the top
        w, h = pad.GetWNDC() * W, pad.GetHNDC() * H
        rects[padID] = (x0, x0 + w, y1 - h, y1)
        if padID not in want["pads"]:
            continue
        kind, frame = want["pads"][padID]
        got = (x0 + w * pad.GetLeftMargin(), x0 + w * (1 - pad.GetRightMargin()), y1 - h * (1 - pad.GetTopMargin()), y1 - h * pad.GetBottomMargin())
        if kind == "scale":
            got = (x0, x0 + w, got[2], got[3])  # a colour-scale cell is its column, its margins those of the frames it belongs to
        if kind == "panel":
            frames[padID] = got
        if kind != "empty":
            check(f"frame of pad {padID} where the lengths put it: {what}", all(abs(a - b) < 0.01 for a, b in zip(got, frame)), f"left, right, top, bottom {tuple(round(v, 2) for v in got)}, expected {frame}")
    # pads may share the margins around an empty corner, but none covers the frame of another panel (a filled pad would hide its data)
    overlaps = [(a, b) for a in rects for b in frames if a != b and min(rects[a][1], frames[b][1]) - max(rects[a][0], frames[b][0]) > 0.01 and min(rects[a][3], frames[b][3]) - max(rects[a][2], frames[b][2]) > 0.01]
    check(f"no pad covers the frame of another one: {what}", not overlaps, f"pad, frame of pad: {overlaps}")
    outside = [i for i, (x0, x1, y0, y1) in rects.items() if x0 < -0.01 or y0 < -0.01 or x1 > W + 0.01 or y1 > H + 0.01]
    check(f"pads inside the canvas: {what}", not outside, str(outside))
finish()
