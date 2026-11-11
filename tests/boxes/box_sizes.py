"""A box (legend, text, statistics box) is as large as what it shows, as painted in the image the plot is saved as: its rows lie inside
its outline, apart from it (also rows with drawn parts below or above the line, such as #frac), which leaves them no more room than
a margin; the outline of a box is a line (no shadow); a text of a font sized in pixels
(precision 3) has the same size in pixels in any canvas, and so has its box where the smaller side of the pad is the same; at bitmapscale 2
everything of a box is twice as large in pixels (within the few % rendered text grows more); an automatic box wider than the frame is
warned about, so is a box at a position set for it that reaches beyond its pad (drawn, cut off at the pad).
Cases: legends (one column, two columns), texts and statistics boxes × a font sized relative to the pad (42) or in pixels (43) × plain
text, TLatex with drawn parts (#sqrt, #frac), unicode, three rows × a wide, square and tall canvas × bitmapscale 1 and 2.
Reference: the png of the plot: the box's outline (drawn red) and its rows (black) found by their colour.
Covers: a #sqrt row measured with a few pixels of garbage, shadows at bitmapscale 2, a legend row with #frac touching the bottom of the outline in a pad taller than wide, precision-3 texts in
tall pads, boxes at bitmapscale 2, and the named cases.
"""

import numpy as np

from srptest import ROOT, Project, S, bitmap, cases, check, describe, finish

CONTENTS = {
    "plain": ["plain text of some length"],
    "latex": ["#sqrt{#it{s}} = 13.6 TeV, #frac{1}{2} #it{p}_{T}^{2}"],
    "unicode": ["ünïcødé → ±∞ µ"],
    "mathtext": ["\\sqrt{s} = 13\\ \\mathrm{TeV}"],  # a backslash: drawn by TMathText
    "rows": ["first row", "a second, longer row", "third"],
}
CANVASES = {"wide": (1000, 600), "square": (600, 600), "tall": (600, 1000)}
FONTS = {42: 0.04, 43: 24.0}
BOXES = ("legend", "legend_columns", "text", "stats")


def valid(case):
    return case["box"] != "stats" or case["content"] == "plain"  # a statistics box shows its values


todo = cases({"box": list(BOXES), "font": list(FONTS), "content": list(CONTENTS), "canvas": list(CANVASES)}, valid, always=lambda c: c["content"] == "plain" or c["canvas"] == "tall")

p = Project("box_sizes")
h = ROOT.TH1D("h", "", 20, 0.0, 10.0)
for b in range(1, 21):
    h.SetBinContent(b, 0.05)
    h.SetBinError(b, 0.01)
p.source("s", {"h": h})
for i, case in enumerate(todo):
    plot = S.Plot(f"b{i}", "sizes", "1d")
    plot.SetDimensions(*CANVASES[case["canvas"]])
    pad = plot[1]
    rows = CONTENTS[case["content"]]
    data = pad.AddData("h", "s", rows[0]).SetColor(ROOT.kBlue)
    pad["Y"].SetRange(0.0, 1.0)
    font, size = case["font"], FONTS[case["font"]]
    if case["box"] == "stats":
        box = S.StatsBox()
        box.SetPosition(0.2, 0.9)
        data.SetShowStats("emr").SetStatsBox(box)
    elif case["box"] == "text":
        box = pad.AddText(0.2, 0.9, rows)
    else:
        box = pad.AddLegend(0.2, 0.9)
        for row in rows[1:] + (["fourth", "fifth"] if case["box"] == "legend_columns" else []):
            box.AddEntry(row)
        if case["box"] == "legend_columns":
            box.SetNumColumns(2)
    box.SetPadCoordinates().SetText(font, size, ROOT.kBlack).SetBorder(ROOT.kSolid, 1, ROOT.kRed)
    if case["box"] == "stats":
        data.SetStatsBox(box)
    p.pm.AddPlot(plot)

# a row wider than the frame: an automatic box says so, a box at a position set for it reaching beyond its pad too
for name, position in (("too_long", ()), ("too_long_fixed", (0.2, 0.9))):
    plot = S.Plot(name, "long", "1d")
    plot[1].AddData("h", "s", "#it{p}_{T} " * 30)
    plot[1].AddLegend(*position)
    plot[1].AddText(*position, "#it{p}_{T} " * 30)
    p.pm.AddPlot(plot)

# texts with a backslash (drawn by TMathText) and an empty group, on which ROOT crashes: in every kind of text, and plots after it
EMPTY_GROUPS = {"legend row": "\\alpha {} row", "legend title": "\\LaTeX{} title", "text": "{{ }}\\beta text", "axis title": "\\gamma{} axis", "stats title": "\\delta {} stats"}
plot = S.Plot("empty_groups", "groups", "1d")
plot[1].AddData("h", "s", EMPTY_GROUPS["legend row"]).SetShowStats("e", EMPTY_GROUPS["stats title"])
plot[1].AddLegend(EMPTY_GROUPS["legend title"])
plot[1].AddText(EMPTY_GROUPS["text"])
plot[1]["X"].SetTitle(EMPTY_GROUPS["axis title"])
p.pm.AddPlot(plot)
plot = S.Plot("after_groups", "groups", "1d")
plot[1].AddData("h", "s")
p.pm.AddPlot(plot)

images = {}
for scale in (1, 2):
    p.srp("bitmapscale", str(scale))
    run = p.plot("sizes", mode="png")
    check(f"bitmapscale {scale}: the plots are saved without errors", not run.errors(), " | ".join(run.errors()[:3]))
    images[scale] = {i: bitmap(p.path("out", "sizes", f"b{i}.png")) for i in range(len(todo))}
p.srp("bitmapscale", "1")


def measure(img):
    """the outline (red) of the box: its bounding box and the thickness of each of its sides; the bounding box of what is drawn inside
    (anything not white or red); in pixels, rows from the top"""
    r, g, b = (img >> 16) & 0xFF, (img >> 8) & 0xFF, img & 0xFF
    red = (r > 180) & (g < 90) & (b < 90)
    rows, cols = np.nonzero(red)
    if not len(rows):
        return None
    top, bottom, left, right = rows.min(), rows.max(), cols.min(), cols.max()
    midRow, midCol = (top + bottom) // 2, left + 3

    def thickness(line):
        n = 0
        for v in line:
            if not v:
                break
            n += 1
        return n

    sides = (thickness(red[midRow, left:]), thickness(red[midRow, right::-1]), thickness(red[top:, midCol]), thickness(red[bottom::-1, midCol]))
    inner = (slice(top + sides[2], bottom - sides[3] + 1), slice(left + sides[0], right - sides[1] + 1))
    drawn = ((r < 200) | (g < 200) | (b < 200)) & ~red
    drawnRows, drawnCols = np.nonzero(drawn[inner])
    content = None
    if len(drawnRows):
        content = (drawnRows.min() + inner[0].start, drawnRows.max() + inner[0].start, drawnCols.min() + inner[1].start, drawnCols.max() + inner[1].start)
    # what is drawn in black just outside the outline, on its right and below it (a row running beyond the box)
    black = (r < 80) & (g < 80) & (b < 80)
    outsideBlack = black[top : bottom + 1, right + 1 : right + 25].sum() + black[bottom + 1 : bottom + 15, left : right + 1].sum()
    return {"box": (top, bottom, left, right), "sides": sides, "content": content, "outside": outsideBlack}


found = {}
for i, case in enumerate(todo):
    what = describe(case)
    for scale in (1, 2):
        img = images[scale][i]
        m = measure(img) if img is not None else None
        found[(i, scale)] = m
        where = f"{what} bitmapscale={scale}"
        if not m or not m["content"]:
            check(f"{where}: the box and its rows are drawn", False, "no outline" if not m else "nothing inside the outline")
            continue
        top, bottom, left, right = m["box"]
        cTop, cBottom, cLeft, cRight = m["content"]
        sides = m["sides"]
        check(f"{where}: the outline is a line, no shadow", sides[0] == sides[1] and sides[2] == sides[3], f"left {sides[0]}, right {sides[1]}, top {sides[2]}, bottom {sides[3]} pixels")
        gaps = (cLeft - left - sides[0], right - sides[1] - cRight, cTop - top - sides[2], bottom - sides[3] - cBottom)
        # also rows with drawn parts below or above the line (#frac), which TLegend does not centre in pads taller than wide
        check(f"{where}: the rows lie inside the outline, apart from it", min(gaps) >= 1 and not m["outside"], f"gaps left, right, top, bottom {gaps}; {m['outside']} black pixels just outside")
        # no more room than a margin: on each side at most the height of a row (the content height divided by the rows)
        nRows = len(CONTENTS[case["content"]]) if case["box"] in ("text", "legend") else 3 if case["box"] == "legend_columns" else 4
        rowHeight = (cBottom - cTop + 1) / nRows
        check(f"{where}: the box leaves no more room around its rows than a margin", max(gaps) <= 1.6 * rowHeight + 2 * scale, f"gaps left, right, top, bottom {gaps} for rows of {rowHeight:.0f} pixels")

for i, case in enumerate(todo):
    one, two = found[(i, 1)], found[(i, 2)]
    if not one or not two or not one["content"] or not two["content"]:
        continue
    sizes = [(m["box"][1] - m["box"][0], m["box"][3] - m["box"][2], m["content"][1] - m["content"][0], m["content"][3] - m["content"][2]) for m in (one, two)]
    ok = all(abs(b - 2 * a) <= max(6, 0.04 * 2 * a) for a, b in zip(*sizes))  # rendered text grows by a few % more than its size (hinting)
    check(f"{describe(case)}: at bitmapscale 2 the box and its rows are twice as large", ok, f"height, width of box and rows at scale 1 {sizes[0]}, at 2 {sizes[1]} pixels")

long = p.plot("long")
for box in ("legend", "text"):
    check(f"an automatic {box} wider than the frame is warned about", any(f"The {box} (" in m and "is larger than the space inside the frame" in m for m in long.messages("too_long", "long")), " | ".join(long.messages("too_long", "long")))
    check(f"a {box} at a position reaching beyond its pad is warned about", any(f"The {box} reaches beyond its pad" in m for m in long.messages("too_long_fixed", "long")), " | ".join(long.messages("too_long_fixed", "long")))
check("boxes at positions inside their pad give no such warning", not any("reaches beyond its pad" in m for m in run.warnings()), " | ".join(run.warnings()[:3]))

groups = p.plot("groups")
check("texts with an empty group and a backslash: the plot and the plots after it are made", groups.canvas("empty_groups", "groups") and groups.canvas("after_groups", "groups"), " | ".join(groups.errors()[:3]))
groupPad = groups.pad("empty_groups", "groups")
drawnTexts = []
if groupPad:
    for obj in groupPad.GetListOfPrimitives():
        if obj.InheritsFrom("TLegend") or obj.InheritsFrom("TPaveText"):
            drawnTexts += [str(entry.GetLabel() if obj.InheritsFrom("TLegend") else entry.GetTitle()) for entry in (obj.GetListOfPrimitives() if obj.InheritsFrom("TLegend") else obj.GetListOfLines())]
            if obj.InheritsFrom("TLegend") and obj.GetHeader():
                drawnTexts.append(str(obj.GetHeader()))
        elif obj.InheritsFrom("TH1"):
            drawnTexts.append(str(obj.GetXaxis().GetTitle()))
groupMessages = groups.messages("empty_groups", "groups")
for kind, text in EMPTY_GROUPS.items():
    shown = text.replace("{{ }}", "").replace("{}", "")
    check(f"{kind} '{text}': drawn as '{shown}', warned once", shown in drawnTexts and sum(f"Text '{text}' has an empty group" in m for m in groupMessages) == 1, f"drawn {drawnTexts}; {' | '.join(groupMessages)}")

# the same text in pads whose smaller side is the same: the same box in pixels
for i, case in enumerate(todo):
    if case["canvas"] != "square":
        continue
    j = next((k for k, c in enumerate(todo) if c == {**case, "canvas": "tall"}), None)
    if j is None or not found[(i, 1)] or not found[(j, 1)]:
        continue
    a, b = found[(i, 1)]["box"], found[(j, 1)]["box"]
    check(f"{describe(case)}: as large in a tall canvas as in a square one", abs((a[3] - a[2]) - (b[3] - b[2])) <= 2 and abs((a[1] - a[0]) - (b[1] - b[0])) <= 2, f"height, width square {(a[1] - a[0], a[3] - a[2])}, tall {(b[1] - b[0], b[3] - b[2])} pixels")
finish()
