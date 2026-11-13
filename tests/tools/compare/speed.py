"""How fast builds or installations make the same plots: the plot program timed on benchmark projects, the prefixes taking turns, each run
repeated; prints the median time per project and prefix. Projects: many histograms in one pad with a legend (--items histograms of --bins
bins), large 2d histograms (one per plot, --bins x --bins / 10), random plot definitions (srptest.random_definition, --seeds), many plots
of one histogram each (--many plots), one pad crowded with data (--crowded histograms in one plot).
Runs on a 2-core machine vary by about 10 %: compare medians of several repeats, with nothing else running. For reference, on the 2-core
container (2026-11-12, mode file): 3000 plots of one project 55 s, 300 data in one pad 1.2 s.

usage: PYTHONPATH=tests python3 tests/tools/compare/speed.py <prefix> [<prefix> ...] [--repeat 3] [--mode file] [--items 50] [--bins 1000]
       [--plots 20] [--seeds 1] [--many 3000] [--crowded 300] [--out compare-speed]
"""

import argparse
import os
import shutil
import statistics
import subprocess
import time

args = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
args.add_argument("prefixes", nargs="+")
args.add_argument("--repeat", type=int, default=3)
args.add_argument("--mode", default="file")
args.add_argument("--items", type=int, default=50)
args.add_argument("--bins", type=int, default=1000)
args.add_argument("--plots", type=int, default=20)
args.add_argument("--seeds", type=int, nargs="*", default=[1])
args.add_argument("--many", type=int, default=3000)
args.add_argument("--crowded", type=int, default=300)
args.add_argument("--out", default="compare-speed")
args = args.parse_args()
out = os.path.abspath(args.out)
prefixes = [os.path.abspath(p) for p in args.prefixes]
os.environ["SRP_PREFIX"], os.environ["SRP_TESTS_WORK"] = prefixes[0], os.path.join(out, "work")
import srptest as S  # noqa: E402  (after SRP_PREFIX)
from srptest import ROOT  # noqa: E402

inputs = os.path.join(S.WORK, "bench.root")
rng = ROOT.TRandom3(1)
objects = {}
for i in range(args.items):
    h = ROOT.TH1D(f"h{i}", "", args.bins, -5.0, 5.0)
    h.FillRandom("gaus", 20000)
    objects[f"h{i}"] = h
side = max(args.bins // 10, 10)
h2 = ROOT.TH2D("h2", "", side, -5.0, 5.0, side, -5.0, 5.0)
for _ in range(200000):
    h2.Fill(rng.Gaus(), rng.Gaus())
objects["h2"] = h2
S.write_objects(inputs, objects)
programs = {
    "histograms": f'''from SciRooPlot import *
pm = PlotManager("histograms")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
pm.AddDataSource("s", {inputs!r})
for n in range({args.plots}):
    p = Plot(f"p{{n}}", "g", "1d")
    for i in range({args.items}):
        p[1].AddData(f"h{{i}}", "s", f"h{{i}}")
    p[1].AddLegend()
    pm.AddPlot(p)
pm.SaveProject()
''',
    "maps": f'''from SciRooPlot import *
pm = PlotManager("maps")
pm.AddBasePlot(PlotManager.MakeBasePlot("2d"))
pm.AddDataSource("s", {inputs!r})
for n in range({args.plots}):
    p = Plot(f"p{{n}}", "g", "2d")
    p[1].AddData("h2", "s")
    pm.AddPlot(p)
pm.SaveProject()
''',
    "many_plots": f'''from SciRooPlot import *
pm = PlotManager("many_plots")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
pm.AddDataSource("s", {inputs!r})
for n in range({args.many}):
    p = Plot(f"p{{n}}", "g", "1d")
    p[1].AddData(f"h{{n % {args.items}}}", "s")
    pm.AddPlot(p)
pm.SaveProject()
''',
    "crowded": f'''from SciRooPlot import *
pm = PlotManager("crowded")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
pm.AddDataSource("s", {inputs!r})
p = Plot("p", "g", "1d")
for i in range({args.crowded}):
    p[1].AddData(f"h{{i % {args.items}}}", "s")
pm.AddPlot(p)
pm.SaveProject()
''',
}
nPlots = {"many_plots": args.many, "crowded": 1}  # the others make --plots plots
sources = S.random_inputs(S.WORK) if args.seeds else {}
for seed in args.seeds:
    programs[f"random{seed}"] = S.random_definition(seed, args.plots, sources, f"random{seed}")[0]

times = {}  # (project, prefix): [seconds]
for k, prefix in enumerate(prefixes):
    config = os.path.join(out, f"p{k}", "config")
    shutil.rmtree(os.path.dirname(config), ignore_errors=True)
    for project, code in programs.items():
        folder = os.path.join(out, f"p{k}", project)
        os.makedirs(folder)
        with open(os.path.join(folder, "DefinePlots.py"), "w") as f:
            f.write(code)
        env = S.prefix_env(prefix, config)
        subprocess.run(["python3", "-W", "ignore", "DefinePlots.py"], cwd=folder, env=env, capture_output=True, check=False)
        subprocess.run([os.path.join(prefix, "bin", "srp"), "add", project, os.path.join(folder, "DefinePlots.py"), os.path.join(folder, "out")], env=env, capture_output=True, check=False)
for _ in range(args.repeat):
    for project in programs:
        for k, prefix in enumerate(prefixes):
            env = S.prefix_env(prefix, os.path.join(out, f"p{k}", "config"))
            subprocess.run([os.path.join(prefix, "bin", "srp"), "select", project], env=env, capture_output=True, check=False)
            start = time.time()
            done = subprocess.run([os.path.join(prefix, "bin", "plot"), ".+", ".+", args.mode], env=env, capture_output=True, text=True, check=False)
            times.setdefault((project, prefix), []).append(time.time() - start)
            if done.stdout.count("Created plot") != nPlots.get(project, args.plots):
                print(f"   {project} with {prefix}: {done.stdout.count('Created plot')} of {nPlots.get(project, args.plots)} plots made")
width = max(len(p) for p in prefixes)
for project in programs:
    print(f"== {project} (mode {args.mode}, median of {args.repeat})")
    for prefix in prefixes:
        runs = times[(project, prefix)]
        print(f"   {prefix:<{width}}  {statistics.median(runs):7.2f} s   ({', '.join(f'{t:.2f}' for t in runs)})")
