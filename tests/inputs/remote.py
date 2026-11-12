"""Inputs on a remote server (root://, read through ROOT's XRootD plugin) give the same data as the same inputs on the local disk: a
file, a folder in it (also with wildcards), wildcards in file names and in directory levels (alphabetically, a directory without the
file is left out), a directory (every ROOT file below it, alphabetically), an environment variable in the URL, a query after the file
name (also before a folder), a restricted data source, trees and RNTuples chained over remote files. A remote file that cannot be
opened, a wildcard matching nothing and a remote table are reported and the other inputs are still read.
Needs the xrootd server and ROOT's XRootD plugin (skipped without them); the server runs on a free local port, serving a temporary directory.
Covers: a wildcard level whose match is a directory without the file, a query before a folder.
"""

import atexit
import os
import shutil
import socket
import subprocess
import tempfile
import time

import numpy as np
from srptest import ROOT, Project, S, check, finish, missing, skip, write_tree

if not shutil.which("xrootd"):
    skip("no xrootd server installed")
if not ROOT.gSystem.DynamicPathName("libNetxNG", True):
    skip("ROOT is built without its XRootD plugin")

served = tempfile.mkdtemp(prefix="srp-xrootd-")  # outside the work directory: the server may run as another user
os.chmod(served, 0o755)
with socket.socket() as s:
    s.bind(("localhost", 0))
    port = s.getsockname()[1]
config = os.path.join(served, "..", os.path.basename(served) + ".cfg")
with open(config, "w") as f:
    f.write(f"all.export /\noss.localroot {served}\nxrd.port {port}\n")
command = ["xrootd", "-c", config, "-l", os.path.join(served, "..", os.path.basename(served) + ".log")]
if os.geteuid() == 0:
    command[1:1] = ["-R", "nobody"]  # xrootd refuses to run as root
server = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def stop():
    server.terminate()
    server.wait()
    shutil.rmtree(served, ignore_errors=True)
    for ending in (".cfg", ".log"):
        if os.path.exists(path := os.path.join(served, "..", os.path.basename(served) + ending)):
            os.remove(path)


atexit.register(stop)
for _ in range(100):
    with socket.socket() as s:
        if s.connect_ex(("localhost", port)) == 0:
            break
    time.sleep(0.1)
else:
    check("the xrootd server starts", False, f"no answer on port {port}")
    finish()

URL = f"root://localhost:{port}/"
keep = []


def write(relative, contents, folders=None, tree=0):
    """a served file with histograms {name: content}, folders {folder: {name: content}} and a tree t of tree entries (x from 0.5)"""
    path = os.path.join(served, relative)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    f = ROOT.TFile(path, "RECREATE")
    for folder, items in [("", contents), *(folders or {}).items()]:
        directory = f.mkdir(folder) if folder else f
        directory.cd()
        for name, content in items.items():
            h = ROOT.TH1D(name, "", 4, 0.0, 4.0)
            h.SetBinContent(1, content)
            h.Write(name)
            keep.append(h)
    f.Close()
    if tree:
        write_tree(path, {"x": np.arange(tree) + 0.5})


write("a.root", {"h": 1, "only_a": 11}, {"DF_1": {"h": 21}, "DF_2": {"h": 22}}, tree=10)
write("b.root", {"h": 2, "only_b": 12}, tree=10)
write("runs/run2.root", {"h": 102}, tree=10)
write("runs/run10.root", {"h": 110}, tree=10)
write("runs/sub/run1.root", {"h": 101}, tree=10)  # below the directory, not matched by run*.root of runs/
write("periods/A/out.root", {"h": 201})
write("periods/B/out.root", {"h": 202})
os.makedirs(os.path.join(served, "periods", "C"))  # a directory without the file
os.makedirs(os.path.join(served, "ntuples"))
write_tree(os.path.join(served, "ntuples", "n1.root"), {"x": np.arange(5) + 0.5}, name="n", ntuple=True)
write_tree(os.path.join(served, "ntuples", "n2.root"), {"x": np.arange(5) + 0.5}, name="n", ntuple=True)
with open(os.path.join(served, "table.csv"), "w") as f:
    f.write("x,y\n1,2\n")
for root, dirs, files in os.walk(served):
    for name in dirs + files:
        os.chmod(os.path.join(root, name), 0o755)
os.environ["SRP_TEST_SERVER"] = f"root://localhost:{port}"

p = Project("remote")
sources = {
    "file": [URL + "/a.root"],
    "folder": [URL + "/a.root:DF_2"],
    "folders": [URL + "/a.root:DF_*"],
    "wildcard": [URL + "/runs/run*.root"],
    "level": [URL + "/periods/*/out.root"],
    "dir": [URL + "/runs/"],
    "env": ["${SRP_TEST_SERVER}//b.root"],
    "query": [URL + "/a.root?xrd.wantprot=unix"],
    "query_folder": [URL + "/a.root?xrd.wantprot=unix:DF_1"],
    "two": [URL + "/a.root", URL + "/b.root"],
    "ntuples": [URL + "/ntuples/*.root"],
    "missing_file": [URL + "/nothing.root", URL + "/b.root"],
    "no_match": [URL + "/runs/nothing*.root", URL + "/b.root"],
    "table": [URL + "/table.csv", URL + "/b.root"],
}
for name, inputs in sources.items():
    p.pm.AddDataSource(name, inputs)

expected = {}  # plot: (what, [(data name, data source, value or entries)], kind of value)


def case(what, items, value="content"):
    name = f"r{len(expected)}"
    plot = S.Plot(name, "remote", "1d")
    for dataName, source, _ in items:
        data = plot[1].AddData(dataName, source)
        if value == "entries":
            data.Project1D(("x", 10, [0.0, 10.0]))
    p.pm.AddPlot(plot)
    expected[name] = (what, items, value)


case("a remote file", [("h", "file", 1), ("DF_1/h", "file", 21)])
case("a folder in a remote file", [("h", "folder", 22)])
case("a wildcard for folders in a remote file: the first matching folder wins", [("h", "folders", 21)])
case("wildcards in remote file names: alphabetically", [("h", "wildcard", 110)])
case("wildcards in remote file names: a tree chained over the matches", [("t", "wildcard", 20)], "entries")
case("a wildcard for a remote directory level: alphabetically, a directory without the file left out", [("h", "level", 201)])
case("a remote directory: every ROOT file below it, alphabetically", [("h", "dir", 110)])
case("a remote directory: a tree chained over every file below it", [("t", "dir", 30)], "entries")
case("an environment variable in a URL", [("h", "env", 2)])
case("a query after the file name", [("h", "query", 1)])
case("a query before a folder", [("h", "query_folder", 21)])
case("a restricted data source of a remote file", [("h", "file:DF_2", 22)])
case("two remote files: the first holding a name wins", [("h", "two", 1), ("only_b", "two", 12)])
case("two remote files: a tree chained over both", [("t", "two", 20)], "entries")
case("an RNTuple chained over remote files", [("n", "ntuples", 10)], "entries")
case("a remote file that cannot be opened: the other inputs are read", [("h", "missing_file", 2)])
case("no remote file matching a wildcard: the other inputs are read", [("h", "no_match", 2)])
case("a remote table: the other inputs are read", [("h", "table", 2)])

# ROOT opens a root:// file on this machine as a local file wherever it is not opened with READ (RDataFrame): a server elsewhere is never local
home = p.path("home")
os.makedirs(home)
with open(os.path.join(home, ".rootrc"), "w") as f:
    f.write("TFile.ForceRemote: 1\n")
run = p.plot("remote", env={"HOME": home})
reported = missing(run)
for name, (what, items, value) in expected.items():
    drawn = run.objects(name, "remote")
    if len(drawn) != len(items):
        check(what, False, f"{len(drawn)} of {len(items)} drawn; missing {sorted(reported)[:5]}")
        continue
    got = [(o.GetEntries() if value == "entries" else o.GetBinContent(1)) for o in drawn]
    wrong = [f"{s}:{n} gives {g:g}, expected {v}" for g, (n, s, v) in zip(got, items) if g != v]
    check(what, not wrong, "; ".join(wrong))

warnings = run.lines("WARN")
for what, text in (
    ("a remote file that cannot be opened is reported", f"Cannot open input file {URL}/nothing.root (data source missing_file)"),
    ("no remote file matching a wildcard is reported", f"No files match {URL}/runs/nothing*.root (data source no_match)"),
    ("a remote table is reported", f"Skipping table {URL}/table.csv (data source table): tables can only be read from local files."),
):
    lines = [line for line in warnings if text in line]
    check(f"{what}, once", len(lines) == 1, f"{len(lines)} lines: {lines[:2]}")
errors = [line for line in run.errors() if "nothing.root" not in line]  # ROOT reports the file it cannot open too
check("no other errors", not errors, " | ".join(errors[:5]))
finish()
