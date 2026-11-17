"""A setting the program of a project declares (PlotManager::Setting) is given its value by srp set: the program gets that value, otherwise
the default, in its type (a switch as bool, an integer, a number, a text), the same in C++ and Python; without a project, for a project not
registered yet and for a stored value the declaration does not allow (warned) the default. srp set accepts exactly the values the
declaration allows (on or off, an integer, a finite number, one of the choices, any text) and refuses anything else and an undeclared name
(listing the settings, or saying that the program declares none yet) with exit status 1 and one error, changing nothing; srp get prints the
value or the default; srp unset goes back to the default and removes a value left from a setting the program no longer declares. A
declaration with a reserved or illegal name or with a default it does not allow is refused with one error, gives the default and is not
saved; a setting declared again otherwise is warned, declared again alike is not. srp projects lists the settings in the order declared,
each with its value, the default where it differs and its description, and the values left from settings no longer declared.
SaveProject saves the declarations and removes them once the program declares none.
Covers: a value with spaces, quotes and braces, a value left from a setting with another type, a setting no longer declared, a program
declaring none, invalid declarations, a PlotManager without project."""

import os
import shutil

from srptest import SRP, WORK, cases, check, config_files, config_folder, cpp_program, finish, python, run, syntax_check

# name: default, description, choices, values srp set accepts, values it refuses, the value the program gets for an accepted one
KINDS = {
    "switch": (True, "draw systematic uncertainties", None, ["off", "on"], ["yes", "1", "On", "true"], lambda v: v == "on"),
    "integer": (50, "", None, ["3", "-7", "0"], ["3.5", "x", "99999999999", "1e3"], int),
    "number": (0.5, "lowest pt", None, ["1e-3", "2", "-0.25"], ["nan", "inf", "1,5", "x"], float),
    "choice": ("5.02", "collision energy", ["5.02", "13.6"], ["13.6", "5.02"], ["7", "13.60", "5.02 "], str),
    "text": ("", "folder of the inputs", None, ["/data/my files", '{x} "q" \\ y'], [], str),
}
DEFAULTS = {kind: default for kind, (default, _, _, _, _, _) in KINDS.items()}


def declarations(language):
    lines = []
    for kind, (default, description, choices, _, _, _) in KINDS.items():
        if language == "py":
            lines.append(f"values[{kind!r}] = pm.Setting({kind!r}, {default!r}, {description!r}{'' if choices is None else f', {choices!r}'})")
        else:
            value = {True: "true", False: "false"}.get(default, f'"{default}"' if isinstance(default, str) else repr(default))
            choiceList = "" if choices is None else ", {" + ", ".join(f'"{c}"' for c in choices) + "}"
            lines.append(f'  std::cout << "{kind}\\t" << pm.Setting("{kind}", {value}, "{description}"{choiceList}) << "\\n";')
    return "\n".join(lines)


PY = """from SciRooPlot import *
pm = PlotManager("proj")
values = {}
%s
for kind, value in values.items():
    print(f"{kind}\\t{type(value).__name__}\\t{value!r}")
pm.SaveProject()
""" % declarations("py")
CPP = """#include "SciRooPlot/PlotManager.h"

#include <iomanip>
#include <iostream>

using namespace SciRooPlot;

int main()
{
  PlotManager pm("proj");
  std::cout << std::setprecision(17) << std::boolalpha;
%s
  return 0;
}
""" % declarations("cpp")
TYPES = {"switch": "bool", "integer": "int", "number": "float", "choice": "str", "text": "str"}


def values(out):
    """the values the Python program got, by kind; None if a type is wrong"""
    got = {}
    for line in out.stdout.splitlines():
        if line.count("\t") != 2:
            continue
        kind, typeName, value = line.split("\t")
        got[kind] = eval(value) if typeName == TYPES[kind] else None
    return got


config = config_folder("config")
folder = os.path.join(WORK, "program")
os.makedirs(folder)
program = os.path.join(folder, "def.py")
with open(program, "w") as f:
    f.write(PY)
settingsFile = os.path.join(config, "proj", "settings.info")


def define(code=PY):
    return python(code, config=config)


out = define()
check("a project not registered yet: the defaults, in their types", out.rc == 0 and values(out) == DEFAULTS and not out.errors() and not out.warnings(), out.tail())
out = run(SRP, "set", "proj", "switch", "off", config=config)
check("srp set on a project not registered yet fails", out.rc == 1 and len(out.errors()) == 1 and "srp add proj" in out.errors()[0], out.tail())
run(SRP, "add", "proj", program, config=config)
out = run(SRP, "projects", "proj", config=config)
rows = [line.split(None, 1) for line in out.stdout.splitlines()[3:8]]
want = [["switch", "on    draw systematic uncertainties"], ["integer", "50"], ["number", "0.5   lowest pt"], ["choice", "5.02  collision energy"], ["text", "-     folder of the inputs"]]
check("srp projects lists the settings in the order declared, with value and description", rows == want, out.tail())
for kind, default in DEFAULTS.items():
    shown = {True: "on"}.get(default, str(default))
    out = run(SRP, "get", "proj", kind, config=config)
    check(f"srp get of a setting not set prints its default: {kind}", out.rc == 0 and out.stdout == shown + "\n", out.tail())

# every value accepted is what the program gets, every value refused changes nothing
todo = cases({"kind": list(KINDS)}, always=lambda c: True)
for case in todo:
    kind = case["kind"]
    _, _, _, accepted, refused, convert = KINDS[kind]
    for value in accepted:
        out = run(SRP, "set", "proj", kind, value, config=config)
        got = run(SRP, "get", "proj", kind, config=config).stdout
        ran = define()
        check(f"srp set {kind} {value!r}: srp get and the program have it", out.rc == 0 and not out.errors() and got == value + "\n" and values(ran)[kind] == convert(value), f"{out.tail()} {got!r} {ran.tail()}")
    for value in refused:
        before = config_files(config)
        out = run(SRP, "set", "proj", kind, value, config=config)
        check(f"srp set {kind} {value!r} is refused with one error, nothing changed", out.rc == 1 and len(out.errors()) == 1 and kind in out.errors()[0] and config_files(config) == before, out.tail())
    out = run(SRP, "unset", "proj", kind, config=config)
    ran = define()
    check(f"srp unset {kind}: back to the default", out.rc == 0 and values(ran)[kind] == DEFAULTS[kind], f"{out.tail()} {ran.tail()}")

before = config_files(config)
out = run(SRP, "set", "proj", "swich", "on", config=config)
check("an undeclared name is refused with one error listing the settings", out.rc == 1 and len(out.errors()) == 1 and "switch, integer, number, choice, text" in out.errors()[0] and config_files(config) == before, out.tail())
for args in (("get", "proj", "swich"), ("unset", "proj", "swich")):
    out = run(SRP, *args, config=config)
    check(f"srp {args[0]} of an undeclared name fails with one error", out.rc == 1 and len(out.errors()) == 1 and config_files(config) == before, out.tail())

run(SRP, "set", "proj", "switch", "off", config=config)
run(SRP, "set", "proj", "integer", "3", config=config)
out = run(SRP, "projects", "proj", config=config)
check("srp projects shows the default where the value differs", "off  (default on)" in out.stdout and "3  (default 50)" in out.stdout, out.tail())

# C++: the same declarations give the same values
if not (shutil.which("cmake") and (shutil.which("c++") or shutil.which("g++") or shutil.which("clang++"))):
    print("no cmake or C++ compiler: the C++ checks are left out")
else:
    built, build, command = cpp_program(os.path.join(WORK, "cpp"), CPP, "settings_cpp")
    check("the C++ declarations compile (no ambiguous overload)", built is not None, build.tail(1500))
    if built and shutil.which("clang++"):
        clang = syntax_check("clang++", command)
        check("the C++ declarations compile with clang", clang.rc == 0, clang.tail(1500))
    if built:
        out = run(built, config=config)
        cpp = dict(line.split("\t", 1) for line in out.stdout.splitlines())
        want = {"switch": "false", "integer": "3", "number": "0.5", "choice": "5.02", "text": ""}
        check("C++ gets the values set, in its types", out.rc == 0 and cpp == want, out.tail())

# a stored value the declaration does not allow (the program changed the type): warned, the default
out = define(PY.replace("pm.Setting('integer', 50", "pm.Setting('integer', True"))
check("a stored value the declaration does not allow: warned once, the default", out.rc == 0 and "integer\tbool\tTrue" in out.stdout and len(out.warnings()) == 1 and "'3'" in out.warnings()[0], out.tail())

# a setting no longer declared: its value is listed and can be unset, not set
out = define(PY.replace("values['integer'] = pm.Setting('integer', 50, '')\n", ""))
listed = run(SRP, "projects", "proj", config=config)
refused = run(SRP, "set", "proj", "integer", "4", config=config)
unset = run(SRP, "unset", "proj", "integer", config=config)
after = run(SRP, "projects", "proj", config=config)
check(
    "a value left from a setting no longer declared: listed, refused by set, removed by unset",
    out.rc == 0 and "integer" not in open(settingsFile).read() and "3  (not a setting of the program" in listed.stdout and refused.rc == 1 and unset.rc == 0 and "integer" not in after.stdout,
    listed.tail() + refused.tail() + unset.tail(),
)

# a program declaring none: the saved declarations are removed
out = python("from SciRooPlot import *\npm = PlotManager('proj')\npm.SaveProject()", config=config)
refused = run(SRP, "set", "proj", "switch", "on", config=config)
check("a program declaring no setting removes the saved ones", out.rc == 0 and not os.path.exists(settingsFile), out.tail())
check("srp set when the program declares none: one error saying so", refused.rc == 1 and len(refused.errors()) == 1 and "declares none" in refused.errors()[0], refused.tail())

# invalid declarations: one error each, the default, not saved; declared again
INVALID = {
    "reserved name": ('pm.Setting("program", 1)', 1),
    "illegal name": ('pm.Setting("a b", 1)', 1),
    "empty name": ('pm.Setting("", 1)', 1),
    "default not among the choices": ('pm.Setting("c", "x", "", ["a", "b"])', "x"),
    "default not a finite number": ('pm.Setting("n", float("nan"))', None),
}
for why, (call, default) in INVALID.items():
    out = python(f"import math\nfrom SciRooPlot import *\npm = PlotManager('proj')\nv = {call}\nprint('GOT', repr(v))\npm.SaveProject()", config=config)
    gotDefault = f"GOT {default!r}" in out.stdout if default is not None else "GOT nan" in out.stdout
    check(f"an invalid declaration: one error, the default, not saved: {why}", out.rc == 0 and len(out.errors()) == 1 and gotDefault and not os.path.exists(settingsFile), out.tail())
out = python("from SciRooPlot import *\npm = PlotManager('proj')\npm.Setting('s', 1)\npm.Setting('s', 1)\npm.SaveProject()", config=config)
check("a setting declared again alike: no message", out.rc == 0 and not out.warnings() and not out.errors(), out.tail())
out = python("from SciRooPlot import *\npm = PlotManager('proj')\npm.Setting('s', 1)\nprint('GOT', pm.Setting('s', 2))\npm.SaveProject()", config=config)
check("a setting declared again otherwise: one warning, the last declaration saved", out.rc == 0 and len(out.warnings()) == 1 and "GOT 2" in out.stdout and "default 2" in open(settingsFile).read(), out.tail())

# without a project: the defaults, no message
out = python("from SciRooPlot import *\npm = PlotManager()\nprint('GOT', pm.Setting('s', True), pm.Setting('t', 'x', 'd', ['x', 'y']))", config=config)
check("a PlotManager without project: the defaults, no message", out.rc == 0 and "GOT True x" in out.stdout and not out.errors() and not out.warnings(), out.tail())
finish()
