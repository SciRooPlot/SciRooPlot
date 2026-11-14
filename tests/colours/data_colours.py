"""Data takes the colours it does not set itself from one entry of the default lists: the n-th data item of a pad that takes a colour gets
entry n of the marker, line and fill lists (repeating) of its pad, else of pad 0, else SciRooPlot's colours (DEFAULT_COLORS; fills only
where a fill style is set for the data, the pad or pad 0). Each colour setter sets only its own part (SetColor all three); data taking
any of the three moves the next data on to the next entry, data setting all three or keeping the style of its input (SetKeepInputStyle,
its colours as stored) does not, nor does the reference function of the pad. Alpha setters change only the transparency.
Data that is not drawn (a graph without points) takes its entry too, so the data after it gets the colours it would get if it were drawn.
Every data kind, with lists of the pad, of pad 0 and of both, separate marker, line and fill lists of different lengths, and plots
without a base plot.
Covers: each colour setter sets only its own part, marker and line of one data item from the same entry.
"""

from srptest import DATA_KINDS, DEFAULT_COLORS, ROOT, Project, S, add_data, base_of, cases, check, colour_diff, data_objects, describe, drawn_type, finish, source_of

L3 = [ROOT.kRed + 1, "#00aa55", (0.2, 0.3, 0.9)]  # every colour form
L3_PAD0 = [ROOT.kOrange + 7, ROOT.kCyan + 2, ROOT.kMagenta + 3]
MARKERS = [ROOT.kRed + 1, ROOT.kRed + 2, ROOT.kRed + 3]
LINES = [ROOT.kGreen + 1, ROOT.kGreen + 2, ROOT.kGreen + 3, ROOT.kGreen + 4]
FILLS = [ROOT.kBlue - 7, ROOT.kBlue - 9]
SET = {"marker": ROOT.kViolet + 1, "line": ROOT.kSpring - 6, "fill": ROOT.kAzure + 7, "all": "#d06010"}
KEPT = {"marker": ROOT.kGreen + 3, "line": ROOT.kOrange + 7, "fill": ROOT.kCyan + 2}  # stored with the inputs of the source "styled"

# lists of the pads: (setup of the plot, base plot used, marker list, line list, fill list; None: not set)
PADS = {
    "srp": (lambda pl: None, True, None, None, None),
    "plain": (lambda pl: None, False, None, None, None),  # no base plot: no fill styles
    "pad": (lambda pl: pl[1].SetDefaultColors(L3), True, L3, L3, L3),
    "pad0": (lambda pl: pl[0].SetDefaultColors(L3), True, L3, L3, L3),
    "both": (lambda pl: (pl[0].SetDefaultColors(L3_PAD0), pl[1].SetDefaultColors(L3)), True, L3, L3, L3),
    "parts": (lambda pl: pl[1].SetDefaultMarkerColors(MARKERS).SetDefaultLineColors(LINES).SetDefaultFillColors(FILLS), True, MARKERS, LINES, FILLS),
    "marker_only": (lambda pl: pl[1].SetDefaultMarkerColors(MARKERS), True, MARKERS, None, None),
    "fill_pad0": (lambda pl: (pl[0].SetDefaultFillColors(FILLS), pl[1].SetDefaultLineColors(LINES)), True, None, LINES, FILLS),
    "ref_func": (lambda pl: pl[1].SetRefFunc("2"), True, None, None, None),
}
# setups of the data under test: (setup, parts it sets)
SETUPS = {
    "none": (lambda d: d, ()),
    "color": (lambda d: d.SetColor(SET["all"]), ("marker", "line", "fill")),
    "marker": (lambda d: d.SetMarkerColor(SET["marker"]), ("marker",)),
    "line": (lambda d: d.SetLineColor(SET["line"]), ("line",)),
    "fill": (lambda d: d.SetFillColor(SET["fill"]), ("fill",)),
    "marker_line": (lambda d: d.SetMarkerColor(SET["marker"]).SetLineColor(SET["line"]), ("marker", "line")),
    "all_parts": (lambda d: d.SetMarkerColor(SET["marker"]).SetLineColor(SET["line"]).SetFillColor(SET["fill"]), ("marker", "line", "fill")),
    "fill_style": (lambda d: d.SetFillStyle(3004), ()),
    "alpha": (lambda d: d.SetAlpha(0.5), ()),
    "keep": (lambda d: d.SetKeepInputStyle(), ()),
}


def from_lists(case, part, entry, stylish):
    """the colour of the lists for a part at an entry (None: no list gives one); stylish: a fill style is set"""
    _, _, *lists = PADS[case["pads"]]
    own = dict(zip(("marker", "line", "fill"), lists))[part]
    if own is not None:
        return own[entry % len(own)]
    if part != "fill" or stylish:
        return DEFAULT_COLORS[entry % len(DEFAULT_COLORS)]
    return None


def expected(case, part, entry, stylish):
    """the colour a part of a data item gets at a list entry (None: not set by SciRooPlot)"""
    if part == "fill" and DATA_KINDS[case["kind"]][1] == "TF3":
        return ROOT.kWhite  # the faces of a 3d function: white without a fill colour of its own, as by ROOT (see drawing tests)
    return from_lists(case, part, entry, stylish)


def takes(case, part, stylish):
    """whether the data under test takes a part from the lists"""
    setup = case["setup"]
    return setup != "keep" and part not in SETUPS[setup][1] and from_lists(case, part, 0, stylish) is not None


styled = {}
for kind, obj in data_objects().items():
    obj = obj.Clone()
    for part, color in KEPT.items():
        attr = {"marker": "Marker", "line": "Line", "fill": "Fill"}[part]
        if obj.InheritsFrom(f"TAtt{attr}"):
            getattr(obj, f"Set{attr}Color")(color)
    if obj.InheritsFrom("TAttFill"):
        obj.SetFillStyle(3004)
    ROOT.SetOwnership(obj, False)
    styled[kind] = obj
p = Project("data_colours")
p.inputs(("normal", "empty"))
p.source("styled", styled)

todo = cases(
    {"kind": list(DATA_KINDS), "pads": list(PADS), "setup": list(SETUPS), "before": [1, 8], "drawn": ["drawn", "undrawn"]},
    valid=lambda c: (c["pads"] != "ref_func" or DATA_KINDS[c["kind"]][0] == 1) and (c["drawn"] == "drawn" or drawn_type(c["kind"]) in ("graph", "graph2d")),
    always=lambda c: c["kind"] in ("hist", "graph") and c["before"] == 1 and c["drawn"] == "drawn",
    named=[
        {"kind": "hist", "pads": "srp", "setup": "marker", "before": 1, "drawn": "drawn", "why": "a colour setter sets only its own part"},
        {"kind": "hist", "pads": "srp", "setup": "marker_line", "before": 1, "drawn": "drawn", "why": "a colour setter sets only its own part"},
        {"kind": "hist", "pads": "parts", "setup": "none", "before": 8, "drawn": "drawn", "why": "the default colours of a data item stay together"},
        {"kind": "tf3", "pads": "srp", "setup": "alpha", "before": 1, "drawn": "drawn", "why": "the faces of a 3d function transparent with the alpha"},
        {"kind": "graph", "pads": "srp", "setup": "none", "before": 1, "drawn": "undrawn", "why": "an empty graph before takes its colour"},
    ],
)
for i, case in enumerate(todo):
    setupPlot, withBase, *_ = PADS[case["pads"]]
    plot = S.Plot(f"c{i}", "colours", base_of(case["kind"])) if withBase else S.Plot(f"c{i}", "colours")
    setupPlot(plot)
    for _ in range(case["before"]):  # undrawn: graphs without points
        add_data(plot[1], case["kind"], *(("inputs", "normal") if case["drawn"] == "drawn" else (source_of("empty"), "empty")))
    source = "styled" if case["setup"] == "keep" else source_of("normal")
    SETUPS[case["setup"]][0](add_data(plot[1], case["kind"], source))
    add_data(plot[1], case["kind"])
    p.pm.AddPlot(plot)
run = p.plot("colours")

for i, case in enumerate(todo):
    what = describe(case)
    # by data number (data that is not drawn keeps its number); 0: the reference function
    drawn = {int(obj.GetName().split(":")[0]) - 1: obj for obj, _ in run.drawn(f"c{i}", "colours") if not obj.GetName().startswith("0:")}
    if sorted(drawn) != (list(range(case["before"] + 2)) if case["drawn"] == "drawn" else [case["before"], case["before"] + 1]):
        check(f"plot made: {what}", False, f"data {sorted(drawn)} drawn | " + " | ".join(run.errors()[:2]))
        continue
    _, withBase, *_ = PADS[case["pads"]]
    stylish = withBase  # the base plots set fill styles
    setup = case["setup"]
    entry = case["before"]
    for part in ("marker", "line", "fill"):
        attr = {"marker": "Marker", "line": "Line", "fill": "Fill"}[part]
        obj = drawn[entry]
        if not obj.InheritsFrom(f"TAtt{attr}"):
            continue
        index = getattr(obj, f"Get{attr}Color")()
        alpha = 0.5 if setup == "alpha" else None
        if setup == "keep":
            want = KEPT[part] if "formula" not in DATA_KINDS[case["kind"]][2] and "projected" not in DATA_KINDS[case["kind"]][2] else None
            if part == "fill" and DATA_KINDS[case["kind"]][1].startswith("TF3"):
                want = None  # ROOT's colour of the faces of a 3d function is kept with the style (see drawing tests)
        elif part in SETUPS[setup][1]:
            want = SET["all"] if setup == "color" else SET[part]
        else:
            want = expected(case, part, entry, stylish or setup == "fill_style")
        if want is not None:
            check(f"{part} colour of the data: {what}", not colour_diff(want, index, alpha), colour_diff(want, index, alpha))
    # the data before and after: entries 0, 1, ... and the next one, or the same if the data under test took none
    took = any(takes(case, part, stylish or setup == "fill_style") for part in ("marker", "line", "fill"))
    for position, entryUsed in ((0, 0), (entry + 1, entry + int(took))):
        if position not in drawn:
            continue
        obj = drawn[position]
        found = [colour_diff(expected(case, part, entryUsed, stylish), getattr(obj, f"Get{attr}Color")()) for part, attr in (("marker", "Marker"), ("line", "Line"), ("fill", "Fill")) if obj.InheritsFrom(f"TAtt{attr}") and expected(case, part, entryUsed, stylish) is not None]
        found = [f for f in found if f]
        check(f"data {position + 1} takes entry {entryUsed}: {what}", not found, " | ".join(found))
others = [line for line in run.warnings() if "inputs_empty has no points: it is not drawn" not in line]
check("no warnings but those of graphs without points", not others, " | ".join(others[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
