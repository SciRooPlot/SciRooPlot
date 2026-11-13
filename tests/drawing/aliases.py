"""Every drawing option alias on every data kind is drawn with the ROOT option of the documented alias table (docs/skill: drawing-options.md),
without ROOT errors, and the drawn data paints something: the colour scale (the Z of COLZ, LEGO2Z, ...) is drawn by the frame, not by the data;
an alias marked as drawn as the other type converts histograms to graphs and graphs to histograms; an alias neither 1d type can draw gives one
warning and draws histograms and graphs as without a drawing option (other types with ROOT's default), as do a graph that cannot be made a
histogram (points at one x) and error bands, boxes and brackets of a graph without errors.
Documented adjustments: polar graphs draw the error bars of the points aliases with E, 2d graphs with errors add ERR, a TH2Poly is drawn
flat (col, colz) with a warning instead of lego, surf, polar, boxes, contours, candles and violins, and the frame of a 3d view is an empty lego plot
(no surface), keeping POL. area without a fill paints what line paints. The drawn data carries the name of the input.
Covers: colz_text, clamped maps, 3d histograms, functions, 2d functions, 2d graphs, TH2Poly drawn flat,
frame of 3d views, error bars of polar graphs, frame of 2d graphs drawn in 3d, and a TEfficiency drawn
under the name ROOT gives the graph or histogram it makes; data that painted nothing: a graph or a histogram without stored errors drawn
with a 2d alias, a graph with points at one x drawn as a histogram, error bands and brackets of a graph without errors, boxes, contours and
candles of a TH2Poly; lines down to zero at both ends of area of a graph; the steps of a histogram keeping its style with a fill colour
filled by every alias that fills them; no warning about asymmetric errors where an alias shows none (text).
"""

import re

from srptest import ALIASES, DATA_KINDS, ROOT, Project, S, add_data, alias_table, base_of, cases, check, data_name, data_objects, describe, drawn_type, finish, in_chunks, is_data, message_name, no_scale, per_error_set, pixels, rgba

TABLE = alias_table()
REPRESENTATIVE = ("hist", "graph_errors", "hist2d", "graph2d_errors")  # every alias in the fast run
FLAT_ONLY = ("lego", "surf", "polar", "box", "cont", "candle", "violin")  # aliases a TH2Poly is drawn flat (and cartesian) for
NEED_ERRORS = ("band", "band_smooth", "boxes_nomarkers", "brackets")  # aliases that draw only the errors of a graph
WITHOUT_ERRORS = tuple(k for k, obj in data_objects().items() if obj.InheritsFrom("TGraph") and not any(obj.GetErrorX(i) > 0 or obj.GetErrorY(i) > 0 for i in range(obj.GetN())))


def area_1d(case):
    """a case of area of cartesian 1d data, which is also drawn as area and as line in one frame (that of area spans zero, without the x errors);
    area of a polar graph is drawn in cartesian coordinates"""
    return case["alias"] == "area" and DATA_KINDS[case["kind"]][0] == 1 and "polar" not in DATA_KINDS[case["kind"]][2]


def expected(kind, alias):
    """(type the data is drawn as, None if the alias is not available, "default" if drawn as without a drawing option; option of the data;
    frame draws a colour scale; warnings)"""
    kindType = drawn_type(kind)
    entry = TABLE[alias][kindType]
    if entry == "-":
        return ("default" if kindType in ("hist1d", "graph") else None), "", False, 1
    if kind in WITHOUT_ERRORS and alias in NEED_ERRORS:
        return "default", "", False, 1
    drawnType = entry if entry in ("graph", "hist1d") else kindType
    opt = TABLE[alias][drawnType]
    warnings = 0
    if DATA_KINDS[kind][2] >= {"polygons"} and any(word in alias for word in FLAT_ONLY):
        opt, warnings = "COLZ" if "Z" in opt.split()[0] else "COL", 1
    data = no_scale(opt)
    if alias.startswith("points") and DATA_KINDS[kind][1] == "TGraphPolar":
        data += " E"
    if alias in ("points", "points_col", "points_colz") and DATA_KINDS[kind][1] == "TGraph2DErrors":
        data += " ERR"
    return drawnType, data, opt != no_scale(opt), warnings


todo = cases(
    {"kind": list(DATA_KINDS), "alias": ALIASES},
    always=lambda c: c["kind"] in REPRESENTATIVE,
    named=[
        {"kind": "hist2d", "alias": "colz_text", "why": "colz with text"},
        {"kind": "hist3d", "alias": "iso", "why": "iso of a 3d histogram"},
        {"kind": "tf1", "alias": "area", "why": "area of a function"},
        {"kind": "graph2d_errors", "alias": "points_col", "why": "coloured points of a 2d graph with errors"},
        {"kind": "hist2poly", "alias": "lego_colz", "why": "a TH2Poly drawn flat for lego"},
        {"kind": "hist2d", "alias": "surf_col", "why": "the frame of a 3d view is an empty lego"},
        {"kind": "graph_polar", "alias": "points", "why": "polar legend rows show error bars only with e"},
        {"kind": "graph2d", "alias": "triangles", "why": "the frame of a 2d graph drawn as triangles is an empty lego"},
        {"kind": "efficiency", "alias": "points", "why": "drawn as eff_graph"},
        {"kind": "efficiency2d", "alias": "colz", "why": "drawn as eff_histo"},
        {"kind": "hist_counts", "alias": "col", "why": "a histogram without stored errors drawn with a 2d alias"},
        {"kind": "graph", "alias": "colz", "why": "a graph drawn with a 2d alias"},
        {"kind": "graph", "alias": "band", "why": "an error band of a graph without errors"},
        {"kind": "graph", "alias": "brackets", "why": "brackets of a graph without errors"},
        {"kind": "graph", "alias": "area", "why": "area of a graph without a fill"},
        {"kind": "hist2poly", "alias": "box", "why": "boxes of a TH2Poly drawn flat"},
        {"kind": "hist2poly", "alias": "candle", "why": "candles of a TH2Poly drawn flat"},
        {"kind": "hist2poly", "alias": "cont_colz", "why": "contours of a TH2Poly drawn flat"},
        {"kind": "graph_asymm", "alias": "text", "why": "a graph with asymmetric errors written as values (no errors shown)"},
    ],
)
KEPT_FILL = ("hist", "hist_open", "hist_open_clipped")  # aliases filling the steps, drawn on a histogram keeping its input's style

p = Project("aliases")
p.inputs()
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "aliases", base_of(case["kind"]))
    add_data(plot[1], case["kind"]).SetDrawOption(getattr(S, case["alias"]))
    p.pm.AddPlot(plot)
    if area_1d(case):
        for alias in ("area", "line"):
            plot = S.Plot(f"c{i}_{alias}", "aliases", base_of(case["kind"]))
            add_data(plot[1], case["kind"]).SetDrawOption(getattr(S, alias))
            plot[1]["X"].SetRange(0.0, 11.0)
            plot[1]["Y"].SetRange(-1.0, 10.0)
            p.pm.AddPlot(plot)
for kind in {c["kind"] for c in todo if expected(c["kind"], c["alias"])[0] == "default"}:  # the data drawn without a drawing option
    plot = S.Plot(f"default_{kind}", "aliases", base_of(kind))
    add_data(plot[1], kind)
    p.pm.AddPlot(plot)
# a graph with two points at one x has no bins: drawn as without a drawing option, one warning naming why
repeated = ROOT.TGraph(3)
for i, x in enumerate((1.0, 1.0, 2.0)):
    repeated.SetPoint(i, x, 1.0 + i)
repeated.SetName("repeated")
p.source("repeated_x", {"repeated": repeated})
REPEATED = ("hist", "hist_clipped")
for alias in REPEATED:
    plot = S.Plot(f"repeated_{alias}", "aliases", "1d")
    plot[1].AddData("repeated", "repeated_x")
    plot[1].AddData("repeated", "repeated_x").SetDrawOption(getattr(S, alias))
    p.pm.AddPlot(plot)
# a histogram keeping its input's style (no fill) with a fill colour set but no fill style: every alias filling the steps fills them
for alias in KEPT_FILL:
    plot = S.Plot(f"kept_fill_{alias}", "aliases", "1d")
    add_data(plot[1], "hist").SetKeepInputStyle().SetFillColor(ROOT.kOrange).SetDrawOption(getattr(S, alias))
    p.pm.AddPlot(plot)
run = p.plot("aliases")

warned = {}
for line in run.warnings():
    m = re.search(r"Drawing option (\w+) is not available for|has no errors to draw with drawing option (\w+)", line)
    if m:
        warned.setdefault(m.group(1) or m.group(2), []).append(line)
for i, case in enumerate(todo):
    what = describe(case)
    kind, alias = case["kind"], case["alias"]
    drawn = run.drawn(f"c{i}", "aliases")
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    drawnType, data, scale, nWarnings = expected(kind, alias)
    obj, opt = drawn[0]
    if drawnType not in (None, "default") and obj.InheritsFrom("TGraphMultiErrors"):
        data = per_error_set(data)
    if drawnType is None:
        check(f"not available: drawn with the default of ROOT: {what}", opt == "", f"drawn with '{opt}'")
    elif drawnType == "default":
        refObj, refOpt = run.drawn(f"default_{kind}", "aliases")[0]
        check(f"not drawable: drawn as without a drawing option: {what}", (obj.ClassName(), opt) == (refObj.ClassName(), refOpt), f"drawn as {obj.ClassName()} with '{opt}', without an option as {refObj.ClassName()} with '{refOpt}'")
    else:
        check(f"drawn with the ROOT option of the table: {what}", opt == data, f"drawn with '{opt}', expected '{data}'")
    if drawnType not in (drawn_type(kind), None, "default"):
        converted = obj.InheritsFrom("TGraph") if drawnType == "graph" else obj.InheritsFrom("TH1")
        check(f"drawn as {drawnType}: {what}", converted, obj.ClassName())
    frame = run.frame_option(f"c{i}", "aliases")
    if DATA_KINDS[kind][0] > 1 and frame is not None:
        frameScale = re.search(r"(?<=[A-Z0-9])Z", frame.replace("AXIS", "").replace("AXIG", "")) is not None
        check(f"colour scale drawn by the frame only if asked for: {what}", frameScale == scale, f"frame '{frame}'")
        if any(word in data for word in ("LEGO", "SURF", "TRI")):
            check(f"3d view on an empty lego frame: {what}", "LEGO" in frame and "SURF" not in frame and frame.split().count("0") == 1 and ("POL" in frame) == ("POL" in data), f"frame '{frame}'")
    name = data_name(kind)
    check(f"drawn under the name of the input: {what}", obj.GetName().split(":", 1)[1].startswith(name), obj.GetName())
    said = message_name(kind)
    found = [line for line in warned.get(alias, []) if f" {said}, drawing it as " in line or f" {said} (ROOT " in line or f"Graph {said} has no errors" in line or line.rstrip(".").endswith(f" {said}, using the default of ROOT")]
    check(f"{nWarnings} warning(s): {what}", len(found) == nWarnings, " | ".join(found))
for alias in REPEATED:
    drawn = run.drawn(f"repeated_{alias}", "aliases")
    check(f"a graph with points at one x drawn {alias}: drawn as without a drawing option", len(drawn) == 2 and drawn[1][1] == drawn[0][1] and drawn[1][1] != "", str([(o.ClassName(), opt) for o, opt in drawn]))
repeatedWarnings = run.warnings("graph repeated:repeated_x")
named = [m.group(1) for w in repeatedWarnings if (m := re.search(r"Drawing option (\w+) .* \(it needs points with distinct x values\), drawing it as ", w))]
check(
    "a graph with points at one x drawn as a histogram: one warning naming the option and why",
    len(repeatedWarnings) == len(REPEATED) and sorted(named) == sorted(REPEATED),
    " | ".join(repeatedWarnings),
)
# a graph with asymmetric errors drawn as a histogram is drawn with their mean, with a warning
ASYMMETRIC = {k for k, (_, cls, _) in DATA_KINDS.items() if cls in ("TGraphAsymmErrors", "TEfficiency")}
asymmetric = run.warnings("has asymmetric y errors")
check("asymmetric errors drawn as their mean only for graphs that have them", all(any(f"Graph {k}:" in w for k in ASYMMETRIC) for w in asymmetric), " | ".join(asymmetric))
asText = next(i for i, c in enumerate(todo) if c["kind"] == "graph_asymm" and c["alias"] == "text")
check("a graph with asymmetric errors written as values: no warning about drawing its errors with their mean", not any("has asymmetric y errors" in m for m in run.messages(f"c{asText}", "aliases")), " | ".join(run.messages(f"c{asText}", "aliases")))
fills = {alias: [(rgba(obj.GetFillColor()), obj.GetFillStyle()) for obj, _ in run.drawn(f"kept_fill_{alias}", "aliases")] for alias in KEPT_FILL}
check("a histogram keeping its style with a fill colour: filled alike by every alias filling the steps", fills["hist"] and fills["hist"][0][1] != 0 and all(f == fills["hist"] for f in fills.values()), str(fills))
expectedWarnings = sum(expected(c["kind"], c["alias"])[3] for c in todo) + len(asymmetric) + len(REPEATED)
check("no other warnings", len(run.warnings()) == expectedWarnings, " | ".join(w for w in run.warnings() if "is not available" not in w)[:500])
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))


def compare(_, item):
    """the data of a plot paints something (the plot differs from it painted without the data), and area without a fill paints what line
    paints; not checked where the plot cannot be painted here: a canvas read back with CONT4 (cont_smooth, drawn in projected coordinates)
    crashes when painted from Python"""
    name, what, case = item
    drawn = run.drawn(name, "aliases")
    if not drawn or any("CONT4" in opt for _, opt in drawn):
        return []
    painted, _ = pixels(run.canvas(name, "aliases"), lambda obj: True)
    background, _ = pixels(run.canvas(name, "aliases"), lambda obj: not is_data(obj))
    checks = [(f"the data paints: {what}", (painted != background).any(), "the plot looks the same without it")]
    if case and area_1d(case):
        area, _ = pixels(run.canvas(f"{name}_area", "aliases"), lambda obj: True)
        line, _ = pixels(run.canvas(f"{name}_line", "aliases"), lambda obj: True)
        checks.append((f"area without a fill paints what line paints: {what}", (area == line).all(), f"{int((area != line).sum())} pixels differ"))
    return checks


plots = [(f"c{i}", describe(case), case) for i, case in enumerate(todo)] + [(f"repeated_{alias}", f"a graph with points at one x drawn {alias}", None) for alias in REPEATED]
in_chunks(plots, compare, size=150)  # up to four paints per plot
finish()
