"""The plot definition is the same written in C++ and in Python.

The cases are those of round_trip.py (every overload on every object it can be called on, in every variant of its values), each written
in both languages, and the C++ forms that Python has no spelling for (braced lists that could be ambiguous between overloads, the
type each call returns): a C++ program saves them as Python does. Reference: the file Python saves.
- the program compiles (checked with g++ and clang++, where there, -Wall -Wextra without warnings) and runs;
- every plot is the same as Python's, value by value, and each call gives the same messages (a thrown exception the same text).
Cases written only in Python (a list for a Data) are left out. The invalid values of definition/setter_checks.py are reported alike in
both languages (but values Python's conversion of the argument refuses, which C++ converts itself).
Covers: braced lists of projections, invalid values reported alike in both languages.
"""

import os
import shutil

import srptest as S

if not (shutil.which("cmake") and (shutil.which("c++") or shutil.which("g++") or shutil.which("clang++"))):
    S.skip("no cmake or C++ compiler")

cases, problems = S.setter_cases()
S.check("every overload of the interface has a case", not problems, "; ".join(problems))
cases = [case for case in cases if case["cpp"] is not None]
# C++ forms: braced lists for the projections of histograms and trees (Project/Profile share names; a single range also fits the weight
# or the profile string, two strings the iterator constructor of std::vector<uint8_t>), the Ratio returned by each call
BRACED = [
    ('Project([2], [(0, 10, 50)])', "Project({2}, {{0, 10, 50}})"),
    ('Profile([2], [(0, 10, 50)])', "Profile({2}, {{0, 10, 50}})"),
    ('Profile(3, [0, 1], [(2, 10, 50)])', "Profile(3, {0, 1}, {{2, 10, 50}})"),
    ('Profile(1, [0])', "Profile(1, {0})"),
    ('Project([2])', "Project({2})"),
    ('Project([2, 0], [(1, 20, 80), (3, 10, 40)])', "Project({2, 0}, {{1, 20, 80}, {3, 10, 40}})"),
    ('Project([0], [(2, 1., 5.)], True)', "Project({0}, {{2, 1., 5.}}, true)"),
    ('Project(["x"])', 'Project({"x"})'),
    ('Project(["x", "y"])', 'Project({"x", "y"})'),
    ('Project([("x", 50), ("y", 20)], "w")', 'Project({{"x", 50}, {"y", 20}}, "w")'),
    ('Profile(["x"], "y")', 'Profile({"x"}, "y")'),
]
for i, (py, cpp) in enumerate(BRACED):
    for host, expr in (("data", 'p[1].AddData("h", "s")'), ("ratio", 'p[1].AddRatio("h", "s", "h2", "s2").Denom()')):
        tail = ".SetCorrelated()" if host == "ratio" else ".SetColor(633)"  # SetCorrelated: only a Ratio has it
        cases.append(
            {"cls": "C++", "method": cpp, "params": (), "host": host, "variant": "braced", "why": "braced lists of projections", "py": f'p = Plot("{{name}}", "g", "1d")\n{expr}.{py}{tail}\npm.AddPlot(p)', "cpp": f'{{\n  Plot p("{{name}}", "g", "1d");\n  {expr}.{cpp}{tail};\n  pm.AddPlot(p);\n}}'}
        )


# invalid values (those of definition/setter_checks.py) on every setter where they are meant: each is reported alike in both languages;
# values Python's conversion refuses (a number out of the range of an integer type, a wrong type) are left out: C++ converts them itself
SKIPPED = ("PlotManager", "PanelLayout", "Panel", "BasePlotShape")  # as in setter_checks.py
for case in list(cases):
    if case["cls"] == "C++" or case["variant"] != "plain" or not case["meant"] or case["unset"] or case["host"] in ("statement", "base", "base_pad", "c") or case["cls"] in SKIPPED:
        continue
    py, cpp = S.SETTER_HOSTS[case["cls"]][case["host"]]
    for i, param in enumerate(case["params"]):
        for bad in S.setter_invalid(case["cls"], case["method"], param):
            if isinstance(bad, list) and len(bad) == 1:
                continue  # {x} in C++ is the scalar overload (Python's [x] the list)
            given = list(case["values"])
            given[i] = bad
            invalid = {**case, "variant": f"invalid {param}={S.literal(bad, 'py')}", "py": py.replace("{call}", S.call(case["method"], given, "py")), "cpp": cpp.replace("{call}", S.call(case["method"], given, "cpp"))}
            cases.append(invalid)


def label(case):
    variant = f" {case['variant']}" if case["variant"] != "plain" else ""
    return f"{case['cls']}.{case['method']}({', '.join(case['params'])}) on {case['host']}{variant}" + (f" [{case['why']}]" if case.get("why") else "")


pm = S.S.PlotManager("python")
namespace = dict(vars(S.S), pm=pm)
for i, case in enumerate(cases):
    case["name"] = f"c{i}"
    result, printed = S.captured(lambda case=case: exec(case["py"].replace("{name}", case["name"]), namespace))  # noqa: S102  the cases
    # what the call reports (a thrown exception as its message), as C++ prints it
    case["messages"] = [line.strip() for line in printed.splitlines() if line.strip()] + ([f"thrown: {result}"] if isinstance(result, Exception) else [])
    case["converted"] = isinstance(result, TypeError)  # refused by Python's conversion of the argument
cases = [case for case in cases if not (case["converted"] and case["variant"].startswith("invalid"))]
pyFile, cppFile = os.path.join(S.WORK, "python.info"), os.path.join(S.WORK, "cpp.info")
pm.SavePlots(file=pyFile)

# the C++ program: the cases in functions of 100 (the compiler needs less memory for many small functions than for one large one)
chunks = [cases[i : i + 100] for i in range(0, len(cases), 100)]
functions = []
for n, chunk in enumerate(chunks):
    body = "\n".join(f'std::cout << "CASE {case["name"]}" << std::endl;\ntry\n{case["cpp"].replace("{name}", case["name"])}\ncatch (const std::exception& ex) {{\n  std::cout << "thrown: " << ex.what() << std::endl;\n}}' for case in chunk)
    functions.append(f"void define{n}(PlotManager& pm)\n{{\n{body}\n}}\n")
source = f"""#include "SciRooPlot/PlotManager.h"

#include <iostream>
#include <limits>
#include <string>

using namespace SciRooPlot;
using Data = Plot::Pad::Data;
using Ratio = Plot::Pad::Ratio;
using LegendEntry = Plot::Pad::LegendEntry;
using StatsBox = Plot::Pad::StatsBox;

{chr(10).join(functions)}
int main()
{{
  PlotManager pm("cpp");
{chr(10).join(f"  define{n}(pm);" for n in range(len(chunks)))}
  pm.SavePlots(".+", ".+", "{cppFile}");
  return 0;
}}
"""
program, build, command = S.cpp_program(os.path.join(S.WORK, "cpp"), source)
S.check("the C++ program compiles", program is not None, build.tail(1500))
if program is None:
    S.finish()
for compiler in ("g++", "clang++"):
    if shutil.which(compiler) and command[0]:
        out = S.syntax_check(compiler, command)
        S.check(f"the C++ program compiles with {compiler} -Wall -Wextra without warnings", out.rc == 0 and "warning" not in out.text, out.tail(1500))
out = S.run("bash", "-c", f'"{program}" 2>&1')  # messages in the order they are written
S.check("the C++ program runs", out.rc == 0, out.tail())
cppMessages = {}
current = []  # lines before the first case belong to no call (a warning of the system's fontconfig at the first text)
for line in out.text.splitlines():
    if line.startswith("CASE "):
        current = cppMessages.setdefault(line[5:].strip(), [])
    elif line.strip():
        current.append(line.strip())

py = {key.split(":")[0]: values for key, values in S.info_plots(pyFile).items()}
cpp = {key.split(":")[0]: values for key, values in S.info_plots(cppFile).items()} if os.path.exists(cppFile) else {}
for case in cases:
    names = [n for n in py if n == case["name"] or n.startswith(case["name"] + "_")]
    diff = {}
    for name in names:
        a, b = py[name], cpp.get(name, {})
        diff.update({f"{name}/{k}": (a.get(k), b.get(k)) for k in set(a) | set(b) if a.get(k) != b.get(k)})
    thrown = any(m.startswith("thrown: ") for m in case["messages"])
    S.check(f"{label(case)}: C++ as Python", (bool(names) or thrown) and not diff, str(diff)[:400] if names or thrown else "no plot")
    messages = cppMessages.get(case["name"], [])
    S.check(f"{label(case)}: the same messages", messages == case["messages"], f"C++ {messages}, Python {case['messages']}"[:400])
S.check("C++ saves no plot Python does not", not set(cpp) - set(py), str(sorted(set(cpp) - set(py)))[:300])
S.finish()
