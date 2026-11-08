"""Every drawing option alias on every data kind is drawn with the ROOT option of the documented alias table (docs/skill: drawing-options.md),
without ROOT errors: the colour scale (the Z of COLZ, LEGO2Z, ...) is drawn by the frame, not by the data; an alias marked as drawn as the
other type converts histograms to graphs and graphs to histograms; an alias a type cannot draw gives one warning and ROOT's default.
Documented adjustments: polar graphs draw the error bars of the points aliases with E, 2d graphs with errors add ERR, a TH2Poly is drawn
flat (col, colz) with a warning instead of lego and surf, and the frame of a 3d view is an empty lego plot (no surface), keeping POL.
The drawn data carries the name of the input.
Covers: colz_text, clamped maps, 3d histograms, functions, 2d functions, 2d graphs, TH2Poly drawn flat,
frame of 3d views, error bars of polar graphs, frame of 2d graphs drawn in 3d, and a TEfficiency drawn
under the name ROOT gives the graph or histogram it makes.
"""

import re

from srptest import ALIASES, DATA_KINDS, Project, S, add_data, alias_table, base_of, cases, check, data_name, describe, drawn_type, finish, no_scale, per_error_set

TABLE = alias_table()
REPRESENTATIVE = ("hist", "graph_errors", "hist2d", "graph2d_errors")  # every alias in the fast run


def expected(kind, alias):
    """(type the data is drawn as, None if the alias is not available; option of the data; frame draws a colour scale; warnings)"""
    kindType = drawn_type(kind)
    entry = TABLE[alias][kindType]
    if entry == "-":
        return None, "", False, 1
    drawnType = entry if entry in ("graph", "hist1d") else kindType
    opt = TABLE[alias][drawnType]
    warnings = 0
    if DATA_KINDS[kind][2] >= {"polygons"} and ("lego" in alias or "surf" in alias):
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
    ],
)

p = Project("aliases")
p.inputs()
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "aliases", base_of(case["kind"]))
    add_data(plot[1], case["kind"]).SetDrawOption(getattr(S, case["alias"]))
    p.pm.AddPlot(plot)
run = p.plot("aliases")

warned = {}
for line in run.warnings("is not available for"):
    m = re.search(r"Drawing option (\w+) is not available for", line)
    if m:
        warned.setdefault(m.group(1), []).append(line)
for i, case in enumerate(todo):
    what = describe(case)
    kind, alias = case["kind"], case["alias"]
    drawn = run.drawn(f"c{i}", "aliases")
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    drawnType, data, scale, nWarnings = expected(kind, alias)
    obj, opt = drawn[0]
    if drawnType is not None and obj.InheritsFrom("TGraphMultiErrors"):
        data = per_error_set(data)
    if drawnType is None:
        check(f"not available: drawn with the default of ROOT: {what}", opt == "", f"drawn with '{opt}'")
    else:
        check(f"drawn with the ROOT option of the table: {what}", opt == data, f"drawn with '{opt}', expected '{data}'")
    if drawnType != drawn_type(kind) and drawnType is not None:
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
    found = [line for line in warned.get(alias, []) if line.rstrip(".").endswith(f" {name}, using the default of ROOT") or f" {name} (ROOT draws" in line]
    check(f"{nWarnings} warning(s): {what}", len(found) == nWarnings, " | ".join(found))
# a graph with asymmetric errors drawn as a histogram is drawn with their mean, with a warning
ASYMMETRIC = {k for k, (_, cls, _) in DATA_KINDS.items() if cls in ("TGraphAsymmErrors", "TEfficiency")}
asymmetric = run.warnings("has asymmetric y errors")
check("asymmetric errors drawn as their mean only for graphs that have them", all(any(f"Graph {k}:" in w for k in ASYMMETRIC) for w in asymmetric), " | ".join(asymmetric))
expectedWarnings = sum(expected(c["kind"], c["alias"])[3] for c in todo) + len(asymmetric)
check("no other warnings", len(run.warnings()) == expectedWarnings, " | ".join(w for w in run.warnings() if "is not available" not in w)[:500])
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
