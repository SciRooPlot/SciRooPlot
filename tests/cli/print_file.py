"""srp print <file>[:<path>] lists every object of a ROOT file with its class and the path SciRooPlot reads it by: objects of every data
kind at the top, in directories, lists, stacks (their histograms) and multigraphs (their graphs), trees with their branches; below a path
only what is there; also for files below a folder with ':' in its name, and for a directory hidden by a newer key of data of its name
(as the reader finds it); a name written in several cycles once, the value of a map under its key. A missing file, a file that is no
ROOT file and a path not in the file fail with exit status 1 and one error. srp open without a file, with a missing file or one that is
no ROOT file, and srp open and srp colorwheel without a graphical display (X11 only) end with exit status 1 and one error.
Covers: stack and multigraph members, file below a folder with ':', several cycles of a name, map values, srp open and colorwheel
without a file or display."""

import os
import subprocess
import sys

import numpy as np
from srptest import ROOT, SRP, check, config_folder, data_objects, finish, own, run

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

text = os.path.join(folder, "notes.txt")
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

# a remote file, read through ROOT as the inputs of data sources (file:// goes through the same path as root:// and https://)
for start in ("", ":dir"):
    out = run(SRP, "print", f"file://{path}{start}")
    found, _ = listing(out.stdout)
    want = {k: v for k, v in expected.items() if not start or k.startswith("dir/")}
    check(f"srp print of a remote file{start or ''}", out.rc == 0 and found == want, out.tail())
finish()
