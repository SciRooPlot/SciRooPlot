"""Data without a drawing option follows one rule, the same for histograms and graphs: data with a fill is drawn as hist (graphs as area),
data with a marker style or size set, with errors (any non-zero error, as plain ROOT gives them: the square roots of the contents of a
histogram filled without weights count) or a graph of a single point as points, other data (all errors zero) as hist (graphs as line); 2d data and 3d histograms as colz (the colour scale on the frame); other functions with ROOT's default.
The fill of the input counts only when its style is kept (SetKeepInputStyle), and the fill ROOT gives every TEfficiency is not a fill.
Covers: ROOT's default fill of a TEfficiency.
"""

from srptest import DATA_KINDS, ROOT, Project, S, add_data, alias_table, base_of, cases, check, close, data_objects, describe, drawn_type, finish, has_variant, no_scale, per_error_set, source_of

TABLE = alias_table()
STYLES = {  # style of the data: (setup, sets a fill, sets a marker)
    "none": (lambda d: d, False, False),
    "fill": (lambda d: d.SetFill(1001, ROOT.kRed), True, False),
    "hollow_fill": (lambda d: d.SetFill(0, ROOT.kRed), False, False),
    "marker_style": (lambda d: d.SetMarkerStyle(20), False, True),
    "marker_size": (lambda d: d.SetMarkerSize(1.2), False, True),
    "input_style": (lambda d: d.SetKeepInputStyle(), False, False),
    "input_fill": (lambda d: d.SetKeepInputStyle(), True, False),  # from the source "filled": inputs with a red solid fill
    "input_fill_unkept": (lambda d: d, False, False),
}
VARIANTS = ("normal", "single", "zeros")  # a single point or filled bin; all values and errors zero
INPUT_FILL = ("input_fill", "input_fill_unkept")


def has_errors(obj):
    """whether plain ROOT gives the data a non-zero error (GetBinError: also without Sumw2)"""
    if obj.InheritsFrom("TEfficiency"):
        obj = obj.CreateGraph()
    elif obj.InheritsFrom("THnBase"):
        obj = obj.Projection(0)
    if obj.InheritsFrom("TH1"):
        return any(obj.GetBinError(b) != 0 for b in range(1, obj.GetNbinsX() + 1))
    return any(obj.GetErrorYlow(i) > 0 or obj.GetErrorYhigh(i) > 0 for i in range(obj.GetN()))


def expected(case, obj):
    """the alias the rule chooses (None: ROOT's default)"""
    kindType = drawn_type(case["kind"])
    if kindType in ("hist2d", "graph2d", "function2d", "hist3d"):
        return "colz"
    if kindType not in ("hist1d", "graph"):
        return None
    _, fill, marker = STYLES[case["style"]]
    graph = kindType == "graph"
    if fill:
        return "area" if graph else "hist"
    single = graph and (obj.CreateGraph() if obj.InheritsFrom("TEfficiency") else obj).GetN() == 1
    if marker or has_errors(obj) or single:
        return "points"
    return "line" if graph else "hist"


p = Project("default_rule")
p.inputs(VARIANTS)
filled = {}
for kind, obj in data_objects().items():
    if DATA_KINDS[kind][0] == 1 and obj.InheritsFrom("TAttFill"):
        filled[kind] = obj.Clone()
        filled[kind].SetFillColor(ROOT.kRed)
        filled[kind].SetFillStyle(1001)
p.source("filled", filled)
objects = {variant: data_objects(variant) for variant in VARIANTS}
todo = cases(
    {"kind": list(DATA_KINDS), "style": list(STYLES), "variant": list(VARIANTS)},
    valid=lambda c: has_variant(c["kind"], c["variant"]) and (c["style"] not in INPUT_FILL or (c["kind"] in filled and c["variant"] == "normal")),
    always=lambda c: drawn_type(c["kind"]) in ("hist1d", "graph") and c["variant"] == "normal",
    named=[
        {"kind": "efficiency", "style": "input_style", "variant": "normal", "why": "the default fill ROOT gives a TEfficiency is ignored"},
        {"kind": "hist_counts", "style": "none", "variant": "normal", "why": "counts: points with their square roots"},
        {"kind": "hist_noerrors", "style": "none", "variant": "normal", "why": "errors all zero: hist"},
    ],
)
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "rule", base_of(case["kind"]))
    source = "filled" if case["style"] in INPUT_FILL else source_of(case["variant"])
    STYLES[case["style"]][0](add_data(plot[1], case["kind"], source, case["variant"]))
    p.pm.AddPlot(plot)
run = p.plot("rule")

for i, case in enumerate(todo):
    what = describe(case)
    drawn = run.drawn(f"c{i}", "rule")
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    alias = expected(case, objects[case["variant"]].get(case["kind"]))
    kindType = drawn_type(case["kind"])
    want = no_scale(TABLE[alias][kindType]) if alias else ""
    if alias and alias.startswith("points") and DATA_KINDS[case["kind"]][1] == "TGraphPolar":
        want += " E"
    obj, opt = drawn[0]
    if alias and obj.InheritsFrom("TGraphMultiErrors"):
        want = per_error_set(want)
    check(f"drawn as {alias or 'the default of ROOT'}: {what}", opt == want, f"drawn with '{opt}', expected '{want}'")
    if alias == "area":  # the line through the points, filled below by the polygon closed at zero
        fill = next((o for o, _ in run.links(f"c{i}", "rule") if o.GetName() == f"fill_{obj.GetName()}"), None)
        check(f"area filled down to zero: {what}", bool(fill) and fill.GetN() == obj.GetN() + 2 and close(fill.GetPointY(0), 0.0) and close(fill.GetPointY(fill.GetN() - 1), 0.0), obj.ClassName())
    if alias == "colz":
        frame = run.frame_option(f"c{i}", "rule") or ""
        check(f"colour scale on the frame: {what}", frame.split()[0].endswith("Z") if frame else False, f"frame '{frame}'")
check("no warnings", not run.warnings(), " | ".join(run.warnings()[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
