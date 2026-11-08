"""Colours taken from the palette by raw drawing options (PLC line, PMC marker, PFC fill) are those plain ROOT gives the same objects drawn
with the same options into one pad with the same palette: the frame and other things SciRooPlot draws are not counted. Every 1d kind,
alone and several in a pad, with only some data asking for palette colours (not functions: ROOT colours the histogram it paints a
function with, which is not stored).
Covers: a single HIST PLC got the last palette colour: the frame was counted.
"""

from srptest import DATA_1D, ROOT, Project, S, add_data, cases, check, describe, drawn_type, finish, option, rgba

OPTIONS = {"hist1d": "HIST {}", "graph": "LP {}"}  # functions: ROOT gives the colour to the histogram it paints them with, not to the function
PATTERNS = {  # palette options of the data of a pad, in order
    "one": ["PLC"],
    "two": ["PLC PMC", "PLC PMC"],
    "three": ["PLC PMC PFC", "PLC PMC PFC", "plc pmc pfc"],  # lower case too
    "first_only": ["PLC", ""],
    "later_only": ["", "PLC PMC", "PMC PLC"],
    "fill": ["PFC", "PFC"],
}
PALETTE = ROOT.kViridis


def palette_colours(opt, obj):
    """the colours of an object a palette option sets, as (r, g, b, alpha)"""
    return tuple(rgba(getattr(obj, f"Get{part}Color")()) for key, part in (("PLC", "Line"), ("PMC", "Marker"), ("PFC", "Fill")) if key in opt.upper())


p = Project("palette_options")
p.inputs()
todo = cases({"kind": list(DATA_1D), "pattern": list(PATTERNS)}, valid=lambda c: drawn_type(c["kind"]) in OPTIONS)
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "options", "1d")
    plot[1].SetPalette(PALETTE)
    for extra in PATTERNS[case["pattern"]]:
        add_data(plot[1], case["kind"]).SetDrawOption(OPTIONS[drawn_type(case["kind"])].format(extra).strip())
    p.pm.AddPlot(plot)
run = p.plot("options")

for i, case in enumerate(todo):
    what = describe(case)
    drawn = run.drawn(f"c{i}", "options")
    extras = PATTERNS[case["pattern"]]
    if len(drawn) != len(extras):
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    # plain ROOT: clones of the drawn objects, drawn with their options into one pad with the same palette and painted
    ROOT.gStyle.SetPalette(PALETTE)
    reference = ROOT.TCanvas(f"ref{i}", "", 400, 300)
    clones = []
    for k, (obj, opt) in enumerate(drawn):
        clone = obj.Clone(f"ref{i}_{k}")
        for key, part in (("PLC", "Line"), ("PMC", "Marker"), ("PFC", "Fill")):
            if key in extras[k].upper():
                getattr(clone, f"Set{part}Color")(ROOT.kWhite)  # plain ROOT sets them when painting
        clone.Draw(("A" if k == 0 and obj.InheritsFrom("TGraph") else "") + opt + (" SAME" if k else ""))
        clones.append(clone)
    reference.Print(p.path(f"ref{i}.svg"))
    for k, ((obj, opt), clone) in enumerate(zip(drawn, clones)):
        found, want = palette_colours(extras[k], obj), palette_colours(extras[k], clone)
        check(f"data {k + 1} has the palette colours of plain ROOT: {what}", found == want and option(opt).count("PLC") == ("PLC" in extras[k].upper()), f"{found}, plain ROOT {want} ({opt})")
    reference.Close()
check("no warnings", not run.warnings(), " | ".join(run.warnings()[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
