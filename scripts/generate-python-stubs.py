#!/usr/bin/env python3
"""Generates the type stubs of the Python module (python/SciRooPlot-stubs) with pybind11-stubgen.

The stubs give IDEs and type checkers the classes, methods, argument names and return types of the compiled module.
They are committed to the repository and installed next to the module, so users need no extra package;
the CI regenerates them and fails if they differ from the committed ones.

usage: scripts/generate-python-stubs.py [--check]
Needs pybind11-stubgen (pip install pybind11-stubgen) and the built module importable (source env.sh of the build or install).
"""

import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "python/SciRooPlot-stubs"
FIXED_SIZE = re.compile(r"typing\.Annotated\[(list\[[\w.]+\]), pybind11_stubgen\.typing_ext\.FixedSize\(\d+\)\]")
DATA_ARGUMENT = re.compile(r"(\w+: )Data(?=[,)])")


def generate(target: Path) -> dict[str, str] | None:
    # --exit-code: a signature with a raw C++ type or an unprintable default value fails instead of ending up as '...' in the stubs
    command = [sys.executable, "-m", "pybind11_stubgen", "SciRooPlot", "--root-suffix=-stubs", "--enum-class-locations=.+:SciRooPlot", "--exit-code", "-o", str(target)]
    if subprocess.run(command).returncode != 0:
        return None
    stubs = {}
    for file in sorted((target / "SciRooPlot-stubs").rglob("*.pyi")):
        # std::array arguments are annotated with a class of pybind11-stubgen itself: keep the plain list type, so the stubs import nothing extra
        text = FIXED_SIZE.sub(r"\1", file.read_text()).replace("import pybind11_stubgen.typing_ext\n", "")
        # a Data argument also takes a [name, dataSource] list (py::implicitly_convertible), which the signatures do not show
        text = DATA_ARGUMENT.sub(r"\1Data | list[str]", text)
        # trailing whitespace from docstrings would be stripped by the pre-commit hooks
        lines = [line.rstrip() for line in text.splitlines()]
        stubs[str(file.relative_to(target / "SciRooPlot-stubs"))] = "\n".join(lines).strip("\n") + "\n"
    return stubs


def main() -> int:
    check = "--check" in sys.argv[1:]
    with tempfile.TemporaryDirectory() as tmp:
        stubs = generate(Path(tmp))
    if stubs is None:
        print("pybind11-stubgen failed: fix the signatures it reports in python/PythonBindings.cpp")
        return 1
    committed = {str(f.relative_to(OUTPUT)): f.read_text() for f in sorted(OUTPUT.rglob("*.pyi"))} if OUTPUT.exists() else {}
    if check:
        if stubs == committed:
            return 0
        changed = sorted(name for name in stubs.keys() | committed.keys() if stubs.get(name) != committed.get(name))
        print(f"Python stubs are outdated ({', '.join(changed)}): run scripts/generate-python-stubs.py and commit the result")
        return 1
    for name in committed.keys() - stubs.keys():
        (OUTPUT / name).unlink()
    for name, text in stubs.items():
        (OUTPUT / name).parent.mkdir(parents=True, exist_ok=True)
        (OUTPUT / name).write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
