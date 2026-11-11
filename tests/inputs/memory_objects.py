"""Objects handed over in memory (AddDataSource with a PyROOT object or a list of them) are drawn as the same objects read from a file:
every data kind in every variant, given directly, inside a named list (a folder of the data source: addressed by its path and found by
the search), inside a nameless list or an open file (their contents added directly) and inside a directory (a folder). They are copied
when they are added: deleting them afterwards (also by Python's garbage collection), changing them or filling the list they came in
leaves the data source as it was, and the caller's lists and files are left unchanged. The same object can be added to two data sources.
A second AddDataSource with the same name adds to the data source (an object of a name already there replaces it with a warning), with
replace only the new inputs are left; objects and files can be mixed in one data source. A nameless object is skipped with a warning.
Trees in lists, from memory or read from a file, are copied with all their entries. Two PlotManagers of one project keep the objects of
both. The objects reach the plot program through the project (UserData.root); GeneratePlots in the defining process draws the same
(without an output directory one error for every mode that writes files; print, which writes none, shows the definitions).
Covers: lists, directories and files handed over; trees in lists copied; same-name folders merged; the replace warning also across
calls; the caller's lists and files unchanged, two PlotManagers of one project keep both in-memory sources.
"""

import gc
import os
import re

from srptest import DATA_KINDS, ROOT, Project, S, add_data, base_of, cases, check, data_objects, describe, drawn_diff, finish, has_variant, missing, python, source_of

VARIANTS = ("normal", "negative", "nan", "empty", "single", "unsorted", "overflow")
HOLDERS = ("object", "list", "nameless_list", "directory", "file")
keep = []


def own(obj):
    ROOT.SetOwnership(obj, False)
    keep.append(obj)
    return obj


def named_list(name, items, cls="TList"):
    lst = own(getattr(ROOT, cls)())
    if name:
        lst.SetName(name)
    for item in items:
        lst.Add(item)
    return lst


def hist(name, content, owned=True):
    h = ROOT.TH1D(name, "", 4, 0.0, 4.0)
    h.SetBinContent(1, content)
    return own(h) if owned else h


p = Project("memory_objects")
p.inputs(VARIANTS)
todo = cases(
    {"kind": [k for k in DATA_KINDS if "formula" not in DATA_KINDS[k][2]], "holder": list(HOLDERS), "variant": list(VARIANTS)},
    valid=lambda c: has_variant(c["kind"], c["variant"]),
)
memfiles = {}


def holder_file(variant):
    """an open file for the variant (its objects only in memory, nothing written), and a directory in it"""
    if variant not in memfiles:
        memfiles[variant] = own(ROOT.TMemFile(f"m_{variant}.root", "RECREATE"))
        memfiles[variant].mkdir("dir")
    return memfiles[variant]


given = {}  # (holder, variant): objects to hand over once all of the holder are collected
for i, case in enumerate(todo):
    given.setdefault((case["holder"], case["variant"]), set()).add(case["kind"])
for (holder, variant), kinds in given.items():
    objects = {kind: obj for kind, obj in data_objects(variant).items() if kind in kinds}
    source = f"{holder}_{variant}"
    if holder == "object":
        p.pm.AddDataSource(source, list(objects.values()))
    elif holder in ("list", "nameless_list"):
        p.pm.AddDataSource(source, named_list("folder" if holder == "list" else None, list(objects.values())))
    else:
        f = holder_file(variant)
        target = f.GetDirectory("dir") if holder == "directory" else f
        for obj in objects.values():
            # a directory holds an object by its name: functions and graphs are added by hand, histograms move there themselves
            target.Append(obj) if not obj.InheritsFrom("TH1") else obj.SetDirectory(target)
for holder, variant in given:
    if holder in ("directory", "file"):
        f = holder_file(variant)
        p.pm.AddDataSource(f"{holder}_{variant}", f.GetDirectory("dir") if holder == "directory" else f)
for i, case in enumerate(todo):
    kind, holder, variant = case["kind"], case["holder"], case["variant"]
    paths = {"object": [kind], "list": [f"folder/{kind}", kind], "nameless_list": [kind], "directory": [f"dir/{kind}", kind], "file": [kind]}[holder]
    for j, name in enumerate(paths):
        plot = S.Plot(f"c{i}_{j}", "kinds", base_of(kind))
        data = plot[1].AddData(name, f"{holder}_{variant}")
        if "projected" in DATA_KINDS[kind][2]:
            data.Project([0] if DATA_KINDS[kind][0] == 1 else [0, 1])
        p.pm.AddPlot(plot)
for variant in VARIANTS:
    for kind in data_objects(variant):
        if "formula" in DATA_KINDS[kind][2]:
            continue
        plot = S.Plot(f"ref_{kind}_{variant}", "kinds", base_of(kind))
        add_data(plot[1], kind, source_of(variant), variant)
        p.pm.AddPlot(plot)

# lifetimes: what is drawn is the object as it was when it was added
lifetimes = {}  # what: (data name, data source, content of the first bin; None: reported missing)
deleted = ROOT.TH1D("deleted", "", 4, 0.0, 4.0)  # owned by Python
deleted.SetBinContent(1, 11)
p.pm.AddDataSource("deleted", deleted)
del deleted
gc.collect()
lifetimes["deleted by Python after adding it"] = ("deleted", "deleted", 11)
changed = hist("changed", 12)
p.pm.AddDataSource("changed", changed)
changed.SetBinContent(1, 99)
lifetimes["changed after adding it"] = ("changed", "changed", 12)
grown = named_list("grown", [hist("first", 13)])
p.pm.AddDataSource("grown", grown)
grown.Add(hist("late", 14))
lifetimes["its list filled after adding it: the item added before"] = ("grown/first", "grown", 13)
lifetimes["its list filled after adding it: the item added later is not there"] = ("grown/late", "grown", None)
twice = hist("twice", 15)
p.pm.AddDataSource("two_a", twice)
p.pm.AddDataSource("two_b", twice)
lifetimes["one object in two data sources: the first"] = ("twice", "two_a", 15)
lifetimes["one object in two data sources: the second"] = ("twice", "two_b", 15)
p.pm.AddDataSource("appended", hist("one", 16))
p.pm.AddDataSource("appended", [hist("two", 17), hist("one", 18)])
lifetimes["a second call adds to the data source"] = ("two", "appended", 17)
lifetimes["a second call: an object of the same name replaces the first"] = ("one", "appended", 18)
p.pm.AddDataSource("replaced", [hist("old", 19), hist("both", 20)])
p.pm.AddDataSource("replaced", hist("both", 21), True)
lifetimes["replace: the new object"] = ("both", "replaced", 21)
lifetimes["replace: the earlier objects are gone"] = ("old", "replaced", None)
p.pm.AddDataSource("merged", [named_list("out", [hist("h1", 22), hist("h", 23)]), named_list("out", [hist("h2", 24), hist("h", 25)])])
lifetimes["lists of the same name merged: the first"] = ("out/h1", "merged", 22)
lifetimes["lists of the same name merged: the second"] = ("out/h2", "merged", 24)
lifetimes["lists of the same name merged: the same name replaced"] = ("out/h", "merged", 25)
p.pm.AddDataSource("lists", [named_list("la", [hist("h", 26)]), named_list("lb", [hist("h", 27)])])
lifetimes["two lists, the same name in each: the first by path"] = ("la/h", "lists", 26)
lifetimes["two lists, the same name in each: the second by path"] = ("lb/h", "lists", 27)
lifetimes["two lists, the same name in each: the search finds the first"] = ("h", "lists", 26)
p.pm.AddDataSource("nameless", [named_list(None, [hist("k1", 28)]), named_list(None, [hist("k2", 29)], "TObjArray")])
lifetimes["two nameless lists: the first"] = ("k1", "nameless", 28)
lifetimes["two nameless lists: the second"] = ("k2", "nameless", 29)
nested = named_list("outer", [hist("ho", 30), named_list("inner", [hist("hi", 31)], "THashList")])
p.pm.AddDataSource("nested", nested)
lifetimes["a list in a list: by path"] = ("outer/inner/hi", "nested", 31)
lifetimes["a list in a list: by the search"] = ("hi", "nested", 31)
withFile = p.path("mixed.root")
f = ROOT.TFile(withFile, "RECREATE")
hist("onfile", 32, owned=False).Write()
hist("mixed", 33, owned=False).Write()
f.Close()
p.pm.AddDataSource("mixed", hist("mixed", 34))
p.pm.AddDataSource("mixed", withFile)
lifetimes["objects and a file in one data source: the file's object"] = ("onfile", "mixed", 32)
lifetimes["objects and a file in one data source: the first input wins"] = ("mixed", "mixed", 34)

# the caller's lists and files stay as they were
callerFile = own(ROOT.TFile(p.path("caller.root"), "RECREATE"))
hist("ondisk", 35, owned=False).Write()
callerSub = callerFile.mkdir("sub")
callerSub.cd()
hist("insub", 36, owned=False).Write()
callerFile.cd()
inMemory = hist("inmemory", 37)
inMemory.SetDirectory(callerFile)  # newer than anything written: only in memory
ROOT.gROOT.cd()
keysBefore = sorted(k.GetName() for k in callerFile.GetListOfKeys())
memoryBefore = sorted(o.GetName() for o in callerFile.GetList())
p.pm.AddDataSource("callerfile", callerFile)
p.pm.AddDataSource("callerdir", callerSub)
lifetimes["an open file: written objects"] = ("ondisk", "callerfile", 35)
lifetimes["an open file: objects only in memory"] = ("inmemory", "callerfile", 37)
lifetimes["an open file: its directories as folders"] = ("sub/insub", "callerfile", 36)
lifetimes["a directory of a file: a folder"] = ("sub/insub", "callerdir", 36)
check("the caller's file keeps its keys and objects", sorted(k.GetName() for k in callerFile.GetListOfKeys()) == keysBefore and sorted(o.GetName() for o in callerFile.GetList()) == memoryBefore and callerFile.IsOpen(), "changed")
check("the caller's lists keep their items", nested.GetSize() == 2 and nested.At(1).GetSize() == 1, f"sizes {nested.GetSize()}")
callerFile.Close()

# trees in lists: copied with all their entries (10 each, x from 0.5 to 9.5)
ROOT.gInterpreter.Declare(r"""
TTree* srptest_file_tree(const char* path, const char* name) {
  auto* f = new TFile(path, "RECREATE");
  auto* t = new TTree(name, name);
  double x;
  t->Branch("x", &x, "x/D");
  for (int i = 0; i < 10; ++i) { x = i + 0.5; t->Fill(); }
  t->Write();
  delete f;
  return TFile::Open(path)->Get<TTree>(name);
}
TTree* srptest_memory_tree(const char* name) {
  auto* t = new TTree(name, name);
  t->SetDirectory(nullptr);
  double x;
  t->Branch("x", &x, "x/D");
  for (int i = 0; i < 10; ++i) { x = i + 0.5; t->Fill(); }
  return t;
}
""")
trees = {
    "a tree in memory": ("mt", "trees", lambda: own(ROOT.srptest_memory_tree("mt"))),
    "a tree read from a file": ("ft", "trees", lambda: own(ROOT.srptest_file_tree(p.path("ft.root"), "ft"))),
    "a tree read from a file, in a list": ("lst/lt", "trees", lambda: named_list("lst", [own(ROOT.srptest_file_tree(p.path("lt.root"), "lt"))])),
    "a tree read from a file, in a nameless list": ("nt", "trees", lambda: named_list(None, [own(ROOT.srptest_file_tree(p.path("nt.root"), "nt"))])),
    "a tree in memory, in a list": ("lst2/mt2", "trees", lambda: named_list("lst2", [own(ROOT.srptest_memory_tree("mt2"))])),
}
for what, (name, source, make) in trees.items():
    p.pm.AddDataSource(source, make())
    plot = S.Plot(f"tree_{name.replace('/', '_')}", "trees", "1d")
    plot[1].AddData(name, source).Project1D(("x", 10, [0.0, 10.0]))
    p.pm.AddPlot(plot)

for j, (what, (name, source, _)) in enumerate(lifetimes.items()):
    plot = S.Plot(f"l{j}", "lifetimes", "1d")
    plot[1].AddData(name, source)
    p.pm.AddPlot(plot)

run = p.plot()
for i, case in enumerate(todo):
    kind, holder, variant = case["kind"], case["holder"], case["variant"]
    ref = run.objects(f"ref_{kind}_{variant}", "kinds")
    for j, how in enumerate({"list": ("path", "search"), "directory": ("path", "search")}.get(holder, ("search",))):
        found = run.objects(f"c{i}_{j}", "kinds")
        if variant == "empty" and DATA_KINDS[kind][1].startswith(("TGraph", "TEfficiency")) and not ref:
            check(f"not drawn as from a file: {describe(case)} {how}", not found, "drawn")  # an empty graph is not drawn at all
            continue
        check(f"drawn as from a file: {describe(case)} {how}", not drawn_diff(ref, found), drawn_diff(ref, found))
reported = missing(run)
for j, (what, (name, source, content)) in enumerate(lifetimes.items()):
    drawn = run.objects(f"l{j}", "lifetimes")
    if content is None:
        check(f"{what}: reported missing", not drawn and f"{source}:{name}" in reported, f"drawn {len(drawn)}")
    else:
        got = drawn[0].GetBinContent(1) if drawn else None
        check(what, got == content, f"{got}, expected {content}")
for what, (name, _, _) in trees.items():
    drawn = run.objects(f"tree_{name.replace('/', '_')}", "trees")
    entries = drawn[0].GetEntries() if drawn else None
    check(f"{what}: all entries", entries == 10, f"{entries} entries")
expectedErrors = ("cannot be created", "could not be created")
errors = [line for line in run.errors() if not any(e in line for e in expectedErrors)]
check("no errors but the missing data", not errors, " | ".join(errors[:5]))

# messages while defining, a second PlotManager of the project and GeneratePlots in the defining process: in a process of their own
out = python(f"""
import ROOT
ROOT.TH1.AddDirectory(False)
import SciRooPlot as S
def hist(name, content):
    h = ROOT.TH1D(name, "", 4, 0.0, 4.0)
    h.SetBinContent(1, content)
    ROOT.SetOwnership(h, False)
    return h
pm = S.PlotManager("defining")
pm.AddDataSource("nameless", [hist("", 1), hist("named", 2)])
pm.AddDataSource("twice", hist("h", 3))
pm.AddDataSource("twice", hist("h", 4))
lst = ROOT.TList(); lst.SetName("l"); lst.Add(hist("x", 5))
ROOT.SetOwnership(lst, False)
pm.AddDataSource("lists", [lst, lst])
clash = ROOT.TList(); clash.SetName("clash"); clash.Add(hist("x", 7))
inner = ROOT.TList(); inner.SetName("x"); inner.Add(hist("y", 8)); clash.Add(inner)
ROOT.SetOwnership(clash, False)
ROOT.SetOwnership(inner, False)
pm.AddDataSource("clashing", clash)
second = S.PlotManager("defining")
second.AddDataSource("other", hist("o", 6))
pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d"))
for name, data, source in (("a", "named", "nameless"), ("b", "h", "twice")):
    plot = S.Plot(name, "g", "1d")
    plot[1].AddData(data, source)
    pm.AddPlot(plot)
print("PRINTED", pm.GeneratePlots("print"))  # no output directory needed
pm.GeneratePlots("file")  # no output directory yet: said once for all plots
pm.SetOutputDirectory({p.path("defining")!r})
pm.GeneratePlots("file")
f = ROOT.TFile.Open({p.path("defining", "Plots.root")!r})
for name in ("a", "b"):
    key = next((k for k in f.Get("g").GetListOfKeys() if k.GetName().split(":")[0] == name), None)
    pad = key.ReadObj().GetPrimitive("Pad_1") if key else None
    data = [o for o in pad.GetListOfPrimitives() if o.GetName().startswith("1:")] if pad else []
    print("DRAWN", name, data[0].GetBinContent(1) if data else None)
""")
drawnHere = dict(re.findall(r"^DRAWN (\w) (\S+)$", out.stdout, re.M))
check("a nameless object is skipped with a warning", "Cannot add nameless object of type TH1D to data source nameless" in out.text, out.tail())
check("an object beside a nameless one is added", drawnHere.get("a") == "2.0", out.tail())
check("a second object of the same name replaces the first, with a warning", drawnHere.get("b") == "4.0" and "Data source twice receives a second object named h" in out.text, out.tail())
check("GeneratePlots without an output directory: one error for all plots, naming SetOutputDirectory", out.text.count("No output directory was specified") == 1 and "SetOutputDirectory" in out.text, out.tail())
check("GeneratePlots print without an output directory shows the definitions", "PRINTED True" in out.stdout and "Settings of plot" in out.text, out.tail())
check("a sub-list named like an object beside it: one warning in SciRooPlot's words, no ROOT error", "the list clash/x cannot be stored beside the object of the same name" in out.text and "mkdir" not in out.text and "Could not create folder" not in out.text, out.tail())
check("the replace warning names the path in the data source", "Data source lists receives a second object named l/x" in out.text, out.tail())
userData = os.path.join(os.environ["SCIROOPLOT_CONFIG_PATH"], "defining", "UserData.root")
stored = ROOT.TFile.Open(userData) if os.path.exists(userData) else None
check("two PlotManagers of one project: the objects of both are kept in the project", bool(stored and stored.Get("nameless/named") and stored.Get("other/o")), userData)
finish()
