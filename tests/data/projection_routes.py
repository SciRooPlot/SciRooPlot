"""Every route to the same histogram gives the one plain ROOT gives: one sample of weighted events (x, y, z, with entries below and above
every axis) is stored as TH3D, THnD, THnSparseD, as the TH2D (x, y) and TProfile2D (mean of z) ROOT makes of it, and as a tree; each is
projected onto x, y, (x, y) and (y, x), or profiled, with the same selection of the other axes (none, bin numbers, bin numbers including
under- and overflow, ranges on bin edges and inside bins in axis units), as a projection of the stored histogram or as a tree request
with a filter, and must give the contents and errors of the histogram filled directly with the selected events. A selection by bin
numbers (0, -1: ROOT's whole axis, flows included) and in axis units (the bins the range reaches into) selects the same events in
every route. A profile of a histogram averages the centres of the bins of the profiled axis (a tree the values), its under- and overflow
only when a selection names them by number: ROOT's ProfileX leaves them out for 0, -1, where ProjectionX counts them. Entries are not compared: ROOT counts them differently per class (a THn all fills, a TH3 projection the effective entries).
Covers: projected axes named by the axis titles of the input, the projections (ProjectX by bins and in user coordinates, a single bin, the last bin, ProfileX,
THnSparse with a range).
"""

import numpy as np
from srptest import ROOT, Project, S, cases, check, describe, finish, first_five, hist_diff, match_messages

NBINS, LOW, HIGH = 9, 1.0, 10.0  # every axis
rng = np.random.default_rng(11)
N = 6000
EVENTS = {axis: rng.uniform(LOW - 2, HIGH + 2, N) for axis in "xyz"}  # beyond both ends: under- and overflow on every axis
EVENTS["w"] = rng.uniform(0.5, 1.5, N)
AXES = "xyz"

# selections of the axes not kept, (axis, start, end, in axis units): the bins of each are given by bins() below
SELECTIONS = {
    "none": [],
    "bins": [(1, 3, 5, False)],
    "single_bin": [(1, 4, 4, False)],
    "last_bin": [(1, NBINS, NBINS, False)],
    "with_flows": [(1, 0, NBINS + 1, False)],
    "whole_axis": [(1, 0, -1, False)],
    "edges": [(1, 3.0, 6.0, True), (2, 2.0, 8.0, True)],
    "inside": [(1, 2.5, 6.5, True), (2, 4.2, 4.3, True)],
}
TARGETS = {"x": [0], "y": [1], "xy": [0, 1], "yx": [1, 0], "profile_x": [0], "profile_xy": [0, 1]}  # profiles: the mean of the next axis
INPUTS = ("TH3D", "THnD", "THnSparseD", "TH2D", "tree")


def bins(sel):
    """the bins (0 underflow to NBINS + 1 overflow) a selection keeps on its axis"""
    _, start, end, user = sel
    if not user:
        return range(0, NBINS + 2) if (start, end) == (0, -1) else range(int(start), int(end) + 1)
    width = (HIGH - LOW) / NBINS
    return [b for b in range(1, NBINS + 1) if LOW + (b - 1) * width < end and LOW + b * width > start] or [round((start - LOW) / width) + 1]


def bin_of(values):
    return np.clip(np.floor((values - LOW) / (HIGH - LOW) * NBINS).astype(int) + 1, 0, NBINS + 1)


def reference(target, selection, binned):
    """the histogram filled directly with the events the selection keeps (an axis without selection: all of its bins, flows included);
    a profile of the mean of the next axis, at the centres of its bins for a histogram (binned), the values themselves for a tree"""
    keep = np.ones(N, dtype=bool)
    for sel in SELECTIONS[selection]:
        keep &= np.isin(bin_of(EVENTS[AXES[sel[0]]]), list(bins(sel)))
    dims = TARGETS[target]
    profile = target.startswith("profile")
    if len(dims) == 1:
        h = ROOT.TProfile("ref", "", NBINS, LOW, HIGH) if profile else ROOT.TH1D("ref", "", NBINS, LOW, HIGH)
    else:
        h = ROOT.TProfile2D("ref", "", NBINS, LOW, HIGH, NBINS, LOW, HIGH) if profile else ROOT.TH2D("ref", "", NBINS, LOW, HIGH, NBINS, LOW, HIGH)
    h.Sumw2()
    width = (HIGH - LOW) / NBINS
    named = [sel for sel in SELECTIONS[selection] if sel[0] == len(dims) and not sel[3] and (sel[1], sel[2]) != (0, -1)]
    profiled = set(bins(named[0])) if named else set(range(1, NBINS + 1))
    for i in np.nonzero(keep)[0]:
        coords = [EVENTS[AXES[d]][i] for d in dims]
        if profile:
            value = EVENTS[AXES[len(dims)]][i]
            if binned:  # the centre of its bin; its flows only when the selection names them by number (ROOT's ProfileX)
                b = bin_of(np.array([value]))[0]
                if b not in profiled:
                    continue
                value = LOW + (b - 0.5) * width
            coords.append(value)
        h.Fill(*coords, EVENTS["w"][i])
    h.ClearUnderflowAndOverflow()  # not drawn
    return h


def possible(c):
    """routes that can express the case: a TH2D has no z (its selections and targets only use x and y), a tree selects with a filter;
    a profile is of a TH2D along x or of a TH3D along x and y, the mean taken of the next axis"""
    sels, target = SELECTIONS[c["selection"]], c["target"]
    if target == "profile_x" and c["input"] not in ("TH2D", "tree", "THnD"):
        return False
    if target == "profile_xy" and c["input"] not in ("TH3D", "tree"):
        return False
    if c["input"] == "TH2D":
        return all(s[0] < 2 for s in sels) and target in ("x", "profile_x")
    return all(s[0] not in TARGETS[target] for s in sels)


p = Project("routes")
f = ROOT.TFile(p.path("in.root"), "RECREATE")
h3 = ROOT.TH3D("TH3D", "", NBINS, LOW, HIGH, NBINS, LOW, HIGH, NBINS, LOW, HIGH)
axes = np.array([NBINS] * 3, dtype=np.int32)
lows, highs = np.array([LOW] * 3), np.array([HIGH] * 3)
hn = ROOT.THnD("THnD", "", 3, axes, lows, highs)
hs = ROOT.THnSparseD("THnSparseD", "", 3, axes, lows, highs)
for h in (h3, hn, hs):
    h.Sumw2()
for i in range(N):
    point = np.array([EVENTS[a][i] for a in AXES])
    h3.Fill(*point, EVENTS["w"][i])
    hn.Fill(point, EVENTS["w"][i])
    hs.Fill(point, EVENTS["w"][i])
h2 = h3.Project3D("yx")  # all z, flows included (TH3 projections include the flows of the axes integrated out)
h2.SetName("TH2D")
for obj in (h3, hn, hs, h2):
    f.WriteObject(obj, obj.GetName())
f.Close()
ROOT.RDF.FromNumpy({k: np.ascontiguousarray(v) for k, v in EVENTS.items()}).Snapshot("tree", p.path("tree.root"))
p.pm.AddDataSource("hists", p.path("in.root"))
p.pm.AddDataSource("trees", p.path("tree.root"))

todo = cases(
    {"input": list(INPUTS), "target": list(TARGETS), "selection": list(SELECTIONS)},
    valid=possible,
    always=lambda c: True,  # all of it is fast
    named=[
        {"input": "TH2D", "target": "x", "selection": "single_bin", "why": "a single bin"},
        {"input": "TH2D", "target": "x", "selection": "last_bin", "why": "the last bin, in user coordinates"},
        {"input": "THnSparseD", "target": "x", "selection": "edges", "why": "a THnSparse with a range"},
    ],
)


def tree_filter(selection):
    """the filter selecting the events of the bins of a selection (an edge bin open to the flow side)"""
    width = (HIGH - LOW) / NBINS
    terms = []
    for sel in SELECTIONS[selection]:
        chosen = list(bins(sel))
        var = AXES[sel[0]]
        lo, hi = min(chosen), max(chosen)
        if lo > 0:
            terms.append(f"{var} >= {LOW + (lo - 1) * width!r}")
        if hi <= NBINS:
            terms.append(f"{var} < {LOW + hi * width!r}")
    return " && ".join(terms)


for i, case in enumerate(todo):
    dims = TARGETS[case["target"]]
    plot = S.Plot(f"r{i}", "routes", "1d" if len(dims) == 1 else "2d")
    if case["input"] == "tree":
        data = plot[1].AddData("tree", "trees")
        filter_ = tree_filter(case["selection"])
        if filter_:
            data.Filter(filter_)
        dimsSpec = [(AXES[d], NBINS, [LOW, HIGH]) for d in dims]
        if case["target"].startswith("profile"):
            data.Profile(dimsSpec, AXES[len(dims)], "w")
        elif len(dims) == 1:
            data.Project1D(dimsSpec[0], "w")
        else:
            data.Project2D(dimsSpec[0], dimsSpec[1], "w")
    else:
        data = plot[1].AddData(case["input"], "hists")
        ranges = [(s[0], s[1], s[2]) for s in SELECTIONS[case["selection"]]]
        users = {s[3] for s in SELECTIONS[case["selection"]]}
        user = users.pop() if users else None
        profile = case["target"].startswith("profile")
        if case["input"] == "TH2D":
            select = data.ProfileX if profile else data.ProjectX
            if ranges:
                select(ranges[0][1], ranges[0][2], user)
            else:
                select()
        else:
            select = data.Profile if profile else data.Project
            select(dims, ranges, user) if user is not None else select(dims, ranges)
    data.SetDrawOption("hist" if len(dims) == 1 else "colz")
    p.pm.AddPlot(plot)
run = p.plot("routes")

expectedErrors = []
for i, case in enumerate(todo):
    objs = run.drawn(f"r{i}", "routes")
    if case["target"].startswith("profile") and case["input"] == "THnD":  # ROOT has no profiles of a THn
        expectedErrors += ["Cannot do profiles for type THnT<double> (THnD:hists)", "Projection failed for THnD:hists", f"Plot r{i} from group routes could not be created"]
        check(f"reported as not possible: {describe(case)}", not objs, "drawn")
        continue
    rel = 1e-6 if case["target"].startswith("profile") else 1e-9  # the spread of a profile bin with one value is rounding noise
    diff = hist_diff(reference(case["target"], case["selection"], case["input"] != "tree"), objs[0][0] if objs else None, rel)
    check(f"as the selected events: {describe(case)}", not diff, diff or "")
check("no warnings", not run.warnings(), first_five(run.warnings()))
unmatched, unexpected = match_messages(run.errors(), expectedErrors, False)
check("every error expected", not unexpected, first_five(unexpected))
check("every expected error given", not unmatched, first_five(unmatched))
finish()
