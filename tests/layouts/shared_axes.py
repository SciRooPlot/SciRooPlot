"""Pads sharing an axis (touching panels of a layout: x within a column, y within a row, z of a 2d grid with one colour scale; or groups
declared with Plot::ShareAxis) show it with one scale: without a range set, the union of the ranges the pads get on their own (with their
room), for every pair of data kinds and data variants, on linear and log scales; the colour-scale cell of a z group shows that range, the
panels show no scale of their own. The colour axis of a z group is z, for a 3d histogram its contents (C): it shares their range (a log
scale of the group does not reach it, with a warning); a TF3 has no colour scale.
A range, log scale or time format set in one pad of the group or in pad 0 holds for all of them (differing ranges: the first pad's,
with a warning). 3D views and polar drawings share no x and y axes: they and the flat pads beside them keep their own ranges (an explicit
group warns that it leaves them out; a flat panel right of one warns that its hidden y axis is not shared). ShareAxis regroups: an empty list undoes the groups of the axis (the layout's too, with a warning
that the layout shows the axis once), a single pad leaves its group, a pad listed twice or a pad the plot does not have is warned about,
an unknown axis and pad 0 are refused when defined. Panels spanning other columns share no x with the narrow ones, panels apart by a
panel spacing still share. The title of a row (x) or column (y) of touching panels showing the same title is drawn once, under or beside
all of them (centred if the axis says so); panels with titles of their own keep them, and SetCommonTitles(false) keeps all titles.
Covers: shared axes, common titles, colour-scale cells, undoing the axes of touching panels warns, a
colour-scale cell naming its panel, 3d histograms in z groups, flat panels
beside 3d views keeping their labels hidden.
"""

from srptest import Chunks, DATA_1D, DATA_2D, DATA_3D, DATA_POLAR, Project, S, add_data, cases, check, close, data_objects, describe, finish, has_variant, hist_diff, python, source_of

FLAT_2D = tuple(k for k in DATA_2D)  # drawn as flat maps (colz) by default
VARIANTS = ("normal", "negative", "single", "huge")

# x or y shared by two pads (by the layout or by ShareAxis), the second pad drawing another kind in another variant
# (each pair of kinds once; the explicit groups with the same kind in both pads; log with the variants that have positive values)
FLAT = DATA_1D + FLAT_2D
pairs = cases(
    {"first": list(FLAT), "second": list(FLAT), "variant": list(VARIANTS), "axis": ["X", "Y"], "log": [False, True], "group": ["layout", "explicit"]},
    valid=lambda c: has_variant(c["second"], c["variant"]) and FLAT.index(c["first"]) <= FLAT.index(c["second"]) and (c["group"] == "layout" or c["first"] == c["second"]) and (not c["log"] or c["variant"] in ("normal", "huge")),
    named=[{"first": "hist", "second": "graph_errors", "variant": "normal", "axis": "Y", "log": False, "group": "layout", "why": "touching panels share y"}],
)
# z shared by two 2d pads (one colour scale in a cell, or ShareAxis between panels with own scales)
COLOURED = tuple(k for k in FLAT_2D + DATA_3D if not k.startswith("tf3"))  # drawn with a colour scale
zpairs = cases(
    {"first": list(COLOURED), "second": list(COLOURED), "variant": ["normal", "negative", "huge"], "log": [False, True], "group": ["cell", "explicit"]},
    valid=lambda c: has_variant(c["second"], c["variant"]) and COLOURED.index(c["first"]) <= COLOURED.index(c["second"]) and (not c["log"] or c["variant"] != "negative"),
    named=[{"first": "hist2d", "second": "hist3d", "variant": "normal", "log": True, "group": "cell", "why": "3d histograms in z groups"}],
)
# drawings without flat frame beside a flat one: nothing shared in x and y
views = cases({"view": list(DATA_3D + DATA_POLAR) + ["lego"], "axis": ["X", "Y"], "group": ["layout", "explicit"]})
BASES = {("X", "layout"): "1d_1x2", ("Y", "layout"): "1d_2x1", ("X", "explicit"): "1d_1x2_gap", ("Y", "explicit"): "1d_2x1_gap", ("Z", "cell"): "2d_2x1", ("Z", "explicit"): "2d_2x1_gap"}

p = Project("shared_axes", bases=("1d_2x1", "1d_1x2", "1d_2x1_gap", "1d_1x2_gap", "2d", "2d_2x1", "2d_2x1_gap", "1d_3x1", "1d_2x2", "1d_2x1_ratio", "1d_ratio"))
p.inputs(VARIANTS)
chunks = Chunks("shared")


def pair_plot(name, base, first, second, variant, shared, explicit, axis, log):
    plot = S.Plot(name, chunks.add(name), base)
    add_data(plot[1], first, "inputs")
    add_data(plot[2], second, source_of(variant), variant)
    if explicit and shared:
        plot.ShareAxis(axis, [1, 2])
    if not explicit and not shared:
        plot.ShareAxis(axis, [])  # the reference: each pad on its own
    if log:
        plot[1][axis].SetLog()  # one pad of a group sets it for all; on their own both set it
        if not shared:
            plot[2][axis].SetLog()
    p.pm.AddPlot(plot)


for i, c in enumerate(pairs):
    for shared in (True, False):
        pair_plot(f"p{i}_{shared}", BASES[(c["axis"], c["group"])], c["first"], c["second"], c["variant"], shared, c["group"] == "explicit", c["axis"], c["log"])
for i, c in enumerate(zpairs):
    for shared in (True, False):
        pair_plot(f"z{i}_{shared}", BASES[("Z", c["group"])], c["first"], c["second"], c["variant"], shared, c["group"] == "explicit", "Z", c["log"])
for i, c in enumerate(views):
    for shared in (True, False):
        plot = S.Plot(f"v{i}_{shared}", chunks.add(f"v{i}_{shared}"), BASES[(c["axis"], c["group"])])
        if c["view"] == "lego":
            add_data(plot[1], "hist2d").SetDrawOption(S.lego)
        else:
            add_data(plot[1], c["view"])
        add_data(plot[2], "hist", "inputs_negative")
        if c["group"] == "explicit" and shared:
            plot.ShareAxis(c["axis"], [1, 2])
        if c["group"] == "layout" and not shared:
            plot.ShareAxis(c["axis"], [])
        p.pm.AddPlot(plot)


THREE = ("hist", "graph_errors", "hist")  # the second pad in the negative variant
SETTINGS = {  # name: (base plot, kinds of pads 1, 2, ..., setup)
    "range in one pad": ("1d_3x1", THREE, lambda pl: pl[2]["Y"].SetRange(0.5, 2000.0)),
    "range in pad 0": ("1d_3x1", THREE, lambda pl: pl[0]["Y"].SetRange(-7.0, 30.0)),
    "differing ranges": ("1d_3x1", THREE, lambda pl: (pl[1]["Y"].SetRange(0.0, 10.0), pl[3]["Y"].SetRange(0.0, 20.0))),
    "log in one pad": ("1d_3x1", THREE, lambda pl: pl[3]["Y"].SetLog()),
    "log in pad 0": ("1d_3x1", THREE, lambda pl: pl[0]["Y"].SetLog()),
    "room in pad 0": ("1d_3x1", THREE, lambda pl: pl[0]["Y"].SetRoomAboveMax(0.3)),
    "room in pad 0, each pad on its own": ("1d_3x1", THREE, lambda pl: (pl[0]["Y"].SetRoomAboveMax(0.3), pl.ShareAxis("Y", []))),
    "z range in a map": ("2d_2x1", ("hist2d", "hist3d"), lambda pl: pl[1]["Z"].SetRange(1.0, 30.0)),
    "C range in a 3d histogram": ("2d_2x1", ("hist2d", "hist3d"), lambda pl: pl[2]["C"].SetRange(1.0, 30.0)),
    "C title of 3d histograms": ("2d_2x1", ("hist3d", "hist3d"), lambda pl: pl[1]["C"].SetTitle("contents")),
    "time format in one pad": ("1d_1x2", ("hist", "hist"), lambda pl: pl[1]["X"].SetTimeFormat("%d.%m")),
    "undone": ("1d_3x1", THREE, lambda pl: pl.ShareAxis("Y", [])),
    "regrouped": ("1d_3x1", THREE, lambda pl: pl.ShareAxis("Y", [3])),
    "listed twice": ("1d_2x1_gap", ("hist", "graph_errors"), lambda pl: (pl.ShareAxis("Y", [1, 2]), pl.ShareAxis("Y", [2, 1]))),
    "missing pad": ("1d_2x1_gap", ("hist", "graph_errors"), lambda pl: pl.ShareAxis("Y", [1, 7, 2])),
    "pad drawing nothing": ("1d_3x1", ("hist", "graph_errors"), lambda pl: (pl[3].AddText(0.5, 0.5, "text"), pl.ShareAxis("X", [1, 3]))),
    "spacing": (S.PanelLayout([[S.Panel(), S.Panel().SpacingLeft(20.0)]]), ("hist", "graph_errors"), lambda pl: None),
    "span": (S.PanelLayout([[S.Panel(2, 1).Span(2)], [S.Gap()], [S.Panel(), S.Panel()]]), THREE, lambda pl: None),
    "ratio": ("1d_ratio", ("hist",), lambda pl: pl[2].AddRatio(["hist", "inputs"], ["hist_counts", "inputs"])),
    "ratios of a grid": ("1d_2x1_ratio", ("hist", "graph_errors", "hist", "graph"), lambda pl: None),
    "grid": ("1d_2x2", ("hist", "graph_errors", "hist", "graph"), lambda pl: None),
}
for i, (what, (base, kinds, setup)) in enumerate(SETTINGS.items()):
    if isinstance(base, str):
        plot = S.Plot(f"s{i}", chunks.add(f"s{i}"), base)
    else:
        plot = S.Plot(f"s{i}", base)
        plot.SetGroup(chunks.add(f"s{i}"))
    for padID, kind in enumerate(kinds, 1):
        add_data(plot[padID], kind, source_of("negative" if padID == 2 else "normal"), "negative" if padID == 2 else "normal")
    setup(plot)
    p.pm.AddPlot(plot)

# colour-scale cells and z groups of own layouts
CELLS = {  # name: (rows of the layout, kind by pad (variant huge for the pads listed in huge), huge, z groups)
    "a cell per row": ([[S.Panel(), S.Panel(), S.ColorScale()], [S.Panel(), S.Panel(), S.ColorScale()]], {1: "hist2d", 2: "hist2d", 4: "hist2d", 5: "hist2d"}, (5,), ([1, 2], [4, 5])),
    "a cell in front naming its panel": ([[S.ColorScale(2), S.Panel(), S.Panel()]], {2: "hist2d", 3: "hist2d"}, (3,), ([2, 3],)),
    "a cell without z group": ([[S.Panel(), S.Panel(), S.ColorScale()]], {1: "hist2d", 2: "hist2d"}, (2,), ()),
    "z shared without a cell": ([[S.Panel().ColorScale(), S.Gap(), S.Panel().ColorScale()]], {1: "hist2d", 2: "hist2d"}, (2,), ([1, 2],)),
    "map with projections and a hole": ([[S.Panel(1, 0.37), S.Hole().Span(2)], [S.Panel(), S.Panel(0.37, 1), S.ColorScale(2)]], {1: "hist", 2: "hist2d", 3: "hist"}, (2,), ()),
    "map with a projection set off": ([[S.Panel(), S.Panel(0.37, 1).SpacingLeft(10), S.ColorScale(1)]], {1: "hist2d", 2: "hist"}, (1,), ()),
}
for name, variant in (("zref_normal", "normal"), ("zref_huge", "huge")):  # the z range of a map on its own
    plot = S.Plot(name, chunks.add(name), "2d")
    add_data(plot[1], "hist2d", source_of(variant), variant)
    p.pm.AddPlot(plot)
for i, (what, (rows, kinds, huge, groups)) in enumerate(CELLS.items()):
    plot = S.Plot(f"cell{i}", S.PanelLayout(rows))
    plot.SetGroup(chunks.add(f"cell{i}"))
    for padID, kind in kinds.items():
        variant = "huge" if padID in huge else "normal"
        data = add_data(plot[padID], kind, source_of(variant), variant)
        if kind == "hist" and padID == 3 and what.startswith("map with projections"):
            data.SetDrawOption(S.hbars)
    for group in groups:
        plot.ShareAxis("Z", group)
    p.pm.AddPlot(plot)

# titles drawn once
TITLES = {  # name: (base plot, number of pads, x titles of the pads (y titles all "counts"), setup, expected common x titles, common y titles)
    "same titles": ("1d_3x1", 3, ("pt", "pt", "pt"), lambda pl: None, ["pt"], []),
    "centred": ("1d_3x1", 3, ("pt", "pt", "pt"), lambda pl: pl[0]["X"].SetTitleCenter(), ["pt"], []),
    "own titles": ("1d_3x1", 3, ("a", "b", "c"), lambda pl: None, [], []),
    "no common titles": ("1d_3x1_own", 3, ("pt", "pt", "pt"), lambda pl: None, [], []),
    "with a gap": ("1d_2x1_gap", 2, ("pt", "pt"), lambda pl: None, [], []),
    "grid": ("1d_2x2", 4, ("pt",) * 4, lambda pl: None, ["pt"], ["counts"]),
    "grid with ratios": ("1d_2x1_ratio", 4, ("pt",) * 4, lambda pl: None, ["pt"], ["counts"]),
}
own = S.PlotManager.MakeBasePlot("1d_3x1", S.PanelLayout().SetCommonTitles(False))
own.SetName("1d_3x1_own")
p.pm.AddBasePlot(own)
for i, (what, (base, nPads, titles, setup, _, _)) in enumerate(TITLES.items()):
    plot = S.Plot(f"t{i}", chunks.add(f"t{i}"), base)
    for padID, title in enumerate(titles, 1):
        add_data(plot[padID], "hist")
        plot[padID]["X"].SetTitle(title)
        plot[padID]["Y"].SetTitle("counts")
    setup(plot)
    p.pm.AddPlot(plot)

refused = python("import SciRooPlot as S\nplot = S.Plot('a', 'b', '1d_2x1')\nplot.ShareAxis('W', [1, 2])\nplot.ShareAxis('Y', [0, 1])\n")
check("ShareAxis with an unknown axis warns when defined", any("ShareAxis needs the axis 'X', 'Y' or 'Z' (got 'W')" in line for line in refused.lines()), refused.tail())
check("ShareAxis with pad 0 warns when defined", any("pad 0" in line for line in refused.warnings()), refused.tail())

run = chunks.plot(p)
check("the plot runs wrote their files", not run.unwritten(), " | ".join(run.unwritten()))
errors = run.errors()
check("no errors", not errors, " | ".join(errors[:5]))


def axis_range(name, pad, axis):
    if axis == "Z":
        frame = run.frame_hist(name, pad)
        return (frame.GetMinimum(), frame.GetMaximum()) if frame else None
    frame = run.frame(name, pad)
    return frame[axis] if frame else None


def same(a, b):
    return a is not None and b is not None and close(a[0], b[0], 1e-6) and close(a[1], b[1], 1e-6)


def union(a, b):
    return (min(a[0], b[0]), max(a[1], b[1]))


def own_scale(name, padID):
    frame = run.frame_hist(name, padID)
    return bool(frame and frame.GetListOfFunctions().FindObject("palette"))


def compare_pair(prefix, i, c, axis):
    what = describe(c)
    alone = [axis_range(f"{prefix}{i}_False", pad, axis) for pad in (1, 2)]
    shared = [axis_range(f"{prefix}{i}_True", pad, axis) for pad in (1, 2)]
    if None in alone + shared:
        check(f"plots made: {what}", False, " | ".join(line for line in run.errors() if f"{prefix}{i}_" in line)[:300])
        return
    want = union(*alone)
    check(f"both pads show the union of their own {axis} ranges: {what}", same(shared[0], want) and same(shared[1], want), f"{shared}, alone {alone}")
    if prefix == "z" and c["group"] == "cell":
        cell = run.pad(f"{prefix}{i}_True", 3)
        scale = cell.GetPrimitive("color_scale_pad_3") if cell else None
        check(f"the colour-scale cell shows the union: {what}", scale and same((scale.GetMinimum(), scale.GetMaximum()), want), f"{(scale.GetMinimum(), scale.GetMaximum()) if scale else 'no scale'}, expected {want}")
        check(f"the cell draws the scale of a flat map (no 3D view): {what}", scale and scale.GetDimension() == 2 and not cell.GetView(), f"dimension {scale.GetDimension()}, view {bool(cell.GetView())}" if scale else "no scale")
        check(f"the panels draw no colour scale of their own: {what}", not any(own_scale(f"{prefix}{i}_True", pad) for pad in (1, 2)), str([own_scale(f"{prefix}{i}_True", pad) for pad in (1, 2)]))


for prefix, todo, axisOf in (("p", pairs, lambda c: c["axis"]), ("z", zpairs, lambda c: "Z")):
    for i, c in enumerate(todo):
        with run.held(colours=False):  # thousands of canvases: each pair frees those it read
            compare_pair(prefix, i, c, axisOf(c))
# a log colour axis does not reach a 3d histogram of the group (ROOT colours its boxes linearly): said once
linear = run.warnings("whose boxes ROOT colours linearly")
for i, c in enumerate(zpairs):
    expected = c["log"] and c["first"] not in DATA_3D and c["second"] in DATA_3D
    warned = [line for line in linear if f"Plot z{i}_True:" in line]
    check(f"a log scale not reaching the 3d histogram is warned about{'' if expected else ' only there'}: {describe(c)}", len(warned) == (1 if expected else 0) and (not expected or "pad 2" in warned[0]), str(warned))
leftOut = run.warnings("is left out of the pads sharing")
besides = run.warnings("are hidden beside pad")
for i, c in enumerate(views):
    what = describe(c)
    alone = [run.frame(f"v{i}_False", pad) for pad in (1, 2)]
    shared = [run.frame(f"v{i}_True", pad) for pad in (1, 2)]
    if None in alone + shared:
        check(f"plots made: {what}", False, " | ".join(line for line in run.errors() if f"v{i}_" in line)[:300])
        continue
    check(f"the flat pad keeps its own {c['axis']} range beside a drawing without flat frame: {what}", same(shared[1][c["axis"]], alone[1][c["axis"]]), f"{shared[1]}, alone {alone[1]}")
    warned = [line for line in leftOut if f"Plot v{i}_True:" in line]
    if c["group"] == "explicit":
        check(f"an explicit group warns that it leaves out the pad without flat frame: {what}", len(warned) == 1 and "pad 1" in warned[0], str(warned))
    else:
        check(f"the layout leaves it out silently: {what}", not warned, str(warned))
    # the flat pad right of the drawing keeps its y labels hidden (no room for them), with a warning; below it (x) the drawing shows its own
    for shared in (True, False):
        warned = [line for line in besides if f"Plot v{i}_{shared}:" in line]
        expected = c["group"] == "layout" and c["axis"] == "Y"
        check(
            f"a flat pad beside the drawing warns that its hidden {c['axis'].lower()} axis is not shared{'' if expected else ' only where it touches'}: {what} shared={shared}",
            len(warned) == (1 if expected else 0) and (not expected or "the y labels and title of pad 2 are hidden beside pad 1" in warned[0]),
            str(warned),
        )

names = {what: f"s{i}" for i, what in enumerate(SETTINGS)}


def ranges(what, axis="Y", pads=(1, 2, 3)):
    return [axis_range(names[what], pad, axis) for pad in pads]


def warned(what, text):
    return [line for line in run.warnings(text) if f"Plot {names[what]}:" in line]


check("a range set in one pad holds for the whole group", all(same(r, (0.5, 2000.0)) for r in ranges("range in one pad")), str(ranges("range in one pad")))
check("a range set in pad 0 holds for the whole group", all(same(r, (-7.0, 30.0)) for r in ranges("range in pad 0")), str(ranges("range in pad 0")))
check("differing ranges: the first pad's holds for all", all(same(r, (0.0, 10.0)) for r in ranges("differing ranges")), str(ranges("differing ranges")))
check("differing ranges are warned about", len(warned("differing ranges", "set different maxima")) == 1, str(run.warnings("set different")))
for what in ("z range in a map", "C range in a 3d histogram"):
    hists = [run.frame_hist(names[what], pad) for pad in (1, 2)]
    got = [(h.GetMinimum(), h.GetMaximum()) if h else None for h in hists]
    check(f"{what}: the colour axis of the map (z) and of the 3d histogram (its contents) has the range", all(g and same(g, (1.0, 30.0)) for g in got), str(got))
    zAxis = hists[1].GetZaxis() if hists[1] else None
    check(f"{what}: the binned z axis of the 3d histogram keeps its range", zAxis and zAxis.GetFirst() == 1 and zAxis.GetLast() == zAxis.GetNbins(), f"bins {zAxis.GetFirst()} to {zAxis.GetLast()} of {zAxis.GetNbins()}" if zAxis else "no frame")
cell = run.pad(names["C title of 3d histograms"], 3)
scale = cell.GetPrimitive("color_scale_pad_3") if cell else None
check("the cell of 3d histograms shows the title of their contents", scale and scale.GetZaxis().GetTitle() == "contents", scale.GetZaxis().GetTitle() if scale else "no scale")
for what in ("log in one pad", "log in pad 0"):
    pads = [run.pad(names[what], pad) for pad in (1, 2, 3)]
    check(f"{what}: log in every pad of the group", all(q and q.GetLogy() for q in pads), str([q.GetLogy() if q else None for q in pads]))
    check(f"{what}: one range", all(same(r, ranges(what)[0]) for r in ranges(what)), str(ranges(what)))
room, alone = ranges("room in pad 0"), ranges("room in pad 0, each pad on its own")
want = union(union(alone[0], alone[1]), alone[2])
check("room in pad 0: the union of the ranges the pads get with their room on their own", all(same(r, want) for r in room), f"{room}, expected {want}")
frames = [run.frame_hist(names["time format in one pad"], pad) for pad in (1, 2)]
check("a time format set in one pad holds for the whole group", all(f and f.GetXaxis().GetTimeDisplay() and f.GetXaxis().GetTimeFormat().startswith("%d.%m") for f in frames), str([(f.GetXaxis().GetTimeDisplay(), f.GetXaxis().GetTimeFormat()) if f else None for f in frames]))
undone = ranges("undone")
check("ShareAxis(axis, {}) undoes the axes of touching panels", not same(undone[0], undone[1]), str(undone))
check("undoing the axes of touching panels warns that the layout shows the axis once", len(warned("undone", "the layout shows the Y axis of pads 1, 2, 3 once")) == 1, str(run.warnings("layout shows")))
regrouped = ranges("regrouped")
check("a single pad leaves its group, the others keep sharing", same(regrouped[0], regrouped[1]) and not same(regrouped[2], regrouped[0]), str(regrouped))
check("regrouping touching panels warns that the layout shows the axis once", len(warned("regrouped", "the layout shows the Y axis of pads 1, 2, 3 once")) == 1, str(run.warnings("layout shows")))
twice = ranges("listed twice", pads=(1, 2))
check("a pad listed in two groups: the later group holds", same(twice[0], twice[1]), str(twice))
check("a pad listed in two groups is warned about", len(warned("listed twice", "pad 2 is listed in two groups")) == 1, str(run.warnings("listed")))
missing = ranges("missing pad", pads=(1, 2))
check("a pad the plot does not have is left out, the others share", same(missing[0], missing[1]), str(missing))
check("a pad the plot does not have is warned about", len(warned("missing pad", "the plot has no pad 7")) == 1, str(run.warnings("no pad 7")))
check("a pad drawing nothing is left out of an explicit group with a warning", len(warned("pad drawing nothing", "pad 3 is left out of the pads sharing the X axis: it draws nothing")) == 1, str(run.warnings("left out")))
spaced = ranges("spacing", pads=(1, 2))
check("panels apart by a spacing still share y", same(spaced[0], spaced[1]), str(spaced))
spans = [axis_range(names["span"], pad, "X") for pad in (1, 2, 3)]
spansY = [axis_range(names["span"], pad, "Y") for pad in (2, 3)]
check("a panel spanning two columns shares no x with the narrow ones below the gap, these share y", not same(spans[0], spans[1]) and same(spansY[0], spansY[1]), f"x {spans}, y {spansY}")
ratio = [axis_range(names["ratio"], pad, "X") for pad in (1, 2)]
check("panel and ratio share x", same(ratio[0], ratio[1]), str(ratio))
inputs = data_objects()
reference = inputs["hist"].Clone("reference")
reference.Divide(inputs["hist_counts"])
drawn = run.objects(names["ratio"], 2)
check("the ratio pad of 1d_ratio draws the ratio (as TH1::Divide)", not hist_diff(reference, drawn[1] if len(drawn) > 1 else None), hist_diff(reference, drawn[1] if len(drawn) > 1 else None))
grid = {a: [axis_range(names["ratios of a grid"], pad, a) for pad in (1, 2, 3, 4)] for a in "XY"}
check("a grid with ratios: y per row, x per column", same(grid["Y"][0], grid["Y"][1]) and same(grid["Y"][2], grid["Y"][3]) and same(grid["X"][0], grid["X"][2]) and same(grid["X"][1], grid["X"][3]), str(grid))
grid = {a: [axis_range(names["grid"], pad, a) for pad in (1, 2, 3, 4)] for a in "XY"}
check("a 2x2 grid: y per row, x per column", same(grid["Y"][0], grid["Y"][1]) and same(grid["Y"][2], grid["Y"][3]) and same(grid["X"][0], grid["X"][2]) and same(grid["X"][1], grid["X"][3]), str(grid))
known = ("set different maxima", "the layout shows", "listed in two groups", "no pad 7", "is left out of the pads sharing", "whose boxes ROOT colours linearly", "are hidden beside pad")
extra = [line for line in run.lines("WARN") if not any(k in line for k in known)]
check("no other warnings", not extra, " | ".join(extra[:5]))

zalone = {v: axis_range(f"zref_{v}", 1, "Z") for v in ("normal", "huge")}


def cell_range(name, padID):
    cell = run.pad(name, padID)
    scale = cell.GetPrimitive(f"color_scale_pad_{padID}") if cell else None
    return (scale.GetMinimum(), scale.GetMaximum()) if scale else None


cells = {what: f"cell{i}" for i, what in enumerate(CELLS)}
both = union(zalone["normal"], zalone["huge"])
name = cells["a cell per row"]
check("a cell per row shows the z range of its row", same(cell_range(name, 3), zalone["normal"]) and same(cell_range(name, 6), both), f"{cell_range(name, 3)}, {cell_range(name, 6)}")
check("panels showing their scale in a cell draw none of their own", not any(own_scale(name, padID) for padID in (1, 2, 4, 5)))
name = cells["a cell in front naming its panel"]
check("a cell in front naming its panel shows the z range of its group", same(cell_range(name, 1), both), str(cell_range(name, 1)))
name = cells["a cell without z group"]
check("a cell without z group shows the scale of the panel on its left", same(cell_range(name, 3), zalone["huge"]) and same(axis_range(name, 1, "Z"), zalone["normal"]), f"cell {cell_range(name, 3)}, pad 1 {axis_range(name, 1, 'Z')}")
name = cells["z shared without a cell"]
check("z shared without a cell: both panels keep their scales, with the common range", own_scale(name, 1) and own_scale(name, 2) and same(axis_range(name, 1, "Z"), both) and same(axis_range(name, 2, "Z"), both), f"{axis_range(name, 1, 'Z')}, {axis_range(name, 2, 'Z')}")
for what in ("map with projections and a hole", "map with a projection set off"):
    name = cells[what]
    cell, mapPad = (4, 2) if "hole" in what else (3, 1)
    check(f"the cell shows the scale of the map it names: {what}", same(cell_range(name, cell), zalone["huge"]), str(cell_range(name, cell)))
    check(f"the map draws no scale of its own: {what}", not own_scale(name, mapPad))
name = cells["map with projections and a hole"]
check("map with projections and a hole: projection and map share x", same(axis_range(name, 1, "X"), axis_range(name, 2, "X")), f"{axis_range(name, 1, 'X')}, {axis_range(name, 2, 'X')}")

for i, (what, (base, nPads, titles, _, wantX, wantY)) in enumerate(TITLES.items()):
    canvas = run.canvas(f"t{i}")
    common = {a: [o for o in canvas.GetListOfPrimitives() if o.GetName().startswith(f"common_title_{a}")] if canvas else [] for a in "XY"}
    check(f"common x titles {wantX}: {what}", [o.GetTitle() for o in common["X"]] == wantX, str([o.GetTitle() for o in common["X"]]))
    check(f"common y titles {wantY}: {what}", [o.GetTitle() for o in common["Y"]] == wantY, str([o.GetTitle() for o in common["Y"]]))
    kept = [run.frame_hist(f"t{i}", pad).GetXaxis() for pad in range(1, nPads + 1)] if canvas else []
    if wantX:
        check(f"the panels lose their own x titles: {what}", all(not a.GetTitle() or a.GetTitleSize() == 0 for a in kept), str([(a.GetTitle(), a.GetTitleSize()) for a in kept]))
    else:
        check(f"the panels keep their own x titles: {what}", all(a.GetTitle() and a.GetTitleSize() > 0 for a in kept), str([(a.GetTitle(), a.GetTitleSize()) for a in kept]))
    if wantX and base == "1d_3x1":
        pads = [canvas.GetPrimitive(f"Pad_{pad}") for pad in (1, 3)]
        left = pads[0].GetAbsXlowNDC() + pads[0].GetLeftMargin() * pads[0].GetAbsWNDC()
        right = pads[1].GetAbsXlowNDC() + (1 - pads[1].GetRightMargin()) * pads[1].GetAbsWNDC()
        centred = what == "centred"
        x = common["X"][0].GetX()
        check(
            f"the common title {'centred under' if centred else 'at the right end of'} the row: {what}",
            abs(x - (0.5 * (left + right) if centred else right)) < 1e-3 and common["X"][0].GetTextAlign() == (22 if centred else 32),
            f"at {x:.4f} (row {left:.4f} to {right:.4f}), align {common['X'][0].GetTextAlign()}",
        )
finish()
