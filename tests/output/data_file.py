"""Mode data writes the input objects of the selected plots, as read, into Data.root of the output directory: each under its data source
and its path in it (a data source restricted to a folder, source:folder, and a path in a name become subdirectories), equal to the object
in the input file, for every data kind of srptest read from a file in every variant (every value at least once in the fast run); a
function of the plot definition under its data source, named by its formula, points of the plot definition likewise, named by their
coordinates. Keys are written in a fixed order: the same plots give the
same file. The plots themselves are not painted. Only the data of the selected plots is written, also when the process read other data
before; a plot whose data is missing is reported with its reasons and the run fails, as in the other modes; when no plot has its data,
nothing is written (an earlier Data.root stays).
Covers: a data source restricted to a folder (also given with a trailing slash, or empty), a path in a name, functions and points of the plot
definition, the fixed key order, missing data, data read by earlier calls in the same process, a canvas of Plots.root drawn in the
process that wrote it (a plot whose canvas ROOT deleted that way and that is not plotted again is left out, with a warning), names of Plots.root and Data.root outside of the output directory or the same for both (refused with a warning
where set), a picture that cannot be written over an earlier one (reported, not logged as created), a picture written again within the
same second on a file system with timestamps in seconds (written, not reported; only where an ext2 image can be mounted, as root)."""

import os
import re
import subprocess

from srptest import DATA_KINDS, ROOT, VARIANTS, Project, S, add_data, base_of, captured, cases, check, data_objects, describe, finish, func_diff, graph_diff, has_variant, hist_diff, points_name, source_of
from srptest import formula as formula_of
from srptest import python

p = Project("data_file")
p.inputs(VARIANTS)
INPUT = tuple(kind for kind, (_, _, props) in DATA_KINDS.items() if not props & {"formula", "given"})
todo = cases({"kind": INPUT, "variant": VARIANTS}, valid=lambda c: has_variant(c["kind"], c["variant"]))
for i, case in enumerate(todo):
    plot = S.Plot(f"d{i}", "data", base_of(case["kind"]))
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    p.pm.AddPlot(plot)
plot = S.Plot("formula", "data", "1d")
add_data(plot[1], "function")
add_data(plot[1], "points")
p.pm.AddPlot(plot)
# a path in a name, and a data source restricted to a folder
f = ROOT.TFile(p.path("folders.root"), "RECREATE")
for folder in ("a", "a/b"):
    f.mkdir(folder, "", True).cd()
    h = ROOT.TH1D("h", "", 4, 0.0, 4.0)
    h.Fill(1.5, 2.0 if folder == "a" else 3.0)
    h.Write()
f.Close()
p.pm.AddDataSource("folders", p.path("folders.root"))
p.pm.AddDataSource("inner", p.path("folders.root") + ":a")
plot = S.Plot("paths", "data", "1d")
plot[1].AddData("a/b/h", "folders")
plot[1].AddData("b/h", "inner")
p.pm.AddPlot(plot)
plot = S.Plot("folder_forms", "folders", "1d")
plot[1].AddData("h", "folders:a/")
plot[1].AddData("a/h", "folders:")
p.pm.AddPlot(plot)
plot = S.Plot("missing", "missing", "1d")
plot[1].AddData("nothere", "folders")
plot[1].AddData("h", "undefined")
p.pm.AddPlot(plot)


def diff(ref, obs):
    """differences between an input object and the one written (functions compared at their sample points)"""
    if not obs:
        return "not written"
    if obs.ClassName() != ref.ClassName():
        return f"{obs.ClassName()}, expected {ref.ClassName()}"
    if ref.InheritsFrom("TEfficiency"):
        return hist_diff(ref.GetPassedHistogram(), obs.GetPassedHistogram()) or hist_diff(ref.GetTotalHistogram(), obs.GetTotalHistogram())
    if ref.InheritsFrom("THnBase"):
        dims = list(range(ref.GetNdimensions()))
        return hist_diff(ref.Projection(*dims) if len(dims) <= 3 else None, obs.Projection(*dims))
    if ref.InheritsFrom("TH1"):
        return hist_diff(ref, obs)
    if ref.InheritsFrom("TF1"):
        return func_diff(ref, obs)
    return graph_diff(ref, obs)


def keys(path):
    """every key of a file with its directory, in the order written"""
    found = []

    def walk(directory, prefix):
        for key in directory.GetListOfKeys():
            found.append(prefix + key.GetName())
            if key.IsFolder() and key.GetClassName().startswith("TDirectory"):
                walk(directory.Get(key.GetName()), prefix + key.GetName() + "/")

    f = ROOT.TFile.Open(path)
    walk(f, "")
    f.Close()
    return found


run = p.plot("data", ".+", "data")
check("mode data: no errors", not run.errors(), " | ".join(run.errors()[:3]))
check("mode data paints nothing: no plot reported as created", "Created plot" not in run.log, " | ".join(line for line in run.log.splitlines() if "Created plot" in line)[:300])
written = ROOT.TFile.Open(p.path("out", "Data.root"))
check("mode data: Data.root in the output directory", bool(written))
if written:
    inputs = {variant: data_objects(variant) for variant in VARIANTS}
    for case in todo:
        obj = written.Get(f"{source_of(case['variant'])}/{case['kind']}")
        problem = diff(inputs[case["variant"]][case["kind"]], obj)
        check(f"written as read: {describe(case)}", not problem, problem)
    contents = {path: written.Get(path) for path in ("folders/a/b/h", "inner/b/h")}
    check("a path in a name: subdirectories of the data source", bool(contents["folders/a/b/h"]) and contents["folders/a/b/h"].GetBinContent(2) == 3.0)
    check("a data source restricted to a folder: its folder and the path below it", bool(contents["inner/b/h"]) and contents["inner/b/h"].GetBinContent(2) == 3.0)
    names = keys(p.path("out", "Data.root"))
    formula = formula_of("function")
    # a key holding '/' is read through the key (TFile::Get takes it for a path)
    func = next((key.ReadObj() for source in names if "/" not in source for key in written.Get(source).GetListOfKeys() if key.GetName() == formula), None)
    problem = func_diff(ROOT.TF1("reference", formula, 1.0, 10.0), func) if func else "not written"
    check("a function of the plot definition: under its data source, named by its formula", not problem, problem)
    pointsName = points_name()
    points = next((key.ReadObj() for source in names if "/" not in source for key in written.Get(source).GetListOfKeys() if key.GetName() == pointsName), None)
    problem = graph_diff(data_objects()["points"], points) if points else "not written"
    check("points of the plot definition: under their data source, named by their coordinates", not problem, problem)
    written.Close()
    first = names
    p.plot("data", ".+", "data")
    check("the same plots give the keys in the same order", keys(p.path("out", "Data.root")) == first)

run = p.plot("folders", ".+", "data")
check("a folder given with a trailing slash or empty: no errors", not run.errors(), " | ".join(run.errors()[:3]))
written = ROOT.TFile.Open(p.path("out", "Data.root"))
names = keys(p.path("out", "Data.root")) if written else []
check("a folder given with a trailing slash or empty: written below the data source", bool(written) and "folders/a/h" in names, names)
if written:
    written.Close()

run = p.plot("missing", ".+", "data")
expected = ("missing folders:nothere (not found in the inputs of folders)", "missing undefined:h (data source not defined)")
check("missing data in mode data: the plot is reported with the reasons", all(e in run.log for e in expected), " | ".join(run.errors()[:4]))
check("missing data in mode data: the run fails", run.rc != 0, run.rc)

# data read by an earlier call of the same process is not written with the selected plots
p.pm.SetOutputDirectory(p.path("in_process"))
captured(lambda: p.pm.GeneratePlots("png", "paths", "data"))
created, _ = captured(lambda: p.pm.GeneratePlots("data", "formula", "data"))
names = keys(p.path("in_process", "Data.root"))
check("only the data of the selected plots, not that of earlier calls", created is True and not any(name.startswith(("folders", "inner")) for name in names), names)
# a run whose plots all lack their data writes nothing: the Data.root of an earlier run stays
created, printed = captured(lambda: p.pm.GeneratePlots("data", "missing", "missing"))
kept = keys(p.path("in_process", "Data.root"))
check("missing data of every plot in mode data: the earlier Data.root stays, nothing said saved", created is False and bool(names) and kept == names and "Saved data" not in printed, f"{kept}, before {names} | {printed.strip()[-300:]}")
# a canvas read back from Plots.root and drawn in the process that wrote it (as a notebook checking its output does): ROOT deletes the
# manager's canvas of that name, which the manager does not delete again when it ends
out = python(f"""
import ROOT
import SciRooPlot as S
ROOT.gROOT.SetBatch(True)
pm = S.PlotManager("")
pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d"))
plot = S.Plot("drawn", "g", "1d")
plot[1].AddData(S.Function("x"), S.kUserFunctions)
pm.AddPlot(plot)
pm.SetOutputDirectory({p.path("redrawn")!r})
pm.GeneratePlots("file")
f = ROOT.TFile.Open({p.path("redrawn", "Plots.root")!r})
canvas = f.Get("g/" + next(k.GetName() for k in f.Get("g").GetListOfKeys()))
canvas.Draw()
pm.GeneratePlots("file")
print("DONE")
""")
check("a canvas of Plots.root drawn in the process that wrote it: the process ends cleanly, Plots.root written again", out.rc == 0 and "DONE" in out.stdout and out.text.count("Saved 1 plot") == 2, f"exit status {out.rc}: {out.text[-400:]}")

# a plot of an earlier call whose canvas ROOT deleted (one of its name read back and drawn), not plotted again: left out of Plots.root, said
out = python(f"""
import ROOT
import SciRooPlot as S
ROOT.gROOT.SetBatch(True)
pm = S.PlotManager("")
pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d"))
for name in ("a", "b"):
    plot = S.Plot(name, "g", "1d")
    plot[1].AddData(S.Function("x"), S.kUserFunctions)
    pm.AddPlot(plot)
pm.SetOutputDirectory({p.path("redrawn_other")!r})
pm.GeneratePlots("file")
f = ROOT.TFile.Open({p.path("redrawn_other", "Plots.root")!r})
f.Get("g/a").Draw()
pm.GeneratePlots("file", "b")
pm.GeneratePlots("file", "b")
""")
left = [line for line in out.text.splitlines() if "is left out of Plots.root: ROOT deleted its canvas" in line]
check("a plot whose canvas ROOT deleted is left out of Plots.root, said once", len(left) == 1 and "Plot a from group g" in re.sub(r"\x1b\[[0-9;]*m", "", left[0]) and out.text.count("Saved 1 plot") == 2, out.text[-600:])

# names of the two files outside of the output directory or the same for both: refused where set; a picture whose earlier file cannot be
# written over (immutable for root, read-only otherwise) is reported, not logged as created
out = python(f"""
import os
import subprocess
import ROOT
import SciRooPlot as S
ROOT.gROOT.SetBatch(True)
pm = S.PlotManager("")
pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d"))
for name in ("", "sub/P.root"):
    pm.SetPlotsRootFile(name)
pm.SetPlotsRootFile("Data.root")
pm.SetDataRootFile("Plots.root")
plot = S.Plot("kept", "g", "1d")
plot[1].AddData(S.Function("x"), S.kUserFunctions)
pm.AddPlot(plot)
pm.SetOutputDirectory({p.path("names")!r})
pm.GeneratePlots("file")
print("PLOTS", os.path.isfile({p.path("names", "Plots.root")!r}), flush=True)
pm.GeneratePlots("png")
png = {p.path("names", "g", "kept.png")!r}
locked = os.geteuid() == 0 and subprocess.run(["chattr", "+i", png], capture_output=True).returncode == 0
if not locked:
    os.chmod(png, 0o444)
print("LOCKED", flush=True)
pm.GeneratePlots("png")
if locked:
    subprocess.run(["chattr", "-i", png])
""")
refused = [line for line in out.text.splitlines() if "is ignored: it must be a name within the output directory" in line]
check("names of Plots.root and Data.root outside of the output directory or the same for both: refused where set, Plots.root written", len(refused) == 4 and "PLOTS True" in out.stdout, out.text[-600:])
after = out.text.split("LOCKED")[-1]
check("a picture that cannot be written over an earlier one: reported, not logged as created", "Could not write" in after and "Created plot" not in after, after[-400:])

# a picture written again within the same second on a file system with timestamps in seconds (ext2 with 128-byte inodes, like AFS or HFS+):
# its time does not change, but it was written
image, mounted = p.path("coarse.img"), p.path("coarse")
os.makedirs(mounted, exist_ok=True)
coarse = os.geteuid() == 0 and subprocess.run(["truncate", "-s", "8M", image]).returncode == 0 and subprocess.run(["mkfs.ext2", "-q", "-F", "-I", "128", image], capture_output=True).returncode == 0 and subprocess.run(["mount", "-o", "loop", image, mounted], capture_output=True).returncode == 0
if coarse:
    out = python(f"""
import ROOT
import SciRooPlot as S
ROOT.gROOT.SetBatch(True)
pm = S.PlotManager("")
pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d"))
plot = S.Plot("again", "g", "1d")
plot[1].AddData(S.Function("x"), S.kUserFunctions)
pm.AddPlot(plot)
pm.SetOutputDirectory({os.path.join(mounted, "out")!r})
print("RESULTS", [pm.GeneratePlots("png") for _ in range(4)], flush=True)
""")
    subprocess.run(["umount", mounted])
    check("a picture written again within one second of a file system with timestamps in seconds: not reported", "RESULTS [True, True, True, True]" in out.stdout and "Could not write" not in out.text, out.text[-400:])
finish()
