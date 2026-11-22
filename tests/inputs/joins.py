"""Join gives the rows of the joined tree beside those of the tree, as computed from the raw arrays: row by row from the same data source
(every input of the tree holds its joined tree, in a folder relative to where the tree is), from another data source (its rows in the order
of its inputs, however they are split into files), by one or two integer keys from another data source (rows of the tree without a match
skipped and counted in the reading summary; the joined data source may be restricted to a folder, the joined tree named by a path), by
an index column holding the row number within the joined tree of the same input (AO2D-like DF_ folders: rows whose index is -1 or beyond
the joined tree skipped, also into a joined tree whose first column is an array with a size column, folders that are empty or have no
joined rows, read in parallel; an index column with a dot in its name
taken whole, a leaf of a leaflist branch; an index column of a tree joined before that keeps rows without a match: rows without a match
there skipped). Keeping rows without a match: alias.matched, the joined columns NaN, 0 or empty there (also Double32_t and strings; a
column of two joined trees by its bare name that of the first), every row without a match for a joined tree without rows.
Columns of the joined tree are reached as alias.column, or by their name where the tree has no column of that name (the tree's own column
wins), those of the tree also as <tree>.column; filters (also on C strings of the joined tree), defines, weights, entry ranges (counted over the rows of the tree, before rows without a match are skipped) and ratios
work on joined columns; two joins at once, and requests with other joins on the same tree, each give their own result. A join that is
not possible is reported with the reason and loses the plot: keys appearing more than once, keys that are no integers or reach 2^53,
row counts that differ (per input, or in total from another data source), a joined tree or key column missing (also in only one input),
a tree written by a multi-threaded Snapshot joined row by row, a tree in a list, a table or an RNTuple, an undefined data source, an
alias equal to the name of the tree, an index column missing or not integer, a joined tree without rows in any input (or in its data
source). A data source only joined has its inputs expanded once per run (its warnings said once); ROOT's warning about inputs whose tree
has no rows is not shown.
Covers: joins (row by row, by key, by an index column, inner join, shuffled tracks, entry ranges, ratios, the error cases).
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
# AO2D-like files: tracks refer to the collisions of their DF_ folder by row number (-1 or beyond the collisions: no collision),
# the first track of each folder to none (its joined tree is then unknown when the folder is opened); some folders empty or without
# collisions; the folders of each file are the inputs. Labels row by row with the tracks refer to the particles of their folder.
# Collisions and FT0-like rows carry the bunch crossing of their folder (fIndexBCs, restarting in every folder; some without FT0).
ao2d = [p.path("AO2D_1.root"), p.path("AO2D_2.root")]


def write_ft0(path, folder, bc, timeA, amplitudes):
    """an FT0-like tree as in AO2Ds: an array whose size is a column of its own (fAmplitudeA[fAmplitudeA_size])"""
    f = ROOT.TFile(path, "UPDATE")
    directory = f.GetDirectory(folder) or f.mkdir(folder)
    directory.cd()
    tree = ROOT.TTree("O2ft0", "")
    bcBuf, timeBuf, sizeBuf, ampBuf = np.zeros(1, np.int32), np.zeros(1, np.float32), np.zeros(1, np.int32), np.zeros(32, np.float32)
    tree.Branch("fIndexBCs", bcBuf, "fIndexBCs/I")
    tree.Branch("fTimeA", timeBuf, "fTimeA/F")
    tree.Branch("fAmplitudeA_size", sizeBuf, "fAmplitudeA_size/I")
    tree.Branch("fAmplitudeA", ampBuf, "fAmplitudeA[fAmplitudeA_size]/F")
    for b, t, a in zip(bc, timeA, amplitudes):
        bcBuf[0], timeBuf[0], sizeBuf[0] = b, t, len(a)
        ampBuf[: len(a)] = a
        tree.Fill()
    tree.Write()
    f.Close()


AO = {k: [] for k in ("pt", "idx", "posZ", "mult", "part", "vz", "bcVal", "bcTime")}
AOC = {"timeA": [], "sumA": []}  # per collision: the FT0 time and summed amplitudes of its bunch crossing (nan without)
for path, nColls in zip(ao2d, ((40, 0, 0, 25), (0, 60))):
    for d, nColl in enumerate(nColls):
        folder = f"DF_{d}"
        posZ, mult = rng.uniform(-10, 10, nColl), rng.integers(1, 100, nColl).astype(np.int32)
        nTrk = 0 if (path, d) == (ao2d[0], 1) else int(rng.integers(100, 400))
        idx = rng.integers(-1, nColl + 3, nTrk).astype(np.int32)
        idx[:1] = -1
        pt = rng.exponential(1.0, nTrk)
        nPart = int(rng.integers(20, 80))
        vz = rng.uniform(-5, 5, nPart)
        part = rng.integers(-1, nPart + 3, nTrk).astype(np.int32)
        collBC = rng.permutation(nColl + 5)[:nColl].astype(np.int32)
        ft0BC = rng.permutation(nColl + 5)[: max(nColl - 3, 0)].astype(np.int32)
        timeA = rng.uniform(-2, 2, len(ft0BC))
        write_tree(path, {"fPosZ": posZ, "fMult": mult, "fIndexBCs": collBC}, name="O2collision", folder=folder)
        amplitudes = [rng.uniform(0, 10, int(rng.integers(0, 6))).astype(np.float32) for _ in ft0BC]
        write_ft0(path, folder, ft0BC, timeA.astype(np.float32), amplitudes)
        dupBC = np.append(ft0BC, ft0BC[:1]) if (path, d) == (ao2d[1], 1) else ft0BC  # one key twice in one folder
        write_tree(path, {"fIndexBCs": dupBC, "fTimeA": rng.uniform(-2, 2, len(dupBC))}, name="O2dupft0", folder=folder)
        timeOf = dict(zip(ft0BC.tolist(), timeA.astype(np.float32).tolist()))
        sumOf = dict(zip(ft0BC.tolist(), (float(a.sum(dtype=np.float32)) for a in amplitudes)))
        AOC["timeA"].extend(timeOf.get(bc, np.nan) for bc in collBC.tolist())
        AOC["sumA"].extend(sumOf.get(bc, np.nan) for bc in collBC.tolist())
        write_tree(path, {"fIndexCollisions": idx, "fPt": pt}, name="O2track", folder=folder)
        write_tree(path, {"fIndexMcParticles": part}, name="O2mctracklabel", folder=folder)
        write_tree(path, {"fVz": vz}, name="O2mcparticle", folder=folder)
        bcVal = rng.uniform(0, 100, nColl + 5)  # a bunch crossing per row the collisions refer to (fIndexBCs)
        write_tree(path, {"fVal": bcVal}, name="O2bc", folder=folder)
        ok = (idx >= 0) & (idx < nColl)
        okPart = (part >= 0) & (part < nPart)
        AO["pt"].extend(pt.tolist())
        AO["idx"].extend(np.where(ok, idx, -1).tolist())
        AO["posZ"].extend(np.where(ok, posZ[np.clip(idx, 0, max(nColl - 1, 0))] if nColl else 0.0, np.nan).tolist())
        AO["mult"].extend(np.where(ok, mult[np.clip(idx, 0, max(nColl - 1, 0))] if nColl else 0, 0).tolist())
        AO["part"].extend(np.where(okPart, part, -1).tolist())
        AO["vz"].extend(np.where(okPart, vz[np.clip(part, 0, nPart - 1)], np.nan).tolist())
        AO["bcVal"].extend(np.where(ok, bcVal[collBC[np.clip(idx, 0, max(nColl - 1, 0))]] if nColl else 0.0, np.nan).tolist())
        # the FT0 row given by the bunch crossing of the track's collision taken as row number (beyond the FT0 rows: none)
        bcRow = collBC[np.clip(idx, 0, max(nColl - 1, 0))] if nColl else np.full(nTrk, -1)
        okBC = ok & (bcRow < len(ft0BC))
        AO["bcTime"].extend(np.where(okBC, timeA.astype(np.float32)[np.clip(bcRow, 0, max(len(ft0BC) - 1, 0))] if len(ft0BC) else 0.0, np.nan).tolist())
AO = {k: np.array(v) for k, v in AO.items()}
AOC = {k: np.array(v) for k, v in AOC.items()}
aoMatched = AO["idx"] >= 0
noColl = p.path("AO2D_none.root")
for d in range(2):
    write_tree(noColl, {"fPosZ": np.zeros(0)}, name="O2collision", folder=f"DF_{d}")
    write_tree(noColl, {"fIndexCollisions": np.zeros(5, dtype=np.int32), "fPt": np.ones(5)}, name="O2track", folder=f"DF_{d}")
# keys k of a tree joined to trees with some of them: the columns of the joined trees of types without a value of their own in rows without
# a match (Double32_t, strings), a column v in two joined trees, and a joined tree without rows
km = p.path("keys.root")
K = {"k": np.arange(12, dtype=np.int32)}
write_tree(km, K, name="main")
sideKeys, side2Keys = np.array([0, 2, 3, 5, 8, 9], dtype=np.int32), np.array([1, 2, 5, 6, 11], dtype=np.int32)
write_tree(km, {"k": side2Keys, "v": side2Keys * 10.0}, name="side2")
write_tree(km, {"k": np.zeros(0, dtype=np.int32), "v": np.zeros(0)}, name="emptySide")
kmFile = ROOT.TFile(km, "UPDATE")
side = ROOT.TTree("side", "")
sideK, sideD32, sideV, sideS = np.zeros(1, np.int32), np.zeros(1), np.zeros(1), ROOT.std.string()
side.Branch("k", sideK, "k/I")
side.Branch("d32", sideD32, "d32/d")  # Double32_t
side.Branch("v", sideV, "v/D")
side.Branch("s", sideS)
for key in sideKeys:
    sideK[0], sideD32[0], sideV[0] = key, key + 0.5, key * 100.0
    sideS.replace(0, sideS.size(), "text")
    side.Fill()
side.Write()
kmFile.Close()
inSide, inSide2 = np.isin(K["k"], sideKeys), np.isin(K["k"], side2Keys)
# an index column with a dot in its name (a branch of a split object, event.fIndex): taken whole, not as alias.column
dotted = ROOT.TFile(p.path("dotted.root"), "RECREATE")
dottedColl = ROOT.TTree("coll", "")
dottedZ = np.zeros(1, dtype=np.float32)
dottedColl.Branch("z", dottedZ, "z/F")
for i in range(5):
    dottedZ[0] = i
    dottedColl.Fill()
dottedTrk = ROOT.TTree("trk", "")
dottedIdx = np.zeros(1, dtype=np.int32)
dottedTrk.Branch("ev.idx", dottedIdx, "ev.idx/I")
for i in range(20):
    dottedIdx[0] = i % 5
    dottedTrk.Fill()
# an index column that is a leaf of a leaflist branch (ev.fIndex of a branch ev with leaves fIndex and other)
leafTrk = ROOT.TTree("trkLeaf", "")
leafIdx = np.zeros(2, dtype=np.int32)
leafTrk.Branch("ev", leafIdx, "fIndex/I:other/I")
for i in range(20):
    leafIdx[0], leafIdx[1] = (3 * i) % 5, 7
    leafTrk.Fill()
dotted.Write()
dotted.Close()
p.pm.AddDataSource("dotted", p.path("dotted.root"))
write_tree(p.path("emptyKeys.root"), {"run": np.zeros(0, dtype=np.int32), "event": np.zeros(0, dtype=np.int32)}, name="events")
p.pm.AddDataSource("emptyKeys", p.path("emptyKeys.root"))
p.pm.AddDataSource("evInfoMissing", [info, p.path("missing_*.root")])  # joined only, with an input matching no file
p.pm.AddDataSource("km", km)
p.pm.AddDataSource("kmOther", km)
for name, inputs in (("ao2d", [f"{path}:DF_*" for path in ao2d]), ("aoNone", f"{noColl}:DF_*")):
    p.pm.AddDataSource(name, inputs)
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
aoPt, aoZ, aoMult = AO["pt"][aoMatched], AO["posZ"][aoMatched], AO["mult"][aoMatched]
case("by an index column: a column of the joined tree in the same DF folder", H(20, -10, 10, aoZ), lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions").Project1D(("fPosZ", 20, [-10.0, 10.0])))
case("by an index column: rows without a row in the joined tree are skipped", H(30, 0, 6, aoPt), lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions").Project1D(("fPt", 30, [0.0, 6.0])))
case("by an index column: an expression of both trees, with an alias", H(30, 0, 300, aoPt * aoMult), lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions", "coll").Project1D(("fPt * coll.fMult", 30, [0.0, 300.0])))
case(
    "by an index column: the tree's own columns named <tree>.column",
    H(30, 0, 300, aoPt * aoMult),
    lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions", "coll").Project1D(("O2track.fPt * coll.fMult", 30, [0.0, 300.0])).Filter("O2track.fIndexCollisions >= 0"),
)
case("by an index column: a column with a dot in its name", H(5, -0.5, 4.5, [i % 5 for i in range(20)]), lambda pad: pad.AddData("trk", "dotted").JoinByIndex("coll", "ev.idx").Project1D(("z", 5, [-0.5, 4.5])))
case("by an index column: a filter on a joined column", H(30, 0, 6, aoPt[aoZ > 0]), lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions").Project1D(("fPt", 30, [0.0, 6.0])).Filter("fPosZ > 0"))
withFT0 = ~np.isnan(AOC["timeA"])
case("by a key of the same data source: matched within the joined tree of each input (keys restart per folder)", H(40, -2, 2, AOC["timeA"][withFT0]), lambda pad: pad.AddData("O2collision", "ao2d").Join("O2ft0", ["fIndexBCs"], "ft0").Project1D(("ft0.fTimeA", 40, [-2.0, 2.0])))
# keeping rows without a match: alias.matched, and the joined columns there NaN (floating point), 0 (integers) or empty (arrays)
case("keeping rows without a match by a key: alias.matched", H(2, -0.5, 1.5, withFT0.astype(float)), lambda pad: pad.AddData("O2collision", "ao2d").Join("O2ft0", ["fIndexBCs"], "ft0", True).Project1D(("ft0.matched", 2, [-0.5, 1.5])))
case(
    "keeping rows without a match by a key: an array with a size column, summed in the rows with a match",
    H(30, 0, 60, AOC["sumA"][withFT0]),
    lambda pad: pad.AddData("O2collision", "ao2d").Join("O2ft0", ["fIndexBCs"], "ft0", True).Project1D(("Sum(ft0.fAmplitudeA)", 30, [0.0, 60.0])).Filter("ft0.matched"),
)
case("keeping rows without a match by a key: the array empty in the rows without", H(30, 0, 60, np.zeros(int((~withFT0).sum()))), lambda pad: pad.AddData("O2collision", "ao2d").Join("O2ft0", ["fIndexBCs"], "ft0", True).Project1D(("Sum(ft0.fAmplitudeA)", 30, [0.0, 60.0])).Filter("!ft0.matched"))
case("keeping rows without a match by a key: a floating-point column by its bare name NaN in the rows without", H(2, -0.5, 1.5, (~withFT0).astype(float)), lambda pad: pad.AddData("O2collision", "ao2d").Join("O2ft0", ["fIndexBCs"], "ft0", True).Project1D(("std::isnan(fTimeA)", 2, [-0.5, 1.5])))
case("keeping rows without a match by an index column: alias.matched", H(2, -0.5, 1.5, aoMatched.astype(float)), lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions", "col", True).Project1D(("col.matched", 2, [-0.5, 1.5])))
case(
    "keeping rows without a match by an index column: an integer column 0 in the rows without",
    H(10, -0.5, 9.5, np.zeros(int((~aoMatched).sum()))),
    lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions", "col", True).Project1D(("col.fMult", 10, [-0.5, 9.5])).Filter("!col.matched"),
)
case("keeping rows without a match by keys of another data source: alias.matched", H(2, -0.5, 1.5, matched.astype(float)), lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"], "ev", True).Project1D(("ev.matched", 2, [-0.5, 1.5])))
case("keeping rows without a match by keys of another data source: a column 0 there", H(20, 0, 100, tCent), lambda pad: pad.AddData("tracks", "trk").Join(byKey, ["run", "event"], "ev", True).Project1D(("ev.matched ? ev.cent : 0.", 20, [0.0, 100.0])))
both = aoMatched & (AO["part"] >= 0)
for form in ("fIndexMcParticles", "O2mctracklabel.fIndexMcParticles"):
    case(
        f"by an index column of a tree joined row by row ({form}), together with a join by an index column of the tree",
        H(40, -15, 15, AO["vz"][both] - AO["posZ"][both]),
        lambda pad, form=form: pad.AddData("O2track", "ao2d").Join("O2mctracklabel").JoinByIndex("O2collision", "fIndexCollisions", "col").JoinByIndex("O2mcparticle", form, "mcp").Project1D(("mcp.fVz - col.fPosZ", 40, [-15.0, 15.0])),
    )
case(
    "by an index column of a tree joined before that keeps rows without a match: rows without a match there are skipped",
    H(20, 0, 100, AO["bcVal"][aoMatched]),
    lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions", "col", True).JoinByIndex("O2bc", "col.fIndexBCs", "bc").Project1D(("bc.fVal", 20, [0.0, 100.0])),
)
case("by an index column that is a leaf of a leaflist branch", H(5, -0.5, 4.5, [(3 * i) % 5 for i in range(20)]), lambda pad: pad.AddData("trkLeaf", "dotted").JoinByIndex("coll", "ev.fIndex").Project1D(("z", 5, [-0.5, 4.5])))
case("keeping rows without a match: a Double32_t column NaN in the rows without", H(2, -0.5, 1.5, (~inSide).astype(float)), lambda pad: pad.AddData("main", "km").Join("side", ["k"], "sd", True).Project1D(("std::isnan(sd.d32)", 2, [-0.5, 1.5])))
case("keeping rows without a match: a string column empty in the rows without", H(5, -0.5, 4.5, np.where(inSide, 4, 0)), lambda pad: pad.AddData("main", "km").Join("side", ["k"], "sd", True).Project1D(("sd.s.size()", 5, [-0.5, 4.5])))
case(
    "keeping rows without a match: a column of two joined trees by its bare name is that of the first, NaN in its rows without",
    H(2, -0.5, 1.5, (~inSide).astype(float)),
    lambda pad: pad.AddData("main", "km").Join("side", ["k"], "a", True).Join("side2", ["k"], "b", True).Project1D(("std::isnan(v)", 2, [-0.5, 1.5])),
)
case("keeping rows without a match: a column of two joined trees as alias.column of the second", H(2, -0.5, 1.5, (~inSide2).astype(float)), lambda pad: pad.AddData("main", "km").Join("side", ["k"], "a", True).Join("side2", ["k"], "b", True).Project1D(("std::isnan(b.v)", 2, [-0.5, 1.5])))
for what, joinedTree in (("of the same data source", "emptySide"), ("of another data source", ["emptySide", "kmOther"])):
    case(f"keeping rows without a match by a key: a joined tree without rows {what} leaves every row without a match", H(2, -0.5, 1.5, np.zeros(12)), lambda pad, joinedTree=joinedTree: pad.AddData("main", "km").Join(joinedTree, ["k"], "e", True).Project1D(("e.matched", 2, [-0.5, 1.5])))
    case(
        f"keeping rows without a match by a key: the columns of a joined tree without rows {what} NaN in every row",
        H(2, -0.5, 1.5, np.ones(12)),
        lambda pad, joinedTree=joinedTree: pad.AddData("main", "km").Join(joinedTree, ["k"], "e", True).Project1D(("std::isnan(v) && std::isnan(e.v)", 2, [-0.5, 1.5])),
    )
case(
    "keeping rows without a match by an index column: a joined tree without rows in any input leaves every row without a match",
    H(2, -0.5, 1.5, np.zeros(10)),
    lambda pad: pad.AddData("O2track", "aoNone").JoinByIndex("O2collision", "fIndexCollisions", "col", True).Project1D(("col.matched", 2, [-0.5, 1.5])),
)
withBC = ~np.isnan(AO["bcTime"])
case(
    "by an index column of a tree joined before into a tree whose first column is an array with a size column (FT0-like)",
    H(40, -2, 2, AO["bcTime"][withBC]),
    lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions", "col").JoinByIndex("O2ft0", "col.fIndexBCs", "ft").Project1D(("ft.fTimeA", 40, [-2.0, 2.0])),
)
first = aoMatched & (np.arange(len(AO["pt"])) < 500)
case("by an index column: the first entries of the tree, then rows without a match skipped", H(30, 0, 6, AO["pt"][first]), lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fIndexCollisions").Project1D(("fPt", 30, [0.0, 6.0])).Entries(500))

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
    "an index column missing": (lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fNope"), "O2track has no column fNope"),
    "an index column in neither the tree nor a tree joined before": (lambda pad: pad.AddData("O2track", "ao2d").Join("O2mctracklabel").JoinByIndex("O2mcparticle", "fNope"), "neither O2track nor a tree joined before has a column fNope"),
    "an index column of a tree joined after it": (lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2mcparticle", "fIndexMcParticles").Join("O2mctracklabel"), "O2track has no column fIndexMcParticles"),
    "an index column of an alias not joined": (lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2mcparticle", "lbl.fIndexMcParticles"), "index column lbl.fIndexMcParticles: lbl is neither O2track nor a tree joined before"),
    "a key appearing twice in the joined tree of one input": (lambda pad: pad.AddData("O2collision", "ao2d").Join("O2dupft0", ["fIndexBCs"]), f"appears in more than one row of {ao2d[1]}:DF_1/O2dupft0"),
    "an index column that is no integer": (lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2collision", "fPt"), "index column fPt of O2track is of type Double_t, but it must hold one integer per row"),
    "a tree joined by an index column without rows in any input": (lambda pad: pad.AddData("O2track", "aoNone").JoinByIndex("O2collision", "fIndexCollisions"), "it has no rows in any input of O2track"),
    "a joined tree missing beside a tree joined by an index column": (lambda pad: pad.AddData("O2track", "ao2d").JoinByIndex("O2nobc", "fIndexCollisions"), f"{ao2d[0]}:DF_0/O2nobc does not exist"),
    "a tree joined by keys from another data source without rows": (lambda pad: pad.AddData("tracks", "trk").Join(["events", "emptyKeys"], ["run", "event"], "ev"), "it has no rows in data source emptyKeys"),
    "an alias equal to the name of the tree": (lambda pad: pad.AddData("events", "A").Join(byKey, ["run", "event"]), "the columns of both would be called events.column, please specify an alias"),
}
for i, (what, (setup, _)) in enumerate(FAILING.items()):
    plot = S.Plot(f"f{i}", "failing", "1d")
    setup(plot[1]).Project1D(("x", 10, [0.0, 1.0]))
    p.pm.AddPlot(plot)
# a data source only joined, by two groups of requests: its inputs expanded once (the input matching no file said once)
for name, setup in (("tracks", lambda pad: pad.AddData("tracks", "trk").Join(["events", "evInfoMissing"], ["run", "event"]).Project1D(("cent", 10, [0.0, 100.0]))), ("items", lambda pad: pad.AddData("items", "gid").Join(["byGid", "evInfoMissing"], ["gid"]).Project1D(("val", 10, [0.0, 1.0])))):
    plot = S.Plot(name, "joinwarn", "1d")
    setup(plot[1])
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
check("the joins give no warnings or errors", not run.warnings() and not run.errors(), " | ".join(run.warnings() + run.errors()))
check("no ROOT warning about inputs whose tree has no rows", "Adding tree with no entries" not in run.log, " | ".join(line for line in run.log.splitlines() if "no entries" in line))
warnRun = p.plot("joinwarn")
said = [w for w in warnRun.warnings() if "No files match" in w]
check("a data source only joined, by two groups of requests: an input matching no file said once", len(said) == 1, " | ".join(said))

failed = p.plot("failing")
for i, (what, (_, reason)) in enumerate(FAILING.items()):
    lost = not failed.canvas(f"f{i}", "failing")
    given = [line for line in failed.errors() if reason in line]
    check(f"a join that is not possible is reported and loses the plot: {what}", lost and len(given) == 1, f"plot {'lost' if lost else 'made'}, reason given {len(given)} times: {given or failed.errors()[:3]}")
check("data of a failed join is reported missing", len(missing(failed)) >= 5, str(sorted(missing(failed))))
try:
    S.Data("events", "A").Join("scores", [], "", True)
    refused = ""
except Exception as e:  # noqa: BLE001
    refused = str(e)
check("keeping rows without a match is refused for a join row by row, where every row has its match", "every row has its match" in refused, refused or "not refused")
finish()
