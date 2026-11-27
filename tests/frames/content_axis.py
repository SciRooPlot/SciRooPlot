"""The 'C' axis of a pad is the content of a 3d histogram (its z axis is binned): a range on it (both ends, or one) is the minimum and maximum
of the frame and of the data, with the colour levels spread over it; its title is the title of the colour scale; a log scale on it is
warned about (ROOT colours the boxes linearly) and not applied; with a log scale on the binned z axis the colour scale stays linear, with
its colours; the settings of 'Z' (also its divisions) stay on the binned z axis and do not reach the colour scale, the divisions of 'C'
are those of the colour scale; the colour scale takes the text colour and alpha of the pad; a 'C' axis in a pad without a 3d histogram
is warned about. The look set on 'C' (colours, alphas, ticks) and colour levels given for the data are also those of the scale in a
ColorScale() cell. A 3d histogram of one value in every bin gets the range widened by 10 % of it, with the colour levels spread over it.
Cases: every 3d histogram kind × a range on 'C' (both ends, minimum, maximum), a title on 'C', a log 'C', a log 'Z', a range and title on 'Z',
divisions on 'C' and 'Z', and the named cases.
Reference: the frame, data and colour scale read back (minimum, maximum, colour levels, the colour scale's axis title and divisions, the
z axis: the bins holding the range and its divisions).
Covers: the content axis of 3d histograms.
"""

from srptest import DATA_3D, DATA_KINDS, ROOT, Project, S, add_data, cases, check, close, data_objects, describe, finish, rgba

KINDS = tuple(k for k in DATA_3D if "binned" in DATA_KINDS[k][2])
SETTINGS = {"range": (5.0, 400.0), "min_only": (50.0, None), "max_only": (None, 300.0), "title": None, "log": None, "log_z": None, "z_axis": None, "pad_text": None, "divisions": None}
todo = cases({"kind": list(KINDS), "setting": list(SETTINGS)}, always=lambda c: True)

p = Project("content_axis")
p.inputs()
inputs = data_objects()
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "content", "2d")
    pad = plot[1]
    add_data(pad, case["kind"]).SetDrawOption(S.colz)
    setting = case["setting"]
    if setting in ("range", "min_only", "max_only"):
        lo, hi = SETTINGS[setting]
        if lo is not None and hi is not None:
            pad["C"].SetRange(lo, hi)
        elif lo is not None:
            pad["C"].SetMinRange(lo)
        else:
            pad["C"].SetMaxRange(hi)
    elif setting == "title":
        pad["C"].SetTitle("content title")
    elif setting == "log":
        pad["C"].SetLog()
    elif setting == "log_z":
        pad["Z"].SetLog()
    elif setting == "pad_text":
        pad.SetDefaultTextColor(S.kRed).SetDefaultTextAlpha(0.3)
    elif setting == "divisions":
        pad["C"].SetNumDivisions(4)
        pad["Z"].SetNumDivisions(503)
    else:
        pad["Z"].SetRange(2.0, 8.0).SetTitle("binned z").SetLabelSize(0.06)
    p.pm.AddPlot(plot)
plot = S.Plot("no_th3", "named", "2d")
plot[1].AddData("hist2d", "inputs")
plot[1]["C"].SetRange(1.0, 2.0)
p.pm.AddPlot(plot)

# the look of 'C' in a ColorScale() cell
plot = S.Plot("look_cell", S.PanelLayout([[S.Panel(), S.ColorScale(1)]]))
plot.SetGroup("named")
add_data(plot[1], KINDS[0]).SetDrawOption(S.colz)
plot[1]["C"].SetLabelColor(S.kRed).SetLabelAlpha(0.2).SetAxisColor(S.kBlue).SetAxisAlpha(0.4).SetTickLength(0.05)
p.pm.AddPlot(plot)
# the look set on 'Z' (the binned z axis) does not reach the scale in a ColorScale() cell: it is that of a cell without it
for name, zLook in (("z_look_cell", True), ("plain_cell", False)):
    plot = S.Plot(name, S.PanelLayout([[S.Panel(), S.ColorScale(1)]]))
    plot.SetGroup("named")
    add_data(plot[1], KINDS[0]).SetDrawOption(S.colz)
    if zLook:
        plot[1]["Z"].SetLabelColor(S.kRed).SetTitleColor(S.kBlue).SetAxisColor(S.kGreen).SetTickLength(0.11).SetNumDivisions(203)
    p.pm.AddPlot(plot)
# colour levels given for the 3d histogram: the scale in a ColorScale() cell has the same levels
LEVELS = [10.0, 50.0, 100.0, 300.0]
plot = S.Plot("levels_cell", S.PanelLayout([[S.Panel(), S.ColorScale(1)]]))
plot.SetGroup("named")
add_data(plot[1], KINDS[0]).SetDrawOption(S.colz).SetContours(LEVELS)
p.pm.AddPlot(plot)

# a 3d histogram of one value in every bin: its range is widened by 10 % of the value, the colour levels spread over the widened range
flat = ROOT.TH3D("flat3", "", 3, 0.0, 3.0, 3, 0.0, 3.0, 3, 0.0, 3.0)
for x in range(1, 4):
    for y in range(1, 4):
        for z in range(1, 4):
            flat.SetBinContent(x, y, z, 5.0)
p.source("flat", {"flat3": flat})
plot = S.Plot("flat", "named", "2d")
plot[1].AddData("flat3", "flat")
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


cell = run.pad("look_cell", "named", 2)
scale = cell.GetPrimitive("color_scale_pad_2") if cell else None
palette = scale.GetListOfFunctions().FindObject("palette") if scale else None
look = (rgba(palette.GetAxis().GetLabelColor()), rgba(palette.GetAxis().GetLineColor()), round(palette.GetAxis().GetTickSize(), 4)) if palette else None
check("the look of 'C' in a ColorScale() cell: label and axis colours with their alphas, tick length", look == (rgba(632, 0.2), rgba(600, 0.4), 0.05), str(look))


def cell_look(name):
    cellPad = run.pad(name, "named", 2)
    cellScale = cellPad.GetPrimitive("color_scale_pad_2") if cellPad else None
    if not cellScale:
        return None
    z = cellScale.GetZaxis()
    return (rgba(z.GetLabelColor()), rgba(z.GetTitleColor()), rgba(z.GetAxisColor()), round(z.GetTickLength(), 4), z.GetNdivisions() % 1000000)


zLookCell, plainCell = cell_look("z_look_cell"), cell_look("plain_cell")
check("the look of 'Z' does not reach the scale in a ColorScale() cell", zLookCell is not None and zLookCell == plainCell, f"{zLookCell}, without it {plainCell}")
levelsCell = run.pad("levels_cell", "named", 2)
levelsScale = levelsCell.GetPrimitive("color_scale_pad_2") if levelsCell else None
cellLevels = [levelsScale.GetContourLevel(k) for k in range(levelsScale.GetContour())] if levelsScale else None
check("colour levels given for a 3d histogram are those of the scale in a ColorScale() cell", cellLevels == LEVELS, f"{cellLevels}, expected {LEVELS}")

for i, case in enumerate(todo):
    what = describe(case)
    frame = run.frame_hist(f"c{i}", "content")
    drawn = run.objects(f"c{i}", "content")
    if not frame or not drawn:
        check(f"{what}: drawn", False, " | ".join(run.messages(f"c{i}", "content")))
        continue
    data = drawn[0]
    setting = case["setting"]
    padObj = run.pad(f"c{i}", "content")
    scaleHist = padObj.GetPrimitive("content_scale_pad_1") if padObj else None
    palette = scaleHist.GetListOfFunctions().FindObject("palette") if scaleHist else None
    if setting != "log":
        check(f"{what}: the colour scale is drawn, the frame has none", bool(palette) and not frame.GetListOfFunctions().FindObject("palette"), f"scale {bool(palette)}")
    if setting in ("range", "min_only", "max_only"):
        lo, hi = SETTINGS[setting]
        for label, obj in (("frame", frame), ("data", data)):
            ok = (lo is None or close(obj.GetMinimum(), lo, 1e-9)) and (hi is None or close(obj.GetMaximum(), hi, 1e-9))
            check(f"{what}: the range is the minimum and maximum of the {label}", ok, f"{obj.GetMinimum()} to {obj.GetMaximum()}")
        n = frame.GetContour()
        levels = [frame.GetContourLevel(k) for k in range(n)]
        ok = n > 1 and close(levels[0], frame.GetMinimum(), 1e-9) and all(b > a for a, b in zip(levels, levels[1:])) and levels[-1] < frame.GetMaximum()
        check(f"{what}: the colour levels spread from the minimum", ok, f"{n} levels from {levels[0] if levels else None} to {levels[-1] if levels else None}, frame {frame.GetMinimum()} to {frame.GetMaximum()}")
    elif setting == "title":
        title = palette.GetAxis().GetTitle() if palette else frame.GetZaxis().GetTitle()
        check(f"{what}: the title of the colour scale", "content title" in title and (palette is None or palette.GetAxis().GetTitle() == "content title"), f"{title!r}")
    elif setting == "log":
        check(f"{what}: a log scale warned about, not applied", not padObj.GetLogz() and any("The log scale of the C axis of pad 1 is ignored" in m for m in run.messages(f"c{i}", "content")), " | ".join(run.messages(f"c{i}", "content")))
    elif setting == "log_z":
        padObj.cd()
        linear = bool(palette) and padObj.GetLogz() and palette.GetLog() == 0
        n = scaleHist.GetContour() if scaleHist else 0
        levels = [scaleHist.GetContourLevel(k) for k in range(n)]
        spread = n > 1 and all(close(b - a, levels[1] - levels[0], 1e-6) for a, b in zip(levels, levels[1:]))
        check(f"{what}: with a log binned z axis the colour scale stays linear (levels spread linearly)", linear and spread, f"log {palette.GetLog() if palette else None}, levels {levels[:3]}")
    elif setting == "divisions":
        scaleDivisions = scaleHist.GetZaxis().GetNdivisions() % 1000000 if scaleHist else None
        check(f"{what}: the divisions of 'C' on the colour scale, those of 'Z' on the binned z axis", scaleDivisions == 4 and frame.GetZaxis().GetNdivisions() % 1000000 == 503, f"scale {scaleDivisions}, z {frame.GetZaxis().GetNdivisions() % 1000000}")
    elif setting == "pad_text":
        scale = palette.GetAxis() if palette else None  # (a TGaxis: its title in the text attributes)
        colours = {"labels": rgba(scale.GetLabelColor()), "title": rgba(scale.GetTextColor())} if scale else {}
        check(f"{what}: the colour scale in the text colour and alpha of the pad", bool(colours) and all(c == rgba(632, 0.3) for c in colours.values()), str(colours))
    else:
        z = frame.GetZaxis()
        shown = (z.GetBinLowEdge(z.GetFirst()), z.GetBinUpEdge(z.GetLast()))
        ref = inputs[case["kind"]].GetZaxis().Clone()  # the bins holding the range, as ROOT's SetRangeUser takes them
        ref.SetRangeUser(2.0, 8.0)
        want = (ref.GetBinLowEdge(ref.GetFirst()), ref.GetBinUpEdge(ref.GetLast()))
        check(f"{what}: the range of 'Z' on the binned z axis", close(shown[0], want[0], 1e-9) and close(shown[1], want[1], 1e-9), f"{shown} instead of {want}")
        check(f"{what}: the look of 'Z' stays off the colour scale", palette is None or (abs(palette.GetAxis().GetLabelSize() - 0.06) > 1e-3 and palette.GetAxis().GetTitle() != "binned z"), f"{palette.GetAxis().GetLabelSize() if palette else None}")
flatFrame = run.frame_hist("flat", "named")
flatLevels = [flatFrame.GetContourLevel(k) for k in range(flatFrame.GetContour())] if flatFrame else []
ok = len(flatLevels) > 1 and close(flatFrame.GetMinimum(), 4.5, 1e-9) and close(flatFrame.GetMaximum(), 5.5, 1e-9) and close(flatLevels[0], 4.5, 1e-9) and close(flatLevels[-1] + (flatLevels[1] - flatLevels[0]), 5.5, 1e-9)
check("a 3d histogram of one value: range widened by 10 % of it, the colour levels spread over the widened range", ok, f"frame {flatFrame and (flatFrame.GetMinimum(), flatFrame.GetMaximum())}, levels {flatLevels[:1]} … {flatLevels[-1:]}")
check("a 'C' axis in a pad without a 3d histogram is warned about", any("The C axis of pad 1 is ignored" in m for m in run.messages("no_th3", "named")), " | ".join(run.messages("no_th3", "named")))
finish()
