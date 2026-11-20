"""Every modifier of the data on every data kind in every variant gives the result of the same operation done with plain ROOT on the data
as drawn without the modifier, or is reported as not supported for that kind and leaves the data as it is. The modifiers act in one
order, whatever the order they are set in: ScaleX/Y/Z of the axes, Rebin, DivideBinWidth, Smooth, Normalize, Cumulative,
NormalizeToMaximum, Scale; for histograms also stored as float, integer and char (TH1F, TH1I, TH1C, ...: ROOT computes in their type).
Histograms (also projected THn, TEfficiency 2d): ROOT's Rebin, Scale(1, "width") (a TH2Poly by bin area),
Smooth (1d and 2d), Integral, GetCumulative (1d), GetMaximum; profiles: their means, smoothed over the filled bins; graphs (also TEfficiency
1d): the integral is the sum of the values, the maximum the largest value, smoothing sorts the points (TGraphSmooth::SmoothSuper); 2d graphs:
NormalizeToMaximum and the scale factors; functions: the scale factors (of the arguments or of the value). Normalize and
NormalizeToMaximum within a range in x take the bins reaching into it, the points inside it. Each warning names its data, and every
warning is expected. Rebinning a labelled axis is warned about (ROOT merges the bins but keeps the first labels; not compared). A
rebinning group larger than its axis leaves that axis as it is, with a warning; contents a modifier brings beyond
the largest float or to infinity are reported as those of the input, and are set to zero without adding bins to an axis with bin
labels.
Covers: TGraphMultiErrors scaled: both error dimensions follow; the Normalize range of a TH2Poly; graphs scaled by a negative
factor (Normalize of values summing up below zero) keep positive errors, low and high swapped; a TGraphMultiErrors sorted for Smooth keeps
each error with its point; messages after Cumulative name the data; scaled functions keep their points per axis and their contour
levels (given levels scaled with the values, as TH1::Scale does).
"""

import bisect
import math
import re

import numpy as np

from srptest import (
    COMBINATIONS,
    ROOT,
    DATA_KINDS,
    GRAPHS,
    MODIFIERS,
    STORAGES,
    STORED_KINDS,
    VARIANTS,
    ModifierExpected,
    Project,
    S,
    add_data,
    base_of,
    cases,
    check,
    data_name,
    describe,
    drawn_type,
    family,
    finish,
    func_diff,
    graph_diff,
    has_variant,
    holds_nonfinite,
    hist_diff,
    inner_cells,
    modifier_reference,
    profile_means,
    source_of,
    variant_warnings,
)

p = Project("modifiers", bases=("1d", "2d"))
p.inputs(VARIANTS, tuple(STORAGES))
todo = cases(
    {"kind": list(DATA_KINDS), "mods": [(m,) for m in MODIFIERS] + list(COMBINATIONS), "variant": list(VARIANTS), "storage": list(STORAGES)},
    valid=lambda c: has_variant(c["kind"], c["variant"]) and (c["storage"] == "double" or c["kind"] in STORED_KINDS),
    always=lambda c: c["variant"] == "normal" and c["storage"] == "double",
    named=[
        {"kind": "graph_multi", "mods": ("scale",), "variant": "normal", "storage": "double", "why": "both error dimensions of a scaled TGraphMultiErrors follow"},
        {"kind": "hist_var", "mods": ("rebin_x",), "variant": "normal", "storage": "double", "why": "bins no multiple of the group"},
        {"kind": "hist2poly", "mods": ("normalize_range",), "variant": "normal", "storage": "double", "why": "range of a TH2Poly"},
        {"kind": "hist2poly", "mods": ("norm_max_range",), "variant": "normal", "storage": "double", "why": "range of a TH2Poly"},
        {"kind": "graph_asymm", "mods": ("normalize_range",), "variant": "negative", "storage": "double", "why": "negative factor of a graph"},
        {"kind": "graph_multi", "mods": ("normalize_range",), "variant": "negative", "storage": "double", "why": "negative factor of a graph"},
        {"kind": "graph_multi", "mods": ("smooth",), "variant": "unsorted", "storage": "double", "why": "sorting a TGraphMultiErrors"},
        {"kind": "hist", "mods": ("norm_max", "cumulative"), "variant": "zeros", "storage": "double", "why": "name after Cumulative"},
        {"kind": "efficiency_weighted", "mods": ("scale",), "variant": "empty", "storage": "double", "why": "an efficiency without events is not drawn"},
    ],
)
bases = {}  # (kind, variant): the plot of the data as drawn without modifiers


def add_plot(name, case, mods):
    p.pm.AddDataSource(name, p.path(f"{source_of(case['variant'], case['storage'])}.root"))
    plot = S.Plot(name, "modifiers", base_of(case["kind"]))
    data = add_data(plot[1], case["kind"], name, case["variant"])
    for mod in mods:
        MODIFIERS[mod](data)
    p.pm.AddPlot(plot)


for i, case in enumerate(todo):
    key = (case["kind"], case["variant"], case["storage"])
    if key not in bases:
        bases[key] = f"base{len(bases)}"
        add_plot(bases[key], case, ())
    add_plot(f"mod{i}", case, case["mods"])
run = p.plot("modifiers")

expectedWarnings = [(f" {kind}:{name} has", "NaN or infinite") for (kind, variant, storage), name in bases.items() if holds_nonfinite(kind, variant, storage)]  # (data name, fragment)
expectedWarnings += [(f"Graph {kind}:{name} ", "has no points: it is not drawn") for (kind, variant, _), name in bases.items() if variant == "empty" and (DATA_KINDS[kind][1] in GRAPHS or drawn_type(kind) == "graph")]
expectedWarnings += [w for kind, variant, _ in bases for w in variant_warnings(kind, variant)]
silent = set()  # data whose warnings are not compared
for i, case in enumerate(todo):
    what = describe(case)
    kind = case["kind"]
    formulaKind = "formula" in DATA_KINDS[kind][2]
    if holds_nonfinite(kind, case["variant"], case["storage"]):
        expectedWarnings.append((f" {kind}:mod{i} has", "NaN or infinite"))
    expectedWarnings += variant_warnings(kind, case["variant"])
    base, obs = run.drawn(bases[(kind, case["variant"], case["storage"])], "modifiers"), run.drawn(f"mod{i}", "modifiers")
    if not base and not obs and case["variant"] == "empty" and (DATA_KINDS[kind][1] in GRAPHS or drawn_type(kind) == "graph"):
        expectedWarnings.append((f"Graph {kind}:mod{i} ", "has no points: it is not drawn"))
        silent.add(f"{kind}:mod{i}")  # what its modifiers report about no points is not compared
        continue  # an empty graph is not drawn (see frames/robust_frames.py), modified or not
    if not base or not obs:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    base, obs = base[0][0], obs[0][0]
    exp = ModifierExpected(data_name(kind, case["variant"]) if formulaKind else obs.GetName().split(":", 1)[1].removesuffix("_cumulative").split("_Proj")[0])
    fam = family(base)
    ref = modifier_reference(base, case["mods"], exp)
    expectedWarnings += [(exp.name, w) for w in exp.warnings]
    if exp.mixed:
        continue  # a labelled axis rebinned: warned, ROOT's result has the wrong labels
    if fam == "func":
        diff = func_diff(ref, obs, 1e-9)
        points = [tuple(getattr(f, f"GetNp{a}")() for a in "xyz"[: f.GetNdim()]) for f in (base, obs)]
        if not diff and points[0] != points[1]:
            diff = f"points per axis {points[1]}, unmodified {points[0]}"
    elif fam in ("graph", "graph2d"):
        diff = graph_diff(ref, obs, 1e-9)
    else:
        # float histograms (TEfficiency 2d) and profiles (sums of squares) are compared to float precision
        diff = hist_diff(ref, profile_means(obs) if fam == "profile" else obs, 1e-6 if fam == "profile" or base.ClassName().endswith("F") else 1e-9)
        if fam == "profile" and not diff and set(case["mods"]) & {"smooth", "smooth_3"}:
            entries = [obs.GetBinEntries(c) for c in range(obs.GetNcells())] == [base.GetBinEntries(c) for c in range(base.GetNcells())]
            diff = "" if entries else "entries of the bins changed"
    check(f"as plain ROOT: {what}", not diff, diff)

unmatched = list(expectedWarnings)
unexpected = []
for line in run.warnings():
    match = next((e for e in unmatched if re.search(re.escape(e[0]) + "(?![0-9])", line) and e[1] in line), None)
    if match:
        unmatched.remove(match)
    elif not any(re.search(re.escape(name) + "(?![0-9])", line) for name in silent):
        unexpected.append(line)
check("every warning expected", not unexpected, " | ".join(unexpected[:5]) + (f" (+{len(unexpected) - 5})" if len(unexpected) > 5 else ""))
check("every expected warning given", not unmatched, " | ".join(f"{n}: {w}" for n, w in unmatched[:5]) + (f" (+{len(unmatched) - 5})" if len(unmatched) > 5 else ""))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))

# the limits of the modifiers: a group larger than its axis, contents brought beyond the largest float or to infinity
q = Project("modifier_limits")
limits = {"h1": ROOT.TH1D("h1", "", 10, 0.0, 10.0), "h2": ROOT.TH2D("h2", "", 10, 0.0, 10.0, 8, 0.0, 8.0)}
limits["h3"] = ROOT.TH3D("h3", "", 4, 0.0, 4.0, 4, 0.0, 4.0, 4, 0.0, 4.0)
for h in limits.values():
    for c in range(h.GetNcells()):
        h.SetBinContent(c, 1.0 + c % 7)
limits["h1l"] = limits["h1"].Clone("h1l")  # bin labels: ROOT doubles such an axis when its overflow bin is set
for b in range(1, 11):
    limits["h1l"].GetXaxis().SetBinLabel(b, f"bin {b}")
q.source("lim", limits)
limitCases = {  # plot: (data, base, modifier, expected warning, expected bins along x, y, z)
    "x1": ("h1", "1d", lambda d: d.RebinX(11), "RebinX(11) of h1:lim is ignored: it needs at least as many bins, the axis has 10", (10,)),
    "x2": ("h2", "2d", lambda d: d.RebinX(11), "RebinX(11) of h2:lim is ignored: it needs at least as many bins, the axis has 10", (10, 8)),
    "xy2": ("h2", "2d", lambda d: d.RebinXY(11, 2), "RebinX(11) of h2:lim is ignored: it needs at least as many bins, the axis has 10", (10, 4)),
    "z3": ("h3", "2d", lambda d: d.RebinXYZ(2, 2, 5), "RebinZ(5) of h3:lim is ignored: it needs at least as many bins, the axis has 4", (2, 2, 4)),
    "float": ("h1", "1d", lambda d: d.Scale(1e40), "h1:lim has 12 bins with contents beyond the largest float", (10,)),
    "inf": ("h1", "1d", lambda d: d.Scale(1e300), "h1:lim has 12 bins with NaN or infinite values", (10,)),
    "inf_labels": ("h1l", "1d", lambda d: d.Scale(1e300), "h1l:lim has 12 bins with NaN or infinite values", (10,)),
}
for name, (data, base, modify, _, _) in limitCases.items():
    plot = S.Plot(name, "limits", base)
    modify(plot[1].AddData(data, "lim"))
    q.pm.AddPlot(plot)
limitRun = q.plot("limits")
for name, (_, _, _, warning, bins) in limitCases.items():
    obs = limitRun.objects(name, "limits")
    got = tuple(getattr(obs[0], f"GetNbins{axis}")() for axis in "XYZ"[: len(bins)]) if obs else None
    check(f"limits of the modifiers, {name}: the warning", bool(limitRun.warnings(warning)), " | ".join(limitRun.warnings()[:3]))
    check(f"limits of the modifiers, {name}: the bins", got == bins, f"{got}, expected {bins}")
check("limits of the modifiers: no errors", not limitRun.errors(), " | ".join(limitRun.errors()[:3]))

# rebinning to new edges: every histogram class against the sum of the contents and squared errors of the old cells in each new bin
# (profiles against ROOT's TProfile::Rebin), unsupported kinds and edges that are no bin edges of the axis warned about and left as they are
v = Project("rebin_edges")
varEdges = ROOT.std.vector("double")([0.0, 1.0, 3.0, 4.0, 6.0, 10.0])
edgeInputs = {
    "e1": ROOT.TH1D("e1", "", 10, 0.0, 10.0),
    "e1n": ROOT.TH1D("e1n", "", 10, 0.0, 10.0),  # without stored errors
    "e1v": ROOT.TH1D("e1v", "", varEdges.size() - 1, varEdges.data()),
    "e1t": ROOT.TH1D("e1t", "", 3, 0.0, 1.0),  # edges at thirds
    "e2": ROOT.TH2D("e2", "", 10, 0.0, 10.0, 8, 0.0, 8.0),
    "e2n": ROOT.TH2D("e2n", "", 10, 0.0, 10.0, 8, 0.0, 8.0),
    "e2f": ROOT.TH2F("e2f", "", 10, 0.0, 10.0, 8, 0.0, 8.0),
    "e3": ROOT.TH3D("e3", "", 4, 0.0, 4.0, 6, 0.0, 6.0, 4, 0.0, 4.0),
    "ep1": ROOT.TProfile("ep1", "", 10, 0.0, 10.0),
    "ep2": ROOT.TProfile2D("ep2", "", 4, 0.0, 4.0, 4, 0.0, 4.0),
    "ep3": ROOT.TProfile3D("ep3", "", 4, 0.0, 4.0, 4, 0.0, 4.0, 4, 0.0, 4.0),
    "epoly": ROOT.TH2Poly("epoly", "", 0.0, 4.0, 0.0, 4.0),
    "eg": ROOT.TGraph(3, np.array([1.0, 2.0, 3.0]), np.array([1.0, 4.0, 9.0])),
}
edgeInputs["eg"].SetName("eg")
for name, h in edgeInputs.items():
    if not h.InheritsFrom("TH1") or h.InheritsFrom("TProfile") or h.InheritsFrom("TProfile2D") or h.InheritsFrom("TProfile3D") or name == "epoly":
        continue
    if not name.endswith("n"):
        h.Sumw2()
    for c in range(h.GetNcells()):
        h.SetBinContent(c, 1.0 + (7 * c) % 11)
        if not name.endswith("n"):
            h.SetBinError(c, 0.1 + 0.05 * (c % 5))
    h.SetEntries(100)
for i in range(10):
    edgeInputs["ep1"].Fill(i + 0.5, 2.0 + i)
    edgeInputs["ep1"].Fill(i + 0.5, 3.0 + 2 * i)
for i in range(4):
    edgeInputs["ep2"].Fill(i + 0.5, i + 0.5, 1.0 + i)
    edgeInputs["ep3"].Fill(i + 0.5, i + 0.5, i + 0.5, 1.0 + i)
    cell = ROOT.TGraph(4, np.array([i, i + 1.0, i + 1.0, i]), np.array([0.0, 0.0, 4.0, 4.0]))
    edgeInputs["epoly"].AddBin(cell)  # owned by the TH2Poly
edgeInputs["e1l"] = edgeInputs["e1"].Clone("e1l")  # bin labels: ROOT doubles such an axis when it is painted after a rebin
for b in range(1, 11):
    edgeInputs["e1l"].GetXaxis().SetBinLabel(b, f"bin {b}")
v.source("ed", edgeInputs)
edgeCases = {  # plot: (data, base, modifier, new edges per axis (None: unchanged) or None: as the input, expected warning fragments)
    "x1": ("e1", "1d", lambda d: d.RebinX([0.0, 2.0, 5.0, 10.0]), ([0.0, 2.0, 5.0, 10.0],), []),
    "x1_part": ("e1", "1d", lambda d: d.RebinX([2.0, 5.0, 8.0]), ([2.0, 5.0, 8.0],), []),
    "x1_rounded": ("e1", "1d", lambda d: d.RebinX([0.0, 0.1 * 3 * 10, 10.0]), ([0.0, 3.0, 10.0],), []),
    "x1_typed": ("e1t", "1d", lambda d: d.RebinX([0.0, 0.3333, 1.0]), ([0.0, 1.0 / 3, 1.0],), []),
    "x1_noerrors": ("e1n", "1d", lambda d: d.RebinX([0.0, 4.0, 10.0]), ([0.0, 4.0, 10.0],), []),
    "x1_var": ("e1v", "1d", lambda d: d.RebinX([0.0, 3.0, 6.0, 10.0]), ([0.0, 3.0, 6.0, 10.0],), []),
    "x1_numpy": ("e1", "1d", lambda d: d.RebinX(np.array([0.0, 5.0, 10.0])), ([0.0, 5.0, 10.0],), []),
    "x1_then_group": ("e1", "1d", lambda d: d.RebinX([0.0, 5.0, 10.0]).RebinX(2), ([0.0, 2.0, 4.0, 6.0, 8.0, 10.0],), []),
    "x1_group_then": ("e1", "1d", lambda d: d.RebinX(2).RebinX([0.0, 5.0, 10.0]), ([0.0, 5.0, 10.0],), []),
    "x1_labels": ("e1l", "1d", lambda d: d.RebinX([0.0, 5.0, 10.0]), ([0.0, 5.0, 10.0],), ["RebinX on the labelled axis of e1l:ed mixes up the labels"]),
    "x2": ("e2", "2d", lambda d: d.RebinX([0.0, 5.0, 10.0]), ([0.0, 5.0, 10.0], None), []),
    "xy2": ("e2", "2d", lambda d: d.RebinX([0.0, 1.0, 10.0]).RebinY([0.0, 2.0, 8.0]), ([0.0, 1.0, 10.0], [0.0, 2.0, 8.0]), []),
    "xy2_group": ("e2", "2d", lambda d: d.RebinX(2).RebinY([0.0, 3.0, 8.0]), ([0.0, 2.0, 4.0, 6.0, 8.0, 10.0], [0.0, 3.0, 8.0]), []),
    "y2_noerrors": ("e2n", "2d", lambda d: d.RebinY([0.0, 3.0, 8.0]), (None, [0.0, 3.0, 8.0]), []),
    "y2_float": ("e2f", "2d", lambda d: d.RebinY([0.0, 3.0, 8.0]), (None, [0.0, 3.0, 8.0]), []),
    "z3": ("e3", "2d", lambda d: d.RebinZ([0.0, 1.0, 4.0]).RebinY([1.0, 5.0]), (None, [1.0, 5.0], [0.0, 1.0, 4.0]), []),
    "x1_profile": ("ep1", "1d", lambda d: d.RebinX([0.0, 4.0, 5.0, 10.0]), ([0.0, 4.0, 5.0, 10.0],), []),
    "no_edge": ("e1", "1d", lambda d: d.RebinX([0.0, 2.5, 10.0]), None, ["RebinX of e1:ed is ignored: 2.5 is no bin edge of the axis (from 0 to 10)"]),
    "beyond": ("e1", "1d", lambda d: d.RebinX([0.0, 5.0, 12.0]), None, ["RebinX of e1:ed is ignored: 12 is no bin edge of the axis (from 0 to 10)"]),
    "y1": ("e1", "1d", lambda d: d.RebinY([0.0, 1.0]), None, ["RebinY of e1:ed is ignored: it is not supported for a TH1D"]),
    "z2": ("e2", "2d", lambda d: d.RebinZ([0.0, 1.0]), None, ["RebinZ of e2:ed is ignored: it is not supported for a TH2D"]),
    "profile2d": ("ep2", "2d", lambda d: d.RebinX([0.0, 2.0, 4.0]), None, ["RebinX of ep2:ed is ignored: it is not supported for a TProfile2D"]),
    "profile3d": ("ep3", "2d", lambda d: d.RebinX([0.0, 2.0, 4.0]), None, ["Rebinning of ep3:ed is ignored: it is not supported for a TProfile3D"]),
    "poly": ("epoly", "2d", lambda d: d.RebinX([0.0, 2.0, 4.0]), None, ["RebinX of epoly:ed is ignored: it is not supported for a TH2Poly"]),
    "graph": ("eg", "1d", lambda d: d.RebinX([0.0, 2.0, 4.0]), None, ["RebinX of eg:ed is ignored: it is not supported for a TGraph"]),
}
for name, (data, base, modify, _, _) in edgeCases.items():
    plot = S.Plot(name, "edges", base)
    modify(plot[1].AddData(data, "ed"))
    v.pm.AddPlot(plot)
ratioPlot = S.Plot("ratio", "edges", "1d")  # both inputs rebinned to the same edges: divided bin by bin
ratioPlot[1].AddRatio(S.Data("e1", "ed"), S.Data("e1", "ed")).Both().RebinX([0.0, 3.0, 10.0])
v.pm.AddPlot(ratioPlot)
edgeRun = v.plot("edges")


def merged(h, newEdges):
    """the reference: h (as drawn without modifiers) with the cells merged into the bins between newEdges, flows left out"""
    dim = h.GetDimension()
    axes = [getattr(h, f"Get{a}axis")() for a in "XYZ"[:dim]]
    edges = [ne if ne is not None else [a.GetBinLowEdge(b) for b in range(1, a.GetNbins() + 2)] for a, ne in zip(axes, newEdges)]
    vecs = [ROOT.std.vector("double")(e) for e in edges]
    args = [x for vec in vecs for x in (vec.size() - 1, vec.data())]
    ref = {1: ROOT.TH1D, 2: ROOT.TH2D, 3: ROOT.TH3D}[dim]("merged", "", *args)
    ref.SetDirectory(0)
    sums, squares = [0.0] * ref.GetNcells(), [0.0] * ref.GetNcells()
    for c in inner_cells(h):
        bx, by, bz = ROOT.std.vector("int")(1), ROOT.std.vector("int")(1), ROOT.std.vector("int")(1)
        h.GetBinXYZ(c, bx.data(), by.data(), bz.data())
        centre = [a.GetBinCenter(b) for a, b in zip(axes, (bx[0], by[0], bz[0]))]
        target = [bisect.bisect_right(e, x) for e, x in zip(edges, centre)]  # 1 to n inside the new edges
        if any(t < 1 or t >= len(e) for t, e in zip(target, edges)):
            continue
        cell = ref.GetBin(*(target + [0] * (3 - dim)))
        sums[cell] += h.GetBinContent(c)
        squares[cell] += h.GetBinError(c) ** 2
    for cell in range(ref.GetNcells()):
        ref.SetBinContent(cell, sums[cell])
        ref.SetBinError(cell, math.sqrt(squares[cell]) if h.GetSumw2N() else math.sqrt(abs(sums[cell])))
    return ref


for name, (data, base, _, newEdges, warnings) in edgeCases.items():
    obs = edgeRun.objects(name, "edges")
    if not obs:
        check(f"rebinning to edges, {name}: drawn", False, " | ".join(edgeRun.errors()[:2]))
        continue
    obs = obs[0]
    for warning in warnings:
        check(f"rebinning to edges, {name}: the warning", bool(edgeRun.warnings(warning)), " | ".join(edgeRun.warnings()[:3]))
    if data == "eg":
        check(f"rebinning to edges, {name}: left as it is", obs.GetN() == 3, f"{obs.GetN()} points")
        continue
    inputHist = edgeInputs[data]
    if newEdges is None:
        same = all(getattr(obs, f"GetNbins{a}")() == getattr(inputHist, f"GetNbins{a}")() for a in "XYZ")
        check(f"rebinning to edges, {name}: left as it is", same, f"{[getattr(obs, f'GetNbins{a}')() for a in 'XYZ']}")
        continue
    if family(inputHist) == "profile":
        expected = inputHist.Clone("expected")
        vec = ROOT.std.vector("double")(newEdges[0])
        expected = expected.Rebin(vec.size() - 1, "expected_rebinned", vec.data())
        diff = hist_diff(profile_means(expected), profile_means(obs), 1e-9)
    else:
        diff = hist_diff(merged(inputHist, newEdges), obs, 1e-6 if inputHist.ClassName().endswith("F") else 1e-9)
    check(f"rebinning to edges, {name}: as the merged cells", not diff, diff)
    fixed = [not getattr(obs, f"Get{a}axis")().IsVariableBinSize() for a, ne in zip("XYZ", newEdges) if ne is None]
    check(f"rebinning to edges, {name}: an axis left as it is keeps fixed bins", all(fixed), str(fixed))
ratioObs = edgeRun.objects("ratio", "edges")
ratioContents = [ratioObs[0].GetBinContent(b) for b in range(1, ratioObs[0].GetNbinsX() + 1)] if ratioObs else None
check("rebinning to edges, both ratio inputs: divided bin by bin", ratioContents == [1.0, 1.0], str(ratioContents))
expectedEdgeWarnings = [w for case in edgeCases.values() for w in case[4]]
unexpectedEdgeWarnings = [line for line in edgeRun.warnings() if not any(w in line for w in expectedEdgeWarnings)]
check("rebinning to edges: every warning expected", not unexpectedEdgeWarnings, " | ".join(unexpectedEdgeWarnings[:3]))
check("rebinning to edges: no errors", not edgeRun.errors(), " | ".join(edgeRun.errors()[:3]))

# contour levels of a scaled 2d function: a number of levels stays, given levels are scaled with the values
c = Project("contour_levels", bases=("2d",))
levelFuncs = {"auto": ROOT.TF2("auto", "x * y", 0.0, 4.0, 0.0, 4.0), "given": ROOT.TF2("given", "x * y", 0.0, 4.0, 0.0, 4.0)}
levelFuncs["auto"].SetContour(4)
givenLevels = ROOT.std.vector["double"]([1.0, 2.0, 5.0])
levelFuncs["given"].SetContour(3, givenLevels.data())
c.source("lev", levelFuncs)
for name in levelFuncs:
    plot = S.Plot(name, "levels", "2d")
    plot[1].AddData(name, "lev").Scale(2.0)
    c.pm.AddPlot(plot)
levelRun = c.plot("levels")
for name, expected in {"auto": 4, "given": [2.0, 4.0, 10.0]}.items():
    obs = levelRun.objects(name, "levels")
    got = None
    if obs:
        n = obs[0].GetContour()
        got = n if name == "auto" else [obs[0].GetContourLevel(i) for i in range(n)]
    check(f"contour levels of a scaled 2d function, {name}", got == expected, f"{got}, expected {expected}")
finish()
