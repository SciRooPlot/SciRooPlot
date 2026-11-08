"""Every modifier on each side of a ratio: Numer(), Denom() and Both() act on the inputs before the division, Result() (the default) on the
ratio. The ratio is the division of the inputs as drawn alone with the same modifiers (data/modifiers.py holds those against plain ROOT),
done as data/ratios.py does it (spline of a denominator at other positions, errors, zeros left out); in Result mode the modifiers of the
plain ratio as for any data. Every input kind divided by itself, and pairs of a histogram, a graph and a function. Each warning names its
data (the modifiers of an input name that input; the denominator of a function numerator, prepared to sample it and to divide by,
reports once), and every warning is expected.
Covers: the ratio sides (both normalized, both rebinned, result scaled, numerator scaled), profiles
rebinned before the division.
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
    base_of,
    cases,
    check,
    data_objects,
    describe,
    division_name,
    division_reference,
    family,
    finish,
    first_five,
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
VARIANT = "normal"

p = Project("ratiosides")
p.inputs([VARIANT])
inputs = data_objects(VARIANT)
todo = cases(
    {"pair": PAIRS, "side": list(SIDES), "mods": [(m,) for m in MODIFIERS] + list(COMBINATIONS)},
    always=lambda c: c["pair"] == ("hist", "hist"),
    named=[
        {"pair": ("hist", "hist"), "side": "both", "mods": ("normalize",), "why": "both sides normalized"},
        {"pair": ("hist", "hist"), "side": "numer", "mods": ("rebin_x",), "why": "numerator rebinned: other binning, interpolated"},
        {"pair": ("profile", "profile"), "side": "both", "mods": ("rebin_x",), "why": "profiles divided as their means"},
        {"pair": ("tf1", "tf1"), "side": "denom", "mods": ("normalize",), "why": "the denominator of a function numerator reported once"},
    ],
)


def add_plot(name, kind, mods=(), source=None):
    """the data alone (with modifiers) in a plot of its own, read through a data source of its own"""
    p.pm.AddDataSource(source or name, p.path(f"{source_of(VARIANT)}.root"))
    plot = S.Plot(name, "sides", base_of(kind))
    data = plot[1].AddData(ratio_input(kind, source or name, VARIANT))
    for mod in mods:
        MODIFIERS[mod](data)
    p.pm.AddPlot(plot)


def add_ratio(name, pair, side=None, mods=()):
    for prefix in ("n", "d"):
        p.pm.AddDataSource(f"{prefix}{name}", p.path(f"{source_of(VARIANT)}.root"))
    plot = S.Plot(name, "sides", base_of(pair[0]))
    ratio = plot[1].AddRatio(ratio_input(pair[0], f"n{name}", VARIANT), ratio_input(pair[1], f"d{name}", VARIANT))
    if side:
        SIDES[side](ratio)
        for mod in mods:
            MODIFIERS[mod](ratio)
    p.pm.AddPlot(plot)


bases, plain = {}, {}  # the inputs drawn alone without modifiers, the plain ratio of each pair
for i, case in enumerate(todo):
    num, den = case["pair"]
    for kind in (num, den):
        if kind not in bases:
            bases[kind] = f"b{len(bases)}"
            add_plot(bases[kind], kind)
    if case["side"] in ("numer", "both"):
        add_plot(f"on{i}", num, case["mods"])
    if case["side"] in ("denom", "both"):
        add_plot(f"od{i}", den, case["mods"])
    if case["side"] == "result" and case["pair"] not in plain:
        plain[case["pair"]] = f"p{len(plain)}"
        add_ratio(plain[case["pair"]], case["pair"])
    add_ratio(f"r{i}", case["pair"], case["side"], case["mods"])
run = p.plot("sides")


def drawn(name):
    objs = run.drawn(name, "sides")
    return objs[0][0] if objs else None


def name_of(kind, source):
    """the name of the data (an input or a ratio) in messages of its modifiers"""
    return division_name(kind, VARIANT, source)


def operand(kind, source, alone, mods, warnings):
    """an input as divided: drawn alone (plot alone) with its modifiers, a function with its exact values; the warnings of its modifiers
    in the ratio (input read from source)"""
    base = drawn(bases[kind])
    exp = ModifierExpected(name_of(kind, source))
    ref = modifier_reference(base, mods, exp)
    warnings += [(exp.name, w) for w in exp.warnings]
    obj = drawn(alone) if mods else base
    if not DATA_KINDS[kind][1].startswith("TF"):
        return obj, None
    return obj, Func(kind, VARIANT, obj, ref if mods else None)


def division(num, den, numFunc, denFunc, names, denMods, warnings, errors):
    if den[1].InheritsFrom("TH1"):  # the axis of the denominator as modified (a rebin may leave out the last bins)
        denRange = (den[1].GetXaxis().GetXmin(), den[1].GetXaxis().GetXmax())
    else:  # a graph drawn alone has lost its x errors: those of the input
        denInput = inputs.get(den[0])
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


expectedWarnings, expectedErrors = [], []
for pair, name in plain.items():  # the plain ratios themselves
    numObj, denObj = drawn(bases[pair[0]]), drawn(bases[pair[1]])
    numFunc = Func(pair[0], VARIANT, numObj) if DATA_KINDS[pair[0]][1].startswith("TF") else None
    denFunc = Func(pair[1], VARIANT, denObj) if DATA_KINDS[pair[1]][1].startswith("TF") else None
    _, warns = division((pair[0], numObj), (pair[1], denObj), numFunc, denFunc, (name_of(pair[0], f"n{name}"), name_of(pair[1], f"d{name}")), (), [], expectedErrors)
    expectedWarnings += warns
for i, case in enumerate(todo):
    what = describe(case)
    (num, den), side, mods = case["pair"], case["side"], case["mods"]
    names = (name_of(num, f"nr{i}"), name_of(den, f"dr{i}"))
    caseWarnings, caseErrors = [], []
    for kind, source, sideMods in ((num, f"on{i}", mods if side in ("numer", "both") else ()), (den, f"od{i}", mods if side in ("denom", "both") else ())):
        if sideMods:  # the input drawn alone with the modifiers
            exp = ModifierExpected(name_of(kind, source))
            modifier_reference(drawn(bases[kind]), sideMods, exp)
            expectedWarnings.extend((exp.name, w) for w in exp.warnings)
    numMods = mods if side in ("numer", "both") else ()
    denMods = mods if side in ("denom", "both") else ()
    numObj, numFunc = operand(num, f"nr{i}", f"on{i}", numMods, caseWarnings)
    denObj, denFunc = operand(den, f"dr{i}", f"od{i}", denMods, caseWarnings)
    ref, warns = division((num, numObj), (den, denObj), numFunc, denFunc, names, denMods, caseWarnings, caseErrors)
    caseWarnings += warns
    if side == "result" and ref is not None:
        base = drawn(plain[case["pair"]])
        exp = ModifierExpected(names[0])
        for found, text in (  # modifiers that are rarely meaningful on a ratio warn
            (any(m.startswith("rebin") for m in mods), "Rebinning ratio {} sums ratio values"),
            (any(m.startswith("cumulative") for m in mods), "Cumulative of ratio {} sums ratio values"),
            (any(m in ("divide_width", "density") for m in mods), "Dividing ratio {} by bin width is rarely meaningful"),
            (any(m.startswith("normalize") or m == "density" for m in mods), "Normalizing ratio {} to its integral is rarely meaningful"),
        ):
            if found:
                exp.warn(text.format(names[0]))
        ref = modifier_reference(base, mods, exp)
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
        expectedWarnings.append((f"Graph {num} ", "has no points: it is not drawn"))
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
finish()
