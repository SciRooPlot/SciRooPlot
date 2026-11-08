"""plot <group> <name> [<mode>] selects exactly the plots the documented rule names: group and name are regular expressions
(ECMAScript, as std::regex) matching the whole name in matchmode exact and any part of it in matchmode contains, case-insensitive with
matchcase insensitive; a group pattern also selects the subgroups of the groups it matches; slashes at the end of a group pattern end the
group name there (group/ is group and its subgroups, in contains mode also other/group, never groupx), reported in one info line. The
reference is that rule written with Python's re, for every combination of the settings, over groups and names with regex characters,
subgroups and case variants. It exits 1 with one error when nothing matches (naming shell wildcards it suspects), for an invalid pattern
(naming it), an invalid mode (before the plots are looked up), wrong arguments or no selected project; srp plotmode sets the mode used
when none is given.
Covers: trailing slash ends the group name, wildcard hint keeps \\d*, invalid mode rejected
early, exit codes."""

import re

from srptest import PLOT, Project, S, cases, check, config_folder, describe, finish, listed, run

GROUPS = ["a", "a/b", "a/b/c", "ab", "x/a", "A", "grp", "QA1", "QA1/sub", "QA2", "qa3", "QA_x", "group", "group/sub", "groupx", "other/group"]
NAMES = ["p", "p2", "P", "h_1"]
GROUP_PATTERNS = [".+", "a", "a/b", "A", "ab|grp", "a.b", "a\\.b", "a\\+", "QA\\d", "qa\\d", "QA\\D", "Q.\\d|ab", "[aA]", "QA*", "ro"]
GROUP_PATTERNS += ["group/", "group//", "group/sub/", "a\\/", "/", "x/", "a("]
NAME_PATTERNS = [".+", "p", "P", "p\\d", "h_\\d", "p|P", "\\w+", "["]
SETTINGS = {"exact": ("exact", "sensitive"), "contains": ("contains", "sensitive"), "exact_icase": ("exact", "insensitive"), "contains_icase": ("contains", "insensitive")}

p = Project("selection")
for group in GROUPS:
    for name in NAMES:
        plot = S.Plot(name, group, "1d")
        plot[1].AddFunction("x")
        p.pm.AddPlot(plot)
p.pm.SaveProject()
p.srp("add", p.name, p.path("DefinePlots.py"))
p.srp("select", p.name)


def reference(group, name, setting):
    """the plots the rule selects, None for an invalid pattern; and whether the group pattern ended in slashes"""
    contains, insensitive = SETTINGS[setting][0] == "contains", SETTINGS[setting][1] == "insensitive"
    flags = re.I if insensitive else 0
    pattern = group
    while len(pattern) > 1 and pattern.endswith("/") and not pattern.endswith("\\/"):
        pattern = pattern[:-1]
    endsHere = pattern != group
    try:
        groupRegex = re.compile(f"(?:{pattern})(?=/|$)" if endsHere and contains else f"(?:{pattern})(?:/.*)?", flags)
        nameRegex = re.compile(name, flags)
    except re.error:
        return None, endsHere
    match = (lambda regex, text: regex.search(text)) if contains else (lambda regex, text: regex.fullmatch(text))
    return {(g, n) for g in GROUPS for n in NAMES if match(groupRegex, g) and match(nameRegex, n)}, endsHere


def settings(setting):
    mode, case = SETTINGS[setting]
    p.srp("matchmode", mode)
    p.srp("matchcase", case)


todo = cases({"group": GROUP_PATTERNS, "name": ["p", ".+"], "setting": list(SETTINGS)})
todo += cases(
    {"group": [".+"], "name": NAME_PATTERNS, "setting": list(SETTINGS)},
    named=[
        {"group": "group/", "name": ".+", "setting": "exact", "why": "a trailing slash ends the group name"},
        {"group": "group/", "name": ".+", "setting": "contains", "why": "not groupx"},
        {"group": "group//", "name": "p", "setting": "contains", "why": "a trailing slash ends the group name"},
        {"group": "a\\/", "name": "p", "setting": "exact", "why": "an escaped slash stays"},
        {"group": "/", "name": "p", "setting": "exact", "why": "a slash alone stays"},
    ],
)
current = None
for case in sorted(todo, key=lambda c: c["setting"]):
    if case["setting"] != current:
        current = case["setting"]
        settings(current)
    what = describe(case)
    want, endsHere = reference(case["group"], case["name"], case["setting"])
    out = run(PLOT, case["group"], case["name"], "list")
    got = listed(out)
    slashLines = out.lines("the slash ends the group name")
    if want is None:
        bad = "group" if reference(case["group"], ".+", case["setting"])[0] is None else "name"
        named = any(f"'{case[bad]}' is not a valid {bad} pattern" in line for line in out.errors())
        check(f"invalid pattern refused: {what}", out.rc == 1 and not got and named, f"rc {out.rc}, {len(got)} listed: {out.tail()}")
        continue
    check(f"selection: {what}", got == want, f"extra {sorted(got - want)[:6]}, missing {sorted(want - got)[:6]}")
    if want:
        check(f"exit 0 without errors: {what}", out.rc == 0 and not out.errors(), f"rc {out.rc}: {out.tail()}")
    else:
        check(f"nothing matches: exit 1 with the reason: {what}", out.rc == 1 and any("Found no plots matching" in line for line in out.errors()), f"rc {out.rc}: {out.tail()}")
    check(f"one info line exactly for a trailing slash: {what}", len(slashLines) == (1 if endsHere else 0), f"{len(slashLines)} lines")

# the arguments: the mode is checked before the plots are looked up, also when none would match
settings("exact")
for group, name in (("a", "p"), ("nosuchgroup", "p")):
    out = run(PLOT, group, name, "nosuchmode")
    check(f"invalid mode refused before reading the plots ({group})", out.rc == 1 and any("Invalid plot mode 'nosuchmode'" in line for line in out.errors()) and not out.lines("Found no plots") and not listed(out), out.tail())
out = run(PLOT, "a", "p", "list", "extra")
check("too many arguments refused", out.rc == 1 and any("Too many arguments" in line for line in out.errors()), out.tail())
out = run(PLOT, "a")
check("a missing name refused", out.rc == 1 and any("Specify the group and the name" in line for line in out.errors()), out.tail())
p.srp("plotmode", "list")
out = run(PLOT, "a", "p")
check("srp plotmode sets the mode used without one", out.rc == 0 and listed(out) == reference("a", "p", "exact")[0], out.tail())
p.srp("plotmode", "show")
out = run(PLOT, "a", "p", "list", config=config_folder("config_none"))
check("no project selected: exit 1 with the reason", out.rc == 1 and any("No plotting project is selected" in line for line in out.errors()), out.tail())

# shell wildcards in the patterns: named in the error when nothing matches; a * after an escape is a regex repetition
for group, name, hint in (("QA*", "p", "did you mean group 'QA.*'"), ("a", "x*", "did you mean name 'x.*'"), ("zz*", "q*", "group 'zz.*' and name 'q.*'"), ("zz", "q\\d*", None)):
    out = run(PLOT, group, name, "list")
    message = " ".join(out.errors())
    ok = out.rc == 1 and (hint in message if hint else "did you mean" not in message and "plot .+ .+ list shows all plots" in message)
    check(f"no match for {group} {name}: {'hint ' + hint if hint else 'no wildcard hint'}" + ("" if hint is None else ""), ok, message[-300:])
finish()
