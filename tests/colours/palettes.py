"""A pad paints its data with the palette set for it (SetPalette with a ROOT palette and an alpha), else with the one of pad 0, else with ROOT's
kBird (a ROOT palette of the pad wins over a gradient of pad 0): the colours plain ROOT gives for that palette (gStyle->SetPalette(n, nullptr, alpha)), set before anything of the pad is painted and
restored to kBird after the canvas, so a session drawing other things afterwards is unaffected; whatever the values of the data (all
zero, crossing zero, near the largest float). A ColorScale() cell shows its map's scale
in the palette the map is painted with; panels sharing one colour scale warn once when they are painted with different palettes.
Covers: the scale cell in the palette of its map, different palettes in a shared scale warn; no warning for settings
giving the same palette (ROOT's default or alpha 1 set explicitly).
"""

from srptest import DATA_2D, DATA_3D, ROOT, Project, S, add_data, base_of, cases, check, describe, finish, has_variant, rgba, source_of

PALETTES = {"default": None, "kBird": ROOT.kBird, "kViridis": ROOT.kViridis, "kCividis": ROOT.kCividis, "kDeepSea": ROOT.kDeepSea, "kRainBow": ROOT.kRainBow, "kGreyScale": ROOT.kGreyScale, "pretty": 1}
OTHER = ROOT.kSunset  # pad 0's palette where the pad sets its own
OTHER_GRADIENT = [(1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 1.0)]  # pad 0's gradient where the pad sets a ROOT palette
MAPS = tuple(k for k in DATA_2D + DATA_3D if k not in ("tf3", "tf3_cpp"))  # data drawn with colours of the palette
VARIANTS = ("normal", "zeros", "negative", "huge")  # hostile values: all zero, crossing zero, near the largest float


def plain(palette, alpha):
    """the colours of a palette in plain ROOT, as (r, g, b, alpha)"""
    ROOT.gStyle.SetPalette(palette or ROOT.kBird, ROOT.nullptr, 1.0 if alpha is None else alpha)
    return [rgba(ROOT.TColor.GetColorPalette(k)) for k in range(ROOT.TColor.GetNumberOfColors())]


p = Project("palettes", bases=("1d", "2d", "2d_2x1", "2d_2x1_gap"))
p.inputs(VARIANTS)
todo = cases(
    {"kind": list(MAPS), "palette": list(PALETTES), "where": ["pad", "pad0", "both", "pad0_gradient"], "alpha": [None, 0.5], "variant": list(VARIANTS)},
    valid=lambda c: (c["palette"] != "default" or (c["where"] == "pad" and c["alpha"] is None)) and has_variant(c["kind"], c["variant"]),
)
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "palettes", base_of(case["kind"]))
    palette = PALETTES[case["palette"]]
    if palette is not None:
        if case["where"] in ("pad0", "both"):
            plot[0].SetPalette(OTHER if case["where"] == "both" else palette, case["alpha"])
        if case["where"] == "pad0_gradient":
            plot[0].SetPalette(OTHER_GRADIENT)
        if case["where"] in ("pad", "both", "pad0_gradient"):
            plot[1].SetPalette(palette, case["alpha"])
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    p.pm.AddPlot(plot)

# a ColorScale() cell beside a map with its projection
CELL = {  # palette of the map: setup of the plot
    "own": lambda pl: pl[1].SetPalette(ROOT.kViridis),
    "own_alpha": lambda pl: pl[1].SetPalette(ROOT.kViridis, 0.4),
    "pad0": lambda pl: pl[0].SetPalette(ROOT.kCividis),
    "gradient": lambda pl: pl[1].SetPalette([((1.0, 1.0, 1.0), 0.0), ((0.5, 0.0, 0.2), 1.0)]),
    "default": lambda pl: None,
}
for name, setup in CELL.items():
    plot = S.Plot(f"cell_{name}", S.PanelLayout([[S.Panel(), S.Panel(0.4, 1), S.ColorScale(1)]]))
    plot.SetGroup("cells")
    add_data(plot[1], "hist2d").SetDrawOption(S.colz)
    add_data(plot[2], "hist").SetDrawOption(S.hbars)
    setup(plot)
    p.pm.AddPlot(plot)

# panels sharing one colour scale: the palettes the pads are painted with (pad 0's for a pad without one)
SHARED = {  # (base plot, palette of pad 0, of pad 1, of pad 2, warns)
    "different": ("2d_2x1", None, ROOT.kViridis, ROOT.kCividis, True),
    "same": ("2d_2x1", None, ROOT.kViridis, ROOT.kViridis, False),
    "pad0_only": ("2d_2x1", ROOT.kViridis, None, None, False),
    "pad0_and_own": ("2d_2x1", ROOT.kViridis, None, ROOT.kCividis, True),
    "pad0_same_as_own": ("2d_2x1", ROOT.kViridis, None, ROOT.kViridis, False),
    "other_alpha": ("2d_2x1", None, ROOT.kViridis, (ROOT.kViridis, 0.5), True),
    "gradient": ("2d_2x1", None, [(1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 1.0)], ROOT.kViridis, True),
    "explicit_default": ("2d_2x1", None, ROOT.kBird, None, False),  # painted with kBird both
    "explicit_alpha": ("2d_2x1", None, ROOT.kViridis, (ROOT.kViridis, 1.0), False),
    "gap": ("2d_2x1_gap", None, ROOT.kViridis, ROOT.kCividis, False),
}
for name, (base, *palettes, _) in SHARED.items():
    plot = S.Plot(f"shared_{name}", "shared", base)
    for padID, palette in enumerate(palettes):
        if padID > 0:
            add_data(plot[padID], "hist2d")
        if isinstance(palette, tuple):
            plot[padID].SetPalette(*palette)
        elif palette is not None:
            plot[padID].SetPalette(palette)
    p.pm.AddPlot(plot)
run = p.plot()

for i, case in enumerate(todo):
    what = describe(case)
    pad = run.pad(f"c{i}", "palettes")
    found = run.palette(f"c{i}", "palettes")
    want = plain(PALETTES[case["palette"]], case["alpha"])
    check(f"painted with the colours of ROOT's palette: {what}", found == want, f"{len(found or [])} colours {(found or [])[:2]}, expected {len(want)} {want[:2]}")
    names = [obj.GetName() for obj in pad.GetListOfPrimitives()] if pad else []
    first = next((n for n in names if n != "setup_pad_1" and not n.startswith("define")), None)
    check(f"palette set before anything of the pad is painted: {what}", bool(names) and names[0] == "setup_pad_1", f"{names[:3]} (first painted: {first})")
canvas = run.canvas("c0", "palettes")
for obj in canvas.GetListOfPrimitives() if canvas else []:  # painted as ROOT paints the canvas: its primitives in order
    if obj.InheritsFrom("TExec"):
        obj.Exec()
    elif obj.InheritsFrom("TPad"):
        setup = obj.GetPrimitive(f"setup_pad_{obj.GetNumber()}")
        if setup:
            setup.Exec()
check("ROOT's palette restored after the canvas", bool(canvas) and [rgba(ROOT.TColor.GetColorPalette(k)) for k in range(ROOT.TColor.GetNumberOfColors())] == plain(None, None))

for name in CELL:
    canvas = run.canvas(f"cell_{name}", "cells")
    if not canvas:
        check(f"scale cell made: {name}", False, " | ".join(run.errors()[:2]))
        continue
    cell = canvas.GetPrimitive("Pad_3")
    cellPalette, mapPalette = run.palette(f"cell_{name}", "cells", 3), run.palette(f"cell_{name}", "cells", 1)
    check(f"scale cell in the palette of its map: {name}", cellPalette is not None and cellPalette == mapPalette, f"cell {(cellPalette or [])[:2]}, map {(mapPalette or [])[:2]}")
    names = [obj.GetName() for obj in cell.GetListOfPrimitives()] if cell else []
    check(f"scale cell: palette set before the scale is painted: {name}", "setup_pad_3" in names and "color_scale_pad_3" in names and names.index("setup_pad_3") < names.index("color_scale_pad_3"), names)

warned = run.warnings("another palette than the pads it shares one colour scale with")
for name, (*_, warns) in SHARED.items():
    count = sum(f"Plot shared_{name} from group " in line for line in warned)
    check(f"shared colour scale {name}: {'one warning' if warns else 'no warning'}", count == int(warns), f"{count} warnings")
others = [line for line in run.warnings() if line not in warned]
check("no other warnings", not others, " | ".join(others[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
