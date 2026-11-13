"""Every modifier of the data on every data kind in every variant gives the result of the same operation done with plain ROOT on the data
as drawn without the modifier, or is reported as not supported for that kind and leaves the data as it is. The modifiers act in one
order, whatever the order they are set in: ScaleX/Y/Z of the axes, Rebin, DivideBinWidth, Smooth, Normalize, Cumulative,
NormalizeToMaximum, Scale. Histograms (also projected THn, TEfficiency 2d): ROOT's Rebin, Scale(1, "width") (a TH2Poly by bin area),
Smooth (1d and 2d), Integral, GetCumulative (1d), GetMaximum; profiles: their means, smoothed over the filled bins; graphs (also TEfficiency
1d): the integral is the sum of the values, the maximum the largest value, smoothing sorts the points (TGraphSmooth::SmoothSuper); 2d graphs:
NormalizeToMaximum and the scale factors; functions: the scale factors (of the arguments or of the value). Normalize and
NormalizeToMaximum within a range in x take the bins reaching into it, the points inside it. Each warning names its data, and every
warning is expected. A rebinning group larger than its axis leaves that axis as it is, with a warning; contents a modifier brings beyond
the largest float or to infinity are reported as those of the input.
Covers: TGraphMultiErrors scaled: both error dimensions follow; the Normalize range of a TH2Poly; graphs scaled by a negative
factor (Normalize of values summing up below zero) keep positive errors, low and high swapped; a TGraphMultiErrors sorted for Smooth keeps
each error with its point; messages after Cumulative name the data.
"""

import re

from srptest import (
    COMBINATIONS,
    ROOT,
    DATA_KINDS,
    GRAPHS,
    MODIFIERS,
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
    family,
    finish,
    func_diff,
    graph_diff,
    has_variant,
    hist_diff,
    modifier_reference,
    profile_means,
    source_of,
    variant_warnings,
)

p = Project("modifiers", bases=("1d", "2d"))
p.inputs(VARIANTS)
todo = cases(
    {"kind": list(DATA_KINDS), "mods": [(m,) for m in MODIFIERS] + list(COMBINATIONS), "variant": list(VARIANTS)},
    valid=lambda c: has_variant(c["kind"], c["variant"]),
    always=lambda c: c["variant"] == "normal",
    named=[
        {"kind": "graph_multi", "mods": ("scale",), "variant": "normal", "why": "both error dimensions of a scaled TGraphMultiErrors follow"},
        {"kind": "hist_var", "mods": ("rebin_x",), "variant": "normal", "why": "bins no multiple of the group"},
        {"kind": "hist2poly", "mods": ("normalize_range",), "variant": "normal", "why": "range of a TH2Poly"},
        {"kind": "hist2poly", "mods": ("norm_max_range",), "variant": "normal", "why": "range of a TH2Poly"},
        {"kind": "graph_asymm", "mods": ("normalize_range",), "variant": "negative", "why": "negative factor of a graph"},
        {"kind": "graph_multi", "mods": ("normalize_range",), "variant": "negative", "why": "negative factor of a graph"},
        {"kind": "graph_multi", "mods": ("smooth",), "variant": "unsorted", "why": "sorting a TGraphMultiErrors"},
        {"kind": "hist", "mods": ("norm_max", "cumulative"), "variant": "zeros", "why": "name after Cumulative"},
    ],
)
bases = {}  # (kind, variant): the plot of the data as drawn without modifiers


def add_plot(name, case, mods):
    p.pm.AddDataSource(name, p.path(f"{source_of(case['variant'])}.root"))
    plot = S.Plot(name, "modifiers", base_of(case["kind"]))
    data = add_data(plot[1], case["kind"], name, case["variant"])
    for mod in mods:
        MODIFIERS[mod](data)
    p.pm.AddPlot(plot)


for i, case in enumerate(todo):
    key = (case["kind"], case["variant"])
    if key not in bases:
        bases[key] = f"base{len(bases)}"
        add_plot(bases[key], case, ())
    add_plot(f"mod{i}", case, case["mods"])
run = p.plot("modifiers")

expectedWarnings = [(f" {kind} has", "NaN or infinite") for kind, variant in bases if variant in ("nan", "inf")]  # (data name, fragment)
expectedWarnings += [(f"Graph {kind} ", "has no points: it is not drawn") for kind, variant in bases if variant == "empty" and (DATA_KINDS[kind][1] in GRAPHS or kind == "efficiency")]
expectedWarnings += [w for kind, variant in bases for w in variant_warnings(kind, variant)]
silent = set()  # data whose warnings are not compared
for i, case in enumerate(todo):
    what = describe(case)
    kind = case["kind"]
    formulaKind = "formula" in DATA_KINDS[kind][2]
    if case["variant"] in ("nan", "inf"):
        expectedWarnings.append((f" {kind} has", "NaN or infinite"))
    expectedWarnings += variant_warnings(kind, case["variant"])
    base, obs = run.drawn(bases[(kind, case["variant"])], "modifiers"), run.drawn(f"mod{i}", "modifiers")
    if not base and not obs and case["variant"] == "empty" and (DATA_KINDS[kind][1] in GRAPHS or kind == "efficiency"):
        expectedWarnings.append((f"Graph {kind} ", "has no points: it is not drawn"))
        silent.add(f"{kind}:mod{i}")  # what its modifiers report about no points is not compared
        continue  # an empty graph is not drawn (see frames/robust_frames.py), modified or not
    if not base or not obs:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    base, obs = base[0][0], obs[0][0]
    exp = ModifierExpected(data_name(kind, case["variant"]) if formulaKind else obs.GetName().split(":", 1)[1].removesuffix("_cumulative"))
    fam = family(base)
    ref = modifier_reference(base, case["mods"], exp)
    expectedWarnings += [(exp.name, w) for w in exp.warnings]
    if fam == "func":
        diff = func_diff(ref, obs, 1e-9)
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
q.source("lim", limits)
limitCases = {  # plot: (data, base, modifier, expected warning, expected bins along x, y, z)
    "x1": ("h1", "1d", lambda d: d.RebinX(11), "RebinX(11) needs at least as many bins, h1:lim has 10", (10,)),
    "x2": ("h2", "2d", lambda d: d.RebinX(11), "RebinX(11) needs at least as many bins, h2:lim has 10", (10, 8)),
    "xy2": ("h2", "2d", lambda d: d.RebinXY(11, 2), "RebinX(11) needs at least as many bins, h2:lim has 10", (10, 4)),
    "z3": ("h3", "2d", lambda d: d.RebinXYZ(2, 2, 5), "RebinZ(5) needs at least as many bins, h3:lim has 4", (2, 2, 4)),
    "float": ("h1", "1d", lambda d: d.Scale(1e40), "h1 has 12 bins with contents beyond the largest float", (10,)),
    "inf": ("h1", "1d", lambda d: d.Scale(1e300), "h1 has 12 bins with NaN or infinite values", (10,)),
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
finish()
