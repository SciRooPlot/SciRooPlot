"""The shell functions of env.sh, in bash and zsh: the script must be sourced; sourced, plot and srp work also in scripts. At the prompt,
plot gets its patterns unexpanded (.* is not a list of dot files), also after && and inside braces after ||, keeps its exit status, and
file globbing works again afterwards, also after plot in a pipeline. srp makes relative program and output paths absolute (add, set),
keeps other values and paths starting with ~ or a variable as written; srp cd goes to the program folder; srp cd and srp out of an
unknown project, srp out without an output folder or before it exists fail with a message. The srp program alone refuses the
commands that need env.sh, unknown commands and a missing command, with exit status 1. In zsh without a terminal, env.sh registers the
completion silently also with an insecure directory in fpath.
Covers: the bash plot alias in compound commands (pre-release braces rule), 2026-11-03 (zsh read-only names in env.sh)."""

import os

from srptest import ENV_SCRIPT, SHELLS, SRP, check, config_folder, finish, run, shell

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
    check(f"{sh}: sourced in a script, plot and srp work", out.stdout.count("status 0") == 2 and " - p in group g" in out.stdout, out.tail())

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
    check(f"{sh}: plot .* .* at the prompt gets the patterns unexpanded", plotted[:2] == [" - p in group g", " - q in group g"], out.tail())
    check(f"{sh}: plot after && and in braces after || gets the patterns unexpanded", plotted[2:5] == [" - p in group g", " - q in group g", " - q in group g"], f"{plotted}")
    check(f"{sh}: plot keeps its exit status", "status 1" in lines, out.tail())
    check(f"{sh}: globbing works after plot, also after plot in a pipeline", globs == ["glob a.txt b.txt"] * 2, f"{globs}")
    check(f"{sh}: no errors at the prompt", not [line for line in out.stderr.splitlines() if "rror" in line or "not found" in line or "read-only" in line], out.stderr[-300:])

    # relative paths of srp add and srp set
    cfg = config_folder(f"config_paths_{sh}")
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
    check(f"{sh}: srp cd of an unknown project fails with a message", "status 1" in lines[1:2] and "could not cd project 'nosuch'" in out.stderr, out.tail())
    check(f"{sh}: srp out without an output folder fails with a message", lines[2:3] == ["status 1"] and "has no output folder" in out.stderr, out.tail())
    run(SRP, "set", "proj", "outdir", os.path.join(folder, "notyet"), config=config)
    out = shell("srp out proj; echo status $?", sh, config=config)
    check(f"{sh}: srp out before the output folder exists fails with a message", out.stdout.strip() == "status 1" and "does not exist yet" in out.stderr, out.tail())
    run(SRP, "unset", "proj", "outdir", config=config)

# an insecure directory in fpath (group or world writable, as Homebrew's on macOS): compinit cannot ask without a terminal
if "zsh" in SHELLS:
    insecure = config_folder("fpath_insecure")
    os.chmod(insecure, 0o777)
    default = run("zsh", "-f", "-c", "print -r -- $FPATH").stdout.strip()
    out = shell("print -r -- $_comps[plot]", "zsh", config=config, interactive=True, env={"FPATH": f"{insecure}:{default}"})
    check("zsh: without a terminal and with an insecure directory in fpath, env.sh registers the completion silently", out.stdout.strip() == "_plot_completions_zsh" and not out.stderr.strip(), out.tail())

# the srp program alone
for args, message in ((("init-py", "x"), "Source environment script"), (("cd",), "Source environment script"), (("nosuch",), "Unknown command nosuch"), ((), "Not enough arguments")):
    out = run(SRP, *args, config=config)
    check(f"srp {' '.join(args) or '(nothing)'} without env.sh: exit 1, {message}", out.rc == 1 and len(out.errors()) == 1 and message in out.errors()[0], out.tail())
finish()
