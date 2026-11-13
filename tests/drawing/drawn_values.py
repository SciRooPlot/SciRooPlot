"""What is drawn shows the values of the input, also when an alias draws it as the other type:
- a histogram drawn as a graph (points_line, bars, brackets, the clipped steps ...) has a point at the centre of each bin it draws (in its
  range, not empty) with the content, half the bin width as x error and the bin error as y error, and keeps the statistics box and the
  placeholders of the histogram; its clipped steps have the frame of the steps they clip;
- a graph drawn as a histogram (hist, bars, text ...) has a bin per point holding its value and the mean of its y errors, the bins
  touching where the x errors of neighbouring points meet, an empty bin in a gap between them (for area_smooth, whose curve goes
  through every bin, the two bins meeting in its middle), else half way between the points (documented rule);
- points_text writes the value of every point that is drawn and of no other: the bins and points inside the frame and the ranges of the
  data, polar graphs at their points on the polar grid;
- the colours of the 2d data of a flat pad span the values of all of it: the colour axis covers the range each data item has alone (for
  2d graphs with errors the range of their values, which ROOT's frame of such a graph widens by the errors), at each end the user did not
  set (Z SetMinRange or SetMaxRange alone).
Covers: the smooth curve of a graph with gaps between its x errors dipping to zero in each gap, converted histograms losing their statistics box and placeholders, points_text writing values of points outside the frame and
ranges, and a single value at the wrong place for polar graphs, 2d functions drawn after other data cut at the colour axis of the frame.
"""

import math

from srptest import DATA_KINDS, ROOT, Project, S, add_data, alias_table, cases, check, close, data_objects, describe, drawn_type, finish, graph_values, has_variant, source_of, values_diff

TABLE = alias_table()
ONE_D = tuple(k for k, (dim, _, props) in DATA_KINDS.items() if dim == 1 and drawn_type(k) in ("hist1d", "graph") and "polar" not in props)
CONVERTING = {t: [a for a, row in TABLE.items() if row[t] in ("graph", "hist1d")] for t in ("hist1d", "graph")}
STEPS = ("hist_clipped", "hist_open_clipped", "curve_clipped")  # graphs of the steps or the bin centres (their values: clipped_steps.py)
VARIANTS = ("normal", "negative", "single", "unsorted", "overflow", "zeros")
LABEL = "N=<entries> mean=<mean> integral=<integral> max=<maximum> min=<minimum>"

p = Project("drawn_values")
p.inputs(VARIANTS)
INPUTS = {variant: data_objects(variant) for variant in VARIANTS}


def plot_of(name, group, kind, variant, alias, setup=lambda plot, data: None):
    plot = S.Plot(name, group, "1d")
    data = add_data(plot[1], kind, source_of(variant), variant).SetDrawOption(getattr(S, alias))
    setup(plot, data)
    p.pm.AddPlot(plot)


# converted data, beside the same data drawn as points (the reference: drawn as its own type)
converted = cases(
    {"kind": list(ONE_D), "alias": sorted({a for v in CONVERTING.values() for a in v}), "variant": list(VARIANTS)},
    valid=lambda c: c["alias"] in CONVERTING[drawn_type(c["kind"])] and has_variant(c["kind"], c["variant"]),
    always=lambda c: c["kind"] in ("hist", "graph_errors") and c["variant"] == "normal",
    named=[
        {"kind": "hist", "alias": "points_line", "variant": "normal", "why": "statistics box and placeholders of a histogram drawn as graph"},
        {"kind": "graph", "alias": "hist", "variant": "unsorted", "why": "a graph in no order of x drawn as a histogram"},
        {"kind": "graph_errors", "alias": "area_smooth", "variant": "normal", "why": "no empty bins in the gaps between x errors under a smooth curve"},
        {"kind": "graph_bent", "alias": "area_smooth", "variant": "normal", "why": "no empty bins in the gaps between asymmetric x errors under a smooth curve"},
    ],
)


for i, case in enumerate(converted):
    hist = drawn_type(case["kind"]) == "hist1d"

    def boxes(plot, data, hist=hist):
        data.SetLegendLabel(LABEL)
        if hist:
            data.SetShowStats("emri")
        plot[1].AddLegend()

    plot_of(f"c{i}", "converted", case["kind"], case["variant"], case["alias"], boxes)
    plot_of(f"c{i}_ref", "converted", case["kind"], case["variant"], "points", boxes)

# points_text
SETUPS = {  # setup of the plot and the data
    "none": lambda plot, data: None,
    "data_x": lambda plot, data: data.SetRangeX(3.0, 7.0),
    "data_y": lambda plot, data: data.SetRangeY(2.0, 6.0),
    "pad_x": lambda plot, data: plot[1]["X"].SetRange(2.0, 6.0),
    "pad_y": lambda plot, data: plot[1]["Y"].SetRange(2.5, 5.5),
    "log_y": lambda plot, data: plot[1]["Y"].SetLog().SetRange(2.5, 5.5),
}
TEXT_KINDS = tuple(k for k, (dim, _, props) in DATA_KINDS.items() if dim == 1 and TABLE["points_text"][drawn_type(k)] != "-")
texts = cases(
    {"kind": list(TEXT_KINDS), "setup": list(SETUPS), "variant": ["normal", "negative"]},
    valid=lambda c: has_variant(c["kind"], c["variant"]) and not ("polar" in DATA_KINDS[c["kind"]][2] and c["setup"] != "none"),
    always=lambda c: c["kind"] in ("hist", "graph") or "polar" in DATA_KINDS[c["kind"]][2],
    named=[
        {"kind": "hist", "setup": "pad_y", "variant": "normal", "why": "values outside the frame not written"},
        {"kind": "graph_polar", "setup": "none", "variant": "normal", "why": "the values of a polar graph at its points"},
    ],
)
for i, case in enumerate(texts):
    plot_of(f"t{i}", "texts", case["kind"], case["variant"], "points_text", SETUPS[case["setup"]])

# the frames of clipped steps
CLIPPED = {"hist_clipped": "hist", "hist_open_clipped": "hist_open", "curve_clipped": "curve"}
clipped = cases(
    {"kind": [k for k in ONE_D if drawn_type(k) == "hist1d"], "alias": list(CLIPPED), "variant": ["normal", "negative", "single"], "log": [False, True]},
    valid=lambda c: has_variant(c["kind"], c["variant"]),
    always=lambda c: c["kind"] == "hist",
)
for i, case in enumerate(clipped):
    for suffix, alias in (("", case["alias"]), ("_ref", CLIPPED[case["alias"]])):
        plot_of(f"s{i}{suffix}", "clipped", case["kind"], case["variant"], alias, lambda plot, data, log=case["log"]: plot[1]["Y"].SetLog(log))

# the colour axis of the 2d data of a pad: a pair of kinds (the second shifted to negative values) and each of them alone
FLAT_2D = tuple(k for k, (dim, _, _) in DATA_KINDS.items() if dim == 2)
USER_Z = {"none": (None, None), "min": (-1e6, None), "max": (None, 1e6)}  # one end of Z set by the user: the other spans all data
pairs = cases(
    {"first": list(FLAT_2D), "second": list(FLAT_2D), "user": list(USER_Z)},
    valid=lambda c: has_variant(c["second"], "negative"),
    always=lambda c: c["first"] == "hist2d" and c["user"] == "none",
    named=({"first": "hist2d", "second": "hist2d", "user": "max", "why": "the minimum spans all data when only the maximum is set"},),
)
for i, case in enumerate(pairs):
    for suffix, items in (("", ((case["first"], "normal"), (case["second"], "negative"))), ("_first", ((case["first"], "normal"),)), ("_second", ((case["second"], "negative"),))):
        plot = S.Plot(f"z{i}{suffix}", "colours", "2d")
        for kind, variant in items:
            add_data(plot[1], kind, source_of(variant), variant)
        userMin, userMax = USER_Z[case["user"]]
        if suffix == "" and userMin is not None:
            plot[1]["Z"].SetMinRange(userMin)
        if suffix == "" and userMax is not None:
            plot[1]["Z"].SetMaxRange(userMax)
        p.pm.AddPlot(plot)
run = p.plot()


def bins_drawn(h):
    """the bins of a drawn histogram ROOT shows a marker for: in its range, not empty"""
    axis = h.GetXaxis()
    return [b for b in range(axis.GetFirst(), axis.GetLast() + 1) if h.GetBinContent(b) != 0 or h.GetBinError(b) != 0]


def as_graph(h, xErrors):
    """the points a histogram drawn as a graph must have (the x errors half the bin width, if the alias draws them)"""
    found = bins_drawn(h)
    half = [0.5 * h.GetBinWidth(b) if xErrors else 0.0 for b in found]
    errors = [h.GetBinError(b) for b in found]
    return {"x": [h.GetBinCenter(b) for b in found], "y": [h.GetBinContent(b) for b in found], "ErrorXlow": half, "ErrorXhigh": half, "ErrorYlow": errors, "ErrorYhigh": errors}


def as_hist(g, alias):
    """the bins a graph drawn as a histogram must have: (edges, contents, errors), by the documented rule"""
    points = sorted(zip(*(graph_values(g)[k] for k in ("x", "y")), range(g.GetN())))
    xs = [x for x, _, _ in points]
    ex = [(max(g.GetErrorXlow(i), 0.0), max(g.GetErrorXhigh(i), 0.0)) for _, _, i in points]
    ey = [0.5 * (max(g.GetErrorYlow(i), 0.0) + max(g.GetErrorYhigh(i), 0.0)) for _, _, i in points]
    edges, contents, errors = [], [], []
    if all(lo + hi > 0 for lo, hi in ex) and all(xs[j] + ex[j][1] <= xs[j + 1] - ex[j + 1][0] + 1e-6 * (ex[j][0] + ex[j][1]) for j in range(len(xs) - 1)):
        for j, (x, (lo, hi)) in enumerate(zip(xs, ex)):
            if edges and abs(x - lo - edges[-1]) > 1e-6 * (lo + hi) and alias == "area_smooth":
                edges[-1] = 0.5 * (edges[-1] + x - lo)  # a gap: the bins meet in its middle
            elif not edges or abs(x - lo - edges[-1]) > 1e-6 * (lo + hi):
                if edges:  # a gap: an empty bin
                    contents.append(0.0)
                    errors.append(0.0)
                edges.append(x - lo)
            edges.append(x + hi)
            contents.append(points[j][1])
            errors.append(ey[j])
    else:
        n = len(xs)
        mids = [0.5 * (a + b) for a, b in zip(xs, xs[1:])]
        edges = [xs[0] - 0.5, xs[0] + 0.5] if n == 1 else [xs[0] - 0.5 * (xs[1] - xs[0]), *mids, xs[-1] + 0.5 * (xs[-1] - xs[-2])]
        contents, errors = [y for _, y, _ in points], ey
    return edges, contents, errors


def label_and_stats(name, group):
    pad = run.pad(name, group)
    legend = next((o for o in pad.GetListOfPrimitives() if o.InheritsFrom("TLegend")), None) if pad else None
    stats = next((o for o in pad.GetListOfPrimitives() if o.InheritsFrom("TPaveStats")), None) if pad else None
    rows = list(legend.GetListOfPrimitives()) if legend else []
    return rows[0].GetLabel() if rows else None, [line.GetTitle() for line in stats.GetListOfLines()] if stats else None


for i, case in enumerate(converted):
    what = describe(case)
    got, ref = run.drawn(f"c{i}", "converted"), run.drawn(f"c{i}_ref", "converted")
    if not ref:
        check(f"reference drawn: {what}", case["variant"] == "zeros" and drawn_type(case["kind"]) == "graph", " | ".join(run.errors()[:2]))
        continue
    reference = ref[0][0]
    if drawn_type(case["kind"]) == "hist1d":
        check(f"statistics box and placeholders of the histogram: {what}", label_and_stats(f"c{i}", "converted") == label_and_stats(f"c{i}_ref", "converted"), f"{label_and_stats(f'c{i}', 'converted')} vs {label_and_stats(f'c{i}_ref', 'converted')}")
        if case["alias"] in STEPS:
            continue
        expected = as_graph(reference, "xerr" in case["alias"] or case["alias"] in ("bars", "hbars", "brackets", "boxes", "boxes_nomarkers", "band", "band_smooth"))
        if not expected["x"]:
            check(f"a histogram without drawn bins draws nothing: {what}", not got, str([o.ClassName() for o, _ in got]))
            continue
        obj = got[0][0] if got else None
        check(f"drawn as a graph of the drawn bins: {what}", obj is not None and obj.InheritsFrom("TGraph") and not values_diff(expected, graph_values(obj)), values_diff(expected, graph_values(obj)) if obj and obj.InheritsFrom("TGraph") else str(obj))
    else:
        obj = got[0][0] if got else None
        if not obj or not obj.InheritsFrom("TH1"):
            check(f"drawn as a histogram: {what}", False, obj.ClassName() if obj else "not drawn")
            continue
        given = INPUTS[case["variant"]][case["kind"]]
        edges, contents, errors = as_hist(given.CreateGraph() if given.InheritsFrom("TEfficiency") else given, case["alias"])
        axis = obj.GetXaxis()
        found = ([axis.GetBinLowEdge(b) for b in range(1, axis.GetNbins() + 2)], [obj.GetBinContent(b) for b in range(1, axis.GetNbins() + 1)], [obj.GetBinError(b) for b in range(1, axis.GetNbins() + 1)])
        same = all(len(a) == len(b) and all(close(x, y, 1e-9) or abs(x - y) < 1e-12 for x, y in zip(a, b)) for a, b in zip((edges, contents, errors), found))
        check(f"drawn as the bins of the points: {what}", same, f"edges {found[0][:4]}..., contents {found[1][:4]}..., expected {edges[:4]}..., {contents[:4]}...")


def frame_of(name, group):
    frame = run.frame(name, group)
    return frame and (frame["X"], frame["Y"])


for i, case in enumerate(texts):
    what = describe(case)
    pad = run.pad(f"t{i}", "texts")
    drawn = run.drawn(f"t{i}", "texts")
    if not pad:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    if not drawn:  # no point in the ranges: the graph is not drawn
        check(f"no values without points: {what}", not [t for t in pad.GetListOfPrimitives() if t.IsA() == ROOT.TLatex.Class()])
        continue
    obj = drawn[0][0]
    labels = [(float(t.GetTitle()), t.GetX(), t.GetY()) for t in pad.GetListOfPrimitives() if t.IsA() == ROOT.TLatex.Class()]
    if obj.InheritsFrom("TGraphPolar"):
        grid = next(o for o in pad.GetListOfPrimitives() if o.InheritsFrom("TGraphPolargram"))
        radius = [(obj.GetPointY(j) - grid.GetRMin()) / (grid.GetRMax() - grid.GetRMin()) for j in range(obj.GetN())]
        turn = (grid.GetTMax() - grid.GetTMin()) / (2 * math.pi)
        points = [(obj.GetPointY(j), r * math.cos((obj.GetPointX(j) - grid.GetTMin()) / turn), r * math.sin((obj.GetPointX(j) - grid.GetTMin()) / turn)) for j, r in enumerate(radius) if 0 <= r <= 1]
    else:
        frame = run.frame(f"t{i}", "texts")
        (x0, x1), (y0, y1) = frame["X"], frame["Y"]
        if obj.InheritsFrom("TH1"):
            low = obj.GetMinimumStored() if obj.GetMinimumStored() != -1111 else -math.inf
            high = obj.GetMaximumStored() if obj.GetMaximumStored() != -1111 else math.inf
            values = [(obj.GetBinCenter(b), obj.GetBinContent(b)) for b in bins_drawn(obj) if low <= obj.GetBinContent(b) <= high]
        else:
            values = [(obj.GetPointX(j), obj.GetPointY(j)) for j in range(obj.GetN())]
        points = [(y, x, None) for x, y in values if x0 <= x <= x1 and y0 <= y <= y1]
    got = sorted((v, round(x, 6)) for v, x, _ in labels)
    want = sorted((float(f"{v:g}"), round(x, 6)) for v, x, _ in points)
    check(f"a value for every point drawn and no other: {what}", got == want, f"labels {got[:6]}, expected {want[:6]}")
    if obj.InheritsFrom("TGraphPolar"):
        gap = 0.015 * (pad.GetUymax() - pad.GetUymin())
        at = sorted((round(x, 6), round(y, 6)) for _, x, y in labels)
        check(f"values of a polar graph just above its points: {what}", at == sorted((round(x, 6), round(y + gap, 6)) for _, x, y in points), f"{at[:3]}")

for i, case in enumerate(clipped):
    what = describe(case)
    got, ref = frame_of(f"s{i}", "clipped"), frame_of(f"s{i}_ref", "clipped")
    check(f"the frame of the steps clipped: {what}", got is not None and ref is not None and all(close(a, b, 1e-9) for g, r in zip(got, ref) for a, b in zip(g, r)), f"{got} vs {ref}")
for i, case in enumerate(pairs):
    what = describe(case)
    ranges = [run.frame_hist(f"z{i}{suffix}", "colours") for suffix in ("", "_first", "_second")]
    if not all(ranges):
        check(f"plots made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    both, first, second = ((h.GetMinimum(), h.GetMaximum()) for h in ranges)
    if DATA_KINDS[case["second"]][1] in ("TGraph2DErrors", "TGraph2DAsymmErrors"):  # its values, not the z errors ROOT's frame of it covers
        graph = run.drawn(f"z{i}_second", "colours")[0][0]
        second = (graph.GetZmin(), graph.GetZmax())
    userMin, userMax = USER_Z[case["user"]]
    want = (min(first[0], second[0]) if userMin is None else userMin, max(first[1], second[1]) if userMax is None else userMax)
    check(f"the colour axis covers the values of all 2d data: {what}", close(both[0], want[0], 1e-9) and close(both[1], want[1], 1e-9), f"{both}, expected {want}")
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
