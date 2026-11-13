"""Data is drawn through the points it was given: a graph with points in no order of x (a contour, a trajectory) is drawn in their given
order by every alias that draws it as a graph, with the values of the input; only area sorts them (the area below the points needs the
order of x), with one warning; drawn as a histogram (hist, text, ...) or its steps (hist_clipped) a graph becomes bins, whose order is that of x. A function keeps
the number of points it is sampled at (SetNpx, SetNpy, SetNpz), few or many, for every alias it can be drawn with. Points given in the
plot definition (AddPoints) are named in messages by their label, or by their number and x range.
Covers: area sorted the points of a graph silently, points of the definition named by all their coordinates (warnings of kilobytes).
"""

from srptest import ALIASES, DATA_KINDS, Project, S, add_data, alias_table, base_of, cases, check, close, data_objects, describe, drawn_type, finish, has_variant, message_name, source_of

TABLE = alias_table()
GRAPHS = tuple(k for k in DATA_KINDS if drawn_type(k) == "graph" and has_variant(k, "unsorted"))
FUNCTIONS = tuple(k for k in DATA_KINDS if drawn_type(k).startswith("function") and has_variant(k, "coarse"))


def draws_as_graph(alias):
    """whether an alias draws a graph through its points (not as bins, nor as the steps of bins)"""
    return TABLE[alias]["graph"] not in ("-", "hist1d") and alias not in ("hist_clipped", "hist_open_clipped")


def available(kind, alias):
    return alias is None or TABLE[alias][drawn_type(kind)] not in ("-", "graph", "hist1d")


def valid(case):
    """unsorted graphs (sorted ones too for area) drawn as graphs, functions sampled coarsely or finely with the aliases they can draw"""
    if not has_variant(case["kind"], case["variant"]):
        return False
    if case["kind"] in GRAPHS:
        return (case["alias"] is None or draws_as_graph(case["alias"])) and (case["variant"] == "unsorted" or case["alias"] == "area")
    return case["variant"] != "normal" and available(case["kind"], case["alias"])


todo = cases(
    {"kind": list(GRAPHS + FUNCTIONS), "variant": ["unsorted", "normal", "coarse", "fine"], "alias": [None, *ALIASES]},
    valid=valid,
    always=lambda c: c["kind"] in ("graph", "tf1") or c["alias"] == "area",
)
p = Project("given_points")
p.inputs(["normal", "unsorted", "coarse", "fine"])
inputs = {variant: data_objects(variant) for variant in ("normal", "unsorted", "coarse", "fine")}
for i, case in enumerate(todo):
    plot = S.Plot(f"c{i}", "given", base_of(case["kind"]))
    data = add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    if case["alias"]:
        data.SetDrawOption(getattr(S, case["alias"]))
    p.pm.AddPlot(plot)
run = p.plot("given")


def points(graph):
    return [(graph.GetPointX(i), graph.GetPointY(i)) for i in range(graph.GetN())]


def same(a, b):
    return len(a) == len(b) and all(close(x1, x2) and close(y1, y2) for (x1, y1), (x2, y2) in zip(a, b))


def no_errors(obj):
    return obj.InheritsFrom("TGraph") and not any(obj.GetErrorX(i) > 0 or obj.GetErrorY(i) > 0 for i in range(obj.GetN()))


sortWarnings = run.warnings("are not in the order of x")
for i, case in enumerate(todo):
    what = describe(case)
    drawn = run.drawn(f"c{i}", "given")
    if not drawn:
        check(f"plot made: {what}", False, " | ".join(run.errors()[:2]))
        continue
    obj, _ = drawn[0]
    given = inputs[case["variant"]][case["kind"]]
    if case["kind"] in FUNCTIONS:
        sampling = [(obj.GetNpx(), given.GetNpx())]
        if DATA_KINDS[case["kind"]][0] > 1:
            sampling.append((obj.GetNpy(), given.GetNpy()))
        if DATA_KINDS[case["kind"]][0] > 2:
            sampling.append((obj.GetNpz(), given.GetNpz()))
        check(f"function keeps its sampling: {what}", all(a == b for a, b in sampling), f"drawn, given: {sampling}")
        continue
    if not obj.InheritsFrom("TGraph"):
        check(f"drawn as a graph: {what}", False, obj.ClassName())
        continue
    if case["alias"] == "area":
        check(f"area drawn through the points sorted by x: {what}", same(points(obj), sorted(points(given))), f"drawn {points(obj)[:5]}")
    else:
        check(f"drawn through the points as given: {what}", same(points(obj), points(given)), f"drawn {[(round(x, 3), round(y, 3)) for x, y in points(obj)[:6]]}")
for kind in GRAPHS:  # the warnings name the data, not the plot
    found = [w for w in sortWarnings if f"graph {message_name(kind, 'unsorted')} " in w]
    unsorted = sum(c["kind"] == kind and c["alias"] == "area" and c["variant"] == "unsorted" for c in todo)
    check(f"one warning about the order per area of unsorted points: {kind}", len(found) == unsorted, " | ".join(found))
# error bands, boxes and brackets of a graph without errors would draw nothing: it is drawn as without a drawing option, with a warning
errorWarnings = run.warnings("has no errors to draw with drawing option")
withoutErrors = sum(c["alias"] in ("band", "band_smooth", "boxes_nomarkers", "brackets") and no_errors(inputs[c["variant"]][c["kind"]]) for c in todo)
check("one warning per graph without errors drawn with an option that draws only errors", len(errorWarnings) == withoutErrors, " | ".join(errorWarnings))
check("no other warnings", len(run.warnings()) == len(sortWarnings) + len(errorWarnings), " | ".join(w for w in run.warnings() if w not in sortWarnings + errorWarnings)[:500])
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))

named = Project("given_points_named")
xs = [0.5 * i for i in range(400)]
ys = [1.0 + 0.01 * i for i in range(400)]
ys[7] = float("nan")
plot = S.Plot("named", "given", "1d")
plot[1].AddPoints(xs, ys, "rising")
plot[1].AddPoints(xs, ys)
named.pm.AddPlot(plot)
namedRun = named.plot("given")
said = sorted(w.split("]", 1)[-1].strip().split(" has ")[0] for w in namedRun.warnings("NaN or infinite"))
check("points of the definition named by their label, or their number and x range", said == sorted(["points 'rising'", "points (400 at x = 0 to 199.5)"]), str(said)[:300])
finish()
