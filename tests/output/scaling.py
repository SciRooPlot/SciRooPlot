"""The personal scale settings: bitmapscale s makes the bitmaps (png, jpg, gif) s times the canvas in each direction, with everything drawn
scaled alike: the dashes and gaps of line styles 2 to 4 s times as long, the outline of open markers s pixels wide, filled markers still
filled; vector files (svg, pdf, eps) do not depend on it. screenscale s does the same for the plots shown on screen (mode show): the window
shows the dashes and markers as the png at bitmapscale s (X11 has fixed patterns for styles 2 to 4, which SciRooPlot replaces by styles of
its own). The screen part needs Xvfb and ImageMagick's import for the screenshot; it is left out without them.
Covers: dashes of line styles 2 to 4 and outlines of open markers scaled, the screen of mode show."""

import os
import re
import shutil
import subprocess
import time

import numpy as np

from srptest import PLOT, Project, ROOT, S, bitmap, check, display, finish, gif_info, rgb

p = Project("scaling")
plot = S.Plot("lines", "scaling", "1d")
STYLES = (2, 3, 4)
for i, style in enumerate(STYLES):
    plot[1].AddLine((0.0, 0.2 + 0.15 * i), (1.0, 0.2 + 0.15 * i)).SetLine(style, 4.0, ROOT.kRed + i)
plot[1].AddPoints([0.3], [0.8], "open").SetMarker(ROOT.kOpenCircle, 6.0, ROOT.kBlue)
plot[1].AddPoints([0.7], [0.8], "full").SetMarker(ROOT.kFullCircle, 6.0, ROOT.kBlue)
plot[1]["X"].SetRange(0, 1)
plot[1]["Y"].SetRange(0, 1)
p.pm.AddPlot(plot)
reference = p.plot()
canvas = reference.canvas("lines", "scaling")
SIZE = (canvas.GetWw(), canvas.GetWh())
SCALES = (1, 2, 3)


def matches(pixels, colour, tolerance=40):
    return np.max([np.abs(((pixels >> s) & 0xFF).astype(int) - ((colour >> s) & 0xFF)) for s in (16, 8, 0)], axis=0) < tolerance


def runs(row):
    """(value, length) of the runs of equal values in a row"""
    edges = np.flatnonzero(np.diff(row.astype(int))) + 1
    starts = np.concatenate(([0], edges))
    return [(bool(row[a]), int(b - a)) for a, b in zip(starts, np.concatenate((edges, [len(row)])))]


def dashes(pixels, colour):
    """(longest dash, longest gap) of the horizontal line of a colour, inner runs only (the shorter ones are its antialiased ends)"""
    mask = matches(pixels, colour)
    rows = np.flatnonzero(mask.sum(axis=1) > 0.05 * mask.shape[1])
    if not len(rows):
        return (0, 0)
    inner = runs(mask[rows[len(rows) // 2]])[1:-1]
    return (max((n for v, n in inner if v), default=0), max((n for v, n in inner if not v), default=0))


def blue(pixels):
    return matches(pixels, rgb(ROOT.kBlue), 70)


def ring(pixels):
    """widths of the blue runs on the centre row of the open marker (left half)"""
    mask = blue(pixels)[:, : pixels.shape[1] // 2]
    ys = np.flatnonzero(mask.any(axis=1))
    if not len(ys):
        return []
    return [n for v, n in runs(mask[(ys[0] + ys[-1]) // 2]) if v]


def filled(pixels):
    """whether the centre of the filled marker (right half) is blue"""
    mask = blue(pixels)[:, pixels.shape[1] // 2 :]
    ys, xs = np.nonzero(mask)
    return bool(len(ys)) and bool(mask[int(ys.mean()), int(xs.mean())])


def measure(pixels):
    return {"dashes": [dashes(pixels, rgb(ROOT.kRed + i)) for i in range(len(STYLES))], "ring": ring(pixels)[:2], "filled": filled(pixels)}


def contents(path):
    with open(path, "rb") as f:
        return re.sub(rb"(CreationDate|ModDate|%%CreationDate)[^\n]*", b"", f.read())  # the time of writing


files, vectors = {}, {}
for scale in SCALES:
    p.srp("bitmapscale", str(scale))
    for mode in ("png", "jpg", "gif", "svg", "pdf", "eps"):
        run = p.plot("scaling", "lines", mode)
        path = p.path("out", "scaling.gif") if mode == "gif" else p.path("out", "scaling", f"lines.{mode}")
        check(f"bitmapscale {scale}, mode {mode}: written without errors", os.path.isfile(path) and not run.errors(), " | ".join(run.errors()[:2]))
        if not os.path.isfile(path):
            continue
        if mode in ("svg", "pdf", "eps"):
            vectors.setdefault(mode, {})[scale] = contents(path)
            continue
        size = gif_info(path)["size"] if mode == "gif" else bitmap(path).shape[::-1]
        check(f"bitmapscale {scale}: the {mode} is the canvas times {scale}", tuple(size) == (scale * SIZE[0], scale * SIZE[1]), f"{size}, canvas {SIZE}")
        if mode == "png":
            files[scale] = measure(bitmap(path))
for mode, byScale in vectors.items():
    check(f"{mode} files do not depend on bitmapscale", len(set(byScale.values())) == 1, f"differ for scales {sorted(byScale)}")
p.srp("bitmapscale", "1")

one = files.get(1)
for scale, found in files.items():
    for i, style in enumerate(STYLES):
        dash, gap = found["dashes"][i]
        check(f"bitmapscale {scale}: dashes and gaps of line style {style} {scale} times as long", one and dash > 0 and (dash, gap) == (scale * one["dashes"][i][0], scale * one["dashes"][i][1]), f"{(dash, gap)}, at scale 1 {one['dashes'][i] if one else None}")
    check(f"bitmapscale {scale}: open marker outline {scale} pixels wide", found["ring"] == [scale, scale], f"{found['ring']}")
    check(f"bitmapscale {scale}: filled marker filled", found["filled"])

# the plot on screen: a screenshot of the window of mode show (a virtual screen without window manager: the window has no decoration)
if not shutil.which("import") or not display(own=True):
    print("the screen part is left out: it needs Xvfb and ImageMagick's import")
else:
    for scale in (1, 2):
        p.srp("screenscale", str(scale))
        log = p.path(f"show{scale}.log")
        with open(log, "w") as out:
            show = subprocess.Popen([PLOT, "scaling", "lines", "show"], stdout=out, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
        shot = p.path(f"screen{scale}.png")
        pixels = None
        try:
            deadline = time.time() + 120
            while time.time() < deadline:  # until the window is drawn: the screen is black around it
                time.sleep(1)
                subprocess.run(["import", "-window", "root", shot], stderr=subprocess.DEVNULL, check=False, timeout=60)
                screen = bitmap(shot) if os.path.isfile(shot) else None
                if screen is not None:
                    ys, xs = np.nonzero(screen & 0xFFFFFF)
                    window = screen[ys.min() : ys.max() + 1, xs.min() : xs.max() + 1] if len(ys) else None
                    if window is not None and window.shape[1] > 0.9 * scale * SIZE[0] and len(ring(window)) >= 2:
                        pixels = window
                        break
        finally:
            show.kill()
            show.wait()
        if pixels is None or scale not in files:
            check(f"screenscale {scale}: screenshot of the window taken", False, open(log).read()[-300:])
            continue
        found = measure(pixels)
        for i, style in enumerate(STYLES):
            check(f"screenscale {scale}: dashes and gaps of line style {style} as in the png at bitmapscale {scale}", found["dashes"][i] == files[scale]["dashes"][i] and found["dashes"][i][0] > 0, f"screen {found['dashes'][i]}, png {files[scale]['dashes'][i]}")
        check(f"screenscale {scale}: open marker outline {scale} pixels wide", found["ring"] == [scale, scale], f"{found['ring']}")
        check(f"screenscale {scale}: filled marker filled", found["filled"])
    p.srp("screenscale", "1")
finish()
