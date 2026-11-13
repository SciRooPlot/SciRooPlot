"""Every numerator kind divided by every denominator kind, in every variant, with uncorrelated and binomial (SetCorrelated) errors, gives
the ratio plain ROOT gives, or is reported with an error and loses only its plot. The numerator and denominator are the data as drawn
without the division (profiles as the histograms of their means, TEfficiency as the graph or 2d histogram ROOT makes of it, THn projected).
Histograms by histograms of the same binning: TH1::Divide (TH2Poly only by the same polygons). Otherwise the division is that of the points
(the bin centres of a 1d histogram, with its errors): a denominator at other positions is interpolated with a cubic spline (TSpline3) of its
values and of its error envelope, a histogram through its filled bins only (no knots at zero for empty bins); a zero denominator, or a
point inside an empty bin of a histogram denominator, drops the point (empty bin); uncorrelated errors add in quadrature, binomial ones as
TH1::Divide with option B; a negative denominator swaps asymmetric errors; a numerator without errors takes the errors of the denominator.
By a function: the value at the bin centre or point (no binomial errors), bins and points outside its range left out (a formula of the plot
definition has none). A function as numerator: sampled at the positions of the denominator. The result of a histogram is a histogram, of a
graph or function a graph. Every warning is expected.
Covers: function numerator by a denominator with errors, graph points removed with their errors, profiles
divided as their means, formulas as denominators divide the whole numerator, TGraphMultiErrors divided; TH2Poly
divided without reading past its bins; NaN in a denominator removed before the division (a spline through it was NaN everywhere); two
formulas without a range reported once; a TGraphMultiErrors denominator sorted with its errors; a ratio by a function at its zeros reported; a function numerator without values where the denominator has them
reported as such; two formulas giving the frame divided over the pad range; graphs at the same
x values divided point by point whatever their kinds of errors; empty denominator bins (also NaN) no
knots of the interpolation (the ratio swung around them).
"""

import math

from srptest import (
    DATA_KINDS,
    ROOT,
    DivisionExpected,
    Func,
    Project,
    S,
    base_of,
    cases,
    check,
    data_objects,
    describe,
    message_name,
    division_reference,
    finish,
    first_five,
    graph_values,
    has_variant,
    hist_diff,
    input_range,
    match_messages,
    ratio_input,
    source_of,
    values_diff,
)

NUM_VARIANTS = ("normal", "negative", "nan", "empty", "single")
DEN_VARIANTS = ("normal", "negative", "zeros", "nan", "empty", "single", "unsorted")


p = Project("ratios")
p.inputs(set(NUM_VARIANTS) | set(DEN_VARIANTS))
inputs = {variant: data_objects(variant) for variant in set(NUM_VARIANTS) | set(DEN_VARIANTS)}
KINDS = list(DATA_KINDS)
todo = cases(
    {"num": KINDS, "den": KINDS, "num_variant": list(NUM_VARIANTS), "den_variant": list(DEN_VARIANTS), "correlated": [False, True]},
    valid=lambda c: has_variant(c["num"], c["num_variant"]) and has_variant(c["den"], c["den_variant"]) and "normal" in (c["num_variant"], c["den_variant"]),
    always=lambda c: c["num_variant"] == c["den_variant"] == "normal" and not c["correlated"],
    named=[
        {"num": "function", "den": "graph_errors", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "a function numerator by a denominator with errors"},
        {"num": "graph_multi", "den": "graph", "num_variant": "nan", "den_variant": "normal", "correlated": False, "why": "graph points removed with their errors"},
        {"num": "graph_multi", "den": "graph_errors", "num_variant": "normal", "den_variant": "negative", "correlated": False, "why": "a TGraphMultiErrors divided"},
        {"num": "profile", "den": "hist", "num_variant": "normal", "den_variant": "normal", "correlated": True, "why": "profiles divided as their means"},
        {"num": "hist2d", "den": "function2d", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "a formula denominator divides the whole numerator"},
        {"num": "hist2poly", "den": "function2d", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "TH2Poly statistics"},
        {"num": "hist2poly", "den": "hist2poly", "num_variant": "normal", "den_variant": "normal", "correlated": True, "why": "TH2Poly statistics"},
        {"num": "hist", "den": "graph_errors", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "NaN denominator"},
        {"num": "hist", "den": "tf1", "num_variant": "normal", "den_variant": "zeros", "correlated": False, "why": "a function zero at every bin, reported"},
        {"num": "graph_errors", "den": "tf1_cpp", "num_variant": "normal", "den_variant": "zeros", "correlated": False, "why": "a function zero at every point, reported"},
        {"num": "graph_errors", "den": "hist_var", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "NaN denominator"},
        {"num": "hist", "den": "hist_var", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "empty denominator bins"},
        {"num": "graph_errors", "den": "hist_var", "num_variant": "normal", "den_variant": "empty", "correlated": False, "why": "empty denominator bins"},
        {"num": "tf1", "den": "hist", "num_variant": "normal", "den_variant": "single", "correlated": False, "why": "empty denominator bins"},
        {"num": "tf1", "den": "graph_errors", "num_variant": "normal", "den_variant": "empty", "correlated": False, "why": "a function numerator by a graph without points"},
        {"num": "function", "den": "function", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "one error for two formulas"},
        {"num": "graph_multi", "den": "graph_errors", "num_variant": "normal", "den_variant": "unsorted", "correlated": False, "why": "sorting a TGraphMultiErrors"},
        {"num": "hist", "den": "graph_multi", "num_variant": "normal", "den_variant": "unsorted", "correlated": False, "why": "sorting a TGraphMultiErrors"},
    ],
)


bases = {}  # (kind, variant): the plot of the data as drawn alone
for i, case in enumerate(todo):
    for kind, variant in ((case["num"], case["num_variant"]), (case["den"], case["den_variant"])):
        if (kind, variant) not in bases:
            bases[(kind, variant)] = name = f"base{len(bases)}"
            p.pm.AddDataSource(name, p.path(f"{source_of(variant)}.root"))
            plot = S.Plot(name, "ratios", base_of(kind))
            plot[1].AddData(ratio_input(kind, name, variant))
            p.pm.AddPlot(plot)
    for side, key in (("n", "num_variant"), ("d", "den_variant")):
        p.pm.AddDataSource(f"{side}{i}", p.path(f"{source_of(case[key])}.root"))
    plot = S.Plot(f"r{i}", "ratios", base_of(case["num"]))
    ratio = plot[1].AddRatio(ratio_input(case["num"], f"n{i}", case["num_variant"]), ratio_input(case["den"], f"d{i}", case["den_variant"]))
    if case["correlated"]:
        ratio.SetCorrelated()
    p.pm.AddPlot(plot)
# a function numerator whose range holds none of the points of the denominator: an error saying so (not an unsupported division)
outside = ROOT.TF1("outside", "2*x", -5.0, -4.0)
p.source("outside", {"outside": outside})
plot = S.Plot("outside_range", "ratios", "1d")
p.pm.AddDataSource("outside_den", p.path(f"{source_of('normal')}.root"))
plot[1].AddRatio(S.Data("outside", "outside"), S.Data("graph_errors", "outside_den"))
p.pm.AddPlot(plot)
# two formulas of the plot definition giving the frame are divided over the x range of the pad
plot = S.Plot("formulas_pad_range", "ratios", "1d")
plot[1]["X"].SetRange(1.0, 3.0)
plot[1].AddRatio(S.Data("x*x", S.kUserFunctions), S.Data("x", S.kUserFunctions))
p.pm.AddPlot(plot)
# graphs at the same x values divide point by point whatever their kinds of errors, also with a repeated x (no interpolation needed)
repeatedX = (1.0, 2.0, 2.0)
asymm, symm = ROOT.TGraphAsymmErrors(3), ROOT.TGraphErrors(3)
for i, x in enumerate(repeatedX):
    asymm.SetPoint(i, x, 2.0 + i)
    asymm.SetPointError(i, 0.0, 0.0, 0.2, 0.4)
    symm.SetPoint(i, x, 1.0 + i)
    symm.SetPointError(i, 0.0, 0.1)
asymm.SetName("asymm")
symm.SetName("symm")
p.source("same_x", {"asymm": asymm, "symm": symm})
for name, (num, den) in {"same_x_asymm": ("asymm", "symm"), "same_x_symm": ("symm", "asymm")}.items():
    plot = S.Plot(name, "ratios", "1d")
    plot[1].AddRatio(S.Data(num, "same_x"), S.Data(den, "same_x"))
    p.pm.AddPlot(plot)
run = p.plot("ratios")


def stand_in(kind, variant):
    """an empty graph is not drawn alone (see frames/robust_frames.py): the input instead (no points either)"""
    dim, cls, _ = DATA_KINDS[kind]
    if variant != "empty" or not (cls.startswith("TGraph") or (cls == "TEfficiency" and dim == 1)):
        return None
    return ROOT.TGraphAsymmErrors() if cls == "TEfficiency" else inputs[variant][kind]


expectedWarnings, expectedErrors = [], []
for kind, variant in bases:
    if variant in ("nan", "inf"):
        expectedWarnings.append((f" {kind}:{bases[(kind, variant)]} has", "NaN or infinite"))
    if stand_in(kind, variant) is not None:
        expectedWarnings.append((f"Graph {kind}:{bases[(kind, variant)]} ", "has no points: it is not drawn"))


def compare(i, case):
    """checks one ratio against its reference; collects its expected messages"""
    what = describe(case)
    objs = []
    for k in ("num", "den"):
        drawnAlone = run.drawn(bases[(case[k], case[f"{k}_variant"])], "ratios")
        objs.append(drawnAlone[0][0] if drawnAlone else stand_in(case[k], case[f"{k}_variant"]))
    numObj, denObj = objs
    if not numObj or not denObj:
        check(f"data drawn alone: {what}", False, " | ".join(run.errors()[:2]))
        return
    denInput = inputs[case["den_variant"]].get(case["den"])
    exp = DivisionExpected(message_name(case["num"], case["num_variant"], f"n{i}"), message_name(case["den"], case["den_variant"], f"d{i}"), input_range(denInput) if denInput else None)
    caseWarnings = []
    for k in ("num", "den"):  # each input reported by its name before the division
        if case[f"{k}_variant"] in ("nan", "inf") and not DATA_KINDS[case[k]][1].startswith("TF"):
            caseWarnings.append((f" {case[k]}:{k[0]}{i} has", "NaN or infinite"))
    profiles = [k for k in ("num", "den") if any(base.InheritsFrom(c) for base in ([numObj] if k == "num" else [denObj]) for c in ("TProfile", "TProfile2D", "TProfile3D"))]
    binomial = case["correlated"]
    if binomial and profiles:
        binomial = False
        caseWarnings.append((f"Ratio {message_name(case['num'], case['num_variant'], f'n{i}')}:", "binomial errors (SetCorrelated) are not defined for the mean values of a profile"))
    funcs = {k: Func(case[k], case[f"{k}_variant"], numObj if k == "num" else denObj) for k in ("num", "den") if DATA_KINDS[case[k]][1].startswith("TF")}
    ref = division_reference(numObj, denObj, funcs.get("num"), funcs.get("den"), binomial, exp)
    caseWarnings += [(exp.num, w) for w in exp.warnings]
    if ref is not None:  # a failing ratio, the first data of its pad, stops the plot before the pass that reports warnings
        expectedWarnings.extend(caseWarnings)
    drawn = run.drawn(f"r{i}", "ratios")
    if ref is None:
        expectedErrors.extend(exp.errors + [f"Plot r{i} from group ratios could not be created"])
        check(f"reported as not possible: {what}", not drawn, "the ratio was drawn")
        return
    if isinstance(ref, dict) and not ref["x"]:  # no points left: a graph without points is not drawn
        expectedWarnings.append((f"Graph {message_name(case['num'], case['num_variant'], f'n{i}')} ", "has no points: it is not drawn"))
        check(f"a ratio without points not drawn: {what}", not drawn, "drawn")
        return
    if not drawn:
        check(f"ratio made: {what}", False, " | ".join(line for line in run.errors() if f":n{i}" in line or f"r{i} " in line)[:300])
        return
    obs = drawn[0][0]
    rel = 1e-6 if numObj.ClassName().endswith("F") or denObj.ClassName().endswith("F") or "_cpp" in case["num"] + case["den"] else 1e-9
    if isinstance(ref, dict):
        ref = {k: v for k, v in ref.items() if not k.startswith("ErrorX")}  # the x errors are the numerator's (drawn or not by the alias)
        diff = values_diff(ref, graph_values(obs), rel) if obs.InheritsFrom("TGraph") else f"drawn as {obs.ClassName()}, expected a graph"
    else:
        diff = hist_diff(ref, obs, rel)
    check(f"as plain ROOT: {what}", not diff, diff)


for i, case in enumerate(todo):
    with run.held(colours=False):  # thousands of canvases: each case frees those it read
        compare(i, case)


check("a function numerator without values at the denominator: not drawn", not run.drawn("outside_range", "ratios"))
expectedErrors += ["none of the points of", "Plot outside_range from group ratios could not be created"]
padRange = run.drawn("formulas_pad_range", "ratios")
ratioPoints = [padRange[0][0].GetPointX(i) for i in range(padRange[0][0].GetN())] if padRange and padRange[0][0].InheritsFrom("TGraph") else []
check(
    "two formulas giving the frame: divided over the x range of the pad",
    ratioPoints and math.isclose(ratioPoints[0], 1.0) and math.isclose(ratioPoints[-1], 3.0) and all(math.isclose(padRange[0][0].GetPointY(i), x) for i, x in enumerate(ratioPoints)),
    str(ratioPoints[:3]) + " | " + " | ".join(run.errors("formulas_pad_range")),
)
for name, (num, den) in {"same_x_asymm": (asymm, symm), "same_x_symm": (symm, asymm)}.items():
    drawn = run.drawn(name, "ratios")
    got = [drawn[0][0].GetPointY(i) for i in range(drawn[0][0].GetN())] if drawn else []
    want = [num.GetPointY(i) / den.GetPointY(i) for i in range(3)]
    check(f"graphs at the same x values with a repeated x divided point by point: {name}", len(got) == 3 and all(math.isclose(a, b) for a, b in zip(got, want)), f"{got} expected {want}" + " | ".join(run.errors(name)))
unmatched, unexpected = match_messages(run.warnings(), expectedWarnings, True)
check("every warning expected", not unexpected, first_five(unexpected))
check("every expected warning given", not unmatched, first_five(unmatched))
unmatched, unexpected = match_messages(run.errors(), expectedErrors, False)
check("every error expected", not unexpected, first_five(unexpected))
check("every expected error given", not unmatched, first_five(unmatched))
finish()
