"""A 2d histogram of every kind drawn as a map (ROOT's projections AITOFF, MERCATOR, SINUSOIDAL, PARABOLIC, MOLLWEIDE) is painted over the
range of its frame, coloured over the z range of the pad, with a colour scale right of the frame when the option has a Z (ROOT draws none
for projections) and none otherwise, painting no cell outside the map; a range on the pad applies to the map, a range on the data is
ignored with a warning; the axes ROOT paints for the map look like those of the frame, and no frame box is drawn again. Data drawn after
a map must be a map of the same projection: other data is not drawn, with one warning, and the data after it keeps its number.
COL with a projection is warned about (ROOT paints the map twice).
Covers: a map over a part of the sphere too small in its frame, maps without colour scale, cells painted outside of the map on log z,
flat maps and maps of other projections drawn silently into a map.
"""

import os
import re

import numpy as np

from srptest import ROOT, Project, S, bitmap, cases, check, describe, finish

PROJECTIONS = ("aitoff", "mercator", "sinusoidal", "parabolic", "mollweide")
SCALES = {"none": ("", False), "z": (" z", False), "z_log": (" z", True)}
RANGES = ("none", "pad_y", "data_y", "z")
SECOND = {"none": None, "same": "{projection}", "other": "{other}", "flat": "colz"}  # a second data after the map (a third always follows)


def sphere(kind):
    """a map of the whole sphere of a kind, the values growing to the north from 1 to 10 (5 degrees in latitude: an edge at 0)"""
    if kind == "efficiency2d":
        passed = ROOT.TH2D("p", ";longitude;latitude", 90, -180, 180, 36, -90, 90)
        total = passed.Clone("t")
        for i in range(1, 91):
            for j in range(1, 37):
                total.SetBinContent(i, j, 40)
                passed.SetBinContent(i, j, j)
        return ROOT.TEfficiency(passed, total)
    if kind == "thn_sparse2d":
        sparse = ROOT.THnSparseD("s", ";longitude;latitude", 2, np.array([90, 36], dtype=np.int32), np.array([-180.0, -90.0]), np.array([180.0, 90.0]))
        for i in range(1, 91):
            for j in range(1, 37):
                sparse.SetBinContent(np.array([i, j], dtype=np.int32), 1.0 + 9.0 * j / 36.0)
        return sparse
    hist = (ROOT.TProfile2D if kind == "profile2d" else ROOT.TH2D)(kind, ";longitude;latitude;value", 90, -180, 180, 36, -90, 90)
    for i in range(1, 91):
        for j in range(1, 37):
            x, y = hist.GetXaxis().GetBinCenter(i), hist.GetYaxis().GetBinCenter(j)
            hist.Fill(x, y, 1.0 + 9.0 * j / 36.0)
    return hist


KINDS = ("hist2d", "profile2d", "efficiency2d", "thn_sparse2d")
p = Project("maps", bases=("2d_wide",))
p.source("maps", {kind: sphere(kind) for kind in KINDS})


def add(pad, kind, option):
    data = pad.AddData(kind, "maps").SetDrawOption(option)
    return data.Project([0, 1]) if kind == "thn_sparse2d" else data


todo = cases(
    {"kind": list(KINDS), "projection": list(PROJECTIONS), "scale": list(SCALES), "range": list(RANGES), "second": list(SECOND)},
    always=lambda c: c["kind"] == "hist2d" and c["projection"] == "mollweide" and c["second"] == "none",
    named=[
        {"kind": "hist2d", "projection": "mollweide", "scale": "z", "range": "pad_y", "second": "none", "why": "a map over a part of the sphere fills its frame"},
        {"kind": "hist2d", "projection": "aitoff", "scale": "z_log", "range": "none", "second": "none", "why": "no cells outside the map on log z"},
        {"kind": "hist2d", "projection": "mollweide", "scale": "none", "range": "none", "second": "flat", "why": "a flat map in a map"},
    ],
)
for i, case in enumerate(todo):
    option, logZ = SCALES[case["scale"]]
    plot = S.Plot(f"m{i}", "maps", "2d_wide")
    data = add(plot[1], case["kind"], case["projection"] + option)
    if case["range"] == "data_y":
        data.SetRangeY(0.0, 90.0)
    elif case["range"] == "pad_y":
        plot[1]["Y"].SetRange(0.0, 90.0)
    elif case["range"] == "z":
        plot[1]["Z"].SetRange(2.0, 20.0)
    plot[1]["Z"].SetLog(logZ)
    if SECOND[case["second"]]:
        other = PROJECTIONS[(PROJECTIONS.index(case["projection"]) + 1) % len(PROJECTIONS)]
        add(plot[1], case["kind"], SECOND[case["second"]].format(projection=case["projection"], other=other))
    add(plot[1], case["kind"], case["projection"])
    p.pm.AddPlot(plot)
col = S.Plot("col", "maps", "2d_wide")
add(col[1], "hist2d", "colz mollweide")
p.pm.AddPlot(col)
run = p.plot()

for i, case in enumerate(todo):
    what = describe(case)
    links = run.links(f"m{i}", "maps")
    pad = run.pad(f"m{i}", "maps")
    if not pad:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    data = [(obj, opt) for obj, opt in links if re.match(r"\d+:", obj.GetName())]
    check(f"painted without SAME (fits its frame): {what}", data and "SAME" not in data[0][1].upper(), str([opt for _, opt in data]))
    second = SECOND[case["second"]]
    drawn = second is None or case["second"] == "same"
    numbers = [obj.GetName().split(":")[0] for obj, _ in data]
    want = ["1", "2", "3"] if second and drawn else (["1", "3"] if second else ["1", "2"])
    check(f"data after a map drawn only as a map of its projection, the next keeping its number: {what}", numbers == want, f"drawn {numbers}, expected {want}")
    refused = [line for line in run.messages(f"m{i}", "maps") if "It is not drawn" in line]
    check(f"{0 if drawn else 1} warning that data is not drawn: {what}", len(refused) == (0 if drawn else 1), " | ".join(refused))
    mapObj = data[0][0] if data else None
    scale = pad.GetPrimitive("color_scale_pad_1")
    palette = scale.GetListOfFunctions().FindObject("palette") if scale else None
    if case["scale"] == "none":
        check(f"no colour scale: {what}", not palette)
    else:
        check(f"a colour scale right of the frame: {what}", bool(palette) and palette.GetX1NDC() > 1.0 - pad.GetRightMargin() - 1e-6, f"at {palette.GetX1NDC():.3f}" if palette else "none")
    if case["range"] == "z" and mapObj:
        check(f"coloured over the z range of the pad: {what}", (mapObj.GetMinimumStored(), mapObj.GetMaximumStored()) == (2.0, 20.0), f"{mapObj.GetMinimumStored()} to {mapObj.GetMaximumStored()}")
    if mapObj:
        axis = mapObj.GetYaxis()
        shown = (axis.GetBinLowEdge(axis.GetFirst()), axis.GetBinUpEdge(axis.GetLast()))
        check(f"over the range of the pad: {what}", shown == ((0.0, 90.0) if case["range"] == "pad_y" else (-90.0, 90.0)), str(shown))
        frame = run.frame_hist(f"m{i}", "maps")
        look = all(getattr(mapObj, a)().GetTickLength() == getattr(frame, a)().GetTickLength() and getattr(mapObj, a)().GetLabelSize() == getattr(frame, a)().GetLabelSize() for a in ("GetXaxis", "GetYaxis"))
        check(f"axes of the map like those of the frame: {what}", look)
    ranged = [line for line in run.messages(f"m{i}", "maps") if "does not apply to a map projection" in line]
    check(f"{1 if case['range'] == 'data_y' else 0} warning for a range on the data: {what}", len(ranged) == (case["range"] == "data_y"), " | ".join(ranged))
    check(f"no frame box drawn again: {what}", not [obj for obj, _ in links if obj.InheritsFrom("TLine")])


# a corner of the frame of a map of the whole sphere with a colour scale, outside the map: not painted (in the png SciRooPlot writes; a
# map canvas read back is not painted again in this ROOT)
corners = [i for i, c in enumerate(todo) if c["range"] in ("none", "z") and c["scale"] != "none" and c["projection"] != "mercator"]
margins = {i: (run.pad(f"m{i}", "maps").GetLeftMargin(), run.pad(f"m{i}", "maps").GetTopMargin()) for i in corners if run.pad(f"m{i}", "maps")}
p.plot("maps", "|".join(f"m{i}" for i in corners), mode="png")
for i in corners:
    what = describe(todo[i])
    path = p.path("out", "maps", f"m{i}.png")
    if i not in margins or not os.path.exists(path):
        check(f"png written: {what}", False)
        continue
    image = bitmap(path)
    height, width = image.shape[:2]
    left, top = margins[i]
    value = int(image[int((top + 0.02) * height), int((left + 0.01) * width)]) & 0xFFFFFF
    rgb = (value >> 16, (value >> 8) & 0xFF, value & 0xFF)
    check(f"no cells painted outside the map: {what}", min(rgb) > 200, f"pixel {rgb}")
check("COL with a projection warned once", len(run.warnings("paints COL with a map projection twice")) == 1, " | ".join(run.warnings("COL")))
check("no other warnings", all(any(t in w for t in ("It is not drawn", "does not apply to a map projection", "paints COL with a map projection")) for w in run.warnings()), " | ".join(run.warnings()[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
