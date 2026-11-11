"""A raw ROOT drawing option reaches ROOT as given, for every data kind with the raw options of what it is drawn as, alone and over a
first data drawn the same way; only SciRooPlot's own cleanups change it:
- SAME, SAMES, SAME0, SAMES0 (any case) are removed, silently except SAME0 (its own colour range is lost);
- AXIS and AXIG are removed with a warning (the frame draws the axes);
- for graphs A (new axes) and RX, RY (reversed axes) are removed with a warning, a polar graph keeps its R;
- the letters of the polar grid (N, O) go from the first polar graph to the grid, N or O alone draw a line; D and G set the angle unit;
- a TH2Poly asked for LEGO or SURF is drawn as COL (COLZ) with a warning (ROOT draws polygon bins in 3d only with OpenGL);
- SetTextAngle is written after TEXT (an angle already in the option is kept, angles outside 0-90 are clamped with a warning);
- arrows (ARR) get ROOT's line width 1 unless the data sets one.
A histogram is drawn with stored errors (Sumw2) only where its input has them, so the empty option draws a histogram filled without
weights as ROOT does: as steps, without its errors.
The colour scale Z of 2d and 3d data is drawn by the frame (see aliases.py), so it is compared without it.
Covers: SAME, RX/RY, A of graphs, polar grid letters, arrow width, AXIS/AXIG.
"""

import re

from srptest import DATA_KINDS, Project, S, add_data, base_of, captured, cases, check, data_name, data_objects, describe, drawn_type, finish, no_scale, option

RAW = {  # raw ROOT options without an alias, by what the data is drawn as
    "hist1d": ["", "E", "E0", "E1", "E2", "E3", "E4", "E5", "E6", "HIST", "L", "C", "P", "P0", "B", "BAR", "HBAR", "LF2", "HIST F", "HIST P", "E P", "E2 P", "*H", "HIST LP", "HIST E", "HIST E1", "HIST E2", "HIST E3", "TEXT", "X0 E"],
    "graph": ["P", "L", "C", "B", "F", "LP", "CP", "PZ", "2", "3", "4", "5", "3L", "4C", "3PL", "LF", "E", "E1", "||", "[]", "0", "X P", "*", "P >", "P |>"],
    "function": ["L", "C", "P", "LF2", "FC", "*"],
    "hist2d": ["COL", "COLZ", "COL0", "BOX", "BOX1", "SCAT", "TEXT", "ARR", "CONT", "CONT0", "CONT1", "CONT2", "CONT3", "CONT4", "LEGO", "LEGO1", "LEGO2", "SURF", "SURF1", "SURF2", "SURF3", "SURF4", "CANDLE", "VIOLIN", "COL POL"],
    "graph2d": ["P", "P0", "PCOL", "LINE", "TRI", "TRI1", "TRI2", "TRIW", "COL", "COLZ", "CONT", "CONT4", "SURF1", "LEGO"],
    "function2d": ["COL", "COLZ", "CONT", "CONT1", "SURF", "SURF1", "LEGO", "LEGO2"],
    "hist3d": ["BOX", "BOX1", "BOX2", "BOX2Z", "BOX3", "ISO", "SCAT"],
    "function3d": ["ISO", "FB", "BB"],
}
BASE = {"hist1d": "HIST", "graph": "P", "function": "L", "hist2d": "COL", "graph2d": "COL", "function2d": "COL", "hist3d": "BOX", "function3d": "ISO"}
# cleanup tokens: (warnings per data, only for graphs)
TOKENS = {"SAME": (0, False), "SAMES": (0, False), "SAME0": (1, False), "SAMES0": (1, False), "AXIS": (1, False), "AXIG": (1, False), "A": (1, True), "RX": (1, True), "RY": (1, True)}


def expected_raw(kind, raw):
    """(drawn option, warnings) for a raw option"""
    what = drawn_type(kind)
    if DATA_KINDS[kind][1] == "TH2Poly" and ("LEGO" in raw or "SURF" in raw):
        return "COL", 1
    if what in ("hist2d", "graph2d", "function2d", "hist3d", "function3d"):
        return no_scale(option(raw)), 0
    if what == "graph" and "*" in raw and DATA_KINDS[kind][1] != "TGraphPolar":  # TGraph::Draw itself replaces * by P (marker style 3)
        return option(raw.replace("*", "P")), 0
    return option(raw), 0


def valid_cleanup(case):
    kind, token = case["kind"], case["token"]
    graphOnly = TOKENS[token][1]
    if graphOnly and drawn_type(kind) != "graph":
        return False
    return not (token in ("RX", "RY") and DATA_KINDS[kind][1] == "TGraphPolar")


p = Project("raw_options")
p.inputs()


def add_plot(name, kind, opt, second, setup=lambda data: data):
    plot = S.Plot(name, "raw", base_of(kind))
    if second:
        add_data(plot[1], kind).SetDrawOption(opt)
    setup(add_data(plot[1], kind).SetDrawOption(opt))
    p.pm.AddPlot(plot)  # a copy: the data is set up before


# raw options pass through
passing = cases({"kind": list(DATA_KINDS), "raw": sorted({r for v in RAW.values() for r in v}), "position": ["alone", "second"]}, valid=lambda c: c["raw"] in RAW[drawn_type(c["kind"])])
for i, case in enumerate(passing):
    add_plot(f"pass{i}", case["kind"], case["raw"], case["position"] == "second")
# cleanups
cleanups = cases({"kind": list(DATA_KINDS), "token": list(TOKENS), "place": ["glued_before", "lower_after"], "position": ["alone", "second"]}, valid=valid_cleanup)
for i, case in enumerate(cleanups):
    base = BASE[drawn_type(case["kind"])]
    opt = f"{case['token']}{base}" if case["place"] == "glued_before" else f"{base} {case['token'].lower()}"
    add_plot(f"clean{i}", case["kind"], opt, case["position"] == "second")
# the polar grid: (option, grid option, angle unit, letters the graph is drawn with)
POLAR = {"plain": ("P", "", "radian", "P"), "n": ("N", "N", "radian", "L"), "o": ("O", "O", "radian", "L"), "pn": ("PN", "N", "radian", "P"), "lo": ("LO", "O", "radian", "L"), "deg": ("PD", "", "degree", "PD"), "grad": ("PG", "", "grad", "PG"), "rad": ("PR", "", "radian", "PR")}
for name, (opt, *_) in POLAR.items():
    add_plot(f"polar_{name}", "graph_polar", opt, False)
plot = S.Plot("polar_second", "raw", "1d")  # a later polar graph's N stays out of the grid, the graph is drawn as line
add_data(plot[1], "graph_polar").SetDrawOption("P")
add_data(plot[1], "graph_polar").SetDrawOption("N")
p.pm.AddPlot(plot)
# arrows: every kind drawn as a 2d histogram, alone and over a COLZ map, with and without a width of its own
ARROW_KINDS = [k for k in DATA_KINDS if drawn_type(k) == "hist2d" and DATA_KINDS[k][1] != "TH2Poly"]
arrows = cases({"kind": ARROW_KINDS, "width": [None, 3], "position": ["alone", "over_map"]})
for i, case in enumerate(arrows):
    plot = S.Plot(f"arr{i}", "raw", "2d")
    if case["position"] == "over_map":
        add_data(plot[1], case["kind"]).SetDrawOption("COLZ")
    data = add_data(plot[1], case["kind"]).SetDrawOption("ARR")
    if case["width"]:
        data.SetLineWidth(case["width"])
    p.pm.AddPlot(plot)
# text angle: (option, angle, drawn option, warnings)
TEXTS = {"inserted": ("TEXT", 45, "TEXT45", 0), "kept": ("TEXT30", 45, "TEXT30", 0), "clamped": ("TEXT", 120, "TEXT90", 1), "no_text": ("HIST", 45, "HIST", 1)}
for name, (opt, angle, *_) in TEXTS.items():
    add_plot(f"text_{name}", "hist", opt, False, lambda data, angle=angle: data.SetTextAngle(angle))
# the name of an alias given as raw option: passed as it is, warned when it is set unless ROOT knows the word as an option too
ALIAS_NAMES = {
    "col_clamped": ("hist2d", 1),
    "points": ("hist", 1),
    "line": ("hist", 1),
    "text": ("hist", 0),
    "lego": ("hist2d", 0),
    "cont": ("hist2d", 0),
    "hist": ("hist2d", 0),
    "colz": ("hist", 0),
}
aliasSaid = {name: captured(lambda name=name, kind=kind: add_plot(f"alias_name_{name}", kind, name, False))[1] for name, (kind, _) in ALIAS_NAMES.items()}
run = p.plot("raw")


def warnings_of(kind, text=""):
    name = re.compile(rf"(?<![\w]){re.escape(data_name(kind))}(?![\w])")
    return [w for w in run.warnings(text) if name.search(w)]


inputs = data_objects()
counted = {}  # warnings expected per (kind, text) over the run


def expect_warnings(kind, text, n):
    counted[(kind, text)] = counted.get((kind, text), 0) + n


for i, case in enumerate(passing):
    what = describe(case)
    drawn = run.drawn(f"pass{i}", "raw")
    want, nWarnings = expected_raw(case["kind"], case["raw"])
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    opts = [opt for _, opt in drawn]
    _, cls, props = DATA_KINDS[case["kind"]]
    if cls.startswith(("TH1", "TH2", "TH3")) and "projected" not in props:
        stored = inputs[case["kind"]].GetSumw2N() > 0
        check(f"stored errors only as in the input ({stored}): {what}", all((obj.GetSumw2N() > 0) == stored for obj, _ in drawn), [obj.GetSumw2N() for obj, _ in drawn])
    check(f"raw option reaches ROOT: {what}", opts == [want] * len(opts) and len(opts) == (2 if case["position"] == "second" else 1), f"drawn with {opts}, expected '{want}'")
    expect_warnings(case["kind"], "is not available for TH2Poly", nWarnings * len(opts))
for i, case in enumerate(cleanups):
    what = describe(case)
    drawn = run.drawn(f"clean{i}", "raw")
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    want, _ = expected_raw(case["kind"], BASE[drawn_type(case["kind"])])
    opts = [opt for _, opt in drawn]
    check(f"{case['token']} removed: {what}", opts == [want] * len(opts), f"drawn with {opts}, expected '{want}'")
    if case["token"] in ("SAME0", "SAMES0", "RX", "RY"):  # the frame draws SAME for its axis copy and AXIS itself
        frame = run.frame_option(f"clean{i}", "raw") or ""
        check(f"{case['token']} not in the frame: {what}", case["token"] not in frame, frame)
    text = {"SAME0": "SAME0 is removed", "SAMES0": "SAME0 is removed", "AXIS": "AXIS and AXIG", "AXIG": "AXIS and AXIG", "A": "A (new axes)", "RX": "RX and RY", "RY": "RX and RY"}.get(case["token"])
    if text:
        expect_warnings(case["kind"], text, TOKENS[case["token"]][0] * len(opts))
for name, (opt, gridOpt, unit, letters) in POLAR.items():
    links = run.links(f"polar_{name}", "raw", 1) if run.canvas(f"polar_{name}", "raw") else []
    grids = [(o, option(lo)) for o, lo in links if o.InheritsFrom("TGraphPolargram")]
    graphs = ["".join(sorted(option(lo).replace(" ", ""))) for o, lo in links if o.InheritsFrom("TGraphPolar")]
    check(f"polar grid takes '{gridOpt}' from '{opt}'", len(grids) == 1 and grids[0][1] == gridOpt, [g for _, g in grids])
    if grids:
        grid = grids[0][0]
        isUnit = {"radian": grid.IsRadian() and not grid.IsDegree() and not grid.IsGrad(), "degree": grid.IsDegree(), "grad": grid.IsGrad()}[unit]
        check(f"polar grid of '{opt}' in {unit}", isUnit, (grid.IsRadian(), grid.IsDegree(), grid.IsGrad()))
    check(f"polar graph of '{opt}' drawn with '{letters}'", graphs == ["".join(sorted(letters))], graphs)
links = run.links("polar_second", "raw", 1) if run.canvas("polar_second", "raw") else []
grids = [option(lo) for o, lo in links if o.InheritsFrom("TGraphPolargram")]
graphs = [option(lo).replace(" ", "") for o, lo in links if o.InheritsFrom("TGraphPolar")]
check("a later polar graph's N stays out of the grid, the graph is drawn as line", grids == [""] and graphs == ["P", "L"], (grids, graphs))
for i, case in enumerate(arrows):
    drawn = run.drawn(f"arr{i}", "raw")
    widths = [obj.GetLineWidth() for obj, opt in drawn if "ARR" in opt]
    check(f"arrow line width {case['width'] or 1}: {describe(case)}", widths == [case["width"] or 1], f"{widths} {' | '.join(run.errors()[:2])}")
for name, (opt, angle, want, _) in TEXTS.items():
    drawn = run.drawn(f"text_{name}", "raw")
    opts = [o for _, o in drawn]
    check(f"text angle {angle} with '{opt}': drawn with '{want}'", opts == [want], opts)
expect_warnings("hist", "text angle", sum(n for *_, n in TEXTS.values()))
for name, (kind, n) in ALIAS_NAMES.items():
    drawn = run.drawn(f"alias_name_{name}", "raw")
    said = [line for line in aliasSaid[name].splitlines() if f"Drawing option '{name}' of" in line]
    check(f"raw option '{name}' of a {kind}: passed as it is, {'warned as name of an alias' if n else 'no warning'}", all(opt.lower() == name for _, opt in drawn) and len(said) == n, f"drawn with {[o for _, o in drawn]}; {' | '.join(said)}")
for (kind, text), n in sorted(counted.items()):
    found = warnings_of(kind, text)
    if n or found:
        check(f"{n} warnings '{text}' for {kind}", len(found) == n, f"{len(found)}: {' | '.join(found[:2])}")
# col_clamped as raw option: ROOT makes a 3d frame of it, into which the map is not drawn
known = [text for _, text in counted] + ["text angle", "is drawn flat (col_clamped)"]
others = [w for w in run.warnings() if not any(t in w for t in known)]
check("no other warnings", not others, " | ".join(others[:4]))
check("no errors", not run.errors(), " | ".join(run.errors()[:4]))
finish()
