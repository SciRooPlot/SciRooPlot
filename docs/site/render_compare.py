#!/usr/bin/env python3
"""Renders the SciRooPlot side of the "ROOT vs SciRooPlot" page from the page's own Python snippets (the CMP list of template.html),
so that the picture shown is the one the code beside it gives: each snippet is run on its own and its plot written to <out>/<id>_srp.png.
usage: render_compare.py <template.html> <out folder>   (with env.sh of the installed SciRooPlot sourced)"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

template, out = sys.argv[1], sys.argv[2]
page = open(template).read()
entries = page[page.index("const CMP = [") : page.index("const CATS")]
ids = re.findall(r'\{ id: "(\w+)"', entries)
commands = re.findall(r'cmd: "plot (\S+) (\S+) \w+"', entries)
snippets = re.findall(r"py: `(.*?)`,\n", entries, re.S)
if not ids or not len(ids) == len(commands) == len(snippets):
    sys.exit(f"render_compare.py: {len(ids)} entries, {len(commands)} commands, {len(snippets)} Python snippets in {template}")
work = tempfile.mkdtemp()
try:
    for entry, (group, name), snippet in zip(ids, commands, snippets):
        # the template literal escapes ${ and backslashes; instead of saving the project, the snippet draws its plot
        code = snippet.replace("\\${", "${").replace("\\\\", "\\")
        code = code.replace("pm.SaveProject()", f'pm.SetOutputDirectory("{work}/{entry}")\npm.GeneratePlots("png", "{name}", "{group}")')
        script = os.path.join(work, f"{entry}.py")
        with open(script, "w") as f:
            f.write("from SciRooPlot import *\n" + code + "\n")
        subprocess.run([sys.executable, "-W", "ignore", script], check=True)
        shutil.copy(os.path.join(work, entry, group, f"{name}.png"), os.path.join(out, f"{entry}_srp.png"))
finally:
    shutil.rmtree(work)
