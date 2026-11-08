"""A range set on data (Data::SetRangeX, SetRangeY) limits what of it is drawn: the bins holding the range and those between them, the
points inside it (and their y inside a y range), a 2d graph's map inside it, a function over exactly the range; the y range of a 1d
histogram is its minimum and maximum. It leaves the frame alone: data drawn first gives the frame of all of it (the pad's axis ranges are
there for the frame), but a function, whose range is where it is drawn at all. A range without any bin of the data is warned about and
the whole data is drawn. A 2d function in a 3d view is drawn over the range of the frame's axes, in every coordinate system of the view
(the view holds the coordinates it paints in: -1 to 1 for polar, cylindrical, spherical and pseudo-rapidity systems, log10 on a log axis).
Cases: every data kind of a flat frame × a range on x or y (2d data, graphs) inside the data × the data first in its pad or after a
wider histogram that gives the frame, and the named cases.
Reference: the input objects (bins holding the range ends, points inside it).
Covers: f49b65a (a data range without bins), the range of a 2d graph (y ignored, x lost in the saved canvas), the range of functions in 3d
views (backlog 44 of the old harness: cut to the coordinates of the view), and the named cases.
"""

from srptest import DATA_1D, DATA_2D, DATA_KINDS, ROOT, Project, S, add_data, axis_range, base_of, cases, check, close, data_objects, describe, finish

RANGES = {"X": (3.3, 6.6), "Y": (3.4, 6.7)}
EFFICIENCY_Y = (0.3, 0.6)  # the values of an efficiency are between 0 and 1
KINDS = tuple(k for k in DATA_1D + DATA_2D)


def valid(case):
    kind, axis = case["kind"], case["axis"]
    dim, cls, props = DATA_KINDS[kind]
    if axis == "Y":
        return dim == 2 or (cls.startswith("TGraph") or cls == "TEfficiency") or cls in ("TH1D", "TProfile") and "projected" not in props
    return True


todo = cases({"kind": list(KINDS), "axis": ["X", "Y"], "place": ["first", "after"]}, valid, always=lambda c: c["place"] == "first")

p = Project("data_ranges")
p.inputs()
inputs = data_objects()
wide = ROOT.TH1D("wide", "", 40, -20.0, 40.0)
wide2 = ROOT.TH2D("wide2", "", 30, -20.0, 40.0, 30, -20.0, 40.0)
for b in range(1, 41):
    wide.SetBinContent(b, 1.0)
for b in range(1, 31):
    for c in range(1, 31):
        wide2.SetBinContent(b, c, 1.0)
p.source("other", {"wide": wide, "wide2": wide2})


def range_of(case):
    return EFFICIENCY_Y if case["axis"] == "Y" and DATA_KINDS[case["kind"]][1] == "TEfficiency" and DATA_KINDS[case["kind"]][0] == 1 else RANGES[case["axis"]]


for kind in KINDS:
    plot = S.Plot(f"alone_{kind}", "ranges", base_of(kind))
    add_data(plot[1], kind)
    p.pm.AddPlot(plot)
for i, case in enumerate(todo):
    kind = case["kind"]
    plot = S.Plot(f"r{i}", "ranges", base_of(kind))
    if case["place"] == "after":
        plot[1].AddData("wide" if DATA_KINDS[kind][0] == 1 else "wide2", "other")
    data = add_data(plot[1], kind)
    (data.SetRangeX if case["axis"] == "X" else data.SetRangeY)(*range_of(case))
    p.pm.AddPlot(plot)
VIEWS = {"surf_col": S.surf_col, "surf_col_polar": S.surf_col_polar, "lego_polar": S.lego_polar, "cyl": "SURF1 CYL", "sph": "SURF1 SPH", "psr": "LEGO1 PSR"}
VIEW_RANGES = {"none": None, "x": ("X", 3.0, 7.0), "y": ("Y", 2.0, 6.0), "logx": ("logx",)}
viewCases = cases({"kind": [k for k in DATA_2D if DATA_KINDS[k][1] == "TF2"], "view": list(VIEWS), "range": list(VIEW_RANGES)})
for i, case in enumerate(viewCases):
    plot = S.Plot(f"v{i}", "views", "2d")
    add_data(plot[1], case["kind"]).SetDrawOption(VIEWS[case["view"]])
    setting = VIEW_RANGES[case["range"]]
    if setting and setting[0] == "logx":
        plot[1]["X"].SetLog()
    elif setting:
        plot[1][setting[0]].SetRange(*setting[1:])
    p.pm.AddPlot(plot)
plot = S.Plot("no_bins", "named", "1d")
plot[1].AddData("hist", "inputs").SetRangeX(20.0, 30.0)
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def points(graph):
    return [(graph.GetX()[k], graph.GetY()[k]) for k in range(graph.GetN())]


for i, case in enumerate(todo):
    kind, axis = case["kind"], case["axis"]
    what = describe(case)
    lo, hi = range_of(case)
    drawn = run.objects(f"r{i}", "ranges")
    obj = drawn[-1] if drawn else None
    if not obj:
        check(f"{what}: drawn", False, " | ".join(run.messages(f"r{i}", "ranges")))
        continue
    frame = run.frame(f"r{i}", "ranges")
    if case["place"] == "first" and not obj.InheritsFrom("TF1"):
        alone = run.frame(f"alone_{kind}", "ranges")
        check(f"{what}: the frame of all of the data", alone is not None and all(close(a, b, 1e-9) for ax_ in ("X", "Y") for a, b in zip(frame[ax_], alone[ax_])), f"{frame} instead of {alone}")
    if obj.InheritsFrom("TH2Poly"):
        kept = [b for b in obj.GetBins()]
        ok = all(lo <= 0.5 * ((b.GetXMin() + b.GetXMax()) if axis == "X" else (b.GetYMin() + b.GetYMax())) <= hi for b in kept) and len(kept) > 0
        check(f"{what}: only the polygons with their centre inside drawn", ok, f"{len(kept)} polygons")
    elif obj.InheritsFrom("TH1") and (axis == "X" or obj.GetDimension() > 1):
        ax = obj.GetXaxis() if axis == "X" else obj.GetYaxis()
        source = inputs[kind] if kind in inputs else None
        ref = source.GetXaxis() if source and source.InheritsFrom("TH1") and axis == "X" else (source.GetYaxis() if source and source.InheritsFrom("TH1") else ax)
        first, last = ref.FindFixBin(lo), ref.FindFixBin(hi)
        shown = (ax.GetBinLowEdge(ax.GetFirst()), ax.GetBinUpEdge(ax.GetLast()))
        want = (ref.GetBinLowEdge(first), ref.GetBinUpEdge(last))
        check(f"{what}: the bins holding the range and those between drawn", close(shown[0], want[0], 1e-9) and close(shown[1], want[1], 1e-9), f"{shown} instead of {want}")
    elif obj.InheritsFrom("TH1"):
        check(f"{what}: the minimum and maximum of a 1d histogram", close(obj.GetMinimum(), lo, 1e-9) and close(obj.GetMaximum(), hi, 1e-9), f"{obj.GetMinimum()}, {obj.GetMaximum()}")
    elif obj.InheritsFrom("TGraph2D"):
        coord = obj.GetX() if axis == "X" else obj.GetY()
        found = sorted(coord[k] for k in range(obj.GetN()))
        source = inputs[kind]
        sourceCoord = source.GetX() if axis == "X" else source.GetY()
        want = sorted(v for v in (sourceCoord[k] for k in range(source.GetN())) if lo <= v <= hi)
        check(f"{what}: the points inside drawn (the map of their interpolation, also in the saved canvas)", found == want, f"{len(found)} points instead of {len(want)}")
    elif obj.InheritsFrom("TGraph"):
        coord = 0 if axis == "X" else 1
        found = points(obj)
        source = inputs.get(kind)
        if source is not None and source.InheritsFrom("TGraph"):
            want = sorted(pt for pt in points(source) if lo <= pt[coord] <= hi)
            check(f"{what}: the points inside drawn", sorted(found) == want, f"{len(found)} points instead of {len(want)}")
        else:  # a TEfficiency: the graph ROOT makes of it
            check(f"{what}: only the points inside drawn", found and all(lo <= pt[coord] <= hi for pt in found), f"{found[:3]}")
    elif obj.InheritsFrom("TF1"):
        rng = (obj.GetXmin(), obj.GetXmax()) if axis == "X" else (obj.GetYmin(), obj.GetYmax())
        check(f"{what}: the function drawn over exactly the range", close(rng[0], lo, 1e-9) and close(rng[1], hi, 1e-9), f"{rng}")
        if case["place"] == "first":
            fx = frame[axis]
            check(f"{what}: the frame of the function's range", close(fx[0], lo, 1e-9) and close(fx[1], hi, 1e-9), f"{fx}")
    else:
        check(f"{what}: drawn as data of a known kind", False, obj.ClassName())

for i, case in enumerate(viewCases):
    what = describe(case)
    drawn = [o for o in run.objects(f"v{i}", "views") if o.InheritsFrom("TF2")]
    if not drawn:
        check(f"{what}: drawn", False, " | ".join(run.messages(f"v{i}", "views")))
        continue
    lo, hi = axis_range()
    want = [lo, hi, lo, hi]
    setting = VIEW_RANGES[case["range"]]
    if setting and setting[0] in ("X", "Y"):
        want[(0 if setting[0] == "X" else 2) : (2 if setting[0] == "X" else 4)] = setting[1:]
    found = [drawn[0].GetXmin(), drawn[0].GetXmax(), drawn[0].GetYmin(), drawn[0].GetYmax()]
    check(f"{what}: drawn over the range of the frame's axes", all(close(a, b, 1e-6) for a, b in zip(found, want)), f"{found} instead of {want}")

warned = [m for m in run.messages("no_bins", "named") if "contains none of its bins" in m]
drawn = run.objects("no_bins", "named")
ax = drawn[0].GetXaxis() if drawn else None
check("a range without any bin of the data: warned, the whole data drawn", len(warned) == 1 and ax is not None and (ax.GetFirst(), ax.GetLast()) in ((1, ax.GetNbins()), (0, 0)), f"{warned} {(ax.GetFirst(), ax.GetLast()) if ax else None}")
finish()
