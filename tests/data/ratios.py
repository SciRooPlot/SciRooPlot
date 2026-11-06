"""Every numerator kind divided by every denominator kind, in every variant, with uncorrelated and binomial (SetCorrelated) errors, gives
the ratio plain ROOT gives, or is reported with an error and loses only its plot. The numerator and denominator are the data as drawn
without the division (profiles as the histograms of their means, TEfficiency as the graph or 2d histogram ROOT makes of it, THn projected).
Histograms by histograms of the same binning: TH1::Divide (TH2Poly only by the same polygons). Otherwise the division is that of the points
(the bin centres of a 1d histogram, with its errors): a denominator at other positions is interpolated with a cubic spline (TSpline3) of its
values and of its error envelope; a zero denominator drops the point (empty bin); uncorrelated errors add in quadrature, binomial ones as
TH1::Divide with option B; a negative denominator swaps asymmetric errors; a numerator without errors takes the errors of the denominator.
By a function: the value at the bin centre or point (no binomial errors), bins and points outside its range left out (a formula of the plot
definition has none). A function as numerator: sampled at the positions of the denominator. The result of a histogram is a histogram, of a
graph or function a graph. Every warning is expected.
Covers: 82a875c (function numerator by a denominator with errors), e3de5c4 (graph points removed with their errors), 4986674 (profiles
divided as their means), 5d432dd (formulas as denominators divide the whole numerator), f934095 (TGraphMultiErrors divided); TH2Poly
divided without reading past its bins; NaN in a denominator removed before the division (a spline through it was NaN everywhere); two
formulas without a range reported once; a TGraphMultiErrors denominator sorted with its errors.
"""

import itertools
import math
import re

from srptest import DATA_KINDS, ROOT, Project, S, base_of, cases, check, close, data_name, data_objects, describe, finish, formula, graph_values, has_variant, hist_diff, source_of, values_diff

NUM_VARIANTS = ("normal", "negative", "nan", "empty", "single")
DEN_VARIANTS = ("normal", "negative", "zeros", "nan", "empty", "single", "unsorted")


def family(obj):
    if obj.InheritsFrom("TF1"):
        return f"func{obj.GetNdim()}"
    if obj.InheritsFrom("TGraph2D"):
        return "graph2d"
    if obj.InheritsFrom("TGraph"):
        return "graph"
    return f"hist{obj.GetDimension()}"


def means(h):
    """a profile as the histogram of its means; other histograms as they are"""
    if not any(h.InheritsFrom(c) for c in ("TProfile", "TProfile2D", "TProfile3D")):
        return h.Clone(f"{h.GetName()}_c")
    if h.InheritsFrom("TProfile"):
        m = h.ProjectionX(f"{h.GetName()}_m")
    elif h.InheritsFrom("TProfile2D"):
        m = h.ProjectionXY(f"{h.GetName()}_m")
    else:
        m = h.ProjectionXYZ(f"{h.GetName()}_m")
    m.SetDirectory(0)
    return m


class Points:
    """data as divided point by point: x, y and the y errors (kind none, sym or asym), per error dimension for a TGraphMultiErrors"""

    def __init__(self, obj):
        if obj.InheritsFrom("TH1"):
            n = obj.GetNbinsX()
            self.x = [obj.GetBinCenter(b) for b in range(1, n + 1)]
            self.y = [obj.GetBinContent(b) for b in range(1, n + 1)]
            self.kind, self.e = "sym", [obj.GetBinError(b) for b in range(1, n + 1)]
            self.multi = None
            return
        v = graph_values(obj)
        self.x, self.y = v["x"], v["y"]
        if "ErrorYlow" not in v:
            self.kind = "none"
        elif obj.GetEY():
            self.kind, self.e = "sym", v["ErrorYlow"]
        else:
            self.kind, self.lo, self.hi = "asym", v["ErrorYlow"], v["ErrorYhigh"]
        self.multi = [(v[f"ey{d}low"], v[f"ey{d}high"]) for d in range(obj.GetNYErrors())] if obj.InheritsFrom("TGraphMultiErrors") else None


def interpolate(num, den, exp):
    """the denominator at the numerator's positions: (values, errors as sym list or (low, high) lists or None), or None if impossible"""
    n = len(den.x)
    if n < 2:
        exp.error(f"Cannot interpolate {exp.den} for the division")
        return None
    order = sorted(range(n), key=lambda i: den.x[i])
    xs = [den.x[i] for i in order]
    if any(a == b for a, b in itertools.pairwise(xs)):
        exp.error(f"Cannot divide {exp.num} by graph {exp.den}: it has two points at x")
        return None

    def spline(values):
        knotsX, knotsY = ROOT.std.vector("double")(xs), ROOT.std.vector("double")(values)  # alive while the spline is made
        sp = ROOT.TSpline3("s", knotsX.data(), knotsY.data(), n)
        return [sp.Eval(x) for x in num.x]

    ys = [den.y[i] for i in order]
    values = spline(ys)
    errors = None
    if den.kind != "none":
        low = spline([ys[k] - (den.e[i] if den.kind == "sym" else den.lo[i]) for k, i in enumerate(order)])
        high = spline([ys[k] + (den.e[i] if den.kind == "sym" else den.hi[i]) for k, i in enumerate(order)])
        if num.kind == "sym":
            errors = [math.sqrt(0.5 * ((v - lo) ** 2 + (hi - v) ** 2)) for v, lo, hi in zip(values, low, high)]
        elif num.kind == "asym":
            errors = ([abs(v - lo) for v, lo in zip(values, low)], [abs(hi - v) for v, hi in zip(values, high)])
    lo, hi = exp.denRange if exp.denRange else (xs[0], xs[-1])
    if any(x < lo or x > hi for x in num.x):
        exp.warn(f"points of {exp.num} are outside the range")
    oneSign = all(y > 0 for y in ys) or all(y < 0 for y in ys)
    if oneSign and any((v <= 0) if ys[0] > 0 else (v >= 0) for v in values):
        exp.warn(f"The interpolation of {exp.den} for")
    return values, errors


def divide_points(num, den, binomial, exp):
    """the ratio of two Points objects as a dict like graph_values (with "bins": the index of each kept point), or None on an error"""
    same = len(num.x) <= len(den.x) and not ((num.kind == "sym" and den.kind == "asym") or (num.kind == "asym" and den.kind == "sym")) and all(a == b for a, b in zip(num.x, den.x))
    if same:
        values = den.y[: len(num.x)]
        if den.kind == "none" or num.kind == "none":
            errors = None
        elif num.kind == "sym":
            errors = den.e
        else:
            errors = (den.lo, den.hi)
    else:
        found = interpolate(num, den, exp)
        if found is None:
            return None
        values, errors = found
    out = {"x": [], "y": [], "lo": [], "hi": [], "index": [], "multi": [] if num.multi else None}
    for i, (x, y, d) in enumerate(zip(num.x, num.y, values)):
        if not d:
            continue
        same_sign = (y > 0) == (d > 0)

        def propagate(e, de, y=y, d=d):
            if binomial:
                return 0.0 if y == d else math.sqrt(abs(((1 - 2 * y / d) * e**2 + y**2 * de**2 / d**2) / d**2))
            return math.sqrt((e / d) ** 2 + (de * y / d**2) ** 2)

        if num.kind == "sym":
            lo = hi = propagate(num.e[i], errors[i] if errors else 0.0)
        elif num.kind == "asym":
            dLow, dHigh = (errors[0][i], errors[1][i]) if errors else (0.0, 0.0)
            lo = propagate(num.lo[i], dHigh if same_sign else dLow)
            hi = propagate(num.hi[i], dLow if same_sign else dHigh)
            if d < 0:
                lo, hi = hi, lo
        else:
            lo = hi = None
        out["x"].append(x)
        out["y"].append(y / d)
        out["lo"].append(lo)
        out["hi"].append(hi)
        out["index"].append(i)
        if num.multi:
            oldLow, oldHigh = num.lo[i], num.hi[i]
            if d < 0:
                oldLow, oldHigh = oldHigh, oldLow
            dims = [(dl[i], dh[i]) if d > 0 else (dh[i], dl[i]) for dl, dh in num.multi]
            out["multi"].append([(a * lo / oldLow if oldLow else (lo if k == 0 else 0.0), b * hi / oldHigh if oldHigh else (hi if k == 0 else 0.0)) for k, (a, b) in enumerate(dims)])
    if num.x and not out["x"]:
        exp.warn(f"{exp.den} is zero at every point of {exp.num}")
    return out


class Expected:
    def __init__(self, num, den, denRange):
        self.num, self.den = num, den  # the names in messages
        self.denRange = denRange  # where the denominator has values: beyond, it is extrapolated
        self.warnings, self.errors = [], []

    def warn(self, text):
        self.warnings.append(text)

    def error(self, text):
        self.errors.append(text)


def subset(points, keep):
    """the points with the given indices"""
    for key in ("x", "y", "e", "lo", "hi", "exlow", "exhigh"):
        if getattr(points, key, None) is not None:
            setattr(points, key, [getattr(points, key)[i] for i in keep])
    if points.multi:
        points.multi = [([lo[i] for i in keep], [hi[i] for i in keep]) for lo, hi in points.multi]
    return points


def with_errors_of(num, den):
    """a numerator graph without errors takes the kind of errors of the denominator (zero errors of its own)"""
    if num.kind == "none" and den.kind != "none":
        num.kind = den.kind
        if den.kind == "sym":
            num.e = [0.0] * len(num.x)
        else:
            num.lo, num.hi = [0.0] * len(num.x), [0.0] * len(num.x)
    return num


def x_errors(obj, points):
    """the x errors a Points object keeps (for the result and the extrapolation check)"""
    if obj.InheritsFrom("TH1"):
        points.exlow = points.exhigh = [obj.GetBinWidth(b) / 2 for b in range(1, obj.GetNbinsX() + 1)]
    elif obj.InheritsFrom("TGraph") and (obj.GetEX() or obj.GetEXlow()):
        points.exlow = [obj.GetErrorXlow(i) for i in range(obj.GetN())]
        points.exhigh = [obj.GetErrorXhigh(i) for i in range(obj.GetN())]
    else:
        points.exlow = points.exhigh = None
    return points


def reference(numObj, denObj, numFunc, denFunc, binomial, exp):
    """the expected ratio: a histogram, a dict like graph_values, or None (an error is expected)"""
    nf, df = family(numObj), family(denObj)
    if nf == "graph2d" or nf in ("func2", "func3"):
        exp.error(f"Unsupported division of {means(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}")
        return None
    if nf == "func1":
        if df == "graph" or df == "hist1":
            den = x_errors(denObj, Points(means(denObj) if df == "hist1" else denObj))
            keep = [i for i, x in enumerate(den.x) if numFunc.inside(x)]  # a function of the inputs within its range
            if not keep:  # nothing to sample at: the function itself cannot be divided
                exp.error("Unsupported division of TF1")
                return None
            den = subset(den, keep)
            num = Points(denObj if df == "graph" else means(denObj))
            num.x = list(den.x)
            num.y = [numFunc(x) for x in den.x]
            num.kind, num.multi = "none", None
            num.exlow = num.exhigh = None
            num = with_errors_of(num, den)
            return result_graph(divide_points(num, den, binomial, exp), num, None)
        if df == "func1":  # sampled at the numerator's points where both are defined
            if numFunc.formula and denFunc.formula:
                exp.error("two formulas of the plot definition have no range to be divided in")
                return None
            ranges = [(f.obj.GetXmin(), f.obj.GetXmax()) for f in (numFunc, denFunc) if not f.formula]
            lo, hi = max(r[0] for r in ranges), min(r[1] for r in ranges)
            npx = max(numObj.GetNpx(), 2)
            xs = [lo + (hi - lo) * i / (npx - 1) for i in range(npx)]
            graph = ROOT.TGraph(npx)
            for i, x in enumerate(xs):
                graph.SetPoint(i, x, numFunc(x))
            return divide_by_function_graph(graph, denFunc, binomial, exp)
        exp.error("Unsupported division of TF1")
        return None
    if nf.startswith("hist"):
        dim = int(nf[4:])
        num = means(numObj)
        if df.startswith("func"):
            if int(df[4:]) > dim:
                exp.error(f"Cannot divide {exp.num} by the higher dimensional function")
                return None
            return divide_by_function_hist(num, denFunc, int(df[4:]), binomial, exp)
        if df == "graph":
            if dim != 1:
                exp.error(f"Unsupported division of {means(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}")
                return None
            return graph_to_hist(num, divide_points(x_errors(num, Points(num)), x_errors(denObj, Points(denObj)), binomial, exp))
        if df == "graph2d":
            exp.error(f"Unsupported division of {means(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}")
            return None
        den = means(denObj)
        if all(den.GetBinContent(c) == 0 for c in range(den.GetNcells())):
            exp.warn(f"{exp.den} is empty: the ratio {exp.num} has no values")
        if den.GetDimension() != dim:
            exp.error(f"Cannot divide {exp.num} ({dim}d) by {exp.den}")
            return None
        if numObj.InheritsFrom("TH2Poly") or denObj.InheritsFrom("TH2Poly"):
            if not (numObj.InheritsFrom("TH2Poly") and denObj.InheritsFrom("TH2Poly")):
                exp.error(f"Cannot divide {exp.num} by {exp.den}: a TH2Poly")
                return None
        elif not same_binning(num, den):
            if dim > 1:
                exp.error(f"Cannot divide {exp.num} by {exp.den}: their binnings differ")
                return None
            return graph_to_hist(num, divide_points(x_errors(num, Points(num)), x_errors(den, Points(den)), binomial, exp))
        num.Divide(num, den, 1.0, 1.0, "B" if binomial else "")
        if not num.InheritsFrom("TH2Poly"):
            num.ClearUnderflowAndOverflow()
        return num
    # a graph numerator
    if df == "graph" or df == "hist1":
        den = x_errors(denObj, Points(means(denObj) if df == "hist1" else denObj))
        num = with_errors_of(x_errors(numObj, Points(numObj)), den)
        return result_graph(divide_points(num, den, binomial, exp), num, numObj)
    if df == "func1":
        return divide_by_function_graph(numObj, denFunc, binomial, exp)
    exp.error(f"Unsupported division of {means(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}")
    return None


def same_binning(a, b):
    for axis in "XYZ"[: a.GetDimension()]:
        x, y = getattr(a, f"Get{axis}axis")(), getattr(b, f"Get{axis}axis")()
        if x.GetNbins() != y.GetNbins() or any(not close(x.GetBinLowEdge(i), y.GetBinLowEdge(i), 1e-10) for i in range(1, x.GetNbins() + 2)):
            return False
    return True


def graph_to_hist(num, ratio):
    """the ratio of a 1d histogram: the bins of the kept points, the others empty"""
    if ratio is None:
        return None
    h = num.Clone("reference")
    h.Reset()
    for i, y, e in zip(ratio["index"], ratio["y"], ratio["lo"]):
        h.SetBinContent(i + 1, y)
        h.SetBinError(i + 1, e)
    return h


def result_graph(ratio, num, numObj):
    """the ratio of a graph as a dict like graph_values"""
    if ratio is None:
        return None
    v = {"x": ratio["x"], "y": ratio["y"]}
    if num.kind != "none":
        v["ErrorYlow"], v["ErrorYhigh"] = ratio["lo"], ratio["hi"]
        v["ErrorXlow"] = [num.exlow[i] if num.exlow else 0.0 for i in ratio["index"]]
        v["ErrorXhigh"] = [num.exhigh[i] if num.exhigh else 0.0 for i in ratio["index"]]
    if ratio["multi"] is not None:
        for d in range(len(num.multi)):
            v[f"ey{d}low"] = [m[d][0] for m in ratio["multi"]]
            v[f"ey{d}high"] = [m[d][1] for m in ratio["multi"]]
    return v


def divide_by_function_hist(num, func, funcDim, binomial, exp):
    """a histogram by a function: the value at the bin centre (a TH2Poly: the centre of the bounding box), bins outside its range empty"""
    outside = 0
    if num.InheritsFrom("TH2Poly"):
        for b in num.GetBins():
            c = b.GetBinNumber()
            x, y = 0.5 * (b.GetXMin() + b.GetXMax()), 0.5 * (b.GetYMin() + b.GetYMax())
            d = func.value(*[x, y][:funcDim])
            outside += d is None and num.GetBinContent(c) != 0
            num.SetBinError(c, num.GetBinError(c) / abs(d) if d else 0.0)
            num.SetBinContent(c, num.GetBinContent(c) / d if d else 0.0)
    else:
        if not num.GetSumw2N():
            num.Sumw2()
        bx, by, bz = (ROOT.std.vector("int")(1) for _ in range(3))
        for c in range(num.GetNcells()):
            if num.IsBinUnderflow(c) or num.IsBinOverflow(c):
                num.SetBinContent(c, 0.0)
                num.SetBinError(c, 0.0)
                continue
            num.GetBinXYZ(c, bx.data(), by.data(), bz.data())
            xyz = [num.GetXaxis().GetBinCenter(bx[0]), num.GetYaxis().GetBinCenter(by[0]), num.GetZaxis().GetBinCenter(bz[0])]
            d = func.value(*xyz[:funcDim])
            outside += d is None and num.GetBinContent(c) != 0
            num.SetBinError(c, num.GetBinError(c) / abs(d) if d else 0.0)
            num.SetBinContent(c, num.GetBinContent(c) / d if d else 0.0)
    if outside:
        exp.warn(f"{outside} bins of {exp.num} lie outside the range of function {exp.den}")
    if binomial:
        exp.warn(f"Binomial errors are not supported for the division of {exp.num} by function {exp.den}")
    return num


def divide_by_function_graph(numObj, func, binomial, exp):
    """a graph by a function: points outside its range and where it is zero left out"""
    v = graph_values(numObj)
    keep = [i for i, x in enumerate(v["x"]) if func.inside(x)]
    if len(keep) < len(v["x"]):
        exp.warn(f"{len(v['x']) - len(keep)} points of {exp.num} lie outside the range of function {exp.den}")
    keep = [i for i in keep if func.value(v["x"][i])]
    out = {k: [values[i] for i in keep] for k, values in v.items()}
    for n, i in enumerate(keep):
        d = func.value(v["x"][i])
        out["y"][n] = v["y"][i] / d
        swap = d < 0 and "ErrorYlow" in v and not numObj.GetEY()
        for k in [k for k in v if k.startswith(("ErrorY", "ey"))]:
            out[k][n] = v[k][i] / abs(d)
        if swap:
            for low, high in [("ErrorYlow", "ErrorYhigh")] + [(f"ey{d}low", f"ey{d}high") for d in range(numObj.GetNYErrors() if numObj.InheritsFrom("TGraphMultiErrors") else 0)]:
                out[low][n], out[high][n] = out[high][n], out[low][n]
    if binomial:
        exp.warn(f"Binomial errors are not supported for the division of {exp.num} by function {exp.den}")
    return out


def input_range(obj):
    """the x range an input covers as denominator: a histogram its axis, a graph its finite points with their x errors (as read, before
    drawing removes x errors)"""
    if obj.InheritsFrom("TEfficiency"):
        obj = obj.CreateGraph()
    if not obj:
        return None
    if obj.InheritsFrom("THnBase"):
        return (obj.GetAxis(0).GetXmin(), obj.GetAxis(0).GetXmax())
    if obj.InheritsFrom("TH1"):
        return (obj.GetXaxis().GetXmin(), obj.GetXaxis().GetXmax())
    if not obj.InheritsFrom("TGraph") or obj.InheritsFrom("TGraph2D"):
        return None
    points = sorted((obj.GetPointX(i), max(0.0, obj.GetErrorXlow(i)), max(0.0, obj.GetErrorXhigh(i))) for i in range(obj.GetN()) if all(math.isfinite(v) for v in (obj.GetPointX(i), obj.GetPointY(i), obj.GetErrorYlow(i), obj.GetErrorYhigh(i))))
    return (points[0][0] - points[0][1], points[-1][0] + points[-1][2]) if points else None


class Func:
    """a function as divisor or numerator: its value and whether a point is inside its range (a formula of the plot definition has none)"""

    def __init__(self, kind, variant, obj):
        self.formula = "formula" in DATA_KINDS[kind][2]
        self.obj = obj if not self.formula else {1: ROOT.TF1, 2: ROOT.TF2}[DATA_KINDS[kind][0]](f"f_{kind}_{variant}", formula(kind, variant))

    def inside(self, *x):
        if self.formula:
            return True
        lows = [getattr(self.obj, f"Get{a}min")() for a in "XYZ"[: len(x)]]
        highs = [getattr(self.obj, f"Get{a}max")() for a in "XYZ"[: len(x)]]
        return all(lo <= c <= hi for c, lo, hi in zip(x, lows, highs))

    def value(self, *x):
        return self.obj.Eval(*x) if self.inside(*x) else None

    def __call__(self, x):
        return self.obj.Eval(x)


p = Project("ratios")
p.inputs(set(NUM_VARIANTS) | set(DEN_VARIANTS))
inputs = {variant: data_objects(variant) for variant in set(NUM_VARIANTS) | set(DEN_VARIANTS)}
KINDS = list(DATA_KINDS)
todo = cases(
    {"num": KINDS, "den": KINDS, "num_variant": list(NUM_VARIANTS), "den_variant": list(DEN_VARIANTS), "correlated": [False, True]},
    valid=lambda c: has_variant(c["num"], c["num_variant"]) and has_variant(c["den"], c["den_variant"]) and "normal" in (c["num_variant"], c["den_variant"]),
    always=lambda c: c["num_variant"] == c["den_variant"] == "normal" and not c["correlated"],
    named=[
        {"num": "function", "den": "graph_errors", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "82a875c"},
        {"num": "graph_multi", "den": "graph", "num_variant": "nan", "den_variant": "normal", "correlated": False, "why": "e3de5c4"},
        {"num": "graph_multi", "den": "graph_errors", "num_variant": "normal", "den_variant": "negative", "correlated": False, "why": "f934095"},
        {"num": "profile", "den": "hist", "num_variant": "normal", "den_variant": "normal", "correlated": True, "why": "4986674"},
        {"num": "hist2d", "den": "function2d", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "5d432dd"},
        {"num": "hist2poly", "den": "function2d", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "TH2Poly statistics"},
        {"num": "hist2poly", "den": "hist2poly", "num_variant": "normal", "den_variant": "normal", "correlated": True, "why": "TH2Poly statistics"},
        {"num": "hist", "den": "graph_errors", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "NaN denominator"},
        {"num": "graph_errors", "den": "hist_var", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "NaN denominator"},
        {"num": "function", "den": "function", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "one error for two formulas"},
        {"num": "graph_multi", "den": "graph_errors", "num_variant": "normal", "den_variant": "unsorted", "correlated": False, "why": "sorting a TGraphMultiErrors"},
        {"num": "hist", "den": "graph_multi", "num_variant": "normal", "den_variant": "unsorted", "correlated": False, "why": "sorting a TGraphMultiErrors"},
    ],
)


def data(kind, source, variant):
    if "formula" in DATA_KINDS[kind][2]:
        return S.Data(formula(kind, variant), S.kUserFunctions)
    d = S.Data(kind, source)
    if "projected" in DATA_KINDS[kind][2]:
        d.Project([0] if DATA_KINDS[kind][0] == 1 else [0, 1])
    return d


bases = {}  # (kind, variant): the plot of the data as drawn alone
for i, case in enumerate(todo):
    for kind, variant in ((case["num"], case["num_variant"]), (case["den"], case["den_variant"])):
        if (kind, variant) not in bases:
            bases[(kind, variant)] = name = f"base{len(bases)}"
            p.pm.AddDataSource(name, p.path(f"{source_of(variant)}.root"))
            plot = S.Plot(name, "ratios", base_of(kind))
            plot[1].AddData(data(kind, name, variant))
            p.pm.AddPlot(plot)
    for side, key in (("n", "num_variant"), ("d", "den_variant")):
        p.pm.AddDataSource(f"{side}{i}", p.path(f"{source_of(case[key])}.root"))
    plot = S.Plot(f"r{i}", "ratios", base_of(case["num"]))
    ratio = plot[1].AddRatio(data(case["num"], f"n{i}", case["num_variant"]), data(case["den"], f"d{i}", case["den_variant"]))
    if case["correlated"]:
        ratio.SetCorrelated()
    p.pm.AddPlot(plot)
run = p.plot("ratios")


def stand_in(kind, variant):
    """an empty graph is not drawn alone (see frames/robust_frames.py): the input instead (no points either)"""
    dim, cls, _ = DATA_KINDS[kind]
    if variant != "empty" or not (cls.startswith("TGraph") or (cls == "TEfficiency" and dim == 1)):
        return None
    return ROOT.TGraphAsymmErrors() if cls == "TEfficiency" else inputs[variant][kind]


def message_name(kind, variant, source):
    """the name of data in messages about the division"""
    if "formula" in DATA_KINDS[kind][2]:
        return formula(kind, variant)
    return f"{kind}:{source}" + ("_Proj{0}" if DATA_KINDS[kind][0] == 1 else "_Proj{01}") * ("projected" in DATA_KINDS[kind][2])


expectedWarnings, expectedErrors = [], []
for kind, variant in bases:
    if variant in ("nan", "inf"):
        expectedWarnings.append((f" {kind} has", "NaN or infinite"))
    if stand_in(kind, variant) is not None:
        expectedWarnings.append((f"Graph {kind} ", "has no points: it is not drawn"))
for i, case in enumerate(todo):
    what = describe(case)
    objs = []
    for k in ("num", "den"):
        drawnAlone = run.drawn(bases[(case[k], case[f"{k}_variant"])], "ratios")
        objs.append(drawnAlone[0][0] if drawnAlone else stand_in(case[k], case[f"{k}_variant"]))
    numObj, denObj = objs
    if not numObj or not denObj:
        check(f"data drawn alone: {what}", False, " | ".join(run.errors()[:2]))
        continue
    denInput = inputs[case["den_variant"]].get(case["den"])
    exp = Expected(message_name(case["num"], case["num_variant"], f"n{i}"), message_name(case["den"], case["den_variant"], f"d{i}"), input_range(denInput) if denInput else None)
    caseWarnings = []
    for k in ("num", "den"):  # each input reported by its name before the division
        if case[f"{k}_variant"] in ("nan", "inf") and not DATA_KINDS[case[k]][1].startswith("TF"):
            caseWarnings.append((f" {case[k]} has", "NaN or infinite"))
    profiles = [k for k in ("num", "den") if any(base.InheritsFrom(c) for base in ([numObj] if k == "num" else [denObj]) for c in ("TProfile", "TProfile2D", "TProfile3D"))]
    binomial = case["correlated"]
    if binomial and profiles:
        binomial = False
        caseWarnings.append((f"Ratio {data_name(case['num'], case['num_variant'])}:", "binomial errors (SetCorrelated) are not defined for the mean values of a profile"))
    funcs = {k: Func(case[k], case[f"{k}_variant"], numObj if k == "num" else denObj) for k in ("num", "den") if DATA_KINDS[case[k]][1].startswith("TF")}
    ref = reference(numObj, denObj, funcs.get("num"), funcs.get("den"), binomial, exp)
    caseWarnings += [(exp.num, w) for w in exp.warnings]
    if ref is not None:  # a failing ratio, the first data of its pad, stops the plot before the pass that reports warnings
        expectedWarnings += caseWarnings
    drawn = run.drawn(f"r{i}", "ratios")
    if ref is None:
        expectedErrors += exp.errors + [f"Plot r{i} from group ratios could not be created"]
        check(f"reported as not possible: {what}", not drawn, "the ratio was drawn")
        continue
    if isinstance(ref, dict) and not ref["x"]:  # no points left: a graph without points is not drawn
        expectedWarnings.append((f"Graph {data_name(case['num'], case['num_variant'])} ", "has no points: it is not drawn"))
        check(f"a ratio without points not drawn: {what}", not drawn, "drawn")
        continue
    if not drawn:
        check(f"ratio made: {what}", False, " | ".join(line for line in run.errors() if f":n{i}" in line or f"r{i} " in line)[:300])
        continue
    obs = drawn[0][0]
    rel = 1e-6 if numObj.ClassName().endswith("F") or denObj.ClassName().endswith("F") or "_cpp" in case["num"] + case["den"] else 1e-9
    if isinstance(ref, dict):
        ref = {k: v for k, v in ref.items() if not k.startswith("ErrorX")}  # the x errors are the numerator's (drawn or not by the alias)
        diff = values_diff(ref, graph_values(obs), rel) if obs.InheritsFrom("TGraph") else f"drawn as {obs.ClassName()}, expected a graph"
    else:
        diff = hist_diff(ref, obs, rel)
    check(f"as plain ROOT: {what}", not diff, diff)


def match(lines, expected, keyed):
    """the expected messages not given, the messages not expected"""

    def fits(e, line):
        return (re.search(re.escape(e[0]) + "(?![0-9])", line) and e[1] in line) if keyed else e in line

    unmatched, unexpected = list(expected), []
    for line in lines:
        hit = next((e for e in unmatched if fits(e, line)), None)
        if hit is None:
            unexpected.append(line)
        else:
            unmatched.remove(hit)
    return unmatched, unexpected


def more(items):
    return " | ".join(str(x) for x in items[:5]) + (f" (+{len(items) - 5})" if len(items) > 5 else "")


unmatched, unexpected = match(run.warnings(), expectedWarnings, True)
check("every warning expected", not unexpected, more(unexpected))
check("every expected warning given", not unmatched, more(unmatched))
unmatched, unexpected = match(run.errors(), expectedErrors, False)
check("every error expected", not unexpected, more(unexpected))
check("every expected error given", not unmatched, more(unmatched))
finish()
