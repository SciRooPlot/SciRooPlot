"""Every modifier of the data on every data kind in every variant gives the result of the same operation done with plain ROOT on the data
as drawn without the modifier, or is reported as not supported for that kind and leaves the data as it is. The modifiers act in one
order, whatever the order they are set in: ScaleX/Y/Z of the axes, Rebin, DivideBinWidth, Smooth, Normalize, Cumulative,
NormalizeToMaximum, Scale. Histograms (also projected THn, TEfficiency 2d): ROOT's Rebin, Scale(1, "width") (a TH2Poly by bin area),
Smooth (1d and 2d), Integral, GetCumulative (1d), GetMaximum; profiles: their means, smoothed over the filled bins; graphs (also TEfficiency
1d): the integral is the sum of the values, the maximum the largest value, smoothing sorts the points (TGraphSmooth::SmoothSuper); 2d graphs:
NormalizeToMaximum and the scale factors; functions: the scale factors (of the arguments or of the value). Normalize and
NormalizeToMaximum within a range in x take the bins reaching into it, the points inside it. Each warning names its data, and every
warning is expected.
Covers: f934095 (TGraphMultiErrors scaled: both error dimensions follow); the Normalize range of a TH2Poly; graphs scaled by a negative
factor (Normalize of values summing up below zero) keep positive errors, low and high swapped; a TGraphMultiErrors sorted for Smooth keeps
each error with its point; messages after Cumulative name the data.
"""

import re

from srptest import (
    COMBINATIONS,
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
)

p = Project("modifiers", bases=("1d", "2d"))
p.inputs(VARIANTS)
todo = cases(
    {"kind": list(DATA_KINDS), "mods": [(m,) for m in MODIFIERS] + list(COMBINATIONS), "variant": list(VARIANTS)},
    valid=lambda c: has_variant(c["kind"], c["variant"]),
    always=lambda c: c["variant"] == "normal",
    named=[
        {"kind": "graph_multi", "mods": ("scale",), "variant": "normal", "why": "f934095"},
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
silent = set()  # data whose warnings are not compared
for i, case in enumerate(todo):
    what = describe(case)
    kind = case["kind"]
    formulaKind = "formula" in DATA_KINDS[kind][2]
    if case["variant"] in ("nan", "inf"):
        expectedWarnings.append((f" {kind} has", "NaN or infinite"))
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
finish()
