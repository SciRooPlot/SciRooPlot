"""Every modifier on each side of a ratio: Numer(), Denom() and Both() act on the inputs before the division, Result() (the default) on the
ratio. The ratio is the division of the inputs as drawn alone with the same modifiers (data/modifiers.py holds those against plain ROOT),
done as data/ratios.py does it (spline of a denominator at other positions, errors, zeros left out); in Result mode the modifiers of the
plain ratio as for any data. Every input kind divided by itself, and pairs of a histogram, a graph and a function. Each warning names its
data (the modifiers of an input name that input; the denominator of a function numerator, prepared to sample it and to divide by,
reports once), and every warning is expected.
Covers: the ratio sides (both normalized, both rebinned, result scaled, numerator scaled), profiles
rebinned before the division, a modifier refused where it is set naming the input of its side.
"""

from srptest import (
    AXIS_FACTOR,
    COMBINATIONS,
    DATA_KINDS,
    MODIFIERS,
    DivisionExpected,
    Func,
    ModifierExpected,
    Project,
    S,
    ROOT,
    base_of,
    captured,
    cases,
    check,
    data_objects,
    describe,
    message_name,
    division_reference,
    family,
    finish,
    first_five,
    has_variant,
    func_diff,
    graph_diff,
    graph_values,
    hist_diff,
    input_range,
    match_messages,
    modifier_reference,
    profile_means,
    ratio_input,
    source_of,
    values_diff,
)

# inputs divided by themselves, as long as such a ratio exists (2d graphs, 2d and 3d functions cannot be numerators, two formulas of the
# plot definition have no range to be divided in), and mixed pairs
SAME = [k for k, (dim, cls, props) in DATA_KINDS.items() if not cls.startswith("TGraph2D") and not (cls.startswith("TF") and dim > 1) and k != "function"]
PAIRS = [(k, k) for k in SAME] + [
    ("hist", "graph_errors"),
    ("graph_asymm", "hist"),
    ("hist_var", "hist"),
    ("graph_multi", "graph_errors"),
    ("tf1", "hist"),
    ("hist", "tf1"),
    ("function", "graph_errors"),
    ("graph_errors", "function"),
    ("hist2d", "function2d"),
]
SIDES = {"numer": lambda r: r.Numer(), "denom": lambda r: r.Denom(), "both": lambda r: r.Both(), "result": lambda r: r.Result()}
VARIANTS = ("normal", "empty", "single", "nan")  # hostile data on both sides: no entries or points, one, a NaN value

p = Project("ratiosides")
p.inputs(VARIANTS)
inputs = {variant: data_objects(variant) for variant in VARIANTS}
todo = cases(
    {"pair": PAIRS, "side": list(SIDES), "mods": [(m,) for m in MODIFIERS] + list(COMBINATIONS), "variant": list(VARIANTS)},
    valid=lambda c: all(has_variant(kind, c["variant"]) for kind in c["pair"]),
    always=lambda c: c["pair"] == ("hist", "hist") and c["variant"] == "normal",
    named=[
        {"pair": ("hist", "hist"), "side": "both", "mods": ("normalize",), "variant": "normal", "why": "both sides normalized"},
        {"pair": ("hist", "hist"), "side": "numer", "mods": ("rebin_x",), "variant": "normal", "why": "numerator rebinned: other binning, interpolated"},
        {"pair": ("profile", "profile"), "side": "both", "mods": ("rebin_x",), "variant": "normal", "why": "profiles divided as their means"},
        {"pair": ("tf1", "tf1"), "side": "denom", "mods": ("normalize",), "variant": "normal", "why": "the denominator of a function numerator reported once"},
    ],
)


def add_plot(name, kind, variant, mods=(), source=None):
    """the data alone (with modifiers) in a plot of its own, read through a data source of its own"""
    p.pm.AddDataSource(source or name, p.path(f"{source_of(variant)}.root"))
    plot = S.Plot(name, "sides", base_of(kind))
    data = plot[1].AddData(ratio_input(kind, source or name, variant))
    for mod in mods:
        MODIFIERS[mod](data)
    p.pm.AddPlot(plot)


def add_ratio(name, pair, variant, side=None, mods=()):
    for prefix in ("n", "d"):
        p.pm.AddDataSource(f"{prefix}{name}", p.path(f"{source_of(variant)}.root"))
    plot = S.Plot(name, "sides", base_of(pair[0]))
    ratio = plot[1].AddRatio(ratio_input(pair[0], f"n{name}", variant), ratio_input(pair[1], f"d{name}", variant))
    if side:
        SIDES[side](ratio)
        for mod in mods:
            MODIFIERS[mod](ratio)
    p.pm.AddPlot(plot)


bases, plain = {}, {}  # the inputs drawn alone without modifiers, the plain ratio of each pair
for i, case in enumerate(todo):
    (num, den), variant = case["pair"], case["variant"]
    for kind in (num, den):
        if (kind, variant) not in bases:
            bases[(kind, variant)] = f"b{len(bases)}"
            add_plot(bases[(kind, variant)], kind, variant)
    if case["side"] in ("numer", "both"):
        add_plot(f"on{i}", num, variant, case["mods"])
    if case["side"] in ("denom", "both"):
        add_plot(f"od{i}", den, variant, case["mods"])
    if case["side"] == "result" and (case["pair"], variant) not in plain:
        plain[(case["pair"], variant)] = f"p{len(plain)}"
        add_ratio(plain[(case["pair"], variant)], case["pair"], variant)
    add_ratio(f"r{i}", case["pair"], variant, case["side"], case["mods"])
run = p.plot("sides")


def drawn(name):
    objs = run.drawn(name, "sides")
    return objs[0][0] if objs else None


def name_of(kind, variant, source):
    """the name of the data (an input or a ratio) in messages of its modifiers"""
    return message_name(kind, variant, source)


def stand_in(kind, variant):
    """an empty graph is not drawn alone (see frames/robust_frames.py): the input instead (no points either)"""
    dim, cls, _ = DATA_KINDS[kind]
    if variant != "empty" or not (cls.startswith("TGraph") or (cls == "TEfficiency" and dim == 1)):
        return None
    return ROOT.TGraphAsymmErrors() if cls == "TEfficiency" else inputs[variant][kind]


def alone(name, kind, variant):
    """the data drawn alone, or what stands in for an empty graph"""
    return drawn(name) or stand_in(kind, variant)


def operand(kind, variant, source, plot, mods, warnings):
    """an input as divided: drawn alone (plot) with its modifiers, a function with its exact values; the warnings of its modifiers in the
    ratio (input read from source)"""
    base = alone(bases[(kind, variant)], kind, variant)
    exp = ModifierExpected(name_of(kind, variant, source))
    ref = modifier_reference(base, mods, exp)
    warnings += [(exp.name, w) for w in exp.warnings]
    obj = alone(plot, kind, variant) if mods else base
    if not DATA_KINDS[kind][1].startswith("TF"):
        return obj, None
    return obj, Func(kind, variant, obj, ref if mods else None)


def division(num, den, numFunc, denFunc, names, denMods, variant, warnings, errors):
    if den[1].InheritsFrom("TH1"):  # the axis of the denominator as modified (a rebin may leave out the last bins)
        denRange = (den[1].GetXaxis().GetXmin(), den[1].GetXaxis().GetXmax())
    else:  # a graph drawn alone has lost its x errors: those of the input
        denInput = inputs[variant].get(den[0])
        denRange = input_range(denInput) if denInput else None
        if denRange and "scale_x" in denMods:
            denRange = (denRange[0] * AXIS_FACTOR, denRange[1] * AXIS_FACTOR)
    exp = DivisionExpected(*names, denRange)
    ref = division_reference(num[1], den[1], numFunc, denFunc, False, exp)
    errors += exp.errors
    return ref, [(exp.num, w) for w in exp.warnings]


def compare(ref, obs):
    if isinstance(ref, dict):
        ref = {k: v for k, v in ref.items() if not k.startswith("ErrorX")}  # the x errors are the numerator's (drawn or not by the alias)
        return values_diff(ref, graph_values(obs), 1e-9) if obs.InheritsFrom("TGraph") else f"drawn as {obs.ClassName()}, expected a graph"
    if callable(ref):
        return func_diff(ref, obs, 1e-9)
    if any(obs.InheritsFrom(c) for c in ("TProfile", "TProfile2D", "TProfile3D")):
        obs = profile_means(obs)
    return hist_diff(ref, obs, 1e-6 if ref.ClassName().endswith("F") else 1e-9)


def input_warnings(kind, variant, source, drawnAlone):
    """the warnings of an input itself: a NaN value, a graph without points drawn alone"""
    found = []
    if variant in ("nan", "inf") and not DATA_KINDS[kind][1].startswith("TF"):
        found.append((f" {kind}:{source} has", "NaN or infinite"))
    if drawnAlone and stand_in(kind, variant) is not None:
        found.append((f"Graph {kind}:{source} ", "has no points: it is not drawn"))
    return found


expectedWarnings, expectedErrors = [], []
for (kind, variant), name in bases.items():
    expectedWarnings += input_warnings(kind, variant, name, True)
for (pair, variant), name in plain.items():  # the plain ratios themselves
    numObj, denObj = alone(bases[(pair[0], variant)], pair[0], variant), alone(bases[(pair[1], variant)], pair[1], variant)
    numFunc = Func(pair[0], variant, numObj) if DATA_KINDS[pair[0]][1].startswith("TF") else None
    denFunc = Func(pair[1], variant, denObj) if DATA_KINDS[pair[1]][1].startswith("TF") else None
    names = (name_of(pair[0], variant, f"n{name}"), name_of(pair[1], variant, f"d{name}"))
    ref, warns = division((pair[0], numObj), (pair[1], denObj), numFunc, denFunc, names, (), variant, [], expectedErrors)
    if ref is not None:
        expectedWarnings += warns + input_warnings(pair[0], variant, f"n{name}", False) + input_warnings(pair[1], variant, f"d{name}", False)
    if isinstance(ref, dict) and not ref["x"]:
        expectedWarnings.append((f"Graph {names[0]} ", "has no points: it is not drawn"))
    if ref is None:
        expectedErrors.append(f"Plot {name} from group sides could not be created")
for i, case in enumerate(todo):
    what = describe(case)
    (num, den), side, mods, variant = case["pair"], case["side"], case["mods"], case["variant"]
    names = (name_of(num, variant, f"nr{i}"), name_of(den, variant, f"dr{i}"))
    caseWarnings, caseErrors = [], []
    for kind, source, sideMods in ((num, f"on{i}", mods if side in ("numer", "both") else ()), (den, f"od{i}", mods if side in ("denom", "both") else ())):
        if sideMods:  # the input drawn alone with the modifiers
            exp = ModifierExpected(name_of(kind, variant, source))
            modifier_reference(alone(bases[(kind, variant)], kind, variant), sideMods, exp)
            expectedWarnings.extend((exp.name, w) for w in exp.warnings)
            expectedWarnings.extend(input_warnings(kind, variant, source, True))
    for kind, source in ((num, f"nr{i}"), (den, f"dr{i}")):
        caseWarnings.extend(input_warnings(kind, variant, source, False))
    numMods = mods if side in ("numer", "both") else ()
    denMods = mods if side in ("denom", "both") else ()
    numObj, numFunc = operand(num, variant, f"nr{i}", f"on{i}", numMods, caseWarnings)
    denObj, denFunc = operand(den, variant, f"dr{i}", f"od{i}", denMods, caseWarnings)
    ref, warns = division((num, numObj), (den, denObj), numFunc, denFunc, names, denMods, variant, caseWarnings, caseErrors)
    caseWarnings += warns
    base = drawn(plain[(case["pair"], variant)]) if side == "result" else None
    if side == "result" and base is None and isinstance(ref, dict) and not ref["x"]:
        base = ROOT.TGraph()  # a ratio without points is not drawn alone: its modifiers act on a graph without points
    if side == "result" and ref is not None and base is not None:
        exp = ModifierExpected(names[0])
        for found, text in (  # modifiers that are rarely meaningful on a ratio warn
            (any(m.startswith("rebin") for m in mods), "Rebinning ratio {} sums ratio values"),
            (any(m.startswith("cumulative") for m in mods), "Cumulative of ratio {} sums ratio values"),
            (any(m in ("divide_width", "density") for m in mods), "Dividing ratio {} by bin width is rarely meaningful"),
            (any(m.startswith("normalize") or m == "density" for m in mods), "Normalizing ratio {} to its integral is rarely meaningful"),
        ):
            if found:
                exp.warn(text.format(names[0]))
        modified = modifier_reference(base, mods, exp)
        if not (isinstance(ref, dict) and not ref["x"]):  # without points it stays without points
            ref = modified
        caseWarnings += [(exp.name, w) for w in exp.warnings]
        fam = family(base)
    else:
        fam = None
    obs = drawn(f"r{i}")
    if ref is None:
        expectedErrors += caseErrors + [f"Plot r{i} from group sides could not be created"]
        check(f"reported as not possible: {what}", obs is None, "the ratio was drawn")
        continue
    expectedWarnings += caseWarnings  # a failing ratio stops the plot in the frame pass, where warnings are muted
    if isinstance(ref, dict) and not ref["x"]:
        expectedWarnings.append((f"Graph {names[0]} ", "has no points: it is not drawn"))
        check(f"a ratio without points not drawn: {what}", obs is None, "drawn")
        continue
    if obs is None:
        check(f"ratio made: {what}", False, first_five([line for line in run.errors() if f"r{i} " in line or f"r{i}:" in line]))
        continue
    diff = graph_diff(ref, obs, 1e-9) if fam in ("graph", "graph2d") else compare(ref, obs)
    check(f"as plain ROOT: {what}", not diff, diff)

unmatched, unexpected = match_messages(run.warnings(), expectedWarnings, True)
check("every warning expected", not unexpected, first_five(unexpected))
check("every expected warning given", not unmatched, first_five(unmatched))
unmatched, unexpected = match_messages(run.errors(), expectedErrors, False)
check("every error expected", not unexpected, first_five(unexpected))
check("every expected error given", not unmatched, first_five(unmatched))

# a modifier value refused where it is set names the input of the side it is set on
for side, named, other in (("numer", "hnum", "hden"), ("denom", "hden", "hnum"), ("result", "hnum", "hden")):
    plot = S.Plot("refused", "refused", "1d")
    ratio = plot[1].AddRatio(["hnum", "s"], ["hden", "s"])
    _, printed = captured(lambda ratio=ratio, side=side: SIDES[side](ratio).RebinX(0))
    said = [line for line in printed.splitlines() if line.startswith("[ WARN")]
    check(f"a modifier refused on the {side} side names {named}", len(said) == 1 and named in said[0] and other not in said[0], printed.strip())
finish()
