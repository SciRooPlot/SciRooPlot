"""Every file mode writes one file per plot, <output directory>/<group>/<plot>.<mode>, in its format and holding the plot: for every data
kind of srptest in every variant (every value at least once in the fast run; the normal variant of every kind in every mode always).
Checked by the structure of each format: pdf (one page, shown in the canvas' aspect: ROOT turns the page of a wide canvas), eps (bounding
box of the canvas' aspect), ps (one page), svg (an svg document of the canvas' aspect), png and jpg (the canvas' pixels at bitmapscale 1),
html (a page holding the canvas), and root, xml and json, whose canvas must be the one of mode file (Plots.root): the same primitives with
the same drawing options, in every sub-pad. The json canvas has no colour reservation (a TExec only a canvas read back by ROOT needs) and
keeps a histogram drawn twice as one object; ROOT writes infinite values into json as 2e308 (JSROOT reads them as infinity) and NaN as
null, which its own reader refuses: they are read back as the largest double and as 0.
Images (Pad::AddImage) are in every format: svg embeds a png, eps and ps a cell array, the bitmaps show the image's pixels, root, xml and
json hold it in its sub-pad; pdf only from ROOT 6.42 on, before which one warning per run says so (instead of ROOT's line per image).
The same for multi-panel plots (srptest's LAYOUTS: a grid, ratio panels, a gap with an explicit group, a legend strip with the rows of
other pads, a flat panel beside a 3D view): as pdf of every data kind, in every mode of a 1d and a 2d histogram (normal variant; the
variants of the data are the single-pad plots' part).
No mode gives errors, or warnings mode file does not give for the same data.
Mode html needs ROOT 6.42 (before, ROOT writes PostScript under that name): with an older ROOT it is refused with an error and writes
nothing."""

import json
import os
import re
import shutil
import xml.etree.ElementTree as ElementTree

from srptest import DATA_KINDS, FILE_MODES, LAYOUT_BASES, LAYOUTS, VARIANTS, Project, ROOT, S, add_data, base_of, bitmap, canvas_tree, cases, check, describe, finish, has_layout, has_variant, layout_plot, read_canvas, rgb, source_of

ROOT_642 = ROOT.gROOT.GetVersionInt() >= 64200
MODES = tuple(mode for mode in FILE_MODES if ROOT_642 or mode != "html")
p = Project("file_formats", bases=("1d", "2d", *LAYOUT_BASES))
p.inputs(VARIANTS)
todo = cases({"mode": MODES, "kind": tuple(DATA_KINDS), "variant": VARIANTS}, valid=lambda c: has_variant(c["kind"], c["variant"]), always=lambda c: c["variant"] == "normal")
plots = {}  # plot name: its case
for i, case in enumerate(todo):
    plots[f"{case['mode']}_{i}"] = case
    plot = S.Plot(f"{case['mode']}_{i}", "formats/sub", base_of(case["kind"]))
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    p.pm.AddPlot(plot)
# multi-panel plots; named: the plots of the first test of the output of layouts (maps, legend strip, 1d grid, 3D views)
multi = cases(
    {"mode": MODES, "kind": tuple(DATA_KINDS), "layout": LAYOUTS},
    valid=lambda c: has_layout(c["kind"], c["layout"]) and (c["mode"] == "pdf" or c["kind"] in ("hist", "hist2d")),
    named=[{"mode": "pdf", "kind": kind, "layout": layout, "why": "multi-panel plots: maps, legend strip, grid, 3d views"} for kind, layout in (("hist2d", "grid"), ("hist", "strip"), ("hist", "grid"), ("hist2d", "view"))],
)
for i, case in enumerate(multi):
    plots[f"{case['mode']}_L{i}"] = case
    p.pm.AddPlot(layout_plot(f"{case['mode']}_L{i}", "formats/sub", case["kind"], case["layout"]))

logo = p.path("logo.png")  # 300 x 100 pixels of one colour
LOGO_COLOUR = ROOT.kAzure + 2
c = ROOT.TCanvas("logo", "", 300, 100)
c.SetFillColor(LOGO_COLOUR)
c.SaveAs(logo)
for name in ("image1", "image2"):  # two plots with images: the pdf warning is given once per run
    plot = S.Plot(name, "images", "1d")
    add_data(plot[1], "hist")
    plot[1].AddImage(0.3, 0.8, logo).SetWidth(0.3)
    p.pm.AddPlot(plot)

reference = p.plot()  # mode file: the canvases of Plots.root, and the warnings about the data (its NaN and infinite values)
check("mode file: no errors", not reference.errors(), " | ".join(reference.errors()[:3]))


def aspect(width, height):
    return height / width if width else 0.0


def contents(path):
    with open(path, "rb") as f:
        return f.read()


def json_tree(canvas):
    """the primitives of a canvas as json keeps them: names without the number of a copy (axis_hist_pad_1_1), no colour reservation"""
    return [(depth, cls, re.sub(r"(axis_hist_pad_\d+)_\d+$", r"\1", name), opt) for depth, cls, name, opt in canvas_tree(canvas, reservation=False)]


def format_problem(mode, path, canvas):
    """what is wrong with the file of a plot ("" if nothing); canvas: the one of mode file"""
    data = contents(path)
    width, height = canvas.GetWw(), canvas.GetWh()
    if mode == "pdf":
        pages = len(re.findall(rb"/Type\s*/Page\b(?!s)", data))
        if not data.startswith(b"%PDF-") or not data.rstrip().endswith(b"%%EOF") or pages != 1:
            return f"no pdf of one page ({pages} pages)"
        box, turn = re.search(rb"/CropBox\s*\[\s*(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s*\]", data), re.search(rb"/Rotate\s+(\d+)", data)
        if not box:
            return "no crop box"
        x1, y1, x2, y2 = (float(v) for v in box.groups())
        w, h = (y2 - y1, x2 - x1) if turn and int(turn.group(1)) % 180 else (x2 - x1, y2 - y1)  # ROOT turns the page of a wide canvas
        return "" if abs(aspect(w, h) - aspect(width, height)) < 0.01 else f"page shown {w:.0f} x {h:.0f}, canvas {width} x {height}"
    if mode == "eps":
        box = re.search(rb"%%BoundingBox:\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)", data)
        if not data.startswith(b"%!PS-Adobe-") or b"EPSF" not in data.split(b"\n", 1)[0] or not box:
            return "no eps document with a bounding box"
        x1, y1, x2, y2 = (int(v) for v in box.groups())
        return "" if abs(aspect(x2 - x1, y2 - y1) - aspect(width, height)) < 0.01 else f"bounding box {x2 - x1} x {y2 - y1}, canvas {width} x {height}"
    if mode == "ps":
        return "" if data.startswith(b"%!PS-Adobe-") and data.rstrip().endswith(b"%%EOF") and re.search(rb"%%Pages:\s*1\b", data) else "no PostScript document of one page"
    if mode == "svg":
        try:
            svg = ElementTree.fromstring(data)
        except ElementTree.ParseError as e:
            return f"no xml: {e}"
        if not svg.tag.endswith("svg"):
            return f"root element {svg.tag}"
        w, h = float(svg.get("width", 0)), float(svg.get("height", 0))
        return "" if abs(aspect(w, h) - aspect(width, height)) < 0.01 else f"svg {w} x {h}, canvas {width} x {height}"
    if mode in ("png", "jpg"):
        pixels = bitmap(path)
        if pixels is None:
            return f"no {mode} image"
        return "" if pixels.shape == (height, width) else f"{pixels.shape[1]} x {pixels.shape[0]} pixels, canvas {width} x {height}"
    if mode == "html":
        text = data.decode(errors="replace")
        return "" if re.match(r"\s*<!DOCTYPE html>", text, re.I) and '"TCanvas"' in text else f"no html page with the canvas (starts {text[:30]!r})"
    stored = read_canvas(path)
    if not stored:
        return "no canvas in the file"
    want, got = (json_tree(canvas), json_tree(stored)) if mode == "json" else (canvas_tree(canvas), canvas_tree(stored))
    if got != want:
        diff = next((f"{g} instead of {w}" for w, g in zip(want, got) if w != g), f"{len(got)} primitives, expected {len(want)}")
        return f"canvas differs from Plots.root: {diff}"
    return ""


# ---------------------------------------------------------------------------------------------
# every mode, every kind and variant
# ---------------------------------------------------------------------------------------------
for mode in MODES:
    mine = [(name, case) for name, case in plots.items() if case["mode"] == mode]
    run = p.plot("formats/sub", f"{mode}_.*", mode)
    own = [line for line in run.warnings() if line not in reference.warnings()]
    check(f"mode {mode}: no errors, no warnings of its own", not run.errors() and not own, " | ".join((run.errors() + own)[:3]))
    folder = p.path("out", "formats", "sub")
    written = sorted(os.listdir(folder)) if os.path.isdir(folder) else []
    expected = sorted(f"{name}.{mode}" for name, _ in mine)
    check(f"mode {mode}: one file per plot in <output>/<group>", written == expected, f"{len(written)} files, expected {len(expected)}: {sorted(set(expected) ^ set(written))[:4]}")
    for name, case in mine:
        path = os.path.join(folder, f"{name}.{mode}")
        canvas = reference.canvas(name, "formats/sub")
        if not os.path.isfile(path) or not canvas:
            continue  # reported above
        problem = format_problem(mode, path, canvas)
        check(f"{mode} file holds the plot: {describe(case)}", not problem, problem)
if not ROOT_642:
    run = p.plot("images", ".+", "html")
    refused = [line for line in run.lines("ERR") if "Mode html needs ROOT 6.42" in line]
    check("mode html with ROOT before 6.42: refused with an error, nothing written", len(refused) == 1 and not os.path.exists(p.path("out")), f"{run.lines('ERR')[:2]}")

# ---------------------------------------------------------------------------------------------
# images
# ---------------------------------------------------------------------------------------------
logoPixels = 0  # pixels of the image in a bitmap of the plot: the area of its sub-pad
canvas = reference.canvas("image1", "images")
box = canvas.GetPrimitive("Pad_1").GetPrimitive("ImageBox_1") if canvas else None
if box:
    logoPixels = box.GetAbsWNDC() * canvas.GetWw() * box.GetAbsHNDC() * canvas.GetWh()
check("the image is drawn in its sub-pad ImageBox_1 (mode file)", logoPixels > 1000)


def image_problem(mode, path):
    """whether the image is in the file of a plot ("" if so)"""
    data = contents(path)
    if mode == "pdf":
        return "" if not ROOT_642 or re.search(rb"/Subtype\s*/Image", data) else "no image object"
    if mode in ("eps", "ps"):
        cells = re.search(rb"/NX\s*(\d+) def /NY\s*(\d+) def /DrawCell", data)
        return "" if cells and int(cells.group(1)) * int(cells.group(2)) > 0 else "no cell array"
    if mode == "svg":
        return "" if re.search(rb'<image[^>]*href="data:image/png;base64,', data) else "no embedded png"
    if mode in ("png", "jpg", "gif"):
        pixels = bitmap(path)
        colour = rgb(LOGO_COLOUR)
        if pixels is None:
            return "unreadable"
        diff = [abs(((pixels >> s) & 0xFF).astype(int) - ((colour >> s) & 0xFF)) for s in (16, 8, 0)]
        n = int(((diff[0] < 24) & (diff[1] < 24) & (diff[2] < 24)).sum())
        return "" if n > 0.8 * logoPixels else f"{n} pixels of the image's colour, its box has {logoPixels:.0f}"
    if mode == "html":
        return "" if b"TASImage" in data else "no image"
    if mode == "json":  # ROOT cannot read a TASImage back from json (its streamer is custom): its png is looked for in the text

        def pads(obj):
            if isinstance(obj, dict):
                if obj.get("_typename") == "TPad":
                    yield obj
                for value in obj.values():
                    yield from pads(value)
            elif isinstance(obj, list):
                for value in obj:
                    yield from pads(value)

        boxes = [pad for pad in pads(json.loads(data)) if pad.get("fName") == "ImageBox_1"]
        images = [prim for box in boxes for prim in box.get("fPrimitives", {}).get("arr", []) if prim.get("_typename") == "TASImage"]

        def is_png(blob):  # [<image kept>, <size>, [<bytes as signed chars>]]
            return isinstance(blob, list) and len(blob) == 3 and [b % 256 for b in blob[2][:4]] == [137, 80, 78, 71]

        return "" if any(is_png(image.get("_blob")) for image in images) else "no TASImage with a png in ImageBox_1"
    stored = read_canvas(path)
    tree = canvas_tree(stored) if stored else []
    return "" if any(cls == "TPad" and name == "ImageBox_1" for _, cls, name, _ in tree) and any(cls == "TASImage" for _, cls, _, _ in tree) else "no TASImage in ImageBox_1"


for mode in (*MODES, "gif"):
    run = p.plot("images", ".+", mode)
    files = {name: p.path("out", "images", f"{name}.{mode}") for name in ("image1", "image2")} if mode != "gif" else {"animation": p.path("out", "images.gif")}
    missing = [path for path in files.values() if not os.path.isfile(path)]
    noPdfImages = [line for line in run.warnings() if "Images are missing in the pdf files" in line]
    others = [line for line in run.warnings() if line not in noPdfImages]
    if mode == "pdf" and not ROOT_642:
        check("pdf with images: one warning per run that ROOT before 6.42 cannot write them", len(noPdfImages) == 1, f"{len(noPdfImages)} warnings")
        check("pdf with images: ROOT's own warning per image silenced", "PDF not implemented" not in run.log)
    else:
        check(f"{mode} with images: no warning that they are missing", not noPdfImages, noPdfImages[:1])
    check(f"{mode} with images: written, no errors or other warnings", not missing and not run.errors() and not others, " | ".join(missing + run.errors()[:2] + others[:2]))
    for name, path in files.items():
        if os.path.isfile(path):
            problem = image_problem(mode, path)
            check(f"{mode} file holds the image: {name}", not problem, problem)
    shutil.rmtree(p.path("out"), ignore_errors=True)
finish()
