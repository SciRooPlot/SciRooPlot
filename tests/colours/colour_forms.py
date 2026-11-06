"""Every colour setter takes a colour in each of its forms, a ROOT index, "#rrggbb" (any case) and (r, g, b) from 0 to 1, and the drawn
object gets exactly that colour (rgb colours are stored in the plot definitions as "#rrggbb"), with the transparency of the alpha given
with it (the alpha setter or the alpha of a combined setter); colour forms that are not valid raise when they are set.
Covers: f943c45 (rgb colours in every colour setter).
"""

import itertools

from srptest import ROOT, Project, S, add_data, cases, check, colour_diff, describe, finish

FORMS = {"index": ROOT.kRed + 2, "hex": "#3a7bd5", "hex_upper": "#C0FFEE", "rgb": (0.2, 0.6, 0.4), "rgb_ints": (1, 0, 0)}


def data(pl, d):
    return d


def first(run, name):
    drawn = run.drawn(name, "forms")
    return drawn[0][0] if drawn else None


def prim(cls):
    """the first primitive of a class in pad 1"""
    return lambda run, name: next((obj for obj, _ in run.links(name, "forms") if obj.ClassName() == cls), None)


def legend_row(label):
    def find(run, name):
        legend = prim("TLegend")(run, name)
        return next((e for e in legend.GetListOfPrimitives() if e.GetLabel() == label), None) if legend else None

    return find


def axis(run, name):
    frame = run.frame_hist(name, "forms")
    return frame.GetXaxis() if frame else None


def canvas(run, name):
    return run.canvas(name, "forms")


def pad(run, name):
    return run.pad(name, "forms")


# what is read: (object, getter of the colour index)
READ = {
    "data_marker": (first, "GetMarkerColor"),
    "data_line": (first, "GetLineColor"),
    "data_fill": (first, "GetFillColor"),
    "canvas_fill": (canvas, "GetFillColor"),
    "pad_fill": (pad, "GetFillColor"),
    "frame_fill": (pad, "GetFrameFillColor"),
    "frame_border": (pad, "GetFrameLineColor"),
    "axis_line": (axis, "GetAxisColor"),
    "axis_title": (axis, "GetTitleColor"),
    "axis_labels": (axis, "GetLabelColor"),
    "text_text": (prim("TPaveText"), "GetTextColor"),
    "text_border": (prim("TPaveText"), "GetLineColor"),
    "text_fill": (prim("TPaveText"), "GetFillColor"),
    "legend_text": (prim("TLegend"), "GetTextColor"),
    "legend_border": (prim("TLegend"), "GetLineColor"),
    "legend_fill": (prim("TLegend"), "GetFillColor"),
    "row_marker": (legend_row("data"), "GetMarkerColor"),
    "row_line": (legend_row("data"), "GetLineColor"),
    "row_fill": (legend_row("data"), "GetFillColor"),
    "added_marker": (legend_row("added"), "GetMarkerColor"),
    "added_line": (legend_row("added"), "GetLineColor"),
    "added_fill": (legend_row("added"), "GetFillColor"),
    "added_text": (legend_row("added"), "GetTextColor"),
    "shape_line": (prim("TLine"), "GetLineColor"),
    "shape_fill": (prim("TBox"), "GetFillColor"),
}
DATA = ("data_marker", "data_line", "data_fill")


def text(pl, d):
    return pl[1].AddText(0.3, 0.8, "text")


def legend(pl, d):
    box = pl[1].AddLegend(0.6, 0.8)
    box.AddEntry("added")
    return box


def added(pl, d):
    return pl[1].AddLegend(0.6, 0.8).AddEntry("added")


def separate(*setters):
    """a colour setter and its alpha setter"""
    return lambda obj, color, alpha: [getattr(obj, setters[0])(color)] + ([getattr(obj, s)(alpha) for s in setters[1:]] if alpha is not None else [])


# setters: (object made in a plot, call with colour and alpha, what it colours)
SETTERS = {
    "Data.SetColor": (data, separate("SetColor", "SetAlpha"), DATA),
    "Data.SetMarkerColor": (data, separate("SetMarkerColor", "SetMarkerAlpha"), ("data_marker",)),
    "Data.SetMarker": (data, lambda d, c, a: d.SetMarker(ROOT.kFullCircle, 1.2, c, 1.0 if a is None else a), ("data_marker",)),
    "Data.SetLineColor": (data, separate("SetLineColor", "SetLineAlpha"), ("data_line",)),
    "Data.SetLine": (data, lambda d, c, a: d.SetLine(ROOT.kSolid, 2.0, c, 1.0 if a is None else a), ("data_line",)),
    "Data.SetFillColor": (data, separate("SetFillColor", "SetFillAlpha"), ("data_fill",)),
    "Data.SetFill": (data, lambda d, c, a: d.SetFill(1001, c, 1.0 if a is None else a), ("data_fill",)),
    "Plot.SetFillColor": (lambda pl, d: pl, separate("SetFillColor", "SetFillAlpha"), ("canvas_fill",)),
    "Plot.SetFill": (lambda pl, d: pl, lambda p, c, a: p.SetFill(1001, c, 1.0 if a is None else a), ("canvas_fill",)),
    "Pad.SetFillColor": (lambda pl, d: pl[1], separate("SetFillColor", "SetFillAlpha"), ("pad_fill",)),
    "Pad.SetFill": (lambda pl, d: pl[1], lambda p, c, a: p.SetFill(1001, c, 1.0 if a is None else a), ("pad_fill",)),
    "Pad.SetFrameFillColor": (lambda pl, d: pl[1], separate("SetFrameFillColor", "SetFrameFillAlpha"), ("frame_fill",)),
    "Pad.SetFrameFill": (lambda pl, d: pl[1], lambda p, c, a: p.SetFrameFill(1001, c, 1.0 if a is None else a), ("frame_fill",)),
    "Pad.SetFrameBorderColor": (lambda pl, d: pl[1], separate("SetFrameBorderColor", "SetFrameBorderAlpha"), ("frame_border",)),
    "Pad.SetFrameBorder": (lambda pl, d: pl[1], lambda p, c, a: p.SetFrameBorder(ROOT.kSolid, 2.0, c, 1.0 if a is None else a), ("frame_border",)),
    "Pad.SetDefaultTextColor": (lambda pl, d: (text(pl, d), pl[1])[1], separate("SetDefaultTextColor", "SetDefaultTextAlpha"), ("text_text", "axis_title", "axis_labels")),
    "Pad.SetDefaultColors": (lambda pl, d: pl[1], lambda p, c, a: (p.SetDefaultColors([c]), a is not None and p.SetDefaultAlpha(a)), DATA),
    "Pad.SetDefaultMarkerColors": (lambda pl, d: pl[1], lambda p, c, a: (p.SetDefaultMarkerColors([c]), a is not None and p.SetDefaultMarkerAlpha(a)), ("data_marker",)),
    "Pad.SetDefaultLineColors": (lambda pl, d: pl[1], lambda p, c, a: (p.SetDefaultLineColors([c]), a is not None and p.SetDefaultLineAlpha(a)), ("data_line",)),
    "Pad.SetDefaultFillColors": (lambda pl, d: pl[1], lambda p, c, a: (p.SetDefaultFillColors([c]), a is not None and p.SetDefaultFillAlpha(a)), ("data_fill",)),
    "Axis.SetColor": (lambda pl, d: pl[1]["X"], separate("SetColor", "SetAxisAlpha", "SetTitleAlpha", "SetLabelAlpha"), ("axis_line", "axis_title", "axis_labels")),
    "Axis.SetAxisColor": (lambda pl, d: pl[1]["X"], separate("SetAxisColor", "SetAxisAlpha"), ("axis_line",)),
    "Axis.SetTitleColor": (lambda pl, d: pl[1]["X"], separate("SetTitleColor", "SetTitleAlpha"), ("axis_title",)),
    "Axis.SetLabelColor": (lambda pl, d: pl[1]["X"], separate("SetLabelColor", "SetLabelAlpha"), ("axis_labels",)),
    "TextBox.SetTextColor": (text, separate("SetTextColor", "SetTextAlpha"), ("text_text",)),
    "TextBox.SetText": (text, lambda b, c, a: b.SetText(42, 0.04, c, 1.0 if a is None else a), ("text_text",)),
    "TextBox.SetBorderColor": (text, separate("SetBorderColor", "SetBorderAlpha"), ("text_border",)),
    "TextBox.SetBorder": (text, lambda b, c, a: b.SetBorder(ROOT.kSolid, 2.0, c, 1.0 if a is None else a), ("text_border",)),
    "TextBox.SetFillColor": (text, separate("SetFillColor", "SetFillAlpha"), ("text_fill",)),
    "TextBox.SetFill": (text, lambda b, c, a: b.SetFill(1001, c, 1.0 if a is None else a), ("text_fill",)),
    "LegendBox.SetTextColor": (legend, separate("SetTextColor", "SetTextAlpha"), ("legend_text",)),
    "LegendBox.SetBorderColor": (legend, separate("SetBorderColor", "SetBorderAlpha"), ("legend_border",)),
    "LegendBox.SetFillColor": (legend, separate("SetFillColor", "SetFillAlpha"), ("legend_fill",)),
    "LegendBox.SetDefaultColor": (legend, separate("SetDefaultColor", "SetDefaultAlpha"), ("added_marker", "added_line", "added_fill")),
    "LegendBox.SetDefaultMarkerColor": (legend, separate("SetDefaultMarkerColor", "SetDefaultMarkerAlpha"), ("added_marker",)),
    "LegendBox.SetDefaultLineColor": (legend, separate("SetDefaultLineColor", "SetDefaultLineAlpha"), ("added_line",)),
    "LegendBox.SetDefaultFillColor": (legend, separate("SetDefaultFillColor", "SetDefaultFillAlpha"), ("added_fill",)),
    "LegendEntry.SetColor": (added, separate("SetColor", "SetAlpha"), ("added_marker", "added_line", "added_fill")),
    "LegendEntry.SetMarkerColor": (added, separate("SetMarkerColor", "SetMarkerAlpha"), ("added_marker",)),
    "LegendEntry.SetLineColor": (added, separate("SetLineColor", "SetLineAlpha"), ("added_line",)),
    "LegendEntry.SetFillColor": (added, separate("SetFillColor", "SetFillAlpha"), ("added_fill",)),
    "LegendEntry.SetTextColor": (added, separate("SetTextColor", "SetTextAlpha"), ("added_text",)),
    "Data.GetLegendEntry.SetColor": (lambda pl, d: (legend(pl, d), d.GetLegendEntry())[1], separate("SetColor", "SetAlpha"), ("row_marker", "row_line", "row_fill")),
    "Shape.SetLineColor": (lambda pl, d: pl[1].AddLine((2, 2), (5, 5)), separate("SetLineColor", "SetLineAlpha"), ("shape_line",)),
    "Shape.SetFillColor": (lambda pl, d: pl[1].AddRegion((2, 2), (5, 5)), separate("SetFillColor", "SetFillAlpha"), ("shape_fill",)),
}

p = Project("colour_forms")
p.inputs()
todo = cases(
    {"setter": list(SETTERS), "form": list(FORMS), "alpha": [None, 0.4]},
    named=[{"setter": "Data.SetColor", "form": "rgb", "alpha": None, "why": "f943c45"}],
)
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "forms", "1d")
    make, call, _ = SETTERS[case["setter"]]
    obj = make(plot, add_data(plot[1], "hist").SetLegendLabel("data"))
    call(obj, FORMS[case["form"]], case["alpha"])
    p.pm.AddPlot(plot)
run = p.plot("forms")

for i, case in enumerate(todo):
    what = describe(case)
    for target in SETTERS[case["setter"]][2]:
        find, getter = READ[target]
        obj = find(run, f"c{i}")
        if not obj:
            check(f"{target} drawn: {what}", False, " | ".join(run.errors()[:2]))
            continue
        found = colour_diff(FORMS[case["form"]], getattr(obj, getter)(), case["alpha"])
        check(f"{target}: {what}", not found, found)

# forms that are not colours raise when set (also inside lists and gradients)
for bad, setter in itertools.product(("#12345", "#12345g", "red", "", (1.5, 0.0, 0.0), (-0.1, 0.2, 0.3), (0.0, 0.0), (0.1, 0.2, 0.3, 0.4), True, None, 2.5, 70000), ("SetColor", "SetDefaultColors", "gradient")):
    pl = S.Plot("bad", "bad", "1d")
    try:
        if setter == "SetColor":
            pl[1].AddData("a", "b").SetColor(bad)
        elif setter == "SetDefaultColors":
            pl[1].SetDefaultColors([ROOT.kRed, bad])
        else:
            pl[1].SetPalette([(ROOT.kRed, 0.0), (bad, 1.0)])
        raised = ""
    except (ValueError, TypeError) as e:
        raised = type(e).__name__
    check(f"{setter} with {bad!r} raises", bool(raised), "accepted")
check("no warnings", not run.warnings(), " | ".join(run.warnings()[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
