"""A HEPData record as input (hepdata:<record>) gives the values of its tables with the errors as in the record: y<i> with the total
errors, y<i>_<label> with one error source; a histogram where the table has one point per bin and every error is symmetric (a 1d and a
2d table, bin labels kept), a graph otherwise (an asymmetric error source, a table of points without bins). HEPData's own objects stay
readable, a data source restricted to a table works. The record is downloaded once into the project's folder, or into the cache folder
when one is set; a record that does not exist and a malformed record id are reported once and the other inputs are still read.
The record is served by a local web server in the layout of hepdata.net's ROOT download.
Covers: an error label with characters not allowed in data names.
"""

import array
import functools
import http.server
import math
import os
import shutil
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


def write_table(out, index, values, errors, edges=None, labels=None):
    """a 1d table as hepdata.net writes it: errors [(label, symmetric errors or (up, down))]; edges None: points without bins"""
    out.mkdir(f"Table {index}").cd()
    n = len(values)
    xs = [0.5 * (a + b) for a, b in zip(edges[:-1], edges[1:])] if edges else [float(i + 1) for i in range(n)]

    def hist(name, title, contents):
        h = ROOT.TH1F(name, DOI.format(index), len(edges) - 1, array.array("d", edges))
        h.GetXaxis().SetTitle("p_T [GeV]")
        for i, label in enumerate(labels or []):
            h.GetXaxis().SetBinLabel(i + 1, label)
        h.GetYaxis().SetTitle(title)
        for x, c in zip(xs, contents):
            h.Fill(x, c)  # as hepdata.net: the error of a bin is then its content
        h.Write()
        keep.append(h)

    if edges:
        hist("Hist1D_y1", "yield", values)
    low, high = [0.0] * n, [0.0] * n
    for k, (label, error) in enumerate(errors, 1):
        up, down = error if isinstance(error, tuple) else (error, [-e for e in error])
        if edges and isinstance(error, tuple):
            hist(f"Hist1D_y1_e{k}plus", label.replace(" ", "_") + "_plus", up)
            hist(f"Hist1D_y1_e{k}minus", label.replace(" ", "_") + "_minus", down)
        elif edges:
            hist(f"Hist1D_y1_e{k}", label.replace(" ", "_"), up)
        for i in range(n):
            low[i] += min(up[i], down[i], 0.0) ** 2
            high[i] += max(up[i], down[i], 0.0) ** 2
    widths = [0.5 * (b - a) for a, b in zip(edges[:-1], edges[1:])] if edges else [0.0] * n
    g = ROOT.TGraphAsymmErrors(n, *(array.array("d", v) for v in (xs, values, widths, widths, [math.sqrt(v) for v in low], [math.sqrt(v) for v in high])))
    g.SetNameTitle("Graph1D_y1", DOI.format(index))
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
        h.GetZaxis().SetTitle(title)
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


out = ROOT.TFile(os.path.join(served, "record", "ins12345"), "RECREATE")
write_table(out, 1, [10.0, 5.0, 2.0], [("stat", [1.0, 0.5, 0.25]), ("sys", [2.0, 1.0, 0.5])], edges=[0, 1, 2, 4])
write_table(out, 2, [3.0, 2.0], [("stat", [0.25, 0.5]), ("sys,lumi", ([0.5, 0.25], [-0.25, -0.125]))], edges=[0, 1, 2])
write_table(out, 3, [1.0, 2.0], [("stat", [0.125, 0.25])], edges=[0.5, 1.5, 2.5], labels=["pp", "PbPb"])
write_table(out, 4, [4.0, 6.0], [("stat", [0.5, 0.5]), ("sys", [1.0, 1.0])])
write_map(out, 5, [1.0, 2.0, 3.0, 4.0, 5.0, 6.0], [0.5, 0.25, 0.125, 0.5, 0.25, 0.125])
out.Close()

p = Project("hepdata")
p.pm.AddDataSource("ref", "hepdata:ins12345")
p.pm.AddDataSource("table2", "hepdata:ins12345:Table 2")
p.pm.AddDataSource("nothing", ["hepdata:ins99999", p.path("local.root")])
p.pm.AddDataSource("malformed", ["hepdata:ins 1", p.path("local.root")])
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
]
for i, (what, name, source, cls, values, low, high) in enumerate(expected):
    plot = S.Plot(f"h{i}", "hepdata", "2d" if cls == "TH2D" else "1d")
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


run = p.plot("hepdata")
check_values(run)
downloads = [r for r in requests if r.startswith("/record/ins12345")]
check("the record is downloaded once", len(downloads) == 1, f"requests {requests}")
stored = [os.path.join(root, f) for root, _, files in os.walk(os.environ["SCIROOPLOT_CONFIG_PATH"]) for f in files if f == "ins12345.root"]
check("the record is kept in the folder of the project", len(stored) == 1 and os.sep + "hepdata" + os.sep in stored[0], f"{stored}")
lines = run.lines("WARN")
for what, text in (
    ("a record that does not exist is reported", "Cannot download HEPData record ins99999"),
    ("a malformed record id is reported", "Skipping input hepdata:ins 1"),
):
    found = [line for line in lines if text in line]
    check(f"{what}, once", len(found) == 1, f"{len(found)} lines: {lines[:3]}")
titles = [ROOT.TFile.Open(f).Get("Table 1/y1").GetTitle() for f in stored]
check("the objects keep the DOI of their table as title", titles == [DOI.format(1)], f"{titles}")

# a second run reads the kept copy; with a cache folder the record is downloaded into it
requests.clear()
run = p.plot("hepdata")
check_values(run, " (the kept copy)")
check("the kept copy is read without a download", not [r for r in requests if r.startswith("/record/ins12345")], f"requests {requests}")
cache = p.path("cache")
p.srp("cachedir", cache)
run = p.plot("hepdata")
p.srp("cachedir", "off")
check_values(run, " (cache folder)")
check("with a cache folder the record is downloaded into it", os.path.exists(os.path.join(cache, "hepdata", "ins12345.root")), f"{os.listdir(cache) if os.path.isdir(cache) else 'no folder'}")
server.shutdown()
finish()
