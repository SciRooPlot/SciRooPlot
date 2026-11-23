"""Tab completion offers what the programs accept, in bash and zsh (zsh's checks are left out where it is not installed).
plot: the groups and names it offers for a typed regular expression are those plot itself selects for it (the plot program is the
reference: completion matches with the shell's POSIX ERE, plot with std::regex, so env.sh translates \\d \\w \\s and their negations),
in every combination of matchmode and matchcase: a group word offers the main groups (all groups once it holds a slash) starting with a
match (any match in contains mode), a name word the names of the plots plot selects for the complete group word, trailing slashes
included, for the project SCIROOPLOT_PROJECT names if set. bash inserts a single match and keeps the typed word (group or name), open for
editing, when the matches do not all start with it. srp: the commands, plot modes and
setting values it offers are exactly those srp help documents, and srp accepts each of them; the projects offered are the registered ones (also with a ~ in SCIROOPLOT_CONFIG_PATH that the shell did not expand);
the properties offered for a project are program, outdir and the settings its program declares, with their descriptions, and the values
offered for a setting are on and off of a switch or the choices of a text, each accepted by srp set; bash inserts them escaped (a choice with spaces,
quotes or braces stays one word) and offers them for their typed start, also typed escaped as it inserts them.
Completing 3000 plots takes well below two seconds.
Covers: regex dialect, trailing slash, projects with a brace in a setting value, descriptions and choices with quotes, braces and a backslash, completion speed."""

import os
import pty
import re
import select
import shlex
import signal
import time

from srptest import ENV_SCRIPT, PLOT, SHELLS, SRP, WORK, cases, check, config_folder, describe, finish, listed, run, shell

PLOTS = [("p1", "QA1"), ("p2", "QA1"), ("p1", "QA1/sub"), ("p1", "QA2"), ("p1", "QAd"), ("p1", "QA_x"), ("p1", "QA-y"), ("p1", "qa3"), ("h_1", "ab"), ("h_d", "ab"), ("hx", "ab")]
PLOTS += [("p", "group"), ("q", "group/sub"), ("r", "groupx"), ("s", "other/group")]
GROUP_WORDS = ["", "q", "QA\\d", "QA\\D", "QA\\w", "QA\\W", "QA\\s", "QA\\S", "QA[\\d_]", "QA[^\\d]", "QA[\\w-]", "QA[[:digit:]]", "Q.\\d|ab", "QA\\.", "QA1/s", "other/", "gr"]
NAME_WORDS = [("QA\\d", ""), ("QA\\d", "p\\d"), ("ab", "h_\\d"), ("ab", "h\\W"), ("ab", "h\\w\\w"), ("QA1/", "p"), ("group/", ""), ("group/sub/", ""), ("group", ""), ("QA", "")]
SETTINGS = {"exact": ("exact", "sensitive"), "contains": ("contains", "sensitive"), "exact_icase": ("exact", "insensitive"), "contains_icase": ("contains", "insensitive")}


def define(config, project, plots, declare=""):
    """a project whose plots.info and plot list hold plots (pairs of name and group) of one function each, and the settings declare
    declares"""
    code = f"""
from SciRooPlot import *
pm = PlotManager({project!r})
{declare}
pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
for name, group in {plots!r}:
    plot = Plot(name, group, "1d")
    plot[1].AddFunction("x")
    pm.AddPlot(plot)
pm.SaveProject()
"""
    program = os.path.join(config, f"{project}.py")
    with open(program, "w") as f:
        f.write(code)
    out = shell(f'"$SCIROOPLOT_PYTHON_EXECUTABLE" {program}', config=config)
    added = run(SRP, "add", project, program, config=config)
    if out.rc != 0 or added.rc != 0:
        raise SystemExit(f"cannot define project {project}: {out.tail()} {added.tail()}")


def quote(text):
    return "'" + text.replace("'", "'\\''") + "'"


# the settings of project comp: name, default, description, choices (values with quotes, braces, a backslash and spaces)
DECLARED = [
    ("syst", True, "draw systematic uncertainties", None),
    ("energy", "5.02", 'collision energy {in TeV}: "sqrt s" \\ per pair', ["5.02", "13.6", 'a {b} "c"']),
    ("nBins", 50, "", None),
    ("zz", "", "", None),
    ("yy", "", "", None),
]
config = config_folder("config")
define(config, "comp", PLOTS, "\n".join(f"pm.Setting({n!r}, {d!r}, {t!r}{'' if c is None else f', {c!r}'})" for n, d, t, c in DECLARED))
# the reference for group words: one plot in every group and in every main group
refConfig = config_folder("config_reference")
groups = sorted({g for _, g in PLOTS})
mainGroups = sorted({g.split("/")[0] for g in groups})
define(refConfig, "ref", [("r", g) for g in sorted(set(groups) | set(mainGroups))])


def settings(setting):
    for cfg in (config, refConfig):
        run(SRP, "matchmode", SETTINGS[setting][0], config=cfg)
        run(SRP, "matchcase", SETTINGS[setting][1], config=cfg)


def offered_groups(sh, word):
    """the groups the completion of the first word of plot offers (before bash decides whether to insert them)"""
    out = shell(
        f"""_scirooplot_helper load_plot_lists
if [[ {quote(word)} == */* ]]; then names=("${{groups[@]}}"); else names=("${{main_groups[@]}}"); fi
_scirooplot_helper filter_names_regex {quote(word)}
printf '%s\\n' "${{matches[@]}}" """,
        sh,
        config=config,
        interactive=True,
    )
    return {x for x in out.stdout.splitlines() if x}, out


def offered_names(sh, group, word):
    out = shell(
        f"""_scirooplot_helper load_plot_lists
_scirooplot_helper update_plot_names {quote(group)}
_scirooplot_helper filter_names_regex {quote(word)}
printf '%s\\n' "${{matches[@]}}" """,
        sh,
        config=config,
        interactive=True,
    )
    return {x for x in out.stdout.splitlines() if x}, out


todo = cases(
    {"shell": list(SHELLS), "setting": list(SETTINGS), "word": [("group", w) for w in GROUP_WORDS] + [("name", w) for w in NAME_WORDS]},
    named=[
        {"shell": "bash", "setting": "contains", "word": ("name", ("group/", "")), "why": "not groupx"},
        {"shell": "bash", "setting": "exact", "word": ("name", ("group/sub/", "")), "why": "a trailing slash ends the group name"},
        {"shell": "bash", "setting": "exact", "word": ("group", "QA\\d"), "why": "regex classes of ECMAScript translated"},
        {"shell": "bash", "setting": "exact_icase", "word": ("name", ("ab", "h_\\d")), "why": "regex classes of ECMAScript translated"},
    ],
)
current = None
for case in sorted(todo, key=lambda c: c["setting"]):
    if case["setting"] != current:
        current = case["setting"]
        settings(current)
    what = describe(case)
    kind, word = case["word"]
    if kind == "group":
        got, out = offered_groups(case["shell"], word)
        selected = {g for g, _ in listed(run(PLOT, f"({word}).*", ".+", "list", config=refConfig))}
        want = selected & set(groups if "/" in word else mainGroups)
    else:
        group, name = word
        got, out = offered_names(case["shell"], group, name)
        want = {n for _, n in listed(run(PLOT, group, f"({name}).*", "list", config=config))}
    check(f"offers what plot selects: {what}", got == want and not out.stderr.strip(), f"offers {sorted(got)}, plot selects {sorted(want)}; {out.stderr.strip()[-200:]}")

# what ERE cannot compile completes nothing (and says nothing)
settings("exact")
for sh in SHELLS:
    got, out = offered_groups(sh, "(?:QA)1")
    check(f"{sh}: (?:...) completes nothing", got == set() and not out.stderr.strip(), f"{sorted(got)} {out.stderr[-200:]}")


def bash_complete(*words, cfg=config):
    """COMPREPLY of the bash completion of plot or srp for the words typed so far (the last one is being completed)"""
    function = "_srp_completions_bash" if words[0] == "srp" else "_plot_completions_bash"
    out = shell(f"COMP_WORDS=({' '.join(quote(w) for w in words)}); COMP_CWORD={len(words) - 1}; {function}; printf '%s\\n' \"${{COMPREPLY[@]}}\"", "bash", config=cfg, interactive=True)
    return [x for x in out.stdout.splitlines() if x], out


# bash inserts a single match, offers several that start with the typed word, keeps the word otherwise
for words, want in ((("plot", "qa\\d"), ["qa3"]), (("plot", "ab", "h_\\d"), ["h_1"]), (("plot", "QA\\d", "p\\d"), ["p\\d"]), (("plot", "QA"), ["QA1", "QA2", "QA_x", "QA-y", "QAd"])):
    got, out = bash_complete(*words)
    check(f"bash completion of {' '.join(words[1:])}: {want}", sorted(got) == sorted(want), f"{got} {out.stderr[-200:]}")


def bash_tab(*lines, cfg=config):
    """what bash in a terminal puts after each typed line when Tab is pressed at its end (readline's own insertion, not COMPREPLY), all in
    one session (each line is cleared before the next)"""
    home = os.path.join(WORK, "home_bash_tab")
    os.makedirs(home, exist_ok=True)
    pid, fd = pty.fork()
    if pid == 0:
        os.execvpe("bash", ["bash", "--norc", "--noprofile", "-i"], dict(os.environ, HOME=home, SCIROOPLOT_CONFIG_PATH=cfg, PS1="> ", TERM="dumb"))

    def read(seconds):
        out, end = b"", time.time() + seconds
        while time.time() < end:
            if select.select([fd], [], [], 0.1)[0]:
                try:
                    out += os.read(fd, 4096)
                except OSError:
                    break
        return out.decode(errors="replace")

    read(1)
    os.write(fd, f'source "{ENV_SCRIPT}"\n'.encode())
    read(2)
    inserted = []
    for line in lines:
        os.write(fd, line.encode())
        read(1)
        os.write(fd, b"\t")
        inserted.append(read(2))
        os.write(fd, b"\x05\x15")  # end of line, then cleared (the typed line is never run)
        read(0.5)
    os.kill(pid, signal.SIGKILL)
    os.waitpid(pid, 0)
    os.close(fd)
    return inserted


# where bash keeps the typed word (the matches do not all start with it), the word stays open: no space after it; a single match is
# inserted with a space in the same session (completion runs)
settings("exact")
control, group, name = bash_tab("plot QA1/su", "plot QA\\d", "plot QA\\d p\\d")
check("bash inserts a single match with a space (the completion runs)", control.replace("\x07", "") == "b ", repr(control))
for word, inserted in (("group", group), ("name", name)):
    check(f"bash keeps an ambiguous plot {word} word open for editing (no space added)", " " not in inserted.replace("\x07", ""), repr(inserted))

# srp: what is offered is what srp help documents and srp accepts
helpText = run(SRP, "help").stdout
commands = {m.group(1) for m in re.finditer(r"^\s+srp ([a-z-]+)", helpText, re.MULTILINE)}
modeRows = re.search(r"Plot modes\n(.*?)\n\S", helpText, re.DOTALL).group(1)
modes = {re.sub(r"\[.*", "", m) for row in modeRows.splitlines() for m in re.split(r"\s{2,}", row.strip())[0].split()}
values = {"color": re.search(r"srp color \(([^)]*)\)", helpText).group(1), "matchmode": re.search(r"srp matchmode \(([^)]*)\)", helpText).group(1), "matchcase": re.search(r"srp matchcase \(([^)]*)\)", helpText).group(1), "verbosity": re.search(r"^\s+<level>\s+(.*)", helpText, re.MULTILINE).group(1)}
values = {k: set(v.split(" | ")) for k, v in values.items()}
values["plotmode"] = modes
got, out = bash_complete("srp", "")
check("srp offers the commands of srp help", set(got) == commands, f"offers but not documented {sorted(set(got) - commands)}, documented but not offered {sorted(commands - set(got))}")
got, out = bash_complete("plot", "QA1", "p1", "")
check("plot offers the modes of srp help", set(got) == modes, f"offers {sorted(got)}, documented {sorted(modes)}")
settingsConfig = config_folder("config_settings")
for setting, documented in values.items():
    got, out = bash_complete("srp", setting, "")
    check(f"srp {setting} offers the values of srp help", set(got) == documented, f"offers {sorted(got)}, documented {sorted(documented)}")
    refused, notShown = [], []
    for v in sorted(documented):
        if run(SRP, setting, v, config=settingsConfig).rc != 0:
            refused.append(v)
        elif (shown := run(SRP, setting, config=settingsConfig)).rc != 0 or shown.stdout.strip() != v:
            notShown.append(f"{v}: {shown.stdout.strip() or shown.stderr.strip()}")
    check(f"srp {setting} accepts every documented value", not refused, f"refused {refused}")
    check(f"srp {setting} without a value prints the value set", not notShown, f"{notShown}")
# srp cache (on [<folder>] | off): on and off (srp settings.py checks what is stored and shown)
cacheValues = {re.sub(r"\s*\[.*\]", "", v) for v in re.search(r"srp cache \((.*?)\)\s{2,}", helpText).group(1).split(" | ")}
got, out = bash_complete("srp", "cache", "")
check("srp cache offers the values of srp help", set(got) == cacheValues == {"on", "off"}, f"offers {sorted(got)}, documented {sorted(cacheValues)}")
for sh in SHELLS[1:]:  # the lists zsh offers come from the same helper
    out = shell('_scirooplot_helper load_srp_lists; printf "%s\\n" "${general_commands[@]}" "${project_commands[@]}" "${file_commands[@]}"', sh, config=config, interactive=True)
    check(f"{sh}: srp offers the commands of srp help", set(out.stdout.split()) == commands, f"{sorted(set(out.stdout.split()) ^ commands)}")

# the projects offered: the registered ones and @current, also with braces in a setting value
run(SRP, "add", "second", "/tmp/second/def.py", config=config)
run(SRP, "set", "comp", "zz", "{x}", config=config)
run(SRP, "set", "comp", "yy", "a {", config=config)
for sh in SHELLS:
    out = shell('_scirooplot_helper load_srp_lists; printf "%s\\n" "${projects[@]}"', sh, config=config, interactive=True)
    check(f"{sh}: the projects offered are the registered ones", sorted(out.stdout.split()) == ["@current", "comp", "second"], out.tail())
got, out = bash_complete("srp", "select", "")
check("srp select offers the projects", sorted(got) == ["@current", "comp", "second"], f"{got}")
run(SRP, "select", "comp", config=config)

# the properties offered: program, outdir and the settings the program declares, with their descriptions (zsh shows them); the values
# of a setting: on and off of a switch, the choices of a text, and srp set accepts each of them
names = [n for n, _, _, _ in DECLARED]
for command, project in (("get", "comp"), ("set", "@current"), ("unset", "comp")):
    got, out = bash_complete("srp", command, project, "")
    check(f"srp {command} {project} offers program, outdir and the settings declared", got == ["program", "outdir", *names], f"{got}")
got, out = bash_complete("srp", "get", "second", "")
check("srp get offers program and outdir for a project without settings", got == ["program", "outdir"], f"{got}")
for sh in SHELLS:
    out = shell('_scirooplot_helper load_srp_lists; _scirooplot_helper load_project_settings comp; for n in "${setting_names[@]}"; do echo "$n"; done; for d in "${setting_descriptions[@]}"; do echo "D:$d"; done', sh, config=config, interactive=True)
    lines = out.stdout.splitlines()
    want = names + [f"D:{t}" for _, _, t, _ in DECLARED]
    check(f"{sh}: the settings offered with their descriptions as declared", lines == want, f"{lines}")
for name, default, _, choices in DECLARED:
    offered = choices or (["on", "off"] if isinstance(default, bool) else [])
    got, out = bash_complete("srp", "set", "comp", name, "")
    words = [shlex.split(g) for g in got]  # bash inserts the reply as typed: escaped where needed
    check(f"srp set comp {name} offers {offered or 'nothing'}, each as one word", words == [[v] for v in offered], f"{got}")
    for sh in SHELLS[1:]:
        out = shell(f'_scirooplot_helper load_srp_lists; _scirooplot_helper load_project_settings @current {name}; for v in "${{setting_values[@]}}"; do echo "$v"; done', sh, config=config, interactive=True)
        check(f"{sh}: srp set comp {name} offers {offered or 'nothing'}", out.stdout.splitlines() == offered, out.tail())
    for value in offered:
        typed = value.split(" ")[0]  # the start of a value with spaces: bash splits the typed words there
        got, out = bash_complete("srp", "set", "comp", name, typed)
        check(f"srp set comp {name} {typed!r} offers {value!r}", any(shlex.split(g) == [value] for g in got), f"{got}")
        if " " in value:  # typed escaped past a space, as bash inserted it: one word
            escaped = re.sub(r"""([\s"'{}\\$])""", r"\\\1", value)
            typed = escaped[: escaped.index("\\ ") + 2]
            got, out = bash_complete("srp", "set", "comp", name, typed)
            check(f"srp set comp {name} {typed!r} (escaped) offers {value!r}", any(shlex.split(g) == [value] for g in got), f"{got}")
    refused = [v for v in offered if run(SRP, "set", "comp", name, v, config=config).rc != 0]
    check(f"srp set comp {name} accepts every value offered", not refused, f"refused {refused}")
    run(SRP, "unset", "comp", name, config=config)

# a configuration path with a ~ the shell did not expand (SCIROOPLOT_CONFIG_PATH='~/cfg'): the home folder, as srp reads it
for sh in SHELLS:
    home = os.path.join(os.path.dirname(config), f"home_{sh}")
    os.makedirs(home, exist_ok=True)
    link = os.path.join(home, "tilde_config")
    if not os.path.islink(link):
        os.symlink(config, link)
    out = shell('_scirooplot_helper load_config; echo "current: ${current_project}"', sh, interactive=True, env={"SCIROOPLOT_CONFIG_PATH": "~/tilde_config"})
    check(f"{sh}: a ~ in SCIROOPLOT_CONFIG_PATH is the home folder", "current: comp" in out.stdout, out.tail())
    # the project of one process (SCIROOPLOT_PROJECT), as for plot itself
    out = shell('_scirooplot_helper load_config; echo "current: ${current_project}"', sh, config=config, interactive=True, env={"SCIROOPLOT_PROJECT": "second"})
    check(f"{sh}: SCIROOPLOT_PROJECT is the project completed", "current: second" in out.stdout, out.tail())

# speed with 3000 plots in 150 groups
bigConfig = config_folder("config_big")
define(bigConfig, "big", [(f"plot{n}", f"group{g}/sub{n % 3}") for g in range(50) for n in range(60)])
for sh in SHELLS:
    out = shell(
        """_scirooplot_helper load_plot_lists; names=("${main_groups[@]}"); _scirooplot_helper filter_names_regex group1
_scirooplot_helper load_plot_lists; _scirooplot_helper update_plot_names group1/sub1; _scirooplot_helper filter_names_regex ''
echo "${#matches[@]}" """,
        sh,
        config=bigConfig,
        interactive=True,
    )
    check(f"{sh}: completing a group and the names of 3000 plots takes below 2 s", out.stdout.split()[-1:] == ["20"] and out.seconds < 2.0, f"{out.seconds:.2f} s, {out.tail()}")
finish()
