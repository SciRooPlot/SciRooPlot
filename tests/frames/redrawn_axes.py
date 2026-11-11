"""SetRedrawAxes() (on the pad, on pad 0 or by the base plot: the shipped base plots with a colour scale set it) draws the frame of a flat
drawing again on top of its data: the frame box as four lines along the frame in the frame's line attributes (SetFrameBorder: style,
width, colour) and the axes as the frame has them; the colour scale of the frame stays as it is. Without it (or with SetRedrawAxes(false)
on the pad) nothing is drawn again. A 3d view, a polar drawing and a map projection have no frame box: nothing is drawn again there.
Cases: every data kind drawn as it is by default, 2d data in a 3d view, × redrawn by the pad, by pad 0, as the base plot sets it or switched
off on the pad × frame border as the base plot sets it or set, and the named cases.
Reference: the frame of the pad (its TFrame, margins and frame histogram), the values set (colours compared by rgb and alpha) and, for
the colour scale, the same data drawn with SetRedrawAxes(false).
Covers: the frame box drawn again with only the frame's line attributes, the colour scale of the frame lost or moved by drawing its axes
again, a map projection drawn with a frame box.
"""

from srptest import DATA_1D, DATA_2D, DATA_3D, DATA_KINDS, DATA_POLAR, ROOT, Project, S, add_data, base_of, cases, check, close, describe, finish, rgba

KINDS = DATA_1D + DATA_POLAR + DATA_2D + DATA_3D
MAP = "AITOFF"  # a map projection
BORDER = (7, 4, ROOT.kBlue)  # SetFrameBorder(style, width, colour)


def view_option(kind):
    """the alias drawing 2d data of a kind in a 3d view"""
    return S.lego if "binned" in DATA_KINDS[kind][2] else S.surf


todo = cases(
    {"kind": list(KINDS), "draw": ["default", "view"], "redraw": ["pad", "pad0", "base", "off"], "border": ["base", "set"]},
    valid=lambda c: c["draw"] == "default" or (DATA_KINDS[c["kind"]][0] == 2 and "polygons" not in DATA_KINDS[c["kind"]][2]),  # a TH2Poly has no 3d view
    named=[{"kind": "hist2d", "draw": "map", "redraw": "pad", "border": "set", "why": "a map projection has no frame box"}],
)

p = Project("redrawn_axes")
p.inputs()
for i, case in enumerate(todo):
    kind = case["kind"]
    plot = S.Plot(f"r{i}", "redraw", base_of(kind))
    pad = plot[1]
    data = add_data(pad, kind)
    if case["draw"] == "view":
        data.SetDrawOption(view_option(kind))
    elif case["draw"] == "map":
        data.SetDrawOption(MAP)
    if case["redraw"] == "pad":
        pad.SetRedrawAxes()
    elif case["redraw"] == "pad0":
        plot[0].SetRedrawAxes()
    elif case["redraw"] == "off":
        pad.SetRedrawAxes(False)
    if case["border"] == "set":
        pad.SetFrameBorder(*BORDER)
    p.pm.AddPlot(plot)
for kind in KINDS:
    plot = S.Plot(f"alone_{kind}", "redraw", base_of(kind))
    add_data(plot[1], kind)
    plot[1].SetRedrawAxes(False)
    p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def colour_scale(name):
    """the position of the colour scale of the frame of a plot, None without one"""
    frame = run.frame_hist(name, "redraw")
    scale = frame.GetListOfFunctions().FindObject("palette") if frame else None
    return tuple(round(v, 9) for v in (scale.GetX1NDC(), scale.GetX2NDC(), scale.GetY1NDC(), scale.GetY2NDC())) if scale else None


def axis_look(axis):
    return (
        axis.GetTitle(),
        round(axis.GetTitleSize(), 6),
        round(axis.GetTitleOffset(), 6),
        round(axis.GetLabelSize(), 6),
        round(axis.GetLabelOffset(), 6),
        axis.GetLabelFont(),
        axis.GetTitleFont(),
        round(axis.GetTickLength(), 6),
        axis.GetNdivisions(),
        axis.GetFirst(),
        axis.GetLast(),
        axis.GetXmin(),
        axis.GetXmax(),
    )


for i, case in enumerate(todo):
    what = describe(case)
    name = f"r{i}"
    padObj = run.pad(name, "redraw")
    if not padObj:
        check(f"{what}: drawn", False, " | ".join(run.messages(name, "redraw")))
        continue
    links = run.links(name, "redraw")
    lines = [(k, obj) for k, (obj, _) in enumerate(links) if obj.IsA() == ROOT.TLine.Class()]
    copies = [(k, obj) for k, (obj, opt) in enumerate(links) if obj.InheritsFrom("TH1") and "AXIS" in opt.upper() and "SAME" in opt.upper()]
    flat = case["draw"] == "default" and DATA_KINDS[case["kind"]][0] < 3 and "polar" not in DATA_KINDS[case["kind"]][2]
    redrawn = case["redraw"] in ("pad", "pad0") or (case["redraw"] == "base" and base_of(case["kind"]) == "2d")  # the 2d base plot has a colour scale
    if not redrawn or not flat:
        check(f"{what}: nothing drawn again", not lines and not copies, f"{len(lines)} lines, {len(copies)} axis copies")
        continue
    frameBox = next((obj for obj, _ in links if obj.IsA() == ROOT.TFrame.Class()), None)
    want = (BORDER[0], BORDER[1], rgba(BORDER[2])) if case["border"] == "set" else (frameBox.GetLineStyle(), frameBox.GetLineWidth(), rgba(frameBox.GetLineColor())) if frameBox else None
    got = sorted({(ln.GetLineStyle(), ln.GetLineWidth(), rgba(ln.GetLineColor())) for _, ln in lines})
    check(f"{what}: the frame box as four lines in the frame's line attributes", len(lines) == 4 and got == [want], f"{len(lines)} lines with {got}, frame {want}")
    left, right, bottom, top = padObj.GetLeftMargin(), 1.0 - padObj.GetRightMargin(), padObj.GetBottomMargin(), 1.0 - padObj.GetTopMargin()
    sides = {(right, bottom, right, top), (left, bottom, left, top), (left, top, right, top), (left, bottom, right, bottom)}
    found = {(ln.GetX1(), ln.GetY1(), ln.GetX2(), ln.GetY2()) for _, ln in lines}
    check(f"{what}: the lines along the frame", len(found) == 4 and all(any(all(close(a, b, 1e-9) for a, b in zip(f, s)) for s in sides) for f in found), f"{sorted(found)}")
    frame = run.frame_hist(name, "redraw")
    if len(copies) != 1 or not frame:
        check(f"{what}: the axes drawn again once", False, f"{len(copies)} axis copies")
        continue
    copy = copies[0][1]
    check(
        f"{what}: the axes drawn again look like those of the frame",
        all(axis_look(getattr(copy, a)()) == axis_look(getattr(frame, a)()) for a in ("GetXaxis", "GetYaxis")),
        f"{[axis_look(getattr(copy, a)()) for a in ('GetXaxis', 'GetYaxis')]} vs {[axis_look(getattr(frame, a)()) for a in ('GetXaxis', 'GetYaxis')]}",
    )
    alone = colour_scale(f"alone_{case['kind']}")
    check(f"{what}: the colour scale of the frame as without drawing the axes again", colour_scale(name) == alone, f"{colour_scale(name)}, alone {alone}")
    data = [k for k, (obj, _) in enumerate(links) if obj.GetName()[:1].isdigit()]
    check(f"{what}: drawn again after the data", data and min(k for k, _ in lines + copies) > max(data), f"data at {data}, frame at {[k for k, _ in lines + copies]}")
finish()
