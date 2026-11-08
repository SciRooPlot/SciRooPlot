"""Every request of a tree or table (Project1D with fixed bins, edges, automatic range and bins, Project2D, Project in 3d and 4d, Profile1D,
Profile2D, Scatter of 2, 3, 4 and 6 columns) gives what the same rows give when filled directly, for every input format (a tree in a file,
chained over files and over folders of a file, in a list handed over in memory, a table() of numpy arrays, a CSV file, an RNTuple in one
file and chained over two), every column type a user may have (double, float, signed and unsigned integers of 8 to 64 bits, bool, a
collection; values that are not finite) and every selection (Filter, two filters, a filter comparing a string or C-string column, Define
of a new column and of an existing one, an expression and a function body as axis, the first entries and an entry range, weights of
floating point, integer and bool type and an expression). The automatic range takes the finite values selected (warning about the others),
100 bins or the number given; integer values get bins of whole-number width centred on the integers. Requests that differ only slightly
(ranges in the last digits, bins, filters, defines of the same name) give their own results, requests that differ only in whitespace or
in the order of their filters give the same. C strings also as members of a leaf list are strings, an array of Char_t stays numbers.
Weighted profiles read in parallel, where some inputs hold weights of 1 only, have the errors of the profile filled directly. An invalid request (a column that is a string, unknown columns, an expression throwing while
the rows are read, a scatter in 3d of a collection, a profile with too many axes, a tree without request) loses only its own plot, with
the reason; the others of the same tree are read as usual. The reading summary names the entries, the inputs a tree is chained from and
the passes over the data (at most two per group of requests: the automatic ranges, then the results).
Covers: typed requests: the column types, 'return' in identifiers, function bodies, C-string columns in filters,
Scatter in 3d, the weighted profiles merged by ROOT (TProfile::Merge, ROOT's merge of profiles) and the axis titles of function bodies
(semicolons), the automatic ranges of columns with inf and nan, a throwing expression loses only its
request.
"""

import math
import os
import re

import numpy as np
from srptest import (
    FULL,
    ROOT,
    Chunks,
    CStrings,
    Project,
    S,
    Texts,
    Vectors,
    cases,
    check,
    describe,
    drawn_diff,
    finish,
    graph_diff,
    write_csv,
    write_tree,
)

p = Project("requests")
rng = np.random.default_rng(23)
N = 1200
x = rng.normal(size=N)
xnf = x.copy()
xnf[[5, 9]] = [math.inf, -math.inf]
xnf[[7, 11, 13]] = math.nan
WORDS = ("alpha", "b", "gamma ray")
COLUMNS = {
    "x": x,
    "xf": rng.normal(0.0, 2.0, N).astype(np.float32),
    "xnf": xnf,
    "i8": rng.integers(-100, 100, N).astype(np.int8),
    "u8": rng.integers(0, 250, N).astype(np.uint8),
    "i16": rng.integers(-30000, 30000, N).astype(np.int16),
    "u16": rng.integers(0, 65000, N).astype(np.uint16),
    "i32": rng.integers(-1_000_000_000, 1_000_000_000, N).astype(np.int32),  # doubled and incremented by Define without overflow
    "u32": rng.integers(0, 2_100_000_000, N).astype(np.uint32),
    "i64": rng.integers(-(2**40), 2**40, N).astype(np.int64),
    "u64": rng.integers(0, 2**50, N).astype(np.uint64),
    "b": rng.random(N) > 0.4,
    "w": rng.uniform(0.5, 2.0, N),
    "wi": rng.integers(1, 4, N).astype(np.int32),
    "y": rng.uniform(-1.0, 1.0, N),
    "z": rng.normal(size=N),
    "ex": rng.uniform(0.01, 0.3, N),
    "ex2": rng.uniform(0.01, 0.3, N),
    "ey": rng.uniform(0.01, 0.3, N),
    "ey2": rng.uniform(0.01, 0.3, N),
    "s": Texts(rng.choice(WORDS, N)),
}
COLUMNS["cstr"] = CStrings(COLUMNS["s"])
COLUMNS["vec"] = Vectors([[v, 2 * v][: k % 3] for k, v in enumerate(x)])
SCALARS = ("x", "xf", "xnf", "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "b")
INTEGERS = ("i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "b")
FLOAT32 = ("xf", "vec")  # computed in single precision by C++ expressions

# the formats: data source, data name, columns it has (a CSV file and a table() have no collections, only trees C strings)
FORMATS = {
    "tree": ("tree", "t", (*SCALARS, "vec", "s", "cstr")),
    "chain": ("chain", "t", (*SCALARS, "vec", "s", "cstr")),
    "folders": ("folders", "t", (*SCALARS, "vec", "s", "cstr")),
    "memory": ("memory", "lst/t", (*SCALARS, "vec", "s", "cstr")),
    "table": ("table", "t", (*SCALARS, "s")),
    "csv": ("csv", "t", (*SCALARS, "s")),
    "rntuple": ("rntuple", "t", (*SCALARS, "vec", "s")),
    "rntuple_chain": ("rntuple_chain", "t", (*SCALARS, "vec", "s")),
}
SPLITS = ((0, 400), (400, 700), (700, N))  # the rows of each file or folder of a chain

write_tree(p.path("tree.root"), COLUMNS)
write_tree(p.path("tree.root"), {"x": COLUMNS["x"] * 2}, folder="dir")  # the same tree name in a folder
os.makedirs(p.path("chain"))
os.makedirs(p.path("rn"))
os.makedirs(p.path("csv"))
for i, (a, b) in enumerate(SPLITS):
    write_tree(p.path("chain", f"part{i}.root"), COLUMNS, rows=slice(a, b))
    write_tree(p.path("folders.root"), COLUMNS, folder=f"DF_{i + 1}", rows=slice(a, b))
NTUPLE_COLUMNS = {k: v for k, v in COLUMNS.items() if k != "cstr"}
write_tree(p.path("rntuple.root"), NTUPLE_COLUMNS, ntuple=True)
for i, (a, b) in enumerate(((0, 500), (500, N))):
    write_tree(p.path("rn", f"part{i}.root"), NTUPLE_COLUMNS, rows=slice(a, b), ntuple=True)
write_csv(p.path("csv", "t.csv"), {k: v for k, v in COLUMNS.items() if k not in ("cstr", "vec")})
write_tree(p.path("memory.root"), COLUMNS)
memoryFile = ROOT.TFile.Open(p.path("memory.root"))
memoryList = ROOT.TList()
memoryList.SetName("lst")
memoryList.Add(memoryFile.Get("t"))
ROOT.SetOwnership(memoryList, False)
table = S.table("t", {k: (np.array(list(v)) if isinstance(v, Texts) else v) for k, v in COLUMNS.items() if k not in ("cstr", "vec")})
p.pm.AddDataSource("tree", p.path("tree.root"))
p.pm.AddDataSource("tree_again", p.path("tree.root"))
p.pm.AddDataSource("chain", p.path("chain", "part*.root"))
p.pm.AddDataSource("folders", p.path("folders.root") + ":DF_*")
p.pm.AddDataSource("memory", memoryList)
p.pm.AddDataSource("table", [table])
p.pm.AddDataSource("csv", p.path("csv", "t.csv"))
p.pm.AddDataSource("rntuple", p.path("rntuple.root"))
p.pm.AddDataSource("rntuple_chain", p.path("rn", "part*.root"))


# ---------------------------------------------------------------------------------------------
# the values of the first axis of a case, as C++ computes them, with the rows (or rows of the elements of a collection) they belong to
# ---------------------------------------------------------------------------------------------
def column_values(col):
    """the values of a column and the row of each (a collection gives one value per element)"""
    if col == "vec":
        rows = np.array([k for k, v in enumerate(COLUMNS["vec"]) for _ in v], dtype=int)
        return np.array([e for v in COLUMNS["vec"] for e in v], dtype=np.float32), rows
    return np.asarray(COLUMNS[col]), np.arange(N)


def first_axis(col, selection):
    """(the expression of the first axis, the Define calls before it, its values, their rows, whether the values are integers)"""
    values, rows = column_values(col)
    integer = col in INTEGERS
    f32 = col in FLOAT32
    y = COLUMNS["y"][rows]
    if selection == "expression":
        return f"{col} * 2 + y", [], values.astype(np.float64) * 2 + y, rows, False
    if selection == "body":
        return f"if (y > 0) return 2.0 * {col}; return 0.5 * {col};", [], np.where(y > 0, 2.0 * values.astype(np.float64), 0.5 * values.astype(np.float64)), rows, False
    if selection == "define":
        v = (values.astype(np.float32) * np.float32(2) + np.float32(1)) if f32 else values.astype(np.float64) * 2 + 1
        return "srp_d", [("srp_d", f"{col} * 2 + 1")], v.astype(np.float64), rows, integer
    if selection == "redefine":
        return col, [(col, f"{col} * 2")], values.astype(np.float64) * 2, rows, integer
    return col, [], values.astype(np.float64), rows, integer


SELECTIONS = {  # selection: (rows kept, weight expression, weights)
    "none": (None, None),
    "expression": (None, None),
    "body": (None, None),
    "define": (None, None),
    "redefine": (None, None),
    "filter": (COLUMNS["y"] > 0, None),
    "filters": ((COLUMNS["y"] > 0) & (COLUMNS["wi"] != 2), None),
    "filter_string": (np.array(COLUMNS["s"]) == "b", None),
    "filter_cstring": (np.array(COLUMNS["s"]) == "b", None),
    "entries_first": (np.arange(N) < 500, None),
    "entries_range": ((np.arange(N) >= 250) & (np.arange(N) < 950), None),
    "weight": (None, "w"),
    "weight_int": (None, "wi"),
    "weight_bool": (None, "b"),
    "weight_expr": (None, "w * wi"),
}
WEIGHTS = {"w": COLUMNS["w"], "wi": COLUMNS["wi"].astype(np.float64), "b": COLUMNS["b"].astype(np.float64), "w * wi": COLUMNS["w"] * COLUMNS["wi"]}


def select(data, selection, defines):
    """the selection calls of a case on its data"""
    for key, value in defines:
        data.Define(key, value)
    if selection == "filter":
        data.Filter("y > 0")
    elif selection == "filters":
        data.Filter("y > 0").Filter("wi != 2")
    elif selection == "filter_string":
        data.Filter('s == "b"')
    elif selection == "filter_cstring":
        data.Filter('cstr == "b"')
    elif selection == "entries_first":
        data.Entries(500)
    elif selection == "entries_range":
        data.Entries(250, 950)


# ---------------------------------------------------------------------------------------------
# the kinds of requests: the call and the object the same rows give when filled directly
# ---------------------------------------------------------------------------------------------
def fixed_range(values):
    finite = values[np.isfinite(values)]
    lo, hi = np.percentile(finite, [5, 95])
    return (float(lo), float(hi)) if hi > lo else (float(lo) - 0.5, float(lo) + 1.5)


def auto_axis(values, integer, nBins):
    """the automatic binning of the finite values (util/Binning.h: 1% beyond both ends, bins of whole-number width for integers)"""
    finite = values[np.isfinite(values)]
    lo, hi = (float(finite.min()), float(finite.max())) if len(finite) else (math.inf, -math.inf)
    if integer and lo <= hi:
        nValues = round(hi) - round(lo) + 1.0
        width = math.ceil(nValues / max(nBins, 1))
        n = math.ceil(nValues / width)
        low = round(lo) - 0.5
        return n, low, max(low + n * width, math.nextafter(hi, math.inf))
    if not lo <= hi:
        return nBins, 0.0, 1.0
    pad = 0.01 * ((hi - lo) if hi > lo else max(abs(hi), 1.0))
    return nBins, lo - pad, max(hi + pad, math.nextafter(hi, math.inf))


def filled(h, columns, weights):
    if h.InheritsFrom("TH1") and not h.InheritsFrom("TProfile") and not h.InheritsFrom("TProfile2D"):
        h.Sumw2()
    n = len(columns[0])
    if n and len(columns) == 1:
        h.FillN(n, np.ascontiguousarray(columns[0]), np.ascontiguousarray(weights))
    else:
        for k in range(n):
            h.Fill(*(float(c[k]) for c in columns), float(weights[k]))
    return h


EDGES_Y = [-1.0, -0.5, 0.0, 0.3, 1.0]
EDGES_Z = [-3.0, -1.0, 0.0, 1.0, 3.0]


def kind_request(kind, data, var, rng_, weight):
    lo, hi = rng_
    edges = [lo, lo + 0.1 * (hi - lo), lo + 0.5 * (hi - lo), lo + 0.55 * (hi - lo), hi]
    if kind == "project1d":
        data.Project1D((var, 20, [lo, hi]), weight)
    elif kind == "project1d_edges":
        data.Project1D((var, edges), weight)
    elif kind == "project1d_auto":
        data.Project1D(var, weight)
    elif kind == "project1d_bins":
        data.Project1D((var, 7), weight)
    elif kind == "project2d_auto":
        data.Project2D(var, ("y", 5), weight)
    elif kind == "project2d":
        data.Project2D((var, 10, [lo, hi]), ("y", EDGES_Y), weight)
    elif kind == "project3d":
        data.Project([(var, 4, [lo, hi]), ("y", 3, [-1.0, 1.0]), ("z", EDGES_Z)], weight)
    elif kind == "project4d":
        data.Project([(var, 4, [lo, hi]), ("y", 3, [-1.0, 1.0]), ("z", 4, [-3.0, 3.0]), ("w", 2, [0.5, 2.0])], weight).Project([0, 1])
    elif kind == "profile1d":
        data.Profile1D((var, 10, [lo, hi]), "y", weight)
    elif kind == "profile1d_auto":
        data.Profile1D(var, "y", weight)
    elif kind == "profile2d":
        data.Profile2D((var, 6, [lo, hi]), ("y", 4, [-1.0, 1.0]), "z", weight)
    elif kind == "scatter2":
        data.Scatter(var, "y")
    elif kind == "scatter3":
        data.Scatter(var, "y", "z")
    elif kind == "scatter4":
        data.Scatter(var, "y", "ex", "ey").SetDrawOption(S.points_xerr)
    elif kind == "scatter6":
        data.Scatter(var, "y", "ex", "ex2", "ey", "ey2").SetDrawOption(S.points_xerr)
    if not kind.startswith("scatter"):
        data.SetShowOverflowBins()


def kind_reference(kind, values, rows, integer, rng_, weights):
    """the object the rows give filled directly (graphs: their values)"""
    lo, hi = rng_
    edges = np.array([lo, lo + 0.1 * (hi - lo), lo + 0.5 * (hi - lo), lo + 0.55 * (hi - lo), hi])
    col = {c: COLUMNS[c][rows] for c in ("y", "z", "w", "ex", "ex2", "ey", "ey2")}
    if kind == "project1d":
        return filled(ROOT.TH1D("ref", "", 20, lo, hi), [values], weights)
    if kind == "project1d_edges":
        return filled(ROOT.TH1D("ref", "", 4, edges), [values], weights)
    if kind in ("project1d_auto", "project1d_bins"):
        n, a, b = auto_axis(values, integer, 100 if kind == "project1d_auto" else 7)
        return filled(ROOT.TH1D("ref", "", n, a, b), [values], weights)
    if kind == "project2d_auto":
        (nx, ax, bx), (ny, ay, by) = auto_axis(values, integer, 100), auto_axis(col["y"], False, 5)
        return filled(ROOT.TH2D("ref", "", nx, ax, bx, ny, ay, by), [values, col["y"]], weights)
    if kind == "profile1d_auto":
        n, a, b = auto_axis(values, integer, 100)
        return filled(ROOT.TProfile("ref", "", n, a, b), [values, col["y"]], weights)
    if kind in ("project2d", "project4d"):
        h = ROOT.TH2D("ref", "", 10, lo, hi, 4, np.array(EDGES_Y)) if kind == "project2d" else ROOT.TH2D("ref", "", 4, lo, hi, 3, -1.0, 1.0)
        return filled(h, [values, col["y"]], weights)
    if kind == "project3d":
        xe = np.linspace(lo, hi, 5)
        h = ROOT.TH3D("ref", "", 4, xe, 3, np.linspace(-1.0, 1.0, 4), 4, np.array(EDGES_Z))
        return filled(h, [values, col["y"], col["z"]], weights)
    if kind == "profile1d":
        return filled(ROOT.TProfile("ref", "", 10, lo, hi), [values, col["y"]], weights)
    if kind == "profile2d":
        return filled(ROOT.TProfile2D("ref", "", 6, lo, hi, 4, -1.0, 1.0), [values, col["y"], col["z"]], weights)
    if kind == "scatter2":
        return {"x": list(values), "y": list(col["y"])}
    if kind == "scatter3":
        return {"x": list(values), "y": list(col["y"]), "z": list(col["z"])}
    errors = (col["ex"], col["ex"], col["ey"], col["ey"]) if kind == "scatter4" else (col["ex"], col["ex2"], col["ey"], col["ey2"])
    return {"x": list(values), "y": list(col["y"]), **{k: list(e) for k, e in zip(("ErrorXlow", "ErrorXhigh", "ErrorYlow", "ErrorYhigh"), errors)}}


KINDS = ("project1d", "project1d_edges", "project1d_auto", "project1d_bins", "project2d", "project2d_auto", "project3d", "project4d", "profile1d", "profile1d_auto", "profile2d", "scatter2", "scatter3", "scatter4", "scatter6")
BASE = {"project2d": "2d", "project2d_auto": "2d", "project3d": "2d", "project4d": "2d", "profile2d": "2d", "scatter3": "2d"}
AUTO = ("project1d_auto", "project1d_bins", "project2d_auto", "profile1d_auto")  # kinds with an automatic range of the first axis


def valid(case):
    fmt, kind, col, sel = case["format"], case["kind"], case["column"], case["selection"]
    have = FORMATS[fmt][2]
    if col not in have or (sel == "filter_string" and "s" not in have) or (sel == "filter_cstring" and "cstr" not in have):
        return False
    if kind.startswith("scatter") and (SELECTIONS[sel][1] or col in ("vec", "xnf")):
        return False  # a scatter has no weights; graphs of points that are not finite: a matter of drawing
    if not FULL:
        return True
    # the full list: each of kind, column and selection varied on its own and in pairs around the plain values, for every format
    return sum((kind != "project1d", col != "x", sel != "none")) <= 2


todo = cases(
    {"format": list(FORMATS), "kind": list(KINDS), "column": [*SCALARS, "vec"], "selection": list(SELECTIONS)},
    valid,
    named=[
        {"format": "tree", "kind": "project1d", "column": "i32", "selection": "define", "why": "requests booked with the types of their columns"},
        {"format": "tree", "kind": "project1d", "column": "x", "selection": "body", "why": "requests booked with the types of their columns"},
        {"format": "tree", "kind": "project4d", "column": "x", "selection": "body", "why": "axis titles of expressions with ';'"},
        {"format": "tree", "kind": "scatter3", "column": "x", "selection": "body", "why": "axis titles of expressions with ';'"},
        {"format": "tree", "kind": "project1d_auto", "column": "i32", "selection": "none", "why": "requests booked with the types of their columns"},
        {"format": "tree", "kind": "project1d", "column": "x", "selection": "filter_cstring", "why": "filters on C-string columns"},
        {"format": "chain", "kind": "project1d", "column": "x", "selection": "filter_cstring", "why": "filters on C-string columns"},
        {"format": "memory", "kind": "project1d", "column": "x", "selection": "filter_cstring", "why": "filters on C-string columns"},
        {"format": "tree", "kind": "scatter3", "column": "x", "selection": "filter", "why": "scatter of three columns as 3d points"},
        {"format": "csv", "kind": "scatter3", "column": "x", "selection": "none", "why": "scatter of three columns as 3d points"},
        {"format": "tree", "kind": "project1d_auto", "column": "xnf", "selection": "none", "why": "automatic binning of values not finite"},
        {"format": "csv", "kind": "project1d_auto", "column": "xnf", "selection": "none", "why": "automatic binning of values not finite"},
        {"format": "tree", "kind": "project1d_auto", "column": "vec", "selection": "none", "why": "automatic binning of a collection"},
    ],
    always=lambda case: (case["selection"] == "none" and case["column"] == "x") or (case["format"] == "tree" and case["selection"] == "none"),
)

chunks = Chunks("req")
expected = {}  # plot: (case, reference, kind of result)
nonFinite = set()  # warnings expected: (count, expression, data location)
for i, case in enumerate(todo):
    fmt, kind, col, sel = case["format"], case["kind"], case["column"], case["selection"]
    source, name, _ = FORMATS[fmt]
    var, defines, values, rows, integer = first_axis(col, sel)
    keep, weightExpr = SELECTIONS[sel]
    mask = np.ones(len(values), dtype=bool) if keep is None else keep[rows]
    weights = WEIGHTS[weightExpr][rows] if weightExpr else np.ones(len(values))
    rng_ = fixed_range(values)
    plotName = f"r{i}"
    plot = S.Plot(plotName, chunks.add(plotName), BASE.get(kind, "1d"))
    data = plot[1].AddData(name, source)
    select(data, sel, defines)
    kind_request(kind, data, var, rng_, weightExpr)
    p.pm.AddPlot(plot)
    sub = (values[mask], rows[mask], weights[mask])
    expected[plotName] = (case, kind_reference(kind, sub[0], sub[1], integer, rng_, sub[2]))
    nNonFinite = int(np.sum(~np.isfinite(sub[0])))
    if kind in AUTO and nNonFinite:
        nonFinite.add((nNonFinite, var, f"{'table' if fmt == 'csv' else 'tree'} {source}:{name}"))

# requests of one tree that differ only slightly, each with its own result; the same request written differently gives the same
xs, ys, ws, wis, i32s = COLUMNS["x"], COLUMNS["y"], COLUMNS["w"], COLUMNS["wi"], COLUMNS["i32"]
distinct = {}


def same_tree(what, ref, setup, name="t", source="tree"):
    plotName = f"d{len(distinct)}"
    plot = S.Plot(plotName, "distinct", "1d")
    setup(plot[1].AddData(name, source)).SetShowOverflowBins()
    p.pm.AddPlot(plot)
    distinct[plotName] = (what, ref)


for lo in (0.1, 0.1 + 1e-12, 0.10000001, 1 / 10):
    same_tree(f"range from {lo!r}", filled(ROOT.TH1D("ref", "", 10, lo, 2.0), [xs], np.ones(N)), lambda d, lo=lo: d.Project1D(("x", 10, [lo, 2.0])))
for nb in (10, 11, 100, 1000):
    same_tree(f"{nb} bins", filled(ROOT.TH1D("ref", "", nb, -3.0, 3.0), [xs], np.ones(N)), lambda d, nb=nb: d.Project1D(("x", nb, [-3.0, 3.0])))
same_tree("edges equal to fixed bins", filled(ROOT.TH1D("ref", "", 4, np.array([-3.0, -1.5, 0.0, 1.5, 3.0])), [xs], np.ones(N)), lambda d: d.Project1D(("x", [-3.0, -1.5, 0.0, 1.5, 3.0])))
FILTERS = {
    "y > 0": ys > 0,
    "y>0": ys > 0,
    " y  >  0 ": ys > 0,
    "y > 0 && wi < 3": (ys > 0) & (wis < 3),
    "y > 0 || wi < 2": (ys > 0) | (wis < 2),
    "abs(y) < 0.5": np.abs(ys) < 0.5,
    "i32 % 3 == 1": np.fmod(i32s, 3) == 1,
    'TString::Format("%d", wi) == "3"': wis == 3,
    "x/2 > y": xs / 2 > ys,
}
for flt, mask in FILTERS.items():
    same_tree(f"filter {flt!r}", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs[mask]], np.ones(mask.sum())), lambda d, flt=flt: d.Project1D(("x", 20, [-3.0, 3.0])).Filter(flt))
mask = (ys > 0) & (wis > 1)
same_tree("two filters", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs[mask]], np.ones(mask.sum())), lambda d: d.Project1D(("x", 20, [-3.0, 3.0])).Filter("y > 0").Filter("wi > 1"))
same_tree("two filters in the other order", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs[mask]], np.ones(mask.sum())), lambda d: d.Project1D(("x", 20, [-3.0, 3.0])).Filter("wi > 1").Filter("y > 0"))
for expr, vals in (("x*2", xs * 2), ("x*3", xs * 3), ("x+y", xs + ys)):
    same_tree(f"define v = {expr}", filled(ROOT.TH1D("ref", "", 30, -6.0, 6.0), [vals], np.ones(N)), lambda d, expr=expr: d.Define("v", expr).Project1D(("v", 30, [-6.0, 6.0])))
same_tree("a define using a define", filled(ROOT.TH1D("ref", "", 30, -6.0, 6.0), [xs * 2 + 1], np.ones(N)), lambda d: d.Define("a", "x*2").Define("b", "a+1").Project1D(("b", 30, [-6.0, 6.0])))
same_tree("a column whose name contains return", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs], np.ones(N)), lambda d: d.Define("returnValue", "x").Project1D(("returnValue", 20, [-3.0, 3.0])))
same_tree("a lambda in an expression", filled(ROOT.TH1D("ref", "", 20, -6.0, 6.0), [xs * 2], np.ones(N)), lambda d: d.Project1D(("[](double v) { return 2 * v; }(x)", 20, [-6.0, 6.0])))
same_tree("a define replacing the column a filter uses", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs[2 * xs > 1] * 2], np.ones(int((2 * xs > 1).sum()))), lambda d: d.Define("x", "x*2").Filter("x > 1").Project1D(("x", 20, [-3.0, 3.0])))
mask = (ys > 0) & (np.arange(N) >= 100) & (np.arange(N) < 900)
same_tree("a weight, a filter and an entry range", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs[mask]], ws[mask]), lambda d: d.Filter("y > 0").Entries(100, 900).Project1D(("x", 20, [-3.0, 3.0]), "w"))
same_tree("the same tree name in a folder of the file", filled(ROOT.TH1D("ref", "", 20, -6.0, 6.0), [xs * 2], np.ones(N)), lambda d: d.Project1D(("x", 20, [-6.0, 6.0])), "dir/t")
same_tree("the same file as another data source", filled(ROOT.TH1D("ref", "", 20, -6.0, 6.0), [xs], np.ones(N)), lambda d: d.Project1D(("x", 20, [-6.0, 6.0])), "t", "tree_again")
same_tree("weights of whole rows", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs], ws), lambda d: d.Project1D(("x", 20, [-3.0, 3.0]), "w"))
same_tree("weights squared", filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs], ws * ws), lambda d: d.Project1D(("x", 20, [-3.0, 3.0]), "w*w"))
# C strings in a leaf list (ll.s) are strings too, an array of Char_t (arr[3]/B) stays numbers
ROOT.gInterpreter.Declare(r"""
void srptest_leaf_lists(const char* path) {
  TFile f(path, "RECREATE");
  TTree t("t", "t");
  struct { Int_t i; char s[16]; } ll;
  char arr[3];
  t.Branch("ll", &ll, "i/I:s[16]/C");
  t.Branch("arr", arr, "arr[3]/B");
  const char* words[] = {"test", "train", "test", "validation"};
  for (int k = 0; k < 4; ++k) {
    ll.i = k;
    strcpy(ll.s, words[k]);
    for (int j = 0; j < 3; ++j) arr[j] = k + j;
    t.Fill();
  }
  t.Write();
}
""")
ROOT.srptest_leaf_lists(p.path("leaf_lists.root"))
p.pm.AddDataSource("leaf_lists", p.path("leaf_lists.root"))
same_tree("a C string in a leaf list in a filter", filled(ROOT.TH1D("ref", "", 4, -0.5, 3.5), [np.array([0.0, 2.0])], np.ones(2)), lambda d: d.Filter('ll.s == "test"').Project1D(("ll.i", 4, [-0.5, 3.5])), "t", "leaf_lists")
same_tree("an array of Char_t: its numbers", filled(ROOT.TH1D("ref", "", 6, -0.5, 5.5), [np.array([k + j for k in range(4) for j in range(3)], dtype=float)], np.ones(12)), lambda d: d.Project1D(("arr", 6, [-0.5, 5.5])), "t", "leaf_lists")

# the same request in two plots, once normalized: one read, the same values
for plotName, norm in (("same_a", False), ("same_b", True)):
    plot = S.Plot(plotName, "distinct", "1d")
    d = plot[1].AddData("t", "tree").Project1D(("x", 20, [-3.0, 3.0])).Filter("y > 0").SetShowOverflowBins()
    if norm:
        d.Normalize()
    p.pm.AddPlot(plot)

# weighted profiles of a tree read in parallel where some inputs have weights of 1 only: ROOT merges the profiles of its threads
os.makedirs(p.path("ones"))
NP = 3000
ones = {"x": rng.normal(size=8 * NP), "y": rng.normal(size=8 * NP), "z": rng.normal(size=8 * NP), "w": np.ones(8 * NP)}
ones["w"][:NP] = rng.integers(1, 4, NP)
for i in range(8):
    write_tree(p.path("ones", f"part{i}.root"), ones, rows=slice(i * NP, (i + 1) * NP))
p.pm.AddDataSource("ones", p.path("ones", "part*.root"))
merged = {
    "Profile1D": (lambda d: d.Profile1D(("x", 10, [-2.0, 2.0]), "y", "w"), filled(ROOT.TProfile("ref", "", 10, -2.0, 2.0), [ones["x"], ones["y"]], ones["w"])),
    "Profile2D": (lambda d: d.Profile2D(("x", 4, [-2.0, 2.0]), ("y", 4, [-2.0, 2.0]), "z", "w"), filled(ROOT.TProfile2D("ref", "", 4, -2.0, 2.0, 4, -2.0, 2.0), [ones["x"], ones["y"], ones["z"]], ones["w"])),
}
for plotName, (setup, _) in merged.items():
    plot = S.Plot(plotName, "merged", "1d" if plotName == "Profile1D" else "2d")
    setup(plot[1].AddData("t", "ones")).SetShowOverflowBins()
    p.pm.AddPlot(plot)

# invalid requests among valid ones of the same trees: only they are lost, with the reason
errors = {}  # plot: (what, expected error text)


def invalid(what, source, name, setup, text):
    plotName = f"e{len(errors)}"
    plot = S.Plot(plotName, "errors", "1d")
    setup(plot[1].AddData(name, source))
    p.pm.AddPlot(plot)
    errors[plotName] = (what, text)


for source, kind in (("tree", "tree"), ("csv", "table"), ("rntuple", "tree"), ("table", "tree")):
    invalid(f"{source}: a string column as axis", source, "t", lambda d: d.Project1D(("s", 10, [0.0, 1.0])), f"Expression s for {kind} {source}:t is not numeric.")
    invalid(f"{source}: an unknown column", source, "t", lambda d: d.Project1D(("nope", 10, [0.0, 1.0])), f"Illegal expression nope for {kind} {source}:t (available columns:")
    invalid(f"{source}: an unknown column in a filter", source, "t", lambda d: d.Project1D(("x", 10, [0.0, 1.0])).Filter("nope > 1"), f"Illegal filter expression nope > 1 for {kind} {source}:t")
    invalid(f"{source}: an unknown weight", source, "t", lambda d: d.Project1D(("x", 10, [0.0, 1.0]), "nope"), f"Illegal weight expression nope for {kind} {source}:t")
    invalid(f"{source}: an expression throwing while reading", source, "t", lambda d: d.Project1D(("std::stod(s)", 10, [0.0, 1.0])), f"{source}:t (projection of std::stod(s))")
    invalid(f"{source}: a profile with three binned axes", source, "t", lambda d: d.Profile([("x", 2, [0.0, 1.0]), ("y", 2, [0.0, 1.0]), ("z", 2, [0.0, 1.0])], "w"), f"Too many dimensions specified for profile of {kind} {source}:t")
    invalid(f"{source}: no request", source, "t", lambda d: d, f"Cannot plot {kind} {source}:t directly")
invalid("tree: a C-string column as axis", "tree", "t", lambda d: d.Project1D(("cstr", 10, [0.0, 1.0])), "Expression cstr for tree tree:t is not numeric.")
invalid("tree: a scatter in 3d of a collection", "tree", "t", lambda d: d.Scatter("vec", "y", "z"), "A scatter plot in 3d needs scalar values: tree tree:t")
valids = {}
for source in ("tree", "csv", "rntuple", "table"):
    plotName = f"v_{source}"
    plot = S.Plot(plotName, "errors", "1d")
    plot[1].AddData("t", source).Project1D(("x", 20, [-3.0, 3.0])).SetShowOverflowBins()
    plot[1].AddData("t", source).Project1D(("x", 20, [-3.0, 3.0])).Filter("y > 5").SetShowOverflowBins()
    p.pm.AddPlot(plot)
    valids[plotName] = source

# ---------------------------------------------------------------------------------------------
chunks.plot(p)
check("every plot run wrote its file", not chunks.unwritten(), " | ".join(chunks.unwritten()))
for plotName, (case, ref) in expected.items():
    obs = chunks.objects(plotName)
    if isinstance(ref, dict):
        cls = "TGraph2D" if case["kind"] == "scatter3" else "TGraph" if case["kind"] == "scatter2" else "TGraphAsymmErrors"
        diff = (f"{obs[0].ClassName()}, expected {cls}" if obs[0].ClassName() != cls else graph_diff(ref, obs[0])) if obs else "not drawn"
    else:
        diff = drawn_diff([ref], obs)
    check(f"{describe(case)}: the rows filled directly", not diff, diff)
    if obs:
        var = first_axis(case["column"], case["selection"])[0]
        counts = "weighted counts" if SELECTIONS[case["selection"]][1] else "counts"
        want = {"project2d": (var, "y", counts), "project2d_auto": (var, "y", counts), "profile1d_auto": (var, "#LT y #GT"), "project3d": (var, "y", "z"), "project4d": (var, "y"), "profile1d": (var, "#LT y #GT"), "profile2d": (var, "y", "#LT z #GT"), "scatter3": (var, "y", "z")}
        want = want.get(case["kind"], (var, "y") if case["kind"].startswith("scatter") else (var, counts))
        got = tuple(getattr(obs[0], f"Get{axis}axis")().GetTitle() for axis in "XYZ"[: len(want)])
        check(f"{describe(case)}: the axis titles name the expressions", got == want, f"{got}, expected {want}")

warned = chunks.warnings()
for count, var, where in sorted(nonFinite):
    text = f"{count} value{'s' if count > 1 else ''} of {var} in {where} {'are' if count > 1 else 'is'} not finite (inf or nan)"
    check(f"{where}: values of {var} that are not finite are left out of the automatic range, with a warning", any(text in w for w in warned), f"no '{text}'")
known = re.compile(r"not finite \(inf or nan\)")
other = [w for w in warned if not known.search(w)]
check("the requests give no other warnings", not other, " | ".join(other[:5]))
check("the requests give no errors", not chunks.errors(), " | ".join(chunks.errors()[:5]))

# the reading summary: entries, the inputs a tree is chained from, at most two passes per group of requests (parallel and sequential)
log = "\n".join(run.log for run in chunks.runs.values())
for source, inputs in (("tree", ""), ("chain", " from 3 files"), ("folders", " from 3 folders in 1 file"), ("memory", ""), ("table", ""), ("csv", ""), ("rntuple", ""), ("rntuple_chain", " from 2 files")):
    name = FORMATS[source][1]
    lines = re.findall(rf"^.* - {re.escape(source)}:{re.escape(name)} (\d+) entries(.*?), (\d+) requests?, (\d+) pass(?:es)?, .*$", log, re.M)
    ok = lines and all(int(n) == N and rest.startswith(inputs) and (inputs or not rest.startswith(" from")) and int(passes) <= 4 for n, rest, _, passes in lines)
    check(f"{source}: the reading summary names {N} entries{inputs or ''} and at most two passes per group", ok, str(lines[:3]))

distinctRun = p.plot("distinct")
for plotName, (what, ref) in distinct.items():
    diff = drawn_diff([ref], distinctRun.objects(plotName, "distinct"))
    check(f"requests differing slightly: {what}", not diff, diff)
a, b = distinctRun.objects("same_a", "distinct"), distinctRun.objects("same_b", "distinct")
ref = filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs[ys > 0]], np.ones(int((ys > 0).sum())))
check("the same request in two plots: the same values", a and not drawn_diff([ref], a), drawn_diff([ref], a))
if b:
    ref.Scale(1.0 / ref.Integral())
check("the same request in two plots, normalized in one: the values normalized", b and not drawn_diff([ref], b, 1e-12), drawn_diff([ref], b))
check("requests differing slightly: no errors or warnings", not distinctRun.errors() and not distinctRun.warnings(), " | ".join(distinctRun.errors() + distinctRun.warnings()))

mergedRun = p.plot("merged")
for plotName, (_, ref) in merged.items():
    diff = drawn_diff([ref], mergedRun.objects(plotName, "merged"))
    check(f"{plotName} with weights, read in parallel from inputs with weights of 1 only in some: the rows filled directly", not diff, diff)

errorRun = p.plot("errors")
for plotName, (what, text) in errors.items():
    lost = not errorRun.canvas(plotName, "errors")
    reason = any(text in line for line in errorRun.errors())
    check(f"an invalid request loses only its plot, with the reason: {what}", lost and reason, f"plot {'lost' if lost else 'made'}, '{text}' {'given' if reason else 'not given'}: {[e for e in errorRun.errors()][:3]}")
for plotName, source in valids.items():
    obs = errorRun.objects(plotName, "errors")
    ref = filled(ROOT.TH1D("ref", "", 20, -3.0, 3.0), [xs], np.ones(N))
    empty = ROOT.TH1D("ref", "", 20, -3.0, 3.0)
    diff = drawn_diff([ref, empty], obs)
    check(f"{source}: valid requests beside invalid ones are read as usual", not diff, diff)
    kind = "table" if source == "csv" else "tree"
    check(f"{source}: a request selecting no entries is drawn empty, with a warning", any(f"No entries selected in {kind} {source}:t (projection of x; Filter y > 5)" in w for w in errorRun.warnings()), " | ".join(errorRun.warnings()[:3]))
finish()
