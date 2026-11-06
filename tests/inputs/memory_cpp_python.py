"""In-memory inputs are the same handed over from C++ and from Python: a C++ program and a Python program define the same project
(AddDataSource of a histogram, a second object of the same name replacing it, a named list holding a histogram and a tree, a nameless list,
an open file, a vector of objects, a nameless object), each is plotted by the plot program, and both give the same drawn objects, the
values that were handed over, and the same messages while defining.
"""

import os
import shutil

import numpy as np
from srptest import PLOT, ROOT, SRP, WORK, Run, check, cpp_program, drawn_diff, finish, python, run, run_in_terminal, skip

if not (shutil.which("cmake") and (shutil.which("c++") or shutil.which("g++") or shutil.which("clang++"))):
    skip("no cmake or C++ compiler")

folder = os.path.join(WORK, "memory")
os.makedirs(folder)
fileInput = os.path.join(folder, "infile.root")
f = ROOT.TFile(fileInput, "RECREATE")
h = ROOT.TH1D("infile", "", 2, 0.0, 2.0)
h.SetBinContent(1, 11.0)
h.Write()
f.Close()
PLOTS = {"h": "mem", "lst/inlist": "mem", "lst/tree": "mem", "g": "mem", "infile": "file", "h2": "vec", "g2": "vec"}

CPP = f"""#include "SciRooPlot/PlotManager.h"

#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TList.h>
#include <TTree.h>

#include <vector>

using namespace SciRooPlot;

int main()
{{
  TH1::AddDirectory(false);
  PlotManager pm("memory_cpp");
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d"));
  TH1D first("h", "", 5, 0., 5.);
  pm.AddDataSource("mem", &first);
  TH1D h("h", "", 5, 0., 5.);
  for (int i = 1; i <= 5; ++i) {{
    h.SetBinContent(i, i);
    h.SetBinError(i, 0.5);
  }}
  pm.AddDataSource("mem", &h);
  TList named;
  named.SetName("lst");
  auto* inList = new TH1D("inlist", "", 3, 0., 3.);
  for (int i = 1; i <= 3; ++i) inList->SetBinContent(i, 6 + i);
  auto* tree = new TTree("tree", "");
  double x;
  tree->Branch("x", &x);
  for (int i = 0; i < 10; ++i) {{
    x = i + 0.5;
    tree->Fill();
  }}
  named.Add(inList);
  named.Add(tree);
  pm.AddDataSource("mem", &named);
  TList nameless;
  auto* g = new TGraph(3);
  g->SetName("g");
  for (int i = 0; i < 3; ++i) g->SetPoint(i, i + 1, (i + 1) * (i + 1));
  nameless.Add(g);
  pm.AddDataSource("mem", &nameless);
  TFile file("{fileInput}");
  pm.AddDataSource("file", &file);
  TH1D h2("h2", "", 2, 0., 2.);
  h2.SetBinContent(2, 22.);
  TGraph g2(1);
  g2.SetName("g2");
  g2.SetPoint(0, 2., 3.);
  TH1D unnamed("", "", 2, 0., 2.);
  pm.AddDataSource("vec", std::vector<TObject*>{{&h2, &g2, &unnamed}});
  int plotNumber{{}};
  for (const auto& [name, source] : std::vector<std::pair<std::string, std::string>>{{{", ".join(f'{{"{n}", "{s}"}}' for n, s in PLOTS.items())}}}) {{
    Plot plot("p" + std::to_string(plotNumber++), "memory", "1d");
    auto& data = plot[1].AddData(name, source);
    if (name == "lst/tree") data.Project1D({{"x", 10, {{0., 10.}}}});
    pm.AddPlot(plot);
  }}
  pm.SaveProject();
  return 0;
}}
"""
PY = f"""
import ROOT
from SciRooPlot import *
ROOT.TH1.AddDirectory(False)
pm = PlotManager("memory_py")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
first = ROOT.TH1D("h", "", 5, 0.0, 5.0)
pm.AddDataSource("mem", first)
h = ROOT.TH1D("h", "", 5, 0.0, 5.0)
for i in range(1, 6):
    h.SetBinContent(i, i)
    h.SetBinError(i, 0.5)
pm.AddDataSource("mem", h)
named = ROOT.TList()
named.SetName("lst")
inList = ROOT.TH1D("inlist", "", 3, 0.0, 3.0)
for i in range(1, 4):
    inList.SetBinContent(i, 6 + i)
tree = ROOT.TTree("tree", "")
import array
x = array.array("d", [0.0])
tree.Branch("x", x, "x/D")
for i in range(10):
    x[0] = i + 0.5
    tree.Fill()
named.Add(inList)
named.Add(tree)
pm.AddDataSource("mem", named)
nameless = ROOT.TList()
g = ROOT.TGraph(3)
g.SetName("g")
for i in range(3):
    g.SetPoint(i, i + 1, (i + 1) ** 2)
nameless.Add(g)
pm.AddDataSource("mem", nameless)
file = ROOT.TFile({fileInput!r})
pm.AddDataSource("file", file)
h2 = ROOT.TH1D("h2", "", 2, 0.0, 2.0)
h2.SetBinContent(2, 22.0)
g2 = ROOT.TGraph(1)
g2.SetName("g2")
g2.SetPoint(0, 2.0, 3.0)
unnamed = ROOT.TH1D("", "", 2, 0.0, 2.0)
pm.AddDataSource("vec", [h2, g2, unnamed])
for i, (name, source) in enumerate({PLOTS!r}.items()):
    plot = Plot(f"p{{i}}", "memory", "1d")
    data = plot[1].AddData(name, source)
    if name == "lst/tree":
        data.Project1D(("x", 10, [0.0, 10.0]))
    pm.AddPlot(plot)
pm.SaveProject()
"""
program, build, _ = cpp_program(os.path.join(folder, "cpp"), CPP, "memory_cpp")
check("the C++ program compiles", program is not None, build.tail(1500))
if program is None:
    finish()
script = os.path.join(folder, "py", "DefinePlots.py")
os.makedirs(os.path.dirname(script))
with open(script, "w") as fh:
    fh.write(PY)
defined = {"memory_cpp": run(program), "memory_py": python(PY)}
for name, out in defined.items():
    check(f"{name}: the program runs", out.rc == 0, out.tail())


def messages(out):
    return sorted(line.split("]", 1)[1].strip() for line in out.text.splitlines() if "[ WARN" in line or "[ ERR" in line)


check("C++ and Python give the same messages while defining", messages(defined["memory_cpp"]) == messages(defined["memory_py"]), f"{messages(defined['memory_cpp'])} vs {messages(defined['memory_py'])}")
check("the replaced object and the nameless object are reported", len(messages(defined["memory_py"])) == 2, str(messages(defined["memory_py"])))
runs = {}
for name, prog in (("memory_cpp", program), ("memory_py", script)):
    outdir = os.path.join(folder, f"{name}_out")
    run(SRP, "add", name, prog, outdir)
    run(SRP, "select", name)
    runs[name] = Run(run_in_terminal([PLOT, "memory", ".+", "file"], 600), os.path.join(outdir, "Plots.root"), os.path.join(folder, f"{name}.root"))
    check(f"{name}: plotted without errors or warnings", runs[name].file and not runs[name].errors() and not runs[name].warnings(), runs[name].log[-500:])


def hist(nb, lo, hi, contents, error=None):
    ref = ROOT.TH1D("ref", "", nb, lo, hi)
    for i, c in enumerate(contents):
        ref.SetBinContent(i + 1, c)
        if error is not None:
            ref.SetBinError(i + 1, error)
    return ref


EXPECTED = {
    "h": hist(5, 0.0, 5.0, [1, 2, 3, 4, 5], 0.5),
    "lst/inlist": hist(3, 0.0, 3.0, [7, 8, 9]),
    "lst/tree": hist(10, 0.0, 10.0, [1] * 10),
    "g": ROOT.TGraph(3, np.array([1.0, 2.0, 3.0]), np.array([1.0, 4.0, 9.0])),
    "infile": hist(2, 0.0, 2.0, [11, 0]),
    "h2": hist(2, 0.0, 2.0, [0, 22]),
    "g2": ROOT.TGraph(1, np.array([2.0]), np.array([3.0])),
}
for i, name in enumerate(PLOTS):
    cpp, py = runs["memory_cpp"].objects(f"p{i}", "memory"), runs["memory_py"].objects(f"p{i}", "memory")
    diff = drawn_diff(py, cpp)
    check(f"{name}: C++ as Python", py and not diff, diff or "not drawn")
    diff = drawn_diff([EXPECTED[name]], py)
    check(f"{name}: the values handed over", not diff, diff)
finish()
