"""Shared building blocks of the SciRooPlot tests.

A test file defines its plots through the Python interface in one isolated project, runs the plot program once, reads the drawn
objects back from Plots.root and checks them against an independent reference (plain ROOT, an invariant, the requested values).
It exits 0 if every check passes, 1 if one fails and 77 if it cannot run (shown as skipped by CTest).

Environment (set by CTest): SRP_PREFIX, a build or install directory with bin/plot; SRP_TESTS_WORK, the scratch folder of the test;
SRP_TESTS_FULL=1 for the full case lists (weekly), otherwise a subset holding every value of every dimension at least once.
"""

import glob
import itertools
import math
import os
import re
import shutil
import subprocess
import sys
import warnings

import numpy as np

PREFIX = os.environ.get("SRP_PREFIX", "")
if not os.path.isfile(os.path.join(PREFIX, "bin", "plot")):
    sys.exit("SRP_PREFIX must name a build or install directory of SciRooPlot: run the tests with ctest")
WORK = os.path.abspath(os.environ.get("SRP_TESTS_WORK", "work"))
FULL = os.environ.get("SRP_TESTS_FULL") == "1"
shutil.rmtree(WORK, ignore_errors=True)
os.makedirs(WORK)
os.environ["SCIROOPLOT_CONFIG_PATH"] = os.path.join(WORK, "config")  # read once per process: set before SciRooPlot is used

# the module is in lib/ of a build directory and in lib/pythonX.Y/site-packages of an installation
for path in [os.path.join(PREFIX, "lib"), *glob.glob(os.path.join(PREFIX, "lib", "python*", "site-packages"))]:
    if glob.glob(os.path.join(path, "SciRooPlot*.so")):
        sys.path.insert(0, path)
warnings.filterwarnings("ignore")
import ROOT  # noqa: E402

ROOT.gROOT.SetBatch(True)
ROOT.TH1.AddDirectory(False)
import SciRooPlot as S  # noqa: E402

# ---------------------------------------------------------------------------------------------
# bookkeeping
# ---------------------------------------------------------------------------------------------
_failed = []
_passed = 0


def check(name, ok, detail=""):
    """one check; the name states the guarantee, the detail what was found instead"""
    global _passed
    if ok:
        _passed += 1
    else:
        _failed.append(f"{name}: {detail}" if detail else name)
    print(f"[{'PASS' if ok else 'FAIL'}] {name}" + (f": {detail}" if detail and not ok else ""))


def skip(reason):
    print(f"SKIPPED: {reason}")
    sys.exit(77)


def finish():
    print(f"\n==== {_passed} passed, {len(_failed)} failed ====")
    for line in _failed:
        print(f"  FAIL {line}")
    sys.exit(1 if _failed else 0)


def close(a, b, rel=1e-9):
    return abs(a - b) <= rel * max(1.0, abs(a), abs(b))


# ---------------------------------------------------------------------------------------------
# case lists
# ---------------------------------------------------------------------------------------------
def cases(dims, valid=lambda case: True, named=(), always=lambda case: False):
    """The combinations of the values in dims (a dict of lists) as dicts, without those that are not valid.
    The fast run takes those needed to have every value of every dimension at least once, and those always is true for; the full run
    all of them. Named cases (dicts with a "why": a bug that happened) are always included."""
    allCases = [dict(zip(dims, values)) for values in itertools.product(*dims.values())]
    allCases = [case for case in allCases if valid(case)]
    chosen = allCases
    if not FULL:
        chosen, seen = [], set()
        for case in allCases:
            pairs = set(case.items())
            if not pairs <= seen or always(case):
                chosen.append(case)
                seen |= pairs
    for extra in named:
        match = next((case for case in chosen if all(case.get(k) == v for k, v in extra.items() if k != "why")), None)
        if match is None:
            chosen.append(dict(extra))
        else:
            match["why"] = extra.get("why", "")
    return chosen


def describe(case):
    return " ".join(f"{k}={v}" for k, v in case.items() if k != "why") + (f" [{case['why']}]" if case.get("why") else "")


# ---------------------------------------------------------------------------------------------
# data of every kind
# ---------------------------------------------------------------------------------------------
# Every kind of data SciRooPlot draws, by kind: (dimension of the drawn data, class, properties). The data covers 1 to 10 on every axis
# with positive values (usable on log axes): about 1 to 10 for 1d data, products of those for 2d and 3d data, about 0.1 to 0.8 for
# efficiencies. Properties: binned (a histogram: the frame shows whole bins), polar (drawn in a polar grid, no cartesian frame), formula
# (a function of the plot definition, no input object), cpp (a function of compiled C++ code: stored as values sampled over its range),
# projected (an n-dimensional histogram, drawn as its projection onto the first axis, or the first two), polygons (bins that are
# polygons: the axes have no bins, a frame ends where it is asked to as for graphs).
# hist_noerrors is a histogram whose errors are all zero, hist_counts one filled without weights (no Sumw2: its errors are the square
# roots of the contents).
# A test takes its kinds from these tables, so a kind added here is covered everywhere.
DATA_KINDS = {
    "hist": (1, "TH1D", {"binned"}),
    "hist_var": (1, "TH1D", {"binned"}),
    "hist_noerrors": (1, "TH1D", {"binned"}),
    "hist_counts": (1, "TH1D", {"binned"}),
    "profile": (1, "TProfile", {"binned"}),
    "efficiency": (1, "TEfficiency", {"binned"}),
    "thn": (1, "THnD", {"binned", "projected"}),
    "thn_sparse": (1, "THnSparseD", {"binned", "projected"}),
    "graph": (1, "TGraph", set()),
    "graph_errors": (1, "TGraphErrors", set()),
    "graph_asymm": (1, "TGraphAsymmErrors", set()),
    "graph_multi": (1, "TGraphMultiErrors", set()),
    "graph_polar": (1, "TGraphPolar", {"polar"}),
    "tf1": (1, "TF1", set()),
    "tf1_cpp": (1, "TF1", {"cpp"}),
    "function": (1, "TF1", {"formula"}),
    "hist2d": (2, "TH2D", {"binned"}),
    "profile2d": (2, "TProfile2D", {"binned"}),
    "hist2poly": (2, "TH2Poly", {"binned", "polygons"}),
    "efficiency2d": (2, "TEfficiency", {"binned"}),
    "thn_sparse2d": (2, "THnSparseD", {"binned", "projected"}),
    "graph2d": (2, "TGraph2D", set()),
    "graph2d_errors": (2, "TGraph2DErrors", set()),
    "tf2": (2, "TF2", set()),
    "tf2_cpp": (2, "TF2", {"cpp"}),
    "function2d": (2, "TF2", {"formula"}),
    "hist3d": (3, "TH3D", {"binned"}),
    "profile3d": (3, "TProfile3D", {"binned"}),
    "tf3": (3, "TF3", set()),
    "tf3_cpp": (3, "TF3", {"cpp"}),
}
DATA_1D = tuple(k for k, (dim, _, props) in DATA_KINDS.items() if dim == 1 and "polar" not in props)  # cartesian 1d frames
DATA_POLAR = tuple(k for k, (_, _, props) in DATA_KINDS.items() if "polar" in props)
DATA_2D = tuple(k for k, (dim, _, _) in DATA_KINDS.items() if dim == 2)
DATA_3D = tuple(k for k, (dim, _, _) in DATA_KINDS.items() if dim == 3)
HISTS = tuple(k for k, (_, _, props) in DATA_KINDS.items() if "binned" in props)
BINNED_AXES = tuple(k for k in HISTS if "polygons" not in DATA_KINDS[k][2])  # frames ending at bin edges
FUNCTIONS = tuple(k for k, (_, cls, _) in DATA_KINDS.items() if cls.startswith("TF"))
FORMULAS = {"function": "1 + (x - 1) * (1 - 0.3 * ((x - 5.5) / 4.5)^2)"}

# Variants of the data of every kind, for robustness: values crossing zero (negative), one NaN or infinite value (nan, inf), all values
# zero (zeros), no entries or points at all (empty), a single filled bin or point (single), values near the largest float (huge),
# graph points in shuffled order (unsorted), histograms with large underflow and overflow contents (overflow), functions sampled with
# few or many points (coarse, fine). A TEfficiency counts events: it has no negative, NaN, infinite, huge or overflow variant.
VARIANTS = ("normal", "negative", "nan", "inf", "zeros", "empty", "single", "huge", "unsorted", "overflow", "coarse", "fine")
COUNTS_VARIANTS = ("normal", "zeros", "empty", "single")
FUNCTION_VARIANTS = ("normal", "negative", "zeros", "coarse", "fine")
NPX = {"normal": 100, "coarse": 5, "fine": 10000}  # points of a function per axis, at most NPX_MAX in 2d and 3d (memory of the stored samples)
NPX_MAX = {1: 10000, 2: 200, 3: 20}


def has_variant(kind, variant):
    _, cls, props = DATA_KINDS[kind]
    if cls == "TEfficiency":
        return variant in COUNTS_VARIANTS
    if cls.startswith("TF"):
        return variant in FUNCTION_VARIANTS and not ("formula" in props and variant in NPX and variant != "normal")
    if variant in ("coarse", "fine"):
        return False
    if variant == "unsorted":
        return "binned" not in props
    if variant == "overflow":
        return "binned" in props
    return True


def _value(x):
    return 1.0 + (x - 1.0) * (1.0 - 0.3 * ((x - 5.5) / 4.5) ** 2)


def formula(kind, variant="normal"):
    """the formula of a function kind in a variant"""
    dim = DATA_KINDS[kind][0]
    base = {1: FORMULAS["function"], 2: "x * y", 3: "x * y * z"}[dim]
    return {"negative": f"({base}) - {5**dim}", "zeros": f"0 * {' * '.join('xyz'[:dim])}"}.get(variant, base)


def _declare():
    """C++ makers for what PyROOT cannot build: TH2Poly bins (AddBin with numbers fails in a pythonization) and compiled functions"""
    if hasattr(ROOT, "srptest_poly"):
        return
    code = """
    TH2Poly* srptest_poly() {
      auto* h = new TH2Poly("hist2poly", "", 1., 10., 1., 10.);
      for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j) h->AddBin(1. + i, 1. + j, 2. + i, 2. + j);
      return h;
    }
    double srptest_value(double x) { return 1. + (x - 1.) * (1. - 0.3 * std::pow((x - 5.5) / 4.5, 2)); }
    """
    for variant, shift, scale in (("normal", 0, 1), ("negative", 1, 1), ("zeros", 0, 0)):
        code += f"""
    double srptest_f1_{variant}(double* x, double*) {{ return {scale} * (srptest_value(x[0]) - {shift} * 5.); }}
    double srptest_f2_{variant}(double* x, double*) {{ return {scale} * (x[0] * x[1] - {shift} * 25.); }}
    double srptest_f3_{variant}(double* x, double*) {{ return {scale} * (x[0] * x[1] * x[2] - {shift} * 125.); }}"""
    ROOT.gInterpreter.Declare(code)


def sized(cls, n):
    """a graph of n points: cls(0) may be taken as the constructor from a null histogram (cppyy picks the overload)"""
    graph = cls()
    graph.Set(n)
    return graph


def data_objects(variant="normal"):
    """one object of every data kind that has the variant (see VARIANTS), by kind; the formula kinds need none"""
    _declare()
    special = 3  # the item of every object that gets the NaN or infinite value, or stays filled in the single variant

    def value(index, *coords):
        """the value of item index (in fill order) at coords"""
        v = 1e35 if variant == "huge" else 1.0  # ROOT finds minima and maxima of histograms only below the largest float (3.4e38)
        for c in coords:
            v *= _value(c)
        if variant == "negative":
            v -= 5.0 ** len(coords)
        elif variant == "zeros":
            v = 0.0
        elif variant in ("nan", "inf") and index == special:
            v = float(variant)
        return v

    def keep(index):
        """whether item index is filled"""
        return variant != "empty" and (variant != "single" or index == special)

    def order(items):
        """the order the points of a graph are set in"""
        return sorted(items, key=lambda item: (7 * item[0]) % len(items)) if variant == "unsorted" else items

    outside = 1e6 if variant == "overflow" else 0.0  # content of the underflow and overflow bins

    h = ROOT.TH1D("hist", "", 18, 1.0, 10.0)
    hn = ROOT.TH1D("hist_noerrors", "", 18, 1.0, 10.0)
    hc = ROOT.TH1D("hist_counts", "", 18, 1.0, 10.0)
    edges = ROOT.std.vector("double")([1.0, 1.2, 1.5, 2.0, 2.7, 3.5, 4.5, 6.0, 7.5, 9.0, 10.0])
    hv = ROOT.TH1D("hist_var", "", edges.size() - 1, edges.data())
    p = ROOT.TProfile("profile", "", 18, 1.0, 10.0)
    for hist in (h, hv):
        for b in range(1, hist.GetNbinsX() + 1):
            if keep(b - 1):
                hist.SetBinContent(b, value(b - 1, hist.GetBinCenter(b)))
                hist.SetBinError(b, abs(0.1 * value(b - 1, hist.GetBinCenter(b))))
        hist.SetBinContent(0, outside)
        hist.SetBinContent(hist.GetNbinsX() + 1, outside)
        hist.SetEntries(sum(keep(b) for b in range(hist.GetNbinsX())))
    hn.Sumw2()
    for hist in (hn, hc):
        for b in range(1, hist.GetNbinsX() + 1):
            if keep(b - 1):
                hist.SetBinContent(b, value(b - 1, hist.GetBinCenter(b)))
        hist.SetBinContent(0, outside)
        hist.SetBinContent(hist.GetNbinsX() + 1, outside)
    for b in range(1, p.GetNbinsX() + 1):
        if keep(b - 1):
            x = p.GetBinCenter(b)
            p.Fill(x, 0.9 * value(b - 1, x))
            p.Fill(x, 1.1 * value(b - 1, x))
    if outside:
        p.Fill(0.0, outside)
        p.Fill(11.0, outside)
    nBins = np.array([18, 3, 3], dtype=np.int32)
    low, high = np.array([1.0, 0.0, 0.0]), np.array([10.0, 3.0, 3.0])
    thn = ROOT.THnD("thn", "", 2, nBins[:2], low[:2], high[:2])
    sparse = ROOT.THnSparseD("thn_sparse", "", 2, nBins[:2], low[:2], high[:2])
    sparse2 = ROOT.THnSparseD("thn_sparse2d", "", 3, np.array([18, 18, 3], dtype=np.int32), np.array([1.0, 1.0, 0.0]), np.array([10.0, 10.0, 3.0]))
    for b in range(18):
        x = 1.25 + 0.5 * b
        for nd in (thn, sparse):
            if keep(b):
                nd.Fill(np.array([x, 1.5]), value(b, x))
        for c in range(18):
            y = 1.25 + 0.5 * c
            if keep(18 * b + c):
                sparse2.Fill(np.array([x, y, 1.5]), value(18 * b + c, x, y))
    if outside:
        for nd, coords in ((thn, ([0.0, 1.5], [11.0, 1.5])), (sparse, ([0.0, 1.5], [11.0, 1.5])), (sparse2, ([0.0, 5.0, 1.5], [11.0, 5.0, 1.5]))):
            for c in coords:
                nd.Fill(np.array(c), outside)
    xs = [1.0 + 0.5 * i for i in range(19)]
    kept = order([(i, x) for i, x in enumerate(xs) if keep(i)])
    g, ge, ga, gp = (sized(cls, len(kept)) for cls in (ROOT.TGraph, ROOT.TGraphErrors, ROOT.TGraphAsymmErrors, ROOT.TGraphPolar))
    gm = ROOT.TGraphMultiErrors("graph_multi", "", len(kept), 2)
    for n, (i, x) in enumerate(kept):
        y = value(i, x)
        for graph in (g, ge, ga, gm):
            graph.SetPoint(n, x, y)
        ge.SetPointError(n, 0.2, abs(0.1 * y))
        ga.SetPointError(n, 0.2, 0.2, abs(0.05 * y), abs(0.15 * y))
        gm.SetPointEX(n, 0.2, 0.2)
        gm.SetPointEY(n, 0, abs(0.05 * y), abs(0.05 * y))
        gm.SetPointEY(n, 1, abs(0.1 * y), abs(0.1 * y))
        gp.SetPoint(n, 2 * ROOT.TMath.Pi() * i / len(xs), y)
    total = ROOT.TH1D("total", "", 18, 1.0, 10.0)
    passed = ROOT.TH1D("passed", "", 18, 1.0, 10.0)
    total2 = ROOT.TH2D("total2", "", 9, 1.0, 10.0, 9, 1.0, 10.0)
    passed2 = ROOT.TH2D("passed2", "", 9, 1.0, 10.0, 9, 1.0, 10.0)
    for b in range(1, 19):
        if keep(b - 1):
            total.SetBinContent(b, 100)
            passed.SetBinContent(b, 0 if variant == "zeros" else round(8 * _value(total.GetBinCenter(b))))
    for i in range(1, 10):
        for j in range(1, 10):
            if keep(9 * (i - 1) + j - 1):
                total2.SetBinContent(i, j, 100)
                passed2.SetBinContent(i, j, 0 if variant == "zeros" else round(4 * (_value(i + 0.5) + _value(j + 0.5))))
    eff = ROOT.TEfficiency(passed, total)
    eff2 = ROOT.TEfficiency(passed2, total2)
    h2 = ROOT.TH2D("hist2d", "", 18, 1.0, 10.0, 18, 1.0, 10.0)
    p2 = ROOT.TProfile2D("profile2d", "", 18, 1.0, 10.0, 18, 1.0, 10.0)
    for i in range(1, 19):
        for j in range(1, 19):
            index = 18 * (i - 1) + j - 1
            if keep(index):
                x, y = h2.GetXaxis().GetBinCenter(i), h2.GetYaxis().GetBinCenter(j)
                h2.SetBinContent(i, j, value(index, x, y))
                p2.Fill(x, y, 0.9 * value(index, x, y))
                p2.Fill(x, y, 1.1 * value(index, x, y))
    h2.SetEntries(sum(keep(n) for n in range(18 * 18)))
    poly = ROOT.srptest_poly()
    for i in range(9):
        for j in range(9):
            if keep(9 * i + j):
                poly.SetBinContent(1 + 9 * i + j, value(9 * i + j, 1.5 + i, 1.5 + j))
    h3 = ROOT.TH3D("hist3d", "", 9, 1.0, 10.0, 9, 1.0, 10.0, 9, 1.0, 10.0)
    p3 = ROOT.TProfile3D("profile3d", "", 9, 1.0, 10.0, 9, 1.0, 10.0, 9, 1.0, 10.0)
    for i in range(1, 10):
        for j in range(1, 10):
            for k in range(1, 10):
                index = 81 * (i - 1) + 9 * (j - 1) + k - 1
                if keep(index):
                    h3.SetBinContent(i, j, k, value(index, i + 0.5, j + 0.5, k + 0.5))
                    p3.Fill(i + 0.5, j + 0.5, k + 0.5, value(index, i + 0.5, j + 0.5, k + 0.5))
    h3.SetEntries(sum(keep(n) for n in range(9**3)))
    if outside:
        for hist in (h2, p2, poly):
            hist.Fill(0.0, 5.0, outside)
            hist.Fill(11.0, 5.0, outside)
        for hist in (h3, p3):
            hist.Fill(0.0, 5.0, 5.0, outside)
            hist.Fill(11.0, 5.0, 5.0, outside)
    points = order([(n, 1.0 + (n % 10), 1.0 + (n // 10)) for n in range(100) if keep(n)])
    g2, g2e = sized(ROOT.TGraph2D, len(points)), sized(ROOT.TGraph2DErrors, len(points))
    for m, (n, x, y) in enumerate(points):
        for graph in (g2, g2e):
            graph.SetPoint(m, x, y, value(n, x, y))
        g2e.SetPointError(m, 0.1, 0.1, abs(0.1 * value(n, x, y)))
    for graph, name in ((g2, "graph2d"), (g2e, "graph2d_errors")):
        graph.SetName(name)
    objects = {"hist": h, "hist_noerrors": hn, "hist_counts": hc, "hist_var": hv, "profile": p, "efficiency": eff, "thn": thn, "thn_sparse": sparse, "graph": g, "graph_errors": ge}
    objects.update({"graph_asymm": ga, "graph_multi": gm, "graph_polar": gp, "hist2d": h2, "profile2d": p2, "hist2poly": poly})
    objects.update({"efficiency2d": eff2, "thn_sparse2d": sparse2, "graph2d": g2, "graph2d_errors": g2e, "hist3d": h3, "profile3d": p3})
    if variant in FUNCTION_VARIANTS:
        npx = NPX.get(variant, NPX["normal"])
        cpp = variant if variant in ("negative", "zeros") else "normal"
        functions = {
            "tf1": ROOT.TF1("tf1", formula("tf1", variant), 1.0, 10.0),
            "tf2": ROOT.TF2("tf2", formula("tf2", variant), 1.0, 10.0, 1.0, 10.0),
            "tf3": ROOT.TF3("tf3", formula("tf3", variant), 1.0, 10.0, 1.0, 10.0, 1.0, 10.0),
            "tf1_cpp": ROOT.TF1("tf1_cpp", getattr(ROOT, f"srptest_f1_{cpp}"), 1.0, 10.0, 0),
            "tf2_cpp": ROOT.TF2("tf2_cpp", getattr(ROOT, f"srptest_f2_{cpp}"), 1.0, 10.0, 1.0, 10.0, 0),
            "tf3_cpp": ROOT.TF3("tf3_cpp", getattr(ROOT, f"srptest_f3_{cpp}"), 1.0, 10.0, 1.0, 10.0, 1.0, 10.0, 0),
        }
        for name, func in functions.items():
            dim = DATA_KINDS[name][0]
            n = min(npx, NPX_MAX[dim])
            func.SetNpx(n)
            if dim > 1:
                func.SetNpy(n)
            if dim > 2:
                func.SetNpz(n)
        objects.update(functions)
    objects = {kind: obj for kind, obj in objects.items() if has_variant(kind, variant)}
    for obj in objects.values():
        ROOT.SetOwnership(obj, False)
    return objects


def write_objects(path, objects):
    """writes objects (a dict name: object) into a ROOT file"""
    f = ROOT.TFile(path, "RECREATE")
    for name, obj in objects.items():
        obj.Write(name)
    f.Close()


def add_data(pad, kind, source="inputs", variant="normal"):
    """adds data of a kind (see data_objects) to a pad and returns it; source names the data source holding the variant"""
    if kind == "function":
        return pad.AddFunction(formula(kind, variant)).SetRangeX(1.0, 10.0)
    if kind == "function2d":
        return pad.AddFunction(formula(kind, variant)).SetRangeX(1.0, 10.0).SetRangeY(1.0, 10.0)
    data = pad.AddData(kind, source)
    if "projected" in DATA_KINDS[kind][2]:
        data.Project([0] if DATA_KINDS[kind][0] == 1 else [0, 1])
    return data


def source_of(variant):
    """the data source Project.inputs makes for a variant"""
    return "inputs" if variant == "normal" else f"inputs_{variant}"


def base_of(kind):
    """the base plot for data of a kind"""
    return "1d" if DATA_KINDS[kind][0] == 1 else "2d"


def drawn_type(kind):
    """the type data of a kind is drawn as (the columns of the drawing option tables): hist1d, graph, function, hist2d, graph2d,
    function2d, hist3d, function3d; a TEfficiency as the graph or 2d histogram ROOT makes of it, a THn as its projection"""
    dim, cls, _ = DATA_KINDS[kind]
    if cls.startswith("TF"):
        return ("function", "function2d", "function3d")[dim - 1]
    if cls.startswith("TGraph2D"):
        return "graph2d"
    if cls.startswith("TGraph") or (cls == "TEfficiency" and dim == 1):
        return "graph"
    return f"hist{dim}d"


def data_name(kind, variant="normal"):
    """the name SciRooPlot gives data of a kind in messages: the formula of a function of the plot definition, else the input's name"""
    return formula(kind, variant) if "formula" in DATA_KINDS[kind][2] else kind


def option(text):
    """a drawing option as ROOT reads it: upper case, without SAME, single spaces"""
    return " ".join(str(text).upper().replace("SAME", "").split())


def per_error_set(opt, sets=2):
    """the option an alias gives a graph with several sets of y errors (TGraphMultiErrors; that of data_objects has 2), as option() reads
    it: the graph part, then one part per set holding the error styles (boxes and bands 2-5, no errors X, no end lines Z, brackets, arrows)"""
    drawn = "SAME " + opt
    graph = "".join(c for c in drawn if c not in "2345")
    errors = "".join(c for c in drawn if c in "2345XZ[]|> ")
    return option(graph + f";{errors}" * sets)


def no_scale(opt):
    """a drawing option without its colour scale: the Z ending a word of a 2d or 3d option (COLZ, COLZ0, LEGO2Z, CONTZ, PCOLZ, BOX2Z)"""
    return " ".join(re.sub(r"(?<=[A-Z0-9])Z(?=0?$)", "", word) for word in opt.split())


ALIASES = list(S.draw_option_t.__members__)


def alias_table():
    """the documented alias table (docs/skill/scirooplot/reference/drawing-options.md): {alias: {drawn type: ROOT option}}, with "-" for
    an alias the type cannot draw and "graph" or "hist1d" for one it is drawn with as that type (see drawn_type)"""
    columns = {"1D histogram": "hist1d", "graph": "graph", "function": "function", "2D histogram": "hist2d", "2D graph": "graph2d", "2D function": "function2d", "3D histogram": "hist3d", "3D function": "function3d"}
    table, header = {}, None
    with open(os.path.join(os.path.dirname(__file__), "..", "docs", "skill", "scirooplot", "reference", "drawing-options.md"), encoding="utf-8") as f:
        for line in f:
            cells = [c.strip() for c in re.split(r" \| ", line.strip().strip("|").strip())]
            if cells[0] == "alias":
                header = [columns[c] for c in cells[1:]]
            elif header and len(cells) == len(header) + 1 and cells[0].startswith("`"):
                values = [{"*as graph*": "graph", "*as histogram*": "hist1d", "–": "-"}.get(c, c.strip("`")) for c in cells[1:]]
                table[cells[0].strip("`")] = dict(zip(header, values))
    return table


# ---------------------------------------------------------------------------------------------
# references: differences between a drawn object and the one plain ROOT gives ("" if equal)
# ---------------------------------------------------------------------------------------------
def _close_all(name, ref, obs, rel):
    for i, (a, b) in enumerate(zip(ref, obs)):
        if not close(a, b, rel) and not (math.isnan(a) and math.isnan(b)):
            return f"{name}[{i}] {b:g}, expected {a:g}"
    return ""


def hist_diff(ref, obs, rel=1e-9):
    """bin edges, contents and errors of every cell (with under- and overflow) of two histograms of any dimension, TH2Poly by bin"""
    if not obs:
        return "not drawn"
    if ref.GetDimension() != obs.GetDimension() or ref.InheritsFrom("TH2Poly") != obs.InheritsFrom("TH2Poly"):
        return f"{obs.ClassName()}, expected {ref.ClassName()}"
    if ref.InheritsFrom("TH2Poly"):
        bins = range(1, ref.GetNumberOfBins() + 1)
    else:
        for axis in "XYZ"[: ref.GetDimension()]:
            a, b = getattr(ref, f"Get{axis}axis")(), getattr(obs, f"Get{axis}axis")()
            if a.GetNbins() != b.GetNbins():
                return f"{b.GetNbins()} bins in {axis}, expected {a.GetNbins()}"
            edges = _close_all(f"{axis} edge", [a.GetBinLowEdge(i) for i in range(1, a.GetNbins() + 2)], [b.GetBinLowEdge(i) for i in range(1, b.GetNbins() + 2)], rel)
            if edges:
                return edges
        bins = range(ref.GetNcells())
    return _close_all("content", [ref.GetBinContent(i) for i in bins], [obs.GetBinContent(i) for i in bins], rel) or _close_all("error", [ref.GetBinError(i) for i in bins], [obs.GetBinError(i) for i in bins], rel)


def graph_values(graph):
    """the points of a 1d or 2d graph as lists of values by name (x, y, z and the errors it has; per error dimension of a TGraphMultiErrors)"""
    n = graph.GetN()
    if graph.InheritsFrom("TGraph2D"):
        values = {k: [getattr(graph, f"Get{k.upper()}")()[i] for i in range(n)] for k in "xyz"} if n else {k: [] for k in "xyz"}
        if graph.InheritsFrom("TGraph2DErrors"):
            values.update({f"e{k}": [getattr(graph, f"GetError{k.upper()}")(i) for i in range(n)] for k in "xyz"})
        return values
    values = {"x": [graph.GetPointX(i) for i in range(n)], "y": [graph.GetPointY(i) for i in range(n)]}
    if graph.GetEY() or graph.GetEYlow():  # a TGraph has no errors (its getters give -1)
        for e in ("ErrorXlow", "ErrorXhigh", "ErrorYlow", "ErrorYhigh"):
            values[e] = [getattr(graph, f"Get{e}")(i) for i in range(n)]
    if graph.InheritsFrom("TGraphMultiErrors"):
        for d in range(graph.GetNYErrors()):
            values[f"ey{d}low"] = [graph.GetErrorYlow(i, d) for i in range(n)]
            values[f"ey{d}high"] = [graph.GetErrorYhigh(i, d) for i in range(n)]
    return values


def values_diff(ref, obs, rel=1e-9):
    """differences between two dicts of value lists (see graph_values)"""
    for key, values in ref.items():
        if key not in obs:
            return f"no {key}"
        if len(values) != len(obs[key]):
            return f"{len(obs[key])} points, expected {len(values)}"
        found = _close_all(key, values, obs[key], rel)
        if found:
            return found
    return ""


def graph_diff(ref, obs, rel=1e-9):
    """points and errors of two 1d or 2d graphs (ref may also be a dict as by graph_values)"""
    if not obs:
        return "not drawn"
    return values_diff(ref if isinstance(ref, dict) else graph_values(ref), graph_values(obs), rel)


def func_points(func):
    """points inside the range of a function where it is compared: 5 per axis, away from the edges, on the points it is sampled at (a
    function stored as samples gives its exact values there)"""
    dim = func.GetNdim()
    lows = [getattr(func, f"Get{axis}min")() for axis in "XYZ"[:dim]]
    highs = [getattr(func, f"Get{axis}max")() for axis in "XYZ"[:dim]]
    counts = [func.GetNpx(), func.GetNpy() if dim > 1 else 0, func.GetNpz() if dim > 2 else 0][:dim]
    steps = [[lo + (hi - lo) * round(n * (k + 1) / 6) / n for k in range(5)] for lo, hi, n in zip(lows, highs, counts)]
    return list(itertools.product(*steps))


def func_diff(ref, obs, rel=1e-9):
    """values of two functions at func_points(obs); ref a function or a callable taking the coordinates"""
    if not obs:
        return "not drawn"
    points = func_points(obs)
    evaluate = ref if callable(ref) and not hasattr(ref, "Eval") else (lambda *c: ref.Eval(*c))
    return _close_all("value", [evaluate(*c) for c in points], [obs.Eval(*c) for c in points], rel)


# ---------------------------------------------------------------------------------------------
# a project and its runs
# ---------------------------------------------------------------------------------------------
class Run:
    """the result of one run of the plot program"""

    def __init__(self, proc, plotsFile, snapshot):
        self.rc = proc.returncode
        self.log = re.sub(r"\x1b\[[0-9;]*m", "", proc.stdout + proc.stderr)
        self.file = None
        if os.path.exists(plotsFile):
            shutil.copyfile(plotsFile, snapshot)  # the next run deletes the output folder
            self.file = ROOT.TFile.Open(snapshot)

    def lines(self, kind):
        """the lines of a kind: ERR, WARN, INFO"""
        return [line for line in self.log.splitlines() if f"[ {kind}" in line or f"[{kind}" in line]

    def errors(self):
        """the errors of the run: SciRooPlot's and ROOT's (Error in <...>, Fatal in <...>), and crashes"""
        marks = ("[ ERR", "[ERR", "Error in <", "Fatal in <", "*** Break ***")
        return [line for line in self.log.splitlines() if any(mark in line for mark in marks)]

    def canvas(self, name, group):
        groupDir = self.file.Get(group) if self.file else None
        if groupDir:
            for key in groupDir.GetListOfKeys():
                if key.GetName().split(":")[0] == name:
                    return key.ReadObj()
        return None

    def pad(self, name, group, pad=1):
        canvas = self.canvas(name, group)
        return canvas.GetPrimitive(f"Pad_{pad}") if canvas else None

    def objects(self, name, group, pad=1):
        """the drawn data of a pad in the order it was added"""
        padObj = self.pad(name, group, pad)
        found = []
        for prim in padObj.GetListOfPrimitives() if padObj else []:
            m = re.match(r"^(\d+):", prim.GetName())
            if m:
                found.append((int(m.group(1)), prim))
        return [obj for _, obj in sorted(found, key=lambda item: item[0])]

    def drawn(self, name, group, pad=1):
        """the drawn data of a pad in the order it was added, as (object, drawing option as by option())"""
        found = [(int(obj.GetName().split(":")[0]), obj, option(opt)) for obj, opt in self._links(name, group, pad) if re.match(r"^\d+:", obj.GetName())]
        return [(obj, opt) for _, obj, opt in sorted(found, key=lambda item: item[0])]

    def frame_option(self, name, group, pad=1):
        """the drawing option of the frame histogram of a pad (as by option()), None if it has none"""
        return next((option(opt) for obj, opt in self._links(name, group, pad) if obj.GetName() == f"axis_hist_pad_{pad}"), None)

    def links(self, name, group, pad=1):
        """every primitive of a pad as (object, drawing option as stored)"""
        return list(self._links(name, group, pad))

    def _links(self, name, group, pad):
        padObj = self.pad(name, group, pad)
        link = padObj.GetListOfPrimitives().FirstLink() if padObj else None
        while link:
            yield link.GetObject(), str(link.GetOption())
            link = link.Next()

    def frame(self, name, group, pad=1):
        """the frame of a pad in axis values: {"X": (min, max), "Y": (min, max)}, or None if the plot was not made"""
        padObj = self.pad(name, group, pad)
        if not padObj:
            return None
        x = (padObj.GetUxmin(), padObj.GetUxmax())
        y = (padObj.GetUymin(), padObj.GetUymax())
        return {"X": tuple(10**v for v in x) if padObj.GetLogx() else x, "Y": tuple(10**v for v in y) if padObj.GetLogy() else y}

    def frame_hist(self, name, group, pad=1):
        """the histogram drawn as the frame of a pad (axes, z range of 2d data)"""
        padObj = self.pad(name, group, pad)
        return padObj.GetPrimitive(f"axis_hist_pad_{pad}") if padObj else None

    def warnings(self, text=""):
        """the warnings of the run containing text"""
        return [line for line in self.lines("WARN") if text in line]


class Project:
    """one isolated project with its own configuration, inputs and output"""

    def __init__(self, name, bases=("1d", "2d")):
        self.name = name
        self.dir = os.path.join(WORK, name)
        os.makedirs(self.dir)
        self.pm = S.PlotManager(name)
        for base in bases:
            self.pm.AddBasePlot(S.PlotManager.MakeBasePlot(base))
        self.nRuns = 0
        self.saved = False

    def path(self, *parts):
        return os.path.join(self.dir, *parts)

    def srp(self, *args):
        return subprocess.run([os.path.join(PREFIX, "bin", "srp"), *args], capture_output=True, text=True, check=False)

    def source(self, name, objects):
        """a data source of that name: a file holding objects (a dict name: object)"""
        write_objects(self.path(f"{name}.root"), objects)
        self.pm.AddDataSource(name, self.path(f"{name}.root"))

    def inputs(self, variants=("normal",)):
        """the data source "inputs" with one object of every data kind; other variants in the data sources "inputs_<variant>"."""
        for variant in variants:
            self.source(source_of(variant), data_objects(variant))

    def plot(self, group=".+", name=".+", mode="file", timeout=1800):
        if not self.saved:
            self.pm.SaveProject()
            self.srp("add", self.name, self.path("DefinePlots.py"), self.path("out"))
            selected = self.srp("select", self.name)
            if selected.returncode != 0:
                sys.exit(f"cannot select project {self.name}: {selected.stdout}{selected.stderr}")
            self.saved = True
        shutil.rmtree(self.path("out"), ignore_errors=True)
        proc = subprocess.run([os.path.join(PREFIX, "bin", "plot"), group, name, mode], capture_output=True, text=True, timeout=timeout, check=False)
        self.nRuns += 1
        return Run(proc, self.path("out", "Plots.root"), self.path(f"run{self.nRuns}_Plots.root"))
