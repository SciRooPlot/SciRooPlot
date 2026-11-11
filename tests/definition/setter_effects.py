"""Every call of the plot definition changes what is drawn, or says why it cannot: a setter that is stored but silently never reaches the
painting is a bug.

The cases are every overload of the Python interface on every object it is meant for (srptest.setter_cases: a plot and a base plot, pad 1
and pad 0, each axis, data and each side of a ratio, legend rows of data and added ones, boxes, shapes, layouts), with its plain test value
(or, where that is what the shipped base plots set anyway, another one: EFFECT_VALUES), in a plot that draws something there: a histogram
with a legend row, a second one drawn as a band, a legend, a text and a line in pad 1 (a 2d histogram for the z axis, a 3d histogram for
the C axis, both with a colour scale); a base plot or layout is used by a plot of its own. The reference is the same plot without the
call: the drawn plot must differ in what is drawn (srptest.fingerprint: the primitives with their values and attributes, the axes, the
look of the canvas, pads, boxes and shapes, the pad ranges and palettes; where those are the same, the png), or the plot run must give a
message the plot without the call does not give.
Not here: the selections of trees and histograms (they make the data: inputs/requests.py, data/projection_routes.py) and calls that clear a
value (definition/round_trip.py: the saved plot equals the plot without any call).
Calls without an effect in such a plot are listed in NO_EFFECT with the reason (narrowed guarantee).
Covers: a setter stored but ignored when the plot is painted, for every setter of the interface.
"""

import ROOT

import srptest as S

p = S.Project("setter_effects", bases=("1d", "2d"))
objects = S.data_objects("normal")
denominator = objects["hist_var"].Clone("h2")
for b in range(1, denominator.GetNbinsX() + 1):
    denominator.SetBinContent(b, 1.0 + 0.05 * b * b)
    denominator.SetBinError(b, 0.1)
for source in ("s", "s2"):
    p.source(source, {"h": objects["hist"].Clone("h"), "h2": denominator, "hist2d": objects["hist2d"], "hist3d": objects["hist3d"]})
    S.write_tree(p.path(f"{source}.root"), {"x": S.np.linspace(0.5, 9.5, 50)}, name="t")
    S.write_tree(p.path(f"{source}.root"), {"x": S.np.linspace(1.5, 8.5, 50)}, name="t2")
level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
picture = ROOT.TCanvas("picture", "", 120, 60)
picture.SetFillColor(ROOT.kOrange)
picture.Print(f"{S.WORK}/logo.png")
ROOT.gErrorIgnoreLevel = level

# values other than the plain test values, where those are what the plots have anyway (method, parameter): value
EFFECT_VALUES = {
    ("SetOppositeTicks", "isOppositeTicks"): False,  # the base plots have ticks on both sides
    ("SetHideClippedLabels", "hide"): False,  # on in the base plots
    ("SetMaxDigits", "maxDigits"): 1,  # 3 in the base plots
    ("SetDefaultBoxCoordinates", "coordinates"): S.Code("pad_coordinates"),  # frame coordinates are the default of boxes
    ("SetDefaultShapeCoordinates", "coordinates"): S.Code("pad_coordinates"),  # user coordinates are the default of shapes
    ("SetAutoPlacement", "placement"): S.Code("bottom_right"),  # where the legend goes anyway
    ("ImageBox", "SetAutoPlacement", "placement"): S.Code("top_left"),  # where the image and the statistics box go anyway
    ("StatsBox", "SetAutoPlacement", "placement"): S.Code("top_left"),
    ("AddSymbolOf", "dataID"): 2,  # the band of data 2 added to the row of data 3 (points)
    ("BeforeData", "dataID"): 1,
    ("AfterData", "dataID"): 1,
}
# calls that change nothing drawn in these plots: (class, method, host or None for every host): why
DEFAULT = "the value is the default"
MEANINGLESS = "no meaning there (ignored without a message)"
NO_EFFECT = {
    **{(cls, method, None): DEFAULT for cls in ("TextBox", "LegendBox", "StatsBox", "ImageBox") for method in ("SetFrameCoordinates",)},
    **{("Shape", "SetUserCoordinates", None): DEFAULT, ("Pad", "SetDefaultCandleBoxRange", None): "no candle drawn"},
    **{(cls, method, None): "a box placed automatically has no position to read in a coordinate system" for cls in ("LegendBox", "StatsBox") for method in ("SetPadCoordinates", "SetUserCoordinates")},
    ("Plot", "SetName", None): "names the plot (cli/plot_selection.py)",
    ("Plot", "SetGroup", None): "names the plot (cli/plot_selection.py)",
    ("Plot", "AppendGroup", None): "names the plot (cli/plot_selection.py)",
    ("Plot", "SetBasePlot", "base"): "a base plot has no base plot",
    ("Plot", "SetFixAspectRatio", None): "keeps the aspect ratio when a window is resized: no effect on files",
    ("Axis", "SetHideClippedLabels", None): "no label of these plots is clipped (frames/axis_labels.py)",
    ("Data", "SetShowOverflowBins", None): "the histograms have no overflow (flows drawn: inputs/requests.py)",
    ("Ratio", "SetShowOverflowBins", None): "the histograms have no overflow",
    ("Data", "SetDefinesFrame", None): "the data giving the frame anyway is the same histogram",
    **{(cls, method, None): "the shipped look has neither fill nor border" for cls, method in (("Plot", "SetTransparent"), ("Pad", "SetTransparent"), ("LegendBox", "SetTransparent"), ("LegendBox", "SetNoBox"), ("TextBox", "SetTransparent"), ("TextBox", "SetNoBox"), ("StatsBox", "SetTransparent"))},
    ("Ratio", "Scale", "both"): "the same factor on both sides cancels",
    ("Ratio", "ScaleY", "both"): "the same factor on both sides cancels",
    ("PanelLayout", "SetColorScaleOffset", None): "no colour scale in the plot using the layout (layouts/pad_look.py)",
    ("PanelLayout", "SetColorScaleWidth", None): "no colour scale in the plot using the layout (layouts/pad_look.py)",
    ("PanelLayout", "SetCommonTitles", None): "the panels have no titles (layouts/shared_axes.py)",
    **{("Axis", method, host): MEANINGLESS for method in ("SetColorScaleOffset", "SetColorScaleWidth") for host in ("x", "y", "pad0_x")},
    **{("Axis", method, "c"): MEANINGLESS for method in ("SetGrid", "SetRoomAboveMax", "SetRoomBelowMin", "SetTimeFormat", "SetOppositeTicks", "SetLabelAngle")},
    ("Axis", "SetGrid", "z"): MEANINGLESS,
    ("Axis", "SetOppositeTicks", "z"): MEANINGLESS,
    **{("Axis", "SetNoExponent", host): "the labels of that axis need no exponent" for host in ("x", "pad0_x", "c")},
    ("Axis", "SetNumDivisions", "c"): "ROOT reads the divisions of the colour scale of a 3d histogram from its binned z axis at every paint",
    ("Data", "SetContours", "data"): MEANINGLESS,
    ("Ratio", "SetContours", None): MEANINGLESS,
    ("Shape", "SetFill", "line"): MEANINGLESS,
    ("Shape", "SetFillAlpha", "line"): MEANINGLESS,
    ("Shape", "SetFillColor", "line"): MEANINGLESS,
    ("Shape", "SetFillStyle", "line"): MEANINGLESS,
    ("Pad", "SetView", None): MEANINGLESS,
    ("Pad", "SetPosition", "pad0"): MEANINGLESS,
}

# hosts of their own: the order and spacing of columns need a legend of two columns and three rows
HOSTS = {cls: dict(hosts) for cls, hosts in S.SETTER_HOSTS.items()}
HOSTS["LegendBox"]["legend_columns"] = ('p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s", "third")\np[1].AddLegend().SetNumColumns(2){call}\npm.AddPlot(p)', None)
OWN_HOST = {("LegendBox", "SetColumnSpacing"): "legend_columns", ("LegendBox", "SetOrderTopDown"): "legend_columns"}

BACKGROUND = {"z": 'p[1].AddData("hist2d", "s", "data").SetDrawOption(colz)', "c": 'p[1].AddData("hist3d", "s", "data").SetDrawOption(colz)\np[1]["C"].SetTitle("contents")'}
NO_LEGEND = ("legend", "legend_columns", "added_row")  # hosts adding the legend whose rows are checked
LAYOUT_PADS = {"layout": 3, "panel": 4, "shape": 3}


def statement(py, host, group):
    """the host statement made drawable: background data, a legend and a text in pad 1; a base plot used by a plot of its own"""
    py = py.replace("/srptest/logo.png", f"{S.WORK}/logo.png").replace('"g"', f'"{group}"').replace('AddData("h", "s")', 'AddData("h", "s", "row")').replace('AddRatio("h", "s", "h2", "s2")', 'AddRatio("h", "s", "h2", "s2", "ratio")')
    py = py.replace("SetStatsBox(StatsBox()", "SetShowStats().SetStatsBox(StatsBox()")
    lines = py.split("\n")
    if lines[0].startswith("p = Plot(") and '"{name}", ' in lines[0] and not lines[-1].startswith("pm.AddBasePlot"):
        lines[0] = lines[0].replace('"1d")', f'"{"2d" if host in BACKGROUND else "1d"}")')
        extra = [BACKGROUND.get(host, 'p[1].AddData("h", "s", "data")\np[1].AddData("h2", "s", "second").SetDrawOption(band)'), 'p[1].AddText(0.3, 0.8, "text")', "p[1].AddLine((2., 2.), (8., 4.))"] + ([] if host in NO_LEGEND else ["p[1].AddLegend()"])
        return "\n".join([lines[0], *extra, *lines[1:]])
    # base plots and layouts: the base plot is {name}_base, the plot {name} draws into its pads
    py = py.replace('"{name}"', '"{name}_base"').replace('"{name}", PanelLayout', '"{name}_base", PanelLayout')
    use = [f'u = Plot("{{name}}", "{group}", "{{name}}_base")'] + [f'u[{i}].AddData("h", "s", "data")' for i in range(1, LAYOUT_PADS.get(host, 1) + 1)]
    return py + "\n" + "\n".join([*use, "u[1].AddLegend()", 'u[1].AddText(0.3, 0.8, "text")', "u[1].AddLine((2., 2.), (8., 4.))", "pm.AddPlot(u)"])


def excluded(case):
    for key in ((case["cls"], case["method"], case["host"]), (case["cls"], case["method"], None)):
        if key in NO_EFFECT:
            return True
    return False


cases, problems = S.setter_cases()
S.check("every overload of the interface has a case", not problems, "; ".join(problems))
SKIPPED = S.SELECTIONS + S.TREE_SELECTIONS
for c in cases:
    c["host"] = OWN_HOST.get((c["cls"], c["method"]), c["host"]) if c["host"] == "legend" else c["host"]
todo = [c for c in cases if c["meant"] and c["variant"] == "plain" and not c["unset"] and c["host"] not in ("statement", "strip") and c["method"] not in SKIPPED and not excluded(c)]
chunks = S.Chunks("effects")
namespace = dict(vars(S.S), pm=p.pm)
baselines = {}
for cls, hosts in HOSTS.items():
    for host, (py, _) in hosts.items():
        if any(c["cls"] == cls and c["host"] == host for c in todo):
            name = f"base_{cls}_{host}"
            code = statement(py.replace("{call}", ""), host, chunks.add(name)).replace("{name}", name)
            result, printed = S.captured(lambda code=code: exec(code, namespace))  # noqa: S102
            S.check(f"plot without a call on {host} ({cls}) defined", not isinstance(result, Exception), f"{result!r} {printed.strip()}"[:300])
            baselines[(cls, host)] = name


def label(case):
    return f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']}"


for i, case in enumerate(todo):
    case["name"] = f"c{i}"
    values = [EFFECT_VALUES.get((case["cls"], case["method"], param), EFFECT_VALUES.get((case["method"], param), value)) for param, value in zip(case["params"], case["values"])]
    case["values"] = values
    py = HOSTS[case["cls"]][case["host"]][0].replace("{call}", S.call(case["method"], values, "py"))
    code = statement(py, case["host"], chunks.add(case["name"])).replace("{name}", case["name"])
    result, printed = S.captured(lambda code=code: exec(code, namespace))  # noqa: S102
    case["defined"] = not isinstance(result, Exception)
    S.check(f"{label(case)}: defined", case["defined"], f"{result!r} {printed.strip()}"[:300])

chunks.plot(p)
S.check("every plot run wrote its file", not chunks.unwritten(), " | ".join(chunks.unwritten()))


def drawn(name):
    group = chunks.groupOf[name]
    return S.fingerprint(chunks.runs[group], name, group)


def messages(name):
    return {line.replace(name, "{name}") for line in chunks.messages(name)}


reference = {key: (drawn(name), messages(name)) for key, name in baselines.items()}
for key, name in baselines.items():
    S.check(f"plot without a call on {key[1]} ({key[0]}) made", reference[key][0] is not None, " | ".join(chunks.errors()[:2]))
same = []  # cases drawn with the same primitives and attributes, silently: compared once more by the pictures (what the primitives miss)
for case in todo:
    if not case["defined"]:
        continue
    with chunks.runs[chunks.groupOf[case["name"]]].held():
        got = drawn(case["name"])
        base, baseMessages = reference[(case["cls"], case["host"])]
        S.check(f"{label(case)}: plot made", got is not None, " | ".join(chunks.errors()[:2]))
        if got is not None and base is not None and S.fingerprint_diff(base, got) == "" and not messages(case["name"]) - baseMessages:
            same.append(case)
pictures = {}
for group in dict.fromkeys(chunks.groupOf[name] for case in same for name in (case["name"], baselines[(case["cls"], case["host"])])):
    names = sorted({name for case in same for name in (case["name"], baselines[(case["cls"], case["host"])]) if chunks.groupOf[name] == group})
    p.plot(f"^{group}$", f"^({'|'.join(names)})$", "png")
    for name in names:
        pictures[name] = S.bitmap(p.path("out", *group.split("/"), f"{name}.png"))
for case in todo:
    if case["defined"]:
        new, old = pictures.get(case["name"]), pictures.get(baselines[(case["cls"], case["host"])])
        unchanged = case in same and new is not None and old is not None and new.shape == old.shape and (new == old).all()
        S.check(f"{label(case)}: changes what is drawn or says why not", not unchanged, f"call {case['values']} drew the same as without it, silently")
S.finish()
