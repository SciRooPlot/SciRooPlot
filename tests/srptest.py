"""Shared building blocks of the SciRooPlot tests.

A test file defines its plots through the Python interface in one isolated project, runs the plot program once, reads the drawn
objects back from Plots.root and checks them against an independent reference (plain ROOT, an invariant, the requested values).
It exits 0 if every check passes, 1 if one fails and 77 if it cannot run (shown as skipped by CTest).

Environment (set by CTest): SRP_PREFIX, a build or install directory with bin/plot; SRP_TESTS_WORK, the scratch folder of the test;
SRP_TESTS_FULL=1 for the full case lists (weekly), otherwise a subset holding every value of every dimension at least once.
"""

import glob
import itertools
import os
import re
import shutil
import subprocess
import sys
import warnings

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
def cases(dims, valid=lambda case: True, named=()):
    """The combinations of the values in dims (a dict of lists) as dicts, without those that are not valid.
    The fast run takes those needed to have every value of every dimension at least once; the full run all of them.
    Named cases (dicts with a "why": a bug that happened) are always included."""
    allCases = [dict(zip(dims, values)) for values in itertools.product(*dims.values())]
    allCases = [case for case in allCases if valid(case)]
    chosen = allCases
    if not FULL:
        chosen, seen = [], set()
        for case in allCases:
            pairs = set(case.items())
            if not pairs <= seen:
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
# 1d data on x from 1 to 10 with values from about 1 to 10 (positive: usable on log axes)
DATA_1D = ("hist", "hist_var", "profile", "graph", "graph_errors", "graph_asymm", "function")
DATA_2D = ("hist2d", "function2d")
HISTS = ("hist", "hist_var", "profile", "hist2d")


def _value(x):
    return 1.0 + (x - 1.0) * (1.0 - 0.3 * ((x - 5.5) / 4.5) ** 2)


def make_inputs(path):
    """writes one object of every data kind into path (the function kinds need no input)"""
    f = ROOT.TFile(path, "RECREATE")
    h = ROOT.TH1D("hist", "", 18, 1.0, 10.0)
    edges = ROOT.std.vector("double")([1.0, 1.2, 1.5, 2.0, 2.7, 3.5, 4.5, 6.0, 7.5, 9.0, 10.0])
    hv = ROOT.TH1D("hist_var", "", edges.size() - 1, edges.data())
    p = ROOT.TProfile("profile", "", 18, 1.0, 10.0)
    for hist in (h, hv):
        for b in range(1, hist.GetNbinsX() + 1):
            hist.SetBinContent(b, _value(hist.GetBinCenter(b)))
            hist.SetBinError(b, 0.1 * _value(hist.GetBinCenter(b)))
    for b in range(1, p.GetNbinsX() + 1):
        x = p.GetBinCenter(b)
        p.Fill(x, 0.9 * _value(x))
        p.Fill(x, 1.1 * _value(x))
    xs = [1.0 + 0.5 * i for i in range(19)]
    g = ROOT.TGraph(len(xs))
    ge = ROOT.TGraphErrors(len(xs))
    ga = ROOT.TGraphAsymmErrors(len(xs))
    for i, x in enumerate(xs):
        for graph in (g, ge, ga):
            graph.SetPoint(i, x, _value(x))
        ge.SetPointError(i, 0.2, 0.1 * _value(x))
        ga.SetPointError(i, 0.2, 0.2, 0.05 * _value(x), 0.15 * _value(x))
    h2 = ROOT.TH2D("hist2d", "", 18, 1.0, 10.0, 18, 1.0, 10.0)
    for i in range(1, 19):
        for j in range(1, 19):
            h2.SetBinContent(i, j, _value(h2.GetXaxis().GetBinCenter(i)) * _value(h2.GetYaxis().GetBinCenter(j)))
    for obj, name in ((h, "hist"), (hv, "hist_var"), (p, "profile"), (g, "graph"), (ge, "graph_errors"), (ga, "graph_asymm"), (h2, "hist2d")):
        obj.Write(name)
    f.Close()


def add_data(pad, kind, source="inputs"):
    """adds data of a kind (see make_inputs) to a pad and returns it"""
    if kind == "function":
        return pad.AddFunction("1 + (x - 1) * (1 - 0.3 * ((x - 5.5) / 4.5)^2)").SetRangeX(1.0, 10.0)
    if kind == "function2d":
        return pad.AddFunction("x * y").SetRangeX(1.0, 10.0).SetRangeY(1.0, 10.0)
    return pad.AddData(kind, source)


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

    def frame(self, name, group, pad=1):
        """the frame of a pad in axis values: {"X": (min, max), "Y": (min, max)}, or None if the plot was not made"""
        padObj = self.pad(name, group, pad)
        if not padObj:
            return None
        x = (padObj.GetUxmin(), padObj.GetUxmax())
        y = (padObj.GetUymin(), padObj.GetUymax())
        return {"X": tuple(10**v for v in x) if padObj.GetLogx() else x, "Y": tuple(10**v for v in y) if padObj.GetLogy() else y}


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

    def inputs(self):
        """the data source "inputs" with one object of every data kind"""
        make_inputs(self.path("inputs.root"))
        self.pm.AddDataSource("inputs", self.path("inputs.root"))

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
