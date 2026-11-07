"""Tab completion offers what the programs accept, in bash and zsh (zsh's checks are left out where it is not installed).
plot: the groups and names it offers for a typed regular expression are those plot itself selects for it (the plot program is the
reference: completion matches with the shell's POSIX ERE, plot with std::regex, so env.sh translates \\d \\w \\s and their negations),
in every combination of matchmode and matchcase: a group word offers the main groups (all groups once it holds a slash) starting with a
match (any match in contains mode), a name word the names of the plots plot selects for the complete group word, trailing slashes
included. bash inserts a single match and keeps the typed word when the matches do not all start with it. srp: the commands, plot modes and
setting values it offers are exactly those srp help documents, and srp accepts each of them; the projects offered are the registered ones (also with a ~ in SCIROOPLOT_CONFIG_PATH that the shell did not expand).
Completing 3000 plots takes well below two seconds.
Covers: cd59237 (regex dialect), cc729f2 (trailing slash), e7af591 (projects with a brace in a property), suite_cli (speed)."""

import os
import re

from srptest import PLOT, SHELLS, SRP, cases, check, config_folder, describe, finish, listed, run, shell

PLOTS = [("p1", "QA1"), ("p2", "QA1"), ("p1", "QA1/sub"), ("p1", "QA2"), ("p1", "QAd"), ("p1", "QA_x"), ("p1", "QA-y"), ("p1", "qa3"), ("h_1", "ab"), ("h_d", "ab"), ("hx", "ab")]
PLOTS += [("p", "group"), ("q", "group/sub"), ("r", "groupx"), ("s", "other/group")]
GROUP_WORDS = ["", "q", "QA\\d", "QA\\D", "QA\\w", "QA\\W", "QA\\s", "QA\\S", "QA[\\d_]", "QA[^\\d]", "QA[\\w-]", "QA[[:digit:]]", "Q.\\d|ab", "QA\\.", "QA1/s", "other/", "gr"]
NAME_WORDS = [("QA\\d", ""), ("QA\\d", "p\\d"), ("ab", "h_\\d"), ("ab", "h\\W"), ("ab", "h\\w\\w"), ("QA1/", "p"), ("group/", ""), ("group/sub/", ""), ("group", ""), ("QA", "")]
SETTINGS = {"exact": ("exact", "sensitive"), "contains": ("contains", "sensitive"), "exact_icase": ("exact", "insensitive"), "contains_icase": ("contains", "insensitive")}


def define(config, project, plots):
    """a project whose plots.info and plot list hold plots (pairs of name and group) of one function each"""
    code = f"""
from SciRooPlot import *
pm = PlotManager({project!r})
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


config = config_folder("config")
define(config, "comp", PLOTS)
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
        {"shell": "bash", "setting": "contains", "word": ("name", ("group/", "")), "why": "cc729f2: not groupx"},
        {"shell": "bash", "setting": "exact", "word": ("name", ("group/sub/", "")), "why": "cc729f2"},
        {"shell": "bash", "setting": "exact", "word": ("group", "QA\\d"), "why": "cd59237"},
        {"shell": "bash", "setting": "exact_icase", "word": ("name", ("ab", "h_\\d")), "why": "cd59237"},
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
    check(f"{sh}: (?:...) completes nothing [cd59237]", got == set() and not out.stderr.strip(), f"{sorted(got)} {out.stderr[-200:]}")


def bash_complete(*words, cfg=config):
    """COMPREPLY of the bash completion of plot or srp for the words typed so far (the last one is being completed)"""
    function = "_srp_completions_bash" if words[0] == "srp" else "_plot_completions_bash"
    out = shell(f"COMP_WORDS=({' '.join(quote(w) for w in words)}); COMP_CWORD={len(words) - 1}; {function}; printf '%s\\n' \"${{COMPREPLY[@]}}\"", "bash", config=cfg, interactive=True)
    return [x for x in out.stdout.splitlines() if x], out


# bash inserts a single match, offers several that start with the typed word, keeps the word otherwise
for words, want in ((("plot", "qa\\d"), ["qa3"]), (("plot", "ab", "h_\\d"), ["h_1"]), (("plot", "QA\\d", "p\\d"), ["p\\d"]), (("plot", "QA"), ["QA1", "QA2", "QA_x", "QA-y", "QAd"])):
    got, out = bash_complete(*words)
    check(f"bash completion of {' '.join(words[1:])}: {want}", sorted(got) == sorted(want), f"{got} {out.stderr[-200:]}")

# srp: what is offered is what srp help documents and srp accepts
helpText = run(SRP, "help").stdout
commands = {m.group(1) for m in re.finditer(r"^\s+srp ([a-z-]+)", helpText, re.M)}
modeRows = re.search(r"Plot modes\n(.*?)\n\S", helpText, re.S).group(1)
modes = {re.sub(r"\[.*", "", m) for row in modeRows.splitlines() for m in re.split(r"\s{2,}", row.strip())[0].split()}
values = {"color": re.search(r"srp color \(([^)]*)\)", helpText).group(1), "matchmode": re.search(r"srp matchmode \(([^)]*)\)", helpText).group(1), "matchcase": re.search(r"srp matchcase \(([^)]*)\)", helpText).group(1), "verbosity": re.search(r"^\s+<level>\s+(.*)", helpText, re.M).group(1)}
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
    refused = [v for v in sorted(documented) if run(SRP, setting, v, config=settingsConfig).rc != 0]
    check(f"srp {setting} accepts every documented value", not refused, f"refused {refused}")
for sh in SHELLS[1:]:  # the lists zsh offers come from the same helper
    out = shell('_scirooplot_helper load_srp_lists; printf "%s\\n" "${general_commands[@]}" "${project_commands[@]}" "${file_commands[@]}"', sh, config=config, interactive=True)
    check(f"{sh}: srp offers the commands of srp help", set(out.stdout.split()) == commands, f"{sorted(set(out.stdout.split()) ^ commands)}")

# the projects offered: the registered ones and @current, also with braces in a property value (e7af591)
run(SRP, "add", "second", "/tmp/second/def.py", config=config)
run(SRP, "set", "comp", "zz", "{x}", config=config)
run(SRP, "set", "comp", "yy", "a {", config=config)
for sh in SHELLS:
    out = shell('_scirooplot_helper load_srp_lists; printf "%s\\n" "${projects[@]}"', sh, config=config, interactive=True)
    check(f"{sh}: the projects offered are the registered ones [e7af591]", sorted(out.stdout.split()) == ["@current", "comp", "second"], out.tail())
got, out = bash_complete("srp", "select", "")
check("srp select offers the projects", sorted(got) == ["@current", "comp", "second"], f"{got}")
got, out = bash_complete("srp", "get", "comp", "")
check("srp get offers the properties program and outdir", sorted(got) == ["outdir", "program"], f"{got}")
run(SRP, "select", "comp", config=config)

# a configuration path with a ~ the shell did not expand (SCIROOPLOT_CONFIG_PATH='~/cfg'): the home folder, as srp reads it
for sh in SHELLS:
    home = os.path.join(os.path.dirname(config), f"home_{sh}")
    os.makedirs(home, exist_ok=True)
    link = os.path.join(home, "tilde_config")
    if not os.path.islink(link):
        os.symlink(config, link)
    out = shell('_scirooplot_helper load_config; echo "current: ${current_project}"', sh, interactive=True, env={"SCIROOPLOT_CONFIG_PATH": "~/tilde_config"})
    check(f"{sh}: a ~ in SCIROOPLOT_CONFIG_PATH is the home folder", "current: comp" in out.stdout, out.tail())

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
