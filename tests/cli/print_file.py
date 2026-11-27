"""srp print <file>[:<path>] lists every object of a ROOT file with its class and the path SciRooPlot reads it by: objects of every data
kind at the top, in directories, lists, stacks (their histograms) and multigraphs (their graphs), trees with their branches; below a path
only what is there; also for files below a folder with ':' in its name, and for a directory hidden by a newer key of data of its name
(as the reader finds it); a name written in several cycles once, the value of a map under its key. One object named by its path is
given with its details (axes and binning, entries, integral, under- and overflow, contents and the smallest above 0, means of profiles,
points and ranges of graphs, formula and range of functions, entries of trees), as ROOT gives them for the object. A missing file, a file that is no
ROOT file and a path not in the file fail with exit status 1 and one error. srp open without a file, with a missing file or one that is
no ROOT file, and srp open and srp colorwheel without a graphical display (X11 only) end with exit status 1 and one error. srp
colorwheel <file> writes an image file without ROOT's messages, and refuses a name that is no image file. File names may hold ~ and
environment variables, as the inputs of data sources. srp print <table file> gives the data name of a table, its rows and delimiter
and each column with the type the reader gives it; a table that cannot be read fails with exit status 1 and one error.
srp print <data source>[:<path>] (a data source of the current project, unless a file of that name exists) lists its objects as the reader
sees them, with where they are in the inputs; a path names a folder or one object (details as for a file); the inputs only at debug.
Covers: details of every data kind alone, stack and multigraph members, file below a folder with ':', several cycles of a name, map values, srp open and colorwheel
without a file or display, colorwheel into a file."""

import os
import subprocess
import sys

import numpy as np
from srptest import ROOT, SRP, check, config_folder, data_objects, finish, own, python, run

folder = config_folder("files")
special = config_folder("run_2026-11-04T10:00")
objects = data_objects()
expected = {}
path = os.path.join(folder, "all.root")
f = ROOT.TFile(path, "RECREATE")
for name, obj in objects.items():
    obj.Write(name)
    expected[name] = obj.ClassName()
sub = f.mkdir("dir").mkdir("sub")
sub.cd()
objects["hist"].Write("inner")
expected["dir/sub/inner"] = "TH1D"
f.cd()
lst = ROOT.TList()
lst.SetName("list")
for name in ("graph", "hist2d"):
    lst.Add(objects[name])
    expected[f"list/{objects[name].GetName()}"] = objects[name].ClassName()  # in a list an object is found by its own name
lst.Write("list", ROOT.TObject.kSingleKey)
stack = ROOT.THStack("stack", "")
for name in ("hist", "hist_var"):
    stack.Add(objects[name])
    expected[f"stack/{name}"] = "TH1D"
stack.Write()
mg = ROOT.TMultiGraph("mg", "")
for name in ("graph", "graph_errors"):
    mg.Add(objects[name].Clone(name))
    expected[f"mg/{name}"] = objects[name].ClassName()
mg.Write()
tree = ROOT.TTree("tree", "")
x = np.zeros(1)
n = np.zeros(1, dtype=np.int32)
tree.Branch("x", x, "x/D")
tree.Branch("n", n, "n/I")
for i in range(3):
    x[0], n[0] = i, i
    tree.Fill()
tree.Write()
expected["tree"] = "TTree"
f.Close()


def listing(text):
    """(path, class) of every object srp print lists, and the branches of trees as path -> [branch (type)]"""
    found, branches, prefix, last = {}, {}, "", None
    for line in text.splitlines():
        if line.endswith("/") and not line.startswith(("-", " ")):
            prefix = line
        elif line.startswith("- ["):
            cls, name = line[3:].split("] ", 1)
            last = prefix + name
            found[last] = cls
        elif line.startswith("  -> ") and last:
            branches.setdefault(last, []).append(line[5:])
    return found, branches


out = run(SRP, "print", path)
found, branches = listing(out.stdout)
check("srp print lists every object with its class and path", out.rc == 0 and found == expected, f"extra {sorted(set(found.items()) - set(expected.items()))}, missing {sorted(set(expected.items()) - set(found.items()))}")
check("srp print lists the branches of trees", branches.get("tree") == ["x (Double_t)", "n (Int_t)"], f"{branches}")
for start in ("dir", "dir/sub", "stack", "mg", "list"):
    out = run(SRP, "print", f"{path}:{start}")
    found, _ = listing(out.stdout)
    want = {k: v for k, v in expected.items() if k.startswith(start + "/")}
    check(f"srp print below {start} lists only what is there", out.rc == 0 and found == want, f"{found}")


# one object named by its path: its details (binning, ranges, statistics) from the object as ROOT gives them
def g(value):
    return f"{value:.4g}"


def axis_text(axis):
    return f"{axis.GetNbins()} {'variable bins' if axis.IsVariableBinSize() else 'bins'} from {g(axis.GetXmin())} to {g(axis.GetXmax())}"


def details(obj):
    """the detail lines expected for an object (a part of each line where the line holds more)"""
    if isinstance(obj, ROOT.TH2Poly):
        contents = [obj.GetBinContent(b) for b in range(1, obj.GetNumberOfBins() + 1)]
        return [f"x: {g(obj.GetXaxis().GetXmin())} to", f"integral {g(sum(contents))}", f"of {len(contents)} bins not 0"]
    if isinstance(obj, ROOT.TH1):
        axes = [obj.GetXaxis(), obj.GetYaxis(), obj.GetZaxis()][: obj.GetDimension()]
        inside = [b for b in range(obj.GetNcells()) if not obj.IsBinUnderflow(b) and not obj.IsBinOverflow(b)]
        contents = [obj.GetBinContent(b) for b in inside]
        flows = sum(obj.GetBinContent(b) for b in range(obj.GetNcells()) if b not in set(inside))
        profile = obj.ClassName().startswith("TProfile")
        stats = f"entries {g(obj.GetEntries())}" + ("" if profile else f", integral {g(sum(contents))}, under- and overflow {g(flows)}")
        kind = "means" if profile else "contents"
        filled = f"{sum(c != 0 for c in contents)} of {len(contents)} bins not 0"
        return [f"{name}: {axis_text(axis)}" for name, axis in zip("xyz", axes)] + [stats, f"{kind} {g(min(contents))} to {g(max(contents))}", filled]
    if isinstance(obj, ROOT.TGraph):
        xs, ys = [obj.GetPointX(i) for i in range(obj.GetN())], [obj.GetPointY(i) for i in range(obj.GetN())]
        return [f"{obj.GetN()} points", f"x: {g(min(xs))} to {g(max(xs))}", f"y: {g(min(ys))} to {g(max(ys))}"]
    if isinstance(obj, ROOT.TGraph2D):
        return [f"{obj.GetN()} points", f"x: {g(obj.GetXmin())} to {g(obj.GetXmax())}", f"z: {g(obj.GetZmin())} to {g(obj.GetZmax())}"]
    if isinstance(obj, ROOT.THnBase):
        return [f"axis {d}: {axis_text(obj.GetAxis(d))}" for d in range(obj.GetNdimensions())] + [f"entries {g(obj.GetEntries())}, {sum(obj.GetBinContent(b, ROOT.nullptr) != 0 for b in range(obj.GetNbins()))} bins filled"]
    if isinstance(obj, ROOT.TEfficiency):
        total = obj.GetTotalHistogram()
        return [f"x: {axis_text(total.GetXaxis())}", f"passed {g(obj.GetPassedHistogram().GetEntries())} of {g(total.GetEntries())} entries"]
    if isinstance(obj, ROOT.TF1):
        return [f"{obj.GetNpar()} parameters", f"x: {g(obj.GetXmin())} to {g(obj.GetXmax())}"]
    return []


for name in [*objects, "tree"]:
    out = run(SRP, "print", f"{path}:{name}")
    lines = out.stdout.splitlines()
    want = ["entries 3", "-> x (Double_t)"] if name == "tree" else details(objects[name])
    good = out.rc == 0 and lines and lines[0] == f"- [{expected[name]}] {name}" and want and all(any(w in line for line in lines[1:]) for w in want)
    check(f"srp print of the {name} alone gives its details", good, f"want {want} | {out.tail()}")
hist = objects["hist"]
out = run(SRP, "print", f"{path}:dir/sub/inner")
check("srp print of an object in a directory names it by its path, with its details", out.stdout.startswith("- [TH1D] dir/sub/inner\n") and f"x: {axis_text(hist.GetXaxis())}" in out.stdout, out.tail())
log = ROOT.TH1D("log", "counts", 4, 0.0, 4.0)
for b, c in ((1, 0.0), (2, 0.25), (3, 4.0), (4, 0.0)):
    log.SetBinContent(b, c)
f = ROOT.TFile(os.path.join(folder, "log.root"), "RECREATE")
log.Write()
f.Close()
out = run(SRP, "print", os.path.join(folder, "log.root") + ":log")
good = 'title "counts"' in out.stdout and "contents 0 to 4 (smallest above 0: 0.25), 2 of 4 bins not 0" in out.stdout
check("srp print of a histogram with empty bins gives its title and the smallest content above 0", good, out.tail())

# a file below a folder with ':' in its name
inner = os.path.join(special, "f.root")
f = ROOT.TFile(inner, "RECREATE")
objects["hist"].Write("h")
f.Close()
for suffix in ("", ":h"):
    out = run(SRP, "print", inner + suffix)
    check(f"srp print of a file below a folder with ':' ({suffix or 'whole file'})", out.rc == 0 and listing(out.stdout)[0] == {"h": "TH1D"}, out.tail())

# a directory hidden by a newer key of its name holding data: found as the reader finds it (the directory, not the data)
hidden = os.path.join(folder, "hidden.root")
f = ROOT.TFile(hidden, "RECREATE")
f.mkdir("d").cd()
objects["hist"].Write("h")
f.cd()
objects["hist"].Write("d")
f.Close()
for start in (":d", ":d/h"):
    out = run(SRP, "print", hidden + start)
    check(f"srp print {start[1:]} of a directory hidden by a newer key of data: the directory", out.rc == 0 and listing(out.stdout)[0] == {"d/h": "TH1D"}, out.tail())

text = os.path.join(folder, "notes.log")
with open(text, "w") as fh:
    fh.write("no ROOT file\n")
for target, reason in ((os.path.join(folder, "nosuch.root"), "Could not find file"), (text, "Cannot read"), (path + ":nosuch", "Path not found"), (path + ":dir/nosuch", "Path not found")):
    out = run(SRP, "print", target)
    check(f"srp print {os.path.basename(target)}: exit 1, one error ({reason})", out.rc == 1 and len(out.errors()) == 1 and reason in out.errors()[0], out.tail())
# a name written in several cycles is listed once (the latest cycle, as the reader takes it), a value of a map under its key
cycles = os.path.join(folder, "cycles.root")
f = ROOT.TFile(cycles, "RECREATE")
for _ in range(3):
    objects["hist"].Write("h")
tmap = ROOT.TMap()
tmap.Add(own(ROOT.TObjString("key")), objects["graph"])  # the key kept alive (PyROOT would delete it)
tmap.Write("map", ROOT.TObject.kSingleKey)
f.Close()
out = run(SRP, "print", cycles)
rows = [line for line in out.stdout.splitlines() if line.startswith("- [")]
check("srp print lists a name written in several cycles once", out.rc == 0 and sum(row.endswith("] h") for row in rows) == 1, out.tail())
check("srp print lists the value of a map under its key", listing(out.stdout)[0].get("map/key") == "TGraph", out.tail())

# srp open: a missing argument, a missing file or a file that is no ROOT file, and no graphical display end srp with one error
for args, reason in (((), "Specify a file"), ((os.path.join(folder, "nosuch.root"),), "Could not find file"), ((text,), "Cannot read")):
    out = run(SRP, "open", *args, timeout=60)
    check(f"srp open {' '.join(os.path.basename(a) for a in args) or 'without a file'}: exit 1, one error ({reason})", out.rc == 1 and len(out.errors()) == 1 and reason in out.errors()[0], out.tail())
if sys.platform.startswith("linux"):  # X11: without DISPLAY there is no display (macOS draws through Cocoa)
    for args in (("open", path), ("colorwheel",)):
        try:
            out = run(SRP, *args, env={"DISPLAY": None, "WAYLAND_DISPLAY": None}, timeout=60)
            ok, detail = out.rc == 1 and len(out.errors()) == 1 and "needs a graphical display" in out.errors()[0], out.tail()
        except subprocess.TimeoutExpired:
            ok, detail = False, "still running after a minute"
        check(f"srp {args[0]} without a graphical display: exit 1, one error saying so", ok, detail)

# a table file: its data name, rows and delimiter, each column with the type it is read as (integers stay integers, a column of integers
# with a decimal further down is floating point, text is text); a table that cannot be read fails with one error
table = os.path.join(folder, "spectrum.csv")
with open(table, "w") as fh:
    fh.write("pt;n;label\n1;3;low\n2;5;mid\n3.5;7;high\n")
out = run(SRP, "print", table)
want = ["- [table] spectrum: 3 rows, separated by ';'", "  -> pt (double)", "  -> n (Long64_t)", "  -> label (std::string)"]
check("srp print of a table: data name, rows, delimiter and the type each column is read as", out.rc == 0 and out.stdout.splitlines() == want, out.tail())
single = os.path.join(folder, "single.txt")
with open(single, "w") as fh:
    fh.write("x y\n1 2\n")
out = run(SRP, "print", single)
check("srp print of a table of one row: \"1 row\"", out.rc == 0 and out.stdout.splitlines()[:1] == ["- [table] single: 1 row, separated by spaces"], out.tail())
# a row with fields missing, and a value that does not match the type of its column further down
for name, text in (("broken", "a,b\n1,2\n3\n"), ("mismatch", "a,b\n1,2\n2,3\n3,x\n")):
    broken = os.path.join(folder, f"{name}.csv")
    with open(broken, "w") as fh:
        fh.write(text)
    out = run(SRP, "print", broken)
    check(f"srp print of a table that cannot be read ({name}): exit 1, one error naming the table", out.rc == 1 and len(out.errors()) == 1 and f"Cannot read table {broken}" in out.errors()[0], out.tail())

# a ~ and environment variables in the file name, as in the inputs of data sources
out = run(SRP, "print", "${SRP_TESTS_PRINT_DIR}/" + os.path.basename(path), env={"SRP_TESTS_PRINT_DIR": os.path.dirname(path)})
check("srp print expands environment variables in the file name", out.rc == 0 and listing(out.stdout)[0], out.tail())
out = run(SRP, "colorwheel", "${SRP_TESTS_PRINT_DIR}/wheel_var.png", env={"SRP_TESTS_PRINT_DIR": folder}, timeout=120)
check("srp colorwheel expands environment variables in the file name: written, no error", out.rc == 0 and os.path.isfile(os.path.join(folder, "wheel_var.png")) and not out.errors(), out.tail())

# srp colorwheel <file> writes the wheel as an image without ROOT's messages; a name that is no image file (ROOT would write PostScript, also for .PNG)
# ends with exit status 1 and one error, nothing written
# ROOT compares endings case-sensitively, writes PostScript for .tif and cannot write .tiff
for name, ok in (("wheel.png", True), ("wheel.pdf", True), ("wheel", False), ("wheel.txt", False), ("wheel.PNG", False), ("wheel.tif", False), ("wheel.tiff", False)):
    target = os.path.join(folder, name)
    out = run(SRP, "colorwheel", target, timeout=120)
    if ok:
        good = out.rc == 0 and os.path.isfile(target) and os.path.getsize(target) > 0 and "Info in <" not in out.text and not out.errors()
        check(f"srp colorwheel {name}: written, without ROOT's messages", good, out.tail())
    else:
        good = out.rc == 1 and len(out.errors()) == 1 and "give a file name ending in" in out.errors()[0] and not os.path.exists(target)
        check(f"srp colorwheel {name}: exit 1, one error, nothing written", good, out.tail())

# a remote file, read through ROOT as the inputs of data sources (file:// goes through the same path as root:// and https://)
for start in ("", ":dir"):
    out = run(SRP, "print", f"file://{path}{start}")
    found, _ = listing(out.stdout)
    want = {k: v for k, v in expected.items() if not start or k.startswith("dir/")}
    check(f"srp print of a remote file{start or ''}", out.rc == 0 and found == want, out.tail())

# a data source of the current project (not an existing file): its inputs as the reader expands them, each object once with where it is
# (a tree chained over the inputs, other data read from the first input holding it), tables by their data name; a path names a folder
# or one object (its details, a chained tree with its entries in all); the inputs themselves only at verbosity debug
sourceFolder = config_folder("source_files")
for i in range(3):
    rf = ROOT.TFile(os.path.join(sourceFolder, f"run{i}.root"), "RECREATE")
    t = ROOT.TTree("events", "")
    v = np.zeros(1)
    t.Branch("v", v, "v/D")
    for k in range(10 * (i + 1)):
        v[0] = k
        t.Fill()
    t.Write()
    if i < 2:
        ROOT.TH1D("hA", "A", 10, 0.0, 1.0).Write()
    if i == 0:
        rf.mkdir("dir").cd()
        ROOT.TH1D("hB", "", 5, 0.0, 1.0).Write()
    rf.Close()
with open(os.path.join(sourceFolder, "spec.csv"), "w") as fh:
    fh.write("a,b\n1,2\n")
sourceConfig = config_folder("source_config")
program = os.path.join(sourceFolder, "define.py")
with open(program, "w") as fh:
    fh.write(f"from SciRooPlot import PlotManager\npm = PlotManager('sources')\npm.AddDataSource('src', ['{sourceFolder}/run*.root', '{sourceFolder}/spec.csv'])\npm.SaveProject()\n")
setUp = [run(SRP, "add", "sources", program, os.path.join(sourceFolder, "out"), config=sourceConfig), python(open(program).read(), config=sourceConfig), run(SRP, "select", "sources", config=sourceConfig)]
check("srp print of a data source: project set up", all(o.rc == 0 for o in setUp), " | ".join(o.tail() for o in setUp))
out = run(SRP, "print", "src", config=sourceConfig, cwd=sourceFolder)
want = ["data source src: 4 inputs", "- [TTree] events (chained over 3 inputs)", "- [TH1D] hA (in 2 of 3 inputs: read from the first)", "dir/", "- [TH1D] hB (in 1 of 3 inputs)", "- [table] spec"]
check("srp print of a data source: each object once, where it is in the inputs, tables by name", out.rc == 0 and [line.strip("| ").strip() for line in out.stdout.splitlines()] == want, out.tail())
out = run(SRP, "print", "src:events", config=sourceConfig)
check("srp print of a tree in a data source: its details and its entries chained over the inputs", out.rc == 0 and "- [TTree] events" in out.stdout and "chained over 3 inputs: 60 entries in all" in out.stdout, out.tail())
out = run(SRP, "print", "src:dir", config=sourceConfig)
check("srp print of a folder of a data source", out.rc == 0 and "- [TH1D] hB (in 1 of 3 inputs)" in out.stdout and "hA" not in out.stdout, out.tail())
out = run(SRP, "print", "src:nothing", config=sourceConfig)
check("srp print of a path a data source does not have: exit 1, one error", out.rc == 1 and len(out.errors()) == 1 and "Path not found in data source src" in out.errors()[0], out.tail())
out = run(SRP, "print", "src", config=sourceConfig, env={"SCIROOPLOT_VERBOSITY": "debug"})
check("srp print of a data source: the inputs only at verbosity debug", "run1.root" in out.stdout + out.stderr and "run1.root" not in run(SRP, "print", "src", config=sourceConfig).stdout, out.tail())
with open(os.path.join(sourceFolder, "src"), "w") as fh:
    fh.write("not a ROOT file\n")
out = run(SRP, "print", "src", config=sourceConfig, cwd=sourceFolder)
check("srp print: an existing file of the name of a data source is printed as a file", out.rc == 1 and "Cannot read" in " ".join(out.errors()), out.tail())
finish()
