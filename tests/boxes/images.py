"""An image (Pad::AddImage) is drawn at its place with the size asked for: as wide as set (a fraction of the frame, or of the pad in pad
coordinates; 0.2 by default), as high as its aspect ratio gives in pixels, or as high as set, or both; the picture fills that box in the
image the plot is saved as. A relative path is taken from where the plot is defined, an image of a base plot is drawn in its plots. A
missing file, an image beyond its pad and an image in pad 0 are warned about and left out, the plot is made. The image is in every file
format that can hold one (svg, eps, png); a pdf holds none before ROOT 6.42, which is said once per run.
Cases: png and jpg pictures wide and tall × the size set by width, height, both or neither × pad and frame coordinates × a wide and a
tall canvas (the sizes that reach beyond the pad: warned and left out), and the named cases.
Reference: the size of the image box against the size of the picture in pixels, and the picture's colour in the png of the plot.
Covers: images placed and sized, and the named cases.
"""

import os

from srptest import ROOT, WORK, Project, S, bitmap, cases, check, describe, finish

PICTURES = {"wide_png": (300, 100, "png"), "tall_png": (90, 270, "png"), "wide_jpg": (300, 100, "jpg")}
COLOUR = (0x12, 0x80, 0xC0)
SIZES = {"default": (None, None), "width": (0.3, None), "height": (None, 0.25), "both": (0.3, 0.25)}
CANVASES = {"wide": (1000, 600), "tall": (600, 1000)}

level, ROOT.gErrorIgnoreLevel = ROOT.gErrorIgnoreLevel, ROOT.kWarning
for name, (w, h, fmt) in PICTURES.items():
    canvas = ROOT.TCanvas(f"picture_{name}", "", w, h)
    canvas.SetFillColor(ROOT.TColor.GetColor(*COLOUR))
    canvas.Print(f"{WORK}/{name}.{fmt}")
ROOT.gErrorIgnoreLevel = level
SIZE = {}  # the size of each picture in pixels (a canvas is smaller than asked for by its window decorations)
for name, (_, _, fmt) in PICTURES.items():
    picture = ROOT.TImage.Open(f"{WORK}/{name}.{fmt}")
    SIZE[name] = (picture.GetWidth(), picture.GetHeight())

todo = cases({"picture": list(PICTURES), "size": list(SIZES), "system": ["frame", "pad"], "canvas": list(CANVASES)})

p = Project("images")
hist = ROOT.TH1D("h", "", 10, 0.0, 10.0)
for b in range(1, 11):
    hist.SetBinContent(b, 0.1)
p.source("s", {"h": hist})
POSITION = (0.25, 0.9)
for i, case in enumerate(todo):
    plot = S.Plot(f"i{i}", "images", "1d")
    plot.SetDimensions(*CANVASES[case["canvas"]])
    plot[1].AddData("h", "s")
    plot[1]["Y"].SetRange(0.0, 1.0)
    w, h, fmt = PICTURES[case["picture"]]
    image = plot[1].AddImage(*POSITION, f"{WORK}/{case['picture']}.{fmt}")
    if case["system"] == "pad":
        image.SetPadCoordinates()
    width, height = SIZES[case["size"]]
    if width:
        image.SetWidth(width)
    if height:
        image.SetHeight(height)
    p.pm.AddPlot(plot)


def plot(name, setup, base="1d"):
    pl = S.Plot(name, "special", base)
    pl[1].AddData("h", "s", "data")
    setup(pl)
    p.pm.AddPlot(pl)


picture = f"{WORK}/wide_png.png"
plot("missing", lambda pl: pl[1].AddImage(0.3, 0.8, f"{WORK}/nope.png"))
with open(f"{WORK}/text.png", "w") as textFile:
    textFile.write("not an image\n")
plot("no_image", lambda pl: pl[1].AddImage(0.3, 0.8, f"{WORK}/text.png"))
plot("beyond_pad", lambda pl: pl[1].AddImage(0.9, 0.8, picture).SetPadCoordinates().SetWidth(0.3))
plot("pad0", lambda pl: pl[0].AddImage(0.3, 0.8, picture))
base = S.PlotManager.MakeBasePlot("1d")
base.SetName("1d_logo")
base[1].AddImage(0.7, 0.95, picture).SetWidth(0.1)
p.pm.AddBasePlot(base)
plot("from_base", lambda pl: None, "1d_logo")
cwd = os.getcwd()
os.chdir(WORK)
plot("relative", lambda pl: pl[1].AddImage(0.3, 0.8, "wide_png.png"))
os.chdir(cwd)

run = p.plot("images", mode="png")
check("the plots are saved without errors", not run.errors(), " | ".join(run.errors()[:3]))
pngs = {i: bitmap(p.path("out", "images", f"i{i}.png")) for i in range(len(todo))}
info = p.plot("images")  # the file of the plots: the image boxes
for i, case in enumerate(todo):
    what = describe(case)
    padObj = info.pad(f"i{i}", "images")
    box = padObj.GetPrimitive("ImageBox_1") if padObj else None
    if not padObj:
        check(f"{what}: the plot is made", False, " | ".join(info.errors()[:2]))
        continue
    canvas = padObj.GetCanvas()
    W, H = canvas.GetWw(), canvas.GetWh()
    # the unit of sizes: the frame or the pad, as fractions of the canvas
    if case["system"] == "frame":
        unitX = padObj.GetAbsWNDC() * (1.0 - padObj.GetLeftMargin() - padObj.GetRightMargin())
        unitY = padObj.GetAbsHNDC() * (1.0 - padObj.GetTopMargin() - padObj.GetBottomMargin())
    else:
        unitX, unitY = padObj.GetAbsWNDC(), padObj.GetAbsHNDC()
    w, h = SIZE[case["picture"]]
    width, height = SIZES[case["size"]]
    widthPixel = (width or 0.2) * unitX * W if width or not height else None  # 0.2 of the frame or pad without any size
    heightPixel = height * unitY * H if height else None
    if widthPixel is None:
        widthPixel = heightPixel * w / h
    if heightPixel is None:
        heightPixel = widthPixel * h / w
    # the box in the coordinates of the pad: beyond the pad it is left out with a warning
    lm, rm, bm, tm = padObj.GetLeftMargin(), padObj.GetRightMargin(), padObj.GetBottomMargin(), padObj.GetTopMargin()
    x, y = (lm + POSITION[0] * (1.0 - lm - rm), bm + POSITION[1] * (1.0 - bm - tm)) if case["system"] == "frame" else POSITION
    fits = x + widthPixel / (padObj.GetAbsWNDC() * W) <= 1.0 + 1e-3 and y - heightPixel / (padObj.GetAbsHNDC() * H) >= -1e-3
    if not fits:
        said = any("does not fit into its pad" in m for m in info.messages(f"i{i}", "images"))
        check(f"{what}: beyond its pad: warned and left out", said and not box, " | ".join(info.messages(f"i{i}", "images")))
        continue
    if not box:
        check(f"{what}: the image is drawn", False, " | ".join(info.messages(f"i{i}", "images")))
        continue
    foundW, foundH = box.GetAbsWNDC() * W, box.GetAbsHNDC() * H
    check(f"{what}: the image is {widthPixel:.0f} x {heightPixel:.0f} pixels", abs(foundW - widthPixel) <= 1.5 and abs(foundH - heightPixel) <= 1.5, f"{foundW:.1f} x {foundH:.1f}")
    img = pngs[i]
    if img is None:
        check(f"{what}: saved as png", False)
        continue
    left, top = int(box.GetAbsXlowNDC() * W), int((1.0 - box.GetAbsYlowNDC() - box.GetAbsHNDC()) * H)
    inner = img[top + 2 : top + int(foundH) - 2, left + 2 : left + int(foundW) - 2]
    r, g, b = (inner >> 16) & 0xFF, (inner >> 8) & 0xFF, inner & 0xFF
    share = ((abs(r.astype(int) - COLOUR[0]) < 12) & (abs(g.astype(int) - COLOUR[1]) < 12) & (abs(b.astype(int) - COLOUR[2]) < 12)).mean() if inner.size else 0.0
    check(f"{what}: the picture fills its box in the png", share > 0.97, f"{share:.1%} of its pixels in the picture's colour")

special = p.plot("special")
messages = lambda name: special.messages(name, "special")  # noqa: E731
image = lambda name: special.pad(name, "special").GetPrimitive("ImageBox_1") if special.pad(name, "special") else None  # noqa: E731
check("a missing file: warned, the plot made without it", special.canvas("missing", "special") and not image("missing") and any("nope.png does not exist" in m for m in messages("missing")), " | ".join(messages("missing")))
check(
    "a file that is no image: warned as such (not as a ROOT without images), the plot made without it",
    special.canvas("no_image", "special") and not image("no_image") and any("text.png could not be read: it is not an image ROOT can read" in m for m in messages("no_image")),
    " | ".join(messages("no_image")),
)
check("an image beyond its pad: warned, the plot made without it", special.canvas("beyond_pad", "special") and not image("beyond_pad") and any("does not fit into its pad" in m for m in messages("beyond_pad")), " | ".join(messages("beyond_pad")))
check("an image in pad 0: warned", any("pad0" in m and "images added to pad 0" in m for m in special.warnings()), " | ".join(special.warnings()))
check("an image of a base plot is drawn in its plots", bool(image("from_base")))
check("a relative path is taken from where the plot is defined", bool(image("relative")), " | ".join(messages("relative")))

for fmt, marker in (("svg", b"<image"), ("eps", b"/WT"), ("png", None)):  # eps: ROOT writes the pixels as a cell array
    out = p.plot("images", "i0", fmt)
    path = p.path("out", "images", f"i0.{fmt}")
    has = os.path.exists(path) and (marker is None or marker in open(path, "rb").read())
    check(f"mode {fmt}: the file holds the image, no warning", has and not out.warnings(), " | ".join(out.warnings()))
pdf = p.plot("images", "i[01]", "pdf")
said = [line for line in pdf.warnings() if "Images are missing in the pdf files" in line]
if ROOT.gROOT.GetVersionInt() < 64200:
    check("mode pdf before ROOT 6.42: said once that the images are missing, ROOT's own warning silenced", len(said) == 1 and "PDF not implemented" not in pdf.log, f"{len(said)} warnings")
else:
    check("mode pdf: no warning", not said)
finish()
