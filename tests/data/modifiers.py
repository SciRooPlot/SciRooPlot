"""Every modifier of the data on every data kind in every variant gives the result of the same operation done with plain ROOT on the data
as drawn without the modifier, or is reported as not supported for that kind and leaves the data as it is. The modifiers act in one
order, whatever the order they are set in: ScaleX/Y/Z of the axes, Rebin, DivideBinWidth, Smooth, Normalize, Cumulative,
NormalizeToMaximum, Scale. Histograms (also projected THn, TEfficiency 2d): ROOT's Rebin, Scale(1, "width") (a TH2Poly by bin area),
Smooth (1d and 2d), Integral, GetCumulative (1d), GetMaximum; profiles: their means, smoothed over the filled bins; graphs (also TEfficiency
1d): the integral is the sum of the values, the maximum the largest value, smoothing sorts the points (TGraphSmooth::SmoothSuper); 2d graphs:
NormalizeToMaximum and the scale factors; functions: the scale factors (of the arguments or of the value). Normalize and
NormalizeToMaximum within a range in x take the bins reaching into it, the points inside it. Each warning names its data, and every
warning is expected.
Covers: f934095 (TGraphMultiErrors scaled: both error dimensions follow); the Normalize range of a TH2Poly; graphs scaled by a negative
factor (Normalize of values summing up below zero) keep positive errors, low and high swapped; a TGraphMultiErrors sorted for Smooth keeps
each error with its point; messages after Cumulative name the data.
"""

import functools
import operator
import re

from srptest import DATA_KINDS, ROOT, VARIANTS, Project, S, add_data, base_of, cases, check, data_name, describe, finish, func_diff, graph_diff, graph_values, has_variant, hist_diff, source_of

RANGE = (3.0, 7.0)  # range in x of Normalize and NormalizeToMaximum: bin edges of the inputs
FACTOR = 3.7  # Scale
AXIS_FACTOR = 2.0  # ScaleX/Y/Z
GROUP = 3  # Rebin: 18 and 9 bins are multiples, the 10 of hist_var are not
REBIN = {"rebin_x": "X", "rebin_y": "Y", "rebin_z": "Z", "rebin_xy": "XY", "rebin_xyz": "XYZ"}  # the axes of each
MODIFIERS = {  # name: setter of the data
    "normalize": lambda d: d.Normalize(),
    "normalize_density": lambda d: d.Normalize(True),
    "normalize_range": lambda d: d.Normalize(False, *RANGE),
    "density": lambda d: d.Density(),
    "divide_width": lambda d: d.DivideBinWidth(),
    "norm_max": lambda d: d.NormalizeToMaximum(),
    "norm_max_range": lambda d: d.NormalizeToMaximum(*RANGE),
    "scale": lambda d: d.Scale(FACTOR),
    "scale_x": lambda d: d.ScaleX(AXIS_FACTOR),
    "scale_y": lambda d: d.ScaleY(AXIS_FACTOR),
    "scale_z": lambda d: d.ScaleZ(AXIS_FACTOR),
    "rebin_x": lambda d: d.RebinX(GROUP),
    "rebin_y": lambda d: d.RebinY(GROUP),
    "rebin_z": lambda d: d.RebinZ(GROUP),
    "rebin_xy": lambda d: d.RebinXY(GROUP, GROUP),
    "rebin_xyz": lambda d: d.RebinXYZ(GROUP, GROUP, GROUP),
    "smooth": lambda d: d.Smooth(1),
    "smooth_3": lambda d: d.Smooth(3),
    "cumulative": lambda d: d.Cumulative(),
    "cumulative_backward": lambda d: d.Cumulative(False),
}
COMBINATIONS = (  # set in this order, applied in the fixed one
    ("cumulative", "normalize"),  # a cumulative distribution ending at 1
    ("norm_max", "cumulative"),
    ("normalize", "rebin_x"),
    ("normalize_density", "divide_width"),
    ("scale", "scale_x", "norm_max"),
)


def family(obj):
    """how SciRooPlot treats the drawn data: hist, profile (1d to 3d), poly, graph, graph2d, func"""
    if obj.InheritsFrom("TF1"):
        return "func"
    if obj.InheritsFrom("TGraph2D"):
        return "graph2d"
    if obj.InheritsFrom("TGraph"):
        return "graph"
    if obj.InheritsFrom("TH2Poly"):
        return "poly"
    if any(obj.InheritsFrom(c) for c in ("TProfile", "TProfile2D", "TProfile3D")):
        return "profile"
    return "hist"


def sum_in_order(values):
    """the sum in order, as the C++ code adds (Python's sum compensates rounding: values summing up to almost zero would differ)"""
    return functools.reduce(operator.add, values, 0.0)


class Expected:
    """the reference of one case: the result and the warnings (fragments) it must give"""

    def __init__(self, name):
        self.name = name  # the data's name in messages
        self.warnings = []

    def unsupported(self, what, cls):
        self.warnings.append(f"{what} is not supported for {self.name} ({cls})")

    def warn(self, text):
        self.warnings.append(text)


def bins_in_range_x(h):
    """the cells of a histogram (without under- and overflow) reaching into RANGE in x"""
    if h.InheritsFrom("TH2Poly"):
        return [b.GetBinNumber() for b in h.GetBins() if b.GetXMin() < RANGE[1] and b.GetXMax() > RANGE[0]]
    axis = h.GetXaxis()
    xs = [i for i in range(1, axis.GetNbins() + 1) if axis.GetBinLowEdge(i) < RANGE[1] and axis.GetBinUpEdge(i) > RANGE[0]]
    return [c for c in inner_cells(h) if cell_x(h, c) in xs]


def inner_cells(h):
    if h.InheritsFrom("TH2Poly"):
        return list(range(1, h.GetNumberOfBins() + 1))
    return [c for c in range(h.GetNcells()) if not h.IsBinUnderflow(c) and not h.IsBinOverflow(c)]


def cell_x(h, c):
    bx, by, bz = ROOT.std.vector("int")(1), ROOT.std.vector("int")(1), ROOT.std.vector("int")(1)
    h.GetBinXYZ(c, bx.data(), by.data(), bz.data())
    return bx[0]


def cell_width(h, c):
    """the width (area, volume) of a cell"""
    if h.InheritsFrom("TH2Poly"):
        return next(b.GetArea() for b in h.GetBins() if b.GetBinNumber() == c)
    bx, by, bz = ROOT.std.vector("int")(1), ROOT.std.vector("int")(1), ROOT.std.vector("int")(1)
    h.GetBinXYZ(c, bx.data(), by.data(), bz.data())
    width = h.GetXaxis().GetBinWidth(bx[0])
    if h.GetDimension() > 1:
        width *= h.GetYaxis().GetBinWidth(by[0])
    if h.GetDimension() > 2:
        width *= h.GetZaxis().GetBinWidth(bz[0])
    return width


def set_cell(h, c, content, error):
    if h.InheritsFrom("TProfile") or h.InheritsFrom("TProfile2D") or h.InheritsFrom("TProfile3D"):
        raise ValueError("profiles are compared through their means")
    h.SetBinContent(c, content)
    h.SetBinError(c, error)


def means(p):
    """a profile as the histogram of its means (contents and errors per cell), for the comparison"""
    dim = p.GetDimension()
    h = {1: lambda: ROOT.TH1D("m", "", 1, 0, 1), 2: lambda: ROOT.TH2D("m", "", 1, 0, 1, 1, 0, 1), 3: lambda: ROOT.TH3D("m", "", 1, 0, 1, 1, 0, 1, 1, 0, 1)}[dim]()
    for axis in "XYZ"[:dim]:
        a = getattr(p, f"Get{axis}axis")()
        edges = ROOT.std.vector("double")([a.GetBinLowEdge(i) for i in range(1, a.GetNbins() + 2)])
        getattr(h, f"Get{axis}axis")().Set(a.GetNbins(), edges.data())
    h.SetBinsLength()
    for c in range(p.GetNcells()):
        h.SetBinContent(c, p.GetBinContent(c))
        h.SetBinError(c, p.GetBinError(c))
    return h


def scale_hist_axis(h, axis, factor):
    a = getattr(h, f"Get{axis}axis")()
    edges = ROOT.std.vector("double")([a.GetBinLowEdge(i) * factor for i in range(1, a.GetNbins() + 2)])
    a.Set(a.GetNbins(), edges.data())


def reference_hist(base, mods, exp):
    """histograms and profiles (profiles as the histogram of their means): the operations of plain ROOT"""
    h = base.Clone("reference")
    h.SetDirectory(0)
    cls, dim, fam = base.ClassName(), base.GetDimension(), family(base)
    profile, poly = fam == "profile", fam == "poly"
    content = 1.0
    for axis, mod in (("X", "scale_x"), ("Y", "scale_y"), ("Z", "scale_z")):
        if mod not in mods:
            continue
        if "XYZ".index(axis) == dim:
            content *= AXIS_FACTOR  # the value axis
        elif "XYZ".index(axis) > dim:
            exp.warn(f"Cannot scale z axis of 1d histogram {exp.name}")
        elif poly:
            exp.unsupported(f"Scale{axis}", cls)
        else:
            scale_hist_axis(h, axis, AXIS_FACTOR)
    groups = [axis for mod, axes in REBIN.items() if mod in mods for axis in axes]
    for axis in groups:
        if "XYZ".index(axis) >= dim or (poly and axis != "Z"):
            exp.unsupported(f"Rebin{axis}", cls)
    if groups and profile and dim == 3:
        exp.unsupported("Rebinning", cls)
    elif groups and not poly:
        level = ROOT.gErrorIgnoreLevel
        ROOT.gErrorIgnoreLevel = ROOT.kError
        for axis in [a for a in "XYZ"[:dim] if a in groups]:
            a = getattr(h, f"Get{axis}axis")()
            if a.GetNbins() % GROUP:
                exp.warn(f"Rebin{axis}({GROUP}): the {a.GetNbins()} bins of {exp.name} are no multiple of it")
            getattr(h, f"Rebin{axis}" if dim > 1 else "Rebin")(GROUP)
        ROOT.gErrorIgnoreLevel = level
    density = "normalize_density" in mods
    if "divide_width" in mods or "density" in mods:
        if profile:
            exp.unsupported("DivideBinWidth", cls)
        elif poly:
            for c in inner_cells(h):
                width = cell_width(h, c)
                set_cell(h, c, h.GetBinContent(c) / width, h.GetBinError(c) / width)
            density = True
        else:
            h.Scale(1.0, "width")
            density = True
    smooth = 3 if "smooth_3" in mods else (1 if "smooth" in mods else 0)
    if profile and smooth and dim < 3:
        h = means(h)  # the means are smoothed
    if smooth:
        if dim == 3 or poly:
            exp.unsupported("Smooth", cls)
        elif profile:
            filled = [c for c in inner_cells(base) if base.GetBinEntries(c) > 0]
            if dim == 1:
                if len(filled) < 3:
                    exp.warn(f"Smooth needs at least three filled bins in profile histogram {exp.name}")
                else:
                    values = ROOT.std.vector("double")([h.GetBinContent(c) for c in filled])
                    ROOT.TH1.SmoothArray(len(filled), values.data(), smooth)
                    for c, v in zip(filled, values):
                        h.SetBinContent(c, v)
            else:  # the kernel of TH2::Smooth over the filled cells only
                total, weight = h.Clone("total"), h.Clone("weight")
                total.Sumw2(False)
                weight.Sumw2(False)
                for c in range(h.GetNcells()):
                    weight.SetBinContent(c, 1.0 if base.GetBinEntries(c) > 0 else 0.0)
                total.Smooth(smooth)
                weight.Smooth(smooth)
                for c in filled:
                    h.SetBinContent(c, total.GetBinContent(c) / weight.GetBinContent(c))
        elif dim == 1 and h.GetNbinsX() < 3:
            exp.warn(f"Smooth needs at least three bins in histogram {exp.name}")
        else:
            h.Smooth(smooth)
    if any(m.startswith("normalize") for m in mods) or "density" in mods:
        cells = bins_in_range_x(h) if "normalize_range" in mods else inner_cells(h)
        integral = sum_in_order(h.GetBinContent(c) * (cell_width(h, c) if density else 1.0) for c in cells)
        if not cells:
            exp.warn(f"The range of Normalize for {exp.name} contains no bins")
        elif integral == 0:
            exp.warn(f"Cannot normalize {exp.name} because its integral is zero")
        else:
            h.Scale(1.0 / integral)
    if "cumulative" in mods or "cumulative_backward" in mods:
        if profile and dim == 1:
            exp.warn(f"Cumulative is not supported for profile histogram {exp.name}")
        elif dim > 1:
            exp.unsupported("Cumulative", cls)
        else:
            if density:
                for b in range(1, h.GetNbinsX() + 1):
                    h.SetBinContent(b, h.GetBinContent(b) * h.GetBinWidth(b))
                    h.SetBinError(b, h.GetBinError(b) * h.GetBinWidth(b))
            h = h.GetCumulative("cumulative" in mods, "reference_cumulative")
    factor = None
    if "norm_max" in mods or "norm_max_range" in mods:
        cells = bins_in_range_x(h) if "norm_max_range" in mods else inner_cells(h)
        maximum = max(h.GetBinContent(c) for c in cells) if cells else None
        if maximum == 0:
            exp.warn(f"Cannot normalize {exp.name} to maximum because it is zero")
        elif maximum is not None:
            factor = 1.0 / maximum
    if "scale" in mods:
        factor = (factor or 1.0) * FACTOR
    if content != 1.0:
        factor = (factor or 1.0) * content
    if factor:
        h.Scale(factor)
    if not poly:
        h.ClearUnderflowAndOverflow()  # not drawn (the drawn histogram has them cleared)
    return means(h) if family(h) == "profile" else h


def reference_graph(base, mods, exp):
    """1d graphs: the values as plain numbers"""
    cls = base.ClassName()
    v = graph_values(base)
    n = len(v["x"])
    for axis in [axis for mod, axes in REBIN.items() if mod in mods for axis in axes]:
        exp.unsupported(f"Rebin{axis}", cls)
    if "divide_width" in mods or "density" in mods:
        exp.unsupported("DivideBinWidth", cls)
    if "cumulative" in mods or "cumulative_backward" in mods:
        exp.unsupported("Cumulative", cls)
    if "scale_z" in mods:
        exp.warn(f"Cannot scale z axis of graph {exp.name}")

    def scale(keys, factor):
        for key in [k for k in keys if k in v]:
            v[key] = [value * factor for value in v[key]]

    if "scale_x" in mods:
        scale(["x", "ErrorXlow", "ErrorXhigh"], AXIS_FACTOR)
    smooth = 3 if "smooth_3" in mods else (1 if "smooth" in mods else 0)
    if smooth:
        if n < 4:
            exp.warn(f"Smooth needs at least 4 points, {exp.name} has {n}")
        else:
            order = sorted(range(n), key=lambda i: v["x"][i])
            v = {k: [values[i] for i in order] for k, values in v.items()}
            graph = ROOT.TGraph(n)
            for i in range(n):
                graph.SetPoint(i, v["x"][i], v["y"][i])
            smoother = ROOT.TGraphSmooth()
            for _ in range(smooth):
                smoothed = smoother.SmoothSuper(graph)
                for i in range(n):
                    graph.SetPointY(i, smoothed.GetPointY(i))
            v["y"] = [graph.GetPointY(i) for i in range(n)]
    factor = None
    if any(m.startswith("normalize") for m in mods) or "density" in mods:
        inside = [y for x, y in zip(v["x"], v["y"]) if "normalize_range" not in mods or RANGE[0] <= x <= RANGE[1]]
        if not inside and n:
            exp.warn(f"The range of Normalize for {exp.name} contains no points")
        elif sum_in_order(inside) == 0:
            exp.warn(f"Cannot normalize graph {exp.name} because the sum of its values is zero")
        else:
            factor = 1.0 / sum_in_order(inside)
        if "normalize_density" in mods:
            exp.warn(f"Cannot normalize graph {exp.name} by width")
    if "norm_max" in mods or "norm_max_range" in mods:
        inside = [y for x, y in zip(v["x"], v["y"]) if "norm_max_range" not in mods or RANGE[0] <= x <= RANGE[1]]
        if not n:
            exp.warn(f"Cannot normalize graph {exp.name} to maximum because it has no points")
        elif not inside:
            exp.warn(f"The range of NormalizeToMaximum for {exp.name} contains no points")
        elif max(inside) == 0:
            exp.warn(f"Cannot normalize graph {exp.name} to maximum because it is zero")
        else:
            factor = 1.0 / max(inside)
    for mod, f in (("scale", FACTOR), ("scale_y", AXIS_FACTOR)):
        if mod in mods:
            factor = (factor or 1.0) * f
    if factor:
        flip(v, "y", factor)
    return v


def flip(v, axis, factor):
    """values times a factor: errors stay positive, a negative factor swaps low and high errors (also of every error set)"""
    v[axis] = [value * factor for value in v[axis]]
    for key in [k for k in v if k != axis and (k.startswith((f"Error{axis.upper()}", f"e{axis}")))]:
        v[key] = [abs(factor) * value for value in v[key]]
    if factor < 0:
        pairs = [(f"Error{axis.upper()}low", f"Error{axis.upper()}high")] + [(k, k[:-3] + "high") for k in v if k.startswith(f"e{axis}") and k.endswith("low")]
        for low, high in [p for p in pairs if p[0] in v]:
            v[low], v[high] = v[high], v[low]


def reference_graph2d(base, mods, exp):
    cls = base.ClassName()
    v = graph_values(base)
    for axis in [axis for mod, axes in REBIN.items() if mod in mods for axis in axes]:
        exp.unsupported(f"Rebin{axis}", cls)
    for mod, what in (("divide_width", "DivideBinWidth"), ("density", "DivideBinWidth"), ("smooth", "Smooth"), ("smooth_3", "Smooth"), ("cumulative", "Cumulative"), ("cumulative_backward", "Cumulative")):
        if mod in mods:
            exp.unsupported(what, cls)
    if any(m.startswith("normalize") for m in mods) or "density" in mods:
        exp.unsupported("Normalize", cls)
    for axis in "xy":
        if f"scale_{axis}" in mods:
            for key in (axis, f"e{axis}"):
                if key in v:
                    v[key] = [value * AXIS_FACTOR for value in v[key]]
    factor = None
    if "norm_max" in mods or "norm_max_range" in mods:
        inside = [z for x, z in zip(v["x"], v["z"]) if "norm_max_range" not in mods or RANGE[0] <= x <= RANGE[1]]
        if not v["z"]:
            exp.warn(f"Cannot normalize graph {exp.name} to maximum because it has no points")
        elif not inside:
            exp.warn(f"The range of NormalizeToMaximum for {exp.name} contains no points")
        elif max(inside) == 0:
            exp.warn(f"Cannot normalize graph {exp.name} to maximum because it is zero")
        else:
            factor = 1.0 / max(inside)
    for mod, f in (("scale", FACTOR), ("scale_z", AXIS_FACTOR)):
        if mod in mods:
            factor = (factor or 1.0) * f
    if factor:
        flip(v, "z", factor)
    return v


def reference_func(base, mods, exp):
    """functions: a callable giving the expected value at a point"""
    cls, dim = base.ClassName(), base.GetNdim()
    for axis in [axis for mod, axes in REBIN.items() if mod in mods for axis in axes]:
        exp.unsupported(f"Rebin{axis}", cls)
    for mod, what in (("divide_width", "DivideBinWidth"), ("density", "DivideBinWidth"), ("smooth", "Smooth"), ("smooth_3", "Smooth"), ("cumulative", "Cumulative"), ("cumulative_backward", "Cumulative")):
        if mod in mods:
            exp.unsupported(what, cls)
    if "norm_max" in mods or "norm_max_range" in mods:
        exp.unsupported("NormalizeToMaximum", cls)
    if any(m.startswith("normalize") for m in mods) or "density" in mods:
        exp.unsupported("Normalize", cls)
    domain = [AXIS_FACTOR if f"scale_{a}" in mods else 1.0 for a in "xyz"[:dim]]
    content = FACTOR if "scale" in mods else 1.0
    if f"scale_{'xyz'[dim]}" in mods if dim < 3 else False:
        content *= AXIS_FACTOR
    if dim == 1 and "scale_z" in mods:
        exp.unsupported("ScaleZ", cls)
    return lambda *c: content * base.Eval(*[x / f for x, f in zip(c, domain)])


GRAPHS = ("TGraph", "TGraphErrors", "TGraphAsymmErrors", "TGraphMultiErrors", "TGraphPolar", "TGraph2D", "TGraph2DErrors")
REFERENCES = {"hist": reference_hist, "profile": reference_hist, "poly": reference_hist, "graph": reference_graph, "graph2d": reference_graph2d, "func": reference_func}

p = Project("modifiers", bases=("1d", "2d"))
p.inputs(VARIANTS)
todo = cases(
    {"kind": list(DATA_KINDS), "mods": [(m,) for m in MODIFIERS] + list(COMBINATIONS), "variant": list(VARIANTS)},
    valid=lambda c: has_variant(c["kind"], c["variant"]),
    always=lambda c: c["variant"] == "normal",
    named=[
        {"kind": "graph_multi", "mods": ("scale",), "variant": "normal", "why": "f934095"},
        {"kind": "hist_var", "mods": ("rebin_x",), "variant": "normal", "why": "bins no multiple of the group"},
        {"kind": "hist2poly", "mods": ("normalize_range",), "variant": "normal", "why": "range of a TH2Poly"},
        {"kind": "hist2poly", "mods": ("norm_max_range",), "variant": "normal", "why": "range of a TH2Poly"},
        {"kind": "graph_asymm", "mods": ("normalize_range",), "variant": "negative", "why": "negative factor of a graph"},
        {"kind": "graph_multi", "mods": ("normalize_range",), "variant": "negative", "why": "negative factor of a graph"},
        {"kind": "graph_multi", "mods": ("smooth",), "variant": "unsorted", "why": "sorting a TGraphMultiErrors"},
        {"kind": "hist", "mods": ("norm_max", "cumulative"), "variant": "zeros", "why": "name after Cumulative"},
    ],
)
bases = {}  # (kind, variant): the plot of the data as drawn without modifiers


def add_plot(name, case, mods):
    p.pm.AddDataSource(name, p.path(f"{source_of(case['variant'])}.root"))
    plot = S.Plot(name, "modifiers", base_of(case["kind"]))
    data = add_data(plot[1], case["kind"], name, case["variant"])
    for mod in mods:
        MODIFIERS[mod](data)
    p.pm.AddPlot(plot)


for i, case in enumerate(todo):
    key = (case["kind"], case["variant"])
    if key not in bases:
        bases[key] = f"base{len(bases)}"
        add_plot(bases[key], case, ())
    add_plot(f"mod{i}", case, case["mods"])
run = p.plot("modifiers")

expectedWarnings = [(f" {kind} has", "NaN or infinite") for kind, variant in bases if variant in ("nan", "inf")]  # (data name, fragment)
expectedWarnings += [(f"Graph {kind} ", "has no points: it is not drawn") for kind, variant in bases if variant == "empty" and (DATA_KINDS[kind][1] in GRAPHS or kind == "efficiency")]
silent = set()  # data whose warnings are not compared
for i, case in enumerate(todo):
    what = describe(case)
    kind = case["kind"]
    formulaKind = "formula" in DATA_KINDS[kind][2]
    if case["variant"] in ("nan", "inf"):
        expectedWarnings.append((f" {kind} has", "NaN or infinite"))
    base, obs = run.drawn(bases[(kind, case["variant"])], "modifiers"), run.drawn(f"mod{i}", "modifiers")
    if not base and not obs and case["variant"] == "empty" and (DATA_KINDS[kind][1] in GRAPHS or kind == "efficiency"):
        expectedWarnings.append((f"Graph {kind} ", "has no points: it is not drawn"))
        silent.add(f"{kind}:mod{i}")  # what its modifiers report about no points is not compared
        continue  # an empty graph is not drawn (see frames/robust_frames.py), modified or not
    if not base or not obs:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    base, obs = base[0][0], obs[0][0]
    exp = Expected(data_name(kind, case["variant"]) if formulaKind else obs.GetName().split(":", 1)[1].removesuffix("_cumulative"))
    fam = family(base)
    ref = REFERENCES[fam](base, set(case["mods"]), exp)
    expectedWarnings += [(exp.name, w) for w in exp.warnings]
    if fam == "func":
        diff = func_diff(ref, obs, 1e-9)
    elif fam in ("graph", "graph2d"):
        diff = graph_diff(ref, obs, 1e-9)
    else:
        # float histograms (TEfficiency 2d) and profiles (sums of squares) are compared to float precision
        diff = hist_diff(ref, means(obs) if fam == "profile" else obs, 1e-6 if fam == "profile" or base.ClassName().endswith("F") else 1e-9)
        if fam == "profile" and not diff and set(case["mods"]) & {"smooth", "smooth_3"}:
            entries = [obs.GetBinEntries(c) for c in range(obs.GetNcells())] == [base.GetBinEntries(c) for c in range(base.GetNcells())]
            diff = "" if entries else "entries of the bins changed"
    check(f"as plain ROOT: {what}", not diff, diff)

unmatched = list(expectedWarnings)
unexpected = []
for line in run.warnings():
    match = next((e for e in unmatched if re.search(re.escape(e[0]) + "(?![0-9])", line) and e[1] in line), None)
    if match:
        unmatched.remove(match)
    elif not any(re.search(re.escape(name) + "(?![0-9])", line) for name in silent):
        unexpected.append(line)
check("every warning expected", not unexpected, " | ".join(unexpected[:5]) + (f" (+{len(unexpected) - 5})" if len(unexpected) > 5 else ""))
check("every expected warning given", not unmatched, " | ".join(f"{n}: {w}" for n, w in unmatched[:5]) + (f" (+{len(unmatched) - 5})" if len(unmatched) > 5 else ""))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
