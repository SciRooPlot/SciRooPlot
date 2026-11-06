"""The clipped steps are the steps of ROOT cut at the frame: hist_clipped paints what hist paints and hist_open_clipped what hist_open
(HIST ][) paints, except along the lower and upper frame edges, where ROOT moves bins that lie beyond the frame onto the edge. This holds for
every data kind drawn as steps, on linear and log frames whose lower edge lies below, at or above zero, with bins at the ends that are
empty, below or above the frame, with and without a fill (the filled areas of the steps).
Covers: hist_open_clipped kept the bins at the ends that HIST ][ leaves out (empty, or below a lower frame edge above zero), with the step
down to them; the outer edges and the fill of hist_clipped went below a base line at zero, to the lower frame edge.
"""

import numpy as np

from srptest import DATA_KINDS, Project, S, add_data, alias_table, cases, check, describe, drawn_type, finish, has_variant, in_chunks, pixels, source_of

TABLE = alias_table()
KINDS = tuple(k for k in DATA_KINDS if TABLE["hist_open_clipped"][drawn_type(k)] not in ("-", "graph") and DATA_KINDS[k][0] == 1 and "polar" not in DATA_KINDS[k][2])
PAIRS = {"hist_clipped": "hist", "hist_open_clipped": "hist_open"}
# frames: y range (None: automatic) and log y; the data runs from about 1 to 7 over x from 1 to 10
FRAMES = {"auto": (None, False), "zero": ((0.0, 9.0), False), "above": ((2.5, 6.0), False), "below": ((-1.0, 9.0), False), "log": ((2.5, 6.0), True), "log_auto": (None, True)}
VARIANTS = ("normal", "single", "negative", "zeros")  # single: empty bins at both ends; negative: bins below a lower edge at zero
EDGE_PIXELS = 6  # half the height of the rows of a frame edge (the line width scaled to the bitmap, with antialiasing)


def valid(case):
    return has_variant(case["kind"], case["variant"]) and not (FRAMES[case["frame"]][1] and case["variant"] in ("negative", "zeros"))


todo = cases({"kind": list(KINDS), "variant": list(VARIANTS), "frame": list(FRAMES), "clipped": list(PAIRS), "fill": [False, True]}, valid=valid, always=lambda c: c["kind"] == "hist")
p = Project("clipped_steps", bases=("1d",))
p.inputs(VARIANTS)
for i, case in enumerate(todo):
    yRange, logY = FRAMES[case["frame"]]
    for suffix, alias in (("clipped", case["clipped"]), ("plain", PAIRS[case["clipped"]])):
        plot = S.Plot(f"c{i}_{suffix}", "steps", "1d")
        data = add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"]).SetDrawOption(getattr(S, alias))
        if case["fill"]:
            data.SetFill(1001, S.kAzure - 9)  # solid (kFSolid, 1, is drawn hollow)
        plot[1]["X"].SetRange(0.5, 10.5)  # room beside the bins: the outer edges lie inside the frame
        if yRange:
            plot[1]["Y"].SetRange(*yRange)
        if logY:
            plot[1]["Y"].SetLog()
        p.pm.AddPlot(plot)
run = p.plot("steps")
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))


def compare(i, case):
    what = describe(case)
    if not run.drawn(f"c{i}_plain", "steps") or not run.drawn(f"c{i}_clipped", "steps"):
        return [(f"plots made: {what}", False, " | ".join(run.errors()[:2]))]
    padObj = run.pad(f"c{i}_plain", "steps")
    plainPixels, toPixel = pixels(run.canvas(f"c{i}_plain", "steps"), lambda o: True)
    clippedPixels, _ = pixels(run.canvas(f"c{i}_clipped", "steps"), lambda o: True)
    rows = np.nonzero((plainPixels != clippedPixels).any(axis=1))[0]
    edges = [toPixel(0.5, padObj.GetBottomMargin())[1], toPixel(0.5, 1.0 - padObj.GetTopMargin())[1]]
    elsewhere = [int(r) for r in rows if min(abs(r - e) for e in edges) > EDGE_PIXELS]
    return [(f"clipped steps differ only along the frame edges: {what}", not elsewhere, f"rows {elsewhere[:10]} differ (edges at {[round(e) for e in edges]})")]


in_chunks(todo, compare)
finish()
