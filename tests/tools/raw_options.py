"""Raw ROOT drawing options that no alias stands for, as SciRooPlot draws them next to plain ROOT: about 300 options of every kind of
data (alone, drawn over a first item, on a log y axis), contact sheets of pairs (SciRooPlot left, plain ROOT right) to look at.
What reaches ROOT is tested in drawing/raw_options.py; this shows how it looks. Plain ROOT draws graphs with A added; its lines and
markers are black, without fill (SciRooPlot's default colours differ). Prints the plots not made and the warnings per case.
Needs ImageMagick (montage) for the sheets.

usage: SRP_PREFIX=<build or install> SRP_TESTS_WORK=<folder> PYTHONPATH=tests python3 tests/tools/raw_options.py   (about 5 minutes)
"""

import os
import re
import shutil
import subprocess

import srptest as S
from srptest import ROOT

KINDS = {"h1": "hist", "h1b": "hist_var", "p1": "profile", "h2": "hist2d", "p2": "profile2d", "hp": "hist2poly", "h3": "hist3d", "g": "graph_asymm", "ge": "graph_errors", "gp": "graph_polar", "g2": "graph2d", "g2e": "graph2d_errors", "f1": "tf1", "f2": "tf2", "f3": "tf3"}
OPTIONS = {
    "h1": [
        "E0",
        "E1",
        "E3",
        "E4",
        "E X0",
        "E1 X0",
        "E0 X0",
        "B",
        "BAR",
        "BAR1",
        "BAR2",
        "BAR3",
        "BAR4",
        "HBAR1",
        "HBAR4",
        "C",
        "L",
        "P",
        "P0",
        "*H",
        "LF2",
        "HIST F",
        "9",
        "][",
        "HIST MIN0",
        "TEXT45",
        "TEXTE",
        "TEXT E",
        "PIE",
        "LEGO",
        "LEGO1",
        "SURF",
        "E1 X+",
        "HIST Y+",
        "E1 X+ Y+",
        "AXIS",
        "AXIG",
        "FUNC",
        "HIST PLC PMC",
        "HIST PFC",
        "SAME",
        "",
        "hist e",
        "e1",
        "HIST SAME",
        "E2 L",
        "E3 L",
        "E5 P",
        "HIST C L",
        "HIST TEXT0",
    ],
    "p1": ["E", "E1", "E2", "E3", "HIST", "P", "C"],
    "h2": [
        "SCAT",
        "ARR",
        "ARR COLZ",
        "BOX1",
        "COL0",
        "COLZ0",
        "COLZ1",
        "COLZ2",
        "COL2",
        "COLZ CJUST",
        "CONT2",
        "CONT LIST",
        "LEGO4",
        "LEGO BB",
        "LEGO FB",
        "LEGO1 CYL",
        "LEGO1 SPH",
        "LEGO1 PSR",
        "SURF5",
        "SURF7",
        "SURF CYL",
        "SURF1 SPH",
        "SURF1 PSR",
        "SURF3Z",
        "SURF FB BB",
        "TEXT45",
        "TEXTE",
        "COLZ TEXT",
        "TEXT COLZ",
        "E",
        "HIST",
        "AITOFF",
        "MERCATOR",
        "SINUSOIDAL",
        "PARABOLIC",
        "MOLLWEIDE",
        "CANDLEY",
        "VIOLINY",
        "CANDLEY2",
        "CANDLEX(10000311)",
        "VIOLINX(11110111)",
        "Z",
        "COLZ X+ Y+",
        "COL A",
        "CONT1 POL",
        "CONT CYL",
        "LEGO2Z",
        "BOX1 COLZ",
        "colz",
        "",
        "COLZ SAME0",
        "LEGO0",
        "SURF2Z0",
        "CONT4Z POL",
        "COL1",
    ],
    "p2": ["COLZ", "TEXT", "LEGO", "SURF1", "BOX", "E"],
    "hp": ["", "COLZ", "TEXT", "COLZ L", "L", "F", "LEGO", "COLZ0", "COLZ TEXT"],
    "h3": ["", "BOX", "BOX1", "BOX2", "BOX2Z", "BOX3", "ISO", "LEGO", "SCAT", "TEXT", "COLZ", "FB BB BOX"],
    "g": ["*", "PL", "LP", "C", "CP", "F", "LF", "B", "B1", "2", "3", "4", "5", "0", "1", ">", "||", "X", "Z", "Y+", "X+", "RX", "RY", "RX RY", "I", "P PMC PLC", "E", "pl", "AP", "A", "P0", "[]0", "3 L", "4 P", "2 X"],
    "ge": ["P", "PZ", "2", "3", "4", "5", "LP"],
    "gp": ["P", "L", "E", "PE", "N", "PN", "RP", "DP", "GP", "F", "C", "O"],
    "g2": ["TRI1", "P0", "PCOL", "P0 TRI", "CONT5", "LINE", "TRIW P", "SURF1 CYL", "P CYL", "P SPH", "P POL", "TRI1 FB BB", "BOX", "ARR", "TEXT", "LEGO", "COLZ", "TRI", "PFB", "TRI2 0", "ERR"],
    "g2e": ["ERR", "P ERR", "TRI1 ERR", "P0"],
    "f1": ["", "HIST", "E", "P", "*", "LF2", "FC", "C", "L", "B", "BAR", "LP", "HIST L"],
    "f2": ["SURF", "CONT", "COLZ", "LEGO", "SURF1 POL", "BOX", "TEXT", "ARR", "CONT LIST", "SURF FB", "", "CONT1Z", "SURF3"],
    "f3": ["", "ISO", "FB", "BOX"],
}
OVER = {  # (data, first data, option of the first): options of the data drawn over it
    ("h1b", "h1", "X0 E P"): ["E1", "E3", "E4", "BAR2", "C", "L", "LF2", "HIST F", "*H", "TEXT45", "E X0", "HIST PLC", "B"],
    ("h2", "h2", "COLZ"): ["BOX", "CONT3", "TEXT", "ARR", "SCAT", "CONT LIST", "BOX1"],
    ("ge", "g", "P Z"): ["C", "PL", "3", "4", "5", "B", "F", ">", "RX"],
    ("f1", "h1", "X0 E P"): ["", "C", "L", "HIST", "LF2"],
}
LOG_Y = {"h1": ["E1", "B", "BAR2", "HIST F", "C", "LF2", "E3", "*H"], "g": ["3", "B", "F", "C", "RY"]}
cases = [(o, opt, None, None, False) for o, opts in OPTIONS.items() for opt in opts]
cases += [(o, opt, first, firstOpt, False) for (o, first, firstOpt), opts in OVER.items() for opt in opts]
cases += [(o, opt, None, None, True) for o, opts in LOG_Y.items() for opt in opts]

p = S.Project("rawoptions")
p.inputs()
objects = S.data_objects()
for k, (o, opt, first, firstOpt, logY) in enumerate(cases):
    plot = S.S.Plot(f"c{k:03d}", "raw", "1d" if S.DATA_KINDS[KINDS[o]][0] == 1 else "2d")
    if first:
        S.add_data(plot[1], KINDS[first]).SetDrawOption(firstOpt)
    S.add_data(plot[1], KINDS[o]).SetDrawOption(opt)
    if logY:
        plot[1]["Y"].SetLog()
    p.pm.AddPlot(plot)
run = p.plot("raw", mode="png")
logs = {}
pending = []
for line in run.log.splitlines():
    if made := re.search(r"Created plot (\S+) from group", line):
        logs[made.group(1)] = pending
        pending = []
    elif re.search(r"WARN|ERR|Warning in|Error in", line):
        pending.append(line.strip())
out = os.path.join(S.WORK, "pictures")
os.makedirs(out)
for name in os.listdir(p.path("out", "raw")) if os.path.isdir(p.path("out", "raw")) else []:
    shutil.copy(p.path("out", "raw", name), out)
for k in range(len(cases)):  # after a crash: the plots not made, each in a run of its own
    if not os.path.exists(os.path.join(out, f"c{k:03d}.png")):
        single = p.plot("raw", f"^c{k:03d}$", mode="png")
        logs[f"c{k:03d}"] = [line for line in single.log.splitlines() if re.search(r"WARN|ERR|Warning in|Error in|Break", line)]
        if os.path.exists(p.path("out", "raw", f"c{k:03d}.png")):
            shutil.copy(p.path("out", "raw", f"c{k:03d}.png"), out)


def reference(k, o, opt, first, firstOpt, logY):
    """plain ROOT: the objects drawn alone on a canvas of the size of the plot, black lines and markers, no fill"""
    canvas = ROOT.TCanvas(f"ref{k}", "", 710, 500)
    canvas.SetLogy(logY)
    ROOT.gStyle.SetOptStat(0)
    ROOT.gStyle.SetOptTitle(0)
    drawn = []
    for which, option, red in ((first, firstOpt, False), (o, opt, first is not None)):
        if which is None:
            continue
        obj = objects[KINDS[which]].Clone(f"ref{k}_{which}")
        ROOT.SetOwnership(obj, False)
        colour = ROOT.kRed if red else ROOT.kBlack
        for attribute, setter in (("TAttLine", "SetLineColor"), ("TAttMarker", "SetMarkerColor")):
            if obj.InheritsFrom(attribute):
                getattr(obj, setter)(colour)
        if obj.InheritsFrom("TAttFill"):
            obj.SetFillStyle(0)
        if not drawn and obj.InheritsFrom("TGraph") and not obj.InheritsFrom("TGraphPolar") and "A" not in option.upper():
            option = "A " + option
        obj.Draw(option if not drawn else "SAME " + option)
        drawn.append(obj)
    canvas.Print(os.path.join(S.WORK, "reference", f"c{k:03d}.png"))
    canvas.Close()


os.makedirs(os.path.join(S.WORK, "reference"))
for k, case in enumerate(cases):
    reference(k, *case)
for k, (o, opt, first, _, logY) in enumerate(cases):
    name = f"c{k:03d}"
    label = f"{name} {KINDS[o]} '{opt}'" + (f" over {KINDS[first]}" if first else "") + (" log y" if logY else "")
    if not os.path.exists(os.path.join(out, f"{name}.png")):
        print(f"NOT MADE {label}")
    if logs.get(name):
        print(f"== {label}: {' | '.join(sorted(set(logs[name]))[:3])}")
if shutil.which("montage"):
    os.makedirs(os.path.join(S.WORK, "pairs"))
    pairs = []
    for k, (o, opt, first, _, logY) in enumerate(cases):
        name = f"c{k:03d}"
        label = f"{name} {KINDS[o]} '{opt}'" + (f" over {KINDS[first]}" if first else "") + (" log y" if logY else "")
        ours = os.path.join(out, f"{name}.png")
        pair = os.path.join(S.WORK, "pairs", f"{name}.png")
        subprocess.run(["montage", ours if os.path.exists(ours) else "xc:pink", os.path.join(S.WORK, "reference", f"{name}.png"), "-tile", "2x1", "-geometry", "360x260+2+2", "-title", label, "-pointsize", "18", pair], check=False)
        pairs.append(pair)
    for n in range(0, len(pairs), 15):
        subprocess.run(["montage", *pairs[n : n + 15], "-tile", "3x", "-geometry", "+4+4", os.path.join(S.WORK, f"sheet{n // 15:02d}.png")], check=False)
    print(f"sheets: {S.WORK}/sheet*.png (SciRooPlot left, plain ROOT right)")
