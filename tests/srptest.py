"""Shared building blocks of the SciRooPlot tests.

A test file defines its plots through the Python interface in one isolated project, runs the plot program once, reads the drawn
objects back from Plots.root and checks them against an independent reference (plain ROOT, an invariant, the requested values).
It exits 0 if every check passes, 1 if one fails and 77 if it cannot run (shown as skipped by CTest).

Environment (set by CTest): SRP_PREFIX, a build or install directory with bin/plot; SRP_TESTS_WORK, the scratch folder of the test;
SRP_TESTS_FULL=1 for the full case lists (weekly), otherwise a subset holding every value of every dimension at least once.
"""

import functools
import glob
import itertools
import math
import operator
import os
import pickle
import pty
import random
import re
import select
import shutil
import subprocess
import sys
import time
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
# (-4 to 5 in the variant crossing) with positive values (usable on log axes): about 1 to 10 for 1d data, products of those for 2d and 3d data, about 0.1 to 0.8 for
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
# few or many points (coarse, fine), coordinates from -4 to 5 on every axis with the values of normal (crossing: bins and points at and
# below zero, for log axes). A TEfficiency counts events: it has no negative, NaN, infinite, huge or overflow variant. Polar graphs have
# no crossing variant (their coordinates are angles).
VARIANTS = ("normal", "negative", "nan", "inf", "zeros", "empty", "single", "huge", "unsorted", "overflow", "coarse", "fine", "crossing")
COUNTS_VARIANTS = ("normal", "zeros", "empty", "single", "crossing")
FUNCTION_VARIANTS = ("normal", "negative", "zeros", "coarse", "fine", "crossing")
CROSSING_SHIFT = -5.0  # the shift of every coordinate in the variant crossing
NPX = {"normal": 100, "coarse": 5, "fine": 10000}  # points of a function per axis, at most NPX_MAX in 2d and 3d (memory of the stored samples)
NPX_MAX = {1: 10000, 2: 200, 3: 20}


def has_variant(kind, variant):
    _, cls, props = DATA_KINDS[kind]
    if variant == "crossing" and "polar" in props:
        return False
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
    if variant == "crossing":
        return re.sub(r"\b([xyz])\b", rf"(\1 + {-CROSSING_SHIFT:g})", base)
    return {"negative": f"({base}) - {5**dim}", "zeros": f"0 * {' * '.join('xyz'[:dim])}"}.get(variant, base)


def axis_range(variant="normal"):
    """the range the data of a variant covers on every axis"""
    shift = CROSSING_SHIFT if variant == "crossing" else 0.0
    return (1.0 + shift, 10.0 + shift)


def _declare():
    """C++ makers for what PyROOT cannot build: TH2Poly bins (AddBin with numbers fails in a pythonization) and compiled functions"""
    if hasattr(ROOT, "srptest_poly"):
        return
    code = """
    TH2Poly* srptest_poly(double shift) {
      auto* h = new TH2Poly("hist2poly", "", 1. + shift, 10. + shift, 1. + shift, 10. + shift);
      for (int i = 0; i < 9; ++i)
        for (int j = 0; j < 9; ++j) h->AddBin(1. + shift + i, 1. + shift + j, 2. + shift + i, 2. + shift + j);
      return h;
    }
    double srptest_value(double x) { return 1. + (x - 1.) * (1. - 0.3 * std::pow((x - 5.5) / 4.5, 2)); }
    """
    for variant, shift, scale, d in (("normal", 0, 1, 0), ("negative", 1, 1, 0), ("zeros", 0, 0, 0), ("crossing", 0, 1, -CROSSING_SHIFT)):
        code += f"""
    double srptest_f1_{variant}(double* x, double*) {{ return {scale} * (srptest_value(x[0] + {d}) - {shift} * 5.); }}
    double srptest_f2_{variant}(double* x, double*) {{ return {scale} * ((x[0] + {d}) * (x[1] + {d}) - {shift} * 25.); }}
    double srptest_f3_{variant}(double* x, double*) {{ return {scale} * ((x[0] + {d}) * (x[1] + {d}) * (x[2] + {d}) - {shift} * 125.); }}"""
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
    d = CROSSING_SHIFT if variant == "crossing" else 0.0  # the shift of every coordinate
    lo, hi = axis_range(variant)

    def value(index, *coords):
        """the value of item index (in fill order) at coords"""
        v = 1e35 if variant == "huge" else 1.0  # ROOT finds minima and maxima of histograms only below the largest float (3.4e38)
        for c in coords:
            v *= _value(c - d)
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

    h = ROOT.TH1D("hist", "", 18, lo, hi)
    hn = ROOT.TH1D("hist_noerrors", "", 18, lo, hi)
    hc = ROOT.TH1D("hist_counts", "", 18, lo, hi)
    edges = ROOT.std.vector("double")([e + d for e in (1.0, 1.2, 1.5, 2.0, 2.7, 3.5, 4.5, 6.0, 7.5, 9.0, 10.0)])
    hv = ROOT.TH1D("hist_var", "", edges.size() - 1, edges.data())
    p = ROOT.TProfile("profile", "", 18, lo, hi)
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
    low, high = np.array([lo, 0.0, 0.0]), np.array([hi, 3.0, 3.0])
    thn = ROOT.THnD("thn", "", 2, nBins[:2], low[:2], high[:2])
    sparse = ROOT.THnSparseD("thn_sparse", "", 2, nBins[:2], low[:2], high[:2])
    sparse2 = ROOT.THnSparseD("thn_sparse2d", "", 3, np.array([18, 18, 3], dtype=np.int32), np.array([lo, lo, 0.0]), np.array([hi, hi, 3.0]))
    for b in range(18):
        x = lo + 0.25 + 0.5 * b
        for nd in (thn, sparse):
            if keep(b):
                nd.Fill(np.array([x, 1.5]), value(b, x))
        for c in range(18):
            y = lo + 0.25 + 0.5 * c
            if keep(18 * b + c):
                sparse2.Fill(np.array([x, y, 1.5]), value(18 * b + c, x, y))
    if outside:
        for nd, coords in ((thn, ([0.0, 1.5], [11.0, 1.5])), (sparse, ([0.0, 1.5], [11.0, 1.5])), (sparse2, ([0.0, 5.0, 1.5], [11.0, 5.0, 1.5]))):
            for c in coords:
                nd.Fill(np.array(c), outside)
    xs = [lo + 0.5 * i for i in range(19)]
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
    total = ROOT.TH1D("total", "", 18, lo, hi)
    passed = ROOT.TH1D("passed", "", 18, lo, hi)
    total2 = ROOT.TH2D("total2", "", 9, lo, hi, 9, lo, hi)
    passed2 = ROOT.TH2D("passed2", "", 9, lo, hi, 9, lo, hi)
    for b in range(1, 19):
        if keep(b - 1):
            total.SetBinContent(b, 100)
            passed.SetBinContent(b, 0 if variant == "zeros" else round(8 * _value(total.GetBinCenter(b) - d)))
    for i in range(1, 10):
        for j in range(1, 10):
            if keep(9 * (i - 1) + j - 1):
                total2.SetBinContent(i, j, 100)
                passed2.SetBinContent(i, j, 0 if variant == "zeros" else round(4 * (_value(i + 0.5) + _value(j + 0.5))))
    eff = ROOT.TEfficiency(passed, total)
    eff2 = ROOT.TEfficiency(passed2, total2)
    h2 = ROOT.TH2D("hist2d", "", 18, lo, hi, 18, lo, hi)
    p2 = ROOT.TProfile2D("profile2d", "", 18, lo, hi, 18, lo, hi)
    for i in range(1, 19):
        for j in range(1, 19):
            index = 18 * (i - 1) + j - 1
            if keep(index):
                x, y = h2.GetXaxis().GetBinCenter(i), h2.GetYaxis().GetBinCenter(j)
                h2.SetBinContent(i, j, value(index, x, y))
                p2.Fill(x, y, 0.9 * value(index, x, y))
                p2.Fill(x, y, 1.1 * value(index, x, y))
    h2.SetEntries(sum(keep(n) for n in range(18 * 18)))
    poly = ROOT.srptest_poly(d)
    for i in range(9):
        for j in range(9):
            if keep(9 * i + j):
                poly.SetBinContent(1 + 9 * i + j, value(9 * i + j, lo + 0.5 + i, lo + 0.5 + j))
    h3 = ROOT.TH3D("hist3d", "", 9, lo, hi, 9, lo, hi, 9, lo, hi)
    p3 = ROOT.TProfile3D("profile3d", "", 9, lo, hi, 9, lo, hi, 9, lo, hi)
    for i in range(1, 10):
        for j in range(1, 10):
            for k in range(1, 10):
                index = 81 * (i - 1) + 9 * (j - 1) + k - 1
                x, y, z = (lo - 0.5 + n for n in (i, j, k))
                if keep(index):
                    h3.SetBinContent(i, j, k, value(index, x, y, z))
                    p3.Fill(x, y, z, value(index, x, y, z))
    h3.SetEntries(sum(keep(n) for n in range(9**3)))
    if outside:
        for hist in (h2, p2, poly):
            hist.Fill(0.0, 5.0, outside)
            hist.Fill(11.0, 5.0, outside)
        for hist in (h3, p3):
            hist.Fill(0.0, 5.0, 5.0, outside)
            hist.Fill(11.0, 5.0, 5.0, outside)
    points = order([(n, lo + (n % 10), lo + (n // 10)) for n in range(100) if keep(n)])
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
        cpp = variant if variant in ("negative", "zeros", "crossing") else "normal"
        functions = {
            "tf1": ROOT.TF1("tf1", formula("tf1", variant), lo, hi),
            "tf2": ROOT.TF2("tf2", formula("tf2", variant), lo, hi, lo, hi),
            "tf3": ROOT.TF3("tf3", formula("tf3", variant), lo, hi, lo, hi, lo, hi),
            "tf1_cpp": ROOT.TF1("tf1_cpp", getattr(ROOT, f"srptest_f1_{cpp}"), lo, hi, 0),
            "tf2_cpp": ROOT.TF2("tf2_cpp", getattr(ROOT, f"srptest_f2_{cpp}"), lo, hi, lo, hi, 0),
            "tf3_cpp": ROOT.TF3("tf3_cpp", getattr(ROOT, f"srptest_f3_{cpp}"), lo, hi, lo, hi, lo, hi, 0),
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
    for kind, obj in objects.items():
        obj.SetName(kind)  # the name it is found by when handed over in memory (in a file it is the key's)
        ROOT.SetOwnership(obj, False)
    return objects


def write_objects(path, objects):
    """writes objects (a dict name: object) into a ROOT file"""
    f = ROOT.TFile(path, "RECREATE")
    for name, obj in objects.items():
        obj.Write(name)
    f.Close()


# ---------------------------------------------------------------------------------------------
# tables: the same columns as tree, RNTuple or CSV file
# ---------------------------------------------------------------------------------------------
class Texts(list):
    """a column of strings (std::string; in a CSV file the text)"""


class CStrings(list):
    """a column of C strings (a leaf of type C, as Branch("s", buffer, "s/C") or uproot write them; trees only)"""


class Vectors(list):
    """a column of collections (std::vector<float>, one sequence per row; trees and RNTuples only)"""


# the leaf type of a numpy type, and the C++ type of an RNTuple field
LEAF_TYPES = {"float64": "D", "float32": "F", "int8": "B", "uint8": "b", "int16": "S", "uint16": "s", "int32": "I", "uint32": "i", "int64": "L", "uint64": "l", "bool": "O"}


def _declare_tables():
    if hasattr(ROOT, "srptest_columns"):
        return
    ROOT.gInterpreter.Declare(r"""
    #include <ROOT/RNTupleModel.hxx>
    #include <ROOT/RNTupleWriter.hxx>
    struct srptest_columns {
      struct column_t { std::string name; char type; std::vector<double> values; std::vector<std::string> texts; std::vector<std::size_t> offsets; };
      struct buffer_t { double D; float F; Char_t B; UChar_t b; Short_t S; UShort_t s; Int_t I; UInt_t i; Long64_t L; ULong64_t l; bool O; std::string str; char cstr[64]; std::vector<float> vec; };
      std::vector<column_t> columns;
      void Numbers(const char* name, char type, const double* v, std::size_t n) { columns.push_back({name, type, std::vector<double>(v, v + n), {}, {}}); }
      void Texts(const char* name, char type, const std::vector<std::string>& t) { columns.push_back({name, type, {}, t, {}}); }
      void Vectors(const char* name, const double* flat, const std::vector<std::size_t>& offsets) { columns.push_back({name, 'V', std::vector<double>(flat, flat + offsets.back()), {}, offsets}); }
      void Set(const column_t& c, buffer_t& b, std::size_t k) {
        double v = (c.values.empty() || c.type == 'V') ? 0. : c.values[k];
        switch (c.type) {
          case 'D': b.D = v; break; case 'F': b.F = v; break; case 'B': b.B = v; break; case 'b': b.b = v; break;
          case 'S': b.S = v; break; case 's': b.s = v; break; case 'I': b.I = v; break; case 'i': b.i = v; break;
          case 'L': b.L = v; break; case 'l': b.l = v; break; case 'O': b.O = v; break;
          case 'T': b.str = c.texts[k]; break;
          case 'C': std::strncpy(b.cstr, c.texts[k].c_str(), 63); b.cstr[63] = 0; break;
          case 'V': b.vec.assign(c.values.begin() + c.offsets[k], c.values.begin() + c.offsets[k + 1]); break;
        }
      }
      TDirectory* Folder(TFile& f, const char* folder) {
        if (!folder[0]) return &f;
        return f.GetDirectory(folder) ? f.GetDirectory(folder) : f.mkdir(folder, "", true);
      }
      void WriteTree(const char* path, const char* folder, const char* name, std::size_t begin, std::size_t end) {
        TFile f(path, "UPDATE");
        Folder(f, folder)->cd();
        TTree t(name, name);
        std::deque<buffer_t> buffers(columns.size());
        for (std::size_t i = 0; i < columns.size(); ++i) {
          auto& c = columns[i]; auto& b = buffers[i]; const std::string leaf = c.name + "/" + c.type;
          switch (c.type) {
            case 'D': t.Branch(c.name.c_str(), &b.D, leaf.c_str()); break; case 'F': t.Branch(c.name.c_str(), &b.F, leaf.c_str()); break;
            case 'B': t.Branch(c.name.c_str(), &b.B, leaf.c_str()); break; case 'b': t.Branch(c.name.c_str(), &b.b, leaf.c_str()); break;
            case 'S': t.Branch(c.name.c_str(), &b.S, leaf.c_str()); break; case 's': t.Branch(c.name.c_str(), &b.s, leaf.c_str()); break;
            case 'I': t.Branch(c.name.c_str(), &b.I, leaf.c_str()); break; case 'i': t.Branch(c.name.c_str(), &b.i, leaf.c_str()); break;
            case 'L': t.Branch(c.name.c_str(), &b.L, leaf.c_str()); break; case 'l': t.Branch(c.name.c_str(), &b.l, leaf.c_str()); break;
            case 'O': t.Branch(c.name.c_str(), &b.O, leaf.c_str()); break; case 'C': t.Branch(c.name.c_str(), b.cstr, leaf.c_str()); break;
            case 'T': t.Branch(c.name.c_str(), &b.str); break; case 'V': t.Branch(c.name.c_str(), &b.vec); break;
          }
        }
        for (std::size_t k = begin; k < end; ++k) {
          for (std::size_t i = 0; i < columns.size(); ++i) Set(columns[i], buffers[i], k);
          t.Fill();
        }
        t.Write();
      }
      template <typename T> std::function<void(std::size_t)> Field(ROOT::RNTupleModel& model, const column_t& c, T (buffer_t::*member)) {
        auto field = model.MakeField<T>(c.name);
        auto b = std::make_shared<buffer_t>();
        return [this, &c, field, b, member](std::size_t k) { Set(c, *b, k); *field = (*b).*member; };
      }
      void WriteNTuple(const char* path, const char* folder, const char* name, std::size_t begin, std::size_t end) {
        auto model = ROOT::RNTupleModel::Create();
        std::vector<std::function<void(std::size_t)>> setters;
        for (const auto& c : columns) {
          switch (c.type) {
            case 'D': setters.push_back(Field<double>(*model, c, &buffer_t::D)); break; case 'F': setters.push_back(Field<float>(*model, c, &buffer_t::F)); break;
            case 'B': setters.push_back(Field<std::int8_t>(*model, c, reinterpret_cast<std::int8_t buffer_t::*>(&buffer_t::B))); break;
            case 'b': setters.push_back(Field<std::uint8_t>(*model, c, &buffer_t::b)); break;
            case 'S': setters.push_back(Field<std::int16_t>(*model, c, &buffer_t::S)); break; case 's': setters.push_back(Field<std::uint16_t>(*model, c, &buffer_t::s)); break;
            case 'I': setters.push_back(Field<std::int32_t>(*model, c, &buffer_t::I)); break; case 'i': setters.push_back(Field<std::uint32_t>(*model, c, &buffer_t::i)); break;
            case 'L': setters.push_back(Field<std::int64_t>(*model, c, reinterpret_cast<std::int64_t buffer_t::*>(&buffer_t::L))); break;
            case 'l': setters.push_back(Field<std::uint64_t>(*model, c, reinterpret_cast<std::uint64_t buffer_t::*>(&buffer_t::l))); break;
            case 'O': setters.push_back(Field<bool>(*model, c, &buffer_t::O)); break; case 'T': setters.push_back(Field<std::string>(*model, c, &buffer_t::str)); break;
            case 'V': setters.push_back(Field<std::vector<float>>(*model, c, &buffer_t::vec)); break;
            default: throw std::invalid_argument("RNTuples have no C strings");
          }
        }
        TFile f(path, "UPDATE");
        auto writer = ROOT::RNTupleWriter::Append(std::move(model), name, *Folder(f, folder));
        for (std::size_t k = begin; k < end; ++k) {
          for (auto& set : setters) set(k);
          writer->Fill();
        }
      }
    };
    """)


def _columns(columns):
    _declare_tables()
    table = ROOT.srptest_columns()
    for name, values in columns.items():
        if isinstance(values, (Texts, CStrings)):
            table.Texts(name, "T" if isinstance(values, Texts) else "C", ROOT.std.vector["std::string"]([str(v) for v in values]))
        elif isinstance(values, Vectors):
            offsets = np.cumsum([0] + [len(v) for v in values]).astype(np.uint64)
            flat = np.concatenate([np.asarray(v, dtype=np.float64) for v in values] + [np.zeros(1)])
            table.Vectors(name, flat, ROOT.std.vector["std::size_t"](offsets.tolist()))
        else:
            array = np.asarray(values)
            buffer = np.ascontiguousarray(array if len(array) else np.zeros(1), dtype=np.float64)  # an empty one is no buffer for cppyy
            table.Numbers(name, LEAF_TYPES[array.dtype.name], buffer, len(array))
    return table


def write_tree(path, columns, name="t", folder="", rows=None, ntuple=False):
    """writes columns (a dict name: numpy array, Texts, CStrings or Vectors) as a tree (an RNTuple with ntuple) into a ROOT file, created if
    needed, into a folder of it; rows: the slice of rows to write (all by default). The type of a column is its numpy type."""
    table = _columns(columns)
    n = len(next(iter(columns.values())))
    begin, end, _ = (rows or slice(None)).indices(n)
    (table.WriteNTuple if ntuple else table.WriteTree)(path, folder, name, begin, end)


def write_csv(path, columns, rows=None, delimiter=","):
    """writes columns (a dict name: numpy array or Texts) as a table file with a header: numbers as Python writes them (they read back
    exactly), bools as true and false"""
    n = len(next(iter(columns.values())))

    def text(v):
        if isinstance(v, (bool, np.bool_)):
            return "true" if v else "false"
        return repr(float(v)) if isinstance(v, (float, np.floating)) else str(v)

    with open(path, "w") as f:
        f.write(delimiter.join(columns) + "\n")
        for k in range(n)[rows or slice(None)]:
            f.write(delimiter.join(text(values[k]) for values in columns.values()) + "\n")


def add_data(pad, kind, source="inputs", variant="normal"):
    """adds data of a kind (see data_objects) to a pad and returns it; source names the data source holding the variant"""
    if kind == "function":
        return pad.AddFunction(formula(kind, variant)).SetRangeX(*axis_range(variant))
    if kind == "function2d":
        return pad.AddFunction(formula(kind, variant)).SetRangeX(*axis_range(variant)).SetRangeY(*axis_range(variant))
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


# Multi-panel plots of a data kind: grid (every panel of <dim>_2x2 draws it; 2d with the colour-scale cell), ratio (1d_2x1_ratio: the
# kind in both panels, its ratio by itself below each), gap (<dim>_2x1_gap, the panels sharing y or z by ShareAxis), strip (two panels
# under an empty strip holding a legend with the rows of their data, labelled by a placeholder and in the row), view (beside a 2d
# histogram drawn as a 3D view sharing z).
LAYOUTS = ("grid", "ratio", "gap", "strip", "view")
LAYOUT_BASES = ("1d_2x2", "2d_2x2", "1d_2x1_ratio", "1d_2x1_gap", "2d_2x1_gap", "2d_2x1")  # Project(bases=...) of a test using them


def has_layout(kind, layout):
    """whether a multi-panel plot of that layout is made of data of a kind: ratio panels for 1d frames, 3D views beside 2d and 3d data"""
    dim, _, props = DATA_KINDS[kind]
    if layout == "ratio":
        return dim == 1 and "polar" not in props
    return layout != "view" or dim > 1


def layout_plot(name, group, kind, layout, source="inputs", variant="normal"):
    """a multi-panel plot (see LAYOUTS) whose panels draw data of a kind in a variant"""
    dim, cls, props = DATA_KINDS[kind]
    flat = "1d" if dim == 1 else "2d"
    if layout == "strip":
        plot = S.Plot(name, S.PanelLayout([[S.Empty(1, 0.2).Span(2)], [S.Panel(), S.Panel()]]))
        plot.SetGroup(group)
        legend = plot[1].AddLegend()
        legend.SetNumColumns(2)
        add_data(plot[2], kind, source, variant).SetLegendLabel("<name>")
        add_data(plot[3], kind, source, variant)
        legend.AddDataEntry(2, 1)
        legend.AddDataEntry(3, 1).SetLabel("own label")
        return plot
    base = {"grid": f"{flat}_2x2", "ratio": "1d_2x1_ratio", "gap": f"{flat}_2x1_gap", "view": "2d_2x1"}[layout]
    plot = S.Plot(name, group, base)
    pads = {"1d_2x2": (1, 2, 3, 4), "2d_2x2": (1, 2, 4, 5), "1d_2x1_ratio": (1, 2), "1d_2x1_gap": (1, 2), "2d_2x1_gap": (1, 2), "2d_2x1": (1,)}[base]
    for pad in pads:
        add_data(plot[pad], kind, source, variant)
    if layout == "ratio":
        for pad in (3, 4):
            ratio = plot[pad].AddRatio(ratio_input(kind, source, variant), ratio_input(kind, source, variant))
            if "formula" in props:
                ratio.SetRangeX(*axis_range(variant))
    if layout == "gap" and "polar" not in props and cls != "TF3":  # polar drawings and TF3s share no y or z
        plot.ShareAxis("Y" if flat == "1d" else "Z", [1, 2])
    if layout == "view":
        add_data(plot[2], "hist2d").SetDrawOption(S.lego_colz)
    return plot


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
# colours
# ---------------------------------------------------------------------------------------------
# the colours data takes when neither it nor its pad nor pad 0 sets one, in this order and repeating (documented in SKILL.md)
DEFAULT_COLORS = (ROOT.kBlack, ROOT.kP10Blue, ROOT.kP10Red, ROOT.kP10Violet, ROOT.kP8Orange, ROOT.kP10Ash, ROOT.kP10Brown, ROOT.kTeal + 3)


def rgba(color, alpha=None):
    """a colour in any form SciRooPlot takes (ROOT index, "#rrggbb", (r, g, b) from 0 to 1) as (r, g, b, alpha), components from 0 to 255
    and alpha to two decimals (None for an index ROOT does not know); alpha replaces the colour's own"""
    if isinstance(color, str):
        rgb = tuple(int(color[i : i + 2], 16) for i in (1, 3, 5))
        own = 1.0
    elif isinstance(color, (tuple, list)):
        rgb = tuple(int(c * 255 + 0.5) for c in color)  # as SciRooPlot and ROOT round
        own = 1.0
    else:
        found = ROOT.gROOT.GetColor(int(color))
        if not found:
            return None
        rgb = tuple(int(getattr(found, f"Get{c}")() * 255 + 0.5) for c in ("Red", "Green", "Blue"))
        own = found.GetAlpha()
    return (*rgb, round(own if alpha is None else alpha, 2))


def colour_diff(expected, index, alpha=None, steps=1):
    """ "" if the ROOT colour index has the colour expected (any form, with alpha if given), else what it has. ROOT gives an rgb colour the
    index of an existing colour within one step of 255 in each component (TColor::GetColor): steps is the tolerance of the components"""
    want, got = rgba(expected, alpha), rgba(index)
    same = want is not None and got is not None and all(abs(a - b) <= steps for a, b in zip(want[:3], got[:3])) and want[3] == got[3]
    return "" if same else f"{got} (index {index}), expected {want}"


# ---------------------------------------------------------------------------------------------
# references: differences between a drawn object and the one plain ROOT gives ("" if equal)
# ---------------------------------------------------------------------------------------------
def _close_all(name, ref, obs, rel):
    for i, (a, b) in enumerate(zip(ref, obs)):
        if a != b and not close(a, b, rel) and not (math.isnan(a) and math.isnan(b)):
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


def drawn_diff(ref, obs, rel=1e-9):
    """differences between two drawn objects of any kind, or two lists of them (as by Run.objects): class, then histograms by
    hist_diff, graphs by graph_diff, functions by func_diff"""
    if isinstance(ref, (list, tuple)) or isinstance(obs, (list, tuple)):
        ref, obs = list(ref or []), list(obs or [])
        if len(ref) != len(obs):
            return f"{len(obs)} objects drawn, expected {len(ref)}"
        return next((f"{i}: {d}" for i, d in enumerate(drawn_diff(a, b, rel) for a, b in zip(ref, obs)) if d), "")
    if not obs:
        return "not drawn"
    if ref.ClassName() != obs.ClassName():
        return f"{obs.ClassName()}, expected {ref.ClassName()}"
    if ref.InheritsFrom("TH1"):
        return hist_diff(ref, obs, rel)
    if ref.InheritsFrom("TF1"):
        return func_diff(ref, obs, rel)
    return graph_diff(ref, obs, rel)


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
# references of the data area: modifiers and divisions done with plain ROOT on data as drawn without them
# ---------------------------------------------------------------------------------------------
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


class ModifierExpected:
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


def profile_means(p):
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
        h = profile_means(h)  # the means are smoothed
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
    return profile_means(h) if family(h) == "profile" else h


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


def modifier_reference(base, mods, exp):
    """the data as drawn without modifiers with the modifiers mods done by plain ROOT: a histogram (a profile as its means), the values of a
    graph (graph_values) or a function as a callable; what is not supported for the data goes into exp (a ModifierExpected)"""
    fam = family(base)
    reference = {"graph": reference_graph, "graph2d": reference_graph2d, "func": reference_func}.get(fam, reference_hist)
    return reference(base, set(mods), exp)


def division_family(obj):
    if obj.InheritsFrom("TF1"):
        return f"func{obj.GetNdim()}"
    if obj.InheritsFrom("TGraph2D"):
        return "graph2d"
    if obj.InheritsFrom("TGraph"):
        return "graph"
    return f"hist{obj.GetDimension()}"


def as_divided(h):
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
        self.edges = None  # the bin edges of a histogram
        if obj.InheritsFrom("TH1"):
            n = obj.GetNbinsX()
            self.x = [obj.GetBinCenter(b) for b in range(1, n + 1)]
            self.y = [obj.GetBinContent(b) for b in range(1, n + 1)]
            self.kind, self.e = "sym", [obj.GetBinError(b) for b in range(1, n + 1)]
            self.edges = [obj.GetXaxis().GetBinLowEdge(b) for b in range(1, n + 2)]
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
    """the denominator at the numerator's positions: (values, errors as sym list or (low, high) lists or None), or None if impossible;
    a histogram through its filled bins only (an empty bin, no content and no error, has no value: zero where a numerator point lies in it)"""
    knots = range(len(den.x))
    inEmptyBin = [False] * len(num.x)
    if den.edges:
        knots = [i for i in knots if den.y[i] or den.e[i]]
        empty = [(den.edges[i], den.edges[i + 1]) for i in range(len(den.x)) if not (den.y[i] or den.e[i])]
        inEmptyBin = [any(lo <= x < hi for lo, hi in empty) for x in num.x]
        if not knots:
            return [0.0] * len(num.x), None
    n = len(knots)
    if n < 2:
        exp.error(f"Cannot interpolate {exp.den} for the division")
        return None
    order = sorted(knots, key=lambda i: den.x[i])
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
    if any((x < lo or x > hi) and not skip for x, skip in zip(num.x, inEmptyBin)):
        exp.warn(f"points of {exp.num} are outside the range")
    values = [0.0 if skip else v for v, skip in zip(values, inEmptyBin)]
    oneSign = all(y > 0 for y in ys) or all(y < 0 for y in ys)
    if oneSign and any((v <= 0) if ys[0] > 0 else (v >= 0) for v, skip in zip(values, inEmptyBin) if not skip):
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


class DivisionExpected:
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


def division_reference(numObj, denObj, numFunc, denFunc, binomial, exp):
    """the expected ratio: a histogram, a dict like graph_values, or None (an error is expected)"""
    nf, df = division_family(numObj), division_family(denObj)
    if nf == "graph2d" or nf in ("func2", "func3"):
        exp.error(f"Cannot divide {exp.num} ({as_divided(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}) by {exp.den}")
        return None
    if nf == "func1":
        if df == "graph" or df == "hist1":
            den = x_errors(denObj, Points(as_divided(denObj) if df == "hist1" else denObj))
            keep = [i for i, x in enumerate(den.x) if numFunc.inside(x)]  # a function of the inputs within its range
            if not keep:  # nothing to sample at: the function itself cannot be divided
                exp.error(f"Cannot divide {exp.num} (TF1) by {exp.den}")
                return None
            den = subset(den, keep)
            num = Points(denObj if df == "graph" else as_divided(denObj))
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
        exp.error(f"Cannot divide {exp.num} (TF1) by {exp.den}")
        return None
    if nf.startswith("hist"):
        dim = int(nf[4:])
        num = as_divided(numObj)
        if df.startswith("func"):
            if int(df[4:]) > dim:
                exp.error(f"Cannot divide {exp.num} by the higher dimensional function")
                return None
            return divide_by_function_hist(num, denFunc, int(df[4:]), binomial, exp)
        if df == "graph":
            if dim != 1:
                exp.error(f"Cannot divide {exp.num} ({as_divided(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}) by {exp.den}")
                return None
            return graph_to_hist(num, divide_points(x_errors(num, Points(num)), x_errors(denObj, Points(denObj)), binomial, exp))
        if df == "graph2d":
            exp.error(f"Cannot divide {exp.num} ({as_divided(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}) by {exp.den}")
            return None
        den = as_divided(denObj)
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
        if num.InheritsFrom("TH2Poly"):
            return divide_polygons(num, den, binomial)
        for operand in (num, den):  # without stored errors TH1::Divide gives sqrt(ratio) as error, NaN for a negative denominator
            if not operand.GetSumw2N():
                operand.Sumw2()
        num.Divide(num, den, 1.0, 1.0, "B" if binomial else "")
        num.ClearUnderflowAndOverflow()
        return num
    # a graph numerator
    if df == "graph" or df == "hist1":
        den = x_errors(denObj, Points(as_divided(denObj) if df == "hist1" else denObj))
        num = with_errors_of(x_errors(numObj, Points(numObj)), den)
        return result_graph(divide_points(num, den, binomial, exp), num, numObj)
    if df == "func1":
        return divide_by_function_graph(numObj, denFunc, binomial, exp)
    exp.error(f"Cannot divide {exp.num} ({as_divided(numObj).ClassName() if numObj.InheritsFrom('TH1') else numObj.ClassName()}) by {exp.den}")
    return None


def divide_polygons(num, den, binomial):
    """a TH2Poly by one of the same polygons with the formulas of TH1::Divide, whose ResetStats reads past the arrays of a TH2Poly"""
    entries = den.GetEntries() if binomial else num.GetEntries()
    for b in range(1, num.GetNumberOfBins() + 1):
        b1, b2 = num.GetBinContent(b), den.GetBinContent(b)
        e1sq, e2sq = num.GetBinError(b) ** 2, den.GetBinError(b) ** 2
        if not b2:
            errsq = 0.0
        elif binomial:
            errsq = 0.0 if b1 == b2 else abs(((1 - 2 * b1 / b2) * e1sq + b1 * b1 * e2sq / (b2 * b2)) / (b2 * b2))
        else:
            errsq = (e1sq * b2 * b2 + e2sq * b1 * b1) / b2**4
        num.SetBinContent(b, b1 / b2 if b2 else 0.0)
        num.SetBinError(b, math.sqrt(errsq))
    num.SetEntries(entries)
    return num


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
        obj = obj.CreateGraph() if obj.GetDimension() == 1 else obj.GetTotalHistogram()
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

    def __init__(self, kind, variant, obj, evaluate=None):
        self.formula = "formula" in DATA_KINDS[kind][2]
        self.obj = obj if not self.formula else {1: ROOT.TF1, 2: ROOT.TF2}[DATA_KINDS[kind][0]](f"f_{kind}_{variant}", formula(kind, variant))
        self.evaluate = evaluate or (lambda *x: self.obj.Eval(*x))  # exact values of a function drawn as samples (modified)

    def inside(self, *x):
        if self.formula:
            return True
        lows = [getattr(self.obj, f"Get{a}min")() for a in "XYZ"[: len(x)]]
        highs = [getattr(self.obj, f"Get{a}max")() for a in "XYZ"[: len(x)]]
        return all(lo <= c <= hi for c, lo, hi in zip(x, lows, highs))

    def value(self, *x):
        return self.evaluate(*x) if self.inside(*x) else None

    def __call__(self, x):
        return self.evaluate(x)


def ratio_input(kind, source, variant):
    """an input of a ratio (Data) of a kind in a variant: formulas of the plot definition, projected kinds projected"""
    if "formula" in DATA_KINDS[kind][2]:
        return S.Data(formula(kind, variant), S.kUserFunctions)
    d = S.Data(kind, source)
    if "projected" in DATA_KINDS[kind][2]:
        d.Project([0] if DATA_KINDS[kind][0] == 1 else [0, 1])
    return d


def division_name(kind, variant, source):
    """the name of data in messages about the division"""
    if "formula" in DATA_KINDS[kind][2]:
        return formula(kind, variant)
    return f"{kind}:{source}" + ("_Proj{0}" if DATA_KINDS[kind][0] == 1 else "_Proj{01}") * ("projected" in DATA_KINDS[kind][2])


def match_messages(lines, expected, keyed):
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


def first_five(items):
    """the first five items for a message, and how many more"""
    return " | ".join(str(x) for x in items[:5]) + (f" (+{len(items) - 5})" if len(items) > 5 else "")


# ---------------------------------------------------------------------------------------------
# files written by the output modes
# ---------------------------------------------------------------------------------------------
FILE_MODES = ("pdf", "eps", "ps", "svg", "png", "jpg", "html", "json", "xml", "root")  # one file per plot, named <plot>.<mode>
BITMAP_MODES = ("png", "jpg")  # and gif: sized by bitmapscale


def display(own=False):
    """a graphical display for the programs the test starts: the caller's (DISPLAY), else a virtual one (Xvfb) for the rest of the test;
    own: a virtual one in any case, of 3200 x 2400 pixels (a screenshot needs a known screen); False if there is none (the checks that need
    one are left out)"""
    if os.environ.get("DISPLAY") and not own:
        return True
    if not shutil.which("Xvfb"):
        return False
    import atexit

    # a display number of its own, picked at random: with -displayfd two servers starting at the same moment can take the same number,
    # and the one that has to move on writes its number a second time and ends
    for _ in range(10):
        number = random.randrange(100, 30000)
        if os.path.exists(f"/tmp/.X{number}-lock") or os.path.exists(f"/tmp/.X11-unix/X{number}"):
            continue
        with open(os.path.join(WORK, "xvfb.log"), "a") as log:
            server = subprocess.Popen(["Xvfb", f":{number}", "-screen", "0", "3200x2400x24", "-nolisten", "tcp"], stdout=log, stderr=log)
        deadline = time.time() + 10
        while server.poll() is None and not os.path.exists(f"/tmp/.X11-unix/X{number}") and time.time() < deadline:
            time.sleep(0.1)
        if server.poll() is None and os.path.exists(f"/tmp/.X11-unix/X{number}"):
            os.environ["DISPLAY"] = f":{number}"
            atexit.register(server.terminate)
            return True
        server.terminate()
    return False


def bitmap(path):
    """the pixels of a png or jpg file (the first image of a gif) as an array of 0xAARRGGBB values, rows from the top; None if unreadable"""
    image = ROOT.TImage.Open(path)
    if not image or not image.IsValid():
        return None
    width, height = image.GetWidth(), image.GetHeight()
    argb = image.GetArgbArray()
    argb.reshape((width * height,))
    pixels = np.frombuffer(argb, dtype=np.uint32, count=width * height).reshape(height, width).copy()
    del image
    return pixels


def rgb(colour):
    """a ROOT colour index as 0xRRGGBB"""
    c = ROOT.gROOT.GetColor(colour)
    return (round(255 * c.GetRed()) << 16) | (round(255 * c.GetGreen()) << 8) | round(255 * c.GetBlue())


def gif_info(path):
    """the structure of a gif file: {"size": (width, height) of the screen, "frames": [(width, height) of each image], "delays": [centiseconds
    of each frame], "loops": None (played once), 0 (endless) or the number of repeats}; None if it is no gif"""
    with open(path, "rb") as f:
        data = f.read()
    if data[:6] not in (b"GIF87a", b"GIF89a"):
        return None
    info = {"size": (int.from_bytes(data[6:8], "little"), int.from_bytes(data[8:10], "little")), "frames": [], "delays": [], "loops": None}
    pos = 13 + (3 << ((data[10] & 7) + 1) if data[10] & 0x80 else 0)  # after the global colour table

    def skip_blocks(pos):
        while data[pos]:
            pos += data[pos] + 1
        return pos + 1

    while pos < len(data) and data[pos] != 0x3B:
        if data[pos] == 0x21:  # extension: graphic control (the delay) or application (the loop count of NETSCAPE2.0)
            label, block = data[pos + 1], data[pos + 2 : pos + 3 + data[pos + 2]]
            if label == 0xF9:
                info["delays"].append(int.from_bytes(block[2:4], "little"))
            elif label == 0xFF and block[1:12] == b"NETSCAPE2.0":
                info["loops"] = int.from_bytes(data[pos + 16 : pos + 18], "little")
            pos = skip_blocks(pos + 2)
        elif data[pos] == 0x2C:  # an image: its descriptor, local colour table, LZW data
            info["frames"].append((int.from_bytes(data[pos + 5 : pos + 7], "little"), int.from_bytes(data[pos + 7 : pos + 9], "little")))
            flags = data[pos + 9]
            pos += 10 + (3 << ((flags & 7) + 1) if flags & 0x80 else 0)
            pos = skip_blocks(pos + 1)
        else:
            return None
    return info


def canvas_tree(pad, depth=0, reservation=True):
    """the primitives of a canvas or pad and of its sub-pads in drawing order: (depth, class, name, drawing option); reservation False
    leaves out a TExec define_colors that only reserves colour indices (added where ROOT reads the canvas back: modes file, root, xml)"""
    found = []
    link = pad.GetListOfPrimitives().FirstLink()
    while link:
        obj = link.GetObject()
        onlyReservation = obj.ClassName() == "TExec" and obj.GetName() == "define_colors" and re.fullmatch(r"if \(!gROOT->GetColor\(\d+\)\) new TColor\(\d+, 1, 1, 1\);", obj.GetTitle())
        if reservation or not onlyReservation:
            found.append((depth, obj.ClassName(), obj.GetName(), str(link.GetOption())))
        if obj.InheritsFrom("TPad"):
            found += canvas_tree(obj, depth + 1, reservation)
        link = link.Next()
    return found


def read_canvas(path):
    """the canvas stored in a root, xml or json file (a key name may hold '/', which TFile::Get cannot read)"""
    if path.endswith(".json"):
        with open(path) as f:
            # ROOT writes an infinite value as 2e308 (JSROOT reads it as infinity), which its own reader refuses: read as the largest double
            text = re.sub(r"(?<![\w.])2e308\b", "1.7976931348623157e308", f.read())
        try:
            obj = ROOT.TBufferJSON.ConvertFromJSON(text)
        except Exception:  # ROOT throws for json it cannot read
            return None
        return ROOT.BindObject(obj, "TCanvas") if obj else None
    f = ROOT.TFile.Open(path)
    keys = [key for key in f.GetListOfKeys() if key.GetClassName() == "TCanvas"] if f else []
    _open_files.append(f)  # open while the canvas is used
    return keys[0].ReadObj() if len(keys) == 1 else None


_open_files = []


# ---------------------------------------------------------------------------------------------
# a project and its runs
# ---------------------------------------------------------------------------------------------
class Run:
    """the result of one run of the plot program"""

    def __init__(self, proc, plotsFile, snapshot):
        self.rc = proc.returncode
        self.log = re.sub(r"\x1b\[[0-9;]*m", "", proc.stdout + proc.stderr).replace("\r\n", "\n")
        self.file = None
        if os.path.exists(plotsFile):
            shutil.copyfile(plotsFile, snapshot)  # the next run deletes the output folder
            self.file = ROOT.TFile.Open(snapshot)

    def lines(self, kind):
        """the lines of a kind: ERR, WARN, INFO"""
        return [line for line in self.log.splitlines() if f"[ {kind}" in line or f"[{kind}" in line]

    def errors(self, text=""):
        """the errors of the run containing text: SciRooPlot's and ROOT's (Error in <...>, Fatal in <...>), and crashes"""
        marks = ("[ ERR", "[ERR", "Error in <", "Fatal in <", "*** Break ***")
        return [line for line in self.log.splitlines() if any(mark in line for mark in marks) and text in line]

    def canvas(self, name, group):
        groupDir = self.file.Get(group) if self.file else None
        if groupDir:
            for key in groupDir.GetListOfKeys():
                if key.GetName().split(":")[0] == name:
                    canvas = key.ReadObj()
                    # the colours of the plot (rgb colours, gradients) are defined when it is painted: define them as the plot did
                    define = canvas.GetListOfPrimitives().FindObject("define_colors")
                    if define:
                        define.Exec()
                    return canvas
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

    def palette(self, name, group, pad=1):
        """the palette a pad is painted with, as (r, g, b, alpha) per colour (see rgba): the pad's setup run as when it is painted; None if
        the pad has no setup (also a ColorScale() cell has one)"""
        padObj = self.pad(name, group, pad)
        setup = padObj.GetPrimitive(f"setup_pad_{pad}") if padObj else None
        if not setup:
            return None
        ROOT.gStyle.SetPalette(ROOT.kSunset)  # another palette before
        setup.Exec()
        return [rgba(ROOT.TColor.GetColorPalette(k)) for k in range(ROOT.TColor.GetNumberOfColors())]

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

    def messages(self, name, group):
        """the messages logged while a plot was made (the plots are made one after the other, each ending with "Created plot ...")"""
        if not hasattr(self, "_messages"):
            self._messages, pending = {}, []
            for line in self.log.splitlines():
                made = re.search(r"Created plot (\S+) from group (\S+)\.$", line)
                if made:
                    self._messages[made.groups()] = pending
                    pending = []
                else:
                    pending.append(line)
        return self._messages.get((name, group), [])


# ---------------------------------------------------------------------------------------------
# painting read-back canvases
# ---------------------------------------------------------------------------------------------
_PNG = os.path.join(WORK, "painted.png")


def is_data(obj):
    """whether a primitive of a pad shows data: the data (named <data number>:..., the fill of steps fill_<data number>:...), the polar grid
    of polar graphs and the shapes (lines, arrows, regions, polygons, ellipses), not the frame, axes, colour scale or boxes"""
    if re.match(r"(fill_)?\d+:", obj.GetName()) or obj.InheritsFrom("TGraphPolargram"):
        return True
    shape = any(obj.InheritsFrom(cls) for cls in ("TLine", "TBox", "TPolyLine", "TEllipse"))
    return shape and not obj.InheritsFrom("TFrame") and not obj.InheritsFrom("TPave")


def is_box(obj):
    """whether a primitive of a pad is a box: legend, text, statistics box or image"""
    return (obj.InheritsFrom("TPave") and not obj.InheritsFrom("TPaletteAxis")) or obj.GetName().startswith("ImageBox_")


def pixels(canvas, keep, pad=1):
    """Paints a canvas read from Plots.root (Run.canvas gives a fresh copy, which is deleted here) with only the primitives of the pad
    keep(obj) is true for, and returns its pixels as an array of ARGB values (rows from the top) and a function giving the pixel
    (column, row) of a point of the pad in NDC. TExec is left out: ROOT compiles its command at every paint and keeps the code (memory
    runs out over thousands of paints); it only sets colours and styles, which no comparison of two paints depends on."""
    ROOT.SetOwnership(canvas, True)
    padObj = canvas.GetPrimitive(f"Pad_{pad}")
    primitives = padObj.GetListOfPrimitives()
    for obj in list(primitives):
        if not keep(obj) or obj.InheritsFrom("TExec"):
            primitives.Remove(obj)
    level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
    canvas.Print(_PNG)
    ROOT.gErrorIgnoreLevel = level
    image = ROOT.TASImage.Open(_PNG)
    width, height = image.GetWidth(), image.GetHeight()
    argb = np.frombuffer(image.GetArgbArray(), dtype=np.uint32, count=width * height).reshape(height, width).copy()
    x0, y0, w, h = padObj.GetAbsXlowNDC(), padObj.GetAbsYlowNDC(), padObj.GetAbsWNDC(), padObj.GetAbsHNDC()
    del image, canvas
    return argb, lambda x, y: ((x0 + x * w) * width, (1.0 - (y0 + y * h)) * height)


def in_chunks(todo, compare, size=500):
    """Calls compare(index, case) for every case and checks what it returns (a list of (title, passed, detail)), size cases at a time in a
    child process each: ROOT keeps memory at every paint (about 65 kB), which runs out over the thousands of paints of a full list."""
    for start in range(0, len(todo), size):
        chunk = range(start, min(start + size, len(todo)))
        reader, writer = os.pipe()
        if (pid := os.fork()) == 0:
            os.close(reader)
            with os.fdopen(writer, "wb") as out:
                pickle.dump([result for i in chunk for result in compare(i, todo[i])], out)
            os._exit(0)
        os.close(writer)
        try:
            with os.fdopen(reader, "rb") as results:
                for title, passed, detail in pickle.load(results):
                    check(title, passed, detail)
        except EOFError:
            check(f"comparison of cases {chunk.start} to {chunk.stop - 1} ran", False, f"the process comparing them ended with status {os.waitpid(pid, 0)[1]}")
            continue
        os.waitpid(pid, 0)


def run_in_terminal(command, timeout, env=None):
    """Runs a command with its output and errors going to one terminal (a pseudo-terminal): a program writes line by line into a terminal,
    so the messages come in the order they were written (into a pipe, standard output is written in blocks, after the errors); env: its
    environment (None: this process's)."""
    master, slave = pty.openpty()
    proc = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, close_fds=True, env=env)
    os.close(slave)
    chunks, deadline = [], time.time() + timeout
    while True:
        ready, _, _ = select.select([master], [], [], max(deadline - time.time(), 0))
        if not ready:
            proc.kill()
            break
        try:
            chunk = os.read(master, 65536)
        except OSError:  # the terminal is closed once the program ended
            break
        if not chunk:
            break
        chunks.append(chunk)
    os.close(master)
    proc.wait()
    return subprocess.CompletedProcess(command, proc.returncode, b"".join(chunks).decode(errors="replace"), "")


def missing(run):
    """the data reported missing by a run, as "<data source>:<name>" (the lines below "Plot ... could not be created"; a data source
    restricted to a folder as "<data source>:<folder>/<name>")"""
    return set(re.findall(r"^\s*- missing (.+?)(?: \[[^]]*\])?(?: \([^()]*\))?\s*$", run.log, re.M))  # without the description of a request [...] and the reason (...)


class Chunks:
    """The plots of a test spread over the groups <group>/0, <group>/1, ... of CHUNK plots each, every group painted by a plot run of its
    own: a run keeps the interpreter memory of every pad it paints (ROOT runs the pad setups through its interpreter, about 0.3 MB per pad
    of a layout), so that thousands of plots in one run exhaust the memory. Read back by plot name, as from a Run."""

    CHUNK = 300

    def __init__(self, group):
        self.group = group
        self.groupOf = {}  # plot name: its group
        self.runs = {}  # group: its Run

    def add(self, name):
        """the group of the plot of that name, the next one defined"""
        self.groupOf[name] = f"{self.group}/{len(self.groupOf) // self.CHUNK}"
        return self.groupOf[name]

    def plot(self, project, mode="file"):
        for group in dict.fromkeys(self.groupOf.values()):
            self.runs[group] = project.plot(f"^{group}$", mode=mode)
        return self

    def unwritten(self):
        """the runs that wrote no file, with their exit status and the end of their output"""
        return [f"{group}: exit status {run.rc}: {run.log[-300:]}" for group, run in self.runs.items() if not run.file]

    def errors(self):
        return [line for run in self.runs.values() for line in run.errors()]

    def lines(self, kind):
        return [line for run in self.runs.values() for line in run.lines(kind)]

    def warnings(self, text=""):
        return [line for run in self.runs.values() for line in run.warnings(text)]

    def __getattr__(self, method):
        """the readers of a Run (canvas, pad, objects, frame, frame_hist, ...) by plot name, without the group: run.canvas(name)"""
        if method.startswith("_"):
            raise AttributeError(method)

        def read(name, *args, **kwargs):
            group = self.groupOf[name]
            return getattr(self.runs[group], method)(name, group, *args, **kwargs)

        return read


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

    def plot(self, group=".+", name=".+", mode="file", timeout=1800, env=None):
        """runs plot; env: variables changed for it (None as value: removed)"""
        if not self.saved:
            self.pm.SaveProject()
            self.srp("add", self.name, self.path("DefinePlots.py"), self.path("out"))
            selected = self.srp("select", self.name)
            if selected.returncode != 0:
                sys.exit(f"cannot select project {self.name}: {selected.stdout}{selected.stderr}")
            self.saved = True
        shutil.rmtree(self.path("out"), ignore_errors=True)
        environ = {k: v for k, v in dict(os.environ, **(env or {})).items() if v is not None}
        proc = run_in_terminal([os.path.join(PREFIX, "bin", "plot"), group, name, mode], timeout, environ)
        self.nRuns += 1
        return Run(proc, self.path("out", "Plots.root"), self.path(f"run{self.nRuns}_Plots.root"))


# ---------------------------------------------------------------------------------------------
# the command line: the programs srp and plot, the shell functions of env.sh
# ---------------------------------------------------------------------------------------------
SRP = os.path.join(PREFIX, "bin", "srp")
PLOT = os.path.join(PREFIX, "bin", "plot")
ENV_SCRIPT = os.path.join(PREFIX, "share", "scirooplot", "env.sh")
SHELLS = tuple(shell for shell in ("bash", "zsh") if shutil.which(shell))  # zsh is optional: its checks are left out without it


class Output:
    """the result of a command: exit status, stdout and stderr without colours and margins, text (both)"""

    def __init__(self, proc, seconds=0.0):
        self.rc = proc.returncode
        # without colours and without the margin of printed lines ("       | ")
        self.stdout = re.sub(r"(?m)^ {7}\| ", "", re.sub(r"\x1b\[[0-9;]*m", "", proc.stdout))
        self.stderr = re.sub(r"(?m)^ {7}\| ", "", re.sub(r"\x1b\[[0-9;]*m", "", proc.stderr))
        self.text = self.stdout + self.stderr
        self.seconds = seconds

    def lines(self, text=""):
        return [line for line in self.text.splitlines() if text in line]

    def errors(self):
        """SciRooPlot's errors, those of env.sh (Error: ...) and ROOT's"""
        return [line for line in self.text.splitlines() if line.startswith(("[ ERR", "Error", "Fatal")) or "Error in <" in line]

    def warnings(self):
        return [line for line in self.text.splitlines() if line.startswith(("[ WARN", "Warning"))]

    def tail(self, n=400):
        return " | ".join(self.text.strip().splitlines())[-n:]


def run(program, *args, config=None, cwd=None, env=None, timeout=600):
    """runs a program (srp, plot) with a configuration folder (default: the one of the test)"""
    environ = dict(os.environ, **(env or {}))
    if config:
        environ["SCIROOPLOT_CONFIG_PATH"] = config
    start = time.time()
    proc = subprocess.run([program, *args], capture_output=True, text=True, cwd=cwd, env=environ, stdin=subprocess.DEVNULL, timeout=timeout, check=False)
    return Output(proc, time.time() - start)


def shell(code, shell="bash", config=None, cwd=None, env=None, interactive=False, typed=False, timeout=600):
    """runs code in bash or zsh after sourcing env.sh, without the user's start-up files; interactive shells get the tab completion.
    typed: the lines are read from standard input by an interactive shell, as if typed at its prompt (aliases, PROMPT_COMMAND)"""
    home = os.path.join(WORK, f"home_{shell}")
    os.makedirs(home, exist_ok=True)
    environ = dict(os.environ, HOME=home, ZDOTDIR=home, **(env or {}))
    if config:
        environ["SCIROOPLOT_CONFIG_PATH"] = config
    options = ["--norc", "--noprofile"] if shell == "bash" else ["-f"]
    script = f'source "{ENV_SCRIPT}"\n{code}\n'
    start = time.time()
    if typed:
        proc = subprocess.run([shell, *options, "-i"], input=script + "exit\n", capture_output=True, text=True, cwd=cwd, env=environ, timeout=timeout, check=False)
    else:
        proc = subprocess.run([shell, *options, *(["-i"] if interactive else []), "-c", script], capture_output=True, text=True, cwd=cwd, env=environ, stdin=subprocess.DEVNULL, timeout=timeout, check=False)
    out = Output(proc, time.time() - start)
    # an interactive shell without a terminal complains about job control
    noise = ("job control", "terminal process group", "no job control", "can't change group", "cannot set terminal")
    out.stderr = "\n".join(line for line in out.stderr.splitlines() if not any(n in line for n in noise))
    out.text = out.stdout + out.stderr
    return out


def python(code, config=None, cwd=None, env=None, timeout=600):
    """runs Python code in a process of its own that imports SciRooPlot (definition-time messages, a configuration of its own)"""
    module = os.path.dirname(S.__file__)
    environ = {"PYTHONPATH": module + os.pathsep + os.environ.get("PYTHONPATH", ""), **(env or {})}
    return run(sys.executable, "-u", "-W", "ignore", "-c", code, config=config, cwd=cwd, env=environ, timeout=timeout)


def config_folder(name):
    """a new, empty configuration folder"""
    path = os.path.join(WORK, name)
    os.makedirs(path)
    return path


def config_files(config):
    """the contents of every file of a configuration folder by relative path (without the lock file): to see that nothing changed"""
    files = {}
    for root, _, names in os.walk(config):
        for name in names:
            if name != ".lock":
                path = os.path.join(root, name)
                with open(path, "rb") as f:
                    files[os.path.relpath(path, config)] = f.read()
    return files


def listed(output):
    """the plots plot ... list prints, as (group, name)"""
    return {(group, name) for name, group in re.findall(r"^ - (.*) in group (.*)$", output.stdout, re.M)}


# ---------------------------------------------------------------------------------------------
# the plot definition: the interface in both languages, calls written in either, the saved file
# ---------------------------------------------------------------------------------------------
SOURCE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
API_REFERENCE = os.path.join(SOURCE, "docs", "skill", "scirooplot", "reference", "api.md")  # generated from the public headers


def split_args(text):
    """the arguments of a signature or call, split at the commas outside of brackets and quotes"""
    parts, depth, current, quote = [], 0, "", None
    for ch in text:
        if quote:
            quote = None if ch == quote and not current.endswith("\\") else quote
        elif ch in "'\"":
            quote = ch
        elif ch in "<([{":
            depth += 1
        elif ch in ">)]}":
            depth -= 1
        if ch == "," and depth == 0 and not quote:
            parts.append(current.strip())
            current = ""
        else:
            current += ch
    return parts + ([current.strip()] if current.strip() else [])


PY_SPECIAL = ("__init__", "__call__", "__getitem__", "__iadd__")


def python_interface():
    """every overload of the Python bindings: (class, method, ((parameter, type, default or None), ...)), in binding order"""
    overloads = []
    for className in dir(S):
        cls = getattr(S, className)
        if not isinstance(cls, type) or className.endswith("_t"):
            continue
        for method in cls.__dict__:
            if method.startswith("_") and method not in PY_SPECIAL:
                continue
            func = getattr(cls, method)
            for line in (getattr(func, "__doc__", "") or "").splitlines():
                found = re.match(r"^(?:\d+\. )?(\w+)\((.*)\) -> (.*)$", line.strip())
                if found and found.group(1) == method:
                    params = []
                    args = split_args(found.group(2))
                    for arg in args[1:] if args and args[0].startswith("self") else args:
                        name, _, rest = arg.partition(":")
                        typ, _, default = rest.partition(" = ")
                        params.append((name.strip(), typ.strip(), default.strip() or None))
                    overloads.append((className, method, tuple(params)))
    return overloads


def cpp_interface():
    """every public method of the headers as listed in the API reference: (class, method, parameter names, C++ only, default values as
    written, None where there is none); the classes of the setter groups (marker setters, Box<BoxType>, ...) get their methods, Ratio
    those of Data"""
    sections, groups = {}, []
    with open(API_REFERENCE) as f:
        text = f.read()
    for block in re.split(r"^## ", text, flags=re.M)[1:]:
        head, *lines = block.splitlines()
        cls = head.split("::")[-1].split("<")[0].strip()
        entries = []
        for line in lines:
            found = re.match(r"^- `(?:[\w:<>,&* ]+? )?(operator\(\)|operator\[\]|operator\+=|\w+)\((.*)\)`(.*)$", line)
            if found:
                method, params, rest = found.groups()
                args = split_args(params)
                names = tuple(re.findall(r"(\w+)$", arg.split("=")[0].strip())[0] for arg in args)
                defaults = tuple(arg.partition("=")[2].strip() or None for arg in args)
                entries.append((method, names, "C++ only" in rest, defaults))
        remark = next((line for line in lines if line.startswith("*")), "")
        if head.endswith("setters") or cls == "Box":
            targets = ["TextBox", "LegendBox", "StatsBox"] if cls == "Box" else [t.strip() for t in remark.strip("*").split(";")[0].split(",")]
            groups.append((targets, entries))
        else:
            sections.setdefault(cls, []).extend(entries)
    for targets, entries in groups:
        for target in targets:
            sections.setdefault(target, []).extend(entries)
    # a Ratio has the methods of Data except Scatter(x, y, z)
    sections["Ratio"] = sections.get("Ratio", []) + [e for e in sections.get("Data", []) if e[0] not in ("Data", "AsRatio", "operator()") and e[:2] != ("Scatter", ("x", "y", "z"))]
    return [(cls, "__init__" if method == cls else method, names, only, defaults) for cls, entries in sections.items() for method, names, only, defaults in entries]


class Code:
    """an expression written differently in Python and C++ (an object of the interface, an enumeration value)"""

    def __init__(self, py, cpp=None):
        self.py, self.cpp = py, cpp if cpp is not None else py

    def __repr__(self):
        return self.py


class Char(str):
    """a single character (C++ char, Python str)"""


def literal(value, lang):
    """a value written as literal in Python (lang "py") or C++ ("cpp")"""
    if isinstance(value, Code):
        return value.py if lang == "py" else value.cpp
    if isinstance(value, Char):
        return repr(str(value)) if lang == "py" else "'" + value + "'"
    if isinstance(value, bool):
        return ("True" if value else "False") if lang == "py" else ("true" if value else "false")
    if isinstance(value, int):
        return str(value)
    if isinstance(value, float):
        if math.isinf(value):
            sign = "-" if value < 0 else ""
            return f"{sign}float('inf')" if lang == "py" else f"{sign}std::numeric_limits<double>::infinity()"
        text = repr(value)
        return text if lang == "py" or "." in text or "e" in text else text + "."
    if isinstance(value, str):
        if lang == "py":
            return repr(value)
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t") + '"'
    if isinstance(value, (tuple, list)):
        inner = ", ".join(literal(v, lang) for v in value)
        if lang == "cpp":
            return "{" + inner + "}"
        return f"({inner}{',' if len(value) == 1 else ''})" if isinstance(value, tuple) else f"[{inner}]"
    raise TypeError(f"no literal for {value!r}")


def call(method, args, lang):
    """a call of a method on the expression before it"""
    return f".{method}({', '.join(literal(a, lang) for a in args)})"


def read_info(path):
    """a file written by SavePlots as a tree: a list of (key, value, children) with the values unescaped"""
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()
    root, stack = [], []
    current = root
    for line in lines:
        text = line.strip()
        if not text:
            continue
        if text == "{":
            stack.append(current)
            current = current[-1][2]
            continue
        if text == "}":
            current = stack.pop()
            continue
        key, _, value = text.partition(" ")
        value = value.strip()
        if value.startswith('"') and value.endswith('"') and len(value) >= 2:
            value = re.sub(r'\\(.)', lambda m: {"n": "\n", "t": "\t", "0": "\0", "r": "\r", "a": "\a", "b": "\b", "f": "\f", "v": "\v"}.get(m.group(1), m.group(1)), value[1:-1])
        current.append((key, value, []))
    return root


def info_values(tree, prefix=""):
    """the values of a tree by path (a key appearing twice gets #2, #3 ...)"""
    values, seen = {}, {}
    for key, value, children in tree:
        seen[key] = seen.get(key, 0) + 1
        path = prefix + key + (f"#{seen[key]}" if seen[key] > 1 else "")
        if children:
            values.update(info_values(children, path + "/"))
        else:
            values[path] = value
    return values


def info_plots(path):
    """the plots of a saved file by name:group, each as its values by path"""
    return {key: info_values(children) for key, _, children in read_info(path)}


def captured(func):
    """runs func and returns (its result or the exception, everything printed meanwhile, also by C++ code)"""
    sys.stdout.flush()
    sys.stderr.flush()
    with __import__("tempfile").TemporaryFile() as tmp:
        saved = os.dup(1), os.dup(2)
        os.dup2(tmp.fileno(), 1)
        os.dup2(tmp.fileno(), 2)
        try:
            result = func()
        except Exception as ex:  # noqa: BLE001  the caller checks what was raised
            result = ex
        finally:
            sys.stdout.flush()
            sys.stderr.flush()
            os.dup2(saved[0], 1)
            os.dup2(saved[1], 2)
            os.close(saved[0])
            os.close(saved[1])
        tmp.seek(0)
        text = re.sub(r"\x1b\[[0-9;]*m", "", tmp.read().decode(errors="replace"))
    return result, text


# The test values of the parameters of the interface, by parameter name (or Class.method.parameter, method.parameter, which win), and
# by variant: plain (a value that differs from the default), precise (needs every digit: 1/3), extreme (at the ends of the range of
# double), text (a text with every character the file format has to escape or could misread), empty (an empty text), false.
HOSTILE_TEXT = '  a "b" \\c $d; e, f {g} #h µ² \ttab\nline:end  '
TEXT = {"plain": "some text", "text": HOSTILE_TEXT, "empty": ""}
FLAG = {"plain": True, "false": False}
COLOR = {"plain": 633, "precise": (1 / 3, 0.5, 0.25), "text": "#1a2b3c"}
SETTER_VALUES = {
    "alias": {"plain": "j"},
    "align": {"plain": 12},
    "alpha": {"plain": 0.37, "precise": 1 / 3},
    "angle": {"plain": 30.0, "precise": 100 / 3},
    "axis": {"plain": Char("Y")},
    "basePlot": {"plain": "2d"},
    "bottom": {"plain": 0.13, "precise": 1 / 7},
    "top": {"plain": 0.05, "precise": 1 / 11},
    "left": {"plain": 0.11, "precise": 1 / 9},
    "right": {"plain": 0.07, "precise": 1 / 13},
    "center": FLAG,
    "AddEllipse.center": {"plain": (5.0, 0.5), "precise": (1 / 3, 2 / 3), "extreme": (-1e300, 1e-300)},
    "color": COLOR,
    "colors": {"plain": [633, "#00ff00", (0.0, 0.0, 1.0)]},
    "columns": {"plain": 2},
    "Span.rows": {"plain": 1},
    "common": {"plain": False},  # true by default
    "contours": {"plain": [1.0, 2.5, 7.0], "precise": [1 / 3, 2 / 3, 1.0]},
    "correlated": FLAG,
    "data": {"plain": Code('Data("h2", "s2")')},
    "numerator": {"plain": Code('Data("h", "s")')},
    "denominator": {"plain": Code('Data("h2", "s2")')},
    "dataDims": {"plain": ["x", ("y", 20), ("z", 10, (0.0, 5.0))], "precise": [("x", (1 / 3, 0.5, 4.0)), ("y", 7, (-1e300, 1e300))]},
    "dims": {"plain": [2, 0]},
    "ranges": {"plain": [(0, 2.0, 7.0)], "precise": [(1, 1 / 3, 2 / 3)]},
    "dataID": {"plain": 2},
    "dataIDs": {"plain": [1, 2]},
    "dataLayout": {"plain": Code("Data().SetMarkerStyle(33).SetLineWidth(3.)")},
    "dataSource": {"plain": "s2"},
    "numeratorName": {"plain": "h"},
    "numeratorDataSource": {"plain": "s"},
    "denominatorName": {"plain": "h2"},
    "denominatorDataSource": {"plain": "s2"},
    "define": FLAG,
    "divideBinWidth": FLAG,
    "drawOption": {"plain": "e1 x0", "text": "  hist e  "},
    "optionAlias": {"plain": Code("points_line")},
    "startX": {"plain": 2.5, "precise": 1 / 3},
    "endX": {"plain": 7.5, "precise": 22 / 3},
    "startY": {"plain": 1.5},
    "endY": {"plain": 8.5},
    "entryMin": {"plain": 10},
    "entryMax": {"plain": 50, "extreme": 4294967295},
    "nEntries": {"plain": 1000, "extreme": 4294967295},
    "filter": {"plain": "x > 2 && y < 5", "text": 'std::string("$\\\\;") == "$\\\\;" && x > 1'},
    "fixAspectRatio": FLAG,
    "font": {"plain": 62},
    "format": {"plain": "emr"},
    "forward": {"plain": False},
    "frameFraction": {"plain": 0.15, "precise": 1 / 3},
    "function": {"plain": "2*x + 1", "text": "TMath::Gaus(x, 5, 1.5) * (x > 1 ? 1 : 0.5)"},
    "gradient": {"plain": [(633, 0.0), ("#00ff00", 0.5), ((0.0, 0.0, 1.0), 1.0)], "precise": [(633, 0.0), (600, 1 / 3), (416, 1.0)]},
    "rgbEndpoints": {"plain": [(0.0, 1.0, 0.0, 0.0), (1.0, 0.0, 0.0, 1.0)], "precise": [(0.0, 1 / 3, 0.0, 0.0), (1.0, 0.0, 2 / 3, 1.0)]},
    "nColors": {"plain": 7},
    "group": {"plain": "g/sub"},
    "subgroup": {"plain": "sub"},
    "height": {"plain": 0.25, "precise": 1 / 3},
    "Plot.SetDimensions.height": {"plain": 600},
    "Plot.SetHeight.height": {"plain": 600},
    "BasePlotShape.SetRatioHeight.height": {"plain": 0.5, "precise": 1 / 3},
    "BasePlotShape.SetPullHeight.height": {"plain": 0.5, "precise": 1 / 3},
    "width": {"plain": 3.0},
    "Plot.SetDimensions.width": {"plain": 800},
    "Plot.SetWidth.width": {"plain": 800},
    "ImageBox.SetWidth.width": {"plain": 0.25, "precise": 1 / 3},
    "SetSize.width": {"plain": 0.3, "precise": 1 / 3},
    "Axis.SetColorScaleWidth.width": {"plain": 0.04, "precise": 1 / 30},
    "hide": FLAG,
    "isDensity": FLAG,
    "isGrid": FLAG,
    "isLog": FLAG,
    "isNoExponent": FLAG,
    "isOppositeTicks": FLAG,
    "isUserCoord": FLAG,
    "keep": FLAG,
    "redraw": FLAG,
    "showOverflowBins": FLAG,
    "topDown": FLAG,
    "key": {"plain": "xx"},
    "value": {"plain": "x*2", "text": 'std::string("a;b$") == "a;b$" ? x : 0.'},
    "keys": {"plain": ["k1", "k2"]},
    "label": TEXT,
    "legendLabel": TEXT,
    "text": TEXT,
    "title": TEXT,
    "lines": {"plain": ["first", "second"], "text": [HOSTILE_TEXT, ""]},
    "legendEntry": {"plain": Code("LegendEntry().SetMarkerStyle(33).SetLineWidth(3.)")},
    "legendID": {"plain": 2},
    "lineSpacing": {"plain": 0.8},
    "margin": {"plain": 0.3},
    "min": {"plain": 2.5, "precise": 1 / 3, "extreme": -1e300},
    "max": {"plain": 7.5, "precise": 22 / 3, "extreme": 1e300},
    "maxDigits": {"plain": 3},
    "numColumns": {"plain": 2},
    "numDivisions": {"plain": 506, "extreme": -510},
    "nContours": {"plain": 12},
    "nGroup": {"plain": 2},
    "nGroupX": {"plain": 2},
    "nGroupY": {"plain": 3},
    "nGroupZ": {"plain": 4},
    "nIterSmooth": {"plain": 2},
    "nx": {"plain": 250},
    "ny": {"plain": 120},
    "name": {"plain": "h2"},
    "Plot.SetName.name": {"plain": "{name}_renamed"},
    "offset": {"plain": 0.05, "precise": 1 / 30},
    "padID": {"plain": 1},
    "pads": {"plain": [1, 2]},
    "palette": {"plain": 112},
    "phi": {"plain": 45.0},
    "theta": {"plain": 25.0, "precise": 100 / 3},
    "phiMin": {"plain": 30.0},
    "phiMax": {"plain": 300.0},
    "pixel": {"plain": 50.0, "precise": 100 / 3},
    "placement": {"plain": Code("top_left")},
    "coordinates": {"plain": Code("frame_coordinates")},
    "points": {"plain": [(1.0, 2.0), (3.0, 4.0), (5.0, 1.0)], "extreme": [(-1e300, 1e-300), (1e300, 1.0), (0.0, -1e300)]},
    "positions": {"plain": [(1.0, 2.0), (3.0, 4.0)], "precise": [(1 / 3, 2 / 3), (4.0, 1 / 7)]},
    "pos1": {"plain": (1.0, 2.0), "extreme": (-1e300, 1e-300)},
    "pos2": {"plain": (3.0, 4.0), "extreme": (1e300, -1e-300)},
    "profile": {"plain": "y"},
    "radiusX": {"plain": 2.0},
    "radiusY": {"plain": 0.3, "precise": 1 / 3},
    "range": {"plain": 0.5},
    "ratio": {"plain": 1.25},
    "AddRatio.ratio": {"plain": Code('Ratio(Data("h", "s"), Data("h2", "s2"))')},
    "path": {"plain": "/srptest/logo.png"},
    "Span.columns": {"plain": 2},
    "refDataID": {"plain": 1},
    "refFunc": {"plain": "1"},
    "scale": {"plain": 0.8},
    "scaleFactor": {"plain": 2.5, "precise": 1 / 3, "extreme": 1e-300},
    "size": {"plain": 1.3, "precise": 1 / 3},
    "spacing": {"plain": 0.1},
    "statsBox": {"plain": Code("StatsBox().SetPosition(0.3, 0.4)")},
    "style": {"plain": 2},
    "styles": {"plain": [2, 7]},
    "symbolElements": {"plain": "lp"},
    "textFormat": {"plain": ".3g", "text": '.2f "a" \\c $d; {g} µ²'},
    "tickLength": {"plain": 0.04},
    "tickOrientation": {"plain": "+-"},
    "timeFormat": {"plain": "%d.%m.%y %H:%M"},
    "tree": {"plain": "t2", "text": ["t2", "s2"]},
    "weight": {"plain": "w"},
    "x": {"plain": 0.3, "precise": 1 / 3},
    "y": {"plain": 0.6, "precise": 2 / 3},
    "AddVerticalLine.x": {"plain": 4.5, "extreme": 1e300},
    "AddHorizontalLine.y": {"plain": 0.5, "extreme": -1e300},
    "AddPoints.x": {"plain": [1.0, 2.0, 3.0], "precise": [1 / 3, 2 / 3, 1.0]},
    "AddPoints.y": {"plain": [3.0, 1.0, 2.0], "extreme": [1e300, -1e300, 1e-300]},
    "x1": {"plain": 2.0},
    "x2": {"plain": 4.0},
    "y1": {"plain": 0.2},
    "y2": {"plain": 0.6},
    "xPos": {"plain": 0.3, "precise": 1 / 3},
    "yPos": {"plain": 0.7},
    "xlow": {"plain": 0.1},
    "ylow": {"plain": 0.2},
    "xup": {"plain": 0.9},
    "yup": {"plain": 0.95},
}
# the setters of styles take the style of what they set
for _part, _style, _styles in (("Marker", 33, [33, 21]), ("Line", 7, [7, 2]), ("Fill", 3004, [3004, 3005]), ("Border", 2, [2, 3])):
    for _method in (f"Set{_part}", f"Set{_part}Style", f"SetDefault{_part}Style", f"SetFrame{_part}", f"SetFrame{_part}Style"):
        SETTER_VALUES[f"{_method}.style"] = {"plain": _style}
    SETTER_VALUES[f"SetDefault{_part}Styles.styles"] = {"plain": _styles}
SETTER_VALUES["SetArrowHead.style"] = {"plain": "<|>"}
# the parameters of the data selections are column names and expressions
for _method in ("Project1D", "Project2D", "Profile1D", "Profile2D"):
    SETTER_VALUES[f"{_method}.x"] = {"plain": "x", "precise": ("x", 7, (1 / 3, 22 / 3))}
    SETTER_VALUES[f"{_method}.y"] = {"plain": ("y", 20), "precise": ("y", (1 / 3, 0.5, 4.0))}
for _param in ("x", "y", "z", "xErr", "yErr", "xErrLow", "xErrHigh", "yErrLow", "yErrHigh"):
    SETTER_VALUES[f"Scatter.{_param}"] = {"plain": _param, "text": f'{_param} * (std::string("$;") == "$;")'}
SETTER_VARIANTS = ("plain", "precise", "extreme", "text", "empty", "false")


def setter_value(cls, method, param, variant):
    """the test value of a parameter in a variant, None if it has none (the plain value if only the variant is missing)"""
    for key in (f"{cls}.{method}.{param}", f"{method}.{param}", param):
        if key in SETTER_VALUES:
            values = SETTER_VALUES[key]
            return values.get(variant, values.get("plain"))
    return None


# Where each class of the interface is set: statements written in both languages, with {name} (the plot) and {call} (the call).
# The plot statements are put between those making the plot p and adding it.
def _in_plot(expr_py, expr_cpp=None):
    return (f'p = Plot("{{name}}", "g", "1d")\n{expr_py}{{call}}\npm.AddPlot(p)', f'{{\n  Plot p("{{name}}", "g", "1d");\n  {expr_cpp or expr_py}{{call}};\n  pm.AddPlot(p);\n}}')


def _in_base(expr_py, expr_cpp=None):
    return (f'p = Plot("{{name}}")\n{expr_py}{{call}}\npm.AddBasePlot(p)', f'{{\n  Plot p("{{name}}");\n  {expr_cpp or expr_py}{{call}};\n  pm.AddBasePlot(p);\n}}')


SETTER_HOSTS = {
    "Plot": {"plot": _in_plot("p"), "base": _in_base("p")},
    "Pad": {"pad": _in_plot("p[1]"), "pad0": _in_plot("p[0]"), "base_pad": _in_base("p[1]")},
    "Axis": {axis: _in_plot(f'p[{pad}]["{name}"]', f"p[{pad}]['{name}']") for axis, pad, name in (("x", 1, "X"), ("y", 1, "Y"), ("z", 1, "Z"), ("c", 1, "C"), ("pad0_x", 0, "X"))},
    "Data": {"data": _in_plot('p[1].AddData("h", "s")'), "tree": _in_plot('p[1].AddData("t", "s").Project1D("x")')},
    "Ratio": {
        **{side: _in_plot(f'p[1].AddRatio("h", "s", "h2", "s2"){sel}') for side, sel in (("ratio", ""), ("numer", ".Numer()"), ("denom", ".Denom()"), ("both", ".Both()"), ("result", ".Result()"))},
        **{f"tree_{side}": _in_plot(f'p[1].AddRatio("t", "s", "t2", "s2").Both().Project1D("x").{sel}()') for side, sel in (("numer", "Numer"), ("denom", "Denom"), ("both", "Both"))},
    },
    "LegendEntry": {"data_row": _in_plot('p[1].AddData("h", "s").GetLegendEntry()'), "added_row": _in_plot('p[1].AddLegend().AddEntry("row")')},
    "LegendBox": {"legend": _in_plot("p[1].AddLegend()")},
    "TextBox": {"text": _in_plot('p[1].AddText(0.2, 0.3, "t")')},
    "StatsBox": {"stats": ('p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s").SetStatsBox(StatsBox(){call})\npm.AddPlot(p)', '{\n  Plot p("{name}", "g", "1d");\n  p[1].AddData("h", "s").SetStatsBox(StatsBox(){call});\n  pm.AddPlot(p);\n}')},
    "ImageBox": {"image": _in_plot('p[1].AddImage("/srptest/logo.png")')},
    "Shape": {
        "line": _in_plot("p[1].AddLine((1., 2.), (3., 4.))", "p[1].AddLine({1., 2.}, {3., 4.})"),
        "arrow": _in_plot("p[1].AddArrow((1., 2.), (3., 4.))", "p[1].AddArrow({1., 2.}, {3., 4.})"),
        "ellipse": _in_plot("p[1].AddEllipse((5., 0.5), 2., 0.3)", "p[1].AddEllipse({5., 0.5}, 2., 0.3)"),
        "region": _in_plot("p[1].AddRegionX(2., 4.)"),
    },
    "PanelLayout": {"layout": ('pm.AddBasePlot(Plot("{name}", PanelLayout([[Panel(), Panel().ColorScale()], [Panel(), Panel()]]){call}))', '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout({{Panel(), Panel().ColorScale()}, {Panel(), Panel()}}){call}));\n}')},
    "Panel": {
        "panel": ('pm.AddBasePlot(Plot("{name}", PanelLayout([[Panel(), Panel()], [Panel(), Panel(){call}]])))', '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout({{Panel(), Panel()}, {Panel(), Panel(){call}}})));\n}'),
        "strip": ('pm.AddBasePlot(Plot("{name}", PanelLayout([[Empty(2., 0.25){call}], [Panel(), Panel()]])))', '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout({{Empty(2., 0.25){call}}, {Panel(), Panel()}})));\n}'),
    },
    "BasePlotShape": {
        "shape": (
            'p = PlotManager.MakeBasePlot("1d_wide_ratio_pull", PanelLayout(), BasePlotShape(){call})\np.SetName("{name}")\npm.AddBasePlot(p)',
            '{\n  Plot p = PlotManager::MakeBasePlot("1d_wide_ratio_pull", PanelLayout(), BasePlotShape(){call});\n  p.SetName("{name}");\n  pm.AddBasePlot(p);\n}',
        )
    },
}
# The hosts each call is meant for (the others are misplaced: the call must be stored there too, or reported)
SELECTIONS = ("Project", "ProjectX", "ProjectY", "Profile", "ProfileX", "ProfileY", "Project1D", "Project2D", "Profile1D", "Profile2D", "Scatter")
TREE_SELECTIONS = ("Filter", "Define", "Join", "Entries")
PAD_ADDERS = ("AddData", "AddFunction", "AddPoints", "AddRatio", "SetFrame")


def setter_meant_for(cls, method, host):
    """the call is meant for the host (not misplaced)"""
    if cls == "Shape" and method in ("SetArrowHead", "SetArrowSize"):
        return host == "arrow"
    if cls == "Shape" and method in ("SetArc", "SetRotation"):
        return host == "ellipse"
    if cls == "Pad" and method.startswith("Add") or method == "SetFrame":
        return host != "pad0" and not (host == "base_pad" and method in PAD_ADDERS)
    if cls == "Plot" and method in ("SetGroup", "AppendGroup"):
        return host != "base"
    if cls == "Panel" and method == "Span":
        return host == "strip"
    if cls == "Data" and method in SELECTIONS + TREE_SELECTIONS:
        return host == ("tree" if method in TREE_SELECTIONS else "data")
    if cls == "Ratio" and method in SELECTIONS + TREE_SELECTIONS:
        return host in (("tree_numer", "tree_denom", "tree_both") if method in TREE_SELECTIONS else ("numer", "denom", "both"))
    return not (cls == "Data" and host == "tree") and not (cls == "Ratio" and host.startswith("tree_")) and not (cls == "Panel" and host == "strip")


# methods that give access to parts of the definition (their results are the hosts) or select what later calls change: no case of their own
SETTER_ACCESS = {
    ("Plot", "__getitem__"),
    ("Plot", "GetPad"),
    ("Pad", "__getitem__"),
    ("Pad", "__call__"),
    ("Pad", "GetAxis"),
    ("Pad", "GetData"),
    ("Pad", "GetText"),
    ("Pad", "GetLegend"),
    ("Pad", "GetShape"),
    ("Pad", "GetImage"),
    ("Data", "GetLegendEntry"),
    ("Ratio", "GetLegendEntry"),
    ("Data", "GetDataSource"),
    ("Data", "AsRatio"),
    ("Ratio", "Numer"),
    ("Ratio", "Denom"),
    ("Ratio", "Both"),
    ("Ratio", "Result"),
}
# whole statements for the constructors and operators (Python, C++; None: not in that language)
SETTER_STATEMENTS = {
    ("Plot", "__init__", ("name", "group", "basePlot")): ('pm.AddPlot(Plot("{name}", "g/x", "2d"))', '{\n  pm.AddPlot(Plot("{name}", "g/x", "2d"));\n}'),
    ("Plot", "__init__", ("name", "layout")): ('pm.AddBasePlot(Plot("{name}", PanelLayout([[Panel(), Panel(0.5, 1.)]])))', '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout({{Panel(), Panel(0.5, 1.)}})));\n}'),
    ("Plot", "__init__", ("other", "name", "group")): ('q = Plot("{name}_source", "g", "1d")\nq[1].AddData("h", "s").SetColor(633)\npm.AddPlot(Plot(q, "{name}", "g2"))', '{\n  Plot q("{name}_source", "g", "1d");\n  q[1].AddData("h", "s").SetColor(633);\n  pm.AddPlot(Plot(q, "{name}", "g2"));\n}'),
    ("Plot", "__iadd__", ("arg0",)): (
        'p = Plot("{name}_base", "g", "1d")\np[1].AddData("h", "s").SetColor(633)\nq = Plot("{name}", "g", "1d")\nq[1].AddData("h2", "s").SetColor(600)\nq[1].AddText(0.2, 0.3, "t")\np += q\npm.AddPlot(p)',
        '{\n  Plot p("{name}_base", "g", "1d");\n  p[1].AddData("h", "s").SetColor(633);\n  Plot q("{name}", "g", "1d");\n  q[1].AddData("h2", "s").SetColor(600);\n  q[1].AddText(0.2, 0.3, "t");\n  p += q;\n  pm.AddPlot(p);\n}',
    ),
    ("Data", "__init__", ()): ('p = Plot("{name}", "g", "1d")\np[1].AddData("h2", "s2").SetLayout(Data().SetColor(633))\npm.AddPlot(p)', '{\n  Plot p("{name}", "g", "1d");\n  p[1].AddData("h2", "s2").SetLayout(Data().SetColor(633));\n  pm.AddPlot(p);\n}'),
    ("Ratio", "__init__", ("numerator", "denominator")): ('p = Plot("{name}", "g", "1d")\np[1].AddRatio(Ratio(Data("h", "s"), Data("h2", "s2")))\npm.AddPlot(p)', '{\n  Plot p("{name}", "g", "1d");\n  p[1].AddRatio(Ratio(Data("h", "s"), Data("h2", "s2")));\n  pm.AddPlot(p);\n}'),
    ("Ratio", "__init__", ("numeratorName", "numeratorDataSource", "denominatorName", "denominatorDataSource", "label")): (
        'p = Plot("{name}", "g", "1d")\np[1].AddRatio(Ratio("h", "s", "h2", "s2", "lab"))\npm.AddPlot(p)',
        '{\n  Plot p("{name}", "g", "1d");\n  p[1].AddRatio(Ratio("h", "s", "h2", "s2", "lab"));\n  pm.AddPlot(p);\n}',
    ),
    ("Data", "__init__", ("name", "dataSource", "label")): ('p = Plot("{name}", "g", "1d")\np[1].AddData(Data("h2", "s2", "lab"))\npm.AddPlot(p)', '{\n  Plot p("{name}", "g", "1d");\n  p[1].AddData(Data("h2", "s2", "lab"));\n  pm.AddPlot(p);\n}'),
    ("Data", "__init__", ("nameAndSource",)): ('p = Plot("{name}", "g", "1d")\np[1].AddData(["h2", "s2"])\npm.AddPlot(p)', None),
    ("Data", "__call__", ("name", "dataSource")): ('p = Plot("{name}", "g", "1d")\np[1].AddData(Data().SetColor(633)("h2", "s2"))\npm.AddPlot(p)', '{\n  Plot p("{name}", "g", "1d");\n  p[1].AddData(Data().SetColor(633)("h2", "s2"));\n  pm.AddPlot(p);\n}'),
    ("Panel", "__init__", ("width", "height")): ('pm.AddBasePlot(Plot("{name}", PanelLayout([[Panel(1.5, 1.), Panel()]])))', '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout({{Panel(1.5, 1.), Panel()}})));\n}'),
    ("PanelLayout", "__init__", ("rows",)): (
        'pm.AddBasePlot(Plot("{name}", PanelLayout([[Panel(), Gap(), Panel(), ColorScale()], [Panel(), Gap(), Hole(), Hole()]])))',
        '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout({{Panel(), Gap(), Panel(), ColorScale()}, {Panel(), Gap(), Hole(), Hole()}})));\n}',
    ),
    ("PanelLayout", "__init__", ("sizes", "rows")): ('pm.AddBasePlot(Plot("{name}", PanelLayout(PanelLayout().SetTextSize(40.), [[Panel(), Panel()]])))', '{\n  pm.AddBasePlot(Plot("{name}", PanelLayout(PanelLayout().SetTextSize(40.), {{Panel(), Panel()}})));\n}'),
}
# constructors without arguments make the hosts (LegendEntry(), StatsBox(), BasePlotShape(), PanelLayout() ...); PlotManager is the project
SETTER_NOT_DEFINITION = {("LegendEntry", "__init__", ()), ("StatsBox", "__init__", ()), ("BasePlotShape", "__init__", ())}


def setter_cases():
    """The calls of the plot definition: every overload of the interface (except the PlotManager's) on each of its hosts, in each
    variant of its values. Returns (cases, problems): a case is a dict with cls, method, params, host, variant, py, cpp (statements with
    {name}), unset (a call that clears a value: the plot must be the same as without any call); problems lists overloads without values."""
    found, problems = [], []
    overloads = python_interface()
    for cls, method, params in overloads:
        names = tuple(p[0] for p in params)
        if cls == "PlotManager" or (cls, method) in SETTER_ACCESS or (cls, method, names) in SETTER_NOT_DEFINITION or method == "Print":
            continue
        if (cls, method, names) in SETTER_STATEMENTS:
            py, cpp = SETTER_STATEMENTS[(cls, method, names)]
            found.append({"cls": cls, "method": method, "params": names, "host": "statement", "variant": "plain", "py": py, "cpp": cpp, "unset": False, "meant": True})
            continue
        if method in PY_SPECIAL or cls not in SETTER_HOSTS:
            problems.append(f"{cls}.{method}({', '.join(names)}): no statement and no host for it")
            continue
        # a setter without arguments next to one with arguments clears the value: set it first
        unset = not params and method.startswith("Set") and any(c == cls and m == method and p for c, m, p in overloads)
        first = next((p for c, m, p in overloads if c == cls and m == method and p), ()) if unset else None
        for host, (py, cpp) in SETTER_HOSTS[cls].items():
            meant = setter_meant_for(cls, method, host)
            if not meant and (cls, host) in (("Data", "tree"), ("Panel", "strip")) or not meant and host.startswith("tree_"):
                continue  # hosts only made for some calls
            for variant in SETTER_VARIANTS if meant else ("plain",):
                values = [setter_value(cls, method, p[0], variant) for p in (first if unset else params)]
                if None in values:
                    if variant == "plain":
                        missing = [p[0] for p, v in zip(first if unset else params, values) if v is None]
                        problems.append(f"{cls}.{method}({', '.join(names)}): no test value for {', '.join(missing)}")
                    break
                if variant != "plain" and values == [setter_value(cls, method, p[0], "plain") for p in (first if unset else params)]:
                    continue
                calls = {lang: (call(method, values, lang) if not unset else call(method, values, lang) + call(method, [], lang)) for lang in ("py", "cpp")}
                found.append({"cls": cls, "method": method, "params": names, "host": host, "variant": variant, "values": values, "unset": unset, "meant": meant, "py": py.replace("{call}", calls["py"]), "cpp": cpp.replace("{call}", calls["cpp"])})
    return found, problems


def cpp_program(folder, source, name="definePlots"):
    """builds a C++ program of the plot definition with the CMake file of the templates (ROOT found through CMAKE_PREFIX_PATH);
    returns (the program or None, the output of the build, the compile command of the source and its folder)"""
    import json
    import shlex

    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, "DefinePlots.cpp"), "w") as f:
        f.write(source)
    with open(os.path.join(PREFIX, "share", "scirooplot", "templates", "CMakeLists.txt")) as f:
        cmake = f.read().replace("PROJECT_NAME", name).replace("definePlots", name)
    with open(os.path.join(folder, "CMakeLists.txt"), "w") as f:
        f.write(cmake)
    build = os.path.join(folder, "build")
    out = run("bash", "-c", f'cmake -S "{folder}" -B "{build}" -DSciRooPlot_DIR="{os.path.abspath(PREFIX)}" -Wno-dev >/dev/null && cmake --build "{build}" -j2', timeout=1800)
    program = os.path.join(build, name)
    command, directory = [], build
    commands = os.path.join(build, "compile_commands.json")
    if os.path.exists(commands):
        with open(commands) as f:
            entry = next((e for e in json.load(f) if e["file"].endswith("DefinePlots.cpp")), None)
        if entry:
            command, directory = entry.get("arguments") or shlex.split(entry["command"]), entry["directory"]
    return (program if out.rc == 0 and os.path.exists(program) else None), out, (command, directory)


def syntax_check(compiler, command):
    """compiles a source again with another compiler (the compile command of cpp_program), only checking it, with -Wall -Wextra"""
    args, directory = command
    flags, drop = [], False
    for arg in args[1:]:
        if drop:
            drop = False
        elif arg in ("-o", "-MF", "-MT", "-MQ"):
            drop = True
        elif arg not in ("-c", "-MD") and not arg.startswith("-Wno-maybe-uninitialized"):
            flags.append(arg)
    return run(compiler, "-fsyntax-only", "-Wall", "-Wextra", *flags, cwd=directory, timeout=1800)
