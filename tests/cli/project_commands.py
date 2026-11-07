"""Every srp command on a project does what it says, or fails with exit status 1 and one error naming the reason, leaving the
configuration folder exactly as it was: for an existing project, the selected one (@current), an unknown one, @current with none selected,
names that are no project names (.., a b, a/b) and an empty name; and for every command while projects.info cannot be read (the reason
is given once, the file is never written over). Removing, renaming and resetting change only what SciRooPlot wrote: program files stay, folders holding other files are
kept or refused; clean forgets the projects whose code is gone (the folder of the program, for a C++ project also the sources above its
build folder). Registering is safe against srp commands and programs running at the same time, and in-memory data moves with a renamed
project.
Covers: suite_cli #15, #18, #43 (unknown projects), f6e960e (confdir of an unknown project, unset program), b334843 (confdir ..),
07fb006 and e7af591 (add with an unset variable: one message, nothing left), 5b7c72f (program before srp add), f934095 (concurrent
srp add, SaveProject keeping changes made meanwhile, rename with in-memory data), 446f576 (srp set while a program saves), c4fc70a (rename
with the configuration reached through a symlink), 7d65935 and f49b65a (unreadable projects.info: exit 1, reported once)."""

import os
import shutil
import subprocess

from srptest import SRP, cases, check, config_files, config_folder, describe, finish, python, run

PROGRAMS = config_folder("programs")
counter = [0]


def program(name):
    path = os.path.join(PROGRAMS, name, "def.py")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "a").close()
    return path


def setup(state="normal"):
    """a configuration with projects A (selected, variable var=1) and B (var=1), each with a plots.info in its folder"""
    counter[0] += 1
    config = config_folder(f"config{counter[0]}")
    for name in ("B", "A"):
        run(SRP, "add", name, program(name), os.path.join(PROGRAMS, name, "out"), config=config)
        run(SRP, "set", name, "var", "1", config=config)
        os.makedirs(os.path.join(config, name), exist_ok=True)
        with open(os.path.join(config, name, "plots.info"), "w") as f:
            f.write("p\n{\n}\n")
    if state == "none":
        with open(os.path.join(config, "projects.info")) as f:
            text = f.read()
        with open(os.path.join(config, "projects.info"), "w") as f:
            f.write(text.replace("@current A", "@current \"\""))
    elif state == "unreadable":
        with open(os.path.join(config, "projects.info"), "a") as f:
            f.write("broken\n{\n")
    return config


def project_names(config):
    out = run(SRP, "projects", config=config)
    return sorted(line.split()[1] for line in out.stdout.splitlines() if line.startswith("Project "))


def get(config, project, prop):
    return run(SRP, "get", project, prop, config=config).stdout.strip()


# what each command does to project P (the one named, or A for @current); "fails" for a command that must fail on an existing project
COMMANDS = {
    "projects": (lambda P: ("projects", P), lambda c, P, out: f"Project {P}" in out.stdout and program(P) in out.stdout),
    "select": (lambda P: ("select", P), lambda c, P, out: f"@current {P}" in open(os.path.join(c, "projects.info")).read()),
    "remove": (lambda P: ("remove", P), lambda c, P, out: P not in project_names(c) and not os.path.exists(os.path.join(c, P)) and os.path.exists(program(P))),
    "rename": (lambda P: ("rename", P, "renamed"), lambda c, P, out: P not in project_names(c) and get(c, "renamed", "program") == program(P) and os.path.isfile(os.path.join(c, "renamed", "plots.info")) and not os.path.exists(os.path.join(c, P))),
    "get": (lambda P: ("get", P, "var"), lambda c, P, out: out.stdout.strip() == "1"),
    "set": (lambda P: ("set", P, "var", "2"), lambda c, P, out: get(c, P, "var") == "2"),
    "unset": (lambda P: ("unset", P, "var"), lambda c, P, out: get(c, P, "var") == "" and P in project_names(c)),
    "confdir": (lambda P: ("confdir", P), lambda c, P, out: out.stdout.strip() == os.path.join(c, P)),
    "add": (lambda P: ("add", P, program("C"), os.path.join(PROGRAMS, "C", "out")), "fails"),
    "unset_program": (lambda P: ("unset", P, "program"), "fails"),
}
TARGETS = {"existing": "B", "current": "@current", "unknown": "ghost", "none_selected": "@current", "dotdot": "..", "space": "a b", "slash": "a/b", "empty": ""}

todo = cases(
    {"command": list(COMMANDS), "target": list(TARGETS)},
    # projects and confdir without a project name are other commands (all projects, the configuration folder)
    valid=lambda c: not (c["target"] == "empty" and c["command"] in ("projects", "confdir")),
    always=lambda c: True,
    named=[
        {"command": "set", "target": "unknown", "why": "suite_cli #15: no project created"},
        {"command": "get", "target": "unknown", "why": "suite_cli #43"},
        {"command": "select", "target": "unknown", "why": "suite_cli #18"},
        {"command": "remove", "target": "unknown", "why": "suite_cli #18"},
        {"command": "confdir", "target": "unknown", "why": "f6e960e"},
        {"command": "confdir", "target": "dotdot", "why": "b334843"},
        {"command": "unset_program", "target": "existing", "why": "f6e960e"},
    ],
)
for case in todo:
    what = describe(case)
    config = setup("none" if case["target"] == "none_selected" else "normal")
    target = TARGETS[case["target"]]
    args, effect = COMMANDS[case["command"]]
    before = config_files(config)
    out = run(SRP, *args(target), config=config)
    succeeds = case["target"] in ("existing", "current") and effect != "fails"
    if case["command"] == "add" and case["target"] == "unknown":
        succeeds, effect = True, lambda c, P, out: get(c, P, "program") == program("C") and "@current ghost" in open(os.path.join(c, "projects.info")).read()
    if succeeds:
        P = "A" if target == "@current" else target
        check(f"does its job: {what}", out.rc == 0 and not out.errors() and effect(config, P, out), out.tail())
    else:
        # get and confdir of @current print nothing when no project is selected: env.sh asks them whether there is one
        quiet = case["target"] == "none_selected" and case["command"] in ("get", "confdir")
        nErrors = len(out.errors())
        check(
            f"fails with {'no' if quiet else 'one'} error and changes nothing: {what}",
            out.rc == 1 and nErrors == (0 if quiet else 1) and config_files(config) == before,
            f"rc {out.rc}, {nErrors} errors, changed {sorted(k for k in set(before) | set(config_files(config)) if before.get(k) != config_files(config).get(k))}: {out.tail()}",
        )

# an unreadable projects.info: every command reading or changing projects fails, reports it once and never writes the file
for args in (("projects",), ("projects", "A"), ("select", "A"), ("add", "C", program("C")), ("remove", "A"), ("rename", "A", "X"), ("set", "A", "v", "1"), ("unset", "A", "var"), ("reset",), ("clean",)):
    config = setup("unreadable")
    before = config_files(config)
    out = run(SRP, *args, config=config)
    lines = out.text.splitlines()
    reasons = [line for line in lines if "Cannot load projects file" in line and "unmatched" in line]
    check(
        f"unreadable projects.info: srp {' '.join(args)} fails, the reason given once, nothing written [7d65935, f49b65a]",
        out.rc == 1 and len(reasons) == 1 and len(set(lines)) == len(lines) and sum("could not be read" in line for line in lines) <= 1 and config_files(config) == before,
        f"rc {out.rc}: {out.tail()}",
    )

# srp add: paths are checked before anything is stored, an unset variable gives one message (07fb006, e7af591)
config = setup()
before = config_files(config)
for args, reason in (
    (("add", "C", program("C"), "${SRP_NO_SUCH_VAR}/out"), "SRP_NO_SUCH_VAR"),
    (("add", "C", "${SRP_NO_SUCH_VAR}/def.py"), "SRP_NO_SUCH_VAR"),
    (("add", "C", "relative/def.py"), "must not be relative"),
    (("add", "@current", program("C")), "@current"),
    (("add", "C"), "Specify program"),
):
    out = run(SRP, *args, config=config)
    check(f"srp {' '.join(args)}: one error naming {reason}, nothing stored [07fb006, e7af591]", out.rc == 1 and len(out.errors()) == 1 and reason in out.errors()[0] and config_files(config) == before, out.tail())
out = run(SRP, "add", "C", program("C"), "${HOME}/out", config=config)
check("srp add with a variable in the path stores it as written, expanded by get", out.rc == 0 and get(config, "C", "outdir") == os.path.join(os.environ["HOME"], "out") and "${HOME}/out" in open(os.path.join(config, "projects.info")).read(), out.tail())

# removing, renaming, resetting: only SciRooPlot's files
config = setup()
with open(os.path.join(config, "B", "notes.txt"), "w") as f:
    f.write("mine")
out = run(SRP, "rename", "B", "X", config=config)
check("rename refuses a project folder holding other files", out.rc == 1 and "notes.txt" in " ".join(out.errors()) and "B" in project_names(config), out.tail())
out = run(SRP, "remove", "B", config=config)
check("remove keeps a project folder holding other files (and warns), removes the project", out.rc == 0 and project_names(config) == ["A"] and os.path.isfile(os.path.join(config, "B", "notes.txt")) and not os.path.exists(os.path.join(config, "B", "plots.info")) and out.warnings(), out.tail())
config = setup()
out = run(SRP, "reset", config=config)
check("reset removes every project and its folder, programs stay", out.rc == 0 and project_names(config) == [] and not os.path.exists(os.path.join(config, "A")) and os.path.exists(program("A")), out.tail())
config = setup()
shutil.rmtree(os.path.join(PROGRAMS, "B"))
out = run(SRP, "clean", config=config)
check("clean removes the projects whose program folder is gone, and only those", out.rc == 0 and project_names(config) == ["A"], out.tail())
program("B")
# a C++ project whose build folder was deleted: its sources (the CMakeLists.txt above the build folder) are still there
config = setup()
built = os.path.join(PROGRAMS, "cpp", "build", "definePlots")
os.makedirs(os.path.dirname(built))
open(os.path.join(PROGRAMS, "cpp", "CMakeLists.txt"), "w").close()
run(SRP, "add", "cpp", built, config=config)
shutil.rmtree(os.path.dirname(built))
out = run(SRP, "clean", config=config)
check("clean keeps a C++ project whose build folder is gone while its sources are there", out.rc == 0 and project_names(config) == ["A", "B", "cpp"], out.tail())
os.remove(os.path.join(PROGRAMS, "cpp", "CMakeLists.txt"))
out = run(SRP, "clean", config=config)
check("clean removes it once its CMakeLists.txt is gone too", out.rc == 0 and project_names(config) == ["A", "B"], out.tail())

# concurrent registration (f934095)
config = config_folder("config_concurrent")
procs = [subprocess.Popen([SRP, "add", f"p{i}", program("A")], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=dict(os.environ, SCIROOPLOT_CONFIG_PATH=config)) for i in range(12)]
for proc in procs:
    proc.wait()
check("12 srp add at the same time: all registered [f934095]", project_names(config) == sorted(f"p{i}" for i in range(12)), f"{project_names(config)}")
# a program saving its project while srp commands change projects, properties and settings (f934095, 446f576)
config = setup()
out = python(
    f"""
import subprocess
from SciRooPlot import *
pm = PlotManager("A")
for args in (["add", "D", {program("D")!r}], ["set", "A", "user", "x"], ["select", "D"], ["verbosity", "warning"]):
    subprocess.run([{SRP!r}, *args], capture_output=True)
pm.SetOutputDirectory({os.path.join(PROGRAMS, "A", "out2")!r})
pm.SaveProject()
""",
    config=config,
)
settings = open(os.path.join(config, "settings.info")).read()
ok = project_names(config) == ["A", "B", "D"] and get(config, "A", "user") == "x" and get(config, "A", "outdir") == os.path.join(PROGRAMS, "A", "out2")
check("SaveProject keeps the projects, properties, selection and settings changed meanwhile [f934095, 446f576]", ok and "verbosity warning" in settings and "@current D" in open(os.path.join(config, "projects.info")).read(), out.tail() + str(project_names(config)))

# a program run before srp add saves its definitions, but registers nothing; srp add then takes them (5b7c72f)
config = config_folder("config_early")
out = python(
    """
from SciRooPlot import *
pm = PlotManager("early")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
plot = Plot("p", "g", "1d")
plot[1].AddFunction("x")
pm.AddPlot(plot)
pm.SaveProject()
""",
    config=config,
)
confdir = run(SRP, "confdir", "early", config=config)
check("a program before srp add registers nothing, its folder is known [5b7c72f]", project_names(config) == [] and confdir.rc == 0 and os.path.isfile(os.path.join(confdir.stdout.strip(), "plots.info")), out.tail() + confdir.tail())
run(SRP, "add", "early", program("early"), config=config)
listing = run(os.path.join(os.path.dirname(SRP), "plot"), "g", "p", "list", config=config)
check("srp add afterwards uses the saved definitions [5b7c72f]", listing.rc == 0 and " - p in group g" in listing.stdout, listing.tail())

# rename moves the in-memory data with the project, also when the program reached the configuration through a symlink (f934095, c4fc70a)
config = config_folder("config_memory")
link = os.path.join(os.path.dirname(config), "config_memory_link")
os.symlink(config, link)
run(SRP, "add", "mem", program("mem"), os.path.join(PROGRAMS, "mem", "out"), config=config)
out = python(
    """
import ROOT
from SciRooPlot import *
pm = PlotManager("mem")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
h = ROOT.TH1D("h", "", 4, 0, 4)
h.Fill(1)
pm.AddDataSource("memory", h)
plot = Plot("p", "g", "1d")
plot[1].AddData("h", "memory")
pm.AddPlot(plot)
pm.SaveProject()
""",
    config=link,
)
out = run(SRP, "rename", "mem", "moved", config=config)
sources = open(os.path.join(config, "moved", "dataSources.info")).read()
made = run(os.path.join(os.path.dirname(SRP), "plot"), "g", "p", "file", config=config)
check("rename: the in-memory data moves with the project [f934095, c4fc70a]", out.rc == 0 and os.path.join(config, "moved", "UserData.root") in sources and made.rc == 0 and not made.errors(), sources + made.tail())
finish()
