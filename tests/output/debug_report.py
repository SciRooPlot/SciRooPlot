"""At verbosity debug every plot logs what it shows, pad by pad, for scripts and agents that cannot look at it: the frame (axis ranges and
log, or that it is a 3d view or polar), each data item as drawn (class, drawing option, marker, line and fill with their colours, the z
range of 2d data in a flat pad, and for 1d histograms and graphs in a flat pad how many values lie within the x range of the frame and
how many of them above or below it) and the rows of each legend. Below verbosity debug nothing of it is logged.
Covers: the report of every data kind alone (default and set colours, with a log y axis, cut by a y range, a log y range ending at values; 2d histograms with a z range and on log z), and in multi-panel layouts (ratio panels, shared axes, a legend strip,
a 3d view beside a map), checked against the canvas read back from Plots.root (the positions of boxes and shapes: boxes/coordinate_systems.py).
"""

import re

from srptest import DATA_KINDS, LAYOUT_BASES, LAYOUTS, ROOT, Project, S, add_data, base_of, cases, check, data_objects, describe, finish, has_layout, layout_plot, option, own, rgba

p = Project("debug_report", bases=("1d", "2d", *LAYOUT_BASES))
p.inputs()


def values_of(kind):
    """the values (bins not empty, points) of the input of a 1d histogram or graph kind"""
    obj = data_objects()[kind]
    if obj.InheritsFrom("TH1"):
        return [obj.GetBinContent(b) for b in range(1, obj.GetNbinsX() + 1) if obj.GetBinContent(b) != 0.0]
    return [obj.GetPointY(j) for j in range(obj.GetN())]


# ycut: a y range over the second quarter of the values, so that some lie below and more above the frame
CUTTABLE = [k for k, (dim, cls, props) in DATA_KINDS.items() if dim == 1 and cls.startswith(("TH1", "TProfile", "TGraph")) and not props & {"polar", "given"}]
# z: a z range over the second quarter of the contents, or a log z axis, for 2d histograms drawn flat
MAPS = [k for k, (dim, cls, props) in DATA_KINDS.items() if dim == 2 and cls.startswith(("TH2", "TProfile2D")) and not props & {"polar", "given"}]
todo = cases(
    {"kind": list(DATA_KINDS), "layout": ["single", *LAYOUTS], "logy": [False, True], "look": ["default", "set"], "ycut": [False, True], "z": ["auto", "range", "log"]},
    valid=lambda c: (c["layout"] == "single" or (has_layout(c["kind"], c["layout"]) and c["look"] == "default"))
    and (not c["logy"] or (c["layout"] == "single" and DATA_KINDS[c["kind"]][0] == 1))
    and (not c["ycut"] or (c["layout"] == "single" and c["kind"] in CUTTABLE))
    and (c["z"] == "auto" or (c["layout"] == "single" and c["look"] == "default" and c["kind"] in MAPS)),
    always=lambda c: c["kind"] in ("hist", "graph_errors", "hist2d") or c["layout"] == "ratio" or c["ycut"] or c["z"] != "auto",
)
for i, case in enumerate(todo):
    if case["layout"] == "single":
        plot = S.Plot(f"r{i}", "report", base_of(case["kind"]))
        data = add_data(plot[1], case["kind"]).SetLegendLabel("<name> <entries>")
        plot[1].AddLegend()
        if case["look"] == "set":
            data.SetMarkerColor("#aa3311").SetLineColor("#3366cc").SetLineAlpha(0.6).SetFillColor("#22aa55")
        if case["logy"]:
            plot[1]["Y"].SetLog()
        if case["ycut"]:
            values = sorted(v for v in values_of(case["kind"]) if v == v and abs(v) != float("inf") and (v > 0 or not case["logy"]))
            plot[1]["Y"].SetRange(values[len(values) // 4], values[len(values) // 2])  # more above than below
        if case["z"] == "range":
            obj = data_objects()[case["kind"]]
            values = sorted(c for c in (obj.GetBinContent(b) for b in range(obj.GetNcells())) if c > 0)
            plot[1]["Z"].SetRange(values[len(values) // 4], values[len(values) // 2])
        elif case["z"] == "log":
            plot[1]["Z"].SetLog()
    else:
        plot = layout_plot(f"r{i}", "report", case["kind"], case["layout"])
    p.pm.AddPlot(plot)
# contents 1 to 10 on log y, a y range ending at two of them: 10^log10(8) comes back below 8, the values there are still on the frame
steps = own(ROOT.TH1D("steps", "", 10, 0, 10))
for b in range(1, 11):
    steps.SetBinContent(b, b)
p.pm.AddDataSource("steps", steps)
plot = S.Plot("edge", "report", "1d")
plot[1].AddData("steps", "steps")
plot[1]["Y"].SetLog().SetRange(1, 8)
p.pm.AddPlot(plot)
# a map with empty cells on log z: the colour range starts above zero
cells = own(ROOT.TH2D("cells", "", 4, 0, 4, 4, 0, 4))
for c in range(16):
    cells.SetBinContent(c % 4 + 1, c // 4 + 1, c)
p.pm.AddDataSource("cells", cells)
plot = S.Plot("zeros", "report", "2d")
plot[1].AddData("cells", "cells")
plot[1]["Z"].SetLog()
p.pm.AddPlot(plot)
run = p.plot(env={"SCIROOPLOT_VERBOSITY": "debug"})


def number(value, log):
    """a frame end as the report writes it"""
    return f"{10**value if log else value:.4g}"


def colour(text):
    """(r, g, b, alpha) of a colour as the report writes it: #rrggbb, optionally followed by alpha"""
    m = re.match(r"(#[0-9a-f]{6})(?: alpha ([0-9.e-]+))?$", text)
    return rgba(m.group(1), float(m.group(2)) if m.group(2) else None) if m else None


for i, case in enumerate(todo):
    what = describe(case)
    lines = [re.sub(r"\x1b\[[0-9;]*m", "", line) for line in run.messages(f"r{i}", "report") if "[ DEBU" in line]
    lines = [line.split(f"Plot r{i} from group report: ", 1)[1] for line in lines if f"Plot r{i} from group report: " in line]
    canvas = run.canvas(f"r{i}", "report")
    if not canvas:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    pads = [obj for obj in canvas.GetListOfPrimitives() if obj.GetName().startswith("Pad_")]
    for pad in pads:
        k = pad.GetNumber()
        said = [line[len(f"Pad {k}, ") :] for line in lines if line.startswith(f"Pad {k}, ")]
        frame = [line[len("frame: ") :] for line in said if line.startswith("frame: ")]
        polar = any(obj.InheritsFrom("TGraphPolargram") for obj in pad.GetListOfPrimitives())
        if len(frame) != 1:
            check(f"the frame of pad {k} reported once: {what}", False, " | ".join(said))
        elif frame[0] != "a 3d view":  # a canvas read back has no TView: 3d views are reported as such, their ranges are not
            x = f"x {number(pad.GetUxmin(), pad.GetLogx())} to {number(pad.GetUxmax(), pad.GetLogx())}{' (log)' if pad.GetLogx() else ''}"
            y = f"y {number(pad.GetUymin(), pad.GetLogy())} to {number(pad.GetUymax(), pad.GetLogy())}{' (log)' if pad.GetLogy() else ''}"
            want = "polar" if polar else f"{x}, {y}"
            check(f"the frame of pad {k} as drawn: {what}", frame[0] == want, f"{frame[0]}, expected {want}")
        flat = len(frame) == 1 and frame[0] not in ("a 3d view", "polar")
        for obj, opt in run.drawn(f"r{i}", "report", k):
            if "AXIS" in opt:
                continue
            number_ = obj.GetName().split(":")[0]
            who = "reference function" if number_ == "0" else f"data {number_} ("
            found = [line for line in said if line.startswith(who)]
            if len(found) != 1:
                check(f"pad {k}, {who.rstrip(' (')} reported once: {what}", False, " | ".join(said))
                continue
            look = found[0].split("): " if who.endswith("(") else ": ", 1)[1]
            m = re.match(r'(\S+) drawn "([^"]*)"', look)
            check(f"pad {k}, {who.rstrip(' (')}: class and drawing option as drawn: {what}", bool(m) and m.group(1) == obj.ClassName() and option(m.group(2)) == opt, f"{look}, expected {obj.ClassName()} {opt}")
            for attribute, pattern, styles in (
                ("TAttMarker", r"marker (\d+) (#[0-9a-f]{6}(?: alpha [0-9.e-]+)?) size ([0-9.e+-]+)", (obj.GetMarkerStyle, obj.GetMarkerColor, obj.GetMarkerSize) if obj.InheritsFrom("TAttMarker") else ()),
                ("TAttLine", r"line (\d+) (#[0-9a-f]{6}(?: alpha [0-9.e-]+)?) width (\d+)", (obj.GetLineStyle, obj.GetLineColor, obj.GetLineWidth) if obj.InheritsFrom("TAttLine") else ()),
            ):
                if not styles:
                    continue
                n = re.search(pattern, look)
                ok = bool(n) and int(n.group(1)) == styles[0]() and colour(n.group(2)) == rgba(styles[1]()) and abs(float(n.group(3)) - styles[2]()) <= 1e-3 * max(1.0, styles[2]())
                check(f"pad {k}, {who.rstrip(' (')}: {attribute[4:].lower()} as drawn: {what}", ok, f"{look}; drawn {styles[0]()} {rgba(styles[1]())} {styles[2]()}")
            if obj.InheritsFrom("TAttFill"):
                n = re.search(r"(no fill|fill (\d+) (#[0-9a-f]{6}(?: alpha [0-9.e-]+)?))", look)
                ok = bool(n) and ((obj.GetFillStyle() == 0) == (n.group(1) == "no fill")) and (obj.GetFillStyle() == 0 or (int(n.group(2)) == obj.GetFillStyle() and colour(n.group(3)) == rgba(obj.GetFillColor())))
                check(f"pad {k}, {who.rstrip(' (')}: fill as drawn: {what}", ok, f"{look}; drawn {obj.GetFillStyle()} {rgba(obj.GetFillColor())}")
            # the colour range of 2d data is the one of the frame it is drawn on
            if flat and obj.InheritsFrom("TH1") and obj.GetDimension() == 2:
                frameHist = pad.GetPrimitive(f"axis_hist_pad_{k}")
                zHist = frameHist if frameHist and frameHist.GetDimension() == 2 else obj
                want = f"z {zHist.GetMinimum():.4g} to {zHist.GetMaximum():.4g}{' (log)' if pad.GetLogz() else ''}"
                n = re.search(r", (z \S+ to \S+(?: \(log\))?)", look)
                check(f"pad {k}, {who.rstrip(' (')}: z range as drawn: {what}", bool(n) and n.group(1) == want, f"{look}; expected {want}")
            # the values within the x range of the frame and those beyond its top or bottom, counted here from the drawn object
            counted = None
            if flat and ((obj.InheritsFrom("TH1") and obj.GetDimension() == 1) or obj.InheritsFrom("TGraph")):
                # a value at a range end lies on the frame, also when it comes back from a log axis a rounding error beyond it
                spanX, spanY = 1e-9 * (pad.GetUxmax() - pad.GetUxmin()), 1e-9 * (pad.GetUymax() - pad.GetUymin())
                xs = (pad.GetUxmin() - spanX, pad.GetUxmax() + spanX)
                ys = (pad.GetUymin() - spanY, pad.GetUymax() + spanY)
                xMin, xMax = (10**v for v in xs) if pad.GetLogx() else xs
                yMin, yMax = (10**v for v in ys) if pad.GetLogy() else ys
                if obj.InheritsFrom("TH1"):
                    values = [(obj.GetXaxis().GetBinCenter(b), obj.GetBinContent(b)) for b in range(1, obj.GetNbinsX() + 1) if obj.GetBinContent(b) != 0.0]
                else:
                    values = [(obj.GetPointX(j), obj.GetPointY(j)) for j in range(obj.GetN())]
                values = [(x_, y_) for x_, y_ in values if xMin <= x_ <= xMax and y_ == y_ and abs(y_) != float("inf") and not (pad.GetLogy() and y_ <= 0.0)]
                counted = (len(values), sum(y_ > yMax for _, y_ in values), sum(y_ < yMin for _, y_ in values))
            n = re.search(r"; (\d+) (bins|points) within the x range of the frame(?:, (\d+) above it)?(?:, (\d+) below it)?$", look)
            got = (int(n.group(1)), int(n.group(3) or 0), int(n.group(4) or 0)) if n else None
            check(f"pad {k}, {who.rstrip(' (')}: values within, above and below the frame as drawn: {what}", got == counted, f"{look}; counted {counted}")
        legends = [obj for obj in pad.GetListOfPrimitives() if obj.InheritsFrom("TLegend") and obj.GetName().startswith("LegendBox_")]
        for legend in legends:
            rows = [(e.GetLabel(), e.GetOption()) for e in legend.GetListOfPrimitives()]
            prefix = f"legend {legend.GetName()[10:]}, row "
            reported = [line[len(prefix) :] for line in said if line.startswith(prefix)]
            want = [f'{j + 1}: "{label}" ({f"symbol {chr(34)}{opt}{chr(34)}" if opt else "no symbol"})' for j, (label, opt) in enumerate(rows)]
            check(f"pad {k}, the rows of {prefix[:-6]} as drawn: {what}", reported == want, f"{reported}, expected {want}")
edge = [line for line in run.messages("edge", "report") if "Pad 1, data 1 (" in line]
check("values at the ends of a y range on a log axis are on the frame", len(edge) == 1 and edge[0].endswith("; 10 bins within the x range of the frame, 2 above it"), " | ".join(edge))
zeros = [line for line in run.messages("zeros", "report") if "Pad 1, data 1 (" in line]
frameHist = next(pad for pad in run.canvas("zeros", "report").GetListOfPrimitives() if pad.GetName().startswith("Pad_")).GetPrimitive("axis_hist_pad_1")
want = f"z {frameHist.GetMinimum():.4g} to {frameHist.GetMaximum():.4g} (log)"
check("a map with empty cells on log z: the colour range of the frame, above zero", len(zeros) == 1 and want in zeros[0] and frameHist.GetMinimum() > 0, f"{' | '.join(zeros)}; expected {want}")
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))

# below verbosity debug: no report
quiet = p.plot(name="r0", env={"SCIROOPLOT_VERBOSITY": "hint"})
check("nothing of the report below verbosity debug", not [line for line in quiet.log.splitlines() if "[ DEBU" in line], quiet.log[-300:])
finish()
