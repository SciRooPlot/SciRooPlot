"""Data is found in every kind of container ROOT stores (directories, every collection class, folders, maps, stacks of histograms and
multigraphs), at any depth, in a file and handed over in memory: by its path, by the search rule (the folder of the data source first,
then its sub-folders one after the other in stored order, each completely; the first match counts), through a data source restricted to
a folder (src:folder) and through an input folder (file.root:folder), with every split of a path between the folder and the name giving
the same object. A name that is not there is reported missing, and its plot is not made. Every data kind in every container is drawn as
the same object read from the top level of a file. The rule is the one documented in the skill (Data sources).
Special containers: a map gives its values under the names of their keys, a clones array its objects, an object in two lists is found in
both, of several cycles of a key only the latest counts, an item of the same name as a list is found as data with the bare name and the
list's items through the path, a list written after a directory of its name hides the directory (reported once), keys of classes that are no TObject and data that is no named object are not drawn.
Covers: a stored THStack or TMultiGraph read like a list, also inside a TList and in memory, lists and directories
handed over in memory become folders.
"""

from srptest import DATA_KINDS, ROOT, Project, S, add_data, base_of, cases, check, collection, data_objects, describe, drawn_diff, finish, missing, own, put

LEAVES = ("THStack", "TMultiGraph")  # hold only histograms or graphs: the last level of a chain
COLLECTIONS = ("TList", "THashList", "TObjArray", "TOrdCollection", "TSortedList", "THashTable", "TFolder", "TMap")
INNER = ("dir", *COLLECTIONS)  # a directory only below a directory (or the file)
SUBS = ("a", "b")  # the names of the containers below the first level


def ident_object(kind, name, ident):
    """an object of a name that carries ident: a histogram with it in its first bin, a graph with it as its first value"""
    if kind == "TMultiGraph":
        graph = own(ROOT.TGraph(2))
        graph.SetPoint(0, 0.5, ident)
        graph.SetPoint(1, 1.5, ident)
        graph.SetName(name)
        return graph
    hist = own(ROOT.TH1D(name, "", 5, 0.0, 5.0))
    hist.SetBinContent(1, ident)
    return hist


def ident_of(obj):
    if obj.InheritsFrom("TH1"):
        return obj.GetBinContent(1)
    if obj.InheritsFrom("TGraph"):
        return obj.GetPointY(0)
    return None


def level_items(chain, i, k):
    """the objects of level k (1-based) of chain i in stored order; None where the container of the next level goes. Every level has
    u<k> (only there), dup (at every level), a decoy histogram and objects that are no data; the deepest also deep."""
    kind = chain[k - 1]
    base = 100 * i + 10 * k
    items = [ident_object(kind, f"x{k}", -1), ident_object(kind, f"u{k}", base + 1), ident_object(kind, "dup", base + 2)]
    if kind not in LEAVES:
        items += [own(ROOT.TObjString(f"s{k}")), own(ROOT.TNamed(f"n{k}", "title")), own(ROOT.TParameter("double")(f"p{k}", 1.0))]
    if k == len(chain):
        items.append(ident_object(kind, "deep", base + 3))
    else:
        items.insert(i % (len(items) + 1), None)
    return items


def build_collection(chain, i, k, name):
    container = collection(chain[k - 1], name)
    for item in level_items(chain, i, k):
        put(container, build_collection(chain, i, k + 1, SUBS[k - 1]) if item is None else item)
    return container


def write_level(directory, chain, i, k, name):
    """writes level k of chain i into a directory: a directory level as a sub-directory, a container as one key"""
    if chain[k - 1] != "dir":
        directory.cd()
        build_collection(chain, i, k, name).Write(name, ROOT.TObject.kSingleKey)
        return
    sub = directory.mkdir(name)
    for item in level_items(chain, i, k):
        if item is None:
            write_level(sub, chain, i, k + 1, SUBS[k - 1])
        else:
            sub.cd()
            item.Write(item.GetName())


def requests(chain, i, origin):
    """(plot suffix, [(data name, data source, expected ident)], extra sources) of chain i; ident None: reported missing"""
    depth = len(chain)
    top = f"c{i}"
    levels = [top, *SUBS[: depth - 1]]
    src = "S" if origin == "file" else f"M{i}"
    ident = lambda k, slot: 100 * i + 10 * k + slot  # noqa: E731
    path = lambda k, name: "/".join([*levels[:k], name])  # noqa: E731
    found = [(path(k, f"u{k}"), src, ident(k, 1)) for k in range(1, depth + 1)]
    found += [(path(k, "dup"), src, ident(k, 2)) for k in range(1, depth + 1)]
    found.append((path(depth, "deep"), src, ident(depth, 3)))
    out = [("paths", found, {})]
    # the search below a folder: unique names anywhere, dup of the folder itself; and every split of each path between folder and name
    split, extra = [], {}
    for k in range(1, depth + 1):
        for name, slot in ((f"u{k}", 1), ("dup", 2)):
            parts = [*levels[:k], name]
            for j in range(1, k + 1):
                folder, rest = "/".join(parts[:j]), "/".join(parts[j:])
                split.append((rest, f"{src}:{folder}", ident(k, slot)))  # restricted data source
                if name != "dup" or j == k:
                    split.append((name, f"{src}:{folder}", ident(k, slot)))  # searched from the folder
                if origin == "file":
                    entry = f"E{i}_{j}"
                    extra[entry] = folder
                    split.append((rest, entry, ident(k, slot)))  # input folder in the data source definition
                    if j >= 2:  # input folder and restriction combined
                        extra[f"E{i}_1"] = top
                        split.append((rest, f"E{i}_1:" + "/".join(parts[1:j]), ident(k, slot)))
        split.append(("deep", f"{src}:{top}", ident(depth, 3)))
    out.append(("split", split, extra))
    gone = [(f"{top}/nothere", src, None), (f"{top}/zz/nothere", src, None), ("nothere", f"{src}:{top}", None)]
    if depth >= 2:
        gone.append((f"{top}/a/nothere", src, None))
    out.append(("missing", gone, {}))
    return out


def reported_as(name, source):
    """how a missing name of a data source is reported (a restricted data source by its folder: S:folder/name)"""
    return f"{source}/{name}" if ":" in source else f"{source}:{name}"


def chain_of(case):
    return tuple(case[level] for level in ("l1", "l2", "l3") if case[level])


def valid_chain(case):
    chain = chain_of(case)
    if len(chain) != sum(bool(case[level]) for level in ("l1", "l2", "l3")) or (case["l2"] is None and case["l3"]):
        return False
    if any(kind in LEAVES for kind in chain[:-1]):
        return False
    return not any(chain[j] == "dir" and chain[j - 1] != "dir" for j in range(1, len(chain)))


p = Project("containers", bases=("1d", "2d"))
chains = cases(
    {"l1": [*INNER, *LEAVES], "l2": [None, *INNER, *LEAVES], "l3": [None, *INNER, *LEAVES], "origin": ["file", "memory"]},
    valid=valid_chain,
    always=lambda c: c["l2"] is None,  # every container directly
)
stored = ROOT.TFile(p.path("in.root"), "RECREATE")
memory = own(ROOT.TMemFile("memory.root", "RECREATE"))  # directories handed over in memory
expected = {}  # plot name: [(data name, data source, ident)]
for i, case in enumerate(chains):
    chain = chain_of(case)
    if case["origin"] == "file":
        write_level(stored, chain, i, 1, f"c{i}")
    elif chain[0] == "dir":
        write_level(memory, chain, i, 1, f"c{i}")
        p.pm.AddDataSource(f"M{i}", memory.GetDirectory(f"c{i}"))
    else:
        p.pm.AddDataSource(f"M{i}", build_collection(chain, i, 1, f"c{i}"))
    for suffix, items, extra in requests(chain, i, case["origin"]):
        for source, folder in extra.items():
            p.pm.AddDataSource(source, p.path("in.root") + ":" + folder)
        name = f"c{i}_{suffix}"
        plot = S.Plot(name, "chains", "1d")
        for dataName, source, _ in items:
            plot[1].AddData(dataName, source)
        p.pm.AddPlot(plot)
        expected[name] = (describe(case) + f" {suffix}", items)


def fits(kind, container):
    """whether data of a kind can be put into a container: a stack takes 1d and 2d histograms, a multigraph graphs"""
    cl = ROOT.TClass.GetClass(DATA_KINDS[kind][1])
    if container == "THStack":
        return cl.InheritsFrom("TH1") and DATA_KINDS[kind][0] < 3
    if container == "TMultiGraph":
        return cl.InheritsFrom("TGraph")
    return True


# every data kind in every container (and directly in memory), against the same object at the top level of the file
objects = data_objects()
places = cases(
    {"kind": [k for k in DATA_KINDS if "formula" not in DATA_KINDS[k][2]], "container": ["top", *INNER, *LEAVES], "origin": ["file", "memory"]},
    valid=lambda c: fits(c["kind"], c["container"]),
)
stored.cd()
for kind, obj in objects.items():
    obj.Write(kind)
for i, case in enumerate(places):
    kind, container = case["kind"], case["container"]
    obj = objects[kind].Clone(kind)
    own(obj)
    folder = f"k{i}"
    if container == "top":
        holder, name = obj, kind
    elif container == "dir":
        target = (stored if case["origin"] == "file" else memory).mkdir(folder)
        target.cd()
        obj.Write(kind)
        holder, name = target, f"{folder}/{kind}"
    else:
        holder = collection(container, folder)
        if container == "THStack":
            holder.Add(obj)
        else:
            put(holder, obj)
        name = f"{folder}/{kind}"
        if case["origin"] == "file":
            stored.cd()
            holder.Write(folder, ROOT.TObject.kSingleKey)
    source = "S"
    if case["origin"] == "memory":
        source = f"K{i}"
        p.pm.AddDataSource(source, holder)
    elif container == "top":
        continue  # the reference itself
    for how, dataName, dataSource in (("path", name, source), ("search", kind, source if container == "top" or case["origin"] == "memory" else f"S:{folder}")):
        if container == "top" and how == "search":
            continue
        plot = S.Plot(f"k{i}_{how}", "kinds", base_of(kind))
        data = plot[1].AddData(dataName, dataSource)
        if "projected" in DATA_KINDS[kind][2]:
            data.Project([0] if DATA_KINDS[kind][0] == 1 else [0, 1])
        p.pm.AddPlot(plot)
        expected[f"k{i}_{how}"] = (describe(case) + f" {how}", kind)
for kind in objects:
    plot = S.Plot(f"ref_{kind}", "kinds", base_of(kind))
    add_data(plot[1], kind, "S")
    p.pm.AddPlot(plot)


# special containers (written into the file, searched through the data source X, restricted to the folder of each)
def lst(name, *objs, kind="TList"):
    container = collection(kind, name)
    for obj in objs:
        put(container, obj)
    return container


special = {}  # name: [(data name, data source, ident)]
stored.cd()
mapped = own(ROOT.TMap())
put(mapped, ident_object("hist", "m1", 9001))
put(mapped, lst("sub", ident_object("hist", "m2", 9002)))
mapped.Write("map", ROOT.TObject.kSingleKey)
special["map: values under the names of their keys"] = [("map/m1", "S", 9001), ("map/sub/m2", "S", 9002), ("m2", "S:map", 9002)]
clones = own(ROOT.TClonesArray("TH1D", 3))
clones.SetName("clones")
for j in range(3):
    hist = clones.ConstructedAt(j)
    hist.SetName(f"ca{j}")
    hist.SetBins(5, 0.0, 5.0)
    hist.SetBinContent(1, 9010 + j)
clones.Write("clones", ROOT.TObject.kSingleKey)
lst("cloneslist", clones).Write("cloneslist", ROOT.TObject.kSingleKey)
special["clones array"] = [("clones/ca1", "S", 9011), ("ca2", "S:clones", 9012), ("cloneslist/clones/ca0", "S", 9010)]
shared = ident_object("hist", "hs", 9030)
lst("shared", lst("l1", shared), lst("l2", shared)).Write("shared", ROOT.TObject.kSingleKey)
special["an object in two lists"] = [("shared/l1/hs", "S", 9030), ("shared/l2/hs", "S", 9030), ("hs", "S:shared", 9030)]
lst("cycles", ident_object("hist", "hc", 9050), ident_object("hist", "old", 9051)).Write("cycles", ROOT.TObject.kSingleKey)
lst("cycles", ident_object("hist", "hc", 9052)).Write("cycles", ROOT.TObject.kSingleKey)
cycleDir = stored.mkdir("dcycles")
cycleDir.cd()
ident_object("hist", "hd", 9053).Write()
ident_object("hist", "hd", 9054).Write()
stored.cd()
special["latest cycle"] = [("cycles/hc", "S", 9052), ("dcycles/hd", "S", 9054), ("hc", "S:cycles", 9052)]
special["older cycle of a list not searched"] = [("cycles/old", "S", None)]
lst("collision", ident_object("hist", "a", 9060), lst("a", ident_object("hist", "x", 9061))).Write("collision", ROOT.TObject.kSingleKey)
special["data and list of the same name"] = [("collision/a", "S", 9060), ("collision/a/x", "S", 9061)]
lst("siblings", lst("first", ident_object("hist", "y", 9070)), lst("second", ident_object("hist", "y", 9071))).Write("siblings", ROOT.TObject.kSingleKey)
siblings = stored.mkdir("dsiblings")
for j, sub in enumerate(("first", "second")):
    siblings.mkdir(sub).cd()
    ident_object("hist", "y", 9072 + j).Write()
stored.cd()
p.pm.AddDataSource("siblings", lst("siblings", lst("first", ident_object("hist", "y", 9080)), lst("second", ident_object("hist", "y", 9081))))
special["sub-folders searched in stored order"] = [("y", "S:siblings", 9070), ("y", "S:dsiblings", 9072), ("y", "siblings:siblings", 9080)]
nonObject = stored.mkdir("nontobject")
nonObject.WriteObject(own(ROOT.std.vector("double")([1.0, 2.0])), "vec")
nonObject.cd()
ident_object("hist", "hn", 9045).Write()
stored.cd()
special["no TObject: not drawn"] = [("nontobject/vec", "S", None)]
special["beside a key that is no TObject"] = [("nontobject/hn", "S", 9045), ("hn", "S:nontobject", 9045)]
lst("strings", own(ROOT.TObjString("s"))).Write("strings", ROOT.TObject.kSingleKey)
special["no named object: reported as unsupported"] = [("strings/s", "S", "unsupported")]
mixedFolder = own(ROOT.TFolder("fold", ""))
mixedMap = own(ROOT.TMap())
mixedMap.SetName("map")
put(mixedMap, lst("map2", lst("lst", ident_object("hist", "hx", 9090))))
mixedFolder.Add(mixedMap)
mixed = stored.mkdir("mixed")
mixed.cd()
mixedFolder.Write()
stored.cd()
special["directory, folder, map, list"] = [("mixed/fold/map/map2/lst/hx", "S", 9090), ("hx", "S:mixed", 9090)]
hidden = stored.mkdir("hidden")  # a directory, then a list of its name (ROOT allows only this order): the list hides the directory
hidden.cd()
ident_object("hist", "h", 9100).Write()
ident_object("hist", "onlydir", 9102).Write()
stored.cd()
lst("hidden", ident_object("hist", "h", 9101)).Write("hidden", ROOT.TObject.kSingleKey)
special["a list hiding a directory of its name"] = [("hidden/h", "S", 9101), ("h", "S:hidden", 9101)]
special["a list hiding a directory of its name: the directory not read"] = [("hidden/onlydir", "S", None)]
lst("empty").Write("empty", ROOT.TObject.kSingleKey)
stored.mkdir("dempty")
special["empty containers"] = [("empty/x", "S", None), ("dempty/x", "S", None)]
for j, (what, items) in enumerate(special.items()):
    plot = S.Plot(f"s{j}", "special", "1d")
    for dataName, source, _ in items:
        plot[1].AddData(dataName, source)
    p.pm.AddPlot(plot)
    expected[f"s{j}"] = (f"special: {what}", items)
stored.Close()
p.pm.AddDataSource("S", p.path("in.root"))

run = p.plot()
reported = missing(run)
for name, (what, items) in expected.items():
    group = "chains" if name.startswith("c") else "kinds" if name.startswith("k") else "special"
    if isinstance(items, str):  # a data kind against its reference
        check(f"drawn as at the top level of a file: {what}", not drawn_diff(run.objects(f"ref_{items}", "kinds"), run.objects(name, group)), drawn_diff(run.objects(f"ref_{items}", "kinds"), run.objects(name, group)))
        continue
    wanted = [(n, s, i) for n, s, i in items if i is not None]
    gone = [reported_as(n, s) for n, s, i in items if i is None]
    drawn = run.objects(name, group)
    if wanted and wanted[0][2] == "unsupported":
        n, s, _ = wanted[0]
        check(f"not drawn, reported: {what}", not drawn and run.errors(f"{n} (data source {s}) is of unsupported type"), f"drawn {len(drawn)}")
        continue
    if gone:
        unreported = [g for g in gone if g not in reported]
        check(f"reported missing, plot not made: {what}", not drawn and not unreported, f"drawn {len(drawn)}, not reported {unreported}")
        continue
    if len(drawn) != len(wanted):
        check(f"found: {what}", False, f"{len(drawn)} of {len(wanted)} drawn; missing {[reported_as(n, s) for n, s, _ in wanted if reported_as(n, s) in reported]}")
        continue
    wrong = [f"{s}:{n} gives {ident_of(o)}, expected {i}" for o, (n, s, i) in zip(drawn, wanted) if ident_of(o) != i]
    check(f"found: {what}", not wrong, "; ".join(wrong[:4]))
hiding = run.warnings("hidden cannot be read")
check("a directory hidden by a list of its name is reported once", len(hiding) == 1 and "TList" in hiding[0], " | ".join(hiding))
errors = run.errors()
expectedErrors = ("cannot be created", "could not be created", "strings/s (data source S) is of unsupported type")
check("no errors but the missing data", all(any(e in line for e in expectedErrors) for line in errors), " | ".join(errors[:5]))
finish()
