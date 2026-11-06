"""Every numerator kind divided by every denominator kind, in every variant, with uncorrelated and binomial (SetCorrelated) errors, gives
the ratio plain ROOT gives, or is reported with an error and loses only its plot. The numerator and denominator are the data as drawn
without the division (profiles as the histograms of their means, TEfficiency as the graph or 2d histogram ROOT makes of it, THn projected).
Histograms by histograms of the same binning: TH1::Divide (TH2Poly only by the same polygons). Otherwise the division is that of the points
(the bin centres of a 1d histogram, with its errors): a denominator at other positions is interpolated with a cubic spline (TSpline3) of its
values and of its error envelope; a zero denominator drops the point (empty bin); uncorrelated errors add in quadrature, binomial ones as
TH1::Divide with option B; a negative denominator swaps asymmetric errors; a numerator without errors takes the errors of the denominator.
By a function: the value at the bin centre or point (no binomial errors), bins and points outside its range left out (a formula of the plot
definition has none). A function as numerator: sampled at the positions of the denominator. The result of a histogram is a histogram, of a
graph or function a graph. Every warning is expected.
Covers: 82a875c (function numerator by a denominator with errors), e3de5c4 (graph points removed with their errors), 4986674 (profiles
divided as their means), 5d432dd (formulas as denominators divide the whole numerator), f934095 (TGraphMultiErrors divided); TH2Poly
divided without reading past its bins; NaN in a denominator removed before the division (a spline through it was NaN everywhere); two
formulas without a range reported once; a TGraphMultiErrors denominator sorted with its errors.
"""

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
    data_name,
    data_objects,
    describe,
    division_name,
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
        {"num": "function", "den": "graph_errors", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "82a875c"},
        {"num": "graph_multi", "den": "graph", "num_variant": "nan", "den_variant": "normal", "correlated": False, "why": "e3de5c4"},
        {"num": "graph_multi", "den": "graph_errors", "num_variant": "normal", "den_variant": "negative", "correlated": False, "why": "f934095"},
        {"num": "profile", "den": "hist", "num_variant": "normal", "den_variant": "normal", "correlated": True, "why": "4986674"},
        {"num": "hist2d", "den": "function2d", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "5d432dd"},
        {"num": "hist2poly", "den": "function2d", "num_variant": "normal", "den_variant": "normal", "correlated": False, "why": "TH2Poly statistics"},
        {"num": "hist2poly", "den": "hist2poly", "num_variant": "normal", "den_variant": "normal", "correlated": True, "why": "TH2Poly statistics"},
        {"num": "hist", "den": "graph_errors", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "NaN denominator"},
        {"num": "graph_errors", "den": "hist_var", "num_variant": "normal", "den_variant": "nan", "correlated": False, "why": "NaN denominator"},
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
        expectedWarnings.append((f" {kind} has", "NaN or infinite"))
    if stand_in(kind, variant) is not None:
        expectedWarnings.append((f"Graph {kind} ", "has no points: it is not drawn"))
for i, case in enumerate(todo):
    what = describe(case)
    objs = []
    for k in ("num", "den"):
        drawnAlone = run.drawn(bases[(case[k], case[f"{k}_variant"])], "ratios")
        objs.append(drawnAlone[0][0] if drawnAlone else stand_in(case[k], case[f"{k}_variant"]))
    numObj, denObj = objs
    if not numObj or not denObj:
        check(f"data drawn alone: {what}", False, " | ".join(run.errors()[:2]))
        continue
    denInput = inputs[case["den_variant"]].get(case["den"])
    exp = DivisionExpected(division_name(case["num"], case["num_variant"], f"n{i}"), division_name(case["den"], case["den_variant"], f"d{i}"), input_range(denInput) if denInput else None)
    caseWarnings = []
    for k in ("num", "den"):  # each input reported by its name before the division
        if case[f"{k}_variant"] in ("nan", "inf") and not DATA_KINDS[case[k]][1].startswith("TF"):
            caseWarnings.append((f" {case[k]} has", "NaN or infinite"))
    profiles = [k for k in ("num", "den") if any(base.InheritsFrom(c) for base in ([numObj] if k == "num" else [denObj]) for c in ("TProfile", "TProfile2D", "TProfile3D"))]
    binomial = case["correlated"]
    if binomial and profiles:
        binomial = False
        caseWarnings.append((f"Ratio {data_name(case['num'], case['num_variant'])}:", "binomial errors (SetCorrelated) are not defined for the mean values of a profile"))
    funcs = {k: Func(case[k], case[f"{k}_variant"], numObj if k == "num" else denObj) for k in ("num", "den") if DATA_KINDS[case[k]][1].startswith("TF")}
    ref = division_reference(numObj, denObj, funcs.get("num"), funcs.get("den"), binomial, exp)
    caseWarnings += [(exp.num, w) for w in exp.warnings]
    if ref is not None:  # a failing ratio, the first data of its pad, stops the plot before the pass that reports warnings
        expectedWarnings += caseWarnings
    drawn = run.drawn(f"r{i}", "ratios")
    if ref is None:
        expectedErrors += exp.errors + [f"Plot r{i} from group ratios could not be created"]
        check(f"reported as not possible: {what}", not drawn, "the ratio was drawn")
        continue
    if isinstance(ref, dict) and not ref["x"]:  # no points left: a graph without points is not drawn
        expectedWarnings.append((f"Graph {data_name(case['num'], case['num_variant'])} ", "has no points: it is not drawn"))
        check(f"a ratio without points not drawn: {what}", not drawn, "drawn")
        continue
    if not drawn:
        check(f"ratio made: {what}", False, " | ".join(line for line in run.errors() if f":n{i}" in line or f"r{i} " in line)[:300])
        continue
    obs = drawn[0][0]
    rel = 1e-6 if numObj.ClassName().endswith("F") or denObj.ClassName().endswith("F") or "_cpp" in case["num"] + case["den"] else 1e-9
    if isinstance(ref, dict):
        ref = {k: v for k, v in ref.items() if not k.startswith("ErrorX")}  # the x errors are the numerator's (drawn or not by the alias)
        diff = values_diff(ref, graph_values(obs), rel) if obs.InheritsFrom("TGraph") else f"drawn as {obs.ClassName()}, expected a graph"
    else:
        diff = hist_diff(ref, obs, rel)
    check(f"as plain ROOT: {what}", not diff, diff)


unmatched, unexpected = match_messages(run.warnings(), expectedWarnings, True)
check("every warning expected", not unexpected, first_five(unexpected))
check("every expected warning given", not unmatched, first_five(unmatched))
unmatched, unexpected = match_messages(run.errors(), expectedErrors, False)
check("every error expected", not unexpected, first_five(unexpected))
check("every expected error given", not unmatched, first_five(unmatched))
finish()
