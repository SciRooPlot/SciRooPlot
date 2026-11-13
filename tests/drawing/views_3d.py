"""Data drawn in 3d (lego, surf, triangles and the points and lines of 2d graphs, 3d histograms and functions) is drawn into the 3d frame
the first data of its pad makes when it is drawn in 3d itself; data drawn flat (colours, contours, boxes, text) into a flat frame. Data
of the other kind is not drawn, with one warning, and the data after it keeps its number. In a 3d view no data covers the data drawn
before it with the background (ROOT paints the frame fill colour behind the box for 2d graphs drawn in 3d and the colour boxes of 3d
histograms). The edges ROOT draws around every box, bar, cell and triangle of a 3d view have width 1 unless the data sets one (or keeps
the width of its input); the faces of a 3d function are white unless it has a fill colour of its own. The points and lines of a 2d graph
in 3d paint only themselves, also when its z range goes below zero.
Covers: 3d views over a flat map and flat maps in a 3d view drawn silently, a 2d graph drawn as points or lines covering the data before
it, the colour boxes of a 3d histogram on walls in the palette maximum, wide black edges hiding the colours of small 3d boxes, black faces
of 3d functions, a plane of colour behind the coloured points and a mesh under the lines of a 2d graph reaching below zero.
"""

import re

from srptest import DATA_2D, DATA_3D, DATA_KINDS, ROOT, Project, S, add_data, cases, check, data_name, data_objects, describe, drawn_type, finish, in_chunks, pixels, rgba

OPTIONS = {  # aliases of what the data is drawn as, flat and in 3d (None: the default of the kind)
    "hist2d": ("colz", "cont", "box", "text", "lego", "surf", "surf_col"),
    "graph2d": ("colz", "cont", "triangles", "points", "line", "lego", "surf"),
    "function2d": ("colz", "cont", "lego", "surf"),
    "hist3d": (None, "box", "iso"),
    "function3d": (None, "iso"),
}
FRAMES = {  # first data of the pad: (kind, alias, data drawn after the second one as (kind, alias))
    "flat": ("hist2d", "colz", ("hist2d", "cont")),
    "view2d": ("hist2d", "lego", ("hist2d", "surf")),
    "view3d": ("hist3d", None, ("hist3d", "box")),
}
VIEWS = (("hist2d", "lego"), ("hist2d", "surf_col"), ("tf2", "surf"), ("graph2d", "triangles"), ("graph2d", "points"), ("graph2d_errors", "points"), ("graph2d", "line"), ("hist3d", "box"), ("hist3d", None), ("hist3d", "iso"))


def in_3d(kind, alias):
    """whether data of a kind is drawn in 3d with an alias (a TH2Poly is drawn flat for lego and surf, with a warning)"""
    if DATA_KINDS[kind][0] == 3:
        return True
    if DATA_KINDS[kind][2] >= {"polygons"}:
        return False
    return alias is not None and (alias.startswith(("lego", "surf", "triangles")) or (drawn_type(kind) == "graph2d" and alias in ("points", "line")))


def add(pad, kind, alias):
    data = add_data(pad, kind)
    return data.SetDrawOption(getattr(S, alias)) if alias else data


p = Project("views_3d")
p.inputs()
styled = {}  # inputs with a line width and a fill colour of their own
for kind, obj in data_objects().items():
    if DATA_KINDS[kind][0] > 1 and obj.InheritsFrom("TAttLine"):
        styled[kind] = obj.Clone()
        styled[kind].SetLineWidth(4)
        styled[kind].SetFillColor(ROOT.kOrange)
p.source("styled", styled)

# data drawn after a first data that makes a flat or a 3d frame
frames = cases(
    {"frame": list(FRAMES), "kind": list(DATA_2D + DATA_3D), "alias": sorted({a or "" for o in OPTIONS.values() for a in o})},
    valid=lambda c: (c["alias"] or None) in OPTIONS[drawn_type(c["kind"])],
    always=lambda c: c["kind"] in ("hist2d", "graph2d", "hist3d"),
    named=[
        {"frame": "flat", "kind": "hist2d", "alias": "lego", "why": "a lego plot over a flat map"},
        {"frame": "flat", "kind": "hist3d", "alias": "", "why": "a 3d histogram over a flat map"},
        {"frame": "view2d", "kind": "hist2d", "alias": "colz", "why": "a flat map in a lego plot"},
        {"frame": "view2d", "kind": "hist2poly", "alias": "colz", "why": "a TH2Poly in a 3d frame"},
        {"frame": "flat", "kind": "graph2d", "alias": "triangles", "why": "triangles of a 2d graph over a flat map"},
        {"frame": "view2d", "kind": "graph2d", "alias": "triangles", "why": "triangles of a 2d graph on top of a lego plot"},
    ],
)
for i, case in enumerate(frames):
    first, firstAlias, after = FRAMES[case["frame"]]
    plot = S.Plot(f"f{i}", "frames", "2d")
    add(plot[1], first, firstAlias)
    add(plot[1], case["kind"], case["alias"] or None)
    add(plot[1], *after)
    p.pm.AddPlot(plot)

# a 3d view drawn after another one
pairs = cases({"first": list(range(len(VIEWS))), "second": list(range(len(VIEWS)))}, always=lambda c: VIEWS[c["second"]][1] in ("points", "line"))
for i, case in enumerate(pairs):
    plot = S.Plot(f"v{i}", "pairs", "2d")
    add(plot[1], *VIEWS[case["first"]])
    add(plot[1], *VIEWS[case["second"]])
    p.pm.AddPlot(plot)

# the edges of 3d views and the faces of 3d functions: the data alone, with a line width or fill colour of its own, keeping its input's
STYLES = ("none", "own", "input")
looks = cases(
    {"kind": list(DATA_2D + DATA_3D), "alias": sorted({a or "" for o in OPTIONS.values() for a in o}), "style": list(STYLES)},
    valid=lambda c: (c["alias"] or None) in OPTIONS[drawn_type(c["kind"])] and in_3d(c["kind"], c["alias"] or None) and not (c["style"] == "input" and c["kind"] not in styled),
    always=lambda c: c["kind"] in ("hist3d", "tf3"),
)
for i, case in enumerate(looks):
    plot = S.Plot(f"l{i}", "looks", "2d")
    source = "styled" if case["style"] == "input" else "inputs"
    data = plot[1].AddData(case["kind"], source) if "formula" not in DATA_KINDS[case["kind"]][2] else add_data(plot[1], case["kind"])
    if "projected" in DATA_KINDS[case["kind"]][2]:
        data.Project([0, 1])
    if case["alias"]:
        data.SetDrawOption(getattr(S, case["alias"]))
    if case["style"] == "own":
        data.SetLineWidth(3.0).SetFillColor(ROOT.kCyan)
    elif case["style"] == "input":
        data.SetKeepInputStyle()
    p.pm.AddPlot(plot)

# the points and lines of a 2d graph alone in its 3d view, also when its z range goes below zero: ROOT then paints the cells of the empty
# histogram they are drawn from as well, at their coordinates in the view (seen for x and y from 0 to 1)
below = {}
for name, cls in (("below", ROOT.TGraph2D), ("below_errors", ROOT.TGraph2DErrors)):
    graph = cls(36)
    graph.SetName(name)
    for n in range(36):
        graph.SetPoint(n, n % 6, n // 6, (n % 6) * (n // 6) - 3.0)
        if cls == ROOT.TGraph2DErrors:
            graph.SetPointError(n, 0.1, 0.1, 1.0)
    below[name] = graph
p.source("below", below)
lows = cases(
    {"graph": list(below), "alias": ["points", "points_col", "line"]},
    always=lambda c: True,
    named=[{"graph": "below_errors", "alias": "points_col", "why": "coloured points of a 2d graph reaching below zero"}],
)
for i, case in enumerate(lows):
    plot = S.Plot(f"z{i}", "lows", "2d")
    plot[1].AddData(case["graph"], "below").SetDrawOption(getattr(S, case["alias"]))
    p.pm.AddPlot(plot)
run = p.plot()

for i, case in enumerate(frames):
    what = describe(case)
    first, firstAlias, after = FRAMES[case["frame"]]
    alias = case["alias"] or None
    names = [obj.GetName().split(":", 2)[:2] for obj, _ in run.drawn(f"f{i}", "frames")]
    drawn = in_3d(case["kind"], alias) == in_3d(first, firstAlias)
    name = data_name(case["kind"])
    want = [["1", first], ["2", name], ["3", after[0]]] if drawn else [["1", first], ["3", after[0]]]
    check(f"drawn only into a frame of its kind, the next data keeping its number: {what}", names == want, f"drawn {names}, expected {want}")
    messages = [line for line in run.messages(f"f{i}", "frames") if "[ WARN" in line or "[WARN" in line]
    refused = [line for line in messages if line.rstrip(".").endswith("It is not drawn")]
    check(f"{0 if drawn else 1} warning that it is not drawn: {what}", len(refused) == (0 if drawn else 1) and all(f" {name} " in line for line in refused), " | ".join(messages))


def erased(i, case):
    """the share of the pixels of the first data that the second one turns back into the background (frame, axes and box)"""
    title = f"the second view keeps the first visible: {describe(dict(first=VIEWS[case['first']], second=VIEWS[case['second']]))}"
    canvas = run.canvas(f"v{i}", "pairs")
    if not canvas:
        return [(title, False, "plot not made")]
    background, _ = pixels(canvas, lambda obj: not re.match(r"\d+:", obj.GetName()))
    first, _ = pixels(run.canvas(f"v{i}", "pairs"), lambda obj: not obj.GetName().startswith("2:"))
    both, _ = pixels(run.canvas(f"v{i}", "pairs"), lambda obj: True)
    shown = first != background
    share = ((both == background) & shown).sum() / max(1, shown.sum())
    return [(title, share < 0.3, f"{share:.0%} of the first data covered by the background")]  # covered by the faces of the second: 0-20 %


in_chunks(pairs, erased)


def painted(i, case):
    """the share of the plot that a 2d graph drawn alone as points or lines in 3d paints (its points, error bars and lines: a few percent)"""
    title = f"paints only its points and lines: {describe(case)}"
    canvas = run.canvas(f"z{i}", "lows")
    if not canvas:
        return [(title, False, "plot not made")]
    background, _ = pixels(canvas, lambda obj: not re.match(r"\d+:", obj.GetName()))
    alone, _ = pixels(run.canvas(f"z{i}", "lows"), lambda obj: True)
    share = (alone != background).mean()
    return [(title, share < 0.08, f"{share:.1%} of the plot painted by the data")]


in_chunks(lows, painted)

for i, case in enumerate(looks):
    what = describe(case)
    drawn = run.drawn(f"l{i}", "looks")
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    obj, opt = drawn[0]
    if case["kind"] in DATA_3D or any(word in opt for word in ("LEGO", "SURF", "TRI")):
        width = {"none": 1, "own": 3, "input": 4}[case["style"]]
        check(f"edges of width {width}: {what}", obj.GetLineWidth() == width, f"width {obj.GetLineWidth()}")
    if drawn_type(case["kind"]) == "function3d":
        fill = {"none": ROOT.kWhite, "own": ROOT.kCyan, "input": ROOT.kOrange}[case["style"]]
        check(f"faces in {rgba(fill)}: {what}", rgba(obj.GetFillColor()) == rgba(fill), f"faces {rgba(obj.GetFillColor())}")
    pad = run.pad(f"l{i}", "looks")
    if drawn_type(case["kind"]) == "hist3d" and "BOX2" in opt or (drawn_type(case["kind"]) == "graph2d" and case["alias"] in ("triangles", "points", "line")):
        check(f"no background painted behind the box: {what}", pad.GetFrameFillColor() == 0, f"frame fill colour {pad.GetFrameFillColor()}")
check("no warnings but those of data not drawn and TH2Poly drawn flat", all("It is not drawn" in w or "drawing it as col" in w for w in run.warnings()), " | ".join(w for w in run.warnings() if "It is not drawn" not in w)[:500])
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
