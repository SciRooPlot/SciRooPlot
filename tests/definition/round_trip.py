"""Every call of the plot definition is stored as given and read back as stored.

The cases are every overload of the Python interface (except the PlotManager's: the project, see the cli tests) on every object it can be
called on (a plot and a base plot, pad 1 and pad 0, each axis, data and each side of a ratio, legend rows of data and added ones, boxes,
shapes, layouts), with values that differ from the defaults and in the variants precise (1/3), extreme (±1e300), text (every character
the file format escapes), empty text and false (srptest.SETTER_VALUES). The reference is the call itself:
- the call is accepted without a message;
- the saved plot differs from the plot without the call (a call clearing a value: equals it), and holds every number and text given
  (except the layouts, whose values become the positions and sizes of the pads, and the colours of gradients, stored as rgb);
- a call made where it is not meant to be (data in pad 0 or a base plot, a selection of the ratio itself, the arc of a line ...): stored
  or reported, never dropped silently;
- loading the saved file and saving it again gives the same file, plot by plot, without a message.
Named: a ratio denominator without data source takes the numerator's (every form of AddRatio), a numerator needs one.
An overload without a test value fails, so a new setter needs its values in srptest.SETTER_VALUES. The whole list takes a second.
"""

import os
import re

import srptest as S

cases, problems = S.setter_cases()
S.check("every overload of the interface has a case", not problems, "; ".join(problems))
pm = S.S.PlotManager("roundtrip")
namespace = dict(vars(S.S), pm=pm)
baselines = {}
for cls, hosts in S.SETTER_HOSTS.items():
    for host, (py, _) in hosts.items():
        name = f"base_{cls}_{host}"
        baselines[(cls, host)] = name
        S.captured(lambda py=py, name=name: exec(py.replace("{call}", "").replace("{name}", name), namespace))  # noqa: S102  (a host made only for one call can fail without it)


def label(case):
    values = f" {case['variant']}" if case["variant"] != "plain" else ""
    return f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']}{values}"


for i, case in enumerate(cases):
    case["name"] = f"c{i}"
    result, printed = S.captured(lambda case=case: exec(case["py"].replace("{name}", case["name"]), namespace))  # noqa: S102
    case["accepted"] = not isinstance(result, Exception)
    case["printed"] = printed.strip()
    if case["meant"]:
        S.check(f"{label(case)}: accepted without a message", case["accepted"] and not printed.strip(), f"{result!r} {printed.strip()}"[:300])

first, second = os.path.join(S.WORK, "first.info"), os.path.join(S.WORK, "second.info")
pm.SavePlots(file=first)
saved = {key.split(":")[0]: values for key, values in S.info_plots(first).items()}

CONVERTED = ("PanelLayout", "Panel", "BasePlotShape")  # their values become the positions and sizes of the pads
NUMBER = re.compile(r"[-+]?(?:inf|nan|\d+\.?\d*(?:e[-+]?\d+)?|\.\d+(?:e[-+]?\d+)?)", re.I)


def scalars(value, param):
    """the numbers and texts a value is made of (flags and enumerations are not looked for: they are stored as words; colours given as
    (r, g, b) are stored as #rrggbb)"""
    if isinstance(value, (S.Code, bool)):
        return []
    if param == "gradient":
        return [position for _, position in value]
    if param in ("color", "colors", "gradient") and isinstance(value, tuple) and len(value) == 3 and all(isinstance(v, float) for v in value):
        return ["#" + "".join(f"{round(v * 255):02x}" for v in value)]
    if isinstance(value, (tuple, list)):
        return [x for v in value for x in scalars(v, param)]
    return [value]


def holds(changed, value):
    """the changed values of a plot hold a number or a text of the call (texts of lists are joined by $, which they escape as \\$)"""
    if isinstance(value, str):
        escaped = value.replace("\\", "\\\\").replace("$", "\\$")
        return any(value in v or escaped in v for v in changed) or (value.startswith("#") and any(value.lower() in v.lower() for v in changed))
    numbers = [float(n) for v in changed for n in NUMBER.findall(v)]
    return any(n == value or S.close(n, value, 1e-6) for n in numbers)


for case in cases:
    if not case["accepted"]:
        continue
    plot = saved.get(case["name"], saved.get(case["name"] + "_renamed"))
    if plot is None:
        S.check(f"{label(case)}: plot saved", False, "not in the file")
        continue
    if case["host"] == "statement":
        continue
    base = saved.get(baselines[(case["cls"], case["host"])], {})
    changed = {k: v for k, v in plot.items() if base.get(k) != v and (k != "name" or case["method"] == "SetName")}
    removed = [k for k in base if k not in plot]
    if not case["meant"]:
        S.check(f"{label(case)} (not meant for it): stored or reported", bool(changed) or bool(removed) or bool(case["printed"]), "neither")
        continue
    if case["unset"]:
        S.check(f"{label(case)}: the plot is as without any call", not changed and not removed, f"changed {changed}, removed {removed}")
        continue
    if case["variant"] in ("false", "empty"):
        continue
    S.check(f"{label(case)}: stored", bool(changed) or bool(removed), "the plot equals the one without the call")
    if case["cls"] in CONVERTED:
        continue
    missing = [v for param, value in zip(case["params"], case["values"]) for v in scalars(value, param) if changed and not holds(list(changed.values()), v.replace("{name}", case["name"]) if isinstance(v, str) else v)]
    S.check(f"{label(case)}: every value stored as given", not missing, f"{missing} not in {changed}"[:400])

loaded = S.S.PlotManager("roundtrip_loaded")
result, printed = S.captured(lambda: loaded.LoadPlots(file=first))
misplaced = {case["name"] for case in cases if not case["meant"]}
messages = [line for line in printed.splitlines() if line.strip() and "[ INFO ]" not in line and not any(re.search(rf"\b(Base plot|Plot) {name}\b", line) for name in misplaced)]
S.check("the saved file loads without a message (except for the misplaced calls)", result is True and not messages, f"{result!r} {' | '.join(messages)[:400]}")
loaded.SavePlots(file=second)
again = {key.split(":")[0]: values for key, values in S.info_plots(second).items()}
for case in cases:
    if case["accepted"] and case["name"] in saved:
        diff = {k: (saved[case["name"]].get(k), again.get(case["name"], {}).get(k)) for k in set(saved[case["name"]]) | set(again.get(case["name"], {})) if saved[case["name"]].get(k) != again.get(case["name"], {}).get(k)}
        S.check(f"{label(case)}: read back as stored", not diff, str(diff)[:400])
with open(first, "rb") as a, open(second, "rb") as b:
    S.check("the file saved after loading is the same file", a.read() == b.read())

# a ratio denominator without data source takes the numerator's, in every form; a numerator needs one [72984ac]
FORMS = {
    'p[1].AddRatio(["h", "s"], ["g"])': ("s", "s"),
    'p[1].AddRatio(["h", "s2"], ["g"], "label")': ("s2", "s2"),
    'p[1].AddRatio("h", "s", "g", "")': ("s", "s"),
    'p[1].AddRatio(Data("h", "s2"), Data("g"))': ("s2", "s2"),
    'p[1].AddRatio(Ratio(Data("h", "s2"), Data("g")))': ("s2", "s2"),
    'p[1].AddRatio(["h", "s"], ["g", "s2"])': ("s", "s2"),
    'p[1].AddRatio(["h"], ["g"])': None,
    'p[1].AddRatio(Data("h"), Data("g", "s"))': None,
    'p[1].AddRatio(["h", "s", "x"], ["g"])': None,
    'p[1].AddData(Data("g"))': None,
}
sources = S.S.PlotManager("sources")
namespace["pm"] = sources
for i, (form, expected) in enumerate(FORMS.items()):
    result, _ = S.captured(lambda form=form, i=i: exec(f'p = Plot("r{i}", "g", "1d")\n{form}\npm.AddPlot(p)', namespace))  # noqa: S102
    if expected is None:
        S.check(f"{form}: refused with a reason", isinstance(result, Exception) and bool(str(result).strip()), repr(result))
        continue
    path = os.path.join(S.WORK, "sources.info")
    sources.SavePlots(name=f"r{i}", file=path)
    data = next(iter(S.info_plots(path).values()), {})
    found = (data.get("PAD_1/DATA_1/data_source"), data.get("PAD_1/DATA_1/denom_data_source"))
    S.check(f"{form}: data sources of numerator and denominator {expected}", not isinstance(result, Exception) and found == expected, f"{result!r} {found}")
S.finish()
