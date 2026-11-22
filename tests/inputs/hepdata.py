"""A HEPData record as input (hepdata:<record>) gives the values of its tables with the errors as in the record: y<i> with the total
errors, y<i>_<label> with one error source; a histogram where the table has one point per bin and every error is symmetric (a 1d and a
2d table, bin labels kept), a graph otherwise (an asymmetric error source, a table of points without bins). HEPData's own objects stay
readable, a data source restricted to a table works. What the record's ROOT file does not hold is reported: the error sources of tables
without bins and of bins that overlap, tables with three independent variables. The record is downloaded once into the cache folder, also
with srp cache off: the one of the config folder, or the folder SciRooPlot inside the folder of srp cache on <folder>, marked as cache
(CACHEDIR.TAG); a record that does not exist and a malformed record id are reported once and the other inputs are still read.
The record is served by a local web server as hepdata.net serves its ROOT download (a .tar.gz holding the ROOT file).
Covers: an error label with characters not allowed in data names; two labels that become the same name; an asymmetric error with both
shifts on one side; a table with two dependent variables of different points; a 2d table of points without bins; text bins with a bin
without value; axis titles of a 2d table; a table name with a colon; a bare ROOT file as download (a mirror); a download that is no ROOT
file or holds no tables.
"""

import array
import functools
import http.server
import itertools
import math
import os
import shutil
import tarfile
import threading

from srptest import ROOT, Project, S, check, close, finish, missing

served = os.path.join(os.environ.get("SRP_TESTS_WORK", "."), "hepdata-server")
shutil.rmtree(served, ignore_errors=True)
os.makedirs(os.path.join(served, "record"))
requests = []


class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        requests.append(self.path)


server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(Handler, directory=served))
threading.Thread(target=server.serve_forever, daemon=True).start()
os.environ["SCIROOPLOT_HEPDATA_SERVER"] = f"http://127.0.0.1:{server.server_address[1]}"
os.environ["NO_PROXY"] = os.environ["no_proxy"] = "127.0.0.1"
keep = []
DOI = "doi:10.17182/hepdata.12345.v3/t{}"


def write_table(out, index, values, errors, edges=None, labels=None, variable=1, bins=None, name=None):
    """dependent variable y<variable> of a 1d table as hepdata.net writes it: errors [(label, symmetric errors or (up, down))] per point;
    edges None: points without bins; a value None: a bin without value (left out of the graph, an empty bin of the histograms); bins
    [(low, high)] per point where they overlap (the histograms then have the edges of all bins, each point filled at its centre)"""
    name = name or f"Table {index}"
    (out.GetDirectory(name) or out.mkdir(name)).cd()
    n = len(values)
    if bins:
        edges = sorted({e for b in bins for e in b})
        xs, widths = [0.5 * (a + b) for a, b in bins], [0.5 * (b - a) for a, b in bins]
    else:
        xs = [0.5 * (a + b) for a, b in itertools.pairwise(edges)] if edges else [float(i + 1) for i in range(n)]
        widths = [0.5 * (b - a) for a, b in itertools.pairwise(edges)] if edges else [0.0] * n
    kept = [i for i in range(n) if values[i] is not None]

    def hist(name, title, contents):
        h = ROOT.TH1F(name, DOI.format(index), len(edges) - 1, array.array("d", edges))
        h.GetXaxis().SetTitle("p_T [GeV]")
        for i, label in enumerate(labels or []):
            h.GetXaxis().SetBinLabel(i + 1, label)
        h.GetYaxis().SetTitle(title)
        for i in kept:
            h.Fill(xs[i], contents[i])  # as hepdata.net: the error of a bin is then its content
        h.Write()
        keep.append(h)

    if edges:
        hist(f"Hist1D_y{variable}", "yield", values)
    low, high = [0.0] * n, [0.0] * n
    for k, (label, error) in enumerate(errors, 1):
        up, down = error if isinstance(error, tuple) else (error, [-e for e in error])
        if edges and isinstance(error, tuple):
            hist(f"Hist1D_y{variable}_e{k}plus", label.replace(" ", "_") + "_plus", up)
            hist(f"Hist1D_y{variable}_e{k}minus", label.replace(" ", "_") + "_minus", down)
        elif edges:
            hist(f"Hist1D_y{variable}_e{k}", label.replace(" ", "_"), up)
        for i in range(n):
            low[i] += min(up[i], down[i], 0.0) ** 2
            high[i] += max(up[i], down[i], 0.0) ** 2
    columns = (xs, values, widths, widths, [math.sqrt(v) for v in low], [math.sqrt(v) for v in high])
    g = ROOT.TGraphAsymmErrors(len(kept), *(array.array("d", [c[i] for i in kept]) for c in columns))
    g.SetNameTitle(f"Graph1D_y{variable}", DOI.format(index))
    g.GetXaxis().SetTitle("p_T [GeV]")
    g.GetYaxis().SetTitle("yield")
    g.Write()
    keep.append(g)
    out.cd()


def write_map(out, index, values, stat):
    """a 2d table of 2 x 3 bins with one error source"""
    out.mkdir(f"Table {index}").cd()
    xEdges, yEdges = array.array("d", [0, 1, 2]), array.array("d", [0, 1, 2, 3])
    points = [(x + 0.5, y + 0.5) for x in range(2) for y in range(3)]
    for name, title, contents in (("Hist2D_y1", "z", values), ("Hist2D_y1_e1", "stat", stat)):
        h = ROOT.TH2F(name, DOI.format(index), 2, xEdges, 3, yEdges)
        h.GetXaxis().SetTitle("x [cm]")
        h.GetYaxis().SetTitle("y [cm]")
        h.GetZaxis().SetTitle(title)  # the graph of a 2d table has no axis titles in hepdata.net's file
        for (x, y), c in zip(points, contents):
            h.Fill(x, y, c)
        h.Write()
        keep.append(h)
    n = len(points)
    columns = ([p[0] for p in points], [p[1] for p in points], values, [0.5] * n, [0.5] * n, stat)
    g = ROOT.TGraph2DErrors(n, *(array.array("d", v) for v in columns))
    g.SetNameTitle("Graph2D_y1", DOI.format(index))
    g.SetDirectory(ROOT.nullptr)
    g.Write()
    keep.append(g)
    out.cd()


def write_points2d(out, index, values, errors):
    """a 2d table of points without bins: hepdata.net writes the graph alone"""
    out.mkdir(f"Table {index}").cd()
    n = len(values)
    columns = ([1.0, 2.0, 3.0][:n], [10.0, 20.0, 30.0][:n], values, [0.0] * n, [0.0] * n, errors)
    g = ROOT.TGraph2DErrors(n, *(array.array("d", v) for v in columns))
    g.SetNameTitle("Graph2D_y1", DOI.format(index))
    g.SetDirectory(ROOT.nullptr)
    g.Write()
    keep.append(g)
    out.cd()


def write_table3d(out, index):
    """a table with three independent variables: hepdata.net writes 3d histograms only"""
    out.mkdir(f"Table {index}").cd()
    for name in ("Hist3D_y1", "Hist3D_y1_e1"):
        h = ROOT.TH3F(name, DOI.format(index), 2, 0, 2, 2, 0, 2, 2, 0, 2)
        h.Fill(0.5, 0.5, 0.5, 1.0)
        h.Write()
        keep.append(h)
    out.cd()


rootFile = os.path.join(served, "HEPData-ins12345-v3-root.root")
out = ROOT.TFile(rootFile, "RECREATE")
write_table(out, 1, [10.0, 5.0, 2.0], [("stat", [1.0, 0.5, 0.25]), ("sys", [2.0, 1.0, 0.5])], edges=[0, 1, 2, 4])
write_table(out, 2, [3.0, 2.0], [("stat", [0.25, 0.5]), ("sys,lumi", ([0.5, 0.25], [-0.25, -0.125]))], edges=[0, 1, 2])
write_table(out, 3, [1.0, 2.0], [("stat", [0.125, 0.25])], edges=[0.5, 1.5, 2.5], labels=["pp", "PbPb"])
write_table(out, 4, [4.0, 6.0], [("stat", [0.5, 0.5]), ("sys", [1.0, 1.0])])
write_map(out, 5, [1.0, 2.0, 3.0, 4.0, 5.0, 6.0], [0.5, 0.25, 0.125, 0.5, 0.25, 0.125])
# an asymmetric error with both shifts on one side, two labels that become the same name; a second dependent variable with its own
# points (hepdata.net leaves out the points of a variable that has no value there)
write_table(out, 6, [1.0, 2.0, 3.0], [("stat", [0.125, 0.25, 0.5]), ("a,b", ([-0.25, 0.5, 0.25], [0.125, -0.25, 0.0])), ("a b", [0.5, 0.5, 0.5])], edges=[0, 1, 2, 3])
write_table(out, 6, [4.0, 5.0], [("stat", [0.5, 0.25])], edges=[1, 2, 3], variable=2)
write_points2d(out, 7, [1.0, 2.0, 3.0], [0.5, 0.25, 0.125])
write_table(out, 8, [3.0, 7.0, 11.0], [("stat", [0.125, 0.25, 0.5])], bins=[(-0.5, 0.5), (-1.0, 1.0), (-1.5, 1.5)])
write_table(out, 9, [1.0, None, 3.0], [("stat", [0.125, 0.0, 0.25])], edges=[0.5, 1.5, 2.5, 3.5], labels=["a", "b", "c"])
write_table3d(out, 10)
write_table(out, 11, [2.0, 4.0], [("stat", [0.25, 0.5])], edges=[0, 1, 2], name="Table 11: left")
out.Close()
with tarfile.open(os.path.join(served, "record", "ins12345"), "w:gz") as archive:
    archive.add(rootFile, arcname=os.path.basename(rootFile))
shutil.copy(rootFile, os.path.join(served, "record", "insbare"))
with open(os.path.join(served, "record", "insbroken"), "w") as broken:
    broken.write("<html>maintenance</html>")
empty = ROOT.TFile(os.path.join(served, "record", "insnotables"), "RECREATE")
ROOT.TNamed("info", "no tables").Write()
empty.Close()

p = Project("hepdata")
p.pm.AddDataSource("ref", "hepdata:ins12345")
p.pm.AddDataSource("table2", "hepdata:ins12345:Table 2")
p.pm.AddDataSource("colon", "hepdata:ins12345:Table 11: left")
p.pm.AddDataSource("bare", "hepdata:insbare")
p.pm.AddDataSource("nothing", ["hepdata:ins99999", p.path("local.root")])
p.pm.AddDataSource("nothing2", ["hepdata:ins99999", p.path("local.root")])
p.pm.AddDataSource("malformed", ["hepdata:ins 1", p.path("local.root")])
p.pm.AddDataSource("broken", ["hepdata:insbroken", p.path("local.root")])
p.pm.AddDataSource("notables", ["hepdata:insnotables", p.path("local.root")])
local = ROOT.TH1D("h", "", 1, 0.0, 1.0)
local.SetBinContent(1, 7.0)
lf = ROOT.TFile(p.path("local.root"), "RECREATE")
local.Write()
lf.Close()

# (what, data name, data source, expected class, values, errors below, errors above; 2d: values and errors per bin x, y)
expected = [
    ("the total errors of a binned table with symmetric errors: a histogram", "Table 1/y1", "ref", "TH1D", [10, 5, 2], [5**0.5, 1.25**0.5, 0.3125**0.5], None),
    ("an error source of a binned table: a histogram with that error", "Table 1/y1_stat", "ref", "TH1D", [10, 5, 2], [1, 0.5, 0.25], None),
    ("another error source of a binned table", "Table 1/y1_sys", "ref", "TH1D", [10, 5, 2], [2, 1, 0.5], None),
    ("the total errors of a table with an asymmetric error: a graph", "Table 2/y1", "ref", "TGraphAsymmErrors", [3, 2], [(0.25**2 + 0.25**2) ** 0.5, (0.5**2 + 0.125**2) ** 0.5], [(0.25**2 + 0.5**2) ** 0.5, (0.5**2 + 0.25**2) ** 0.5]),
    ("a symmetric error source of a table with an asymmetric one: a graph too", "Table 2/y1_stat", "ref", "TGraphAsymmErrors", [3, 2], [0.25, 0.5], [0.25, 0.5]),
    ("an asymmetric error source, its label in allowed characters", "Table 2/y1_sys_lumi", "ref", "TGraphAsymmErrors", [3, 2], [0.25, 0.125], [0.5, 0.25]),
    ("a data source restricted to a table", "y1_stat", "table2", "TGraphAsymmErrors", [3, 2], [0.25, 0.5], [0.25, 0.5]),
    ("a table with bin labels: a histogram with the labels", "Table 3/y1_stat", "ref", "TH1D", [1, 2], [0.125, 0.25], None),
    ("a table of points without bins: a graph with the total errors", "Table 4/y1", "ref", "TGraphAsymmErrors", [4, 6], [1.25**0.5, 1.25**0.5], [1.25**0.5, 1.25**0.5]),
    ("a 2d table: a 2d histogram with the error source", "Table 5/y1_stat", "ref", "TH2D", [1, 2, 3, 4, 5, 6], [0.5, 0.25, 0.125, 0.5, 0.25, 0.125], None),
    ("HEPData's own objects stay readable", "Table 1/Graph1D_y1", "ref", "TGraphAsymmErrors", [10, 5, 2], [5**0.5, 1.25**0.5, 0.3125**0.5], [5**0.5, 1.25**0.5, 0.3125**0.5]),
    ("a record that does not exist: the other inputs are read", "h", "nothing", "TH1D", [7], [7**0.5], None),
    ("a malformed record id: the other inputs are read", "h", "malformed", "TH1D", [7], [7**0.5], None),
    ("a download that is no ROOT file: the other inputs are read", "h", "broken", "TH1D", [7], [7**0.5], None),
    ("a download without tables: the other inputs are read", "h", "notables", "TH1D", [7], [7**0.5], None),
    ("an asymmetric error with both shifts on one side: below and above as their signs say", "Table 6/y1_a_b", "ref", "TGraphAsymmErrors", [1, 2, 3], [0.25, 0.25, 0], [0.125, 0.5, 0.25]),
    ("a label that becomes the name of another source gets the number of its own", "Table 6/y1_a_b_3", "ref", "TGraphAsymmErrors", [1, 2, 3], [0.5] * 3, [0.5] * 3),
    ("a second dependent variable with its own points: a histogram of them", "Table 6/y2_stat", "ref", "TH1D", [4, 5], [0.5, 0.25], None),
    ("a 2d table of points without bins: a 2d graph with the total errors", "Table 7/y1", "ref", "TGraph2DErrors", [1, 2, 3], [0.5, 0.25, 0.125], None),
    ("bins that overlap: a graph with the total errors", "Table 8/y1", "ref", "TGraphAsymmErrors", [3, 7, 11], [0.125, 0.25, 0.5], [0.125, 0.25, 0.5]),
    ("text bins with a bin without value: a histogram of the bins with values", "Table 9/y1_stat", "ref", "TH1D", [1, 3], [0.125, 0.25], None),
    ("a table name with a colon: the colon is _ in the data name", "Table 11_ left/y1_stat", "ref", "TH1D", [2, 4], [0.25, 0.5], None),
    ("a data source restricted to a table with a colon in its name", "y1_stat", "colon", "TH1D", [2, 4], [0.25, 0.5], None),
    ("a bare ROOT file as download (a mirror)", "Table 1/y1_stat", "bare", "TH1D", [10, 5, 2], [1, 0.5, 0.25], None),
    ("a record that does not exist in a second data source: the other inputs are read", "h", "nothing2", "TH1D", [7], [7**0.5], None),
]
# the labels of the drawn x axis by the data name of a case
labelled = {"Table 3/y1_stat": ["pp", "PbPb"], "Table 9/y1_stat": ["a", "c"]}
for i, (what, name, source, cls, values, low, high) in enumerate(expected):
    plot = S.Plot(f"h{i}", "hepdata", "2d" if cls in ("TH2D", "TGraph2DErrors") else "1d")
    plot[1].AddData(name, source)
    p.pm.AddPlot(plot)


def values_of(obj):
    """values and errors below and above: per bin of a histogram (2d: x outer, y inner), per point of a graph"""
    if obj.InheritsFrom("TH2"):
        bins = [obj.GetBin(x, y) for x in (1, 2) for y in (1, 2, 3)]
        return [obj.GetBinContent(b) for b in bins], [obj.GetBinError(b) for b in bins], [obj.GetBinError(b) for b in bins]
    if obj.InheritsFrom("TH1"):
        bins = range(1, obj.GetNbinsX() + 1)
        return [obj.GetBinContent(b) for b in bins], [obj.GetBinError(b) for b in bins], [obj.GetBinError(b) for b in bins]
    n = obj.GetN()
    if obj.InheritsFrom("TGraph2D"):
        return [obj.GetZ()[i] for i in range(n)], [obj.GetEZ()[i] for i in range(n)], [obj.GetEZ()[i] for i in range(n)]
    return [obj.GetPointY(i) for i in range(n)], [obj.GetErrorYlow(i) for i in range(n)], [obj.GetErrorYhigh(i) for i in range(n)]


def check_values(run, label=""):
    for i, (what, name, source, cls, values, low, high) in enumerate(expected):
        drawn = run.objects(f"h{i}", "hepdata")
        if len(drawn) != 1:
            check(what + label, False, f"not drawn; missing {sorted(missing(run))[:4]}")
            continue
        got = values_of(drawn[0])
        want = (values, low, high or low)
        same = all(len(g) == len(w) and all(close(a, b, 1e-6) for a, b in zip(g, w)) for g, w in zip(got, want))
        check(what + label, drawn[0].ClassName() == cls and same, f"{drawn[0].ClassName()} {got}, expected {cls} {want}")
        if name in labelled and source == "ref":
            axis = drawn[0].GetXaxis()
            labels = [axis.GetBinLabel(b) for b in range(1, axis.GetNbins() + 1)]
            check(what + ": the labels of the bins" + label, labels == labelled[name], f"{labels}")


run = p.plot("hepdata")
check_values(run)
downloads = [r for r in requests if r.startswith("/record/ins12345")]
check("the record is downloaded once", len(downloads) == 1, f"requests {requests}")
stored = [os.path.join(root, f) for root, _, files in os.walk(os.environ["SCIROOPLOT_CONFIG_PATH"]) for f in files if f == "ins12345.root"]
standard = os.path.join(os.environ["SCIROOPLOT_CONFIG_PATH"], "cache")
check("the record is kept in the cache folder of the config folder (srp cache off)", stored == [os.path.join(standard, "hepdata", "ins12345.root")], f"{stored}")
check("the cache folder of the config folder is marked as cache", os.path.exists(os.path.join(standard, "CACHEDIR.TAG")), f"{os.listdir(standard) if os.path.isdir(standard) else 'no folder'}")
lines = run.lines("WARN")
for what, text in (
    ("a record that does not exist is reported", "Cannot download HEPData record ins99999"),
    ("a malformed record id is reported", "Skipping input hepdata:ins 1"),
    ("a download that is no ROOT file is reported", "Cannot read the download of HEPData record insbroken: it is no readable ROOT file"),
    ("a download without tables is reported", "Cannot use HEPData record insnotables: its download contains no tables"),
    ("error sources of bins that overlap are reported", "HEPData record ins12345: no objects per error source for Table 8 (y1)"),
    ("a table with three independent variables is reported", "HEPData record ins12345: Table 10 not converted"),
):
    found = [line for line in lines if text in line]
    check(f"{what}, once", len(found) == 1, f"{len(found)} lines: {lines[:3]}")
infos = run.lines("INFO")
for what, text in (
    ("tables without bins are named: no error sources", "HEPData record ins12345: Table 4, Table 7 have no bins"),
    ("a table name with a colon is named with its data name", "HEPData record ins12345: table 'Table 11: left' is 'Table 11_ left'"),
):
    check(what, len([line for line in infos if text in line]) == 1, f"{[line for line in infos if 'HEPData' in line][:4]}")
copy = ROOT.TFile.Open(stored[0]) if stored else None
titles = [copy.Get("Table 1/y1").GetTitle()] if copy else []
check("the objects keep the DOI of their table as title", titles == [DOI.format(1)], f"{titles}")
y1 = copy.Get("Table 5/y1") if copy else None
axes = [y1.GetXaxis().GetTitle(), y1.GetYaxis().GetTitle(), y1.GetZaxis().GetTitle()] if y1 else []
check("a 2d table: the axis titles of its histograms", axes == ["x [cm]", "y [cm]", "z"], f"{axes}")
made = sorted(k.GetName() for k in copy.Get("Table 8").GetListOfKeys() if k.GetName().startswith("y")) if copy else []
check("bins that overlap: no objects per error source", made == ["y1"], f"{made}")

# a second run reads the kept copy; with a cache folder the record is downloaded into it
requests.clear()
run = p.plot("hepdata")
check_values(run, " (the kept copy)")
check("the kept copy is read without a download", not [r for r in requests if r.startswith("/record/ins12345")], f"requests {requests}")
cache = p.path("cache")
p.srp("cache", "on", cache)
run = p.plot("hepdata")
p.srp("cache", "off")
check_values(run, " (cache folder)")
own = os.path.join(cache, "SciRooPlot")
check("with srp cache on <folder> the record is downloaded into the folder SciRooPlot inside it", os.path.exists(os.path.join(own, "hepdata", "ins12345.root")), f"{os.listdir(cache) if os.path.isdir(cache) else 'no folder'}")
check("the folder SciRooPlot inside it is marked as cache, nothing else is put into <folder>", os.path.exists(os.path.join(own, "CACHEDIR.TAG")) and os.listdir(cache) == ["SciRooPlot"], f"{os.listdir(cache) if os.path.isdir(cache) else 'no folder'}")
server.shutdown()
finish()
