"""SetKeepInputStyle() keeps the line, marker and fill attributes stored with the input instead of the defaults of the pad, for every data
kind that has them; what the plot definition sets for the data still wins. Without it, the input's attributes are replaced by the
defaults of the pad (the base plot), so data looks the same whatever its file stored.
Covers: the line, marker and fill styles and sizes stored with an input, kept or replaced (colours: colours/data_colours.py).
"""

from srptest import DATA_KINDS, ROOT, Project, S, add_data, base_of, cases, check, close, data_objects, describe, finish

STORED = {"GetLineStyle": 7, "GetLineWidth": 4, "GetMarkerStyle": 33, "GetMarkerSize": 2.5, "GetFillStyle": 3005}
SET = {"GetLineStyle": ("SetLineStyle", 2), "GetMarkerStyle": ("SetMarkerStyle", 22), "GetFillStyle": ("SetFillStyle", 3004)}  # set by the definition

p = Project("input_styles")
p.inputs()
styled, plain = {}, {}
for kind, obj in data_objects().items():
    if not obj.InheritsFrom("TAttLine"):  # THn: no attributes of its own
        continue
    styled[kind] = obj.Clone()
    styled[kind].SetLineStyle(STORED["GetLineStyle"])
    styled[kind].SetLineWidth(STORED["GetLineWidth"])
    if obj.InheritsFrom("TAttMarker"):
        styled[kind].SetMarkerStyle(STORED["GetMarkerStyle"])
        styled[kind].SetMarkerSize(STORED["GetMarkerSize"])
    styled[kind].SetFillStyle(STORED["GetFillStyle"])
    styled[kind].SetFillColor(ROOT.kOrange)
p.source("styled", styled)

todo = cases(
    {"kind": list(styled), "keep": [True, False], "set": [False, True]},
    always=lambda c: c["kind"] in ("hist", "graph_errors", "hist2d"),
    named=[{"kind": "hist", "keep": True, "set": True, "why": "the definition wins over the kept style"}],
)
for i, case in enumerate(todo):
    for suffix, source in (("", "styled"), ("_ref", "inputs")):  # the reference: the same data from an input without stored style
        plot = S.Plot(f"s{i}{suffix}", "styles", base_of(case["kind"]))
        data = add_data(plot[1], case["kind"], source).SetDrawOption(S.points if DATA_KINDS[case["kind"]][0] == 1 else S.colz)
        if DATA_KINDS[case["kind"]][1] in ("TF3", "TH3D", "TProfile3D"):
            data.SetDrawOption()
        if case["keep"] and suffix == "":
            data.SetKeepInputStyle()
        if case["set"]:
            for getter, (setter, value) in SET.items():
                getattr(data, setter)(value)
        p.pm.AddPlot(plot)
run = p.plot()

for i, case in enumerate(todo):
    what = describe(case)
    got, ref = run.drawn(f"s{i}", "styles"), run.drawn(f"s{i}_ref", "styles")
    if not got or not ref:
        check(f"plots made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    obj, refObj = got[0][0], ref[0][0]
    for getter, stored in STORED.items():
        if not hasattr(obj, getter) or (getter.startswith("GetMarker") and not obj.InheritsFrom("TAttMarker")):
            continue
        if case["set"] and getter in SET:
            want, source = SET[getter][1], "the definition"
        elif case["keep"]:
            want, source = stored, "the input"
        else:
            want, source = getattr(refObj, getter)(), "the pad"
        value = getattr(obj, getter)()
        check(f"{getter[3:]} from {source}: {what}", close(value, want, 1e-6), f"{value}, expected {want}")
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
