"""A saved plot definition that was changed by hand is read as written or reported, never misread silently.

The file holds the plots of round_trip.py (every key the definition writes). For every key (one place each), the file is changed there:
the value replaced by a word that is no number (junk) or followed by a letter (trailing), the key misspelt (a letter added), and the key
of a part (a pad, a data item, an axis, FRAME, legend_entry ...) changed (its number or letter replaced, or a suffix added). The changed
plot is loaded together with an unchanged one. Reference: what was written:
- the value is read as written (a text), or loading reports it naming the key or the value (the plot is then left out), never
  silently read as something else or dropped;
- a misspelt key, and a part under a changed key, are read as before (the key names its kind) or reported naming the key;
- the unchanged plot loads in every case, and the file as written names no key as unknown (also where repeated parts, the rows of a
  legend, hold different keys).
A file loaded group by group into one manager gives no messages (its base plots are read again with every call and replaced silently).
(Text after a value on the same line is dropped by the reader of the format, before SciRooPlot sees it: not checked.)
"""

import os
import re

import srptest as S

cases, _ = S.setter_cases()
pm = S.S.PlotManager("written")
namespace = dict(vars(S.S), pm=pm)
for i, case in enumerate(cases):
    S.captured(lambda case=case, i=i: exec(case["py"].replace("{name}", f"c{i}"), namespace))  # noqa: S102  the definitions of the cases
intact = S.S.Plot("intact", "g", "1d")
intact[1].AddData("h", "s").SetColor(633)
pm.AddPlot(intact)
rows = S.S.Plot("legend_rows", "g", "1d")  # repeated parts (legend rows) holding different keys
legend = rows[1].AddLegend()
legend.AddEntry("plain")
legend.AddEntry("filled").SetFillColor(633)
legend.AddEntry("marked").SetMarkerStyle(20).SetLineWidth(3.0)
pm.AddPlot(rows)
path = os.path.join(S.WORK, "written.info")
pm.SavePlots(file=path)
with open(path) as f:
    lines = f.read().splitlines()

# the plots as blocks of lines (a line "name:group", then its braces)
blocks, start, depth = {}, None, 0
for n, line in enumerate(lines):
    if depth == 0 and line and not line.startswith((" ", "{", "}")):
        start = n
    depth += line.strip() == "{"
    depth -= line.strip() == "}"
    if depth == 0 and line.strip() == "}" and start is not None:
        blocks[lines[start]] = (start, n)
        start = None
intactBlock = lines[blocks["intact:g"][0] : blocks["intact:g"][1] + 1]
original = {k.split(":")[0]: v for k, v in S.info_plots(path).items()}

# one place for every key (keys with a number such as PAD_1 or DATA_2 by their kind); a key that holds a part somewhere is a part
nodeKinds = {re.sub(r"_\d+$", "_<n>", lines[n].strip().partition(" ")[0]) for n in range(len(lines) - 1) if lines[n + 1].strip() == "{"}
places = {}
for name, (first, last) in blocks.items():
    for n in range(first + 2, last):
        text = lines[n].strip()
        if text in ("{", "}"):
            continue
        key, _, value = text.partition(" ")
        kind = re.sub(r"_\d+$", "_<n>", key)
        isNode = kind in nodeKinds
        places.setdefault((kind, isNode), (name, n, key, value))


def load(changed):
    """loads the changed plot and the intact one; returns (the plots read, by name, as values), what loading printed"""
    file = os.path.join(S.WORK, "changed.info")
    with open(file, "w") as f:
        f.write("\n".join(changed + intactBlock) + "\n")
    loaded = S.S.PlotManager("loaded")
    _, printed = S.captured(lambda: loaded.LoadPlots(file=file))
    again = os.path.join(S.WORK, "again.info")
    if os.path.exists(again):
        os.remove(again)
    loaded.SavePlots(file=again)
    plots = {k.split(":")[0]: v for k, v in S.info_plots(again).items()} if os.path.exists(again) else {}
    return plots, "\n".join(line for line in printed.splitlines() if "[ INFO ]" not in line)


for (kind, isNode), (name, n, key, value) in sorted(places.items()):
    first, last = blocks[name]
    plotName = name.split(":")[0]
    indent = lines[n][: len(lines[n]) - len(lines[n].lstrip())]
    if isNode:
        changes = {"renamed": f"{indent}{re.sub(r'_(\d+|[A-Z])$', '_x', key) if re.search(r'_(\d+|[A-Z])$', key) else key + '_x'}"}
    else:
        # a quoted value gets its letter inside the quotes (after the closing quote the reader of the format stops)
        trailing = f'{value[:-1]}x"' if value.startswith('"') and value.endswith('"') else f"{value}x"
        changes = {"junk": f"{indent}{key} @junk@", "trailing": f"{indent}{key} {trailing}", "misspelt": f"{indent}{key}x {value}"}
    for change, line in changes.items():
        changed = lines[first:n] + [line] + lines[n + 1 : last + 1]
        plots, printed = load(changed)
        written = line.strip().partition(" ")[2] if not isNode else None
        written = written[1:-1] if written and written.startswith('"') else written
        newKey = line.strip().partition(" ")[0]
        reported = bool(printed) and (newKey in printed or (written and written in printed) or key in printed)
        if isNode or change == "misspelt":
            # a part under a key that is not one, or a misspelt key: read as before (the key is read by its kind), or reported
            ok = reported or plots.get(plotName) == original[plotName]
        else:
            # read as written: a loaded plot holds the changed value at this key (the name of a plot: under that name)
            readAsWritten = any(k.split("/")[-1] == key and v == written for name, plot in plots.items() if name != "intact" for k, v in plot.items())
            ok = readAsWritten or reported
        S.check(f"{key} ({kind}) {change}: read as written or reported naming it", ok, f"{line.strip()!r}: {printed.strip()[:300]!r}")
        S.check(f"{key} ({kind}) {change}: the unchanged plot loads", "intact" in plots, printed.strip()[:300])
unchanged = S.S.PlotManager("unchanged")
_, printed = S.captured(lambda: unchanged.LoadPlots(file=path))
unknown = [line for line in printed.splitlines() if "does not know" in line]
S.check("the file as written loads without naming a key as unknown", not unknown, " | ".join(unknown)[:300])

# a file loaded group by group into one manager: its base plots are read with every call and replaced silently
grouped = S.S.PlotManager("grouped")
grouped.AddBasePlot(S.S.PlotManager.MakeBasePlot("1d"))
for group in ("a", "b"):
    plot = S.S.Plot("p", group, "1d")
    plot[1].AddData("h", "s")
    grouped.AddPlot(plot)
groupedFile = os.path.join(S.WORK, "grouped.info")
grouped.SavePlots(file=groupedFile)
loaded = S.S.PlotManager("loaded_by_group")
_, printed = S.captured(lambda: [loaded.LoadPlots(group=group, file=groupedFile) for group in ("a", "b")])
said = [line for line in printed.splitlines() if "[ WARN" in line or "[ ERR" in line]
S.check("a file loaded group by group: no messages (its base plots replaced silently)", not said, " | ".join(said)[:300])
S.finish()
