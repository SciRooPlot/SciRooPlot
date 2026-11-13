"""What two builds or installations draw, compared pixel by pixel: the Python example project and random plot definitions
(srptest.random_definition) plotted by each as png. Prints the plots that differ (or exist with one only) and, with ImageMagick, writes
an image of each next to the other (A left, B right) into <out>/differences. For changes that must not change any output.

usage: PYTHONPATH=tests python3 tests/tools/compare/outputs.py <prefix A> <prefix B> [<out>] [--seeds 1 2 3] [--plots 40] [--mode png]
       (prefixes: build or install directories; out: default ./compare-outputs; a few minutes per seed)
"""

import argparse
import os
import shutil
import subprocess
import sys

args = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
args.add_argument("a")
args.add_argument("b")
args.add_argument("out", nargs="?", default="compare-outputs")
args.add_argument("--seeds", type=int, nargs="*", default=[1, 2, 3])
args.add_argument("--plots", type=int, default=40)
args.add_argument("--mode", default="png")
args = args.parse_args()
out = os.path.abspath(args.out)
prefixes = {"A": os.path.abspath(args.a), "B": os.path.abspath(args.b)}
os.environ["SRP_PREFIX"], os.environ["SRP_TESTS_WORK"] = prefixes["A"], os.path.join(out, "work")
import srptest as S  # noqa: E402  (after SRP_PREFIX)

sources = S.random_inputs(S.WORK)
for tag, prefix in prefixes.items():
    print(f"== {tag}: {prefix}", flush=True)
    folder = os.path.join(out, tag)
    shutil.rmtree(folder, ignore_errors=True)
    env = S.prefix_env(prefix, os.path.join(folder, "config"))
    env["SCIROOPLOT_DATA_DIR"] = os.path.join(prefix, "share", "scirooplot")
    steps = []
    for seed in args.seeds:
        project = f"random{seed}"
        os.makedirs(os.path.join(folder, project))
        program = os.path.join(folder, project, "DefinePlots.py")
        with open(program, "w") as f:
            f.write(S.random_definition(seed, args.plots, sources, project)[0])
        steps.append((project, f'cd "{folder}/{project}" && python3 -W ignore DefinePlots.py >/dev/null && srp add {project} "{program}" "{folder}/{project}/out" >/dev/null && srp select {project} >/dev/null && plot ".+" ".+" {args.mode}'))
    steps.append(("example", f'cd "{folder}" && srp example-py example "{folder}/example" >/dev/null && cd example && srp select example >/dev/null && plot ".+" ".+" {args.mode}'))
    for label, step in steps:
        done = subprocess.run(["bash", "-c", f'source "{prefix}/share/scirooplot/env.sh" && {step}'], env=env, capture_output=True, text=True, check=False)
        made = done.stdout.count("Created plot")
        print(f"   {label}: {made} plots, exit status {done.returncode}", flush=True)

same, differ, alone = 0, [], []
pictures = {tag: {} for tag in prefixes}
for tag in prefixes:
    for root, _, files in os.walk(os.path.join(out, tag)):
        for name in files:
            if name.endswith(f".{args.mode}"):
                pictures[tag][os.path.relpath(os.path.join(root, name), os.path.join(out, tag))] = os.path.join(root, name)
for rel in sorted(set(pictures["A"]) | set(pictures["B"])):
    if rel not in pictures["A"] or rel not in pictures["B"]:
        alone.append(f"{rel}: only {'A' if rel in pictures['A'] else 'B'}")
        continue
    with open(pictures["A"][rel], "rb") as fa, open(pictures["B"][rel], "rb") as fb:
        if fa.read() == fb.read():  # equal files need no decoding (each decoded picture keeps its memory: a long run ran out of it)
            same += 1
            continue
    if args.mode in S.BITMAP_MODES:
        a, b = S.bitmap(pictures["A"][rel]), S.bitmap(pictures["B"][rel])
        changed = a is None or b is None or a.shape != b.shape or (a != b).any()
        detail = "unreadable" if a is None or b is None else f"size {a.shape} / {b.shape}" if a.shape != b.shape else f"{int((a != b).sum())} pixels"
    else:
        with open(pictures["A"][rel], "rb") as fa, open(pictures["B"][rel], "rb") as fb:
            changed, detail = fa.read() != fb.read(), "bytes differ"
    if not changed:
        same += 1
        continue
    differ.append(f"{rel}: {detail}")
    if shutil.which("montage") and args.mode in ("png", "jpg", "gif", "svg", "pdf"):
        target = os.path.join(out, "differences", rel.replace("/", "_") + ".png")
        os.makedirs(os.path.dirname(target), exist_ok=True)
        subprocess.run(["montage", pictures["A"][rel], pictures["B"][rel], "-tile", "2x1", "-geometry", "+4+4", target], check=False)
for line in differ + alone:
    print(line)
print(f"identical {same}, different {len(differ)}, only in one {len(alone)}" + (f"; side by side in {out}/differences" if differ and shutil.which("montage") else ""))
sys.exit(1 if differ or alone else 0)
