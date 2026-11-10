#!/usr/bin/env python3
"""Generates docs/skill/scirooplot/reference/api.md from the public interface in include/SciRooPlot.

The file lists every public method of the user-facing classes, one per line, as declared in the headers.
It is committed to the repository; the CI compares it with a freshly generated version so that it cannot drift.
Also writes the drawing options (drawing-options.md) and the version of CMakeLists.txt into SKILL.md.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADERS = ["Plot.h", "PlotManager.h", "PanelLayout.h"]
OUTPUT = ROOT / "docs/skill/scirooplot/reference/api.md"
OPTIONS_OUTPUT = ROOT / "docs/skill/scirooplot/reference/drawing-options.md"
SKILL = ROOT / "docs/skill/scirooplot/SKILL.md"

# classes in the order they are listed; (header, declaration, title, remark)
CLASSES = [
    ("PlotManager.h", "class PlotManager", "PlotManager", "central object: data sources, plots, base plots, project files"),
    ("Plot.h", "class Plot", "Plot", "plot[padID] gives the Pad (padID 0: defaults for all pads); also has the fill setters"),
    ("Plot.h", "class Plot::Pad", "Plot::Pad", "pad[axis] gives the Axis ('X', 'Y', 'Z', and 'C' for the content of a 3d histogram: its colour scale); pad(dataID) the Data (counting from 1); also has the fill setters"),
    ("Plot.h", "class Plot::Pad::Axis", "Plot::Pad::Axis", ""),
    ("Plot.h", "class Plot::Pad::Data", "Plot::Pad::Data", "every setter returns the Data, so calls can be chained; also has the marker, line and fill setters"),
    ("Plot.h", "class Plot::Pad::Ratio", "Plot::Pad::Ratio", "has all setters, modifiers and selections of Data (returning the Ratio) except Scatter(x, y, z), plus the following"),
    ("Plot.h", "class Plot::Pad::Box", "Plot::Pad::Box<BoxType>", "common to TextBox, LegendBox and StatsBox, which also have the border, text and fill setters"),
    ("Plot.h", "class Plot::Pad::TextBox", "Plot::Pad::TextBox", ""),
    ("Plot.h", "class Plot::Pad::ImageBox", "Plot::Pad::ImageBox", "an image file (e.g. a logo); in pdf files only with ROOT 6.42 or newer"),
    ("Plot.h", "class Plot::Pad::StatsBox", "Plot::Pad::StatsBox", "placement and appearance of the statistics box of a data item (Data::SetStatsBox); has the setters of Box"),
    ("Plot.h", "class Plot::Pad::LegendBox", "Plot::Pad::LegendBox", ""),
    ("Plot.h", "class Plot::Pad::Shape", "Plot::Pad::Shape", "a line, arrow, region, ellipse or polygon that is not data (Pad::AddLine, AddArrow, AddVerticalLine, AddRegionX, ...); also has the line and fill setters"),
    ("Plot.h", "class Plot::Pad::LegendEntry", "Plot::Pad::LegendEntry", "a legend row: the appearance of the row of a data item (Data::GetLegendEntry, SetLegendEntry) or a row added with LegendBox::AddEntry; also has the marker, line, fill and text setters"),
    ("Plot.h", "class MarkerSetters", "marker setters", "Data, Ratio, LegendEntry; Derived is the class itself, so calls can be chained"),
    ("Plot.h", "class LineSetters", "line setters", "Data, Ratio, LegendEntry, Shape"),
    ("Plot.h", "class FillSetters", "fill setters", "Plot, Pad, Data, Ratio, LegendEntry, TextBox, LegendBox, StatsBox, Shape"),
    ("Plot.h", "class TextSetters", "text setters", "LegendEntry, TextBox, LegendBox, StatsBox"),
    ("Plot.h", "class BorderSetters", "border setters", "TextBox, LegendBox, StatsBox"),
    (
        "PanelLayout.h",
        "class Panel",
        "Panel",
        "a cell of a PanelLayout, in units of the standard panel; `Gap()` separates independent panels, `Empty()` leaves a cell without axes, `Empty(width, height)` one with a size of its own, `Hole()` (or `Hole(width, height)`) an empty cell without pad (no pad number), `ColorScale()` (or `ColorScale(padID)`) a cell holding one colour scale for the panels sharing the z axis with the panel left of it (or with pad padID)",
    ),
    ("PanelLayout.h", "class PanelLayout", "PanelLayout", "arrangement of panels and the sizes shared by all base plots"),
    ("PlotManager.h", "class BasePlotShape", "BasePlotShape", "proportions of the panels of PlotManager::MakeBasePlot"),
]

METHOD = re.compile(r"^\s*(?:virtual\s+|static\s+|explicit\s+)*(?:(?P<ret>[\w:<>&\*,\s]+?)\s+)?(?P<name>\w+)\((?P<args>.*)\)\s*(?:const\s*)?(?:override\s*)?(?:=\s*default\s*)?(?:\{.*\}|;)")
SKIP_NAMES = {"Print", "GetPropertyTree", "GetTree", "SetTree", "operator=", "Clone"}
# argument types that are built from literals in user code
TYPES = {
    "data_dim_t": '`"var"` (100 automatic bins; integer columns: bins of whole-number width), `{"var", nBins}`, `{"var", nBins, {min, max}}` or `{"var", {edges...}}` (Python: tuples)',
    "join_tree_t": '`"tree"` (same data source) or `{"tree", "dataSource"}` (Python: list)',
    "color_t": 'a ROOT colour (`kRed + 1`, `632`), `"#rrggbb"` or `{r, g, b}` from 0 to 1 (Python: tuple); transparency with the alpha setters',
}


def public_section(text: str, declaration: str) -> str:
    """the public part of the class body (up to the first protected/private label)"""
    start = text.find(declaration + "\n")
    if start < 0:
        start = text.find(declaration + " ")
    if start < 0:
        sys.exit(f"{declaration} not found")
    body = text[start:]
    end = re.search(r"^\s*(protected|private):", body, re.M)
    return body[: end.start()] if end else body


def split_args(args: str):
    """the arguments of a declaration, split at the top-level commas"""
    parts, depth, current = [], 0, ""
    for char in args:
        depth += (char in "<({[") - (char in ">)}]")
        if char == "," and depth == 0:
            parts.append(current)
            current = ""
        else:
            current += char
    return [part.strip() for part in parts + [current] if part.strip()]


def type_key(types):
    return tuple(re.sub(r"\bconst\b|&|std::|\s", "", t) for t in types)


def python_constructors():
    """argument types of the constructors bound in python/PythonBindings.cpp, per class"""
    text = (ROOT / "python/PythonBindings.cpp").read_text()
    result = {}
    for block in re.split(r"(?=py::class_<)", text)[1:]:
        name = re.match(r"py::class_<(\w+)", block)[1]
        for args in re.findall(r"py::init<([^()]*)>\(\)", block):
            result.setdefault(name, set()).add(type_key(split_args(args)))
    return result


def ctor_key(args: str):
    """argument types of a constructor declaration (defaults and names removed)"""
    types = []
    for arg in split_args(args):
        arg = arg.split("=", 1)[0].strip()
        types.append(re.sub(r"\s*\b\w+$", "", arg))
    return type_key(types)


def methods(section: str, class_name: str):
    result = []
    # macro definitions (with their continuation lines) and macro invocations like FORWARD_TO_DATA(SetColor)
    # are not declarations; the methods they generate are covered by the class remark
    declarations = []
    inMacro = False
    for line in section.splitlines():
        stripped = line.strip()
        if inMacro or stripped.startswith("#"):
            inMacro = stripped.endswith("\\")
            continue
        if re.fullmatch(r"[A-Z_]+\(\w+\)", stripped) or re.fullmatch(r"(public|protected|private):", stripped):
            continue
        declarations.append(line)
    # join declarations that span several lines
    lines = []
    for line in declarations:
        if lines and not re.search(r"[;{}]\s*(//.*)?$", lines[-1]) and not lines[-1].strip().startswith("//"):
            lines[-1] = lines[-1].rstrip() + " " + line.strip()
        else:
            lines.append(line)
    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith(("//", "#", "friend", "using", "template", "class", "struct", "enum")):
            continue
        decl = line.split("//", 1)[0]  # a comment may hold parentheses too
        if re.search(r"\{\s*return\s+\w+<", decl):
            continue  # Ratio's forwards to the templates taking the arguments of the Data setters: covered by the remark of the class
        decl = re.sub(r"\)\s*(const\s*)?\{.*\}\s*;?\s*$", r") \1;", decl)  # an inline body may hold parentheses too
        match = METHOD.match(decl)
        if not match:
            continue
        name = match["name"]
        if name in SKIP_NAMES or name.startswith("Get") and name not in {"GetAxis", "GetData", "GetLegend", "GetLegendEntry", "GetText", "GetShape", "GetImage", "GetProjectProperty", "GetPad"}:
            continue  # plain getters are not part of the user interface
        ret = (match["ret"] or "").strip()
        if re.match(r"\s*static\s", decl):
            ret = "static " + ret
        args = re.sub(r"\s+", " ", match["args"].strip())
        short = class_name.split("::")[-1].split("<")[0]
        if "ptree" in args or args in (f"const {short}& other", f"{short}&&") or name.endswith("_t"):
            continue  # internal, copy and move constructors; helper structs are described in TYPES
        if "initializer_list" in args:
            continue  # overloads for braced lists; the std::vector overloads cover them
        comment = ""
        if "//" in line:
            comment = line.split("//", 1)[1].strip()
        if ret == "" and name == short:
            if ctor_key(args) not in PY_CTORS.get(short, set()):
                comment = f"{comment}; C++ only" if comment else "C++ only"
            result.append((f"{name}({args})", comment, True))
        else:
            result.append((f"{ret} {name}({args})", comment, False))
    return result


PY_CTORS = python_constructors()


def main():
    texts = {h: (ROOT / "include/SciRooPlot" / h).read_text() for h in HEADERS}
    out = [
        "# SciRooPlot API reference",
        "",
        "Generated by `scripts/generate-api-reference.py` from the headers in `include/SciRooPlot` — do not edit by hand.",
        "Types: `std::optional<T>` arguments can be left out (Python: `None`); `std::vector` arguments take `{...}` in C++ and lists in Python;",
        "`draw_option_t` is one of the aliases listed in `drawing-options.md`; colours, styles and fonts are ROOT constants (`kRed + 1`, `kFullCircle`, `kDashed`, 43).",
        "",
    ]
    for header, declaration, title, remark in CLASSES:
        section = public_section(texts[header], declaration)
        out.append(f"## {title}")
        if remark:
            out.append(f"*{remark}*")
        out.append("")
        used_types = set()
        for signature, comment, is_ctor in methods(section, title):
            line = f"- `{signature}`"
            if comment:
                line += f" — {comment}"
            out.append(line)
            used_types |= {t for t in TYPES if t in signature}
        for t in sorted(used_types):
            out.append(f"- `{t}`: {TYPES[t]}")
        out.append("")
    # the drawing option aliases and placements are part of the interface too
    block = texts["Plot.h"][texts["Plot.h"].index("#define DRAW_OPTIONS") : texts["Plot.h"].index("enum draw_option_t")]
    aliases = re.findall(r"OPT\((\w+)", block)
    out += ["## Enumerations", "", "- `draw_option_t`: " + ", ".join(f"`{a}`" for a in aliases), "- `box_placement_t`: `best_corner`, `top_left`, `top_right`, `bottom_right`, `bottom_left`", ""]
    # the skill names the version it describes, so agents can compare it with the installed one
    version = re.search(r"project\(\s*SciRooPlot\s+VERSION\s+([\d.]+)", (ROOT / "CMakeLists.txt").read_text())[1]
    skill = re.sub(r"^Describes SciRooPlot \S+", f"Describes SciRooPlot {version}", SKILL.read_text(), count=1, flags=re.M)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    for path, content in ((OUTPUT, "\n".join(out)), (OPTIONS_OUTPUT, drawing_options(aliases)), (SKILL, skill)):
        if "--check" in sys.argv:
            if not path.exists() or path.read_text() != content:
                sys.exit(f"{path.relative_to(ROOT)} is outdated: run scripts/generate-api-reference.py")
            continue
        path.write_text(content)
        print(f"wrote {path.relative_to(ROOT)}")


def drawing_options(aliases):
    """alias -> ROOT option string per data type, from the maps in src/PlotPainter.h"""
    text = (ROOT / "src/PlotPainter.h").read_text()
    maps = {}
    for name in ("Hist", "Graph", "Func", "Hist2d", "Graph2d", "Func2d", "Hist3d", "Func3d"):
        block = text[text.index(f"defaultDrawOptions_{name}{{") :]
        block = block[: block.index("};")]
        maps[name] = {alias: option for alias, option in re.findall(r"\{(\w+), \"([^\"]*)\"\}", block)}
    out = [
        "# SciRooPlot drawing option aliases",
        "",
        "Generated by `scripts/generate-api-reference.py` from `src/PlotPainter.h` — do not edit by hand.",
        "`SetDrawOption(<alias>)` expands to the ROOT drawing option of the data type; `SetDrawOption(\"<string>\")` passes a raw ROOT option instead (`\"\"`: ROOT's own default); `SetDrawOption()` clears it, so the rule below applies again.",
        "Histograms and graphs share all 1D aliases: where ROOT has no option for one of them, the data is converted and drawn as the other type",
        "(*as graph* / *as histogram*). An alias a type cannot draw at all gives a WARNING and ROOT's default.",
        "Without a drawing option the same rule applies to histograms and graphs: data with a fill is drawn as `hist` (graphs: `area`),",
        "data with a marker style or size set, with non-zero errors or a graph of a single point as `points`, other data as `hist` (graphs: `line`); 2D histograms, graphs and functions, and 3D histograms as `colz`.",
        "",
        "| alias | 1D histogram | graph | function | 2D histogram | 2D graph | 2D function | 3D histogram | 3D function |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for alias in aliases:
        h, g = maps["Hist"].get(alias), maps["Graph"].get(alias)
        others = [maps[name].get(alias) for name in ("Func", "Hist2d", "Graph2d", "Func2d", "Hist3d", "Func3d")]
        if h is None and g is not None:
            h = "*as graph*"
        elif g is None and h is not None:
            g = "*as histogram*"
        cells = [f"`{o}`" if o and not o.startswith("*") else (o or "–") for o in (h, g, *others)]
        out.append(f"| `{alias}` | {' | '.join(cells)} |")
    out += [
        "",
        "Appearance details beyond the ROOT string: `points`, `points_endcaps`, `points_line`, `points_text`, `points_arrows` hide the x errors of graphs (as X0 does for histograms);",
        "`points_text` writes the values above the error bars; `points_arrows` heads are sized by ROOT from the marker size only (not the line width)",
        "and filled with the fill colour (a transparent fill makes them pale); `boxes_nomarkers` removes the markers; `line`/`curve` of histograms and functions are not filled;",
        "a function is drawn from its values at `SetNpx` points: with a fill set, `area`/`area_smooth` (and ROOT's own options) fill the area between the curve and zero;",
        "2D graphs and 2D functions are drawn from a histogram interpolated (sampled) on a grid, so `box`, `text`, `colz_text`, candles and violins are left out;",
        "`colz`/`col` leave out cells below the minimum of the colour scale (a z range or the floor of a log z axis) and paint those above the maximum",
        "in the top colour (ROOT's default); `colz_clamped`/`col_clamped` paint the cells below in the lowest colour too; empty cells stay empty in all of them",
        "(for a TH2Poly ROOT's `0` has another meaning: see its documentation). A TH2Poly is drawn flat only (ROOT has no lego or surface for polygon bins",
        "outside OpenGL): lego and surf options fall back to `col`/`colz` with a warning, and a TH2Poly after a 3D frame is not drawn.",
        "3D histograms are drawn in 3D: `box` a box per cell with a volume following the content, `col`/`colz` equal boxes coloured by the content, `iso` the iso surface",
        "at the mean content of the cells (`SetView` turns them); the colour range follows the contents (the Z axis of a 3D histogram is its third binned axis).",
        "3D functions (TF3) are drawn as the surface f(x, y, z) = 0 (`iso`, also without an option; write f - c for the surface at the value c),",
        "with white faces and the mesh in the data's colour, or faces in the fill colour if the data sets one.",
        "the triangle and points aliases of 2D graphs (and `line`, the polyline through the points) are drawn in 3D: as the first data of a pad they get an empty lego frame;",
        "`points` of a graph with errors also draws the error bars. Data drawn in 3D (lego, surf, triangles, the points and lines of 2D graphs, 3D histograms and functions)",
        "needs the 3D frame of a first data drawn in 3D, flat data (colours, contours, boxes, text) a flat frame: otherwise a WARNING and the data is not drawn (the next data",
        "keeps its number);",
        "`area` of graphs is the polygon between the points and zero (points not in the order of x are sorted, with a warning); `candle` is ROOT preset 2 (box, median, whiskers at 1.5 IQR, outliers), `candle_minmax` preset 1 (whiskers to the extremes).",
        "`hist_clipped`, `hist_open_clipped`, `curve_clipped` draw a histogram as a graph, whose lines ROOT cuts at the frame (`HIST`, and `HIST C` on a log axis,",
        "move bins outside of the frame onto its edge); `hist_clipped` and `hist_open_clipped` are the steps of `hist` and `hist_open` cut at the frame: the outer edges and",
        "a fill go down to the same base line (0, or the lower frame edge if that is above 0 or on a log axis), and `hist_open_clipped` leaves out the bins at the ends",
        "that ROOT's `HIST ][` leaves out (on the base line, or below it if it is the lower frame edge; none if all are);",
        "a graph is drawn by `hist_clipped`/`hist_open_clipped` as the steps of its bins and by `curve_clipped` like by `curve`; the y range of the data does not apply to histograms (a warning says so).",
        "The `_polar` aliases draw x as the angle (the x range is one full turn, oriented as by ROOT's `POL` option) and y as the radius: `col_polar`/`colz_polar` fill a disc",
        "in a square frame (the margins of the longer side grow) without the cartesian axes, measured by a radial axis below it with the title of y; their cells have no outline",
        "unless the data sets a line width; `lego_polar`, `lego_col_polar`, `surf_polar`, `surf_col_polar` are drawn on a disc in 3D (`SetView` turns it).",
        "A polar graph (TGraphPolar: x the angle in radians, y the radius) is drawn on a polar grid instead of a cartesian frame. The grid fills the pad (a square base plot keeps it round)",
        "and takes the label fonts, sizes and colours of the x (angle) and y (radius) axes and the ranges and divisions set on them; the points aliases draw its error bars; shapes are not drawn there;",
        "as in ROOT, the raw option of the first polar graph of the pad also sets the grid: `D` or `G` for an angle in degrees or grads (a full turn), `N` without angle labels,",
        "`O` with the angle labels along the radius (`\"N\"` or `\"O\"` alone draws the graph as a line, as ROOT's empty option);",
        "automatically placed legends and texts keep off the grid and its angle labels, so they go into the corners of the pad (a box that does not fit there gets a warning).",
        "Maps: a 2D histogram in degrees drawn with a raw projection option (`\"AITOFF\"`, `\"MOLLWEIDE\"`, `\"SINUSOIDAL\"`, `\"PARABOLIC\"`, `\"MERCATOR\"`) is coloured",
        "over the z range of the pad; add `Z` (`\"MOLLWEIDE Z\"`) for a colour scale, which ROOT itself does not draw for projections; not `COL`, which ROOT also",
        "paints unprojected below the map. The map covers the range of the frame: restrict it on the pad (`plot[1]['Y'].SetRange(45, 90)` for the Arctic), a range",
        "on the data is ignored with a warning; data drawn after a map must be a map of the same projection (otherwise a WARNING and it is not drawn).",
        "As in ROOT, the x and y axes show the projected coordinates: on a whole-sphere map they match longitude and latitude",
        "at its edges, on a part of the sphere not (hide them with `SetLabelSize(0)`, `SetTickLength(0)` and `SetAxisColor(kWhite)`: ROOT draws transparent",
        "lines opaque in png); `\"MERCATOR\"` gives the latitude in units of",
        "ln tan(π/4 + b/2) and needs latitudes strictly inside ±90 (otherwise ROOT draws the map unprojected).",
        "ROOT paints a projected map as contours between the bin centres, so it ends half a bin inside the frame: use bins that are small against the range",
        "(on a section with coarse bins the half bin shows as a blank strip). Regions such as countries are a TH2Poly with own polygons, drawn as a plain colour map.",
        "",
    ]
    return "\n".join(out)


if __name__ == "__main__":
    main()
