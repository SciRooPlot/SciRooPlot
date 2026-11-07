"""Mode macro writes for every plot a ROOT macro that runs in ROOT and makes its canvas again: the macro compiles and runs without errors,
and the canvas it makes has the primitives of the canvas of mode file (Plots.root), with the same drawing options, in every sub-pad, and
the same legend rows (label and symbol), and the setup of every pad (palette, text format, kept axis ranges) runs before the pad is
painted; for every data kind of srptest in every variant (every value at least once in the fast run, the normal variant of every kind
always), for multi-panel plots of every kind (srptest's LAYOUTS: a grid with a colour-scale cell, ratio panels, a gap, a legend strip
with the rows of other pads, a flat panel beside a 3D view) and for a plot with an image. The macro is <output directory>/<group>/<name>.C,
named after the plot as a C++ identifier (ROOT names the macro's function after the file): other characters replaced by _, a leading
digit and names C++ or ROOT's interpreter know (keywords, types, C functions, main) prefixed or followed by _; names that give the same
file get a number, with a warning. Mode macro needs a display: without one it is refused with an error and writes nothing.
The macro is written by ROOT (TCanvas::SaveAs): what ROOT's SavePrimitive gets wrong is not SciRooPlot's to guarantee, and these data are
left out (ROOT_CANNOT): a TF3 of compiled code (ROOT writes its title as formula), a TGraphMultiErrors with an error of 0 (an ambiguous
SetPointEY call), every TProfile3D (its palette refers to a variable ROOT renamed), a TGraph2D of one point (pad ranges written with 6
digits give an illegal range) and an empty polar graph (the grid drawn alone is not written). Attributes the macro does not restore (ROOT
leaves out line colour 1, so black histograms come back in the style's colour; colour scales in the frame's contour levels) are not
compared: only what is drawn, with which option."""

import concurrent.futures
import os
import re
import subprocess

from srptest import DATA_KINDS, LAYOUT_BASES, LAYOUTS, VARIANTS, WORK, Project, ROOT, S, add_data, base_of, canvas_tree, cases, check, describe, display, finish, has_layout, has_variant, layout_plot, skip, source_of

if not display():
    skip("mode macro needs a display (DISPLAY, or Xvfb to start one)")
if not subprocess.run(["root", "-b", "-q", "-l", "-e", "0"], capture_output=True, check=False).returncode == 0:
    skip("the root executable is needed to run the macros")

# (kind, variant or None for every variant): macros ROOT cannot write
ROOT_CANNOT = {("tf3_cpp", None), ("graph_multi", "zeros"), ("profile3d", None), ("graph2d", "single"), ("graph_polar", "empty")}


def root_can(kind, variant):
    return (kind, None) not in ROOT_CANNOT and (kind, variant) not in ROOT_CANNOT


p = Project("macros", bases=("1d", "2d", *LAYOUT_BASES))
p.inputs(VARIANTS)
todo = cases(
    {"kind": tuple(DATA_KINDS), "variant": VARIANTS},
    valid=lambda c: has_variant(c["kind"], c["variant"]) and root_can(c["kind"], c["variant"]),
    always=lambda c: c["variant"] == "normal",
)
for i, case in enumerate(todo):
    plot = S.Plot(f"m{i}", "macros", base_of(case["kind"]))
    add_data(plot[1], case["kind"], source_of(case["variant"]), case["variant"])
    p.pm.AddPlot(plot)
# multi-panel plots; named: the plots of the first test of the macros of layouts (d29a380: legend strip, 1d grid, 3D views; maps below)
multi = cases(
    {"kind": tuple(DATA_KINDS), "layout": LAYOUTS},
    valid=lambda c: has_layout(c["kind"], c["layout"]) and root_can(c["kind"], "normal"),
    named=[
        *({"kind": kind, "layout": layout, "why": "d29a380"} for kind, layout in (("hist", "strip"), ("hist", "grid"), ("hist2d", "view"))),
        {"kind": "hist2d", "layout": "grid", "why": "the setup of a colour-scale cell runs before it in macros"},
    ],
)
for i, case in enumerate(multi):
    p.pm.AddPlot(layout_plot(f"l{i}", "macros", case["kind"], case["layout"]))
logo = p.path("logo.png")
c = ROOT.TCanvas("logo", "", 300, 100)
c.SetFillColor(ROOT.kAzure + 2)
c.SaveAs(logo)
plot = S.Plot("image", "macros", "1d")
add_data(plot[1], "hist")
plot[1].AddImage(0.3, 0.8, logo)
p.pm.AddPlot(plot)
# plot name -> macro file (without .C); the later of two plots giving the same file gets a number
NAMES = {
    "auto": "auto_",
    "int": "int_",
    "new": "new_",
    "autos": "autos",
    "1st": "_1st",
    "TH1": "TH1_",
    "vector": "vector_",
    "rand": "rand_",
    "main": "main_",
    "eff-pi": "eff_pi",
    "eff_pi": "eff_pi_2",
    "x-1": "x_1",
}
for name in NAMES:
    plot = S.Plot(name, "names/a-b", "1d")
    add_data(plot[1], "hist")
    p.pm.AddPlot(plot)

reference = p.plot()
run = p.plot("names/a-b", "auto", "macro", env={"DISPLAY": None})
check("mode macro without a display: an error, nothing written", not os.path.exists(p.path("out", "names")) and any("needs a graphical display" in line for line in run.lines("ERR")), run.lines("ERR")[:1])

DRIVER = os.path.join(WORK, "replay.C")
with open(DRIVER, "w") as f:
    f.write("""
void tree(TPad* pad, int depth) {
  for (auto link = pad->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
    TObject* obj = link->GetObject();
    printf("TREE|%d|%s|%s|%s\\n", depth, obj->ClassName(), obj->GetName(), link->GetOption());
    if (obj->InheritsFrom(TPad::Class())) tree(static_cast<TPad*>(obj), depth + 1);
    if (obj->InheritsFrom(TLegend::Class())) {
      for (auto* entry : *static_cast<TLegend*>(obj)->GetListOfPrimitives())
        printf("ROW|%s|%s|%s\\n", obj->GetName(), static_cast<TLegendEntry*>(entry)->GetLabel(), static_cast<TLegendEntry*>(entry)->GetOption());
    }
  }
}
void replay(const char* macro) {
  gROOT->Macro(macro);
  auto* canvas = static_cast<TCanvas*>(gROOT->GetListOfCanvases()->Last());
  if (!canvas) { printf("NO CANVAS\\n"); return; }
  tree(canvas, 0);
}
""")


def replay(path):
    """runs a macro in ROOT on the display, as users do: (the primitives of its canvas as by canvas_tree, the lines with errors)"""
    proc = subprocess.run(["root", "-l", "-q", f'{DRIVER}("{path}")'], capture_output=True, text=True, timeout=300, check=False)
    lines = (proc.stdout + proc.stderr).splitlines()
    tree = [(int(depth), cls, obj, opt) for depth, cls, obj, opt in (line[5:].split("|", 3) for line in lines if line.startswith("TREE|"))]
    rows = sorted(tuple(line[4:].split("|", 2)) for line in lines if line.startswith("ROW|"))
    errors = [line for line in lines if re.search(r"error:|Error in <|Fatal|\*\*\* Break", line)]
    if proc.returncode:
        errors.append(f"exit status {proc.returncode}")
    return tree, rows, errors


def legend_rows(pad):
    """the rows of every legend of a canvas or pad and its sub-pads: (legend, label, option)"""
    found = []
    for obj in pad.GetListOfPrimitives():
        if obj.InheritsFrom("TPad"):
            found += legend_rows(obj)
        elif obj.InheritsFrom("TLegend"):
            found += [(obj.GetName(), str(entry.GetLabel()), str(entry.GetOption())) for entry in obj.GetListOfPrimitives()]
    return found


def drawn(tree):
    """what a canvas draws, in any order (ROOT makes the grid of a polar graph again when it draws the graph): its primitives without the
    TExecs (see setups), images without name and option (a macro makes them under a name of its own)"""
    return sorted((depth, cls, "", "") if cls == "TASImage" else (depth, cls, name, opt) for depth, cls, name, opt in tree if cls != "TExec")


def pad_setups(tree):
    """the pads holding a setup (a TExec setup_pad_<n> among their own primitives, wherever it stands)"""
    found, pads = set(), []  # pads: the pads enclosing the current primitive, as (depth, name)
    for depth, cls, name, _ in tree:
        while pads and pads[-1][0] >= depth:
            pads.pop()
        if cls == "TExec" and name.startswith("setup_pad_") and pads and pads[-1][0] == depth - 1:
            found.add((pads[-1][1], name))
        if cls == "TPad":
            pads.append((depth, name))
    return found


def setups(tree):
    """the pads whose setup (the TExec setup_pad_<n> run before the pad is painted: palette, text format, kept axis ranges ...) runs before
    their first other primitive: in the pad itself, or in the pad holding it right before it"""
    found, previous = set(), None
    for i, (depth, cls, name, _) in enumerate(tree):
        if cls == "TPad" and previous and previous[1] == "TExec" and previous[2].startswith("setup_pad_") and previous[0] == depth:
            found.add((name, previous[2]))
        if cls == "TPad" and i + 1 < len(tree) and tree[i + 1][1] == "TExec" and tree[i + 1][2].startswith("setup_pad_") and tree[i + 1][0] == depth + 1:
            found.add((name, tree[i + 1][2]))
        previous = (depth, cls, name)
    return found


run = p.plot("macros|names/a-b", ".+", "macro")
check("mode macro: no errors", not run.errors(), " | ".join(run.errors()[:3]))
jobs = [(f"macro runs in ROOT and makes the canvas: {describe(case)}", p.path("out", "macros", f"m{i}.C"), f"m{i}", "macros") for i, case in enumerate(todo)]
jobs += [(f"macro runs in ROOT and makes the canvas: {describe(case)}", p.path("out", "macros", f"l{i}.C"), f"l{i}", "macros") for i, case in enumerate(multi)]
jobs.append(("macro runs in ROOT and makes the canvas: plot with an image", p.path("out", "macros", "image.C"), "image", "macros"))
for name, file in NAMES.items():
    jobs.append((f"plot {name!r}: macro {file}.C runs in ROOT and makes the canvas", p.path("out", "names", "a-b", f"{file}.C"), name, "names/a-b"))
wanted = {}  # the canvas of Plots.root: what it draws, the setups of its pads
for _, path, name, group in jobs:
    canvas = reference.canvas(name, group)
    if canvas:
        tree = canvas_tree(canvas)
        wanted[path] = (drawn(tree), pad_setups(tree), sorted(legend_rows(canvas)))
    del canvas
# the macros run in ROOT processes of their own, side by side
with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as pool:
    replayed = dict(zip((job[1] for job in jobs), pool.map(lambda job: replay(job[1]) if os.path.isfile(job[1]) else None, jobs)))
for title, path, _, _ in jobs:
    want, result = wanted.get(path), replayed[path]
    if result is None or want is None:
        check(title, False, "no macro written" if result is None else "no canvas in Plots.root")
        continue
    tree, rows, errors = result
    got = drawn(tree)
    missing, extra = [e for e in want[0] if e not in got], [e for e in got if e not in want[0]]
    diff = f"missing {missing[:2]}, not in Plots.root {extra[:2]}" if missing or extra else ""
    lost = sorted(want[1] - setups(tree))
    if not diff and lost:
        diff = f"the setup runs after the pad is painted: {lost[:2]}"
    if not diff and rows != want[2]:
        diff = f"legend rows {rows[:3]}, in Plots.root {want[2][:3]}"
    check(title, not errors and not diff, " | ".join(errors[:2]) or diff)
written = sorted(f[:-2] for f in os.listdir(p.path("out", "names", "a-b")) if f.endswith(".C")) if os.path.isdir(p.path("out", "names", "a-b")) else []
check("macros named after the plots as C++ identifiers", written == sorted(NAMES.values()), f"{written}")
collision = [line for line in run.warnings() if "eff_pi" in line and "eff_pi_2.C" in line]
check("a plot whose macro would replace another's: a number and a warning", len(collision) == 1, run.warnings()[:3])
finish()
