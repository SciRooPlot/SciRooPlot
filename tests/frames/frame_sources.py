"""The frame of a pad comes from one source: the first data drawn, the data marked SetDefinesFrame() (wherever it is in the pad), or an
input given to Pad::SetFrame (not drawn, not numbered: the data keeps its numbers); the other data of the pad does not widen it: data
lying entirely beyond one edge of the frame is warned about (not data whose line or fill reaches into the frame). A graph
without points gives no frame: the next data does (also when it is marked), and a pad of only such graphs is still made. A pad without
data takes its frame from the ranges of its axes, and a pad missing one of them is warned about by name.
The frame input takes nothing from the data: no number, no colour, no legend row; its bin labels label the frame. It wins over data
marked SetDefinesFrame() (with a warning), a second call replaces it, and a pad of only a frame input is made with that frame. A frame
input in pad 0 or in a base plot is ignored with a warning when the plot is added; a ratio or a frame without name or source is refused.
Cases: every data kind of a flat frame giving the frame, first, marked or set, with other data of a wider range in the pad, and the named
cases.
Reference: the frame of the same data drawn alone; for colours and legend rows the same data without frame input.
Covers: an empty graph as first or marked data, the frame of a pad without data from its axis ranges, the rules of Pad::SetFrame.
"""

from srptest import DATA_1D, DATA_2D, DATA_KINDS, ROOT, Project, S, add_data, base_of, captured, cases, check, close, describe, finish

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
narrow = ROOT.TH1D("narrow", "", 5, 2.0, 7.0)
labels = ROOT.TH1D("labels", "", 4, 0.0, 4.0)
four = ROOT.TH1D("four", "", 4, 0.0, 4.0)
for b in range(1, 6):
    narrow.SetBinContent(b, 20.0 + b)
for b, label in enumerate("ABCD", 1):
    labels.GetXaxis().SetBinLabel(b, label)
    four.SetBinContent(b, b)
crossing = ROOT.TGraph(2)  # from below the frame of hist to above it: its line crosses the frame
crossing.SetPoint(0, 3.0, -100.0)
crossing.SetPoint(1, 6.0, 100.0)
crossing.SetName("crossing")
p.source("other", {"wide": wide, "wide2": wide2, "empty": empty, "hist": hist, "narrow": narrow, "labels": labels, "four": four, "crossing": crossing})

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
    "no_y_range": lambda pad: pad["X"].SetRange(-3.0, 17.0),  # without boxes: a pad holding only boxes needs no frame
    "no_ranges": lambda pad: pad["Y"].SetTitle("y"),
    "labelled_frame": lambda pad: (pad.SetFrame("labels", "other"), pad.AddData("four", "other")),
    "set_and_marked": lambda pad: (pad.SetFrame("narrow", "other"), pad.AddData("hist", "other").SetDefinesFrame()),
    "set_twice": lambda pad: (pad.SetFrame("hist", "other"), pad.SetFrame("narrow", "other"), pad.AddData("wide", "other")),
    "frame_only": lambda pad: pad.SetFrame("narrow", "other"),
    "set_colours_legend": lambda pad: (pad.SetFrame("narrow", "other"), pad.AddData("hist", "other", "hist"), pad.AddData("wide", "other", "wide"), pad.AddLegend()),
    "colours_legend": lambda pad: (pad.AddData("hist", "other", "hist"), pad.AddData("wide", "other", "wide"), pad.AddLegend()),
    # data beyond one edge of the frame the first data gives is warned about, data reaching into it or crossing it not
    "outside_above": lambda pad: (pad.AddData("hist", "other"), pad.AddData("narrow", "other")),
    "inside": lambda pad: (pad.AddData("hist", "other"), pad.AddData("four", "other")),
    "line_crossing": lambda pad: (pad.AddData("hist", "other"), pad.AddData("crossing", "other").SetDrawOption("L")),
    "filled_above": lambda pad: (pad.AddData("hist", "other"), pad.AddData("narrow", "other").SetDrawOption(S.hist).SetFill(1001, ROOT.kBlue)),
}
for name, setup in NAMED.items():
    plot = S.Plot(name, "named", "1d")
    setup(plot[1])
    p.pm.AddPlot(plot)
for name in ("hist", "narrow"):
    plot = S.Plot(f"alone_{name}_other", "named", "1d")
    plot[1].AddData(name, "other")
    p.pm.AddPlot(plot)
# a frame input in pad 0 of a plot and in a base plot: ignored with a warning when added
plot = S.Plot("pad0_frame", "named", "1d")
plot[0].SetFrame("narrow", "other")
plot[1].AddData("hist", "other")
_, pad0Said = captured(lambda: p.pm.AddPlot(plot))
base = S.PlotManager.MakeBasePlot("1d")
base.SetName("1d_framed")
base[1].SetFrame("narrow", "other")
_, baseSaid = captured(lambda: p.pm.AddBasePlot(base))
plot = S.Plot("base_frame", "named", "1d_framed")
plot[1].AddData("hist", "other")
p.pm.AddPlot(plot)
refused = {}
for name, setup in {"a ratio": lambda pad: pad.SetFrame(S.Ratio(S.Data("hist", "other"), S.Data("narrow", "other"))), "no name": lambda pad: pad.SetFrame("", "other"), "no data source": lambda pad: pad.SetFrame("hist", "")}.items():
    plot = S.Plot("refused", "named", "1d")
    refused[name] = captured(lambda setup=setup, plot=plot: setup(plot[1]))[0]

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
for name, missing in (("no_y_range", " Y range"), ("no_ranges", " X Y range")):
    said = [m for m in run.messages(name, "named") if "Nothing to be drawn in pad 1" in m]
    check(f"a pad without data, boxes and{missing}: warned naming it", len(said) == 1 and f"no{missing} for an empty frame" in said[0], " | ".join(said))


def same_frame(name, reference):
    found, want = frame_of(name, "named"), frame_of(reference, "named")
    check(f"{name}: the frame of {reference}", found is not None and want is not None and all(close(a, b, 1e-9) for a, b in zip(found, want)), f"{found} instead of {want}")


frameHist = run.frame_hist("labelled_frame", "named")
found = [frameHist.GetXaxis().GetBinLabel(b) for b in range(1, 5)] if frameHist else None
check("a labelled frame input labels the frame", found == list("ABCD"), f"{found}")
same_frame("set_and_marked", "alone_narrow_other")
check("set_and_marked: warned that the marked data does not define the frame", any("has a frame (narrow), data marked SetDefinesFrame() does not define it" in m for m in run.messages("set_and_marked", "named")), " | ".join(run.messages("set_and_marked", "named")))
same_frame("set_twice", "alone_narrow_other")
same_frame("frame_only", "alone_narrow_other")
check("frame_only: nothing drawn as data", run.canvas("frame_only", "named") is not None and not run.objects("frame_only", "named"), f"{[o.GetName() for o in run.objects('frame_only', 'named')]}")


def look(name):
    objs = run.objects(name, "named")
    padObj = run.pad(name, "named")
    legend = next((o for o in padObj.GetListOfPrimitives() if o.InheritsFrom("TLegend")), None) if padObj else None
    rows = [(e.GetLabel(), e.GetOption(), e.GetLineColor(), e.GetMarkerColor()) for e in legend.GetListOfPrimitives()] if legend else None
    return [(o.GetName(), o.GetLineColor(), o.GetMarkerColor(), o.GetFillColor()) for o in objs], rows


found, want = look("set_colours_legend"), look("colours_legend")
check("a frame input takes no number and no colour from the data", found[0] == want[0] and len(found[0]) == 2, f"{found[0]} instead of {want[0]}")
check("a frame input has no legend row", found[1] == want[1] and want[1] is not None and len(want[1]) == 2, f"{found[1]} instead of {want[1]}")
same_frame("pad0_frame", "alone_hist_other")
check("a frame input in pad 0: warned when the plot is added", "added to pad 0 are not drawn" in pad0Said, pad0Said)
same_frame("base_frame", "alone_hist_other")
check("a frame input in a base plot: warned when it is added", "has data or a frame in pad 1, which is ignored" in baseSaid, baseSaid)
for name, data, n in (("outside_above", "narrow", 1), ("inside", "four", 0), ("line_crossing", "crossing", 0), ("filled_above", "narrow", 0)):
    said = [m for m in run.messages(name, "named") if "lies entirely" in m]
    check(f"{name}: {'warned once that ' + data + ' is outside the frame' if n else 'no warning about data outside the frame'}", len(said) == n and all(f"Data {data}:other lies entirely above" in m for m in said), " | ".join(said))
for name, result in refused.items():
    check(f"a frame of {name} is refused", isinstance(result, ValueError), f"{result!r}")
finish()
