"""A pad setting of a plot is taken, in this order, from the pad itself, from that pad of its base plot, from pad 0 of the plot and from
pad 0 of the base plot (ROOT's default without any), for every kind of pad setting: viewing angles of 3D views, the reference function,
log scales, axis titles and ranges, the frame fill colour and the palette, each set in every combination of these four places.
An empty cell of a layout is no panel: the ranges of pad 0 make no frame there. A pad added in one plot made from a base plot does not
reach the other plots made from it.
Covers: reference function from pad 0 of the base plot, view angles from pad 0, empty cells, pads of other plots.
"""

import itertools

from srptest import Project, S, add_data, check, finish

PLACES = ("pad", "base pad", "pad 0", "base pad 0")  # in the order they are taken


def value(setting, place):
    """the value a place gives a setting (different for every place)"""
    k = PLACES.index(place)
    return {
        "view": (20.0 + 10 * k, 100.0 + 40 * k),
        "reference function": f"{k + 2}.5",
        "log": k % 2 == 0,
        "title": f"title from {place}",
        "range": (2.0 + 0.5 * k, 9.0 - 0.5 * k),
        "frame fill": 2 + k,
        "palette": (S.kViridis, S.kCividis, S.kBird, S.kInvertedDarkBodyRadiator)[k],
    }[setting]


def apply(pad, setting, v):
    if setting == "view":
        pad.SetView(*v)
    elif setting == "reference function":
        pad.SetRefFunc(v)
    elif setting == "log":
        pad["Z"].SetLog(v)
    elif setting == "title":
        pad["X"].SetTitle(v)
    elif setting == "range":
        pad["X"].SetRange(*v)
    elif setting == "frame fill":
        pad.SetFrameFillColor(v)
    elif setting == "palette":
        pad.SetPalette(v)


SETTINGS = ("view", "reference function", "log", "title", "range", "frame fill", "palette")
DEFAULTS = {"view": (30.0, 30.0), "reference function": None, "log": False, "title": "", "range": (1.0, 10.0)}  # with none of the places
todo = [(setting, places) for setting in SETTINGS for n in range(len(PLACES) + 1) for places in itertools.combinations(PLACES, n)]

p = Project("inherited_settings", bases=())
p.inputs()
for i, (setting, places) in enumerate(todo):
    base = S.PlotManager.MakeBasePlot("2d")
    base.SetName(f"base{i}")
    plot = S.Plot(f"c{i}", "inherited", f"base{i}")
    data = add_data(plot[1], "hist2d")
    if setting == "view":
        data.SetDrawOption(S.lego)
    for place in places:
        target = {"pad": plot[1], "base pad": base[1], "pad 0": plot[0], "base pad 0": base[0]}[place]
        apply(target, setting, value(setting, place))
    p.pm.AddBasePlot(base)
    p.pm.AddPlot(plot)
# an empty cell is no panel: the ranges of pad 0 make no frame there
plot = S.Plot("empty_cell", S.PanelLayout([[S.Panel(), S.Empty(1, 1)]]))
plot.SetGroup("inherited")
add_data(plot[1], "hist")
plot[2].AddText(0.5, 0.5, "text")
plot[0]["X"].SetRange(2.0, 8.0)
plot[0]["Y"].SetRange(0.0, 20.0)
p.pm.AddPlot(plot)
# a pad set up in one plot made from a base plot stays out of the other plots made from it
p.pm.AddBasePlot(S.PlotManager.MakeBasePlot("1d_2x1"))
for name in ("extra_pad", "base_only"):
    plot = S.Plot(name, "inherited", "1d_2x1")
    for padID in (1, 2):
        add_data(plot[padID], "hist")
    if name == "extra_pad":
        plot[3].AddText(0.5, 0.5, "text").SetPosition(0.5, 0.5)
        plot[3].SetPosition(0.6, 0.6, 0.9, 0.9)
    p.pm.AddPlot(plot)
run = p.plot("inherited")
check("the plot run wrote its file", run.file, f"exit status {run.rc}: {run.log[-300:]}")
errors = run.errors()
check("no errors", not errors, " | ".join(errors[:5]))


def found(name, setting):
    pad = run.pad(name, "inherited")
    if not pad:
        return "not made"
    frame = run.frame_hist(name, "inherited")
    if setting == "view":
        return (round(pad.GetTheta(), 6), round(pad.GetPhi(), 6))
    if setting == "reference function":
        funcs = [o.GetExpFormula().Data() for o in pad.GetListOfPrimitives() if o.InheritsFrom("TF1") and o.GetName().startswith("0:")]
        return funcs[0] if len(funcs) == 1 else (None if not funcs else funcs)
    if setting == "log":
        return bool(pad.GetLogz())
    if setting == "title":
        return frame.GetXaxis().GetTitle()
    if setting == "range":
        return (round(run.frame(name, "inherited")["X"][0], 9), round(run.frame(name, "inherited")["X"][1], 9))
    if setting == "frame fill":
        return pad.GetFrameFillColor()
    if setting == "palette":
        setup = pad.GetPrimitive("setup_pad_1")
        text = setup.GetTitle() if setup else ""
        return next((v for v in (S.kViridis, S.kCividis, S.kBird, S.kInvertedDarkBodyRadiator) if f"SetPalette({int(v)}" in text), None)
    return None


for i, (setting, places) in enumerate(todo):
    got = found(f"c{i}", setting)
    if places:
        want = value(setting, places[0])
        check(f"{setting} from the {places[0]} when set in {', '.join(places)}", got == want, f"{got}, expected {want}")
    elif setting in DEFAULTS:
        check(f"{setting} as ROOT's default when set nowhere", got == DEFAULTS[setting], f"{got}, expected {DEFAULTS[setting]}")
pad = run.pad("empty_cell", "inherited", 2)
check("an empty cell takes no frame from the ranges of pad 0", pad is not None and not any(o.InheritsFrom("TH1") for o in pad.GetListOfPrimitives()), str([o.GetName() for o in pad.GetListOfPrimitives()]) if pad else "no pad")
canvas = run.canvas("base_only", "inherited")
pads = sorted(o.GetName() for o in canvas.GetListOfPrimitives() if o.InheritsFrom("TPad")) if canvas else []
check("a pad added in one plot does not reach another plot of the same base plot", pads == ["Pad_1", "Pad_2"], str(pads))
finish()
