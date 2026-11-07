"""A colour gradient (stops of (r, g, b, position) or (colour, position) in any colour form) gives the data of a pad one colour each, and a
pad its palette: n colours at equal steps from position 0 to 1, linear between the stops and the end colours before the first and after the
last stop; n is the number of data items of the pad that take a colour from the defaults (not those setting all three colours or keeping
the style of their input) unless given, for a palette 255. A gradient of pad 0 is sized for each pad by its own data; a gradient of the
pad wins over pad 0's; the alpha given with it makes every colour transparent. A gradient no data takes from gives no message. Stops
are checked when set: fewer than two stops, a number of colours below 1, positions outside 0 to 1, decreasing positions and components
outside 0 to 1 warn and the gradient is ignored (the colours stay as before, nothing is said again when plotted), equal neighbours (a hard
step) do not.
Covers: 2e30e68 (no gradient error without data taking colours), 9f1978c (stops checked, end colours beyond the first and last stop).
"""

from srptest import DEFAULT_COLORS, ROOT, Project, S, add_data, cases, check, describe, finish, python, rgba

STOPS = {  # positions and colours of the stops, in the forms they are given in
    "two": [((1.0, 0.0, 0.0), 0.0), ((0.0, 0.0, 1.0), 1.0)],
    "three": [((0.1, 0.1, 0.4), 0.0), ((1.0, 0.5, 0.0), 0.3), ((1.0, 1.0, 0.8), 1.0)],
    "inner": [((1.0, 0.0, 0.0), 0.2), ((0.0, 0.0, 1.0), 0.8)],  # end colours before 0.2 and after 0.8
    "step": [((1.0, 0.0, 0.0), 0.0), ((1.0, 0.0, 0.0), 0.5), ((0.0, 0.0, 1.0), 0.5), ((0.0, 0.0, 1.0), 1.0)],
}
FORMS = {  # a stop colour (r, g, b) in the form given to the setter
    "rgb_position": lambda rgb, pos: (*rgb, pos),
    "tuple": lambda rgb, pos: (rgb, pos),
    "hex": lambda rgb, pos: ("#{:02x}{:02x}{:02x}".format(*(int(c * 255 + 0.5) for c in rgb)), pos),
}
SETTERS = ("SetDefaultColors", "SetDefaultMarkerColors", "SetDefaultLineColors", "SetDefaultFillColors", "SetPalette")
PARTS = {"SetDefaultColors": ("Marker", "Line", "Fill"), "SetDefaultMarkerColors": ("Marker",), "SetDefaultLineColors": ("Line",), "SetDefaultFillColors": ("Fill",)}
# data of the pad besides the takers: one setting all colours and one keeping the input's style (both take none and are not counted)
EXTRAS = ("none", "set", "keep")
SET_COLOR = ROOT.kGreen + 3


def gradient_rgb(stops, n):
    """the reference: n colours at equal steps of the stops (end colours beyond the first and last stop), components from 0 to 1"""
    colours = []
    for i in range(n):
        x = i / (n - 1) if n > 1 else 0.0
        k = 0
        while k < len(stops) - 2 and x > stops[k + 1][1]:
            k += 1
        (c1, s1), (c2, s2) = stops[k], stops[k + 1]
        t = min(1.0, max(0.0, (x - s1) / (s2 - s1))) if s2 > s1 else 0.0
        colours.append(tuple(a + t * (b - a) for a, b in zip(c1, c2)))
    return colours


def near(colour, rgb, alpha):
    """whether a colour (a ROOT index or (r, g, b, alpha) as by rgba) has the rgb (from 0 to 1) and alpha: within two steps of 255, as ROOT
    cuts the components of a colour given as fractions to whole steps and then takes an existing colour within one step (TColor::GetColor)"""
    got, want = (colour if isinstance(colour, tuple) else rgba(colour)), rgba(rgb, alpha)
    return got is not None and all(abs(a - b) <= 2 for a, b in zip(got[:3], want[:3])) and got[3] == want[3]


def given(case):
    return [FORMS[case["form"]](rgb, pos) for rgb, pos in STOPS[case["stops"]]]


p = Project("gradients")
p.inputs()
todo = cases(
    {"setter": list(SETTERS), "stops": list(STOPS), "form": list(FORMS), "where": ["pad", "pad0", "both"], "takers": [1, 3, 7], "alpha": [None, 0.6], "count": [None, 4], "extras": list(EXTRAS)},
    valid=lambda c: c["setter"] != "SetPalette" or (c["takers"] == 1 and c["extras"] == "none"),
    named=[
        {"setter": "SetDefaultColors", "stops": "inner", "form": "rgb_position", "where": "pad", "takers": 7, "alpha": None, "count": None, "extras": "none", "why": "9f1978c"},
        {"setter": "SetPalette", "stops": "inner", "form": "rgb_position", "where": "pad", "takers": 1, "alpha": None, "count": 11, "extras": "none", "why": "9f1978c"},
    ],
)
OTHER = [((0.0, 1.0, 0.0), 0.0), ((0.0, 0.0, 0.0), 1.0)]  # pad 0's gradient where the pad has its own
for i, case in enumerate(todo):
    palette = case["setter"] == "SetPalette"
    plot = S.Plot(f"c{i}", "gradients", "2d" if palette else "1d")
    stops = given(case)
    for padID in {"pad": (1,), "pad0": (0,), "both": (0, 1)}[case["where"]]:
        own = stops if padID == 1 or case["where"] == "pad0" else [FORMS[case["form"]](rgb, pos) for rgb, pos in OTHER]
        getattr(plot[padID], case["setter"])(own, case["alpha"], case["count"])
    if palette:
        add_data(plot[1], "hist2d")
    else:
        for n in range(case["takers"]):
            add_data(plot[1], "hist")
            if n == 0 and case["extras"] == "set":
                add_data(plot[1], "hist").SetColor(SET_COLOR)
            elif n == 0 and case["extras"] == "keep":
                add_data(plot[1], "hist").SetKeepInputStyle()
    p.pm.AddPlot(plot)

# a gradient no data takes from gives no message (data setting its colours, a pad with only a text, pad 0 of a pad without data)
quiet = {
    "own_colours": lambda pl: (pl[1].SetDefaultColors(STOPS["two"]), [add_data(pl[1], "hist").SetColor(ROOT.kRed + n) for n in range(3)]),
    "text_only": lambda pl: (pl[1].SetDefaultColors(STOPS["two"]), pl[1].AddText(0.5, 0.5, "text")),
    "pad0_text_only": lambda pl: (pl[0].SetDefaultColors(STOPS["two"]), pl[1].AddText(0.5, 0.5, "text")),
}
for name, setup in quiet.items():
    plot = S.Plot(name, "quiet", "1d")
    setup(plot)
    p.pm.AddPlot(plot)
plot = S.Plot("zero", "zero", "1d")
plot[1].SetDefaultColors(STOPS["two"], None, 0)
add_data(plot[1], "hist")
p.pm.AddPlot(plot)
run = p.plot("gradients")


for i, case in enumerate(todo):
    what = describe(case)
    stops = [(rgb, pos) for rgb, pos in STOPS[case["stops"]]]
    if case["setter"] == "SetPalette":
        n = case["count"] or 255
        found = run.palette(f"c{i}", "gradients")
        want = gradient_rgb(stops, n)
        bad = [k for k, (colour, rgb) in enumerate(zip(found or [], want)) if not near(colour, rgb, case["alpha"])]
        check(f"palette of {n} colours along the gradient: {what}", found is not None and len(found) == n and not bad, f"{len(found or [])} colours, differing at {bad[:5]}: {[found[k] for k in bad[:3]]}" if found else "no setup")
        continue
    drawn = [obj for obj, _ in run.drawn(f"c{i}", "gradients")]
    takers = [obj for k, obj in enumerate(drawn) if not (case["extras"] != "none" and k == 1)]
    if len(takers) != case["takers"]:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    n = case["count"] or case["takers"]
    want = gradient_rgb(stops, n)
    for part in PARTS[case["setter"]]:
        found = [getattr(obj, f"Get{part}Color")() for obj in takers]
        bad = [k for k, index in enumerate(found) if not near(index, want[k % n], case["alpha"])]
        check(f"{part.lower()} colours along the gradient: {what}", not bad, f"data {[k + 1 for k in bad]}: {[rgba(found[k]) for k in bad[:3]]}, expected {[rgba(want[k % n], case['alpha']) for k in bad[:3]]}")
    if case["extras"] == "set":
        obj = drawn[1]
        check(f"data setting its colours keeps them: {what}", rgba(obj.GetMarkerColor()) == rgba(SET_COLOR) and rgba(obj.GetLineColor()) == rgba(SET_COLOR), rgba(obj.GetMarkerColor()))
check("no warnings", not run.warnings(), " | ".join(run.warnings()[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))

run = p.plot("quiet")
for name in quiet:
    check(f"made without messages: {name}", run.pad(name, "quiet") is not None)
check("a gradient no data takes from gives no messages", not run.errors() and not run.warnings(), " | ".join(run.errors()[:3] + run.warnings()[:3]))
run = p.plot("zero")
drawn = run.objects("zero", "zero")
check("a gradient ignored when set (0 colours): the default colours, nothing said when plotted", drawn and rgba(drawn[0].GetMarkerColor()) == rgba(DEFAULT_COLORS[0]) and not run.errors() and not run.warnings(), " | ".join(run.errors()[:3] + run.warnings()[:3]))

# stops are checked when set: warnings of the definition (a process of its own)
STOP_CHECKS = {  # stops, number of colours and the warning they give (None: accepted silently)
    "decreasing": ([(0, 0, 0, 0.0), (1, 0, 0, 0.6), (0, 1, 0, 0.4), (0, 0, 1, 1.0)], None, "stop 3 is at position 0.4, before the previous one"),
    "outside": ([(0, 0, 0, -0.1), (1, 1, 1, 1.0)], None, "stop 1 is at position -0.1, outside of 0 to 1"),
    "beyond": ([(0, 0, 0, 0.0), (1, 1, 1, 1.5)], None, "stop 2 is at position 1.5, outside of 0 to 1"),
    "component": ([(1.5, 0, 0, 0.0), (0, 0, 1, 1.0)], None, "stop 1 has the colour (1.5, 0, 0), components must be between 0 and 1"),
    "one_stop": ([(1, 0, 0, 0.5)], None, "a gradient needs at least two stops, got 1"),
    "zero_colours": ([(1, 0, 0, 0.0), (0, 0, 1, 1.0)], 0, "the number of gradient colours must be positive, got 0"),
    "step": ([(1, 0, 0, 0.0), (1, 0, 0, 0.5), (0, 0, 1, 0.5), (0, 0, 1, 1.0)], None, None),
    "colour_forms": ([("#ff0000", 0.0), (int(ROOT.kBlue), 1.0)], 3, None),
}
for setter in SETTERS:
    out = python("from SciRooPlot import *\n" + "".join(f"Plot('p', 'g', '1d')[1].{setter}({stops!r}, None, {n!r})\n" for stops, n, _ in STOP_CHECKS.values()))
    warned = [line for line in out.warnings() if setter in line]
    for name, (_, _, expected) in STOP_CHECKS.items():
        if expected:
            check(f"{setter}: stops {name} warn", sum(expected in line for line in warned) == 1, " | ".join(warned) or out.tail())
    check(f"{setter}: no other messages (equal neighbours, every colour form)", len(out.warnings()) == len(warned) == sum(e is not None for *_, e in STOP_CHECKS.values()) and not out.errors(), " | ".join(out.warnings() + out.errors()))
finish()
