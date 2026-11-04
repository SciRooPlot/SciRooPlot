"""Checks that every legend symbol shows what is drawn for its data (python3 compare.py <dir with the plots as .root files>).
Each canvas is printed to SVG without the legend, without the data and with neither (the frame): the styles of the elements drawn
for the data (fill, stroke, width, dashes, opacity, marker size) must be those drawn for the legend entry, and a line through the
points must have a line (or a fill outlined by it) in the legend and vice versa. Exit code 1 if a plot differs."""

import collections
import glob
import hashlib
import os
import re
import sys
import tempfile

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gErrorIgnoreLevel = ROOT.kWarning
STYLE_ATTRS = ("fill", "fill-opacity", "stroke", "stroke-width", "stroke-dasharray", "stroke-opacity", "stroke-linecap")
TAG_RE = re.compile(r"<(/?)(\w+)([^>]*?)(/?)>", re.S)
ATTR_RE = re.compile(r'([\w-]+)="([^"]*)"')
SVG = os.path.join(tempfile.mkdtemp(), "canvas.svg")

# differences TLegend cannot avoid, and ROOT options whose drawing rules the legend does not follow
EXPECTED = [
    (r"^(ge|ga)__(F|B)__", "TLegend draws no error bar with a fill symbol (F or B of graphs with errors)"),
    (r"^h_(err|noerr)__(starH|B|BAR)__", "the *H, B and BAR options of histograms (star markers, bar fill rules) are not followed"),
    (r"^h2__(lego|surf|candle|violin)__", "3d shading and the parts of candles and violins are shown by a fill or line"),
    (r"^ga__points_arrows__.*dashed", "ROOT draws arrowed asymmetric error bars solid (symmetric ones dashed)"),
    (r"^(ge|ga)__(3L|3PL|4C)__", "an opaque band hides the line and markers ROOT paints before it (in the SVG, not visible)"),
    (r"^h_err__HIST_E2__hollow_split", "TLegend outlines a hollow fill symbol in the fill colour, HIST E2 its hollow error boxes in the line colour"),
]


def Width(d):
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


def Elements(svg):
    """the visible elements: (style, raw attributes, horizontal extent if it is a piece of a line)"""
    patterns = {}
    for m in re.finditer(r'<pattern id="([^"]+)"(.*?)</pattern>', svg, re.S):
        patterns[m.group(1)] = "pattern:" + hashlib.md5(re.sub(r'id="[^"]*"', "", m.group(2)).encode()).hexdigest()[:6]
    elements = collections.Counter()
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
        piece = Width(d) if "stroke" in style and d.count("M") == 1 and segments >= 2 and Width(d) >= 15 else 0.0
        elements[((shape,) + tuple(sorted(style.items())), attrs.strip(), piece)] += 1
    return elements


def Paint(fileName, keep):
    """the elements drawn for the primitives of the pad that are kept, and the options of the legend entries"""
    inFile = ROOT.TFile(fileName)
    canvas = inFile.Get(inFile.GetListOfKeys()[0].GetName())
    pad = [o for o in canvas.GetListOfPrimitives() if o.InheritsFrom("TPad")][0]
    primitives = pad.GetListOfPrimitives()
    for obj in list(primitives):
        if obj.InheritsFrom("TLegend"):
            obj.SetFillStyle(0)
            obj.SetBorderSize(0)
        if not keep(obj):
            primitives.Remove(obj)
    canvas.SaveAs(SVG)
    options = [entry.GetOption().lower() for obj in primitives if obj.InheritsFrom("TLegend") for entry in obj.GetListOfPrimitives()]
    inFile.Close()
    with open(SVG) as svg:
        return Elements(svg.read()), options


def IsFrame(obj):
    return obj.InheritsFrom("TFrame") or obj.InheritsFrom("TExec") or obj.GetName().startswith("axis_hist")


def Format(style):
    return style[0] + " " + " ".join(f"{k}={v}" for k, v in style[1:])


def main():
    nDifferent = 0
    expected = collections.Counter()
    fileNames = sorted(glob.glob(os.path.join(sys.argv[1], "**", "*.root"), recursive=True))
    if not fileNames:  # nothing written: the check would pass without comparing anything
        print(f"No plots found in {sys.argv[1]}.")
        return 1
    for fileName in fileNames:
        name = os.path.basename(fileName)[:-5]
        frame = Paint(fileName, IsFrame)[0]
        dataElements = Paint(fileName, lambda o: not o.InheritsFrom("TLegend"))[0] - frame
        legendElements, options = Paint(fileName, lambda o: IsFrame(o) or o.InheritsFrom("TLegend"))
        legendElements -= frame
        option = options[0] if options else ""
        dataStyles, legendStyles = {e[0] for e in dataElements}, {e[0] for e in legendElements}
        onlyPlot, onlyLegend = dataStyles - legendStyles, legendStyles - dataStyles
        lineLength = collections.Counter()
        for e in dataElements:
            lineLength[e[0]] += e[2]
        plotLine = any(length > 100 for length in lineLength.values())
        problem = None
        if plotLine and "l" not in option and "f" not in option:
            problem = f"the plot draws a line through the points, the legend has none (option '{option}')"
        if "l" in option and not plotLine:
            problem = f"the legend shows a line (option '{option}'), the plot draws none"
        colours = {dict(s[1:]).get("fill") for s in onlyPlot} | {dict(s[1:]).get("stroke") for s in onlyPlot}
        if option == "" and not onlyLegend and len(colours) >= 5:
            onlyPlot = set()  # coloured by the palette: the entry has no symbol
        if name.startswith("h2__"):
            problem = None  # contour lines and meshes are many short pieces
        if name.split("__")[1] == "points_arrows":
            onlyPlot = set()  # the arrow heads cannot be drawn by TLegend
        if not (onlyPlot or onlyLegend or problem):
            continue
        reason = next((r for p, r in EXPECTED if re.search(p, name)), None)
        if reason:
            expected[reason] += 1
            continue
        nDifferent += 1
        print(f"== {name}")
        for style in sorted(onlyPlot):
            print("   only in the plot:   ", Format(style))
        for style in sorted(onlyLegend):
            print("   only in the legend: ", Format(style))
        if problem:
            print("   " + problem)
    for reason, n in expected.items():
        print(f"expected ({n} plots): {reason}")
    print(f"{nDifferent} plots with legend symbols that differ from the drawn data")
    return 1 if nDifferent else 0


sys.exit(main())
