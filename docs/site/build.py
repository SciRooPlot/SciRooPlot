"""Builds the web page: fills the template with the drawing options, the method list, the changes, the messages, the colours and
palettes, the example code and the description of the example data, and writes the logos next to it.
Usage: build.py <folder with palettes.json, examples.json, data.json> <out folder>  (build-site.sh makes those files and runs it)"""
import html as _html
import json
import os
import re
import subprocess
import sys
import textwrap

import numpy as np
from PIL import Image, ImageDraw

SITE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(SITE))
GEN, OUT = sys.argv[1], sys.argv[2]
DESC = {
    "points": "markers with y errors", "points_xerr": "markers with x and y errors", "points_endcaps": "markers, error bars with end caps",
    "points_xerr_endcaps": "x and y errors with end caps", "points_line": "markers joined by a line", "points_text": "markers with the values written above",
    "points_arrows": "arrow ends on the error bars, e.g. for limits", "line": "straight segments, no markers", "curve": "smooth curve, no markers",
    "curve_clipped": "smooth curve cut at the frame", "hist": "steps", "hist_open": "steps without the vertical lines at the ends",
    "hist_clipped": "steps cut at the frame instead of running along it", "hist_open_clipped": "open steps cut at the frame", "bars": "bars with gaps",
    "hbars": "horizontal bars", "area": "filled down to zero", "area_smooth": "smooth curve filled to zero", "band": "filled error band",
    "band_smooth": "smoothed error band", "boxes": "error boxes with markers", "boxes_nomarkers": "error boxes only", "brackets": "brackets at the error ends",
    "text": "the values written into the plot", "col": "colour map", "colz": "colour map with colour scale", "col_clamped": "colour map, cells below the minimum in the lowest colour", "colz_clamped": "clamped colour map with colour scale",
    "colz_text": "colour map with the values written into the cells", "box": "boxes sized by content",
    "iso": "iso surface at the mean content of the cells",
    "lego": "3D bars", "lego_col": "3D bars coloured by content", "lego_colz": "coloured 3D bars with colour scale", "lego_noborders": "3D bars without borders",
    "surf": "wire-frame surface", "surf_col": "surface lines coloured by content", "surf_colz": "coloured surface lines with colour scale",
    "surf_fill": "filled surface", "surf_fillz": "filled surface with colour scale", "surf_contours": "surface with contours drawn on top",
    "surf_shaded": "shaded surface", "col_polar": "colour map in polar coordinates: x the angle, y the radius",
    "colz_polar": "polar colour map with colour scale", "lego_polar": "3D bars on a disc", "lego_col_polar": "3D bars on a disc coloured by content",
    "surf_polar": "wire-frame surface over a disc", "surf_col_polar": "surface over a disc coloured by content", "triangles": "Delaunay triangles as filled surface", "triangles_wire": "Delaunay triangles as wire frame",
    "triangles_col": "triangles coloured by z, with edges", "triangles_colz": "coloured triangles with colour scale", "triangles_fill": "triangles coloured by z, no edges",
    "triangles_fillz": "coloured triangles without edges, with colour scale", "points_col": "3D markers coloured by z", "points_colz": "coloured 3D markers with colour scale", "cont": "contour lines", "cont_col": "contour lines coloured by level", "cont_colz": "coloured contour lines with scale",
    "cont_fill": "filled contours", "cont_fillz": "filled contours with colour scale", "cont_smooth": "smoothed filled contours",
    "cont_smoothz": "smoothed contours with colour scale", "candle": "box, median, whiskers at 1.5 IQR, outliers", "candle_minmax": "whiskers to the extremes",
    "candle_mean": "candle with the mean marked", "candle_notched": "notched candle", "candle_points": "candle with all points",
    "candle_scatter": "candle with a scatter of the points", "candle_meanline": "candle with mean line", "violin": "distribution per x bin",
    "violin_minmax": "violin with extremes marked",
}
opts = []
for line in open(f"{REPO}/docs/skill/scirooplot/reference/drawing-options.md"):
    m = re.match(r"^\| `(\w+)` \| (.*?) \| (.*?) \| (.*?) \| (.*?) \| (.*?) \| (.*?) \| (.*?) \|$", line)
    if m:
        clean = lambda c: c.strip().strip("`").replace("*", "")
        kinds = ["hist", "graph", "func", "hist2d", "graph2d", "func2d", "hist3d"]
        opts.append({"alias": m.group(1), **{k: clean(m.group(i + 2)) for i, k in enumerate(kinds)}, "desc": DESC.get(m.group(1), "")})
s = open(f"{SITE}/template.html").read()
examples = open(f"{GEN}/examples.json").read()
# every example card shows the plot its code makes (img/ex/<group>/<name>, as exImage in the template)
noImage = [k for k, e in json.loads(examples).items() if not os.path.isfile(f"{OUT}/img/ex/{k}.{'gif' if 'gif' in e else 'png'}")]
if noImage:
    sys.exit(f"no plot in {OUT}/img/ex for the examples: {', '.join(noImage)}")
s = s.replace("/*PALETTES*/", open(f"{GEN}/palettes.json").read()).replace("/*COLORS*/", open(f"{SITE}/colors.json").read()).replace("/*EXAMPLES*/", examples).replace("/*OPTIONS*/", json.dumps(opts))
macros = {}
for m in ["ptSpectrumRatio", "vertexZ", "ptSpectrumPublished", "pi0PeakVsPt", "trackingEfficiency", "trackSelection", "etaPhiDataVsSim", "nuclearModification", "jetSpectra"]:
    lines = open(f"{REPO}/docs/slides/root-examples/{m}.C").read().strip().split("\n")
    while lines and lines[0].startswith("//"):
        lines.pop(0)
    macros[m] = "\n".join(lines)
s = s.replace("/*ROOTMACROS*/", json.dumps(macros))
api, cur = [], None
for line in open(f"{REPO}/docs/skill/scirooplot/reference/api.md"):
    line = line.rstrip("\n")
    if line.startswith("## "):
        cur = {"cls": line[3:].strip(), "desc": "", "items": []}; api.append(cur)
    elif cur and line.startswith("*") and line.endswith("*") and not cur["items"]:
        cur["desc"] = line.strip("*")
    elif cur and line.startswith("- `"):
        m = re.match(r"^- `(.+?)`(?::\s*(.*)|\s+—\s+(.*))?$", line)
        sig, note = m.group(1), (m.group(2) or m.group(3) or "")
        note = note.replace("`", "")
        n = re.search(r"(operator\(\)|operator\[\]|~?\b[A-Za-z_]\w*)\s*\(", sig)
        name = n.group(1) if n else sig.split(":")[0]
        cur["items"].append({"name": name, "sig": sig, "note": note})
# a class without a label in CLASS_LABEL of the template stops the script of the page, and with it everything it renders
labels = re.findall(r'"([^"]+)":', re.search(r"const CLASS_LABEL = \{(.*?)\};", s, re.S).group(1))
missing = [c["cls"] for c in api if c["cls"] not in labels]
if missing:
    sys.exit(f"CLASS_LABEL in {SITE}/template.html has no label for: {', '.join(missing)}")
s = s.replace("/*API*/", json.dumps(api))
def inline(t):  # markdown inline → html: `code`, **bold**, *italic*
    t = _html.escape(t.replace("\\|", "|"), quote=False)
    t = re.sub(r"`([^`]+)`", r"<code>\1</code>", t)
    t = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", t)
    return re.sub(r"(?<![\w*])\*([^*]+)\*(?!\w)", r"<i>\1</i>", t)
changes, sec, grp, intro = [], None, None, []
for line in open(f"{REPO}/docs/api-changes.md"):
    line = line.rstrip("\n")
    if line.startswith("## "):
        title = line[3:].strip()
        sec = {"title": title, "from": title.split(" ")[0], "to": title.split("→")[1].strip(), "note": "", "groups": []}; changes.append(sec); grp = None
    elif line.startswith("### ") and sec:
        grp = {"title": line[4:].strip(), "rows": []}; sec["groups"].append(grp)
    elif line.startswith("| ") and grp is not None:
        cells = [c.strip() for c in re.split(r"(?<!\\)\|", line)[1:-1]]
        if len(cells) == 4 and cells[0] != "Old" and not set(cells[0]) <= set("-: "):
            grp["rows"].append({"old": inline(cells[0]), "new": inline(cells[1]), "kind": cells[2].strip(), "action": inline(cells[3])})
    elif sec and grp is None and line.strip() and not sec["note"]:
        sec["note"] = inline(line.strip())
    elif not sec and line.strip() and not line.startswith("#"):
        intro.append(inline(line.strip()))
s = s.replace("/*CHANGES*/", json.dumps({"intro": intro, "sections": changes}))
def blocks(path):  # the blocks between "# ---- <id>" and the next "# ----" marker of a file, dedented
    out, cur = {}, None
    for line in open(path):
        m = re.match(r"^\s*# ----(?: (\w+))?\s*$", line)
        if m:
            cur = m.group(1)
            if cur:
                out[cur] = []
        elif cur:
            out[cur].append(line)
    return {k: textwrap.dedent("".join(v)).strip("\n") for k, v in out.items()}
s = s.replace("/*NUMPY*/", json.dumps({**blocks(f"{SITE}/numpy/DefinePlots.py"), **blocks(f"{SITE}/numpy/FromFiles.py")}))
s = s.replace("/*MESSAGES*/", subprocess.run([sys.executable, f"{SITE}/extract_messages.py", REPO], capture_output=True, text=True, check=True).stdout)
s = s.replace("/*MSG_HELP*/", open(f"{SITE}/messages_help.js").read())
s = s.replace("/*EXDATA*/", open(f"{GEN}/data.json").read())
s = "<meta charset=\"utf-8\">\n" + s
open(f"{OUT}/index.html", "w").write(s)

# the logo: the one of the slides, its dark background made transparent for the dark hero area, and a small one for the header
os.makedirs(f"{OUT}/img/brand", exist_ok=True)
logo = Image.open(f"{REPO}/docs/slides/logo.png").convert("RGBA")
logo.resize((126, 96), Image.LANCZOS).save(f"{OUT}/img/brand/logo-small.png", optimize=True)
a = np.asarray(logo.resize((520, 398), Image.LANCZOS)).astype(float)
background = np.array([18.5, 25.0, 33.5])
alpha = np.clip((np.abs(a[..., :3] - background).max(-1) - 10) / 45, 0, 1) * a[..., 3] / 255
rgb = np.clip(background + (a[..., :3] - background) / np.where(alpha > 0, alpha, 1)[..., None], 0, 255)
Image.fromarray(np.dstack([rgb, alpha * 255]).round().astype(np.uint8), "RGBA").save(f"{OUT}/img/brand/logo.png", optimize=True)
# the icon of the browser tab (and of a bookmark on a phone's home screen): the bars and the wave of the logo without the name,
# which is unreadable at 32 pixels, on a rounded tile of the logo's dark colour, so it stands out on light and dark tab bars
emblem = logo.crop((235, 125, 1295, 640))
for size, name in ((32, "favicon.png"), (180, "apple-touch-icon.png")):
    drawn = 4 * size  # drawn larger and scaled down: smooth corners
    tile = Image.new("RGBA", (drawn, drawn), (0, 0, 0, 0))
    ImageDraw.Draw(tile).rounded_rectangle((0, 0, drawn - 1, drawn - 1), radius=drawn // 5, fill=(18, 25, 33, 255))
    w = round(0.92 * drawn)
    h = round(emblem.height * w / emblem.width)
    tile.alpha_composite(emblem.resize((w, h), Image.LANCZOS), ((drawn - w) // 2, (drawn - h) // 2))
    tile.resize((size, size), Image.LANCZOS).save(f"{OUT}/img/brand/{name}", optimize=True)
print(f"{OUT}/index.html: {len(opts)} drawing options, {len(s) // 1024} kB")
