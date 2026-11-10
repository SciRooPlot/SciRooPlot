"""Data is found by the search rule in random nests of containers whose names collide everywhere: directories, every collection class,
folders, maps and clones arrays up to depth 4, the same few names for data and containers on every level, key names other than the
names of the objects, older cycles of data and of containers, in a file. Every name is requested bare from the top of each nest and through random paths of container names.
Reference: an independent walk through the file with plain ROOT along the documented rule (a path takes the first container of each
name, of a key the latest cycle that is a container; a bare name is looked for among the data of the folder first, then in its containers one after the
other in stored order, each completely; the first match counts). Found data is drawn as the object found, any other name is reported
missing and its plot is not made.
Covers: a list of a class without dictionary (unknown to the plot program) is not entered, the data in it reported missing (ROOT says
once that it cannot read the class when it reads the list holding it).
"""

import random
import shutil

import srptest as S
from srptest import ROOT, check, finish

NAMES = ("a", "b", "c", "d", "e")  # the names of data and of containers alike
MEMORY = ("TList", "THashList", "TObjArray", "TOrdCollection", "TSortedList", "THashTable", "TFolder", "TMap", "TClonesArray")
seeds, nests = ((1, 2, 3, 4), 250) if S.FULL else ((1,), 80)

p = S.Project("nests")
idents = [0]


def hist(name):
    """a histogram of a name carrying a new ident in its first bin"""
    idents[0] += 1
    h = S.own(ROOT.TH1D(name, "", 2, 0.0, 2.0))
    h.SetBinContent(1, idents[0])
    return h


def collection(rng, kind, name, depth):
    """a collection of random data and containers"""
    if kind == "TClonesArray":
        clones = S.own(ROOT.TClonesArray("TH1D", 4))
        clones.SetName(name)
        for j in range(rng.randint(0, 4)):
            idents[0] += 1
            h = clones.ConstructedAt(j)
            h.SetName(rng.choice(NAMES))
            h.SetBins(2, 0.0, 2.0)
            h.SetBinContent(1, idents[0])
        return clones
    container = S.collection(kind, name)
    items = [hist(rng.choice(NAMES)) for _ in range(rng.randint(0, 4))]
    if depth < 4 and kind != "TSortedList":  # a sorted list sorts by name: only for named objects
        items += [collection(rng, rng.choice(MEMORY), rng.choice(NAMES), depth + 1) for _ in range(rng.randint(0, 3 if depth < 3 else 1))]
    rng.shuffle(items)
    if kind == "TMap":
        keys = set()
        for item in items:
            key = rng.choice(NAMES)
            if key not in keys:
                keys.add(key)
                container.Add(S.own(ROOT.TObjString(key)), item)
    else:
        for item in items:
            container.Add(item)
    return container


def directory(rng, parent, name, depth):
    """a directory of random data (key names may differ from the names of the objects, some with an older cycle) and containers"""
    d = parent.mkdir(name)
    entries = [("data", rng.choice(NAMES)) for _ in range(rng.randint(0, 4))]
    if depth < 4:
        entries += [("dir" if rng.random() < 0.4 else rng.choice(MEMORY), rng.choice(NAMES)) for _ in range(rng.randint(0, 3 if depth < 3 else 1))]
    rng.shuffle(entries)
    for kind, key in entries:
        d.cd()
        if kind == "data":
            hist(rng.choice(NAMES) if rng.random() < 0.3 else key).Write(key)
            if rng.random() < 0.2:
                hist(key).Write(key)  # a newer cycle
        elif kind == "dir":
            if not d.FindKey(key):  # ROOT makes no directory where a key of its name is
                directory(rng, d, key, depth + 1)
        else:
            if rng.random() < 0.2:
                collection(rng, rng.choice(MEMORY), key, depth + 1).Write(key, ROOT.TObject.kSingleKey)  # an older cycle
                d.cd()
            collection(rng, kind, key, depth + 1).Write(key, ROOT.TObject.kSingleKey)
    parent.cd()


# the reference: a walk with plain ROOT
def null(obj):
    return obj is None or ROOT.addressof(obj) == 0


def is_container(cl):
    return not null(cl) and any(cl.InheritsFrom(c) for c in ("TDirectory", "TFolder", "TCollection"))


def key_class(key):
    cl = ROOT.TClass.GetClass(key.GetClassName())
    return cl if not null(cl) and cl.IsLoaded() and cl.IsTObject() else None


def items(container):
    """(name, object) of an in-memory container in stored order (a map: its values under the names of their keys)"""
    if container.InheritsFrom("TFolder"):
        container = container.GetListOfFolders()
    found, it = [], ROOT.TIter(container)  # Python iteration of some collections (THashTable) yields nothing
    while not null(obj := it.Next()):
        if container.InheritsFrom("TMap"):
            if not null(value := container.GetValue(obj)):
                found.append((obj.GetName(), value))
        else:
            found.append((obj.GetName(), obj))
    return found


def read(key):
    return S.own(key.ReadObj())


def search(container, name):
    """the ident of the first data of a name: among the data of the container, then in its containers in stored order"""
    if container.InheritsFrom("TDirectory"):
        keys = list(container.GetListOfKeys())
        for key in keys:
            cl = key_class(key)
            if key.GetName() == name and cl is not None and not is_container(cl):
                return round(read(key).GetBinContent(1))
        seen = set()
        for key in keys:
            cl = key_class(key)
            if not is_container(cl) or key.GetName() in seen:  # the latest cycle of each name
                continue
            seen.add(key.GetName())
            sub = container.GetDirectory(f"{key.GetName()};{key.GetCycle()}") if cl.InheritsFrom("TDirectory") else read(key)
            if (found := search(sub, name)) is not None:
                return found
        return None
    members = items(container)
    for itemName, obj in members:
        if itemName == name and not is_container(obj.IsA()):
            return round(obj.GetBinContent(1))
    for _, obj in members:
        if is_container(obj.IsA()) and (found := search(obj, name)) is not None:
            return found
    return None


def resolve(container, name):
    """the container a path segment names: the latest cycle of a container of that name (data of the name does not hide it)"""
    if container.InheritsFrom("TDirectory"):
        for key in container.GetListOfKeys():  # the latest cycle first
            cl = key_class(key)
            if key.GetName() == name and is_container(cl):
                return container.GetDirectory(f"{name};{key.GetCycle()}") if cl.InheritsFrom("TDirectory") else read(key)
        return None
    return next((obj for itemName, obj in items(container) if itemName == name and is_container(obj.IsA())), None)


def expected(top, path, name):
    container = top
    for segment in [s for s in path.split("/") if s]:
        container = resolve(container, segment) if not null(container) else None
    return search(container, name) if not null(container) else None


# a list of a class the plot program has no dictionary for
unknown = shutil.which("c++") or shutil.which("g++")
if unknown:
    code = p.path("MyList.C")
    with open(code, "w") as f:
        f.write('#include <TList.h>\nclass MyList : public TList {\n public:\n  ClassDefOverride(MyList, 1)\n};\n')
    unknown = ROOT.gROOT.ProcessLine(f".L {code}+") == 0 and hasattr(ROOT, "MyList")
if not unknown:
    print("no C++ compiler: the list of a class without dictionary is left out")

cases = []  # (plot name, data name, data source, expected ident, what)
for seed in seeds:
    rng = random.Random(seed)
    path = p.path(f"nests{seed}.root")
    stored = ROOT.TFile(path, "RECREATE")
    tops = []
    for k in range(nests):
        top = f"r{k}"
        stored.cd()
        if rng.random() < 0.4:
            directory(rng, stored, top, 1)
        else:
            kind = rng.choice([m for m in MEMORY if m != "TClonesArray"])
            collection(rng, kind, top, 1).Write(top, ROOT.TObject.kSingleKey)
        tops.append(top)
    if unknown and seed == seeds[0]:
        stored.cd()
        mine = S.own(ROOT.MyList())
        mine.Add(hist("inside"))
        mine.Write("mine", ROOT.TObject.kSingleKey)
        outer = S.collection("TList", "outer")
        outer.Add(mine)
        outer.Add(hist("beside"))
        outer.Write("outer", ROOT.TObject.kSingleKey)
    stored.Close()
    stored = S.own(ROOT.TFile(path))
    for k, top in enumerate(tops):
        source = f"n{seed}_{k}"
        p.pm.AddDataSource(source, f"{path}:{top}")
        requests = [("", name) for name in (*NAMES, "zz")]
        requests += [("/".join(rng.choice(NAMES) for _ in range(rng.randint(1, 3))), rng.choice(NAMES)) for _ in range(6)]
        for j, (folder, name) in enumerate(requests):
            dataName = f"{folder}/{name}" if folder else name
            ident = expected(stored.Get(top) if not stored.GetDirectory(top) else stored.GetDirectory(top), folder, name)
            what = f"seed {seed} nest {k} ({stored.FindKey(top).GetClassName()}): {dataName or name}"
            cases.append((f"{source}_{j}", dataName, source, ident, what))
    if unknown and seed == seeds[0]:
        p.pm.AddDataSource("unknown", path)
        cases.append(("unknown_path", "outer/mine/inside", "unknown", None, "a list of a class without dictionary: path into it"))
        cases.append(("unknown_search", "inside", "unknown:outer", None, "a list of a class without dictionary: not searched"))
        cases.append(("unknown_beside", "beside", "unknown:outer", round(outer.At(1).GetBinContent(1)), "data beside a list of a class without dictionary"))
chunks = S.Chunks("nests")
for name, dataName, source, _, _ in cases:
    plot = S.S.Plot(name, chunks.add(name), "1d")
    plot[1].AddData(dataName, source)
    p.pm.AddPlot(plot)
chunks.plot(p)
needed = {chunks.groupOf[name] for name, _, _, ident, _ in cases if ident is not None}  # a run of only missing data makes no plot
unwritten = [line for line in chunks.unwritten() if line.split(":")[0] in needed]
check("every plot run with data to find writes its file", not unwritten, " | ".join(unwritten))
reported = set().union(*(S.missing(run) for run in chunks.runs.values()))
for name, dataName, source, ident, what in cases:
    drawn = chunks.objects(name)
    if ident is None:
        missingName = f"{source}/{dataName}" if ":" in source else f"{source}:{dataName}"
        check(f"reported missing, plot not made: {what}", not drawn and missingName in reported, f"drawn {[round(o.GetBinContent(1)) for o in drawn]}, reported {missingName in reported}")
    else:
        found = [round(o.GetBinContent(1)) for o in drawn]
        check(f"found as by the search rule: {what}", found == [ident], f"drawn {found}, expected {ident}")
errors = [line for line in chunks.errors() if not any(e in line for e in ("could not be created", "cannot be created", "emulated class (MyList)"))]
check("no errors but the missing data", not errors, " | ".join(errors[:5]))
finish()
