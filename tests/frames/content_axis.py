"""The 'C' axis of a pad is the content of a 3d histogram (its z axis is binned): a range on it (both ends, or one) is the minimum and maximum
of the frame and of the data, with the colour levels spread over it; its title is the title of the colour scale; a log scale on it is
warned about (ROOT colours the boxes linearly) and not applied; the settings of 'Z' stay on the binned z axis and do not reach the colour
scale; a 'C' axis in a pad without a 3d histogram is warned about.
Cases: every 3d histogram kind × a range on 'C' (both ends, minimum, maximum), a title on 'C', a log 'C', a range and title on 'Z', and the
named cases.
Reference: the frame and data read back (minimum, maximum, colour levels, the colour scale's axis title, the z axis).
Covers: the content axis of 3d histograms.
"""

from srptest import DATA_3D, DATA_KINDS, Project, S, add_data, cases, check, close, describe, finish

KINDS = tuple(k for k in DATA_3D if "binned" in DATA_KINDS[k][2])
SETTINGS = {"range": (5.0, 400.0), "min_only": (50.0, None), "max_only": (None, 300.0), "title": None, "log": None, "z_axis": None}
todo = cases({"kind": list(KINDS), "setting": list(SETTINGS)}, always=lambda c: True)

p = Project("content_axis")
p.inputs()
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
    else:
        pad["Z"].SetRange(2.0, 8.0).SetTitle("binned z").SetLabelSize(0.06)
    p.pm.AddPlot(plot)
plot = S.Plot("no_th3", "named", "2d")
plot[1].AddData("hist2d", "inputs")
plot[1]["C"].SetRange(1.0, 2.0)
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))
for i, case in enumerate(todo):
    what = describe(case)
    frame = run.frame_hist(f"c{i}", "content")
    drawn = run.objects(f"c{i}", "content")
    if not frame or not drawn:
        check(f"{what}: drawn", False, " | ".join(run.messages(f"c{i}", "content")))
        continue
    data = drawn[0]
    setting = case["setting"]
    palette = frame.GetListOfFunctions().FindObject("palette")
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
        padObj = run.pad(f"c{i}", "content")
        check(f"{what}: a log scale warned about, not applied", not padObj.GetLogz() and any("Log scale of the C axis in pad 1 ignored" in m for m in run.messages(f"c{i}", "content")), " | ".join(run.messages(f"c{i}", "content")))
    else:
        z = frame.GetZaxis()
        shown = (z.GetBinLowEdge(z.GetFirst()), z.GetBinUpEdge(z.GetLast()))
        check(f"{what}: the range of 'Z' on the binned z axis", close(shown[0], 2.0, 1e-9) and close(shown[1], 8.0, 1e-9), f"{shown}")
        check(f"{what}: the look of 'Z' stays off the colour scale", palette is None or (abs(palette.GetAxis().GetLabelSize() - 0.06) > 1e-3 and palette.GetAxis().GetTitle() != "binned z"), f"{palette.GetAxis().GetLabelSize() if palette else None}")
check("a 'C' axis in a pad without a 3d histogram is warned about", any("The C axis of pad 1 is ignored" in m for m in run.messages("no_th3", "named")), " | ".join(run.messages("no_th3", "named")))
finish()
