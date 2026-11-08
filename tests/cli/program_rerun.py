"""plot (the shell function of env.sh) runs the program of the current project before plotting exactly when its stored definitions are
older than the program, than a Python file next to it or one folder below (a module it imports), or, for a C++ program in a CMake build
folder, than the program rebuilt from changed sources; never when nothing changed or only other files did. It then plots what the program
saved. A failing program stops plot with a non-zero exit status before anything is plotted; a program that saved another project's
definitions is named in a warning; a missing program is named in a warning and the stored definitions are used. In bash and zsh.
Covers: a changed imported module, C++ program in a build folder other than build/, program of another
project, failing program."""

import os
import shutil
import time

from srptest import PREFIX, SHELLS, SRP, cases, check, config_folder, describe, finish, run, shell

PYTHON = """import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from SciRooPlot import *
from helper import LABEL
from sub.deeper import EXTRA
pm = PlotManager({project!r})
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
plot = Plot("p", "g", "1d")
plot[1].AddFunction("x", LABEL + EXTRA)
pm.AddPlot(plot)
pm.SaveProject()
"""
CHANGES = {
    "none": (None, False),
    "program": ("def.py", True),
    "module": ("helper.py", True),
    "module_below": ("sub/deeper.py", True),
    "data_file": ("notes.txt", False),
    "module_two_below": ("sub/subsub/unused.py", False),
}


def touch(folder, plotsInfo, changed=None):
    """dates every file of folder back, the stored definitions less far and a changed file least: mtimes are compared, not contents"""
    now = time.time()
    for root, _, names in os.walk(folder):
        for name in names:
            os.utime(os.path.join(root, name), (now - 30, now - 30))
    os.utime(plotsInfo, (now - 20, now - 20))
    if changed:
        os.utime(changed, (now - 10, now - 10))


def python_project(config, folder, project):
    os.makedirs(os.path.join(folder, "sub", "subsub"))
    files = {"def.py": PYTHON.format(project=project), "helper.py": 'LABEL = "first"\n', "sub/__init__.py": "", "sub/deeper.py": 'EXTRA = ""\n', "sub/subsub/unused.py": "", "notes.txt": "x\n"}
    for name, text in files.items():
        with open(os.path.join(folder, name), "w") as f:
            f.write(text)
    run(SRP, "add", project, os.path.join(folder, "def.py"), os.path.join(folder, "out"), config=config)


def plots_info(config, project):
    path = os.path.join(config, project, "plots.info")
    return (os.stat(path).st_mtime, open(path).read()) if os.path.exists(path) else (None, "")


todo = cases({"shell": list(SHELLS), "change": list(CHANGES)}, named=[{"shell": "bash", "change": "module", "why": "a changed module re-runs the program"}])
for i, case in enumerate(todo):
    what = describe(case)
    config = config_folder(f"config{i}")
    folder = os.path.normpath(os.path.join(config, "..", f"program{i}"))
    python_project(config, folder, "proj")
    first = shell("plot g p list", case["shell"], config=config)
    stamp, _ = plots_info(config, "proj")
    path, reruns = CHANGES[case["change"]]
    if path:
        with open(os.path.join(folder, path), "a") as f:
            f.write("LABEL = 'second'\n" if path == "helper.py" else "\n")
    touch(folder, os.path.join(config, "proj", "plots.info"), os.path.join(folder, path) if path else None)
    stamp = os.stat(os.path.join(config, "proj", "plots.info")).st_mtime
    out = shell("plot g p list", case["shell"], config=config)
    stamp2, text = plots_info(config, "proj")
    check(f"first plot runs the program: {what}", first.rc == 0 and stamp is not None and " - p in group g" in first.stdout, first.tail())
    check(f"program run {'again' if reruns else 'not again'}: {what}", (stamp2 != stamp) == reruns and out.rc == 0 and " - p in group g" in out.stdout and not out.warnings(), out.tail())
    if case["change"] == "module":
        check(f"plots what the re-run program saved: {what}", "second" in text, text[-200:])

for sh in SHELLS:
    config = config_folder(f"config_{sh}_failing")
    folder = os.path.normpath(os.path.join(config, "..", f"failing_{sh}"))
    python_project(config, folder, "proj")
    shell("plot g p list", sh, config=config)
    with open(os.path.join(folder, "def.py"), "a") as f:
        f.write("raise SystemExit(3)\n")
    out = shell("plot g p list; echo status $?", sh, config=config)
    check(f"{sh}: a failing program stops plot, non-zero exit status", "status 0" not in out.stdout and " - p in group g" not in out.stdout and not out.warnings(), out.tail())
    # a program that saves another project's definitions
    config = config_folder(f"config_{sh}_foreign")
    python_project(config, os.path.join(config, "..", f"own_{sh}"), "own")
    python_project(config, os.path.join(config, "..", f"other_{sh}"), "other")
    run(SRP, "select", "own", config=config)
    shell("plot g p list", sh, config=config)
    shutil.copy(os.path.join(config, "..", f"other_{sh}", "def.py"), os.path.join(config, "..", f"own_{sh}", "def.py"))
    touch(os.path.join(config, "..", f"own_{sh}"), os.path.join(config, "own", "plots.info"), os.path.join(config, "..", f"own_{sh}", "def.py"))
    out = shell("plot g p list", sh, config=config)
    check(f"{sh}: a program saving another project is named in a warning", out.rc == 0 and any("did not save the plot definitions of the current project" in w for w in out.warnings()), out.tail())
    # a missing program
    os.remove(os.path.join(config, "..", f"own_{sh}", "def.py"))
    out = shell("plot g p list", sh, config=config)
    check(f"{sh}: a missing program is named, the stored definitions are used", out.rc == 0 and " - p in group g" in out.stdout and any("does not exist" in w for w in out.warnings()), out.tail())

# C++: programs in a CMake build folder are rebuilt first, also in a folder not named build (#17)
if not (shutil.which("cmake") and (shutil.which("c++") or shutil.which("g++") or shutil.which("clang++"))):
    print("no cmake or C++ compiler: the C++ checks are left out")
else:
    config = config_folder("config_cpp")
    folder = os.path.normpath(os.path.join(config, "..", "cpp"))
    os.makedirs(folder)
    templates = os.path.join(PREFIX, "share", "scirooplot", "templates")
    with open(os.path.join(templates, "skeleton", "DefinePlots.cpp")) as f:
        source = f.read().replace("PROJECT_NAME", "cpp")
    os.makedirs(os.path.join(folder, "src"))
    with open(os.path.join(folder, "src", "DefinePlots.cpp"), "w") as f:
        f.write(source)
    with open(os.path.join(templates, "CMakeLists.txt")) as f:
        cmake = f.read().replace("PROJECT_NAME", "cpp").replace("DefinePlots.cpp", "src/DefinePlots.cpp")
    with open(os.path.join(folder, "CMakeLists.txt"), "w") as f:
        f.write(cmake)
    build = os.path.join(folder, "out-of-tree")
    made = shell(f"cmake -S {folder} -B {build} -Wno-dev >/dev/null && cmake --build {build}", config=config)
    run(SRP, "add", "cpp", os.path.join(build, "definePlots"), os.path.join(folder, "plots"), config=config)
    for sh in SHELLS:
        first = shell("plot '.+' '.+' list", sh, config=config)
        stamp, _ = plots_info(config, "cpp")
        check(f"{sh}: a C++ program in a build folder out-of-tree runs", made.rc == 0 and first.rc == 0 and stamp is not None and " - myFirstPlot" in first.stdout, made.tail() + first.tail())
        touch(folder, os.path.join(config, "cpp", "plots.info"))
        stamp = os.stat(os.path.join(config, "cpp", "plots.info")).st_mtime
        out = shell("plot '.+' '.+' list", sh, config=config)
        check(f"{sh}: an unchanged C++ program is not run again", out.rc == 0 and plots_info(config, "cpp")[0] == stamp and " - myFirstPlot" in out.stdout, out.tail())
        with open(os.path.join(folder, "src", "DefinePlots.cpp"), "a") as f:
            f.write(f"// edited for {sh}\n")
        touch(folder, os.path.join(config, "cpp", "plots.info"), os.path.join(folder, "src", "DefinePlots.cpp"))
        stamp = os.stat(os.path.join(config, "cpp", "plots.info")).st_mtime
        out = shell("plot '.+' '.+' list", sh, config=config)
        check(f"{sh}: a changed C++ source is rebuilt and run", out.rc == 0 and plots_info(config, "cpp")[0] != stamp and " - myFirstPlot" in out.stdout and not out.warnings(), out.tail())
finish()
