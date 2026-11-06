"""Every form of input of a data source gives the documented data (skill, Data sources): a file, a folder in a file (file.root:folder,
also with wildcards: every matching folder), a directory (every ROOT and table file below it, alphabetically), wildcards in file names
(alphabetically: run10.root before run2.root), environment variables, paths with spaces, unicode and parentheses, symlinked files and
directories. Within a data source the first input holding a name wins, and trees found in several inputs are chained; the same file
reached by several paths (a symlink, a hard link, ./ in the path) is read once, with a warning naming the skipped spelling. An input that
cannot be read (a broken symlink, a corrupt file, an unset environment variable, a relative path, no file or folder matching a wildcard)
is reported and the other inputs are still read; a symlinked directory inside a directory input is skipped with a warning. Of several
cycles of a key the latest counts; a directory hidden by a newer key of its name is still reached by a path, the search and wildcards. Tables of a directory are read only when they are requested, and a restricted data source
(src:folder) leaves tables out. Data of an unknown data source is reported missing with the reason, and a data source with an
illegal name is refused. Inputs added after GeneratePlots (appended or replaced) are read by the next GeneratePlots; without a change
the data is not read again.
Covers: 505b135 (the same file through a symlink in a directory source, or ./, chained once), 5434a4e (tables read only when requested;
the same table name in two inputs: the first wins), 52d49d5 (a second AddDataSource replacing an input warns), 7d65935 (a file added
before a second GeneratePlots: the tree read again over both), f934095 (inputs below a directory with ':' in its name).
"""

import os
import re

import numpy as np
from srptest import ROOT, Project, S, check, finish, missing, python

p = Project("data_sources")
keep = []


def hist(name, content):
    h = ROOT.TH1D(name, "", 4, 0.0, 4.0)
    h.SetBinContent(1, content)
    keep.append(h)
    return h


def write(path, contents, trees=(), folders=None):
    """a file with histograms {name: content}, trees {name: entries} (x from 0.5) and folders {folder: {name: content}}"""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    f = ROOT.TFile(path, "RECREATE")
    for name, content in contents.items():
        hist(name, content).Write(name)
    for name, entries in dict(trees).items():
        tree = ROOT.TTree(name, name)
        x = np.zeros(1)
        tree.Branch("x", x, "x/D")
        for i in range(entries):
            x[0] = i + 0.5
            tree.Fill()
        tree.Write()
    for folder, items in (folders or {}).items():
        directory = f.mkdir(folder)
        directory.cd()
        for name, content in items.items():
            if name == "tree":
                tree = ROOT.TTree("t", "t")
                x = np.zeros(1)
                tree.Branch("x", x, "x/D")
                for i in range(content):
                    x[0] = i + 0.5
                    tree.Fill()
                tree.Write()
            else:
                hist(name, content).Write(name)
        f.cd()
    f.Close()


a, b = p.path("in", "a.root"), p.path("in", "b.root")
write(a, {"h": 1, "only_a": 11}, {"t": 10})
write(b, {"h": 2, "only_b": 12}, {"t": 10})
runs = p.path("runs")
for run, content in ((2, 102), (10, 110)):
    write(os.path.join(runs, f"run{run}.root"), {"h": content}, {"t": 10})
with open(os.path.join(runs, "README.txt"), "w") as f:
    f.write("This folder holds the analysis output.\nRun the macro first:\n   root -l make.C\n")
with open(os.path.join(runs, "table.csv"), "w") as f:
    f.write("x,y\n1,2\n2,3\n")
for sub, y in (("ta", 5), ("tb", 7)):
    os.makedirs(p.path(sub))
    with open(p.path(sub, "tab.csv"), "w") as f:
        f.write(f"x,y\n1,{y}\n")
folders = p.path("folders.root")
write(folders, {"h": 0}, folders={"DF_1": {"h": 201, "tree": 10}, "DF_2": {"h": 202, "tree": 10}, "other": {"h": 203}})
odd = p.path("dir with spaces", "ünïcødé (1)", "my file (2).root")
write(odd, {"hist with space": 301, "h-minus+plus": 302}, folders={"folder with space": {"inner": 303}})
colon = p.path("2026-11-06T03:00", "in.root")
write(colon, {"h": 401}, folders={"sub": {"h": 402}})
os.symlink(a, p.path("link.root"))
os.symlink(p.path("does-not-exist.root"), p.path("broken.root"))
with open(p.path("corrupt.root"), "wb") as f:
    f.write(b"root\x00\x00 not really a root file" * 10)
os.link(a, p.path("hardlink.root"))
linkdir = p.path("linkdir")
os.makedirs(linkdir)
os.symlink(p.path("in"), os.path.join(linkdir, "sub"))  # a symlinked directory inside a directory input
os.symlink("run2.root", os.path.join(runs, "z_latest.root"))  # a symlink beside the file it points to
fileLinks = p.path("filelinks")  # a directory of symlinks to files elsewhere
os.makedirs(fileLinks)
for target in (a, b, os.path.join(runs, "run2.root")):
    os.symlink(target, os.path.join(fileLinks, os.path.basename(target)))
os.symlink(runs, p.path("current"))
os.environ["SRP_TEST_INPUTS"] = p.path("in")
cycles = p.path("cycles.root")
f = ROOT.TFile(cycles, "RECREATE")
hist("hc", 501).Write()
hist("hc", 502).Write()
graph = ROOT.TGraph(2)  # ROOT's default name: Graph
for i, y in enumerate((503, 503)):
    graph.SetPoint(i, i, y)
graph.Write()
graph.SetPoint(0, 0, 504)
graph.Write()
inner = f.mkdir("x")
inner.cd()
hist("inside", 505).Write()
deeper = inner.mkdir("y")
deeper.cd()
stack = ROOT.THStack("s", "")
stack.Add(hist("instack", 507))
stack.Write("s", ROOT.TObject.kSingleKey)
f.cd()
hist("x", 506).Write()  # a newer key of the directory's name
f.Close()

sources = {
    "file": [a],
    "two": [a, b],
    "reversed": [b, a],
    "dir": [runs + "/"],
    "wildcard": [os.path.join(runs, "run*.root")],
    "wildcard_one": [os.path.join(runs, "run?.root")],
    "folder": [folders + ":other"],
    "folders": [folders + ":DF_*"],
    "odd": [odd],
    "odd_wildcard": [p.path("dir with spaces") + "/*/my file *.root"],
    "env": ["${SRP_TEST_INPUTS}/a.root"],
    "symlink": [p.path("link.root")],
    "hardlink": [a, p.path("hardlink.root")],
    "dotted": [a, p.path("in") + "/./a.root"],
    "linked_twice": [a, p.path("link.root")],
    "linkdir": [linkdir + "/", b],
    "filelinks": [fileLinks + "/"],
    "dir_and_linked_file": [runs + "/", p.path("current", "run2.root")],
    "cycles_wildcard": [cycles + ":x*"],
    "tables_and_folders": [folders, os.path.join(runs, "table.csv")],
    "broken": [p.path("broken.root"), b],
    "corrupt": [p.path("corrupt.root"), b],
    "no_match": [p.path("in", "nothing*.root"), b],
    "no_folder": [folders + ":XX_*", b],
    "colon": [colon],
    "colon_folder": [colon + ":sub"],
    "cycles": [cycles],
    "tables": [p.path("ta", "tab.csv"), p.path("tb", "tab.csv")],
    "mixed": [runs + "/", a],
}
for name, inputs in sources.items():
    p.pm.AddDataSource(name, inputs)

expected = {}  # plot: (what, [(data name, data source, value or entries; None: missing)], kind of value)


def case(what, items, value="content"):
    name = f"d{len(expected)}"
    plot = S.Plot(name, "sources", "1d")
    for dataName, source, _ in items:
        data = plot[1].AddData(dataName, source)
        if value == "entries":
            data.Project1D(("x", 10, [0.0, 10.0]))
        elif value == "points":
            data.Scatter("x", "y")
    p.pm.AddPlot(plot)
    expected[name] = (what, items, value)


case("a file", [("h", "file", 1)])
case("two files: the first holding a name wins", [("h", "two", 1), ("only_b", "two", 12)])
case("two files in the other order", [("h", "reversed", 2), ("only_a", "reversed", 11)])
case("two files: a tree chained over both", [("t", "two", 20)], "entries")
case("a directory: its files alphabetically, the first holding a name wins", [("h", "dir", 110)])
case("a directory: a tree chained over its files, a symlink beside its file read once", [("t", "dir", 20)], "entries")
case("wildcards in file names: alphabetically", [("h", "wildcard", 110)])
case("wildcards in file names: a tree chained over the matches", [("t", "wildcard", 20)], "entries")
case("a wildcard for one character", [("h", "wildcard_one", 102)])
case("a folder in a file", [("h", "folder", 203)])
case("a wildcard for folders: the first matching folder wins", [("h", "folders", 201)])
case("a wildcard for folders: a tree chained over the matching folders", [("t", "folders", 20)], "entries")
case("spaces, unicode and parentheses in paths and names", [("hist with space", "odd", 301), ("h-minus+plus", "odd", 302), ("folder with space/inner", "odd", 303)])
case("a wildcard in a path with spaces", [("hist with space", "odd_wildcard", 301)])
case("an environment variable", [("h", "env", 1)])
case("a symlinked file", [("h", "symlink", 1)])
for source, how in (("hardlink", "a hard link"), ("dotted", "./ in the path"), ("linked_twice", "a symlink")):
    case(f"the same file through {how}: read once", [("t", source, 10)], "entries")
case("a symlinked directory inside a directory input: skipped, the other inputs are read", [("h", "linkdir", 2)])
case("a directory of symlinks to files elsewhere: every file", [("t", "filelinks", 30)], "entries")
case("a directory and one of its files through a symlinked directory: read once", [("t", "dir_and_linked_file", 20)], "entries")
case("a broken symlink: the other inputs are read", [("h", "broken", 2)])
case("a corrupt file: the other inputs are read", [("h", "corrupt", 2)])
case("no file matching a wildcard: the other inputs are read", [("h", "no_match", 2)])
case("no folder matching a wildcard: the other inputs are read", [("h", "no_folder", 2)])
case("a directory with ':' in its name", [("h", "colon", 401), ("sub/h", "colon", 402), ("h", "colon:sub", 402)])
case("a folder of a file in a directory with ':' in its name", [("h", "colon_folder", 402)])
case("the latest cycle of a key", [("hc", "cycles", 502), ("Graph", "cycles", 504)])
case(
    "a directory hidden by a newer key of its name: the key by name, the directory by a path and by the search",
    [("x", "cycles", 506), ("x/inside", "cycles", 505), ("inside", "cycles", 505), ("inside", "cycles:x", 505), ("x/y/s/instack", "cycles", 507), ("instack", "cycles:x/y", 507), ("instack", "cycles", 507)],
)
case("a directory hidden by a newer key of its name: matched by a wildcard for folders", [("inside", "cycles_wildcard", 505)])
case("tables of a directory: read when requested", [("table", "dir", 2)], "points")
case("the same table name in two inputs: the first wins", [("tab", "tables", 5)], "points")
case("a restricted data source: its folder in the ROOT files", [("h", "tables_and_folders:DF_1", 201)])
case("a restricted data source leaves tables out", [("table", "tables_and_folders:DF_1", None)], "points")
case("data of a data source not defined is reported missing", [("h", "undefined", None)])

# only what is requested is read: a directory with a table and a text file, a plot of a histogram only
plot = S.Plot("only_hist", "lazy", "1d")
plot[1].AddData("h", "dir")
p.pm.AddPlot(plot)
run = p.plot("sources")
reported = missing(run)
warnings = run.lines("WARN") + run.errors()
for name, (what, items, value) in expected.items():
    drawn = run.objects(name, "sources")
    gone = [f"{s}/{n}" if ":" in s else f"{s}:{n}" for n, s, v in items if v is None]
    if gone:
        check(f"{what}: reported missing", not drawn and all(g in reported for g in gone), f"drawn {len(drawn)}, reported {sorted(reported)[:5]}")
        continue
    if len(drawn) != len(items):
        check(what, False, f"{len(drawn)} of {len(items)} drawn; missing {[m for m in reported if any(m.endswith(':' + n) for n, _, _ in items)]}")
        continue

    def got(obj):
        if value == "entries":
            return obj.GetEntries()
        if value == "points":
            return obj.GetPointY(0)
        return obj.GetBinContent(1) if obj.InheritsFrom("TH1") else obj.GetPointY(0)

    wrong = [f"{s}:{n} gives {got(o):g}, expected {v}" for o, (n, s, v) in zip(drawn, items) if got(o) != v]
    check(what, not wrong, "; ".join(wrong))


def reported_once(text):
    lines = [line for line in warnings if text in line]
    return len(lines) == 1, lines


for what, text in (
    ("a skipped spelling of the same file is named", f"Skipping input {p.path('in')}/./a.root of data source dotted: same file as input {a}"),
    ("a skipped hard link of the same file is named", f"Skipping input {p.path('hardlink.root')} of data source hardlink"),
    ("a broken symlink is reported", f"Input file {p.path('broken.root')} not found"),
    ("a corrupt file is reported", f"Cannot open input file {p.path('corrupt.root')} (data source corrupt)"),
    ("a symlink beside the file it points to: the skipped spelling is named", f"Skipping input {os.path.join(runs, 'z_latest.root')} of data source dir: same file as input {os.path.join(runs, 'run2.root')}"),
    ("a symlinked directory inside a directory input is reported", f"Skipping symlinked directory {os.path.join(linkdir, 'sub')} (data source linkdir)"),
    ("no file matching a wildcard is reported", f"No files match {p.path('in', 'nothing*.root')} (data source no_match)"),
    ("no folder matching a wildcard is reported", "No folders match XX_*"),
):
    once, lines = reported_once(text)
    check(f"{what}, once", once, f"{len(lines)} lines: {lines[:2]}")
check("data of an undefined data source: the reason is given", "missing undefined:h (data source not defined)" in run.log, "")
expectedErrors = ("cannot be created", "could not be created")
errors = [line for line in run.errors() if not any(e in line for e in expectedErrors) and "corrupt.root" not in line]  # ROOT reports the corrupt file too
check("no other errors", not errors, " | ".join(errors[:5]))
lazy = p.plot("lazy")
check("tables and text files not requested are not read", lazy.objects("only_hist", "lazy") and not [line for line in lazy.lines("WARN") + lazy.errors() if re.search("table|README|csv", line, re.I)], " | ".join(lazy.lines("WARN") + lazy.errors()))
check("only the requested data is read: the reading summary", "Reading 1 data item from" in lazy.log, " | ".join(lazy.lines("INFO")))

# data source names, and inputs changed between two GeneratePlots in one process
out = python(f"""
import ROOT, shutil
ROOT.TH1.AddDirectory(False)
import SciRooPlot as S
pm = S.PlotManager("regenerate")
pm.AddDataSource("", {a!r})
pm.AddDataSource("bad/name", {a!r})
pm.AddDataSource("unset", ["${{SRP_TESTS_NOT_SET_ANYWHERE}}/a.root", {b!r}])
pm.AddDataSource("relative", ["in/a.root", {b!r}])
pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d"))
pm.AddDataSource("s", {a!r})
pm.AddDataSource("r", {a!r})
for name, data, source in (("t", "t", "s"), ("h", "h", "r"), ("b", "only_b", "s")):
    plot = S.Plot(name, "g", "1d")
    d = plot[1].AddData(data, source)
    if name == "t":
        d.Project1D(("x", 10, [0.0, 10.0]))
    pm.AddPlot(plot)
pm.SetOutputDirectory({p.path("regenerate")!r})
def report(step):
    pm.GeneratePlots("file")
    f = ROOT.TFile.Open({p.path("regenerate", "Plots.root")!r})
    for name in ("t", "h", "b"):
        key = next((k for k in f.Get("g").GetListOfKeys() if k.GetName().split(":")[0] == name), None) if f.Get("g") else None
        pad = key.ReadObj().GetPrimitive("Pad_1") if key else None
        data = [o for o in pad.GetListOfPrimitives() if o.GetName().startswith("1:")] if pad else []
        value = (data[0].GetEntries() if name == "t" else data[0].GetBinContent(1)) if data else None
        print("VALUE", step, name, value)
    f.Close()
report("first")
shutil.copy({b!r}, {a!r} + ".new")
report("unchanged")
pm.AddDataSource("s", {b!r})
pm.AddDataSource("r", {b!r}, True)
report("added")
""")
values = {(step, name): value for step, name, value in re.findall(r"^VALUE (\w+) (\w) (\S+)$", out.stdout, re.M)}
check("a data source without a name is refused", "Specify a name for the data source." in out.text, out.tail())
check("a data source with an illegal character is refused", "Data source 'bad/name' contains illegal character '/'." in out.text, out.tail())
check("an unset environment variable is reported by name", "Environment variable SRP_TESTS_NOT_SET_ANYWHERE used in path ${SRP_TESTS_NOT_SET_ANYWHERE}/a.root is not set." in out.text, out.tail())
check("a relative path is reported and skipped", "The path of an input must not be relative. Skipping in/a.root." in out.text, out.tail())
check("the first GeneratePlots", values.get(("first", "t")) == "10.0" and values.get(("first", "h")) == "1.0" and values.get(("first", "b")) == "None", str(values))
check("a second GeneratePlots without changes gives the same", all(values.get(("unchanged", n)) == values.get(("first", n)) for n in "thb"), str(values))
check("a file appended: the tree chained over both by the next GeneratePlots", values.get(("added", "t")) == "20.0", str(values))
check("a file appended: its data found by the next GeneratePlots", values.get(("added", "b")) == "12.0", str(values))
check("the inputs replaced: the next GeneratePlots reads the new one", values.get(("added", "h")) == "2.0", str(values))
finish()
