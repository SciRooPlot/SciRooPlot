"""Plots made from a PanelLayout look the same in every pad, whatever its size and whatever is drawn in it: texts, axis labels and axis
titles have the pixel sizes of the layout (31.52 and 39.4 by default, or those given to MakeBasePlot or the PanelLayout), as ROOT reads
them (fractions of the smaller pad side, pixel fonts in pixels), in font 42; drawn 1d data has the line width and marker size of the layout;
a colour scale is the colour-scale offset away from its frame and the colour-scale width wide (in its panel and in a ColorScale() cell);
ratio and pull panels of base plots draw their reference line (at 1 and 0) with few divisions and a centred title.
The labels and title of an axis are hidden only where a touching panel shares that axis (towards the panel below for x, the panel on
the left for y); a 3D view or a polar drawing shares no x and y axes, so it keeps its labels and its neighbours keep theirs towards it.
Every data kind is drawn in every panel of a base plot of its dimension (1d: a grid with ratio and pull panels, 2d: a grid with one
colour scale and a gap grid with own scales, 3d: a touching grid), in the map-with-projections layout and in layouts with spanning panels.
Covers: 1bcc1d8 (sizes given to MakeBasePlot), a71579b (colour scales in pixels), 7476c1b (pull panels), the commits of this file's first
version and of its fix "labels between touching panels hidden only between flat frames".
"""

from srptest import DATA_3D, DATA_KINDS, DATA_POLAR, ROOT, Project, S, add_data, cases, check, describe, finish

SIZES = {  # text, title, colour-scale offset and width, line width, marker size
    "default": (31.52, 39.4, 5.12, 46.08, 5, 1.4),
    "given": (24.0, 30.0, 8.0, 30.0, 3, 2.0),
}
LAYOUTS = {  # by dimension of the data: layouts and the pads with data (a ColorScale() cell has none)
    1: {"1d_2x1_ratio_pull": (1, 2, 3, 4, 5, 6), "tall_span": (1, 2, 3), "wide_span": (1, 2, 3)},
    2: {"2d_2x2": (1, 2, 4, 5), "2d_2x1_gap": (1, 2), "projections": (2,)},
    3: {"2d_2x1": (1, 2), "projections": (2,)},
}
OWN = {  # own layouts (a71579b): a panel spanning two rows beside two after a gap, a panel spanning two columns above two after a gap row
    "tall_span": [[S.Panel().Span(1, 2), S.Gap(), S.Panel(1, 0.62)], [S.Panel(1, 0.38)]],
    "wide_span": [[S.Panel(1, 0.75).Span(2)], [S.Gap()], [S.Panel(), S.Panel()]],
}
REFERENCE = {"1d_2x1_ratio_pull": {3: 1.0, 4: 1.0, 5: 0.0, 6: 0.0}}  # reference lines of the ratio and pull pads


def layouts(kind):
    dim = DATA_KINDS[kind][0]
    return list(LAYOUTS[dim]) + (["projections"] if dim == 1 else [])  # 1d data as the projections of a map


todo = cases(
    {"kind": list(DATA_KINDS), "layout": sorted({name for d in LAYOUTS.values() for name in d}), "sizes": list(SIZES)},
    valid=lambda c: c["layout"] in layouts(c["kind"]),
    always=lambda c: c["sizes"] == "default",  # every kind in every layout
)


def view(kind):
    """drawn without flat frame: a 3D view or a polar grid"""
    return kind in DATA_3D or kind in DATA_POLAR


p = Project("pad_look", bases=())
p.inputs()
for name in ("1d_2x1_ratio_pull", "2d_2x2", "2d_2x1_gap", "2d_2x1"):
    for sizes, (text, title, offset, width, line, marker) in SIZES.items():
        given = S.PanelLayout()
        if sizes != "default":
            given.SetTextSize(text).SetTitleSize(title).SetColorScaleOffset(offset).SetColorScaleWidth(width).SetLineWidth(line).SetMarkerSize(marker)
        base = S.PlotManager.MakeBasePlot(name, given)
        base.SetName(f"{name}_{sizes}")
        p.pm.AddBasePlot(base)
withKind = {}  # plot: the pads drawing the data kind of its case
for i, case in enumerate(todo):
    text, title, offset, width, line, marker = SIZES[case["sizes"]]
    dim = DATA_KINDS[case["kind"]][0]
    if case["layout"] == "projections":  # a map with projections above and right, the colour scale of the map after them
        given = S.PanelLayout().SetTextSize(text).SetTitleSize(title).SetColorScaleOffset(offset).SetColorScaleWidth(width).SetLineWidth(line).SetMarkerSize(marker)
        plot = S.Plot(f"c{i}", S.PanelLayout(given, [[S.Panel(1, 0.37), S.Hole().Span(2)], [S.Panel(), S.Panel(0.37, 1), S.ColorScale(2)]]))
        plot.SetGroup("look")
        withKind[i] = (1, 3) if dim == 1 else (2,)
        if dim == 1:
            plot[2].AddData("hist2d", "inputs")
        else:
            plot[1].AddData("hist", "inputs")
            plot[3].AddData("hist", "inputs").SetDrawOption(S.hbars)
        pads = (1, 2, 3)
    elif case["layout"] in OWN:
        given = S.PanelLayout().SetTextSize(text).SetTitleSize(title).SetColorScaleOffset(offset).SetColorScaleWidth(width).SetLineWidth(line).SetMarkerSize(marker)
        plot = S.Plot(f"c{i}", S.PanelLayout(given, OWN[case["layout"]]))
        plot.SetGroup("look")
        pads = withKind[i] = LAYOUTS[dim][case["layout"]]
    else:
        plot = S.Plot(f"c{i}", "look", f"{case['layout']}_{case['sizes']}")
        pads = withKind[i] = LAYOUTS[dim][case["layout"]]
    for padID in pads:
        if padID in withKind[i]:
            add_data(plot[padID], case["kind"], "inputs")
        plot[padID].AddText(0.4, 0.6, "text")
        for axis in "XYZ":
            plot[padID][axis].SetTitle(f"{axis.lower()} title")
    p.pm.AddPlot(plot)
# sizes set for the axis between touching panels: the pad's own (also from its pad of the base plot) win over the hiding, those of pad 0 do not
OWN_SIZES = {  # name: (what pad 1 draws, where the sizes are set, label size, title size; expected y label size and title shown of pad 2)
    "own label size": ("hist", "pad", 0.06, None, (0.06, False)),
    "own title size": ("hist", "pad", None, 0.07, (0.0, True)),
    "sizes of pad 0": ("hist", "pad 0", 0.06, 0.07, (0.0, False)),
    "own sizes 0 beside a 3D view": ("lego", "pad", 0.0, 0.0, (0.0, False)),
}
for name, (kind, place, label, title, _) in OWN_SIZES.items():
    plot = S.Plot(name.replace(" ", "_"), "look", "1d_2x1_ratio_pull_default" if kind == "hist" else "2d_2x1_default")
    add_data(plot[1], "hist" if kind == "hist" else "hist2d", "inputs").SetDrawOption(S.lego if kind == "lego" else S.points)
    add_data(plot[2], "hist" if kind == "hist" else "hist2d", "inputs")
    axis = plot[2 if place == "pad" else 0]["Y"]
    if label is not None:
        axis.SetLabelSize(label)
    if title is not None:
        axis.SetTitleSize(title)
    plot[2]["Y"].SetTitle("y title")
    p.pm.AddPlot(plot)
run = p.plot("look")
check("the plot run wrote its file", run.file, f"exit status {run.rc}: {run.log[-300:]}")
errors = run.errors()
check("no errors", not errors, " | ".join(errors[:5]))


def frame_rect(pad, W, H):
    """left, right, top, bottom of the frame in canvas pixels (y from the top)"""
    x0, y1 = pad.GetXlowNDC() * W, (1 - pad.GetYlowNDC()) * H
    w, h = pad.GetWNDC() * W, pad.GetHNDC() * H
    return (x0 + w * pad.GetLeftMargin(), x0 + w * (1 - pad.GetRightMargin()), y1 - h * (1 - pad.GetTopMargin()), y1 - h * pad.GetBottomMargin())


def pixels(size, font, side):
    return size if font % 10 == 3 else size * side


for i, case in enumerate(todo):
    what = describe(case)
    text, title, offset, width, line, marker = SIZES[case["sizes"]]
    canvas = run.canvas(f"c{i}", "look")
    if not canvas:
        check(f"plot made: {what}", False, " | ".join(line for line in run.errors() if f"c{i} " in line)[:300])
        continue
    W, H = canvas.GetWw(), canvas.GetWh()
    pads = {int(o.GetName()[4:]): o for o in canvas.GetListOfPrimitives() if o.InheritsFrom("TPad") and o.GetName().startswith("Pad_")}
    kinds = {padID: case["kind"] if padID in withKind[i] else "other" for padID in pads}  # what each pad draws
    frames = {padID: frame_rect(pad, W, H) for padID, pad in pads.items() if pad.GetPrimitive(f"axis_hist_pad_{padID}") or kinds[padID] in DATA_POLAR}
    for padID, pad in pads.items():
        side = min(pad.GetWNDC() * W, pad.GetHNDC() * H)
        boxes = [b for b in pad.GetListOfPrimitives() if b.InheritsFrom("TPaveText")]
        if boxes:
            got = [(round(pixels(b.GetTextSize(), b.GetTextFont(), side), 2), b.GetTextFont()) for b in boxes]
            check(f"text {text} px in font 42 in pad {padID}: {what}", all(abs(s - text) < 0.01 and f == 42 for s, f in got), str(got))
        frame = pad.GetPrimitive(f"axis_hist_pad_{padID}")
        if frame and padID in frames:
            left, right, top, bottom = frames[padID]
            ownView = view(case["kind"]) and kinds[padID] == case["kind"]
            # the touching panel below (sharing x) and on the left (sharing y), unless one of the two is drawn without flat frame
            below = [o for o, (l2, r2, t2, b2) in frames.items() if o != padID and abs(t2 - bottom) < 0.01 and min(right, r2) - max(left, l2) > 1]
            onLeft = [o for o, (l2, r2, t2, b2) in frames.items() if o != padID and abs(r2 - left) < 0.01 and min(bottom, b2) - max(top, t2) > 1]
            flat = lambda o: not (view(case["kind"]) and kinds[o] == case["kind"])  # noqa: E731
            hideX = not ownView and any(flat(o) for o in below)
            hideY = not ownView and any(flat(o) for o in onLeft)
            hbars = case["layout"] == "projections" and padID == 3 and DATA_KINDS[case["kind"]][0] > 1  # the histogram's x axis is vertical
            horizontal, vertical = (frame.GetYaxis(), frame.GetXaxis()) if hbars else (frame.GetXaxis(), frame.GetYaxis())
            for axisName, axis, hidden in (("x", horizontal, hideX), ("y", vertical, hideY)):
                label = pixels(axis.GetLabelSize(), axis.GetLabelFont(), side)
                titleSize = pixels(axis.GetTitleSize(), axis.GetTitleFont(), side)
                if hidden:
                    check(f"{axisName} labels and title hidden towards the touching panel in pad {padID}: {what}", label == 0 and titleSize == 0, f"label {label:.2f} px, title {titleSize:.2f} px")
                else:
                    check(f"{axisName} labels {text} px and title {title} px in pad {padID}: {what}", abs(label - text) < 0.01 and abs(titleSize - title) < 0.01, f"label {label:.2f} px, title {titleSize:.2f} px")
            palette = frame.GetListOfFunctions().FindObject("palette")
            if palette and DATA_KINDS[case["kind"]][0] > 1 and kinds[padID] == case["kind"]:
                padWidth = pad.GetWNDC() * W
                got = ((palette.GetX1NDC() * padWidth + pad.GetXlowNDC() * W) - right, (palette.GetX2NDC() - palette.GetX1NDC()) * padWidth)
                check(f"colour scale {offset} px from the frame and {width} px wide in pad {padID}: {what}", abs(got[0] - offset) < 0.05 and abs(got[1] - width) < 0.05, f"{got[0]:.2f} px from the frame, {got[1]:.2f} px wide")
        reference = REFERENCE.get(case["layout"], {}).get(padID)
        scale = pad.GetPrimitive(f"color_scale_pad_{padID}")
        palette = scale.GetListOfFunctions().FindObject("palette") if scale else None
        if scale:
            padWidth = pad.GetWNDC() * W
            got = (palette.GetX1NDC() * padWidth, (palette.GetX2NDC() - palette.GetX1NDC()) * padWidth) if palette else (None, None)
            check(f"colour scale of the cell {offset} px from the frames and {width} px wide in pad {padID}: {what}", palette and abs(got[0] - offset) < 0.05 and abs(got[1] - width) < 0.05, f"{got}")
        for obj in run.objects(f"c{i}", "look", padID)[1 if reference is not None else 0 :]:  # without the reference line (data 0)
            if obj.InheritsFrom("TAttLine") and not obj.InheritsFrom("TH2") and not obj.InheritsFrom("TH3") and DATA_KINDS[case["kind"]][0] == 1:
                check(f"line width {line} of the data in pad {padID}: {what}", obj.GetLineWidth() == line, f"{obj.GetName()}: {obj.GetLineWidth()}")
            if obj.InheritsFrom("TAttMarker") and DATA_KINDS[case["kind"]][0] == 1:
                check(f"marker size {marker} of the data in pad {padID}: {what}", abs(obj.GetMarkerSize() - marker) < 1e-6, f"{obj.GetName()}: {obj.GetMarkerSize()}")
        if reference is not None:
            funcs = [o for o in pad.GetListOfPrimitives() if o.InheritsFrom("TF1") and o.GetName().startswith("0:")]
            check(f"reference line at {reference} in pad {padID}: {what}", len(funcs) == 1 and funcs[0].Eval(5.0) == reference, str([f.GetExpFormula().Data() for f in funcs]))
            if frame:
                axis = frame.GetYaxis()
                check(f"few y divisions and a centred title in pad {padID}: {what}", axis.GetNdivisions() % 1000000 == 305 and axis.TestBit(ROOT.TAxis.kCenterTitle), f"divisions {axis.GetNdivisions()}, centred {axis.TestBit(ROOT.TAxis.kCenterTitle)}")
for name, (kind, place, label, title, want) in OWN_SIZES.items():
    frame = run.frame_hist(name.replace(" ", "_"), "look", 2)
    got = (round(frame.GetYaxis().GetLabelSize(), 6), frame.GetYaxis().GetTitleSize() > 0) if frame else None  # (a y title gets a pixel size)
    check(f"y labels and title of the pad beside a touching panel: {name}", got == want, f"{got}, expected {want}")
finish()
