"""Collects the errors and warnings SciRooPlot prints, from its sources, for the messages topic of the site.
Usage: extract_messages.py <repository> > messages.json"""
import glob
import json
import re
import sys

ROOT = sys.argv[1]
AREA = {"Config.cpp": "Projects and settings", "Paths.cpp": "Projects and settings", "PanelLayout.cpp": "Layouts",
        "Plot.cpp": "Plot definitions", "PythonBindings.cpp": "Plot definitions", "PlotManager.cpp": "Data, files and plotting",
        "PlotPainter.cpp": "Drawing"}
KIND = {"error": "error", "warning": "warning", "throw_invalid_argument": "error, stops the program"}
CALL = re.compile(r'logger::(error|warning|throw_invalid_argument)\s*\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)')
LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')
# settings the painter ignores: ReportIgnored(<setting>, <reason>), in the modifiers ignored(<modifier>, <reason>) ("<modifier> of <data>")
IGNORED = re.compile(r"(?<![\w:])(ReportIgnored|ignored)\(")


def shown(text):
    """a format string as shown on the site: the arguments as …"""
    text = text.replace('\\"', '"').replace("\\n", " ").replace("{{", "\x01").replace("}}", "\x02")
    return re.sub(r"\{[^{}]*\}", "…", text).replace("\x01", "{").replace("\x02", "}").strip()


def arguments(source, start):
    """the text of a call from its opening parenthesis to the closing one"""
    depth, i = 0, start
    while i < len(source):
        if source[i] == '"':
            i = LITERAL.match(source, i).end()
            continue
        depth += {"(": 1, ")": -1}.get(source[i], 0)
        if depth == 0:
            return source[start : i + 1]
        i += 1
    return ""


messages, seen = [], set()


def add(kind, path, text):
    if text in ("…", "") or text in seen:
        return
    seen.add(text)
    messages.append({"k": kind, "a": AREA.get(path.split("/")[-1], "Other"), "t": text})


for path in sorted(glob.glob(f"{ROOT}/src/**/*.cpp", recursive=True) + glob.glob(f"{ROOT}/python/*.cpp")):
    source = open(path).read()
    for m in CALL.finditer(source):
        add(KIND[m.group(1)], path, shown("".join(LITERAL.findall(m.group(2)))))
    for m in IGNORED.finditer(source):
        call = arguments(source, m.end() - 1)
        literals = LITERAL.findall(call)
        if len(literals) >= 2:  # (not the definitions, which pass on what they get)
            verb = " are ignored: " if call.rstrip(")").rstrip().endswith("true") else " is ignored: "
            add("warning", path, shown(literals[0] + (" of {}" if m.group(1) == "ignored" else "") + verb + literals[-1] + "."))
# the report of a plot that could not be made is assembled from pieces
messages.append({"k": "error", "a": "Data, files and plotting", "t": "Plot … from group … could not be created. - missing …"})
json.dump(messages, sys.stdout, ensure_ascii=False, separators=(",", ":"))
