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
messages, seen = [], set()
for path in sorted(glob.glob(f"{ROOT}/src/**/*.cpp", recursive=True) + glob.glob(f"{ROOT}/python/*.cpp")):
    source = open(path).read()
    for m in CALL.finditer(source):
        text = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))).replace('\\"', '"').replace("\\n", " ")
        text = text.replace("{{", "\x01").replace("}}", "\x02")
        text = re.sub(r"\{[^{}]*\}", "…", text).replace("\x01", "{").replace("\x02", "}").strip()
        if text in ("…", "") or text in seen:
            continue
        seen.add(text)
        messages.append({"k": KIND[m.group(1)], "a": AREA.get(path.split("/")[-1], "Other"), "t": text})
# the report of a plot that could not be made is assembled from pieces
messages.append({"k": "error", "a": "Data, files and plotting", "t": "Plot … from group … could not be created. - missing …"})
json.dump(messages, sys.stdout, ensure_ascii=False, separators=(",", ":"))
