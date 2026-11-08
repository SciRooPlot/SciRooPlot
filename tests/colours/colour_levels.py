"""The colour levels of the first map of a pad (Data::SetContours: a number of equal levels or the levels themselves) are those of its colour
scale and of the map, also fewer than ROOT's 20 (its default without levels); levels of later data (contour lines over a map) are their
own and leave the scale alone unless they are more. The boxes of a 3d histogram, which ROOT colours from the whole palette, take the
colours of the scale: the pad paints with a palette of one colour per level; levels given as values warn there (equal steps).
Every 2d kind and the 3d histograms.
Covers: the frame takes the levels of the first data, a 3d histogram a palette of one colour per level; levels of 2d graphs and 2d
functions (were ignored; contour lines over them gave the scale their levels).
"""

from srptest import DATA_2D, Project, S, add_data, cases, check, describe, finish

LEVELS = {"none": None, "five": 5, "thirty": 30, "values": [2.0, 5.0, 10.0, 30.0, 60.0]}
KINDS = DATA_2D + ("hist3d", "profile3d")
OVERLAY = [3.0, 20.0]  # levels of contour lines drawn over the map


def levels_of(hist):
    return [hist.GetContourLevel(k) for k in range(hist.GetContour())] if hist else None


p = Project("colour_levels")
p.inputs()
todo = cases(
    {"kind": list(KINDS), "levels": list(LEVELS), "overlay": [False, True]},
    valid=lambda c: not c["overlay"] or c["kind"] in DATA_2D,
    always=lambda c: True,  # small: every case in the fast run too
)
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "levels", "2d")
    data = add_data(plot[1], case["kind"])
    levels = LEVELS[case["levels"]]
    if levels is not None:
        data.SetContours(levels)
    if case["overlay"]:
        add_data(plot[1], "hist2d").SetDrawOption(S.cont).SetContours(OVERLAY)
    p.pm.AddPlot(plot)
run = p.plot("levels")

for i, case in enumerate(todo):
    what = describe(case)
    frame = run.frame_hist(f"c{i}", "levels")
    drawn = run.drawn(f"c{i}", "levels")
    if not frame or not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    levels = LEVELS[case["levels"]]
    n = len(levels) if isinstance(levels, list) else (levels or 20)
    found = levels_of(frame)
    if isinstance(levels, list):
        ok = found == levels
    else:
        ok = len(found) == n and abs(found[0] - frame.GetMinimum()) <= 1e-9 * max(1.0, abs(frame.GetMinimum()))
    check(f"the colour scale has the levels of the first map: {what}", ok, f"{len(found)} levels {found[:3]}, expected {n}")
    obj = drawn[0][0]
    if obj.InheritsFrom("TGraph2D"):  # painted through a histogram ROOT makes when painting: set up as when the pad is painted
        pad = run.pad(f"c{i}", "levels")
        pad.cd()
        for setup in pad.GetListOfPrimitives():
            if setup.InheritsFrom("TExec") and setup.GetName().startswith("levels_"):
                setup.Exec()
        obj = pad.GetPrimitive(obj.GetName()).GetHistogram()
    if obj.InheritsFrom("TH1") or obj.InheritsFrom("TF2"):
        own = obj.GetContour() if obj.InheritsFrom("TF2") else len(levels_of(obj))
        check(f"the map has the levels: {what}", own == n or (levels is None and own in (0, 20)), f"{own} levels, expected {n}")
    if case["overlay"]:
        lines = drawn[1][0]
        check(f"contour lines keep their own levels: {what}", levels_of(lines) == OVERLAY, levels_of(lines))
    if case["kind"] in ("hist3d", "profile3d"):
        colours = set(run.palette(f"c{i}", "levels"))
        check(f"3d boxes painted with one colour per level: {what}", len(colours) == (n if levels is not None else 255) or (levels is None and len(colours) > 20), f"{len(colours)} colours")
warned = run.warnings("are coloured in")
want = sum(1 for case in todo if case["kind"] in ("hist3d", "profile3d") and isinstance(LEVELS[case["levels"]], list))
check("levels as values of a 3d histogram warn once each", len(warned) == want, f"{len(warned)} warnings, expected {want}")
others = [line for line in run.warnings() if line not in warned]
check("no other warnings", not others, " | ".join(others[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
