"""The frame of a pad comes from one source: the first data drawn, the data marked SetDefinesFrame() (wherever it is in the pad), or an
input given to Pad::SetFrame (not drawn, not numbered: the data keeps its numbers); the other data of the pad does not widen it. A graph
without points gives no frame: the next data does (also when it is marked), and a pad of only such graphs is still made. A pad without
data takes its frame from the ranges of its axes.
Cases: every data kind of a flat frame giving the frame, first, marked or set, with other data of a wider range in the pad, and the named
cases.
Reference: the frame of the same data drawn alone.
Covers: c4fc70a (an empty graph as first or marked data), the frame of a pad without data from its axis ranges (the old pad frame test).
"""

from srptest import DATA_1D, DATA_2D, DATA_KINDS, ROOT, Project, S, add_data, base_of, cases, check, close, describe, finish

KINDS = tuple(k for k in DATA_1D + DATA_2D if "formula" not in DATA_KINDS[k][2])  # a formula is no input for SetFrame
HOW = ("first", "marked", "set")
# an n-dimensional histogram is drawn as a projection, which SetFrame has not: it gives the frame as projected data marked SetDefinesFrame()
todo = cases({"kind": list(KINDS), "how": list(HOW)}, valid=lambda c: c["how"] != "set" or "projected" not in DATA_KINDS[c["kind"]][2], always=lambda c: True)

p = Project("frame_sources")
p.inputs()
wide = ROOT.TH1D("wide", "", 40, -20.0, 40.0)
wide2 = ROOT.TH2D("wide2", "", 30, -20.0, 40.0, 30, -20.0, 40.0)
for b in range(1, 41):
    wide.SetBinContent(b, 100.0 + b)
for b in range(1, 31):
    for c in range(1, 31):
        wide2.SetBinContent(b, c, 300.0 + b + c)
empty = ROOT.TGraph()
empty.SetName("empty")
hist = ROOT.TH1D("hist", "", 10, 0.0, 10.0)
for b in range(1, 11):
    hist.SetBinContent(b, b)
p.source("other", {"wide": wide, "wide2": wide2, "empty": empty, "hist": hist})

for kind in KINDS:
    plot = S.Plot(f"alone_{kind}", "frames", base_of(kind))
    add_data(plot[1], kind)
    p.pm.AddPlot(plot)
for i, case in enumerate(todo):
    kind, how = case["kind"], case["how"]
    plot = S.Plot(f"f{i}", "frames", base_of(kind))
    pad = plot[1]
    other = "wide" if DATA_KINDS[kind][0] == 1 else "wide2"
    if how == "first":
        add_data(pad, kind)
        pad.AddData(other, "other")
    elif how == "marked":
        pad.AddData(other, "other")
        add_data(pad, kind).SetDefinesFrame()
    else:
        pad.SetFrame(kind, "inputs")
        pad.AddData(other, "other")
    p.pm.AddPlot(plot)

NAMED = {  # name: (data of pad 1, frame expected: that of the plot alone_hist or the axis ranges)
    "empty_first": lambda pad: (pad.AddData("empty", "other"), pad.AddData("hist", "other")),
    "empty_marked": lambda pad: (pad.AddData("hist", "other"), pad.AddData("empty", "other").SetDefinesFrame()),
    "only_empty": lambda pad: (pad.AddData("empty", "other"), pad.AddData("empty", "other")),
    "no_data": lambda pad: (pad["X"].SetRange(-3.0, 17.0), pad["Y"].SetRange(2.0, 8.0), pad.AddText(0.3, 0.5, "no data")),
}
for name, setup in NAMED.items():
    plot = S.Plot(name, "named", "1d")
    setup(plot[1])
    p.pm.AddPlot(plot)
plot = S.Plot("alone_hist_other", "named", "1d")
plot[1].AddData("hist", "other")
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def frame_of(name, group):
    padObj = run.pad(name, group)
    return (padObj.GetUxmin(), padObj.GetUxmax(), padObj.GetUymin(), padObj.GetUymax()) if padObj else None


for i, case in enumerate(todo):
    what = describe(case)
    found, want = frame_of(f"f{i}", "frames"), frame_of(f"alone_{case['kind']}", "frames")
    if not found or not want:
        check(f"{what}: the plot is made", False, " | ".join(run.messages(f"f{i}", "frames")))
        continue
    check(f"{what}: the frame of the data drawn alone", all(close(a, b, 1e-9) for a, b in zip(found, want)), f"{found} instead of {want}")
    names = [o.GetName() for o in run.objects(f"f{i}", "frames")]
    if case["how"] == "set":
        check(f"{what}: the input of the frame is not drawn, the data is data 1", len(names) == 1 and names[0].startswith("1:wide"), f"{names}")
    else:
        check(f"{what}: both are drawn as data", len(names) == 2, f"{names}")

reference = frame_of("alone_hist_other", "named")
for name in ("empty_first", "empty_marked"):
    found = frame_of(name, "named")
    check(f"{name}: the frame of the next data", found is not None and reference is not None and all(close(a, b, 1e-9) for a, b in zip(found, reference)), f"{found} instead of {reference}")
    check(f"{name}: the empty graph is warned about", any("has no points" in m for m in run.messages(name, "named")), " | ".join(run.messages(name, "named")))
check("a pad of only empty graphs is still made", run.canvas("only_empty", "named") is not None, " | ".join(run.messages("only_empty", "named")))
found = frame_of("no_data", "named")
check("a pad without data: the frame of its axis ranges", found is not None and all(close(a, b, 1e-9) for a, b in zip(found, (-3.0, 17.0, 2.0, 8.0))), f"{found}")
finish()
