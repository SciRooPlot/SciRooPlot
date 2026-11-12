"""Mode data writes the input objects of the selected plots, as read, into Data.root of the output directory: each under its data source
and its path in it (a data source restricted to a folder, source:folder, and a path in a name become subdirectories), equal to the object
in the input file, for every data kind of srptest read from a file in every variant (every value at least once in the fast run); a
function of the plot definition under its data source, named by its formula. Keys are written in a fixed order: the same plots give the
same file. The plots themselves are not painted."""

from srptest import DATA_KINDS, ROOT, VARIANTS, Project, S, add_data, base_of, cases, check, data_objects, describe, finish, func_diff, graph_diff, has_variant, hist_diff, source_of
from srptest import formula as formula_of

p = Project("data_file")
p.inputs(VARIANTS)
INPUT = tuple(kind for kind, (_, _, props) in DATA_KINDS.items() if "formula" not in props)
todo = cases({"kind": INPUT, "variant": VARIANTS}, valid=lambda c: has_variant(c["kind"], c["variant"]))
for i, case in enumerate(todo):
    plot = S.Plot(f"d{i}", "data", base_of(case["kind"]))
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    p.pm.AddPlot(plot)
plot = S.Plot("formula", "data", "1d")
add_data(plot[1], "function")
p.pm.AddPlot(plot)
# a path in a name, and a data source restricted to a folder
f = ROOT.TFile(p.path("folders.root"), "RECREATE")
for folder in ("a", "a/b"):
    f.mkdir(folder, "", True).cd()
    h = ROOT.TH1D("h", "", 4, 0.0, 4.0)
    h.Fill(1.5, 2.0 if folder == "a" else 3.0)
    h.Write()
f.Close()
p.pm.AddDataSource("folders", p.path("folders.root"))
p.pm.AddDataSource("inner", p.path("folders.root") + ":a")
plot = S.Plot("paths", "data", "1d")
plot[1].AddData("a/b/h", "folders")
plot[1].AddData("b/h", "inner")
p.pm.AddPlot(plot)


def diff(ref, obs):
    """differences between an input object and the one written (functions compared at their sample points)"""
    if not obs:
        return "not written"
    if obs.ClassName() != ref.ClassName():
        return f"{obs.ClassName()}, expected {ref.ClassName()}"
    if ref.InheritsFrom("TEfficiency"):
        return hist_diff(ref.GetPassedHistogram(), obs.GetPassedHistogram()) or hist_diff(ref.GetTotalHistogram(), obs.GetTotalHistogram())
    if ref.InheritsFrom("THnBase"):
        dims = list(range(ref.GetNdimensions()))
        return hist_diff(ref.Projection(*dims) if len(dims) <= 3 else None, obs.Projection(*dims))
    if ref.InheritsFrom("TH1"):
        return hist_diff(ref, obs)
    if ref.InheritsFrom("TF1"):
        return func_diff(ref, obs)
    return graph_diff(ref, obs)


def keys(path):
    """every key of a file with its directory, in the order written"""
    found = []

    def walk(directory, prefix):
        for key in directory.GetListOfKeys():
            found.append(prefix + key.GetName())
            if key.IsFolder() and key.GetClassName().startswith("TDirectory"):
                walk(directory.Get(key.GetName()), prefix + key.GetName() + "/")

    f = ROOT.TFile.Open(path)
    walk(f, "")
    f.Close()
    return found


run = p.plot("data", ".+", "data")
check("mode data: no errors", not run.errors(), " | ".join(run.errors()[:3]))
check("mode data paints nothing: no plot reported as created", "Created plot" not in run.log, " | ".join(line for line in run.log.splitlines() if "Created plot" in line)[:300])
written = ROOT.TFile.Open(p.path("out", "Data.root"))
check("mode data: Data.root in the output directory", bool(written))
if written:
    inputs = {variant: data_objects(variant) for variant in VARIANTS}
    for case in todo:
        obj = written.Get(f"{source_of(case['variant'])}/{case['kind']}")
        problem = diff(inputs[case["variant"]][case["kind"]], obj)
        check(f"written as read: {describe(case)}", not problem, problem)
    contents = {path: written.Get(path) for path in ("folders/a/b/h", "inner/b/h")}
    check("a path in a name: subdirectories of the data source", bool(contents["folders/a/b/h"]) and contents["folders/a/b/h"].GetBinContent(2) == 3.0)
    check("a data source restricted to a folder: its folder and the path below it", bool(contents["inner/b/h"]) and contents["inner/b/h"].GetBinContent(2) == 3.0)
    names = keys(p.path("out", "Data.root"))
    formula = formula_of("function")
    # a key holding '/' is read through the key (TFile::Get takes it for a path)
    func = next((key.ReadObj() for source in names if "/" not in source for key in written.Get(source).GetListOfKeys() if key.GetName() == formula), None)
    problem = func_diff(ROOT.TF1("reference", formula, 1.0, 10.0), func) if func else "not written"
    check("a function of the plot definition: under its data source, named by its formula", not problem, problem)
    written.Close()
    first = names
    p.plot("data", ".+", "data")
    check("the same plots give the keys in the same order", keys(p.path("out", "Data.root")) == first)
finish()
