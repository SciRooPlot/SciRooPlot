"""Memory of one process making the same plots again and again (a long-running analysis program): random plot definitions
(srptest.random_definition) made by GeneratePlots in one process, the data buffer and canvases cleared after each batch; prints per batch
the resident memory, the memory after malloc_trim (what the allocator only keeps for reuse is no leak) and the ROOT classes whose object
count grew (Root.ObjectStat). Memory growing while no object count grows is ROOT's interpreter (about 30 kB per ProcessLine, also
for every TExec run when a pad is painted).

usage: PYTHONPATH=tests python3 tests/tools/compare/memory.py <prefix> [--mode file] [--batches 8] [--plots 30] [--seed 1] [--out compare-memory]
"""

import argparse
import os
import subprocess

args = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
args.add_argument("prefix")
args.add_argument("--mode", default="file")
args.add_argument("--batches", type=int, default=8)
args.add_argument("--plots", type=int, default=30)
args.add_argument("--seed", type=int, default=1)
args.add_argument("--out", default="compare-memory")
args = args.parse_args()
out = os.path.abspath(args.out)
os.environ["SRP_PREFIX"], os.environ["SRP_TESTS_WORK"] = os.path.abspath(args.prefix), os.path.join(out, "work")
import srptest as S  # noqa: E402  (after SRP_PREFIX)

definition = S.random_definition(args.seed, args.plots, S.random_inputs(S.WORK), "memory")[0].replace("pm.SaveProject()", "")
program = f"""import ctypes
import ROOT
{definition}
pm.SetOutputDirectory({os.path.join(S.WORK, "out")!r})
libc = ctypes.CDLL("libc.so.6")


def rss():
    with open("/proc/self/status") as f:
        return next(int(line.split()[1]) // 1024 for line in f if line.startswith("VmRSS"))


def counts():
    ROOT.gObjectTable.UpdateInstCount()
    return {{c.GetName(): c.GetInstanceCount() for c in ROOT.gROOT.GetListOfClasses() if c.GetInstanceCount()}}


before = counts()
print(f"start: {{rss()}} MB", flush=True)
for batch in range({args.batches}):
    pm.GeneratePlots({args.mode!r})
    pm.ClearDataBuffer()
    pm.ClearCanvasRegistry()
    resident = rss()
    libc.malloc_trim(0)
    now = counts()
    grown = {{k: now.get(k, 0) - before.get(k, 0) for k in set(now) | set(before) if now.get(k, 0) != before.get(k, 0)}}
    before = now
    print(f"batch {{batch + 1}}: {{resident}} MB, {{rss()}} MB after malloc_trim; objects: {{dict(sorted(grown.items())) or 'none grew'}}", flush=True)
"""
with open(os.path.join(S.WORK, ".rootrc"), "w") as f:
    f.write("Root.ObjectStat: 1\n")
with open(os.path.join(S.WORK, "memory.py"), "w") as f:
    f.write(program)
env = S.prefix_env(os.path.abspath(args.prefix), os.path.join(S.WORK, "config"))
with subprocess.Popen(["python3", "-W", "ignore", "memory.py"], cwd=S.WORK, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True) as proc:
    for line in proc.stdout:
        if line.startswith(("start:", "batch ", "Traceback")) or "Error" in line:
            print(line.rstrip(), flush=True)
