"""Describes the example input files for the Data page of the site: what is in each file, which data source of the example
reaches it under which name, and which example plots use it.
Usage: extract_data.py <ExampleData folder> <config folder of the example project (dataSources.info, plots.info)> <out folder> > data.json
(the out folder gets one JSON file per object, which the page reads for its previews)"""
import csv
import json
import os
import re
import sys

import ROOT

ROOT.gErrorIgnoreLevel = ROOT.kError
DATA, CFG = sys.argv[1], sys.argv[2]


def info(obj):
    """A short description of an object, as a string, and the JSROOT draw option that suits it."""
    if obj.InheritsFrom("TEfficiency"):
        h = obj.GetTotalHistogram()
        return f"efficiency, {h.GetNbinsX()} bins", ""
    if obj.InheritsFrom("TH1"):
        dims = obj.GetDimension()
        axes = [obj.GetXaxis(), obj.GetYaxis(), obj.GetZaxis()][:dims]
        bins = " × ".join(str(a.GetNbins()) for a in axes)
        labelled = " (labelled bins)" if obj.GetXaxis().GetLabels() else ""
        kind = "profile" if obj.InheritsFrom("TProfile") or obj.InheritsFrom("TProfile2D") else f"{dims}D histogram"
        return f"{kind}, {bins} bins{labelled}, {obj.GetEntries():.0f} entries", "colz" if dims == 2 else ("" if dims == 1 else "box")
    if obj.InheritsFrom("TGraph2D"):
        return f"2D graph, {obj.GetN()} points", "colz"
    if obj.InheritsFrom("TGraph"):
        return f"graph, {obj.GetN()} points", "alp"
    if obj.InheritsFrom("TF1"):
        return f"function {obj.GetExpFormula()}", ""
    return obj.ClassName(), ""


OUT = sys.argv[3] if len(sys.argv) > 3 else None  # folder for one JSON file per object (ROOT's JSON format, read by JSROOT)
FILE = ""


def dump(obj, path, suffix=""):
    if not OUT:
        return
    target = f"{OUT}/{FILE}/{path}{suffix}.json"
    os.makedirs(os.path.dirname(target), exist_ok=True)
    open(target, "w").write(str(ROOT.TBufferJSON.ConvertToJSON(obj)))


def walk(container, path, is_list=False):
    nodes = []
    items = list(container) if is_list else [k for k in container.GetListOfKeys()]
    seen = set()
    for item in items:
        name = item.GetName()
        if name in seen:  # older cycles of a key
            continue
        seen.add(name)
        obj = item if is_list else item.ReadObj()
        p = f"{path}/{name}" if path else name
        node = {"n": name, "c": obj.ClassName(), "p": p}
        if obj.InheritsFrom("TDirectory"):
            node.update(k="dir", ch=walk(obj, p))
        elif obj.InheritsFrom("TCollection"):
            node.update(k="list", ch=walk(obj, p, True))
        elif obj.InheritsFrom("TTree"):
            branches = [[b.GetName(), b.GetLeaf(b.GetName()).GetTypeName() if b.GetLeaf(b.GetName()) else ""] for b in obj.GetListOfBranches()]
            node.update(k="tree", i=f"tree, {obj.GetEntries()} entries, {len(branches)} columns", br=branches)
            for b, _ in branches:  # a histogram of each column for the preview
                obj.Draw(f"{b}>>hPreview(60)", "", "goff")
                h = ROOT.gDirectory.Get("hPreview")
                h.SetTitle(f"{b} of {name}"); h.GetXaxis().SetTitle(b)
                dump(h, p, "@" + b)
                h.Delete()
        else:
            text, opt = info(obj)
            node.update(k="obj", i=text, o=opt)
            dump(obj, p)
            if obj.GetTitle():
                node["t"] = obj.GetTitle()
        nodes.append(node)
    return nodes


def read_info(path):
    """A minimal reader for the boost property-tree INFO files of SciRooPlot: returns nested dicts (repeated keys as lists)."""
    tokens = re.findall(r'"(?:[^"\\]|\\.)*"|[{}]|[^\s{}]+|\n', open(path).read())
    pos = 0

    def block():
        nonlocal pos
        out, key, value = [], None, None
        while pos < len(tokens):
            tok = tokens[pos]
            pos += 1
            if tok == "\n":
                if key is not None:
                    out.append((key, value))
                key, value = None, None
            elif tok == "{":
                if key is None and out and out[-1][1] is None:  # the key stood on the line before the brace
                    key = out.pop()[0]
                out.append((key, block()))
                key, value = None, None
            elif tok == "}":
                if key is not None:
                    out.append((key, value))
                return out
            elif key is None:
                key = tok.strip('"')
            else:
                value = tok.strip('"')
        return out
    return block()


# data sources of the example: name -> list of (file relative to the data folder, entry path inside it)
sources = {}
for name, entries in read_info(f"{CFG}/dataSources.info"):
    for key, value in entries:
        if key != "INPUT" or value is None:
            continue
        rel = value.split("ExampleData/", 1)[-1]
        file, _, entry = rel.partition(":")
        sources.setdefault(name, []).append((file, entry))

# objects used by the example plots: (data source, name) -> plots
uses = {}
for plot_key, plot in read_info(f"{CFG}/plots.info"):
    plot_name = plot_key.replace(":", " in ")
    name, group = plot_key.split(":", 1)
    plot_ref = group if group.startswith("animations/") else f"{group}/{name}"  # an animation is one example, not one per frame

    def scan(items):
        for key, value in items:
            if isinstance(value, list):
                d = dict((k, v) for k, v in value if not isinstance(v, list))
                for nk, sk in (("name", "dataSource"), ("denomName", "denomDataSource")):
                    if d.get(nk) and d.get(sk):
                        uses.setdefault((d[sk], d[nk]), set()).add(plot_ref)
                scan(value)
    scan(plot)

files = []
for fname in sorted(os.listdir(DATA)) + [f"jets/{f}" for f in sorted(os.listdir(f"{DATA}/jets"))]:
    full = f"{DATA}/{fname}"
    if os.path.isdir(full) or not fname.endswith((".root", ".csv", ".dat", ".txt", ".tsv", ".tab")):  # e.g. the logo image
        continue
    entry = {"f": fname, "size": os.path.getsize(full)}
    if fname.endswith(".root"):
        FILE = fname
        tf = ROOT.TFile.Open(full)
        entry.update(k="root", ch=walk(tf, ""))
        tf.Close()
    else:
        rows = list(csv.reader(open(full)))
        entry.update(k="table", head=rows[0], rows=rows[1:41], n=len(rows) - 1)
    files.append(entry)


def find(nodes, name):
    """SciRooPlot's search: a path goes straight there; a plain name is looked for at this level first, then in each folder completely."""
    if "/" in name:
        first, rest = name.split("/", 1)
        for n in nodes:
            if n["n"] == first and "ch" in n:
                return find(n["ch"], rest)
        return None
    for n in nodes:
        if n["n"] == name:
            return n
    for n in nodes:
        if "ch" in n:
            hit = find(n["ch"], name)
            if hit:
                return hit
    return None


def subtree(nodes, path):
    for part in [p for p in path.split("/") if p]:
        nodes = next((n["ch"] for n in nodes if n["n"] == part and "ch" in n), [])
    return nodes


# attach the data sources and the plots to the objects they reach
by_file = {f["f"]: f for f in files}
for (src, name), plots in uses.items():
    for file, entry in sources.get(src, []):
        f = by_file.get(file) or (by_file.get(f"{file}{name}.root") if file.endswith("/") else None)
        if f is None and file.endswith("/"):
            # a directory source: every file of it is an input
            candidates = [g for g in files if g["f"].startswith(file)]
        else:
            candidates = [f] if f else []
        for g in candidates:
            if g["k"] == "table":
                if os.path.splitext(os.path.basename(g["f"]))[0] == name:
                    g.setdefault("use", []).append({"s": src, "name": name, "plots": sorted(plots)})
                continue
            node = find(subtree(g["ch"], entry), name)
            if node:
                node.setdefault("use", []).append({"s": src, "name": name, "plots": sorted(plots)})
                break

json.dump({"sources": {k: [list(v) for v in vs] for k, vs in sources.items()}, "files": files}, sys.stdout, separators=(",", ":"), ensure_ascii=False)
