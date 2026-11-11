"""Join gives the rows of the joined tree beside those of the tree, as computed from the raw arrays: row by row from the same data source
(every input of the tree holds its joined tree, in a folder relative to where the tree is), from another data source (its rows in the order
of its inputs, however they are split into files), by one or two integer keys from another data source (rows of the tree without a match
skipped and counted in the reading summary; the joined data source may be restricted to a folder, the joined tree named by a path).
Columns of the joined tree are reached as alias.column, or by their name where the tree has no column of that name (the tree's own column
wins); filters (also on C strings of the joined tree), defines, weights, entry ranges (counted over the rows of the tree, before rows without a match are skipped) and ratios
work on joined columns; two joins at once, and requests with other joins on the same tree, each give their own result. A join that is
not possible is reported with the reason and loses the plot: keys appearing more than once, keys that are no integers or reach 2^53,
row counts that differ (per input, or in total from another data source), a joined tree or key column missing (also in only one input),
a tree written by a multi-threaded Snapshot joined row by row, a tree in a list, a table or an RNTuple, an undefined data source, an
alias equal to the name of the tree.
Covers: joins (row by row, by key, inner join, shuffled tracks, entry ranges, ratios, the error cases).
"""

import random

import numpy as np
from srptest import ROOT, CStrings, Project, S, check, drawn_diff, finish, missing, write_csv, write_tree

p = Project("joins")
rng = np.random.default_rng(1)

# events in two files, each with the trees scores, reco/events and truth/events beside them (row by row)
A = [p.path("A1.root"), p.path("A2.root")]
nEv = (300, 500)
E = {k: [] for k in ("run", "event", "x", "w", "score", "reco", "truth")}
for fi, (path, n) in enumerate(zip(A, nEv)):
    cols = {"run": np.full(n, 100 + fi, dtype=np.int32), "event": np.arange(n, dtype=np.int32), "x": rng.normal(size=n), "w": rng.uniform(0.5, 2.0, n), "score": rng.uniform(size=n)}
    cols["reco"] = rng.normal(size=n)
    cols["truth"] = cols["reco"] + rng.normal(scale=0.1, size=n)
    write_tree(path, {k: cols[k] for k in ("run", "event", "x", "w")}, name="events")
    write_tree(path, {"score": cols["score"], "label": CStrings(np.where(cols["score"] > 0.5, "high", "low"))}, name="scores")
    write_tree(path, {"x": cols["reco"]}, name="events", folder="reco")
    write_tree(path, {"x": cols["truth"]}, name="events", folder="truth")
    for k in E:
        E[k].extend(cols[k].tolist())
E = {k: np.array(v) for k, v in E.items()}
write_tree(A[0], {"y": rng.normal(size=nEv[0])}, name="onlyInA1")
NE = sum(nEv)
# scores of another data source, split differently over three files (row by row in the order of the inputs)
otherScores = rng.uniform(size=NE)
other = []
for i, (a, b) in enumerate(((0, 100), (100, 550), (550, NE))):
    other.append(p.path(f"S{i}.root"))
    write_tree(other[-1], {"score": otherScores}, name="scores", rows=slice(a, b))
# tracks: several per event, shuffled, some without event, in two files
T = {"run": [], "event": [], "pt": []}
for r, e in zip(E["run"], E["event"]):
    for _ in range(rng.integers(0, 4)):
        T["run"].append(int(r))
        T["event"].append(int(e))
        T["pt"].append(float(rng.exponential(1.0)))
for _ in range(37):
    T["run"].append(999)
    T["event"].append(int(rng.integers(0, 50)))
    T["pt"].append(float(rng.exponential(1.0)))
order = list(range(len(T["pt"])))
random.Random(3).shuffle(order)
T = {k: np.array([v[i] for i in order]) for k, v in T.items()}
NT = len(T["pt"])
tracks = [p.path("T0.root"), p.path("T1.root")]
for path, rows in zip(tracks, (slice(0, NT // 2), slice(NT // 2, None))):
    write_tree(path, {"run": T["run"].astype(np.int32), "event": T["event"].astype(np.int32), "pt": T["pt"]}, name="tracks", rows=rows)
# event information by key, and tables that cannot be joined by key
info = p.path("E.root")
cent = E["x"] * 10 + 50
keys = {"run": E["run"].astype(np.int32), "event": E["event"].astype(np.int32)}
write_tree(info, {**keys, "cent": cent}, name="events")
write_tree(info, {**keys, "cent": cent + 1000}, name="events", folder="sub")
gid = np.arange(NE, dtype=np.int64)
write_tree(info, {"gid": gid, "val": E["score"]}, name="byGid")
write_tree(info, {"run": keys["run"], "event": np.zeros(NE, dtype=np.int32), "cent": np.zeros(NE)}, name="dupKeys")
write_tree(info, {"run": E["run"].astype(np.float64), "event": E["event"].astype(np.float64), "cent": np.zeros(NE)}, name="floatKey")
write_tree(info, {"gid": gid + 2**53, "val": E["score"]}, name="bigKey")
items = p.path("G.root")
G = {"gid": rng.integers(0, NE + 20, 900).astype(np.int64), "q": rng.normal(size=900)}  # some beyond the events: without a match
write_tree(items, G, name="items")
# a tree written by a multi-threaded Snapshot (its entries reshuffled), with a tree beside it
mt = p.path("mt.root")
ROOT.EnableImplicitMT(2)
ROOT.RDataFrame(200000).Define("x", "double(rdfentry_)").Snapshot("events", mt)
ROOT.DisableImplicitMT()
options = ROOT.RDF.RSnapshotOptions()
options.fMode = "UPDATE"
ROOT.RDataFrame(200000).Define("score", "double(rdfentry_)").Snapshot("scores", mt, ["score"], options)
# a tree in a list, a table, an RNTuple
listFile = p.path("list.root")
f = ROOT.TFile(listFile, "RECREATE")
lst = ROOT.TList()
inList = ROOT.TTree("inlist", "")
v = np.zeros(1)
inList.Branch("v", v, "v/D")
for i in range(10):
    v[0] = i / 10
    inList.Fill()
lst.Add(inList)
lst.Write("lst", ROOT.TObject.kSingleKey)
f.Close()
write_csv(p.path("table.csv"), {"a": np.array([1, 3]), "b": np.array([2, 4])})
write_tree(p.path("rn.root"), {"x": E["x"][:10]}, name="events", ntuple=True)
write_tree(p.path("rn.root"), {"score": E["score"][:10]}, name="scores", ntuple=True)
for name, inputs in (("A", A), ("other", other), ("trk", tracks), ("evInfo", info), ("gid", items), ("mt", mt), ("tl", listFile), ("csv", p.path("table.csv")), ("rn", p.path("rn.root"))):
    p.pm.AddDataSource(name, inputs)


def H(nb, lo, hi, values, weights=None):
    h = ROOT.TH1D("ref", "", nb, lo, hi)
    h.Sumw2()
    values = np.asarray(values, dtype=np.float64)
    if len(values):
        h.FillN(len(values), np.ascontiguousarray(values), np.ascontiguousarray(np.ones(len(values)) if weights is None else np.asarray(weights, dtype=np.float64)))
    return h


expected = {}  # plot: (what, reference, compare contents only)


def case(what, ref, setup, base="1d", contents=False):
    plotName = f"j{len(expected)}"
    plot = S.Plot(plotName, "joins", base)
    data = setup(plot[1])
    if data is not None:
        data.SetShowOverflowBins()
    p.pm.AddPlot(plot)
    expected[plotName] = (what, ref, contents)


x, score, w = E["x"], E["score"], E["w"]
case("row by row: a column of the joined tree by its name", H(20, 0, 1, score), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("score", 20, [0.0, 1.0])))
case("row by row: a column as alias.column", H(20, 0, 1, score), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("scores.score", 20, [0.0, 1.0])))
case("row by row: an expression of both trees", H(30, -3, 3, x * score), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("x*scores.score", 30, [-3.0, 3.0])))
case("row by row: a tree in another folder, relative to the tree, with an alias", H(40, -0.5, 0.5, E["truth"] - E["reco"]), lambda pad: pad.AddData("reco/events", "A").Join("truth/events", [], "truth").Project1D(("truth.x - x", 40, [-0.5, 0.5])))
case("row by row from another data source: its rows in the order of its inputs", H(20, 0, 1, otherScores), lambda pad: pad.AddData("events", "A").Join(["scores", "other"]).Project1D(("score", 20, [0.0, 1.0])))
case("row by row from another data source: matched with the rows of the tree", H(20, -3, 3, x + otherScores), lambda pad: pad.AddData("events", "A").Join(["scores", "other"]).Project1D(("x + score", 20, [-3.0, 3.0])))
case("a weight and a filter of the tree", H(20, 0, 1, score[x > 0], w[x > 0]), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("score", 20, [0.0, 1.0]), "w").Filter("x > 0"))
case("a filter on a joined column", H(30, -3, 3, x[score > 0.5]), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("x", 30, [-3.0, 3.0])).Filter("scores.score > 0.5"))
case("a filter on a joined column of C strings", H(30, -3, 3, x[score > 0.5]), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("x", 30, [-3.0, 3.0])).Filter('scores.label == "high"'))
case("a define of a joined column", H(30, -3, 3, x * 2 * score), lambda pad: pad.AddData("events", "A").Join("scores").Define("y", "2*score").Project1D(("x*y", 30, [-3.0, 3.0])))
case("a weight of the joined tree", H(20, -3, 3, x, score), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("x", 20, [-3.0, 3.0]), "scores.score"))
case("the first entries", H(20, 0, 1, score[:123]), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("score", 20, [0.0, 1.0])).Entries(123))
case("an entry range across the files", H(20, 0, 1, score[250:420]), lambda pad: pad.AddData("events", "A").Join("scores").Project1D(("score", 20, [0.0, 1.0])).Entries(250, 420))
case("two joins", H(20, -2, 2, score + otherScores - x), lambda pad: pad.AddData("events", "A").Join("scores").Join(["scores", "other"], [], "o").Project1D(("scores.score + o.score - x", 20, [-2.0, 2.0])))
case("a column of both: the tree's own by its name", H(30, -3, 3, x), lambda pad: pad.AddData("events", "A").Join("reco/events", [], "reco").Project1D(("x", 30, [-3.0, 3.0])))
case("a column of both: the joined one as alias.column", H(30, -3, 3, E["reco"]), lambda pad: pad.AddData("events", "A").Join("reco/events", [], "reco").Project1D(("reco.x", 30, [-3.0, 3.0])))
case("the same tree without join beside joined requests", H(30, -3, 3, x), lambda pad: pad.AddData("events", "A").Project1D(("x", 30, [-3.0, 3.0])))
centByKey = {(r, e): c for r, e, c in zip(E["run"], E["event"], cent)}
matched = np.array([(int(r), int(e)) in centByKey for r, e in zip(T["run"], T["event"])])
tCent = np.array([centByKey.get((int(r), int(e)), 0.0) for r, e in zip(T["run"], T["event"])])
prof = ROOT.TProfile("ref", "", 10, 0.0, 100.0)
for c, pt in zip(tCent[matched], T["pt"][matched]):
    prof.Fill(c, pt)
byKey = ["events", "evInfo"]
case("by two keys: a profile of the joined column", prof, lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"]).Profile1D(("cent", 10, [0.0, 100.0]), "pt"))
case("by two keys: rows without a match are skipped", H(50, 0, 10, T["pt"][matched]), lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"]).Project1D(("pt", 50, [0.0, 10.0])))
first = matched & (np.arange(NT) < 200)
case("by two keys: the first entries of the tree, then rows without a match skipped", H(50, 0, 10, T["pt"][first]), lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"]).Project1D(("pt", 50, [0.0, 10.0])).Entries(200))
case("by two keys: with an alias", H(20, 0, 100, tCent[matched]), lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"], "ev").Project1D(("ev.cent", 20, [0.0, 100.0])))
case("by two keys: a filter on a joined column", H(50, 0, 10, T["pt"][matched & (tCent > 50)]), lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"]).Project1D(("pt", 50, [0.0, 10.0])).Filter("cent > 50"))
case("by two keys from a data source restricted to a folder", H(20, 1000, 1100, tCent[matched] + 1000), lambda pad: pad.AddData("tracks", "trk").Join(["events", "evInfo:sub"], ["run", "event"]).Project1D(("cent", 20, [1000.0, 1100.0])))
case("by two keys: the joined tree named by a path", H(20, 1000, 1100, tCent[matched] + 1000), lambda pad: pad.AddData("tracks", "trk").Join(["sub/events", "evInfo"], ["run", "event"]).Project1D(("cent", 20, [1000.0, 1100.0])))
inRange = G["gid"] < NE
case("by one key of 64 bits", H(20, 0, 1, score[G["gid"][inRange]]), lambda pad: pad.AddData("items", "gid").Join(["byGid", "evInfo"], ["gid"]).Project1D(("val", 20, [0.0, 1.0])))
h2 = ROOT.TH2D("ref", "", 10, 0.0, 1.0, 10, -3.0, 3.0)
h2.Sumw2()
for a, b in zip(score[G["gid"][inRange]], G["q"][inRange]):
    h2.Fill(a, b)
case("by one key: a 2d projection of both trees", h2, lambda pad: pad.AddData("items", "gid").Join(["byGid", "evInfo"], ["gid"]).Project2D(("val", 10, [0.0, 1.0]), ("q", 10, [-3.0, 3.0])), base="2d")
numer, denom = H(20, 0, 1, score[x > 0]), H(20, 0, 1, score)
ratio = numer.Clone("ratio")
ratio.Divide(numer, denom, 1, 1, "B")
case("a ratio joining both sides", ratio, lambda pad: pad.AddRatio(["events", "A"], ["events", "A"]).Both().Join("scores").Project1D(("score", 20, [0.0, 1.0])).Numer().Filter("x > 0").Both().SetCorrelated(), contents=True)
ratio = H(20, 0, 1, score)
ratio.Divide(H(20, 0, 1, otherScores))
case("a ratio joining the numerator only", ratio, lambda pad: pad.AddRatio(["events", "A"], ["scores", "other"]).Numer().Join("scores").Project1D(("score", 20, [0.0, 1.0])).Denom().Project1D(("score", 20, [0.0, 1.0])), contents=True)

FAILING = {  # what: (setup, the reason)
    "keys appearing more than once": (lambda pad: pad.AddData("tracks", "trk").Join(["dupKeys", "evInfo"], ["run", "event"]), "run = 100 and event = 0 appears in more than one row"),
    "keys that are no integers": (lambda pad: pad.AddData("tracks", "trk").Join(["floatKey", "evInfo"], ["run", "event"]), "key column run of floatKey is of type Double_t, but keys must be integers"),
    "keys reaching 2^53": (lambda pad: pad.AddData("items", "gid").Join(["bigKey", "evInfo"], ["gid"]), f"gid = {2**53} cannot be matched exactly, keys must be below 2^53"),
    "row counts that differ": (lambda pad: pad.AddData("tracks", "trk").Join(["events", "evInfo"]), f"it has {NE} rows, tracks has {NT}"),
    "row counts that differ in total from another data source": (lambda pad: pad.AddData("events", "A").Join(["items", "gid"]), f"it has 900 rows, events has {NE}"),
    "a joined tree missing": (lambda pad: pad.AddData("events", "A").Join("nope"), f"{A[0]}:nope does not exist"),
    "a joined tree missing in one input": (lambda pad: pad.AddData("events", "A").Join("onlyInA1"), f"{A[1]}:onlyInA1 does not exist"),
    "a key column missing": (lambda pad: pad.AddData("items", "gid").Join(byKey, ["gid"]), "events has no column gid"),
    "a reshuffled tree joined row by row": (lambda pad: pad.AddData("events", "mt").Join("scores"), "were reshuffled when it was written (multi-threaded Snapshot), it can only be joined by key"),
    "a tree in a list": (lambda pad: pad.AddData("lst/inlist", "tl").Join("lst/inlist", [], "b"), "Cannot join trees to tree tl:lst/inlist"),
    "a table": (lambda pad: pad.AddData("table", "csv").Join("table", [], "t2"), "Cannot join trees to table csv:table"),
    "an RNTuple": (lambda pad: pad.AddData("events", "rn").Join("scores"), "ROOT cannot join RNTuples"),
    "an undefined data source": (lambda pad: pad.AddData("events", "A").Join(["scores", "nosuchsource"]), "data source nosuchsource is not defined"),
    "an alias equal to the name of the tree": (lambda pad: pad.AddData("events", "A").Join(byKey, ["run", "event"]), "the columns of both would be called events.column, please specify an alias"),
}
for i, (what, (setup, _)) in enumerate(FAILING.items()):
    plot = S.Plot(f"f{i}", "failing", "1d")
    setup(plot[1]).Project1D(("x", 10, [0.0, 1.0]))
    p.pm.AddPlot(plot)

run = p.plot("joins")
for plotName, (what, ref, contents) in expected.items():
    obs = run.objects(plotName, "joins")
    if contents:
        diff = "not drawn" if not obs else next((f"content[{b}] {obs[0].GetBinContent(b):g}, expected {ref.GetBinContent(b):g}" for b in range(1, ref.GetNbinsX() + 1) if abs(obs[0].GetBinContent(b) - ref.GetBinContent(b)) > 1e-9 * max(1, abs(ref.GetBinContent(b)))), "")
    else:
        diff = drawn_diff([ref], obs)
    check(f"{what}: computed from the raw arrays", not diff, diff)
nUnmatched = int((~matched).sum())
check("rows without a match are counted in the reading summary", f"{nUnmatched} rows without a match skipped" in run.log, " | ".join(line for line in run.log.splitlines() if " - trk:" in line))
warned = [w for w in run.warnings() if "run into each other" not in w]  # the look of long index labels
check("the joins give no warnings or errors", not warned and not run.errors(), " | ".join(warned + run.errors()))

failed = p.plot("failing")
for i, (what, (_, reason)) in enumerate(FAILING.items()):
    lost = not failed.canvas(f"f{i}", "failing")
    given = [line for line in failed.errors() if reason in line]
    check(f"a join that is not possible is reported and loses the plot: {what}", lost and len(given) == 1, f"plot {'lost' if lost else 'made'}, reason given {len(given)} times: {given or failed.errors()[:3]}")
check("data of a failed join is reported missing", len(missing(failed)) >= 5, str(sorted(missing(failed))))
finish()
