"""A plot read back from Plots.root and shown in ROOT's web canvas (TWebCanvas, which paints the data in the browser with the palette and
gStyle it sends along) keeps the global settings it was painted with: the canvas asks for one palette per pad, and each pad gets the
colours plain ROOT gives for its palette (a ROOT palette of the pad or of pad 0, with its alpha, else kBird); where a web canvas sends one
palette for the whole canvas (its default, e.g. for images made by the web canvas) it is that of the first pad drawing a map. gStyle
exists once per canvas there: it gets the text format of the first data setting one and the candle ranges of the first pad not using
ROOT's (0.5 and 1), else ROOT's defaults. The canvas keeps the size it was painted with instead of being fitted into the browser window
(a fixed canvas size), set once: a size chosen later in the browser stays. The y range of a 1d frame stays when the browser reports the
ranges it drew without a zoom (as after every drawing). The browser (JSROOT) gets what it needs to draw as ROOT does: the z range of each
2d frame and colour scale in their options (HMIN, HMAX), also after the browser reported its options, one colour scale per frame, data
with levels of its own drawn SAME0, x errors left out with X0 right after E, the x divisions of a pad as ROOT scales them by its width,
the divisions of a colour scale those of the z axis, the max digits of the axes (one more where a negative label has as many digits),
no titles where a polar graph switches them off, sampled values for functions, and a white canvas. Skipped where ROOT is built without
its web canvas.
Covers: palettes per pad and for the whole canvas, text formats and candle ranges set in a later pad, by pad 0 or not at all, the
default and a set canvas size, y ranges of histogram and graph frames, linear and log, set and with room above the data; maps in one
pad, in panels with a colour-scale cell and with levels of their own; points in a narrow panel, a polar graph, a formula function
evaluated with TMath, axes with and without negative labels.
"""

import json

from srptest import ROOT, Project, S, add_data, cases, check, close, describe, finish, rgba, skip

if not hasattr(ROOT, "TWebCanvas") or not hasattr(ROOT.TWebCanvas, "CreateCanvasJSON"):
    skip("ROOT is built without its web canvas (webgui)")
ROOT.gROOT.SetWebDisplay("batch")  # canvases read back get a TWebCanvas, as in a session showing them in the browser

PALETTES = {"default": None, "kViridis": ROOT.kViridis, "kRainBow": ROOT.kRainBow, "kGreyScale": ROOT.kGreyScale}


def plain(palette, alpha):
    """the colours of a palette in plain ROOT, as (r, g, b, alpha)"""
    ROOT.gStyle.SetPalette(palette or ROOT.kBird, ROOT.nullptr, 1.0 if alpha is None else alpha)
    return [rgba(ROOT.TColor.GetColorPalette(k)) for k in range(ROOT.TColor.GetNumberOfColors())]


def snapshot(canvas, delivery):
    """the web canvas JSON of a canvas, made with a palette delivery (1: once for the canvas, 3: per pad), as dict"""
    ROOT.gEnv.SetValue("WebGui.PaletteDelivery", delivery)
    return json.loads(str(ROOT.TWebCanvas.CreateCanvasJSON(canvas, 0, True)))


def specials(snap, kind):
    """the snapshots of a kind (4: colours and palette, 5: gStyle) of a pad snapshot, not of its sub-pads"""
    return [prim["fSnapshot"] for prim in snap.get("fPrimitives", []) if prim.get("fKind") == kind]


def sub_pad(snap, name):
    return next((prim for prim in snap.get("fPrimitives", []) if prim.get("fKind") == 3 and prim["fSnapshot"].get("fName") == name), None)


def sent_palette(snap):
    """the colours of the palette a pad snapshot sends, as (r, g, b, alpha)"""
    colors = specials(snap, 4)
    return [rgba(int(round(index))) for index in colors[-1].get("fBuf", [])] if colors else None


p = Project("web_canvas", bases=("1d", "2d", "1d_2x1_gap", "2d_2x1_gap"))
p.inputs()
todo = cases(
    {"first": ["map", "hist"], "palette1": list(PALETTES), "palette2": list(PALETTES), "pad0": [None, "kViridis", "alpha"]},
    named=[
        {"first": "map", "palette1": "kViridis", "palette2": "kRainBow", "pad0": None, "why": "each pad of the web canvas has its own palette"},
        {"first": "hist", "palette1": "default", "palette2": "kGreyScale", "pad0": None, "why": "the palette of the whole canvas is that of the first map"},
    ],
)
expected = {}
for i, case in enumerate(todo):
    plot = S.Plot(f"p{i}", "palettes", "2d_2x1_gap")
    if case["pad0"] == "kViridis":
        plot[0].SetPalette(ROOT.kViridis)
    elif case["pad0"] == "alpha":
        plot[0].SetPalette(ROOT.kCividis, 0.5)
    for padID in (1, 2):
        palette = PALETTES[case[f"palette{padID}"]]
        if palette is not None:
            plot[padID].SetPalette(palette)
    if case["first"] == "map":
        add_data(plot[1], "hist2d").SetDrawOption(S.colz)
    else:
        add_data(plot[1], "hist")
    add_data(plot[2], "hist2d").SetDrawOption(S.col)
    p.pm.AddPlot(plot)

    def painted(padID, case=case):
        own = PALETTES[case[f"palette{padID}"]]
        if own is not None:
            return (own, None)
        return {None: (None, None), "kViridis": (ROOT.kViridis, None), "alpha": (ROOT.kCividis, 0.5)}[case["pad0"]]

    expected[f"p{i}"] = (case, {padID: painted(padID) for padID in (1, 2)}, 1 if case["first"] == "map" else 2)

# gStyle: (setup of the plot, text format, box range, whisker range)
STYLE = {
    "none": (lambda pl, d1, d2: None, "g", 0.5, 1.0),
    "later_pad": (lambda pl, d1, d2: (pl[2].SetDefaultCandleBoxRange(0.2).SetDefaultCandleWhiskerRange(0.6), d2.SetTextFormat("4.2f")), "4.2f", 0.2, 0.6),
    "both_pads": (
        lambda pl, d1, d2: (pl[1].SetDefaultCandleBoxRange(0.3), pl[2].SetDefaultCandleBoxRange(0.2), d1.SetTextFormat(".1e"), d2.SetTextFormat("4.2f")),
        ".1e",
        0.3,
        1.0,
    ),
    "pad0": (lambda pl, d1, d2: pl[0].SetDefaultCandleWhiskerRange(0.9), "g", 0.5, 0.9),
    "quote": (lambda pl, d1, d2: d1.SetTextFormat('5.1f"'), '5.1f"', 0.5, 1.0),
}
for name, (setup, *_) in STYLE.items():
    plot = S.Plot(f"style_{name}", "styles", "2d_2x1_gap")
    first = add_data(plot[1], "hist2d").SetDrawOption(S.text)
    second = add_data(plot[2], "hist2d").SetDrawOption(S.candle)
    setup(plot, first, second)
    p.pm.AddPlot(plot)

SIZES = {"size_default": None, "size_set": (613, 487)}
for name, size in SIZES.items():
    plot = S.Plot(name, "sizes", "1d")
    if size:
        plot.SetDimensions(*size)
    add_data(plot[1], "hist")
    p.pm.AddPlot(plot)

# the report of the browser after drawing a pad (TWebCanvas::DecodePadOptions, protected): the drawn ranges, no zoom
ROOT.gInterpreter.Declare("""
#include "TWebCanvas.h"
#include "TWebPadOptions.h"
#include "TBufferJSON.h"
struct SrpTestWebReply : public TWebCanvas {
  static void Report(TWebCanvas* web, TPad* pad) {
    std::vector<TWebPadOptions> options(1);
    auto& o = options[0];
    o.snapid = std::to_string(TString::Hash(&pad, sizeof(pad)));
    o.logx = pad->GetLogx(); o.logy = pad->GetLogy(); o.logz = pad->GetLogz();
    o.mleft = pad->GetLeftMargin(); o.mright = pad->GetRightMargin(); o.mtop = pad->GetTopMargin(); o.mbottom = pad->GetBottomMargin();
    o.xlow = pad->GetAbsXlowNDC(); o.ylow = pad->GetAbsYlowNDC(); o.xup = o.xlow + pad->GetAbsWNDC(); o.yup = o.ylow + pad->GetAbsHNDC();
    o.ranges = true;
    o.px1 = pad->GetX1(); o.px2 = pad->GetX2(); o.py1 = pad->GetY1(); o.py2 = pad->GetY2();
    o.ux1 = pad->GetUxmin(); o.ux2 = pad->GetUxmax(); o.uy1 = pad->GetUymin(); o.uy2 = pad->GetUymax();
    static_cast<SrpTestWebReply*>(web)->DecodePadOptions(TBufferJSON::ToJSON(&options).Data());
  }
};
""")
RANGES = {
    "range_hist": ("hist", lambda pl: pl[1]["Y"].SetRange(0, 30)),
    "range_hist_log": ("hist", lambda pl: pl[1]["Y"].SetLog().SetRange(0.5, 50)),
    "range_graph": ("graph", lambda pl: pl[1]["Y"].SetRange(0, 30)),
    "room_hist": ("hist", lambda pl: pl[1]["Y"].SetRoomAboveMax(0.5)),
}
for name, (kind, setup) in RANGES.items():
    plot = S.Plot(name, "ranges", "1d")
    add_data(plot[1], kind)
    setup(plot)
    p.pm.AddPlot(plot)

# what the browser needs to draw as ROOT does (see the docstring)
plot = S.Plot("map", "browser", "2d")
add_data(plot[1], "hist2d").SetDrawOption(S.colz)
add_data(plot[1], "hist2d").SetDrawOption(S.cont).SetContours([2.0, 5.0])
p.pm.AddPlot(plot)
plot = S.Plot("panels", "browser", "2d_2x1_gap")
for padID in (1, 2):
    add_data(plot[padID], "hist2d").SetDrawOption(S.col)
p.pm.AddPlot(plot)
plot = S.Plot("narrow", "browser", "1d_2x1_gap")
for padID in (1, 2):
    add_data(plot[padID], "hist").SetDrawOption(S.points)
plot[1]["Y"].SetRange(-500.0, 500.0)
p.pm.AddPlot(plot)
plot = S.Plot("positive", "browser", "1d")
add_data(plot[1], "hist")
plot[1].AddFunction("1 + cos(x * TMath::DegToRad())").SetRangeX(0.0, 10.0)
p.pm.AddPlot(plot)
plot = S.Plot("polar", "browser", "1d")
add_data(plot[1], "graph_polar")
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", run.rc == 0 and not run.errors(), "\n".join(run.errors()))

for name, (case, painted, mapPad) in expected.items():
    canvas = run.canvas(name, "palettes")
    canvas.Draw()  # as a session showing it: the canvas gets its web canvas
    perPad = snapshot(canvas, 3)
    check(f"the web canvas asks for a palette per pad ({describe(case)})", canvas.GetCanvasImp().GetPaletteDelivery() == 3, f"delivery {canvas.GetCanvasImp().GetPaletteDelivery()}")
    sent = {padID: sent_palette(sub_pad(perPad, f"Pad_{padID}") or {}) for padID in (1, 2)}
    once = sent_palette(snapshot(canvas, 1))
    for padID in (1, 2):
        if case["first"] == "hist" and padID == 1:
            continue  # a pad without a map: its palette is not seen
        want = plain(*painted[padID])
        check(f"pad {padID} sends the colours of its palette ({describe(case)})", sent[padID] == want, f"{len(sent[padID] or [])} colours, first {(sent[padID] or [None])[0]}, want {want[0]}")
    want = plain(*painted[mapPad])
    check(f"the palette of the whole canvas is that of pad {mapPad} ({describe(case)})", once == want, f"{len(once or [])} colours, first {(once or [None])[0]}, want {want[0]}")

for name, (_, textFormat, boxRange, whiskerRange) in STYLE.items():
    canvas = run.canvas(f"style_{name}", "styles")
    canvas.Draw()
    styles = specials(snapshot(canvas, 1), 5)
    style = styles[0] if styles else {}
    check(f"gStyle of the web canvas has the text format {textFormat!r} ({name})", style.get("fPaintTextFormat") == textFormat, repr(style.get("fPaintTextFormat")))
    check(
        f"gStyle of the web canvas has the candle ranges {boxRange} and {whiskerRange} ({name})",
        close(style.get("fCandleBoxRange", 0), boxRange, 1e-6) and close(style.get("fCandleWhiskerRange", 0), whiskerRange, 1e-6),
        f"{style.get('fCandleBoxRange')} and {style.get('fCandleWhiskerRange')}",
    )
for name in RANGES:
    canvas = run.canvas(name, "ranges")
    canvas.Draw()
    snapshot(canvas, 1)
    pad = canvas.GetPad(1)
    frame = pad.GetPrimitive("axis_hist_pad_1")
    painted = (frame.GetMinimumStored(), frame.GetMaximumStored())
    ROOT.SrpTestWebReply.Report(canvas.GetCanvasImp(), pad)
    got = (frame.GetMinimumStored(), frame.GetMaximumStored())
    check(
        f"the y range stays when the browser reports the drawn ranges ({name})",
        painted[1] != -1111 and all(close(a, b, 1e-6) for a, b in zip(got, painted)),
        f"{got}, painted {painted}",
    )
for name, size in SIZES.items():
    canvas = run.canvas(name, "sizes")
    painted = (canvas.GetWw(), canvas.GetWh())  # as written
    canvas.Draw()
    snapshot(canvas, 1)
    imp = canvas.GetCanvasImp()
    got = (canvas.GetWw(), canvas.GetWh())
    check(f"the web canvas has a fixed size ({name})", imp.IsFixedSize(), "not fixed")
    check(f"the web canvas has the size the plot was painted with ({name})", got == painted and (not size or got == size), f"{got}, painted {painted}")
    canvas.SetCanvasSize(300, 200)  # as chosen in the browser
    snapshot(canvas, 1)
    got = (canvas.GetWw(), canvas.GetWh())
    check(f"a canvas size chosen in the browser stays ({name})", got == (300, 200), f"{got}")


def prims(snap):
    """(option, object) of the primitives of a pad snapshot that are objects"""
    return [(prim.get("fOption", ""), prim["fSnapshot"]) for prim in snap.get("fPrimitives", []) if prim.get("fKind") == 1 and isinstance(prim.get("fSnapshot"), dict)]


def ndivisions(n):
    """the number of divisions ROOT paints an x axis with in a pad of width w (THistPainter::PaintAxis): n % 100 scaled for n > 1000"""
    return lambda w: (n % 1000000 - n % 100 + int(max(1, n % 100) * w)) if n > 1000 else n


for name in ("map", "panels", "narrow", "positive", "polar"):
    canvas = run.canvas(name, "browser")
    canvas.Draw()
    pads = {f"Pad_{i}": canvas.GetPad(i) for i in range(1, 4) if canvas.GetPad(i)}
    written = {}  # z ranges and divisions as written, before the browser sees the canvas
    for padName, pad in pads.items():
        for obj in pad.GetListOfPrimitives():
            if obj.InheritsFrom("TH1") and (obj.GetName().startswith("axis_hist_pad_") or obj.GetName().startswith("color_scale_pad_")):
                written.setdefault((padName, obj.GetName()), (obj.GetMinimumStored(), obj.GetMaximumStored(), obj.GetXaxis().GetNdivisions(), obj.GetZaxis().GetNdivisions(), pad.GetAbsWNDC()))
    snap = snapshot(canvas, 3)
    for padName, pad in pads.items():
        ROOT.SrpTestWebReply.Report(canvas.GetCanvasImp(), pad)  # the browser reports its options and ranges
    again = snapshot(canvas, 3)
    for which, current in (("drawn", snap), ("after the report of the browser", again)):
        canvasSnap = current.get("fSnapshot", {})
        check(f"the web canvas is white ({name}, {which})", canvasSnap.get("fFillStyle") == 1001 and canvasSnap.get("fFillColor") == 0, f"{canvasSnap.get('fFillStyle')} {canvasSnap.get('fFillColor')}")
        styles = specials(current, 5)
        style = styles[0] if styles else {}
        digits = 4 if name == "narrow" else 3
        check(f"gStyle of the web canvas has the max digits {digits} ({name}, {which})", style.get("fAxisMaxDigits") == digits, f"{style.get('fAxisMaxDigits')}")
        check(f"gStyle of the web canvas has titles {'off' if name == 'polar' else 'on'} ({name}, {which})", style.get("fOptTitle") == (0 if name == "polar" else 1), f"{style.get('fOptTitle')}")
        for padName in pads:
            padSnap = sub_pad(current, padName) or {}
            objects = prims(padSnap)
            if name == "polar":
                check(f"a polar graph has no title box ({which})", not any(obj.get("fName") == "title" for _, obj in objects), "title")
            seen = set()
            for option, obj in objects:
                key = (padName, obj.get("fName"))
                copy = "AXIG" in option.upper() or "AXIS" in option.upper()  # the axes drawn again
                if key in written and key not in seen and not copy:
                    seen.add(key)
                    low, high, xDivisions, zDivisions, width = written[key]
                    if obj["_typename"].startswith("TH2") and low != -1111 and high != -1111:
                        check(f"the first drawing of {key[1]} gives its z range ({name}, {which})", f";HMIN:{low:.17g};HMAX:{high:.17g}" in option, option)
                    check(
                        f"the x divisions of {key[1]} are those ROOT paints ({name}, {which})",
                        obj["fXaxis"]["fNdivisions"] % 1000000 == ndivisions(xDivisions)(width),
                        f"{obj['fXaxis']['fNdivisions']}, written {xDivisions}, pad width {width:.3f}",
                    )
                    palettes = [f for f in obj.get("fFunctions", {}).get("arr", []) if f.get("_typename") == "TPaletteAxis"]
                    for palette in palettes:
                        check(f"the colour scale of {key[1]} is divided as the z axis ({name}, {which})", palette["fAxis"]["fNdiv"] == zDivisions % 1000000, f"{palette['fAxis']['fNdiv']}, z {zDivisions}")
                elif key in written:
                    check(f"a later drawing of {key[1]} draws no colour scale ({name}, {which})", "Z" not in option.upper().replace("HMIN", "").replace("HMAX", ""), option)
                if obj["_typename"] == "TH1D" and not obj.get("fName", "").startswith("axis_hist") and name == "narrow":
                    check(f"points leave out x errors with X0 after E ({which})", "EX0" in option.replace(" ", "") and " X0" not in f" {option}", option)
                if obj["_typename"] == "TH2D" and obj.get("fContour") and len(obj["fContour"]) == 2:
                    check(f"data with levels of its own are drawn SAME0 ({which})", "SAME0" in option, option)
                if obj["_typename"] == "TF1":
                    check(f"a function has its sampled values ({which})", len(obj.get("fSave", [])) > 0, f"{len(obj.get('fSave', []))} values")
finish()
