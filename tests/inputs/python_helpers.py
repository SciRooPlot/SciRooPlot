"""The numpy-style helpers of the Python interface give the objects numpy and pandas describe, and they are read as data of a project:
histo and histo2d fill like numpy.histogram and numpy.histogram2d (bins as a number, with the range given or that of the values widened by
1% on each side, or as edges, also mixed per axis; the last bin includes its upper edge; values outside the range in under- and overflow;
weights with errors from the sum of their squares) from numpy arrays of any numeric type, lists and pandas Series; graph makes a
TGraphErrors with optional errors; table makes a tree of a dict of arrays, a structured array, a DataFrame or a 2d array with column
names: numbers and bools keep their type, strings become std::string, columns that cannot be stored are skipped with a warning naming
the reason. Wrong input raises an exception saying what is wrong (a TypeError for the wrong kind of argument, a ValueError for wrong
values). Handed to AddDataSource, a table works with Project1D and Filter (also on strings), a histogram is drawn as filled.
Covers: 9dbb776 (histo, histo2d, graph, table and their checks).
"""

import math

import numpy as np
from srptest import ROOT, Project, S, captured, check, finish, skip

try:
    import pandas as pd
except ImportError:
    pd = None
if pd is None:
    skip("pandas is not installed")

rng = np.random.default_rng(1)
N = 1000
x = rng.normal(size=N)
y = rng.normal(size=N)
w = rng.uniform(0.5, 1.5, size=N)


def auto_range(values):
    """the range of a number of bins without range: that of the values, 1% beyond both ends (util/Binning.h)"""
    lo, hi = float(np.min(values)), float(np.max(values))
    pad = 0.01 * ((hi - lo) if hi > lo else max(abs(hi), 1.0))
    return lo - pad, max(hi + pad, math.nextafter(hi, math.inf))


def diff_1d(h, values, bins, rng_=None, weights=None):
    """differences of a histogram from numpy.histogram of the same values: inner bins (contents, errors from the squared weights), flows"""
    if not h:
        return f"{h}"
    values = np.asarray(values, dtype=np.float64)
    if isinstance(bins, (int, np.integer)):
        rng_ = rng_ or (auto_range(values) if len(values) else (0.0, 1.0))
        edges = np.linspace(rng_[0], rng_[1], int(bins) + 1)
    else:
        edges = np.asarray(bins, dtype=np.float64)
    counts, _ = np.histogram(values, edges, weights=weights)
    sumw2, _ = np.histogram(values, edges, weights=None if weights is None else np.asarray(weights) ** 2)
    if h.GetNbinsX() != len(counts):
        return f"{h.GetNbinsX()} bins, expected {len(counts)}"
    wts = np.ones(len(values)) if weights is None else np.asarray(weights, dtype=np.float64)
    under, over = wts[values < edges[0]].sum(), wts[values > edges[-1]].sum()
    for i, (c, e2) in enumerate(zip(counts, sumw2)):
        if abs(h.GetBinContent(i + 1) - c) > 1e-9 * max(1, abs(c)) or abs(h.GetBinError(i + 1) ** 2 - e2) > 1e-9 * max(1, e2):
            return f"bin {i + 1}: {h.GetBinContent(i + 1)} ± {h.GetBinError(i + 1)}, expected {c} ± {math.sqrt(e2)}"
    if abs(h.GetBinContent(0) - under) > 1e-9 or abs(h.GetBinContent(len(counts) + 1) - over) > 1e-9:
        return f"flows {h.GetBinContent(0)}, {h.GetBinContent(len(counts) + 1)}, expected {under}, {over}"
    return ""


def diff_2d(h, xs, ys, bins, rng_, weights=None):
    counts, xe, ye = np.histogram2d(xs, ys, bins, rng_, weights=weights)
    if (h.GetNbinsX(), h.GetNbinsY()) != counts.shape:
        return f"{h.GetNbinsX()} x {h.GetNbinsY()} bins, expected {counts.shape}"
    edges = [h.GetXaxis().GetBinLowEdge(i + 1) for i in range(len(xe))], [h.GetYaxis().GetBinLowEdge(i + 1) for i in range(len(ye))]
    if not (np.allclose(edges[0], xe, rtol=1e-12) and np.allclose(edges[1], ye, rtol=1e-12)):
        return f"edges {edges}, expected {xe}, {ye}"
    bad = [(i, j) for i in range(counts.shape[0]) for j in range(counts.shape[1]) if abs(h.GetBinContent(i + 1, j + 1) - counts[i, j]) > 1e-9 * max(1, counts[i, j])]
    return f"bins {bad[:3]} differ" if bad else ""


def raises(what, func, exc, text=""):
    try:
        func()
    except exc as e:
        check(f"{what}: raises {exc.__name__}", text in str(e), f"message '{e}' lacks '{text}'")
        return
    except Exception as e:  # noqa: BLE001
        check(f"{what}: raises {exc.__name__}", False, f"{type(e).__name__}: {e}")
        return
    check(f"{what}: raises {exc.__name__}", False, "no exception")


# histo: forms of the values, bins and ranges, weights
FORMS = {"float64": x, "float32": x.astype(np.float32), "int": np.round(x * 3).astype(np.int64), "list": x.tolist(), "Series": pd.Series(x)}
for form, values in FORMS.items():
    ref = np.asarray(values, dtype=np.float64)
    for bins, label in ((20, "a number of bins"), (np.int64(7), "a numpy number of bins"), ([-4.0, -1.0, 0.0, 0.5, 4.0], "edges"), (np.array([-2.0, 0.0, 2.0]), "edges as array")):
        for rng_ in (None, (-3.0, 3.0)):
            if rng_ and not isinstance(bins, (int, np.integer)):
                continue
            diff = diff_1d(S.histo("h", values, bins, rng_), ref, bins, rng_)
            check(f"histo of {form} values, {label}, {'range ' + str(rng_) if rng_ else 'range of the values'}: as numpy.histogram", not diff, diff)
    for weights, label in ((w, "numpy"), (w.tolist(), "list")):
        diff = diff_1d(S.histo("h", values, 20, (-4.0, 4.0), weights=weights), ref, 20, (-4.0, 4.0), w)
        check(f"histo of {form} values with weights as {label}: as numpy.histogram, errors from the squared weights", not diff, diff)
edges = [0.0, 1.0, 2.0]
diff = diff_1d(S.histo("h", [0.0, 1.0, 2.0, 2.0, 3.0], edges), [0.0, 1.0, 1.9, 1.9, 3.0], edges)
check("histo: the last bin includes its upper edge (as numpy)", not diff, diff)
diff = diff_1d(S.histo("h", [0.0, 0.5, 1.0], 4, (0.0, 1.0)), [0.0, 0.5, 0.9], 4, (0.0, 1.0))
check("histo: the last bin includes its upper edge, even bins", not diff, diff)
h = S.histo("h", [], 5)
check("histo of no values: empty bins on [0, 1] (as numpy)", h.GetNbinsX() == 5 and h.GetXaxis().GetXmin() == 0.0 and h.GetXaxis().GetXmax() == 1.0 and h.GetEntries() == 0, f"{h.GetNbinsX()} [{h.GetXaxis().GetXmin()}, {h.GetXaxis().GetXmax()}]")
h = S.histo("h", x)
check("histo: 100 bins by default", h.GetNbinsX() == 100 and not diff_1d(h, x, 100), diff_1d(h, x, 100))
check("histo: named as given", S.histo("my name", x).GetName() == "my name", "")
raises("histo: one edge", lambda: S.histo("e", x, [1.0]), ValueError, "at least two increasing, finite bin edges")
raises("histo: decreasing edges", lambda: S.histo("e", x, [0.0, 2.0, 1.0]), ValueError, "increasing")
raises("histo: an infinite edge", lambda: S.histo("e", x, [0.0, math.inf]), ValueError, "finite")
raises("histo: no bins", lambda: S.histo("e", x, 0), ValueError, "at least 1")
raises("histo: bins as text", lambda: S.histo("e", x, "auto"), TypeError)
raises("histo: bins as bool", lambda: S.histo("e", x, True), TypeError)
raises("histo: a reversed range", lambda: S.histo("e", x, 10, (1.0, 0.0)), ValueError, "lower end below the upper end")
raises("histo: an infinite range", lambda: S.histo("e", x, 10, (0.0, math.inf)), ValueError, "finite")
raises("histo: values that are not finite without range", lambda: S.histo("e", [1.0, math.nan]), ValueError, "give a range or bin edges")
raises("histo: weights of another length", lambda: S.histo("e", x, weights=w[:5]), ValueError, "as many entries as the values")
raises("histo: text values", lambda: S.histo("e", ["a", "b"]), TypeError, "one-dimensional sequence of numbers")
raises("histo: a 2d array", lambda: S.histo("e", np.ones((3, 3))), TypeError, "one-dimensional")
h = S.histo("h", [1.0, math.nan, math.inf, -math.inf, 0.5], 2, (0.0, 2.0))
check("histo: values that are not finite with a range go to the flows", h.GetBinContent(1) == 1 and h.GetBinContent(2) == 1 and h.GetBinContent(0) == 1 and h.GetBinContent(3) == 2, [h.GetBinContent(i) for i in range(4)])

# histo2d
for form in ("float64", "list", "Series"):
    xs, ys = FORMS[form], (y.tolist() if form == "list" else pd.Series(y) if form == "Series" else y)
    for bins, rng_, label in (
        (10, ((-4.0, 4.0), (-4.0, 4.0)), "one number of bins"),
        ((10, 5), ((-4.0, 4.0), (-3.0, 3.0)), "a number of bins per axis"),
        (([-4.0, -1.0, 0.0, 0.5, 4.0], [-4.0, 0.0, 4.0]), None, "edges per axis"),
        ((8, [-4.0, 0.0, 4.0]), ((-4.0, 4.0), None), "a number and edges"),
    ):
        h = S.histo2d("h", xs, ys, bins, rng_)
        diff = diff_2d(h, x, y, bins if not isinstance(bins, tuple) or not isinstance(bins[0], int) or not isinstance(bins[1], list) else [np.linspace(-4, 4, 9), bins[1]], rng_ if rng_ and None not in rng_ else None)
        check(f"histo2d of {form} values, {label}: as numpy.histogram2d", not diff, diff)
h = S.histo2d("h", x, y, 10, ((-4.0, 4.0), (-4.0, 4.0)), weights=w)
diff = diff_2d(h, x, y, 10, ((-4.0, 4.0), (-4.0, 4.0)), w)
check("histo2d with weights: as numpy.histogram2d", not diff, diff)
h = S.histo2d("h", x, y)
xr, yr = auto_range(x), auto_range(y)
check("histo2d: 50 x 50 bins over the ranges of the values by default", h.GetNbinsX() == 50 and h.GetNbinsY() == 50 and not diff_2d(h, x, y, 50, (xr, yr)), diff_2d(h, x, y, 50, (xr, yr)))
h = S.histo2d("h", x, y, (8, [-4.0, 0.0, 4.0]), ((-4.0, 4.0), None))
check("histo2d: a number of bins beside edges is evenly spaced", not h.GetXaxis().IsVariableBinSize() or np.allclose([h.GetXaxis().GetBinLowEdge(i) for i in range(1, 10)], np.linspace(-4, 4, 9)), "")
raises("histo2d: x and y of other lengths", lambda: S.histo2d("e", x, y[:3]), ValueError, "same length")
raises("histo2d: three numbers of bins", lambda: S.histo2d("e", x, y, (1, 2, 3)), ValueError, "(nx, ny)")
raises("histo2d: a range of one axis only", lambda: S.histo2d("e", x, y, 10, (-4.0, 4.0)), ValueError, "((xmin, xmax), (ymin, ymax))")

# graph
g = S.graph("g", np.arange(5.0), np.arange(5) ** 2, yerr=np.full(5, 0.5))
check("graph: a TGraphErrors through the points with the y errors given", g.ClassName() == "TGraphErrors" and g.GetN() == 5 and g.GetPointY(3) == 9 and g.GetErrorY(2) == 0.5 and g.GetErrorX(2) == 0 and g.GetName() == "g", "")
g = S.graph("g", [1, 2], pd.Series([3.0, 4.0]), xerr=[0.1, 0.2])
check("graph of a list and a Series with x errors", g.GetN() == 2 and g.GetPointY(1) == 4 and g.GetErrorX(1) == 0.2 and g.GetErrorY(1) == 0, "")
raises("graph: x and y of other lengths", lambda: S.graph("e", [1, 2], [3]), ValueError, "same length")
raises("graph: errors of another length", lambda: S.graph("e", [1, 2], [3, 4], xerr=[1]), ValueError, "as many entries as the points")

# table
n = 600
frame = pd.DataFrame(
    {
        "pt": rng.exponential(2.0, n),
        "eta": rng.uniform(-1, 1, n).astype(np.float32),
        "charge": rng.choice(np.array([-1, 1], dtype=np.int8), n),
        "nHits": rng.integers(0, 100, n).astype(np.uint16),
        "id": np.arange(n, dtype=np.int64),
        "u8": rng.integers(0, 200, n).astype(np.uint8),
        "i16": rng.integers(-300, 300, n).astype(np.int16),
        "i32": rng.integers(-3000, 3000, n).astype(np.int32),
        "u32": rng.integers(0, 3000, n).astype(np.uint32),
        "u64": rng.integers(0, 3000, n).astype(np.uint64),
        "isPrimary": rng.uniform(size=n) > 0.3,
        "det": rng.choice(["ITS", "TPC", "TOF"], n),
        "when": pd.date_range("2026-01-01", periods=n, freq="h"),
        "missing": pd.array([1, None] * (n // 2), dtype="Int64"),
        "mixed": [1, "a"] * (n // 2),
    }
)
tree, printed = captured(lambda: S.table("tracks", frame))
types = {b.GetName(): b.GetLeaf(b.GetName()).GetTypeName() for b in tree.GetListOfBranches()}
want = {"pt": "Double_t", "eta": "Float_t", "charge": "Char_t", "nHits": "UShort_t", "id": "Long64_t", "u8": "UChar_t", "i16": "Short_t", "i32": "Int_t", "u32": "UInt_t", "u64": "ULong64_t", "isPrimary": "Bool_t", "det": "string", "missing": "Double_t"}
check("table of a DataFrame: numbers and bools keep their type, strings become std::string", types == want, f"{types}")
check("table: a column of a type that cannot be stored is skipped, with the reason", "Table tracks: column 'when' is skipped (its type datetime64" in printed, printed)
check("table: a column of numbers and strings is skipped, with the reason", "Table tracks: column 'mixed' is skipped (its values are neither numbers of one type nor all strings" in printed, printed)
frameRDF = ROOT.RDataFrame(tree)
sums = {c: frameRDF.Sum(c).GetValue() for c in ("pt", "charge", "id", "u64", "i16")}
check("table: the values of the columns", tree.GetEntries() == n and all(abs(sums[c] - frame[c].astype(float).sum()) < 1e-6 * max(1, abs(frame[c].astype(float).sum())) for c in sums), str(sums))
check("table: strings usable in filters", frameRDF.Filter('det == "TPC"').Count().GetValue() == (frame.det == "TPC").sum(), "")
check("table: nullable integers with missing values become floating point with nan", np.isnan(ROOT.RDataFrame(tree).AsNumpy(["missing"])["missing"][1]), "")
check("table of a dict of lists", S.table("d", {"a": [1.0, 2.0], "b": [True, False]}).GetEntries() == 2, "")
structured = np.zeros(4, dtype=[("x", "f8"), ("n", "i4")])
structured["x"] = [1, 2, 3, 4]
check("table of a structured array", ROOT.RDataFrame(S.table("s", structured)).Sum("x").GetValue() == 10, "")
check("table of a categorical column", S.table("c", pd.DataFrame({"cat": pd.Categorical(["a", "b", "a"])})).GetEntries() == 3, "")
tree, printed = captured(lambda: S.table("i", pd.DataFrame({0: [1.0], "x[2]": [1.0], "ok": [2.0], "a/b": [1.0]})))
check("table: names a branch cannot have are skipped with the reason", tree.GetNbranches() == 2 and "column 'x[2]' is skipped (a branch name cannot be empty or contain / [ ] :)" in printed, printed)
check("table of an empty DataFrame", S.table("z", pd.DataFrame({"a": np.array([], dtype=float)})).GetEntries() == 0, "")
matrix = rng.normal(size=(100, 3))
tm = S.table("m", matrix, columns=["a", "b", "c"])
check("table of a 2d array with column names", tm.GetEntries() == 100 and tm.GetNbranches() == 3 and abs(ROOT.RDataFrame(tm).Sum("b").GetValue() - matrix[:, 1].sum()) < 1e-9, "")
ti = S.table("mi", np.arange(6).reshape(3, 2), columns=["p", "q"])
check("table of a 2d integer array: integers", ti.GetBranch("q").GetLeaf("q").GetTypeName() == "Long64_t" and ROOT.RDataFrame(ti).Sum("q").GetValue() == 9, "")
check("table of rows as lists with column names", ROOT.RDataFrame(S.table("rows", [[1, 2.5], [3, 4.5]], columns=["n", "x"])).Sum("n").GetValue() == 4, "")
check("table of a 1d array and one name", S.table("one", np.arange(5.0), columns=["score"]).GetEntries() == 5, "")
raises("table: fewer names than columns", lambda: S.table("e", matrix, columns=["a", "b"]), ValueError, "3 columns, but 2 names")
raises("table: names for a dict", lambda: S.table("e", {"a": [1.0]}, columns=["b"]), TypeError, "has its own")
raises("table: names as one text", lambda: S.table("e", matrix, columns="abc"), TypeError, "list of names")
raises("table: a 3d array", lambda: S.table("e", np.ones((2, 2, 2)), columns=["a", "b"]), ValueError, "2D array")
raises("table: a 2d array without names, with a hint", lambda: S.table("e", matrix), TypeError, "columns=")
raises("table: columns of other lengths", lambda: S.table("e", {"a": [1, 2], "b": [1]}), ValueError, "same length")
raises("table of a list", lambda: S.table("e", [1, 2]), TypeError)
raises("table: nothing that can be stored", lambda: S.table("e", {"w": pd.date_range("2026-01-01", periods=2)}), ValueError, "no column that can be stored")

# the objects as data of a project
p = Project("python_helpers")
edges = np.array([-4.0, -1.0, 0.0, 0.5, 4.0])
p.pm.AddDataSource("ml", [S.table("events", frame), S.histo("weighted", x, edges, weights=w), S.histo2d("map", x, y, 10, ((-4.0, 4.0), (-4.0, 4.0))), S.graph("points", [1.0, 2.0], [3.0, 4.0], yerr=[0.1, 0.2])])
plot = S.Plot("project", "helpers", "1d")
plot[1].AddData("events", "ml").Project1D(("pt", 10, [0.0, 10.0])).Filter('det == "TPC" && isPrimary').SetShowOverflowBins()
plot[1].AddData("weighted", "ml").SetShowOverflowBins()
plot[1].AddData("points", "ml")
p.pm.AddPlot(plot)
plot = S.Plot("map", "helpers", "2d")
plot[1].AddData("map", "ml")
p.pm.AddPlot(plot)
run = p.plot()
objs = run.objects("project", "helpers")
sel = frame[(frame.det == "TPC") & frame.isPrimary]
check("a table in a project: Project1D with a filter on a string column", len(objs) == 3 and not diff_1d(objs[0], sel.pt, 10, (0.0, 10.0)), diff_1d(objs[0], sel.pt, 10, (0.0, 10.0)) if objs else "not drawn")
check("a weighted histogram in a project: drawn as filled", len(objs) == 3 and not diff_1d(objs[1], x, edges, weights=w), diff_1d(objs[1], x, edges, weights=w) if len(objs) > 1 else "not drawn")
check("a graph in a project", len(objs) == 3 and objs[2].GetN() == 2 and objs[2].GetErrorY(1) == 0.2, "")
maps = run.objects("map", "helpers")
check("a 2d histogram in a project", maps and not diff_2d(maps[0], x, y, 10, ((-4.0, 4.0), (-4.0, 4.0))), diff_2d(maps[0], x, y, 10, ((-4.0, 4.0), (-4.0, 4.0))) if maps else "not drawn")
check("the helpers' objects give no warnings or errors", not run.warnings() and not run.errors(), " | ".join(run.warnings() + run.errors()))
finish()
