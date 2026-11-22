"""An invalid argument of the plot definition is reported once, and nothing is stored silently.

The cases are every overload of the Python interface (as round_trip.py: every object it can be called on) with each invalid value of
one of its parameters (srptest.SETTER_INVALID: not a number, values outside of what the parameter means, empty expressions, numbers of things that do
not exist ...), the other parameters plain. Reference: the same call with the plain value on the same object, defined and plotted.
- the call raises (the plot is not made), or it gives exactly one message more than the plain call, either when the plot is defined
  or when it is plotted (loading the saved plot may say again what was said when it was defined): an invalid value is ignored, what was
  set before stays (a setter reported when set: the plain call followed by the invalid one gives the plot of the plain call; a
  legend row for pad or data 0 is not added; a setter of several values with one invalid sets none);
  named: the same tree joined again, a selection of the entries (Filter) of data that is not projected, pad margins leaving no room
  for the frame (also one margin beside that of the base plot); margins with room are drawn as set, whatever the order of setting; a
  range whose ends are set one by one and end up inverted (of an axis or data, also one end in the base plot);
  the messages give the value as written (a float not widened to double);
- values that mean something in ROOT (VALID: ROOT's default palette, a digit 0 of an alignment, ticks on the other side, no reference
  function, the forms of functions) give no message;
- a call repeated with another value replaces the first one, in either order (the saved plot equals the one with only the second call);
  a call that adds something (Filter, Define, Join, AddSymbolOf, ApplyLayout) keeps the first one; named cases of calls replacing each
  other (Entries, a drawing option and an alias, contour levels and their number, also from a layout, SetDefinesFrame(False),
  SetTransparent after a fill colour).
The messages of the style setters name what was set (the width of SetFrameBorder as "frame border width", SetDefaultLineWidth of a legend
as "legend default line width"), and a style setter given several invalid values names each of them; they name the object it belongs to
where it has a name (the axis, the data, the plot with its group, which margin of a layout).
Style numbers (markers, lines, fills) are not checked: ROOT draws an unknown style in its default style.
Covers: the same tree joined again, a selection of data that is not projected, pad margins without room for the
frame, ranges inverted by single-end setters, calls replacing each other, a projection range in bin numbers with a fraction,
Normalize(min, max) without isDensity, a Define column name that is no identifier, an empty base plot name.
"""

import os
import re

import numpy as np

import srptest as S

NAN, INF = float("nan"), float("inf")
# values that mean something in ROOT or SciRooPlot: no message
VALID = {
    "palette": [0, -1, 1, 20],  # up to 0: ROOT's default palette, 1: its rainbow, up to 50: so many colours of the default palette
    "align": [0, 10, 3],  # a digit 0: left, middle
    "tickLength": [-0.03],  # ticks on the other side
    "offset": [-0.2],
    "refFunc": [""],  # none: hides the one of the base plot or of pad 0
    "margin": [0.0],
    "frameFraction": [0.0],
    "function": ["TMath::Pi()*x", "x[0]*2", "[](double* x, double* p) { return 2 * x[0]; }"],
}
SKIPPED = ("PlotManager", "PanelLayout", "Panel", "BasePlotShape")  # the project (cli tests); the layouts (layouts/geometry.py)
# base plots are left out: their setters are those of plots, and a base plot is not made by itself; the C axis too: it is ignored for
# data other than 3d histograms (with a warning), its setters are those of the other axes


# the cases: each invalid value with its plain twin on the same host
cases = []
for case in S.setter_cases()[0]:
    if case["variant"] != "plain" or not case["meant"] or case["unset"] or case["host"] in ("statement", "base", "base_pad", "c") or case["cls"] in SKIPPED:
        continue
    plain = {"py": case["py"], "label": f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']}"}
    for i, param in enumerate(case["params"]):
        for key in (f"{case['cls']}.{case['method']}.{param}", f"{case['method']}.{param}", param):
            if key in VALID:
                valids = VALID[key]
                break
        else:
            valids = []
        for valid, values in ((False, S.setter_invalid(case["cls"], case["method"], param)), (True, valids)):
            for bad in values:
                given = list(case["values"])
                given[i] = bad
                host = S.SETTER_HOSTS[case["cls"]][case["host"]][0]
                badCall = S.call(case["method"], given, "py")
                # the plain call followed by the invalid one: the plot of the plain call alone, where the value is reported when set
                after = host.replace("{call}", S.call(case["method"], case["values"], "py") + badCall)
                cases.append({**plain, "plainCase": plain, "param": param, "bad": bad, "valid": valid, "py": host.replace("{call}", badCall), "after": after})
# named: a Join of the same tree again is ignored with a warning
join = 'p = Plot("{name}", "g", "1d")\np[1].AddData("t", "s").Project1D("x").Join("t2", ["k1"]){again}\npm.AddPlot(p)'
plain = {"py": join.replace("{again}", ""), "label": "Data.Join(tree, keys) on tree"}
cases.append({**plain, "plainCase": plain, "param": "tree", "bad": "the same tree again", "valid": False, "py": join.replace("{again}", '.Join("t2", ["k1"])')})
# named: a selection of the entries of a tree that is not projected has no effect
for given, data in (("tree", 'p[1].AddData("t", "s")'), ("ratio denominator", 'p[1].AddRatio("h", "s", "h2", "s2").Denom()')):
    plain = {"py": f'p = Plot("{{name}}", "g", "1d")\n{data}\npm.AddPlot(p)', "label": f"Data.Filter(filter) on a {given} without projection"}
    cases.append({**plain, "plainCase": plain, "param": "filter", "bad": "x > 2", "valid": False, "py": f'p = Plot("{{name}}", "g", "1d")\n{data}.Filter("x > 2")\npm.AddPlot(p)'})
# named: margins of a pad leaving no room for the frame (top + bottom or left + right of 1 or more), set together or one of them beside
# the margin of the base plot (said when plotted); a pair with room is accepted whatever the order of setting (top 0.9, bottom 0.05)
for given, bad, good, valid in (
    ("SetMargins(top, bottom, left, right)", "SetMargins(0.6, 0.6, 0.1, 0.1)", "SetMargins(0.1, 0.1, 0.1, 0.1)", False),
    ("SetTopMargin(top) beside the bottom margin of the base plot", "SetTopMargin(0.95)", "SetTopMargin(0.1)", False),
    ("SetMargins(top, bottom, left, right)", "SetMargins(0.9, 0.05, 0.1, 0.1)", "SetMargins(0.1, 0.1, 0.1, 0.1)", True),
):
    plain = {"py": f'p = Plot("{{name}}", "g", "1d")\np[1].{good}\npm.AddPlot(p)', "label": f"Pad.{given}"}
    cases.append({**plain, "plainCase": plain, "param": "margins", "bad": bad, "valid": valid, "py": f'p = Plot("{{name}}", "g", "1d")\np[1].{bad}\npm.AddPlot(p)'})
# named: a range whose ends are set one by one, inverted by the second end (said when plotted): of an axis, of data in x and y, and one end
# in the base plot (base_range: y up to 10), the other in the plot
for given, bad, good, base in (
    ("Axis.SetMinRange(min) beside the maximum set before", 'p[1]["Y"].SetRange(0, 10).SetMinRange(20)', 'p[1]["Y"].SetRange(0, 10).SetMinRange(5)', "1d"),
    ("Axis.SetMaxRange(max) beside the minimum set before", 'p[1]["X"].SetRange(2, 8).SetMaxRange(1)', 'p[1]["X"].SetRange(2, 8).SetMaxRange(9)', "1d"),
    ("Data.SetMinRangeX(min) beside the maximum set before", 'p[1].GetData(1).SetRangeX(2, 8).SetMinRangeX(9)', 'p[1].GetData(1).SetRangeX(2, 8).SetMinRangeX(3)', "1d"),
    ("Data.SetMaxRangeY(max) beside the minimum set before", 'p[1].GetData(1).SetRangeY(6, 14).SetMaxRangeY(5)', 'p[1].GetData(1).SetRangeY(6, 14).SetMaxRangeY(13)', "1d"),
    ("Axis.SetMinRange(min) beside the maximum of the base plot", 'p[1]["Y"].SetMinRange(20)', 'p[1]["Y"].SetMinRange(5)', "base_range"),
):
    head = f'p = Plot("{{name}}", "g", "{base}")\np[1].AddData("h", "s")\n'
    plain = {"py": f"{head}{good}\npm.AddPlot(p)", "label": given}
    cases.append({**plain, "plainCase": plain, "param": "range", "bad": bad, "valid": False, "py": f"{head}{bad}\npm.AddPlot(p)"})
plains = list({id(case["plainCase"]): case["plainCase"] for case in cases}.values())
# projections need data that can be projected: a 3d histogram for those of histograms, the trees for the requests
REQUESTS = ("Project1D", "Project2D", "Profile1D", "Profile2D", "Scatter")
for case in plains + cases:
    method = case["label"].split("(")[0].split(".")[1]
    if method in S.SELECTIONS:
        names = ("t", "t2") if method in REQUESTS else ("h3", "h3")
        for key in ("py", "after"):
            if key in case:
                case[key] = case[key].replace('AddData("h", "s")', f'AddData("{names[0]}", "s")').replace('AddRatio("h", "s", "h2", "s2")', f'AddRatio("{names[0]}", "s", "{names[1]}", "s2")')
for case in plains + cases:  # every plot draws something (a pad without data warns)
    for key in ("py", "after"):
        if key in case:
            case[key] = case[key].replace('p = Plot("{name}", "g", "1d")\n', 'p = Plot("{name}", "g", "1d")\np[1].AddData("h", "s")\n')

# a call repeated with another value: the plot is the one with only the second call; one that adds something keeps the first
ADDERS = ("Filter", "Define", "Join", "JoinByIndex", "AddSymbolOf", "ApplyLayout")  # ApplyLayout merges the layout into what is set
repeated = S.S.PlotManager("repeated")
namespace = dict(vars(S.S), pm=repeated)
pairs = []
for case in S.setter_cases()[0]:
    if case["variant"] == "plain" or not case["meant"] or case["unset"] or case["host"] == "statement" or case["method"].startswith("Add") and case["method"] not in ADDERS:
        continue  # (an Add... of a pad or box adds another object, which a second call cannot reach)
    hostPy = S.SETTER_HOSTS[case["cls"]][case["host"]][0]
    plainValues = [S.setter_value(case["cls"], case["method"], param, "plain") for param in case["params"]]
    calls = {"plain": S.call(case["method"], plainValues, "py"), case["variant"]: S.call(case["method"], case["values"], "py")}
    for first, second in ((case["variant"], "plain"), ("plain", case["variant"])):
        name = f"r{len(pairs)}"
        results = [S.captured(lambda code=hostPy.replace("{call}", given).replace("{name}", name + suffix): exec(code, namespace))[0] for suffix, given in (("_twice", calls[first] + calls[second]), ("_once", calls[second]))]  # noqa: S102
        pairs.append((name, f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']}, {first} then {second}", case["method"] in ADDERS, results[0]))
# calls replacing each other, named: (what, the calls, the calls with the same result)
REPLACING = [
    ("Entries(n) after Entries(min, max)", 'p[1].AddData("t", "s").Project1D("x").Entries(20, 40).Entries(10)', 'p[1].AddData("t", "s").Project1D("x").Entries(10)'),
    ("Denom().Entries(n) after Denom().Entries(min, max)", 'p[1].AddRatio("t", "s", "t2", "s2").Both().Project1D("x").Denom().Entries(20, 40).Entries(10)', 'p[1].AddRatio("t", "s", "t2", "s2").Both().Project1D("x").Denom().Entries(10)'),
    ("a drawing option after an alias", 'p[1].AddData("h", "s").SetDrawOption(points_line).SetDrawOption("hist e")', 'p[1].AddData("h", "s").SetDrawOption("hist e")'),
    ("an alias after a drawing option", 'p[1].AddData("h", "s").SetDrawOption("hist e").SetDrawOption(points_line)', 'p[1].AddData("h", "s").SetDrawOption(points_line)'),
    ("a number of contour levels after levels", 'p[1].AddData("h", "s").SetContours([0.0, 5.0, 10.0]).SetContours(30)', 'p[1].AddData("h", "s").SetContours(30)'),
    ("contour levels after a number of them", 'p[1].AddData("h", "s").SetContours(30).SetContours([0.0, 5.0, 10.0])', 'p[1].AddData("h", "s").SetContours([0.0, 5.0, 10.0])'),
    ("a layout's number of contour levels after levels", 'p[1].AddData("h", "s").SetContours([0.0, 5.0, 10.0]).ApplyLayout(Data("h", "s").SetContours(30))', 'p[1].AddData("h", "s").SetContours(30)'),
    ("SetDefinesFrame(False) after SetDefinesFrame()", 'p[1].AddData("h", "s").SetDefinesFrame().SetDefinesFrame(False)', 'p[1].AddData("h", "s")'),
    ("SetDefinesFrame(False) on a ratio", 'p[1].AddRatio("h", "s", "h2", "s2").SetDefinesFrame().SetDefinesFrame(False)', 'p[1].AddRatio("h", "s", "h2", "s2")'),
    ("SetTransparent of a plot keeps its fill colour", "p.SetFillColor(632).SetTransparent()", "p.SetFillColor(632).SetFillStyle(0)"),
    ("SetTransparent of a pad keeps its fill colour", "p[1].SetFillColor(600).SetTransparent()", "p[1].SetFillColor(600).SetFillStyle(0)"),
]
for what, given, same in REPLACING:
    name = f"r{len(pairs)}"
    for suffix, code in (("_twice", given), ("_once", same)):
        S.captured(lambda code=code, suffix=suffix: exec(f'p = Plot("{name}{suffix}", "g", "1d")\n{code}\npm.AddPlot(p)', namespace))  # noqa: S102
    pairs.append((name, what, False, None))
repeatedFile = os.path.join(S.WORK, "repeated.info")
repeated.SavePlots(file=repeatedFile)
saved = {key.split(":")[0]: values for key, values in S.info_plots(repeatedFile).items()}
for name, label, adds, raised in pairs:
    if isinstance(raised, Exception):  # a second call that cannot be added to the first (a Join of another tree with the same column names)
        S.check(f"{label}: the second call refused with a reason", adds and bool(str(raised).strip()), repr(raised))
        continue
    twice = {k: v for k, v in saved.get(f"{name}_twice", {}).items() if k != "name"}
    once = {k: v for k, v in saved.get(f"{name}_once", {}).items() if k != "name"}
    if adds:
        S.check(f"{label}: both calls stored", bool(twice) and twice != once, "the first call is lost")
    else:
        S.check(f"{label}: the second call replaces the first", bool(twice) and twice == once, str({k: (twice.get(k), once.get(k)) for k in set(twice) | set(once) if twice.get(k) != once.get(k)})[:300])

p = S.Project("setters", bases=("1d", "2d", "1d_ratio"))
rows = 30
columns = {name: np.linspace(0.5, 9.5, rows) for name in ("x", "y", "z", "w", "xErr", "yErr", "xErrLow", "xErrHigh", "yErrLow", "yErrHigh")}
columns.update(k1=np.arange(rows, dtype=float), k2=np.arange(rows, dtype=float) % 3)
for source, (hist, tree) in {"s": ("h", "t"), "s2": ("h2", "t2")}.items():
    h = S.ROOT.TH1D(hist, "", 10, 0, 10)
    for b in range(1, 11):
        h.SetBinContent(b, 5 + b)
        h.SetBinError(b, 1.0)
    h3 = S.ROOT.TH3D("h3", "", 10, 0, 10, 8, 0, 8, 6, 0, 6)
    for i in range(500):
        h3.Fill(i % 10 + 0.5, i % 8 + 0.5, i % 6 + 0.5)
    S.write_objects(p.path(f"{source}.root"), {hist: h, "h3": h3} if source == "s" else {hist: h, "h": h.Clone("h"), "h3": h3})
    S.write_tree(p.path(f"{source}.root"), columns, tree)
    p.pm.AddDataSource(source, p.path(f"{source}.root"))
baseRange = S.S.PlotManager.MakeBasePlot("1d")
baseRange.SetName("base_range")
baseRange[1]["Y"].SetMaxRange(10)
p.pm.AddBasePlot(baseRange)
namespace = dict(vars(S.S), pm=p.pm, nan=NAN, inf=INF)
start = S.S.Plot("c_start", "g", "1d")  # made first: the messages before it are those of reading the data
start[1].AddData("h", "s")
p.pm.AddPlot(start)
for i, case in enumerate(plains + cases):
    case["name"] = f"c{i}"
    result, printed = S.captured(lambda case=case: exec(case["py"].replace("{name}", case["name"]), namespace))  # noqa: S102  the cases
    case["raised"] = result if isinstance(result, Exception) else None
    case["defined"] = [line.strip() for line in printed.splitlines() if line.strip()]
    # a plot after each one, to tell its messages from those of the next one (a plot that is not made says so before the next plot)
    after = S.S.Plot(f"{case['name']}_next", "g", "1d")
    after[1].AddData("h", "s")
    p.pm.AddPlot(after)
    if case["raised"]:
        continue
    # the plot run loads the saved file first (its messages come before the plots are made): the plot loaded alone
    single = os.path.join(S.WORK, "single.info")
    p.pm.SavePlots(name=f"{case['name']}(_renamed)?", file=single)
    _, printed = S.captured(lambda: S.S.PlotManager("loaded").LoadPlots(file=single))
    case["loaded"] = [line.strip() for line in printed.splitlines() if line.strip() and "[ INFO" not in line]

order = S.S.Plot("margins_order", "g", "1d")  # margins with room, which ROOT drops when set one by one in this order
order[1].AddData("h", "s")
order[1].SetMargins(0.9, 0.05, 0.85, 0.1)
p.pm.AddPlot(order)
run = p.plot()
# lines that only follow from a message before them (the plot or data that could not be made because of it)
CONSEQUENCES = re.compile(r"could not be created|^- missing |Projection failed for |plots? cannot be created due to")
lines = run.log.splitlines()
lines = [line.strip() for line in lines[next(i for i, line in enumerate(lines) if "plots matching the request" in line) + 1 :]]
lines = [line for line in lines if line and "[ INFO" not in line and not CONSEQUENCES.search(line)]
created = {m.group(1): i for i, line in enumerate(lines) if (m := re.search(r"Created plot (\S+) from group \S+\.$", line))}
reading = lines[: created["c_start"]]
previous = created["c_start"]
for case in plains + cases:
    # the lines between the plot before (the next one of the case before) and the case's next one; messages of reading the data that
    # name the value (a function is named by its formula)
    end = created.get(case["name"] + "_next", -1)
    segment = lines[previous + 1 : end] if end >= 0 else []
    case["made"] = [line for line in segment if "[ LOG" not in line] if end >= 0 else None
    if case.get("bad") and isinstance(case["bad"], str) and case["made"] is not None:
        case["made"] += [line for line in reading if f"'{case['bad']}'" in line]
    previous = max(previous, end)


def new_messages(case, plain, phase):
    """the messages of a phase that the plain call does not give (the plot named as in the plain one; points named by their values lying
    outside the frame in both)"""
    outside = re.compile(r"Data .+ lies entirely ")
    left = [outside.sub("Data {data} lies entirely ", line.replace(plain["name"], "{name}")) for line in plain[phase] or []]
    new = []
    for line in case[phase] or []:
        line = outside.sub("Data {data} lies entirely ", line.replace(case["name"], "{name}"))
        if line in left:
            left.remove(line)
        else:
            new.append(line)
    return new


def label(case):
    return f"{case['label']} with {case['param']}={case['bad']!r}"


# an invalid value of a setter reported where it is set is ignored there: after the plain call, the saved plot is that of the plain call
# alone (Add..., projections and normalisations make what they make without the invalid part; SetGroup normalizes the group it is given);
# named: a row of a legend for pad or data 0 is not added
NORMALIZING = ("SetGroup",)
ignoredPm = S.S.PlotManager("ignored")
namespace = dict(vars(S.S), pm=ignoredPm, nan=NAN, inf=INF)
for case in cases:
    method = case["label"].split("(")[0].split(".")[1]
    if "after" in case and method.startswith("Set") and method not in NORMALIZING and not case["valid"] and not case["raised"] and new_messages(case, case["plainCase"], "defined"):
        for suffix, code in (("_after", case["after"]), ("_plain", case["plainCase"]["py"])):
            S.captured(lambda code=code, suffix=suffix: exec(code.replace("{name}", case["name"] + suffix), namespace))  # noqa: S102
        case["checkIgnored"] = True
for padID, dataID in ((1, 0), (0, 1)):
    for suffix, row in (("_after", f".AddDataEntry({padID}, {dataID})"), ("_plain", "")):
        S.captured(lambda suffix=suffix, row=row: exec(f'p = Plot("entry_{padID}_{dataID}{suffix}", "g", "1d")\np[1].AddData("h", "s")\np[1].AddLegend(){row}\npm.AddPlot(p)', namespace))  # noqa: S102
# named: a setter of several values with one of them invalid sets none of them (the other value differs from the one set before)
for suffix, calls in (("_after", ".SetDimensions(800, 600).SetDimensions(1000, -1, True)"), ("_plain", ".SetDimensions(800, 600)")):
    S.captured(lambda suffix=suffix, calls=calls: exec(f'p = Plot("dimensions{suffix}", "g", "1d")\np[1].AddData("h", "s")\np{calls}\npm.AddPlot(p)', namespace))  # noqa: S102
ignoredFile = os.path.join(S.WORK, "ignored.info")
ignoredPm.SavePlots(file=ignoredFile)
ignoredSaved = {key.split(":")[0]: {k: v for k, v in values.items() if k != "name"} for key, values in S.info_plots(ignoredFile).items()}
for padID, dataID in ((1, 0), (0, 1)):
    after, alone = ignoredSaved.get(f"entry_{padID}_{dataID}_after"), ignoredSaved.get(f"entry_{padID}_{dataID}_plain")
    S.check(f"LegendBox.AddDataEntry({padID}, {dataID}): no row added", bool(after) and after == alone, str(after)[:300])
after, alone = ignoredSaved.get("dimensions_after"), ignoredSaved.get("dimensions_plain")
S.check("Plot.SetDimensions(1000, -1) after SetDimensions(800, 600): nothing set", bool(after) and after == alone, str({k: (after.get(k), alone.get(k)) for k in set(after) | set(alone) if after.get(k) != alone.get(k)} if after and alone else "not saved"))

for case in cases:
    plain = case["plainCase"]
    if case.get("checkIgnored"):
        after, alone = ignoredSaved.get(case["name"] + "_after"), ignoredSaved.get(case["name"] + "_plain")
        diff = {k: (after.get(k), alone.get(k)) for k in set(after) | set(alone) if after.get(k) != alone.get(k)} if after and alone else "not saved"
        S.check(f"{label(case)}: ignored where set (the value set before stays)", bool(after) and after == alone, str(diff)[:300])
    if case["valid"]:
        new = {phase: new_messages(case, plain, phase) for phase in ("defined", "loaded", "made")} if not case["raised"] else {}
        S.check(f"{label(case)}: accepted without a message", not case["raised"] and case["made"] is not None and not any(new.values()), f"{case['raised']!r} {new}")
        continue
    if case["raised"]:
        S.check(f"{label(case)}: raised with a reason", bool(str(case["raised"]).strip()), repr(case["raised"]))
        continue
    new = {phase: new_messages(case, plain, phase) for phase in ("defined", "loaded", "made")}
    ok = case["made"] is not None and len(new["defined"]) <= 1 and len(new["loaded"] + new["made"]) <= 1 and any(new.values())
    S.check(f"{label(case)}: reported once", ok, f"new messages: {new}" if any(new.values()) else "no new message")


# the messages of the style setters name what was set: "default line width" for SetDefaultLineWidth (of a pad or a legend), "frame border
# width" for the width of SetFrameBorder, "marker alpha" for the alpha of SetMarker
STYLE_SETTER = re.compile(r"Set(Default)?(FrameFill|FrameBorder)?(Marker|Line|Fill|Text|Border)?(Alpha|Width|Size)?")


def attribute(case):
    """the attribute a message about the invalid value of a style setter names, None for other setters and parameters"""
    method = case["label"].split("(")[0].split(".")[1]
    m = STYLE_SETTER.fullmatch(method)
    if not m or case["param"] not in ("alpha", "width", "size") or (m.group(4) or case["param"]).lower() != case["param"] or not (any(m.groups()[:3]) or method == "SetAlpha"):
        return None
    words = [re.sub(r"(?<!^)([A-Z])", r" \1", g).lower() for g in m.groups()[:3] if g]
    return " ".join([*words, case["param"]])


for case in cases:
    if case["valid"] or case["raised"] or not (what := attribute(case)):
        continue
    new = new_messages(case, case["plainCase"], "defined")
    S.check(f"{label(case)}: the message names the {what}", any(what in line for line in new), str(new))

# a setter of several values, each of them invalid: every one of them named (none hidden behind the first)
for case in S.setter_cases()[0]:
    if case["variant"] != "plain" or not case["meant"] or case["host"] in ("statement", "base", "base_pad", "c") or case["cls"] in SKIPPED:
        continue
    bad = {param: S.setter_invalid(case["cls"], case["method"], param) for param in case["params"]}
    bad = {param: values[0] for param, values in bad.items() if values and param in ("style", "font", "size", "width", "alpha")}
    if len(bad) < 2 or not STYLE_SETTER.fullmatch(case["method"]):
        continue
    given = [bad.get(param, value) for param, value in zip(case["params"], case["values"])]
    host = S.SETTER_HOSTS[case["cls"]][case["host"]][0].replace("{name}", f"all_{case['cls']}_{case['method']}_{case['host']}")
    result, printed = S.captured(lambda code=host.replace("{call}", S.call(case["method"], given, "py")): exec(code, namespace))  # noqa: S102
    lines = [line for line in printed.splitlines() if line.strip()]
    S.check(f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']} with {', '.join(bad)} invalid: each named", not isinstance(result, Exception) and len(lines) == len(bad), str(lines)[:300])
widened = [line for case in cases for phase in ("defined", "loaded", "made") for line in case.get(phase) or [] if re.search(r"\d\.\d{12,}", line)]
S.check("values in the messages as given (a float not widened to double: 1.2, not 1.2000000476837158)", not widened, " | ".join(widened[:3]))
pad = run.pad("margins_order", "g")
margins = [round(m, 6) for m in (pad.GetTopMargin(), pad.GetBottomMargin(), pad.GetLeftMargin(), pad.GetRightMargin())] if pad else None
S.check("margins with room for the frame are drawn as set, whatever the order of setting", margins == [0.9, 0.05, 0.85, 0.1], str(margins))

# what a message refers to: the axis, the data, the plot, the margin
NAMED = {
    "axis title size": (lambda: S.S.Plot("a", "g", "1d")[1]["X"].SetTitleSize(-1.0), "title size of axis X (-1)"),
    "axis alpha": (lambda: S.S.Plot("a", "g", "1d")[1]["Y"].SetAxisAlpha(3.0), "axis alpha of axis Y (3)"),
    "data marker size": (lambda: S.S.Data("h", "s").SetMarkerSize(-2.0), "marker size of h (-2)"),
    "data line alpha": (lambda: S.S.Data("h", "s").SetLineAlpha(2.0), "line alpha of h (2)"),
    "layout margin": (lambda: S.S.PanelLayout().SetMargins(-1.0, 0.0, 0.0, 0.0), "top margin of PanelLayout::SetMargins (-1)"),
    "layout marker size": (lambda: S.S.PanelLayout().SetMarkerSize(-1.0), "marker size of PanelLayout::SetMarkerSize (-1)"),
    "layout line width": (lambda: S.S.PanelLayout().SetLineWidth(2.5), "line width of PanelLayout::SetLineWidth (2.5) is not an integer"),
    "plot width": (lambda: S.S.Plot("a", "g", "1d").SetWidth(0), "from group"),
    "plot fill alpha": (lambda: S.S.Plot("a", "g", "1d").SetFillAlpha(3.0), "fill alpha of plot a from group g (3)"),
    "axis title font": (lambda: S.S.Plot("a", "g", "1d")[1]["X"].SetTitleFont(999), "title font of axis X (999)"),
    "axis label font": (lambda: S.S.Plot("a", "g", "1d")[1]["Y"].SetLabelFont(999), "label font of axis Y (999)"),
    "text alignment": (lambda: S.S.Plot("a", "g", "1d")[1].AddText(0.5, 0.5, "t").SetTextAlign(5), "0 for the default"),
    "shape kind": (lambda: S.S.Plot("a", "g", "1d")[1].AddRegionX(1.0, 2.0).SetArc(0.0, 90.0), "ignored on this region X"),
    "projection range in bin numbers with a fraction": (lambda: S.S.Data("h", "s").ProjectX(-0.8, 0.8), "range [-0.8, 0.8] of axis 1 is given in bin numbers"),
}
for what, (call, text) in NAMED.items():
    _, printed = S.captured(call)
    plain = re.sub(r"\x1b\[[0-9;]*m", "", printed)
    S.check(f"the message names what it refers to: {what}", text in plain, plain.strip())
# a normalization range without isDensity would be taken as isDensity and minimum: refused by the conversion (in C++ a deleted overload)
result, _ = S.captured(lambda: S.S.Data("h", "s").Normalize(2.0, 6.0))
S.check("Normalize(min, max) without isDensity is refused", isinstance(result, TypeError), repr(result))
# the column name of Define is used in expressions: one that is no identifier is refused where set; an empty base plot name is ignored
refused, _ = S.captured(lambda: S.S.Data("tracks", "s").Define("a b", "1"))
S.check("Define with a column name that is no identifier: refused where set", isinstance(refused, Exception) and "a column name has letters, digits and _" in str(refused), str(refused))
basePlot = S.S.Plot("base_name", "g", "1d")
_, printed = S.captured(lambda: basePlot.SetBasePlot(""))
S.check("SetBasePlot with an empty name: ignored with a warning where set", "The empty name of the base plot of base_name is ignored" in printed, printed)
S.finish()
