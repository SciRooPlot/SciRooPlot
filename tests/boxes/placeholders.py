"""Placeholders in legend labels, texts and statistics box titles are replaced by what they name:
- <entries> the stored entries of a histogram, the points of a graph; <integral>, <maximum>, <minimum>, <mean> and <stddev> of a histogram
  over the bins drawn (those inside the frame; a TH2Poly by its polygons), <maximum> and <minimum> of a graph the largest and smallest y
  of the points inside the frame, of a 2d graph z; a placeholder a kind of data has no value for is left as it is, with a warning naming
  it and the data
- <name>, <source>, <projection> and <selection> from the definition of the data (the name without request or projection, the data
  source, what is filled in the notation of TTree::Draw, which entries or bins with the axis titles of the input); <title> the title of the
  drawn object; an empty one takes one separating space with it
- in a legend label the data of its row, in a text the data named by number (<mean:2>, <mean:1@2> for data 1 of pad 2), the format in
  brackets ([.2f], [>8.2f], [d]); a placeholder without number in a text, a number without data, a malformed number or format and a word
  that is no placeholder are left as they are, with a warning (a known word followed by other text, <mean value>, is text)
Cases: every data kind × every numeric placeholder × legend label or text × the frame at the data or an x range inside it, and named
definitions for the descriptive ones.
Reference: the drawn objects read back (bins with their centre inside the frame, the points inside it) and the documented descriptions.
Covers: <name> for every kind, axis titles in <projection>/<selection>, <stddev> and data numbers in texts, formats with '>' and
look-alike typos, <mean> and <stddev> of a TH2Poly (0 from ROOT's empty
statistics of its grid), and the named cases.
"""

import math

from srptest import DATA_KINDS, ROOT, Project, S, add_data, base_of, cases, check, data_name, describe, finish, write_tree

import numpy as np

NUMERIC = ("entries", "integral", "maximum", "minimum", "mean", "stddev")
RANGE = (3.0, 7.0)  # an x range inside the data (bin edges of the inputs)

todo = cases(
    {"kind": list(DATA_KINDS), "word": list(NUMERIC), "place": ["legend", "text"], "range": ["data", "inside"]},
    valid=lambda c: c["range"] == "data" or DATA_KINDS[c["kind"]][0] == 1 and "polar" not in DATA_KINDS[c["kind"]][2],
    always=lambda c: (c["place"], c["range"]) in (("legend", "data"), ("text", "inside")),
)

p = Project("placeholders", bases=("1d", "1d_ratio", "2d"))
p.inputs()
FORMAT = "[.9g]"
for i, case in enumerate(todo):
    plot = S.Plot(f"n{i}", "numbers", base_of(case["kind"]))
    data = add_data(plot[1], case["kind"])
    word = f"<{case['word']}{FORMAT}>"
    if case["place"] == "legend":
        data.SetLegendLabel(f"value {word}")
        plot[1].AddLegend()
    else:
        data.SetLegendLabel("data")
        plot[1].AddText(0.2, 0.8, f"value {word[:-1]}:1>")
    if case["range"] == "inside":
        plot[1]["X"].SetRange(*RANGE)
    p.pm.AddPlot(plot)

# descriptive placeholders, from the definition of the data
rng = np.random.default_rng(5)
n = 1000
pt, eta, cent = rng.exponential(1.5, n), rng.uniform(-0.9, 0.9, n), rng.uniform(0, 100, n)
h2 = ROOT.TH2D("hPtEta", "a title;#it{p}_{T};#eta", 50, 0, 10, 18, -0.9, 0.9)
h2u = ROOT.TH2D("hUntitled", "", 50, 0, 10, 18, -0.9, 0.9)
h3 = ROOT.TH3D("hPtEtaCent", ";#it{p}_{T};#eta;centrality", 50, 0, 10, 18, -0.9, 0.9, 10, 0, 100)
hs = ROOT.THnSparseD("hSparse", "", 3, np.array([50, 18, 10], dtype=np.int32), np.array([0, -0.9, 0]), np.array([10, 0.9, 100]))
hs.GetAxis(0).SetTitle("pt")
hs.GetAxis(2).SetTitle("centrality")  # axis 1 without title
h1 = ROOT.TH1D("h", "histogram title", 10, 0, 10)
for k in range(n):
    h2.Fill(pt[k], eta[k])
    h2u.Fill(pt[k], eta[k])
    h3.Fill(pt[k], eta[k], cent[k])
    hs.Fill(np.array([pt[k], eta[k], cent[k]]))
    h1.Fill(pt[k])
g = ROOT.TGraph(3, np.array([1.0, 2.0, 3.0]), np.array([4.0, 5.0, 6.0]))
g.SetName("g")
g.SetTitle("graph title")
p.source("s", {"hPtEta": h2, "hUntitled": h2u, "hPtEtaCent": h3, "hSparse": hs, "h": h1, "g": g})
write_tree(p.path("t.root"), {"x": np.arange(100.0), "y": np.arange(100.0) % 7, "w": np.ones(100), "q": np.arange(100.0) % 2})
p.pm.AddDataSource("t", p.path("t.root"))
p.pm.AddDataSource("other", p.path("s.root"))
L = "<name>|<source>|<projection>|<selection>"
DESCRIPTIONS = {  # name: (definition, label, expected label)
    "hist": (lambda pad, lab: pad.AddData("h", "s", lab), L, "h|s||"),
    "other_source": (lambda pad, lab: pad.AddData("h", "other", lab), L, "h|other||"),
    "graph": (lambda pad, lab: pad.AddData("g", "s", lab), L, "g|s||"),
    "hist_as_graph": (lambda pad, lab: pad.AddData("h", "s", lab).SetDrawOption(S.points_line), L, "h|s||"),
    "ratio": (lambda pad, lab: pad.AddRatio(["h", "s"], ["h", "other"], lab), L, "h|s||"),
    "function": (lambda pad, lab: pad.AddFunction("std::abs(x)", lab), L, "std::abs(x)|||"),
    "points": (lambda pad, lab: pad.AddPoints([1.0, 2.0], [1.0, 2.0], lab), L, "1,2;1,2|||"),
    "tree": (lambda pad, lab: pad.AddData("t", "t", lab).Project1D("x"), L, "t|t|x|"),
    "tree2d_weight": (lambda pad, lab: pad.AddData("t", "t", lab).Project2D("x", "y", "w"), L, "t|t|y:x (w)|"),
    "tree_profile": (lambda pad, lab: pad.AddData("t", "t", lab).Profile1D("x", "y"), L, "t|t|prof. y:x|"),
    "tree_scatter": (lambda pad, lab: pad.AddData("t", "t", lab).Scatter("x", "y", "w", "w"), L, "t|t|y:x|"),
    "tree_filters": (lambda pad, lab: pad.AddData("t", "t", lab).Filter("x > 4").Filter("q == 1 || y < 2").Project1D("x"), L, "t|t|x|x > 4 && (q == 1 || y < 2)"),
    "tree_define_entries": (lambda pad, lab: pad.AddData("t", "t", lab).Define("z", "2 * x").Entries(10, 50).Project1D("z"), L, "t|t|z|entries [10, 50)"),
    "tree_entries_filter": (lambda pad, lab: pad.AddData("t", "t", lab).Filter("x > 4").Entries(30).Project1D("x"), L, "t|t|x|x > 4, entries [0, 30)"),
    "tree2d_projection": (lambda pad, lab: pad.AddData("t", "t", lab).Project2D("x", "y").ProjectX(1, 3), L, "t|t|x|bins 1-3 of y"),
    "projection_bins": (lambda pad, lab: pad.AddData("hPtEta", "s", lab).ProjectX(2, 5), L, "hPtEta|s|#it{p}_{T}|bins 2-5 of #eta"),
    "projection_user": (lambda pad, lab: pad.AddData("hPtEta", "s", lab).ProjectY(2.0, 5.5, True), L, "hPtEta|s|#eta|2 < #it{p}_{T} < 5.5"),
    "projection_untitled": (lambda pad, lab: pad.AddData("hUntitled", "s", lab).ProjectX(2, 5), L, "hUntitled|s|x|bins 2-5 of y"),
    "profile": (lambda pad, lab: pad.AddData("hPtEta", "s", lab).ProfileX(), L, "hPtEta|s|prof. #it{p}_{T}|"),
    "profile_as_graph": (lambda pad, lab: pad.AddData("hPtEta", "s", lab).ProfileX().SetDrawOption(S.points_line), L, "hPtEta|s|prof. #it{p}_{T}|"),
    "h3_projection": (lambda pad, lab: pad.AddData("hPtEtaCent", "s", lab).Project([0, 1], [(2, 1, 3)]), L, "hPtEtaCent|s|#eta:#it{p}_{T}|bins 1-3 of centrality"),
    "h3_projection_user": (lambda pad, lab: pad.AddData("hPtEtaCent", "s", lab).Project([0, 2], [(1, 0.5, 2.5)], True), L, "hPtEtaCent|s|centrality:#it{p}_{T}|0.5 < #eta < 2.5"),
    "h3_profile": (lambda pad, lab: pad.AddData("hPtEtaCent", "s", lab).Profile([0, 1]), L, "hPtEtaCent|s|prof. #eta:#it{p}_{T}|"),
    "thn_untitled_axis": (lambda pad, lab: pad.AddData("hSparse", "s", lab).Project([0], [(1, -0.5, 0.5)], True), L, "hSparse|s|pt|-0.5 < y < 0.5"),
    "thn_titled_axis": (lambda pad, lab: pad.AddData("hSparse", "s", lab).Project([1], [(2, 10.0, 50.0)], True), L, "hSparse|s|y|10 < centrality < 50"),
    "empty_takes_a_space": (lambda pad, lab: pad.AddData("h", "s", lab), "<name> <projection> <selection>", "h"),
    "empty_in_the_middle": (lambda pad, lab: pad.AddData("t", "t", lab).Project1D("x"), "<name> <selection> <projection>", "t x"),
    "title_hist": (lambda pad, lab: pad.AddData("h", "s", lab), "<title>", "histogram title"),
    "title_graph": (lambda pad, lab: pad.AddData("g", "s", lab), "<title>", "graph title"),
    "format_aligned": (lambda pad, lab: pad.AddData("h", "s", lab), "<entries[>8.1f]>|", f"{1000:>8.1f}|"),
    "format_integer": (lambda pad, lab: pad.AddData("h", "s", lab), "<entries[d]> entries", "1000 entries"),
    "format_after_number": (lambda pad, lab: pad.AddData("h", "s", lab), "<entries[.1e]>", f"{1000:.1e}"),
    "not_a_placeholder": (lambda pad, lab: pad.AddData("h", "s", lab), "x < 3, y [cm] <x[cm]>", "x < 3, y [cm] <x[cm]>"),
}
for name, (define, label, _) in DESCRIPTIONS.items():
    plot = S.Plot(name, "descriptions", "1d")
    define(plot[1], label)
    plot[1].AddLegend()
    p.pm.AddPlot(plot)

# texts naming data by number, and the messages of what cannot be replaced
plot = S.Plot("text_numbers", "texts", "1d_ratio")
plot[1].AddData("h", "s")
plot[1].AddData("g", "s")
plot[2].AddData("h", "s").Scale(2.0)
MALFORMED = ("<mean:1@>", "<mean::1>", "<mean: 1>", "<mean:-1>", "<mean:1e3>", "<mean:1@p2>", "<mean:1234567890>", "<mean[.2f]x>")
plot[1].AddText(0.2, 0.8, ["<entries[d]:2> <maximum[.1f]:2> <integral[.1f]:1@2>", "<mean> <mean:5> <mean:1@7> <meen:1> <mean:123456> plain #mu < 3", " ".join(MALFORMED[:4]), " ".join(MALFORMED[4:]) + " <mean value>"])
p.pm.AddPlot(plot)
plot = S.Plot("legend_number", "texts", "1d")
plot[1].AddData("h", "s", "<mean:1>")
plot[1].AddLegend()
p.pm.AddPlot(plot)
plot = S.Plot("stats_title", "texts", "1d")
plot[1].AddData("h", "s").SetShowStats("e", "<name> <source> <entries[d]>")
p.pm.AddPlot(plot)
plot = S.Plot("stats_title_graph", "texts", "1d")
plot[1].AddData("h", "s").SetShowStats("e", "<name> <source>").SetDrawOption(S.points_line)
p.pm.AddPlot(plot)
plot = S.Plot("bad_format", "texts", "1d")
plot[1].AddData("h", "s", "<mean[q]>")
plot[1].AddLegend()
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made", all(run.canvas(f"n{i}", "numbers") for i in range(len(todo))), " | ".join(run.errors()[:3]))


def labels(name, group, pad=1):
    """the legend labels and text lines of a pad"""
    padObj = run.pad(name, group, pad)
    found = []
    for obj in padObj.GetListOfPrimitives() if padObj else []:
        if obj.InheritsFrom("TLegend"):
            found += [e.GetLabel() for e in obj.GetListOfPrimitives()]
        elif obj.InheritsFrom("TPaveText") and not obj.InheritsFrom("TPaveStats"):
            found += [line.GetTitle() for line in obj.GetListOfLines()]
    return found


def inside(lo, hi, frame):
    return frame[0] - 1e-9 * abs(frame[0] or 1) <= 0.5 * (lo + hi) <= frame[1] + 1e-9 * abs(frame[1] or 1)


def reference(obj, word, frame):
    """the value of a numeric placeholder for a drawn object, None if the kind has none"""
    if obj.InheritsFrom("TH1"):
        if word == "entries":
            return obj.GetEntries()
        if obj.InheritsFrom("TH2Poly"):
            values = [(b.GetContent(), 0.5 * (b.GetXMin() + b.GetXMax()), b.GetContent()) for b in obj.GetBins() if inside(b.GetXMin(), b.GetXMax(), frame["X"]) and inside(b.GetYMin(), b.GetYMax(), frame["Y"])]
        else:
            axes = [obj.GetXaxis(), obj.GetYaxis(), obj.GetZaxis()][: obj.GetDimension()]
            limits = [frame["X"], frame["Y"]] if obj.GetDimension() == 2 else [frame["X"]] if obj.GetDimension() == 1 else [None] * 3  # a 3d view shows every bin
            values = []
            for cell in np.ndindex(*[a.GetNbins() for a in axes]):
                bins = [c + 1 for c in cell]
                if all(lim is None or inside(a.GetBinLowEdge(b), a.GetBinUpEdge(b), lim) for a, b, lim in zip(axes, bins, limits)):
                    # a profile is weighted by the entries of its bins (ROOT's mean of x), the contents are mean values
                    content = obj.GetBinContent(*bins)
                    profile = any(obj.InheritsFrom(cls) for cls in ("TProfile", "TProfile2D", "TProfile3D"))
                    values.append((content, axes[0].GetBinCenter(bins[0]), obj.GetBinEntries(obj.GetBin(*bins)) if profile else content))
        contents = [v for v, _, _ in values]
        if word == "integral":
            return sum(contents)
        if word in ("maximum", "minimum"):
            return (max if word == "maximum" else min)(contents) if contents else 0.0
        total = sum(w for _, _, w in values)
        mean = sum(w * x for _, x, w in values) / total if total else 0.0
        if word == "mean":
            return mean
        return math.sqrt(max(sum(w * (x - mean) ** 2 for _, x, w in values) / total, 0.0)) if total else 0.0
    if obj.InheritsFrom("TGraph") and not obj.InheritsFrom("TGraphPolar"):
        if word == "entries":
            return obj.GetN()
        if word in ("maximum", "minimum") and obj.GetN():
            ys = [obj.GetY()[k] for k in range(obj.GetN()) if frame["X"][0] <= obj.GetX()[k] <= frame["X"][1]]
            return (max if word == "maximum" else min)(ys) if ys else 0.0
    if obj.InheritsFrom("TGraph2D"):
        if word == "entries":
            return obj.GetN()
        if word in ("maximum", "minimum") and obj.GetN():
            zs = [obj.GetZ()[k] for k in range(obj.GetN()) if frame["X"][0] <= obj.GetX()[k] <= frame["X"][1] and frame["Y"][0] <= obj.GetY()[k] <= frame["Y"][1]]
            return (max if word == "maximum" else min)(zs) if zs else 0.0
    if obj.InheritsFrom("TGraphPolar"):
        if word == "entries":
            return obj.GetN()
        if word in ("maximum", "minimum") and obj.GetN():
            return (max if word == "maximum" else min)(obj.GetY()[k] for k in range(obj.GetN()))
    return None


for i, case in enumerate(todo):
    name, kind, word = f"n{i}", case["kind"], case["word"]
    drawn = run.objects(name, "numbers")
    found = labels(name, "numbers")
    what = describe(case)
    if not drawn or len(found) != 1:
        check(f"{what}: one label with the value", False, f"labels {found}, {len(drawn)} drawn")
        continue
    frame = run.frame(name, "numbers") or {"X": (-math.inf, math.inf), "Y": (-math.inf, math.inf)}
    want = reference(drawn[0], word, frame)
    placeholder = f"<{word}{FORMAT}>" if case["place"] == "legend" else f"<{word}{FORMAT}:1>"
    warned = [line for line in run.messages(name, "numbers") if "is not available" in line]
    if want is None:
        check(f"{what}: left as it is, with a warning naming the data", found == [f"value {placeholder}"] and len(warned) == 1 and data_name(kind) in warned[0], f"{found} {warned}")
        continue
    got = found[0].split(" ", 1)[1]
    ok = False
    try:
        ok = math.isclose(float(got), want, rel_tol=1e-6, abs_tol=1e-9 * max(1.0, abs(want)))
    except ValueError:
        pass
    check(f"{what}: the value of the data drawn", ok and not warned, f"{got} instead of {want:.9g} {warned}")

for name, (_, _, want) in DESCRIPTIONS.items():
    found = labels(name, "descriptions")
    check(f"{name}: the label reads {want!r}", found == [want], f"{found}")

found = labels("text_numbers", "texts")
h = [o for o in run.objects("text_numbers", "texts", 2) if o.InheritsFrom("TH1")]
first = f"3 6.0 {h[0].Integral():.1f}" if h else None
check("a text names data of its pad and of another pad drawn after it by number", found[:1] == [first], f"{found[:1]} instead of {first}")
check("what cannot be replaced is left as it is", found[1:] == ["<mean> <mean:5> <mean:1@7> <meen:1> <mean:123456> plain #mu < 3", " ".join(MALFORMED[:4]), " ".join(MALFORMED[4:]) + " <mean value>"], f"{found[1:]}")
messages = run.messages("text_numbers", "texts")
for text in ("<mean> in a text needs the number", "<mean:5>: there is no data 5 drawn in this pad", "<mean:1@7>: there is no data 1 drawn in pad 7", "Placeholder <meen:1> is not valid", "<mean:123456>: there is no data 123456"):
    check(f"a warning: {text}", sum(text in line for line in messages) == 1, " | ".join(messages))
for text in MALFORMED:
    check(f"a warning: {text} is not valid", sum(f"Placeholder {text} is not valid: the data is named as" in line for line in messages) == 1, " | ".join(messages))
check("a known word followed by other text is text", not any("<mean value>" in line for line in messages), " | ".join(messages))
check("a number in a legend label is left as it is, with a warning", labels("legend_number", "texts") == ["<mean:1>"] and any("a legend label shows the data of its row" in m for m in run.messages("legend_number", "texts")), f"{labels('legend_number', 'texts')}")
for name in ("stats_title", "stats_title_graph"):
    padObj = run.pad(name, "texts")
    stats = [o for o in padObj.GetListOfPrimitives() if o.InheritsFrom("TPaveStats")] if padObj else []
    title = stats[0].GetListOfLines().First().GetTitle() if stats else None
    want = "h s 1000" if name == "stats_title" else "h s"
    check(f"{name}: the title of the statistics box reads {want!r}", title == want, f"{title!r}")
check("a format that does not fit is an error, the placeholder left as it is", labels("bad_format", "texts") == ["<mean[q]>"] and any("Incompatible format string in <mean[q]>" in m for m in run.messages("bad_format", "texts")), f"{labels('bad_format', 'texts')}")
finish()
