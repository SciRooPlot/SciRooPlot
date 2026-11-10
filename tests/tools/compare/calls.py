"""How often a plot run calls functions of ROOT or SciRooPlot (paints, interpreter calls), counted with gdb breakpoints: to see what a
change adds per plot, pad or bin. The project must be defined and selected in the configuration (SCIROOPLOT_CONFIG_PATH).
With a RelWithDebInfo build, --backtrace prints where the first calls of the first function come from.

usage: python3 tests/tools/compare/calls.py <prefix> <group> <name> [--mode file] [--functions TPad::Paint TExec::Exec ...] [--backtrace 3]
       default functions: TPad::Paint, TExec::Exec, TROOT::ProcessLine, THistPainter::PaintColorLevels, TCanvas::Update
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

args = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
args.add_argument("prefix")
args.add_argument("group")
args.add_argument("name")
args.add_argument("--mode", default="file")
args.add_argument("--functions", nargs="*", default=["TPad::Paint", "TExec::Exec", "TROOT::ProcessLine", "THistPainter::PaintColorLevels", "TCanvas::Update"])
args.add_argument("--backtrace", type=int, default=0)
args = args.parse_args()
if not shutil.which("gdb"):
    sys.exit("needs gdb")
plot = ["--args", os.path.join(args.prefix, "bin", "plot"), args.group, args.name, args.mode]
start = ["set breakpoint pending on", "set pagination off", "set print thread-events off"]
counting = start + [f"break {function}" for function in args.functions]
counting += [f"ignore {k} 100000000" for k in range(1, len(args.functions) + 1)] + ["run", "info breakpoints", "kill", "quit"]
done = subprocess.run(["gdb", "-batch", *sum((["-ex", c] for c in counting), []), *plot], capture_output=True, text=True, check=False)
if args.backtrace:
    tracing = start + [f"break {args.functions[0]}", "run"] + ["bt 12", "continue"] * args.backtrace + ["kill", "quit"]
    traced = subprocess.run(["gdb", "-batch", *sum((["-ex", c] for c in tracing), []), *plot], capture_output=True, text=True, check=False)
    for trace in [t for t in re.split(r"\n(?=#0 )", traced.stdout) if t.startswith("#0 ")]:
        print("\n".join(line for line in trace.splitlines() if line.startswith("#")), "\n")
hits, current = {}, None  # the table of info breakpoints: a row per breakpoint, then its lines ("breakpoint already hit n times")
for line in done.stdout.splitlines():
    if row := re.match(r"^(\d+)\s+breakpoint", line):
        current = int(row.group(1))
    elif current and (hit := re.search(r"breakpoint already hit (\d+) time", line)):
        hits[current] = int(hit.group(1))
for k, function in enumerate(args.functions, start=1):
    print(f"{function:<36} {hits.get(k, 0)}")
print(f"plots made: {done.stdout.count('Created plot')}")
