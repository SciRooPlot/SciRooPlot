"""A tree or RNTuple found in several inputs of a data source is read as one chain, its rows in the documented order (skill, Data sources):
the inputs in the order given, the files of a directory and of a wildcard alphabetically (also a wildcard in a directory of the path),
the folders of a wildcard in a file (file.root:DF_*) in the order of the file; a whole file contributes its first match of the name (the
search: its folder first, then its sub-folders), a data source restricted to a folder (src:DF_1, src:DF_[45]) that folder of every file
holding it; inputs without the tree and trees without rows add nothing; in-memory trees join the chain where they were added. Entries(n)
and Entries(a, b) count the rows of the whole chain, across the boundaries of its inputs. RNTuples are chained when they have the same
path in every input (also given as a path in the name). Inputs that cannot be chained are reported, the plot is lost: a column of another
type or missing in one input (the error names what the tree is chained from and the reason ROOT's tree reader gives), RNTuples under different paths, a TTree in some inputs and
an RNTuple in others. The reading summary names the inputs a chain is made of.
Covers: chained trees (AO2D-like folders in the order of the file, wildcards in directories, ordering by
Entries, the first match per whole file, schema problems), RNTuple chains.
"""

import os

import numpy as np
from srptest import ROOT, Project, S, check, finish, missing, write_tree

p = Project("chains")
blocks = {}  # block number: its row ids


def tree(path, block, n, folder="", name="t", ntuple=False, v_type=np.float64, columns=("idx", "v")):
    """a tree of n rows with the ids block*100 + row (idx) and v = idx / 2, written into a file (a folder of it)"""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    ids = np.arange(block * 100, block * 100 + n, dtype=np.int64)
    blocks[block] = ids
    data = {"idx": ids, "v": (ids * 0.5).astype(v_type), "w": ids * 0.25}
    write_tree(path, {c: data[c] for c in columns}, name=name, folder=folder, ntuple=ntuple)
    return path


def hist_only(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    f = ROOT.TFile(path, "RECREATE")
    ROOT.TH1D("h", "", 1, 0.0, 1.0).Write()
    f.Close()
    return path


def key_order(path, prefix=""):
    """the folders of a file in the order ROOT lists its keys"""
    f = ROOT.TFile(path)
    names = list(dict.fromkeys(k.GetName() for k in f.GetListOfKeys() if k.GetName().startswith(prefix)))
    f.Close()
    return names


A = tree(p.path("a.root"), 1, 30)
B = tree(p.path("b.root"), 2, 40)
for block, run in ((3, 2), (4, 10), (5, 3)):
    tree(p.path("parts", f"part{run}.root"), block, 15 + block)
for block, sub in ((6, "a/x.root"), (7, "b/c/y.root")):
    tree(p.path("dir", sub), block, 10 + block)
tree(p.path("dir", "b", "w.root"), 8, 14, folder="nested")  # the tree only in a sub-folder of the file
hist_only(p.path("dir", "d", "z.root"))
with open(p.path("dir", "a", "table.csv"), "w") as f:
    f.write("idx,v\n1,2\n")
for block, run in ((9, 2), (10, 11), (11, 3)):
    tree(p.path("runs", f"run{run}", "AO2D.root"), block, 10 + block)
AO2D = {"AO2D_1.root": (("DF_3", 12), ("DF_1", 13)), "AO2D_10.root": (("DF_2", 14),), "AO2D_2.root": (("DF_5", 15), ("DF_4", 16), ("other", 17))}
folderBlocks = {}
for fileName, folders in AO2D.items():
    for folder, block in folders:
        tree(p.path("ao", fileName), block, block - 5, folder=folder, name="O2track")
        folderBlocks[(fileName, folder)] = block
E = p.path("empty.root")
write_tree(E, {"idx": np.zeros(0, dtype=np.int64), "v": np.zeros(0)})
H = hist_only(p.path("histonly.root"))
C = tree(p.path("c.root"), 18, 5)
F = tree(p.path("f.root"), 19, 3, folder="DF_1")
tree(F, 20, 4, folder="DF_2")
memoryFile = ROOT.TFile.Open(tree(p.path("memory", "m.root"), 21, 6))
memoryTree = memoryFile.Get("t")
# RNTuples
RA = tree(p.path("rn", "a.root"), 22, 10, name="nt", ntuple=True)
RB = tree(p.path("rn", "b.root"), 23, 11, name="nt", ntuple=True)
tree(RA, 24, 7, folder="sub", name="nt", ntuple=True)
tree(RB, 25, 8, folder="sub", name="nt", ntuple=True)
RC = tree(p.path("rn", "c.root"), 26, 5, folder="other", name="nt", ntuple=True)
TNT = tree(p.path("rn", "tree_nt.root"), 27, 5, name="nt")  # a TTree of the RNTuple's name
# inputs that cannot be chained
S0 = tree(p.path("schema", "s0.root"), 28, 10)
S1 = tree(p.path("schema", "s1.root"), 29, 10, v_type=np.float32)
S2 = tree(p.path("schema", "s2.root"), 30, 10, columns=("idx", "w"))

aoOrder = [(f, folder) for f in sorted(AO2D) for folder in key_order(p.path("ao", f), "DF_")]
firstPerFile = [next((f, folder) for folder in key_order(p.path("ao", f)) if (f, folder) in folderBlocks) for f in sorted(AO2D)]
sources = {  # data source: (inputs, data name, the blocks of the chain in order, what the reading summary names)
    "files": ([B, A], "t", [2, 1], "2 files"),
    "wildcard": ([p.path("parts", "part*.root")], "t", [4, 3, 5], "3 files"),
    "directory": ([p.path("dir") + "/"], "t", [6, 7, 8], "3 files"),
    "directory_wildcard": ([p.path("runs", "run*", "AO2D.root")], "t", [10, 9, 11], "3 files"),
    "folders": ([p.path("ao", "AO2D_*.root") + ":DF_*"], "O2track", [folderBlocks[k] for k in aoOrder], f"{len(aoOrder)} folders in 3 files"),
    "folders_some": ([p.path("ao", "AO2D_?.root") + ":DF_[13]"], "O2track", [folderBlocks[k] for k in aoOrder if k[0] == "AO2D_1.root" and k[1] in ("DF_1", "DF_3")], "2 folders in 1 file"),
    "whole_files": ([p.path("ao", "AO2D_*.root")], "O2track", [folderBlocks[k] for k in firstPerFile], "3 files"),
    "whole_files:DF_1": (None, "O2track", [13], ""),
    "whole_files:DF_[45]": (None, "O2track", [folderBlocks[k] for k in aoOrder if k[1] in ("DF_4", "DF_5")], "2 folders in 1 file"),
    "with_empty": ([E, A], "t", [1], "2 files"),
    "without_tree": ([A, H, B], "t", [1, 2], "2 files"),
    "files_and_folders": ([C, F + ":DF_*"], "t", [18, 19, 20], "1 file and 2 folders in 1 file"),
    "file_and_memory": ([A], "t", [1, 21], "1 file and 1 folder in 1 file"),
    "rntuples": ([RB, RA], "nt", [23, 22], "2 files"),
    "rntuple_folders": ([RA + ":sub", RB + ":sub"], "nt", [24, 25], "2 folders in 2 files"),
    "rntuple_files": ([RA, RB], "sub/nt", [24, 25], "2 files"),
}
for source, (inputs, _, _, _) in sources.items():
    if inputs:
        p.pm.AddDataSource(source, inputs)
p.pm.AddDataSource("file_and_memory", memoryTree)
failing = {  # data source: (inputs, data name, the error)
    "column_type": ([S0, S1], "t", ("contains data of type float", "the tree is chained from 2 files: do all inputs of the data source have the same columns?")),
    "column_missing": ([S0, S2], "t", ("does not have a branch called v", "the tree is chained from 2 files: do all inputs of the data source have the same columns?")),
    "rntuple_paths": ([RA + ":sub", RC + ":other"], "nt", f"ROOT chains RNTuples only under the same path, but it is sub/nt in {RA} and other/nt in {RC}"),
    "tree_and_rntuple": ([RA, TNT], "nt", "it is a TTree in some inputs and an RNTuple in others"),
}
for source, (inputs, _, _) in failing.items():
    p.pm.AddDataSource(source, inputs)


def rows(order):
    return np.concatenate([blocks[b] for b in order]) if order else np.zeros(0, dtype=np.int64)


expected = {}  # plot: (what, expected ids)
for source, (_, name, order, _) in sources.items():
    for entries in (None, (7,), (5, 35)):
        plotName = f"c{len(expected)}"
        plot = S.Plot(plotName, "chains", "1d")
        data = plot[1].AddData(name, source).Scatter("idx", "v")
        ids = rows(order)
        if entries:
            data.Entries(*entries)
            ids = ids[: entries[0]] if len(entries) == 1 else ids[entries[0] : entries[1]]
        p.pm.AddPlot(plot)
        expected[plotName] = (f"{source}{f': Entries{entries}' if entries else ''}", ids)
for source, (_, name, _) in failing.items():
    plot = S.Plot(source, "failing", "1d")
    plot[1].AddData(name, source).Project1D(("v", 10, [0.0, 100.0]))
    p.pm.AddPlot(plot)

run = p.plot("chains")
for plotName, (what, ids) in expected.items():
    obs = run.objects(plotName, "chains")
    got = [int(obs[0].GetPointX(i)) for i in range(obs[0].GetN())] if obs else None
    ok = got == list(ids) and all(obs[0].GetPointY(i) == ids[i] * 0.5 for i in range(len(ids)))
    check(f"{what}: the rows of the chain in the documented order", ok, f"ids {got[:12] if got is not None else None}..., expected {list(ids)[:12]}...")
log = run.log
for source, (_, name, order, inputs) in sources.items():
    location = f"{source}/{name}" if ":" in source else f"{source}:{name}"  # a restricted data source as <source>:<folder>/<name>
    summary = [line for line in log.splitlines() if f" - {location} " in line]
    n = len(rows(order))
    want = f" - {location} {n} entries" + (f" from {inputs}," if inputs else ",")
    check(f"{source}: the reading summary names the inputs of the chain", summary and all(want in line for line in summary), f"{summary[:2]}, expected '{want}'")
expectedWarnings = [f"No folders match DF_[13] in {p.path('ao', 'AO2D_2.root')} (data source folders_some)", "labels of the X axis"]  # the look of long index labels
other = [w for w in run.warnings() if not any(e in w for e in expectedWarnings)]
check("the chains give no other warnings", not other, " | ".join(other[:5]))
check("the chains give no errors", not run.errors(), " | ".join(run.errors()[:5]))

failed = p.plot("failing")
for source, (_, name, text) in failing.items():
    lost = not failed.canvas(source, "failing")
    reported = [line for line in failed.errors() if all(t in line for t in ((text,) if isinstance(text, str) else text)) and f"{source}:{name}" in line]
    check(f"{source}: inputs that cannot be chained are reported, the plot is lost", lost and reported, f"plot {'lost' if lost else 'made'}; errors {failed.errors()[:3]}")
    check(f"{source}: reported as missing", f"{source}:{name}" in missing(failed), str(sorted(missing(failed))))
check("the reason of ROOT's tree reader is in the error, not a line of its own", not [line for line in failed.log.splitlines() if "Error in <TTreeReader" in line], failed.log[-400:])
finish()
