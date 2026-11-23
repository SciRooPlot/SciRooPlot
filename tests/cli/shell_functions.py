"""The shell functions of env.sh, in bash and zsh: the script must be sourced; sourced, plot and srp work also in scripts. At the prompt,
plot gets its patterns unexpanded (.* is not a list of dot files), also after && and inside braces after ||, keeps its exit status, and
file globbing works again afterwards, also after plot in a pipeline. srp makes relative program and output paths absolute (add, set),
keeps other values and paths starting with ~ or a variable as written; srp cd goes to the program folder; srp edit opens the sources of
the program (a C++ program: of its CMake source folder, else of its own folder), and fails with a message without any; srp cd and srp out of an
unknown project, srp out without an output folder or before it exists fail with a message; srp update of a build configured for
another prefix fails naming srp reinstall; plot shows why srp cannot give the program of the current project. The srp program alone refuses the
commands that need env.sh, unknown commands, a missing command and too many arguments, with exit status 1. In zsh without a terminal, env.sh registers the
completion silently also with an insecure directory in fpath. srp info names the version, the installation of the env.sh sourced, the
configuration folder, the current project and the ROOT version.
Covers: the bash plot alias in compound commands (pre-release braces rule), 2026-11-03 (zsh read-only names in env.sh)."""

import os
import re
import shutil

from srptest import ENV_SCRIPT, ROOT, SHELLS, SOURCE, SRP, check, config_folder, finish, run, shell

config = config_folder("config")
folder = config_folder("cwd")
for name in ("a.txt", "b.txt", ".hidden"):
    open(os.path.join(folder, name), "w").close()
program = os.path.join(folder, "prog", "def.py")
os.makedirs(os.path.dirname(program))
with open(program, "w") as f:
    f.write("""from SciRooPlot import *
pm = PlotManager("proj")
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
for name in ("p", "q"):
    plot = Plot(name, "g", "1d")
    plot[1].AddFunction("x")
    pm.AddPlot(plot)
pm.SaveProject()
""")
run(SRP, "add", "proj", program, config=config)

for sh in SHELLS:
    out = run(sh, ENV_SCRIPT)
    check(f"{sh}: env.sh run instead of sourced fails with a message", out.rc != 0 and "must be sourced" in out.text, out.tail())
    out = shell("plot g '.+' list; echo status $?; srp help >/dev/null; echo status $?", sh, config=config)
    check(f"{sh}: sourced in a script, plot and srp work", out.stdout.count("status 0") == 2 and " - p from group g" in out.stdout, out.tail())

    # typed at the prompt
    typed = """cd {folder}
plot .* .* list
echo glob *.txt
true && plot g .* list
false || {{ plot .* q list; }}
plot nosuch .* list; echo status $?
plot g p list | cat
echo glob *.txt
""".format(folder=folder)
    out = shell(typed, sh, config=config, typed=True)
    lines = out.stdout.splitlines()
    plotted = [line for line in lines if line.startswith(" - ")]
    globs = [line for line in lines if line.startswith("glob ")]
    check(f"{sh}: plot .* .* at the prompt gets the patterns unexpanded", plotted[:2] == [" - p from group g", " - q from group g"], out.tail())
    check(f"{sh}: plot after && and in braces after || gets the patterns unexpanded", plotted[2:5] == [" - p from group g", " - q from group g", " - q from group g"], f"{plotted}")
    check(f"{sh}: plot keeps its exit status", "status 1" in lines, out.tail())
    check(f"{sh}: globbing works after plot, also after plot in a pipeline", globs == ["glob a.txt b.txt"] * 2, f"{globs}")
    check(f"{sh}: no errors at the prompt", not [line for line in out.stderr.splitlines() if "rror" in line or "not found" in line or "read-only" in line], out.stderr[-300:])

    # relative paths of srp add and srp set
    cfg = config_folder(f"config_paths_{sh}")
    os.makedirs(os.path.join(cfg, "rel"))
    with open(os.path.join(cfg, "rel", "settings.info"), "w") as f:  # as saved by a program declaring two text settings
        f.write('var\n{\n    type text\n    default ""\n}\nnote\n{\n    type text\n    default ""\n}\n')
    out = shell(
        """srp add rel prog/def.py out >/dev/null
srp get rel program; srp get rel outdir
srp set rel program other/def.py; srp set rel outdir out2; srp set rel var some/thing; srp set rel note '~/x'
srp get rel program; srp get rel outdir; srp get rel var
grep note "$SCIROOPLOT_CONFIG_PATH/projects.info" """,
        sh,
        config=cfg,
        cwd=folder,
    )
    want = [f"{folder}/prog/def.py", f"{folder}/out", f"{folder}/other/def.py", f"{folder}/out2", "some/thing"]
    check(f"{sh}: srp add and set make relative program and output paths absolute, nothing else", out.stdout.splitlines()[:5] == want and "~/x" in out.stdout, out.tail())

    # srp cd and srp out
    out = shell("srp cd proj && pwd; srp cd nosuch; echo status $?; srp out proj; echo status $?", sh, config=config, cwd=folder)
    lines = out.stdout.splitlines()
    check(f"{sh}: srp cd goes to the program folder", lines[:1] == [os.path.dirname(program)], out.tail())
    said = [line for line in out.stderr.splitlines() if line.strip()]
    check(f"{sh}: srp cd of an unknown project fails with one message", "status 1" in lines[1:2] and len(said) == 2 and "Cannot find project nosuch" in said[0], out.tail())
    check(f"{sh}: srp out without an output folder fails with a message", lines[2:3] == ["status 1"] and "has no output folder" in out.stderr, out.tail())
    out = shell("srp update junk; echo status $?; srp cd; echo status $?", sh, config=config_folder(f"no_project_{sh}"), cwd=folder)
    said = [line for line in out.stderr.splitlines() if line.strip()]
    check(f"{sh}: srp update with an unknown argument is refused, nothing run", out.stdout.splitlines()[:1] == ["status 1"] and "takes no arguments" in out.stderr and "install.sh" not in out.stderr, out.tail())
    check(f"{sh}: srp cd without a selected project: one message saying so", out.stdout.splitlines()[1:2] == ["status 1"] and len(said) == 2 and "No project selected" in said[1], out.tail())
    # srp update of a source installation whose build directory is configured for another prefix: the installer's advice works from srp
    fake = config_folder(f"fake_source_{sh}")
    os.makedirs(os.path.join(fake, "scripts"))
    os.makedirs(os.path.join(fake, "build"))
    shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts", "install.sh"), os.path.join(fake, "scripts", "install.sh"))
    with open(os.path.join(fake, "CMakeLists.txt"), "w") as f:
        f.write("project(SciRooPlot)\n")
    with open(os.path.join(fake, "build", "CMakeCache.txt"), "w") as f:
        f.write("CMAKE_INSTALL_PREFIX:PATH=/other/prefix\n")
    os.makedirs(os.path.join(fake, "prefix"))
    out = shell(f"SCIROOPLOT_SOURCE_DIR={fake}; SCIROOPLOT_ROOT={fake}/prefix; srp update; echo status $?", sh, config=config_folder(f"fake_update_{sh}"), cwd=folder)
    check(f"{sh}: srp update into another prefix than configured: fails, naming srp reinstall", "status 1" in out.stdout and "srp reinstall" in out.text, out.tail())
    # plot of a project whose program path names a variable not set in this shell: srp's reason is shown, not only plot's
    cfg = config_folder(f"unset_variable_{sh}")
    open(os.path.join(folder, "var_def.py"), "w").close()
    out = shell(f"SRPVAR={folder}; export SRPVAR; srp add varproj '$SRPVAR/var_def.py' out >/dev/null; srp select varproj >/dev/null; unset SRPVAR; plot g p print", sh, config=cfg, cwd=folder)
    check(f"{sh}: plot of a program path with a variable not set: the reason is shown", "SRPVAR" in out.stderr and "is not set" in out.stderr, out.tail())
    run(SRP, "set", "proj", "outdir", os.path.join(folder, "notyet"), config=config)
    out = shell("srp out proj; echo status $?", sh, config=config)
    check(f"{sh}: srp out before the output folder exists fails with a message", out.stdout.strip() == "status 1" and "does not exist yet" in out.stderr, out.tail())
    run(SRP, "unset", "proj", "outdir", config=config)

# srp edit opens the sources of the program: the .py files of its folder, for a C++ program those of the source folder of its CMake
# build, else of its own folder (not those of neighbouring folders); without source files it says so
stub = config_folder("opener")
for opener in ("open", "xdg-open"):  # macOS opens with open, Linux with xdg-open: neither may reach the real one (it opens windows)
    with open(os.path.join(stub, opener), "w") as f:
        f.write('#!/bin/sh\necho "OPENED $*"\n')
    os.chmod(os.path.join(stub, opener), 0o755)
tree = config_folder("edit")
for path in ("py/def.py", "py/helper.py", "cpp/main.cpp", "cpp/build/prog", "loose/prog/prog", "loose/prog/main.cpp", "loose/other/a.cpp"):
    os.makedirs(os.path.join(tree, os.path.dirname(path)), exist_ok=True)
    open(os.path.join(tree, path), "w").close()
for path in ("cpp/build/prog", "loose/prog/prog"):
    os.chmod(os.path.join(tree, path), 0o755)
with open(os.path.join(tree, "cpp", "build", "CMakeCache.txt"), "w") as f:
    f.write(f"CMAKE_HOME_DIRECTORY:INTERNAL={os.path.join(tree, 'cpp')}\n")
edits = {"py/def.py": ("py/def.py", "py/helper.py"), "cpp/build/prog": ("cpp/main.cpp",), "loose/prog/prog": ("loose/prog/main.cpp",)}
editConfig = config_folder("config_edit")
for k, program in enumerate(edits):
    run(SRP, "add", f"e{k}", os.path.join(tree, program), config=editConfig)
os.makedirs(os.path.join(tree, "bare"), exist_ok=True)
open(os.path.join(tree, "bare", "prog"), "w").close()
os.chmod(os.path.join(tree, "bare", "prog"), 0o755)
run(SRP, "add", f"e{len(edits)}", os.path.join(tree, "bare", "prog"), config=editConfig)
for sh in SHELLS:
    for k, (program, wanted) in enumerate(edits.items()):
        out = shell(f"srp edit e{k}", sh, config=editConfig, env={"PATH": f"{stub}:{os.environ['PATH']}"})
        # open gets all files in one call, xdg-open one call per file
        opened = sorted(os.path.relpath(os.path.normpath(path), tree) for line in out.stdout.splitlines() if line.startswith("OPENED ") for path in line.split()[1:])
        check(f"{sh}: srp edit of {program} opens the sources of the program", opened == sorted(wanted), f"{opened} {out.tail()}")
    # a program without source files next to it: an error saying so, nothing opened
    out = shell(f"srp edit e{len(edits)}; echo status $?", sh, config=editConfig, env={"PATH": f"{stub}:{os.environ['PATH']}"})
    check(f"{sh}: srp edit of a program without sources says so", "OPENED" not in out.stdout and "status 0" not in out.stdout and "No source files found" in out.stderr, out.tail())

# an insecure directory in fpath (group or world writable, as Homebrew's on macOS): compinit cannot ask without a terminal
if "zsh" in SHELLS:
    insecure = config_folder("fpath_insecure")
    os.chmod(insecure, 0o777)
    default = run("zsh", "-f", "-c", "print -r -- $FPATH").stdout.strip()
    out = shell("print -r -- $_comps[plot]", "zsh", config=config, interactive=True, env={"FPATH": f"{insecure}:{default}"})
    check("zsh: without a terminal and with an insecure directory in fpath, env.sh registers the completion silently", out.stdout.strip() == "_plot_completions_zsh" and not out.stderr.strip(), out.tail())

# srp info: the installation of the env.sh sourced, the configuration folder and project, the version and the ROOT found at build time
version = re.search(r"^\s*VERSION (\S+)", open(os.path.join(SOURCE, "CMakeLists.txt")).read(), re.MULTILINE).group(1)
infoConfig = config_folder("info_config")
for sh in SHELLS:
    out = shell("srp info; echo prefix=$SCIROOPLOT_ROOT", sh, config=infoConfig)
    text = re.sub(r"\x1b\[[0-9;]*m", "", out.stdout)
    rows = {m.group(1): m.group(2).strip() for m in re.finditer(r"^\s*\|?\s+(prefix|folder|current project|ROOT)\s{2,}(.*)$", text, re.MULTILINE)}
    sourced = re.search(r"^prefix=(.*)$", text, re.MULTILINE)
    ok = out.rc == 0 and f"SciRooPlot {version}" in text and sourced and rows.get("prefix") == sourced.group(1) and rows.get("folder") == f"{infoConfig}  (SCIROOPLOT_CONFIG_PATH)"
    ok = ok and rows.get("current project") == "none" and rows.get("ROOT", "").split()[:1] == [ROOT.gROOT.GetVersion().replace("/", ".")]
    check(f"{sh}: srp info names the version, the installation sourced, the configuration folder, no current project and the ROOT version", ok, f"{rows} | {out.tail()}")

# the srp program alone
for args, message in (
    (("init-py", "x"), "Source environment script"),
    (("cd",), "Source environment script"),
    (("nosuch",), "Unknown command nosuch"),
    ((), "Not enough arguments"),
    (("set", "proj", "x", "two", "words"), "Too many arguments"),
    (("remove", "proj", "junk"), "Too many arguments for srp remove"),
    (("help", "junk"), "Too many arguments for srp help"),
    (("plotmode", "png", "junk"), "Too many arguments for srp plotmode"),
):
    out = run(SRP, *args, config=config)
    check(f"srp {' '.join(args) or '(nothing)'} without env.sh: exit 1, {message}", out.rc == 1 and len(out.errors()) == 1 and message in out.errors()[0], out.tail())
finish()
