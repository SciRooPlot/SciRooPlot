"""Plots made again and again in one process leave no ROOT objects behind: a program generates the same plots several times in a row
(every data kind read from a file and handed over in memory in a list, data in every collection class in a file by path and by search, a ratio, a multi-panel plot with a colour-scale cell, legends,
texts and shapes), clearing the data buffer and the canvases after each run, in the modes file, data and png; from the
second run on the number of objects of every ROOT class stays the same (ROOT's object table, Root.ObjectStat).
Not counted: what ROOT keeps of every directory written to a file (one identifier string and list entry per directory at most). Not
covered: trees (ROOT's multi-threaded event loops over chains keep objects of every loop, about two of four classes per thread).
"""

import json

import srptest as S
from srptest import ROOT, check, finish

RUNS = 4
p = S.Project("repeated")  # the folder and the inputs; the program runs in a process of its own
inputs = p.path("inputs.root")
S.write_objects(inputs, S.data_objects())
containers = ROOT.TFile(p.path("containers.root"), "RECREATE")  # data in every collection class in a directory, by path and by search (from the top of the file, another data source)
folder = containers.mkdir("dir")
for kind in ("TList", "THashList", "TObjArray", "TOrdCollection", "TSortedList", "THashTable", "TFolder", "TMap"):
    outer, inner = S.collection(kind, kind), S.collection("TList", "inner")
    S.put(inner, S.own(S.data_objects()["hist"].Clone(f"in_{kind}")))
    S.put(outer, inner)
    folder.cd()
    outer.Write(kind, ROOT.TObject.kSingleKey)
containers.Close()
with open(p.path(".rootrc"), "w") as f:
    f.write("Root.ObjectStat: 1\n")  # read from the working directory when ROOT starts

lines = []
for kind, (dim, _, props) in S.DATA_KINDS.items():
    base = "1d" if dim == 1 else "2d"
    if "formula" in props:
        add = f"AddFunction({S.formula(kind)!r}).SetRangeX(1., 10.)" + (".SetRangeY(1., 10.)" if dim == 2 else "")
        lines.append(f'p = Plot("{kind}", "g", "{base}")\np[1].{add}\npm.AddPlot(p)')
        continue
    project = (".Project([0])" if dim == 1 else ".Project([0, 1])") if "projected" in props else ""
    lines.append(f'p = Plot("{kind}", "g", "{base}")\np[1].AddData("{kind}", "inputs"){project}.SetLegendLabel("<name>")\np[1].AddLegend()\npm.AddPlot(p)')
    lines.append(f'p = Plot("{kind}_memory", "g", "{base}")\np[1].AddData("{kind}", "memory"){project}\npm.AddPlot(p)')
lines.append('p = Plot("containers", "g", "1d")\n' + "\n".join(f'p[1].AddData("dir/{k}/inner/in_{k}", "containers")\np[1].AddData("in_{k}", "searched")' for k in ("TList", "THashList", "TObjArray", "TOrdCollection", "TSortedList", "THashTable", "TFolder", "TMap")) + "\npm.AddPlot(p)")
lines.append('p = Plot("ratio", "g", "1d_ratio")\np[1].AddData("hist", "inputs")\np[1].AddData("graph", "inputs")\np[2].AddRatio(Data("hist", "inputs"), Data("graph", "inputs"))\npm.AddPlot(p)')
lines.append('p = Plot("grid", "g", "2d_2x2")\nfor pad in (1, 2, 4, 5):\n    p[pad].AddData("hist2d", "inputs")\np[1].AddText("text")\np[2].AddLine((2., 2.), (5., 5.))\np[4].AddRegionX(3., 4.)\npm.AddPlot(p)')
program = f"""import json
import os
import shutil

import ROOT
from SciRooPlot import *

ROOT.gROOT.SetBatch(True)
ROOT.TH1.AddDirectory(False)  # the histograms read stay when the file is closed
assert ROOT.TObject.GetObjectStat(), "object statistics not on"
source = ROOT.TFile.Open({inputs!r})
memory = ROOT.TList()
memory.SetName("memory")
for key in source.GetListOfKeys():
    obj = key.ReadObj()
    ROOT.SetOwnership(obj, False)
    memory.Add(obj)
source.Close()
pm = PlotManager("repeated")
for base in ("1d", "1d_ratio", "2d", "2d_2x2"):
    pm.AddBasePlot(PlotManager.MakeBasePlot(base))
pm.AddDataSource("inputs", {inputs!r})
pm.AddDataSource("memory", memory)
pm.AddDataSource("containers", {p.path("containers.root")!r})
pm.AddDataSource("searched", {p.path("containers.root")!r})
{chr(10).join(lines)}
pm.SetOutputDirectory({p.path("out")!r})
def directories():  # the directories written to the output files of the run
    def count(directory):
        subs = [directory.GetDirectory(key.GetName()) for key in directory.GetListOfKeys() if key.GetClassName() == "TDirectoryFile"]
        return len(subs) + sum(count(sub) for sub in subs)
    n = 0
    for name in ("Plots.root", "Data.root"):
        path = {p.path("out")!r} + "/" + name
        f = ROOT.TFile.Open(path) if os.path.exists(path) else None
        if f:
            n += count(f) + 1
            f.Close()
    return n


counts = {{}}
for mode in ("file", "data", "png"):
    counts[mode] = []
    for run in range({RUNS}):
        shutil.rmtree({p.path("out")!r}, ignore_errors=True)
        pm.GeneratePlots(mode)
        pm.ClearDataBuffer()
        pm.ClearCanvasRegistry()
        ROOT.gObjectTable.UpdateInstCount()
        found = {{c.GetName(): c.GetInstanceCount() for c in ROOT.gROOT.GetListOfClasses() if c.GetInstanceCount()}}
        found["directories written"] = directories()
        counts[mode].append(found)
print("COUNTS " + json.dumps(counts), flush=True)
"""
out = S.python(program, cwd=p.dir, timeout=3600)
found = [line[7:] for line in out.text.splitlines() if line.startswith("COUNTS ")]
check("the program runs", out.rc == 0 and found, out.tail(1500))
if found:
    counts = json.loads(found[0])
    for mode, runs in counts.items():
        runs[1].pop("directories written")
        for n in range(2, RUNS):
            written = runs[n].pop("directories written")
            grown = {cls: runs[n].get(cls, 0) - runs[n - 1].get(cls, 0) for cls in set(runs[n]) | set(runs[n - 1]) if runs[n].get(cls, 0) != runs[n - 1].get(cls, 0)}
            # ROOT keeps the identifier of every directory it writes (a TObjString in a TList), whatever is deleted
            kept = {cls: k for cls, k in grown.items() if not (cls in ("TObjString", "TList") and 0 < k <= written)}
            check(f"mode {mode}: run {n + 1} leaves as many objects of every class as run {n}", not kept, f"{str(kept)[:600]} ({written} directories written)")
made = sum(line.count("pm.AddPlot") for line in lines) * RUNS * 3
check("every plot made in every run", out.text.count("Created plot") == made, f"{out.text.count('Created plot')} of {made}")
errors = [line for line in out.text.splitlines() if "[ ERR" in line or "Error in <" in line]
check("no errors", not errors, " | ".join(errors[:5]))
finish()
