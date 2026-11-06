"""A legend entry shows what is drawn for its data, for every data kind of srptest, with the drawing option aliases of what it is drawn
as (a 1d TEfficiency as a graph, a 2d one as a histogram), the common ROOT options of 1d histograms and graphs, and a set of styles
(marker red, line blue and fill green where they differ, so the colour shows which part the legend took).
Each canvas is printed to SVG without the legend, without the data and with neither (the frame): the styles of the elements drawn for
the data (fill, stroke, width, dashes, opacity, marker size) must be those drawn for the legend entry, and a line through the points
must have a line (or a fill outlined by it) in the legend and vice versa.
Not compared, since TLegend cannot draw them or ROOT's option does not follow its own drawing rules: the error bars of graphs drawn as
fill (F, B) and of multi-error graphs with a raw box or band option (2-5 without ';': ROOT draws only the first set as boxes or band,
the others as bars, and TLegend has one line for both), the *H, B and BAR options of histograms, 3d views (shaded, or drawn with the
box of the view: lego, surf, the boxes and iso surfaces of 3d data, the markers, lines and triangles of 2d graphs), the parts of candles
and violins, the solid arrowed error bars of asymmetric graphs, line and markers hidden by an opaque band (3L, 3PL, 4C), the outline of
a hollow HIST E2 box (TLegend takes the fill colour), arrow heads, and the lines of 2d drawings (contours and meshes are many short
pieces)."""

import collections
import hashlib
import re

from srptest import DATA_KINDS, WORK, Project, ROOT, S, add_data, cases, describe, drawn_type, finish, in_chunks

ALIASES_POINTS_LINES = ["default", "points", "points_xerr", "points_endcaps", "points_xerr_endcaps", "points_line", "points_arrows", "line", "curve", "curve_clipped"]
ALIASES_1D = ALIASES_POINTS_LINES + ["hist", "hist_open", "hist_clipped", "hist_open_clipped", "bars", "hbars", "area", "area_smooth", "band", "band_smooth", "boxes", "boxes_nomarkers", "brackets"]
RAW_HIST = ["E", "E0", "E1", "E3", "E4", "HIST", "L", "C", "P", "B", "BAR", "HBAR", "LF2", "HIST F", "HIST P", "E P", "E2 P", "*H", "HIST LP", "HIST E", "HIST E1", "HIST E2", "HIST E3"]
RAW_GRAPH = ["P", "L", "C", "B", "F", "LP", "CP", "PZ", "2", "3", "4", "5", "3L", "4C", "3PL", "AP", "LF", "E", "E1", "||", "[]", "0", "X P", "*"]
ALIASES_FUNC = ["default", "points", "line", "curve", "area", "area_smooth"]
ALIASES_2D = ["default", "col", "colz", "box", "lego", "surf", "cont", "cont_col", "cont_fill", "candle", "violin"]
ALIASES_GRAPH_2D = ["default", "points", "points_col", "line", "col", "colz", "lego", "surf", "cont", "cont_col", "triangles", "triangles_col"]
ALIASES_FUNC_2D = ["default", "col", "colz", "lego", "surf", "cont", "cont_col", "cont_fill"]
ALIASES_3D = ["default", "col", "colz", "box", "iso"]
ALIASES_FUNC_3D = ["default", "iso"]
STYLES = {
    "color": lambda d: d.SetColor(S.kRed + 1),
    "fill": lambda d: d.SetColor(S.kRed + 1).SetFill(1001, S.kRed + 1, 0.4),
    "lw0": lambda d: d.SetColor(S.kRed + 1).SetLineWidth(0),
    "dashed": lambda d: d.SetColor(S.kRed + 1).SetLine(S.kDashed, 3, S.kRed + 1),
    "marker": lambda d: d.SetColor(S.kRed + 1).SetMarker(25, 2, S.kRed + 1),
    "hatch": lambda d: d.SetColor(S.kRed + 1).SetFill(3354, S.kRed + 1),
    "split": lambda d: d.SetMarkerColor(S.kRed + 1).SetLineColor(S.kBlue + 1).SetFill(1001, S.kGreen + 2),
    "split_lw0": lambda d: d.SetMarkerColor(S.kRed + 1).SetLineColor(S.kBlue + 1).SetFill(1001, S.kGreen + 2).SetLineWidth(0),
    "hollow_split": lambda d: d.SetMarkerColor(S.kRed + 1).SetLineColor(S.kBlue + 1).SetFillColor(S.kGreen + 2).SetFillStyle(0),
    "split_dashed_ms0": lambda d: d.SetMarkerColor(S.kRed + 1).SetLine(S.kDashed, 3, S.kBlue + 1).SetFill(3354, S.kGreen + 2).SetMarkerSize(0),
    "solid_lw1": lambda d: d.SetColor(S.kRed + 1).SetFill(1001, S.kGreen + 2).SetLineWidth(1).SetMarker(20, 1, S.kRed + 1),
}
KINDS = tuple(DATA_KINDS)
OPTIONS = {
    "hist1d": ALIASES_1D + RAW_HIST,
    "graph": ALIASES_1D + RAW_GRAPH,
    "function": ALIASES_FUNC,
    "hist2d": ALIASES_2D,
    "graph2d": ALIASES_GRAPH_2D,
    "function2d": ALIASES_FUNC_2D,
    "hist3d": ALIASES_3D,
    "function3d": ALIASES_FUNC_3D,
}
# drawn in a 3d view, shaded or with the box of the view (2d graphs draw markers, lines and triangles in 3d)
VIEWS_3D = {
    "hist2d": ("lego", "surf"),
    "function2d": ("lego", "surf"),
    "graph2d": ("points", "line", "lego", "surf", "triangles"),
    "hist3d": ("box", "iso"),
    "function3d": ("default", "iso"),
}
ASYMMETRIC = ("TGraphAsymmErrors", "TGraphMultiErrors", "TEfficiency")  # graphs drawn with asymmetric errors


def has_errors(kind):
    return kind != "hist_noerrors" and DATA_KINDS[kind][1] not in ("TGraph", "TGraphPolar")


def compared(case):
    """the cases whose differences TLegend cannot avoid, or whose ROOT option does not follow its own drawing rules, are left out"""
    kind, option, style = case["kind"], case["option"], case["style"]
    what = drawn_type(kind)
    if option not in OPTIONS[what]:
        return False
    if what == "graph" and has_errors(kind) and option in ("F", "B", "3L", "3PL", "4C"):
        return False
    if kind == "graph_multi" and option in ("2", "3", "4", "5"):  # ROOT draws the first set of errors as boxes or a band, the others as bars
        return False
    if what == "hist1d" and option in ("*H", "B", "BAR"):
        return False
    if option in VIEWS_3D.get(what, ()) or option in ("candle", "violin"):
        return False
    if DATA_KINDS[kind][1] in ASYMMETRIC and option == "points_arrows" and "dashed" in style:
        return False
    return not (what == "hist1d" and has_errors(kind) and option == "HIST E2" and style == "hollow_split")


p = Project("legend_symbols")
p.inputs()
todo = cases({"kind": KINDS, "option": list(dict.fromkeys(sum(OPTIONS.values(), []))), "style": list(STYLES)}, valid=compared)
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "legend", "1d" if drawn_type(case["kind"]) in ("hist1d", "graph", "function") else "2d")
    data = add_data(plot[1], case["kind"]).SetLegendLabel("entry")
    if hasattr(S, case["option"]):
        data.SetDrawOption(getattr(S, case["option"]))
    elif case["option"] != "default":
        data.SetDrawOption(case["option"])  # a ROOT option
    STYLES[case["style"]](data)
    plot[1].AddLegend(0.6, 0.9)
    p.pm.AddPlot(plot)
run = p.plot("legend")

# ---------------------------------------------------------------------------------------------
# the elements of the SVG of a canvas
# ---------------------------------------------------------------------------------------------
STYLE_ATTRS = ("fill", "fill-opacity", "stroke", "stroke-width", "stroke-dasharray", "stroke-opacity", "stroke-linecap")
TAG_RE = re.compile(r"<(/?)(\w+)([^>]*?)(/?)>", re.S)
ATTR_RE = re.compile(r'([\w-]+)="([^"]*)"')
SVG = f"{WORK}/canvas.svg"
ROOT.gErrorIgnoreLevel = ROOT.kWarning


def width(d):
    """horizontal extent of an SVG path"""
    x, xs = 0.0, []
    for cmd, args in re.findall(r"([MmLlHhVvCc])([^MmLlHhVvCc]*)", d):
        nums = [float(n) for n in re.findall(r"-?[\d.]+(?:e-?\d+)?", args)]
        if not nums:
            continue
        if cmd in "Mm":
            x = nums[0] if cmd == "M" else x + nums[0]
            xs.append(x)
            for dx in nums[2::2]:  # implicit lineto
                x += dx
                xs.append(x)
        elif cmd in "LlCc":
            for value in nums[0::2]:
                x = value if cmd in "LC" else x + value
                xs.append(x)
        elif cmd in "Hh":
            for value in nums:
                x = value if cmd == "H" else x + value
                xs.append(x)
    return max(xs) - min(xs) if xs else 0.0


def elements(svg):
    """the visible elements: (style, raw attributes, horizontal extent if it is a piece of a line)"""
    patterns = {}
    for m in re.finditer(r'<pattern id="([^"]+)"(.*?)</pattern>', svg, re.S):
        patterns[m.group(1)] = "pattern:" + hashlib.md5(re.sub(r'id="[^"]*"', "", m.group(2)).encode()).hexdigest()[:6]
    found = collections.Counter()
    groups = [{}]
    for closing, tag, attrs, selfClosing in TAG_RE.findall(svg.split("</defs>", 1)[-1]):
        if tag in ("svg", "title", "desc", "defs", "text", "tspan"):
            continue
        if closing:
            if tag == "g" and len(groups) > 1:
                groups.pop()
            continue
        style = dict(groups[-1])
        style.update({k: v for k, v in ATTR_RE.findall(attrs) if k in STYLE_ATTRS})
        if tag == "g":
            if not selfClosing:
                groups.append(style)
            continue
        for k, v in style.items():
            style[k] = patterns.get(v[5:-1], v) if v.startswith("url(#") else " ".join(v.split())
        if style.get("stroke", "none") == "none" or float(style.get("stroke-opacity", 1)) == 0:
            style = {k: v for k, v in style.items() if not k.startswith("stroke")}
        if style.get("fill", "black") == "none" or float(style.get("fill-opacity", 1)) == 0:
            style["fill"] = "none"
            style.pop("fill-opacity", None)
        if style["fill"] == "none" and "stroke" not in style:
            continue
        shape = "circle r=%.1f" % float(re.search(r'r="([^"]+)"', attrs).group(1)) if tag == "circle" else "shape"
        d = re.search(r' d="([^"]*)"', attrs)
        d = d.group(1) if d else ""
        # a piece of a line: one connected path of at least two segments (ROOT splits long lines into several paths)
        segments = len(re.findall(r"[-\d.]+,\s*[-\d.]+", d)) - 1 + len(re.findall(r"[hvHV]", d))
        piece = width(d) if "stroke" in style and d.count("M") == 1 and segments >= 2 and width(d) >= 15 else 0.0
        found[((shape, *sorted(style.items())), attrs.strip(), piece)] += 1
    return found


def paint(name, keep):
    """the elements drawn for the primitives of the pad that are kept, and the options of the legend entries"""
    canvas = run.canvas(name, "legend")  # a fresh copy from the file
    if not canvas:
        return None, []
    ROOT.SetOwnership(canvas, True)  # deleted when done: thousands of canvases are read
    primitives = canvas.GetPrimitive("Pad_1").GetListOfPrimitives()
    for obj in list(primitives):
        if obj.InheritsFrom("TLegend"):
            obj.SetFillStyle(0)
            obj.SetBorderSize(0)
        # left out in all three: ROOT compiles the command of a TExec at every paint and keeps the code (memory runs out over thousands
        # of paints), and the styles here use only ROOT's preset colours, which need no TExec
        if not keep(obj) or obj.InheritsFrom("TExec"):
            primitives.Remove(obj)
    canvas.SaveAs(SVG)
    entryOptions = [entry.GetOption().lower() for obj in primitives if obj.InheritsFrom("TLegend") for entry in obj.GetListOfPrimitives()]
    del canvas
    with open(SVG) as svg:
        return elements(svg.read()), entryOptions


def is_frame(obj):
    """everything in the pad but the data (named <data number>:..., its fill fill_<data number>:...) and the legend: frame, axes, grids (also the polar one), colour scale"""
    return not obj.InheritsFrom("TLegend") and not re.match(r"(fill_)?\d+:", obj.GetName())


def show(style):
    return style[0] + " " + " ".join(f"{k}={v}" for k, v in style[1:])


def legend_check(i, case):
    """the check of a case: (title, passed, detail)"""
    what = describe(case)
    frame = paint(f"c{i}", is_frame)[0]
    if frame is None:
        return f"plot made: {what}", False, " | ".join(run.lines("ERR")[:2])
    dataElements = paint(f"c{i}", lambda o: not o.InheritsFrom("TLegend"))[0] - frame
    legendElements, entryOptions = paint(f"c{i}", lambda o: is_frame(o) or o.InheritsFrom("TLegend"))
    legendElements -= frame
    option = entryOptions[0] if entryOptions else ""
    dataStyles, legendStyles = {e[0] for e in dataElements}, {e[0] for e in legendElements}
    onlyPlot, onlyLegend = dataStyles - legendStyles, legendStyles - dataStyles
    lineLength = collections.Counter()
    for e in dataElements:
        lineLength[e[0]] += e[2]
    plotLine = any(length > 100 for length in lineLength.values())
    problem = ""
    if drawn_type(case["kind"]) in ("hist1d", "graph", "function"):  # contour lines and meshes are many short pieces
        if plotLine and "l" not in option and "f" not in option:
            problem = f"the plot draws a line through the points, the legend has none (option '{option}')"
        if "l" in option and not plotLine:
            problem = f"the legend shows a line (option '{option}'), the plot draws none"
    colours = {dict(s[1:]).get("fill") for s in onlyPlot} | {dict(s[1:]).get("stroke") for s in onlyPlot}
    if option == "" and not onlyLegend and len(colours) >= 5:
        onlyPlot = set()  # coloured by the palette: the entry has no symbol
    if case["option"] == "points_arrows":
        onlyPlot = set()  # the arrow heads cannot be drawn by TLegend
    detail = "; ".join([*(f"only in the plot: {show(s)}" for s in sorted(onlyPlot)), *(f"only in the legend: {show(s)}" for s in sorted(onlyLegend)), *([problem] if problem else [])])
    return f"legend symbol shows the drawn data: {what}", not detail, detail


in_chunks(todo, lambda i, case: [legend_check(i, case)])
finish()
