"""The drawing option of a data item is the last one given: an alias and a raw ROOT option replace each other, SetDrawOption() without an
argument clears both (the default rule draws the data again), SetDrawOption("") is a value of its own (passed to ROOT as it is: its
default), and ApplyLayout with a layout that has an option replaces both, one without keeps them. This holds for every data kind with
the aliases and raw options of what it is drawn as, for data added directly, from a template (a Data called with name and source) and
for ratios. The reference is the same plot with only the last call (none after a clear): the drawn type and option must be the same.
Covers: an alias and a raw option kept side by side, the alias winning over a raw option given later.
"""

from srptest import DATA_1D, DATA_KINDS, Project, S, add_data, base_of, cases, check, describe, drawn_type, finish

ALIAS = {  # two aliases of what the data is drawn as (None: there is only one), one turning a histogram into a graph or the other way
    "hist1d": ("points_line", "hist_open"),
    "graph": ("curve", "hist_open"),
    "function": ("line", "points"),
    "hist2d": ("text", "cont"),
    "graph2d": ("triangles", "cont"),
    "function2d": ("surf", "cont"),
    "hist3d": ("box", "iso"),
    "function3d": ("iso", None),
}
# two raw options, both drawn otherwise than the aliases and the default rule
RAW = {"hist1d": ("E1", "HIST L"), "graph": ("LP", "C P"), "function": ("FC", "LF2"), "hist2d": ("ARR", "CONT4"), "graph2d": ("PCOL", "CONT"), "function2d": ("CONT1", "SURF1"), "hist3d": ("BOX1", "SCAT"), "function3d": ("FB", "BB")}
# the calls in order: alias a/b, raw a/b, clear (no argument), empty (""), layout_<alias a|raw a|none>
SEQUENCES = {
    "alias": ["alias_a"],
    "raw": ["raw_a"],
    "alias_cleared": ["alias_a", "clear"],
    "raw_cleared": ["raw_a", "clear"],
    "alias_emptied": ["alias_a", "empty"],
    "raw_then_alias": ["raw_a", "alias_a"],
    "alias_then_raw": ["alias_a", "raw_a"],
    "alias_then_alias": ["alias_a", "alias_b"],
    "raw_then_raw": ["raw_a", "raw_b"],
    "cleared_then_alias": ["alias_b", "clear", "alias_a"],
    "layout_alias_over_raw": ["raw_b", "layout_alias_a"],
    "layout_raw_over_alias": ["alias_b", "layout_raw_a"],
    "layout_without_option": ["raw_a", "layout_none"],
    "alias_after_layout": ["layout_raw_a", "alias_a"],
    "cleared_after_layout": ["layout_alias_a", "clear"],
}
HOSTS = ("data", "template", "ratio")


def last(calls):
    """the call the result must equal: the last value given, None for a cleared option"""
    final = None
    for call in calls:
        what = call.removeprefix("layout_")
        if what == "none":
            continue
        final = None if what == "clear" else what
    return final


def value(kind, what):
    kindType = drawn_type(kind)
    if what == "empty":
        return ""
    if what.startswith("alias"):
        return getattr(S, ALIAS[kindType][what.endswith("b")])
    return RAW[kindType][what.endswith("b")]


def apply(data, kind, calls):
    for call in calls:
        if call == "clear":
            data.SetDrawOption()
        elif call.startswith("layout_"):
            layout = S.Data().SetColor(2)
            if call != "layout_none":
                layout.SetDrawOption(value(kind, call.removeprefix("layout_")))
            data.ApplyLayout(layout)
        else:
            data.SetDrawOption(value(kind, call))
    return data


def valid(case):
    kind, host, calls = case["kind"], case["host"], SEQUENCES[case["sequence"]]
    if ALIAS[drawn_type(kind)][1] is None and "alias_b" in calls:
        return False
    if host == "template":
        return "formula" not in DATA_KINDS[kind][2]
    if host == "ratio":
        return kind in DATA_1D and not DATA_KINDS[kind][2] & {"formula", "projected"}
    return True


todo = cases(
    {"kind": list(DATA_KINDS), "host": list(HOSTS), "sequence": list(SEQUENCES)},
    valid=valid,
    always=lambda c: c["kind"] in ("hist", "graph_errors") and c["host"] == "data",
    named=[
        {"kind": "hist", "host": "data", "sequence": "alias_then_raw", "why": "a raw option given after an alias replaces it"},
        {"kind": "hist", "host": "data", "sequence": "alias_emptied", "why": "an empty option is passed to ROOT as it is"},
        {"kind": "hist", "host": "template", "sequence": "alias_cleared", "why": "the option of a template cleared"},
        {"kind": "hist", "host": "ratio", "sequence": "alias_cleared", "why": "the option of a ratio cleared"},
        {"kind": "hist", "host": "data", "sequence": "layout_without_option", "why": "a layout without option keeps the raw option"},
    ],
)

p = Project("option_order")
p.inputs()


def add(plot, kind, host):
    if host == "ratio":
        return plot[1].AddRatio([kind, "inputs"], [kind, "inputs"])
    if host == "template":
        data = plot[1].AddData(S.Data()(kind, "inputs"))
        return data.Project([0] if DATA_KINDS[kind][0] == 1 else [0, 1]) if "projected" in DATA_KINDS[kind][2] else data
    return add_data(plot[1], kind)


for i, case in enumerate(todo):
    kind, host, calls = case["kind"], case["host"], SEQUENCES[case["sequence"]]
    final = last(calls)
    for suffix, steps in (("", calls), ("_ref", [final] if final else [])):
        plot = S.Plot(f"c{i}{suffix}", "order", base_of(kind))
        apply(add(plot, kind, host), kind, steps)
        p.pm.AddPlot(plot)
run = p.plot("order")

for i, case in enumerate(todo):
    what = describe(case)
    got, ref = run.drawn(f"c{i}", "order"), run.drawn(f"c{i}_ref", "order")
    if not got or not ref:
        check(f"plots made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    (obj, opt), (refObj, refOpt) = got[0], ref[0]
    check(f"drawn as with the last call only: {what}", obj.ClassName() == refObj.ClassName() and opt == refOpt, f"{obj.ClassName()} '{opt}', expected {refObj.ClassName()} '{refOpt}'")
# the values compared differ: each alias and raw option draws otherwise than the default rule and the other values
for kind in DATA_KINDS:
    drawn = {}
    for i, case in enumerate(todo):
        if case["kind"] == kind and case["host"] == "data" and case["sequence"] in ("alias", "raw", "alias_cleared", "raw_then_raw", "alias_then_alias"):
            got = run.drawn(f"c{i}", "order")
            drawn[case["sequence"]] = (got[0][0].ClassName(), got[0][1]) if got else None
    if len(drawn) == 5:
        check(f"the values differ: kind={kind}", len(set(drawn.values())) == 5, str(drawn))
    elif len(drawn) == 4:  # one alias only
        check(f"the values differ: kind={kind}", len(set(drawn.values())) == 4, str(drawn))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
