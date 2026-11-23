"""Data not found in its data source is reported with the names like it in the inputs (did you mean …), best first and at most three: the
same name in other case, the same name in another folder, then names a few typos away (a character inserted, left out, replaced or two
neighbours swapped; case ignored; one typo for names of up to 4 characters, two up to 10, three beyond; among equally good ones those
whose whole path is closer first, e.g. in the same folder), searched in every input of the
data source, in directories and in collections (lists, stacks, multigraphs, folders, maps), in tables, RNTuples included, and for a
data source or an input restricted to a folder (src:folder, file.root:folder; a directory or a list) only in that folder. A name nothing is close to gets no suggestion. A data source that is not defined gets the
defined data sources like its name.
Covers: case, folder, insertion, deletion, replacement, swap, too many typos, the limit of three, the order, collections, a second input,
tables, an RNTuple, restricted data sources and inputs (directory, list), an undefined data source.
"""

import os

import numpy as np

from srptest import ROOT, Project, S, check, collection, finish, one_bin, own, put, write_csv, write_tree

p = Project("similar_names")
path = p.path("inputs.root")
f = ROOT.TFile(path, "RECREATE")
one_bin("hPt", 1).Write()
one_bin("hPtSum", 1).Write()
results = f.mkdir("results")
results.cd()
one_bin("hSpec", 1).Write()
one_bin("hSpecRaw", 1).Write()
deep = f.mkdir("sub/deep")
deep.cd()
one_bin("hDeep", 1).Write()
many = f.mkdir("many")
many.cd()
for k in range(1, 6):
    one_bin(f"h{k}", 1).Write()
f.cd()
for kind, name, item in (("TList", "lst", "hEta"), ("TFolder", "fold", "hFolded"), ("TMap", "map", "hMapped")):
    container = collection(kind, name)
    put(container, one_bin(item, 1))
    container.Write(name, ROOT.TObject.kSingleKey)
stack = own(ROOT.THStack("stack", ""))
stack.Add(one_bin("hA", 1))
stack.Add(one_bin("hB", 1))
stack.Write("stack")
graph = own(ROOT.TGraph(2))
graph.SetName("gX")
multi = own(ROOT.TMultiGraph("mg", ""))
multi.Add(graph)
multi.Write("mg")
f.Close()
p.pm.AddDataSource("data", path)
p.pm.AddDataSource("folder", path + ":results")
p.source("other", {"hOther": one_bin("hOther", 1)})
p.pm.AddDataSource("two", [path, p.path("other.root")])
os.makedirs(p.path("tables"))
write_csv(p.path("tables", "ptTable.csv"), {"x": [1.0, 2.0]})
p.pm.AddDataSource("tables", p.path("tables"))
p.pm.AddDataSource("inlist", path + ":lst")
write_tree(p.path("ntuple.root"), {"x": np.array([1.0, 2.0])}, name="events", ntuple=True)
p.pm.AddDataSource("nt", p.path("ntuple.root"))

CASES = (  # name, data source, the suggestions expected (best first), what it checks
    ("hpt", "data", ["hPt"], "the same name in other case"),
    ("hspec", "data", ["results/hSpec"], "the same name in another folder, in other case"),
    ("hSpce", "data", ["results/hSpec"], "two neighbours swapped"),
    ("results/hSpecRw", "data", ["results/hSpecRaw", "results/hSpec"], "a character left out, then a name two typos away"),
    ("results/hSpeec", "data", ["results/hSpec"], "a character inserted"),
    ("hDep", "data", ["sub/deep/hDeep"], "a character left out, two folders down"),
    ("hEtaa", "data", ["lst/hEta"], "in a list"),
    ("fold/hfolded", "data", ["fold/hFolded"], "in a folder"),
    ("hMaped", "data", ["map/hMapped"], "in a map"),
    ("stack/hC", "data", ["stack/hA", "stack/hB", "many/h1"], "in a stack: a character replaced, the same folder first"),
    ("gx", "data", ["mg/gX"], "in a multigraph"),
    ("many/h0", "data", ["many/h1", "many/h2", "many/h3"], "at most three"),
    ("hPx", "data", ["hPt"], "one typo in a short name"),
    ("hXy", "data", [], "two typos in a short name: too many"),
    ("results/hSpecRawXYZW", "data", [], "four typos in a long name: too many"),
    ("zzz", "data", [], "nothing like it"),
    ("hother", "two", ["hOther"], "in the second input"),
    ("pttable", "tables", ["ptTable"], "a table"),
    ("hspec", "folder", ["hSpec"], "a restricted data source: within its folder"),
    ("hpt", "folder", [], "a restricted data source: not outside its folder"),
    ("hSpce", "data:results", ["hSpec"], "a data source restricted to a directory (src:folder)"),
    ("hpt", "data:results", [], "a data source restricted to a directory: not outside it"),
    ("hEtaa", "data:lst", ["hEta"], "a data source restricted to a list (src:list)"),
    ("hEtaa", "inlist", ["hEta"], "an input that is a list (file.root:list)"),
    ("event", "nt", ["events"], "an RNTuple"),
)
for i, (name, source, _, _) in enumerate(CASES):
    plot = S.Plot(f"n{i}", "names", "1d")
    plot[1].AddData(name, source)
    p.pm.AddPlot(plot)
for i, source in enumerate(("dta", "xyz")):
    plot = S.Plot(f"s{i}", "sources", "1d")
    plot[1].AddData("hPt", source)
    p.pm.AddPlot(plot)
run = p.plot(mode="png")


def reason(location):
    """the reason given for data reported missing at a location (data source:name), None if it is not reported"""
    for line in run.log.splitlines():
        if line.strip().startswith(f"- missing {location} ("):
            return line.strip()[len(f"- missing {location} (") : -1]
    return None


for name, source, expected, what in CASES:
    location = f"{source}:{name}" if ":" not in source else f"{source}/{name}"
    want = f"not found in the inputs of {source}" + (f"; did you mean {', '.join(expected)}?" if expected else "")
    got = reason(location)
    check(f"{what}: {location}", got == want, f"{got}, expected {want}")
check("an undefined data source: the defined ones like it", reason("dta:hPt") == "data source not defined; did you mean data?", reason("dta:hPt"))
check("an undefined data source like none: no suggestion", reason("xyz:hPt") == "data source not defined", reason("xyz:hPt"))
finish()
