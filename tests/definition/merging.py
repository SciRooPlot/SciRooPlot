"""Plots, pads and data put together from parts hold every part as given.

The cases are those of round_trip.py (every overload on every object it can be called on, each variant of its values). References:
- Plot += Plot (how a plot is put on top of its base plot): the plot with the calls of both made one after the other on one plot, the
  added plot's last (what both set: the added plot's value; data, boxes and shapes of the added pad after those of the pad). Calls that
  clear a value are left out: the added plot holds no value there, so the first one's stays;
- references to data shift by the data the pad had before (named cases: legend rows placed at or showing other data, the data of the
  implicit legend, placeholders of texts naming data of a pad);
- Plot(other, name, group): the other plot, apart from its name and group;
- Data.SetLayout and ApplyLayout: the layout's properties of the appearance (LAYOUT: drawing option, styles, ranges, legend row, text
  of values, contour levels, samples of functions) as if called on the data, the others (also the statistics box) not taken; SetLayout replaces the appearance
  the data had, ApplyLayout puts the layout's on top of it (a legend row merged like the styles). Every setter of Data is in LAYOUT or
  NOT_LAYOUT, so that a new one must be put into one of them.
Covers: legend rows placed at or showing other data, the data of the implicit legend, placeholders naming data of a pad, a pad added to
another pad of the plot.
"""

import os

import srptest as S

LAYOUT = {
    "SetDrawOption", "SetKeepInputStyle", "SetTextFormat", "SetTextAngle", "SetLegendEntry", "GetLegendEntry", "SetRangeX", "SetRangeY", "SetMinRangeX", "SetMaxRangeX",
    "SetMinRangeY", "SetMaxRangeY", "SetMarker", "SetMarkerColor", "SetMarkerAlpha", "SetMarkerStyle", "SetMarkerSize", "SetLine", "SetLineColor", "SetLineAlpha",
    "SetLineStyle", "SetLineWidth", "SetFill", "SetFillColor", "SetFillAlpha", "SetFillStyle", "SetColor", "SetAlpha", "SetContours", "SetNpx",
}  # fmt: skip
NOT_LAYOUT = {
    "SetDataSource", "SetLegendLabel", "SetLegend", "SetDefinesFrame", "SetShowOverflowBins", "SetShowStats", "SetStatsBox", "SetLayout", "ApplyLayout",
    "Normalize", "NormalizeToMaximum", "Scale", "ScaleX", "ScaleY", "ScaleZ", "DivideBinWidth", "Density", "RebinX", "RebinY", "RebinZ", "RebinXY", "RebinXYZ",
    "Smooth", "Cumulative", *S.SELECTIONS, *S.TREE_SELECTIONS,
}  # fmt: skip
REFERENCES = ("dataID", "dataIDs", "refDataID", "legendID", "padID", "pads")  # numbers of data, legends and pads: shifted by +=
SKIPPED = ("PanelLayout", "Panel", "BasePlotShape")  # their values become the pads of a base plot (layouts/geometry.py)

cases, problems = S.setter_cases()
S.check("every overload of the interface has a case", not problems, "; ".join(problems))
methods = {m for c, m, _ in S.python_interface() if c == "Data"} - {"__init__", "__call__", "Print", "GetDataSource", "AsRatio"}
S.check("every setter of Data is a property of the appearance or not", not methods - LAYOUT - NOT_LAYOUT and not LAYOUT & NOT_LAYOUT, f"not sorted: {sorted(methods - LAYOUT - NOT_LAYOUT)}, both: {sorted(LAYOUT & NOT_LAYOUT)}")


def statements(case):
    """the host statement split into (lines before the call, the line with {call}, lines after)"""
    lines = S.SETTER_HOSTS[case["cls"]][case["host"]][0].split("\n")
    at = next(i for i, line in enumerate(lines) if "{call}" in line)
    return lines[:at], lines[at], lines[at + 1 :]


def plain_call(case):
    return S.call(case["method"], [S.setter_value(case["cls"], case["method"], param, "plain") for param in case["params"]], "py")


def label(case):
    return f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']}" + (f" {case['variant']}" if case["variant"] != "plain" else "")


pm = S.S.PlotManager("merging")
namespace = dict(vars(S.S), pm=pm)
checks = []  # (label, name of the plot made, name of its reference, what may differ)


def define(name, code):
    """runs the statements of a plot named name; returns what it raised"""
    result, _ = S.captured(lambda: exec(code.replace("{name}", name), namespace))  # noqa: S102  the cases
    return result if isinstance(result, Exception) else None


# Plot += Plot: the first plot with the plain call, the added one with the call in its variant
for i, case in enumerate(cases):
    if not case["meant"] or case["unset"] or case["host"] in ("statement", "base", "base_pad") or case["cls"] in SKIPPED or set(case["params"]) & set(REFERENCES):
        continue
    if case["cls"] == "Plot" and case["method"] in ("SetName", "SetGroup", "AppendGroup"):
        continue  # the name and group are those of the added plot
    before, line, after = statements(case)
    first, second = plain_call(case), S.call(case["method"], case["values"], "py")
    added = line.replace("p", "q", 1) if line.startswith("p") else line.replace("(p", "(q")
    merged = "\n".join([*before, line.replace("{call}", first), before[0].replace("p =", "q =", 1), *before[1:], added.replace("{call}", second), "p += q", *after])
    together = "\n".join([*before, line.replace("{call}", first), line.replace("{call}", second), *after])
    a, b = define(f"m{i}", merged), define(f"t{i}", together)
    if a or b:
        S.check(f"{label(case)}: += as both calls on one plot", type(a) is type(b) and str(a) == str(b), f"+= {a!r}, one plot {b!r}")
        continue
    checks.append((f"{label(case)}: += as both calls on one plot", f"m{i}", f"t{i}", ()))
    # a copy is the plot copied, apart from its name and group
    if define(f"c{i}", "\n".join([*before, line.replace("{call}", second), *after]).replace("pm.AddPlot(p)", 'pm.AddPlot(Plot(p, "{name}_copy", "copies"))')) is None:
        define(f"o{i}", "\n".join([*before, line.replace("{call}", second), *after]))
        checks.append((f"{label(case)}: Plot(other, name, group) as the other", f"c{i}_copy", f"o{i}", ("name", "group")))

# Data.SetLayout and ApplyLayout: the call made on a layout, which is given to data with and without the plain call
for i, case in enumerate(cases):
    if case["cls"] not in ("Data", "LegendEntry") or case["host"] not in ("data", "data_row") or case["unset"] or case["method"] in ("SetLayout", "ApplyLayout"):
        continue
    call = S.call(case["method"], case["values"], "py")
    access = ".GetLegendEntry()" if case["cls"] == "LegendEntry" else ""
    inLayout = case["cls"] == "LegendEntry" or case["method"] in LAYOUT
    layout = f"layout = Data()\nlayout{access}{call}\n"
    for setter in ("SetLayout", "ApplyLayout"):
        for own in ("", plain_call(case)):
            name = f"{setter[:1].lower()}{'o' if own else 'f'}{i}"
            data = 'p[1].AddData("h", "s")'
            given = f'{layout}p = Plot("{{name}}", "g", "1d")\n{data}{access}{own}\np[1].GetData(1).{setter}(layout)\npm.AddPlot(p)'
            if not inLayout:
                expected = own  # not taken: the data keeps what it had
            elif setter == "SetLayout" or case["method"] == "AddSymbolOf":
                expected = call  # the appearance is the layout's (the data shown in a legend row: the layout's list replaces the data's)
            else:
                expected = own + call  # put on top of what the data had
            reference = f'p = Plot("{{name}}", "g", "1d")\n{data}{access}{expected}\npm.AddPlot(p)'
            if define(name, given) or define(name + "_ref", reference):
                continue  # a call that cannot be made on a Data without name (selections of trees): not a property of a layout
            what = "taken" if inLayout else "not taken"
            checks.append((f"{label(case)}: {setter} on data {'with' if own else 'without'} the plain call: {what}", name, name + "_ref", ()))

# references to data shift by the data the pad had before: what each pad and box refers to after +=, written by hand
NAMED = [
    (
        "the implicit legend of added data is the added pad's own first legend",
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[1].AddLegend()\nq = Plot("{name}", "g", "1d")\nq[1].AddData("h2", "s2")\nq[1].AddLegend()\np += q\npm.AddPlot(p)',
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[1].AddLegend()\np[1].AddData("h2", "s2").SetLegend(2)\np[1].AddLegend()\npm.AddPlot(p)',
    ),
    (
        "added data without legends there go to the first legend",
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[1].AddLegend()\nq = Plot("{name}", "g", "1d")\nq[1].AddData("h2", "s2")\np += q\npm.AddPlot(p)',
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[1].AddLegend()\np[1].AddData("h2", "s2")\npm.AddPlot(p)',
    ),
    (
        "legend rows placed at, showing or referring to data of the added pad",
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\nq = Plot("{name}", "g", "1d")\nq[1].AddData("h2", "s2")\nq[1].AddData("h", "s").GetLegendEntry().BeforeData(1).AddSymbolOf(1)\nq[1].AddLegend().AddEntry("row").SetRefData(2)\nq[1].GetLegend(1).AddDataEntry(1, 1)\np += q\npm.AddPlot(p)',
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[1].AddData("h2", "s2")\np[1].GetData(2).SetLegend(1)\np[1].AddData("h", "s").SetLegend(1).GetLegendEntry().BeforeData(2).AddSymbolOf(2)\np[1].AddLegend().AddEntry("row").SetRefData(3)\np[1].GetLegend(1).AddDataEntry(1, 2)\npm.AddPlot(p)',
    ),
    (
        "placeholders of texts naming data of their own pad and of other pads",
        'p = Plot("{name}", "g", "1d_ratio")\np[1].AddData("h", "s")\np[2].AddData("h", "s")\nq = Plot("{name}", "g", "1d_ratio")\nq[1].AddData("h2", "s2")\nq[2].AddData("h2", "s2")\nq[1].AddText(0.3, 0.5, "<mean:1> <mean:1@2>")\np += q\npm.AddPlot(p)',
        'p = Plot("{name}", "g", "1d_ratio")\np[1].AddData("h", "s")\np[2].AddData("h", "s")\np[1].AddData("h2", "s2")\np[2].AddData("h2", "s2")\np[1].AddText(0.3, 0.5, "<mean:2> <mean:2@2>")\npm.AddPlot(p)',
    ),
    (
        "a pad added to another pad of the plot (p[2] += p[1]): its data after those of the pad, its texts naming them",
        'p = Plot("{name}", "g", "1d_ratio")\np[1].AddData("h", "s")\np[1].AddText(0.3, 0.5, "<mean:1>")\np[2].AddData("h2", "s2")\np[2] += p[1]\npm.AddPlot(p)',
        'p = Plot("{name}", "g", "1d_ratio")\np[1].AddData("h", "s")\np[1].AddText(0.3, 0.5, "<mean:1>")\np[2].AddData("h2", "s2")\np[2].AddData("h", "s")\np[2].AddText(0.3, 0.5, "<mean:2>")\npm.AddPlot(p)',
    ),
    (
        "an own gradient takes neither the alpha nor the number of colours of the gradient it replaces",
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[0].SetDefaultColors([(633, 0.), (600, 1.)], None, 7).SetDefaultAlpha(0.5)\nq = Plot("{name}", "g", "1d")\nq[0].SetDefaultColors([(416, 0.), (1, 1.)])\np += q\npm.AddPlot(p)',
        'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\np[0].SetDefaultColors([(416, 0.), (1, 1.)])\npm.AddPlot(p)',
    ),
]
for k, (what, merged, reference) in enumerate(NAMED):
    a, b = define(f"n{k}", merged), define(f"n{k}_ref", reference)
    S.check(f"{what}: defined", not a and not b, f"{a!r} {b!r}")
    checks.append((what, f"n{k}", f"n{k}_ref", ()))

saved = os.path.join(S.WORK, "merging.info")
pm.SavePlots(file=saved)
plots = {key.split(":")[0]: values for key, values in S.info_plots(saved).items()}
for what, made, ref, ignored in checks:
    made, ref = (next((n for n in (name, name + "_renamed") if n in plots), name) for name in (made, ref))
    a = {k: v for k, v in plots.get(made, {}).items() if k != "name" and not k.startswith(ignored or ("\0",))}
    b = {k: v for k, v in plots.get(ref, {}).items() if k != "name" and not k.startswith(ignored or ("\0",))}
    diff = {k: (a.get(k), b.get(k)) for k in set(a) | set(b) if a.get(k) != b.get(k)}
    S.check(what, made in plots and ref in plots and not diff, str(diff)[:400] if made in plots and ref in plots else "not saved")
S.finish()
