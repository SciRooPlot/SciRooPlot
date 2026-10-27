"""Cuts the example programs into one snippet per plot: {"group/name": {"py": code, "cpp": code}}.
Usage: extract_examples.py <DefinePlots.py> <DefinePlots.cpp> > examples.json"""
import json
import re
import sys
import textwrap

NAME = re.compile(r'Plot(?:\s+\w+)?\s*\(\s*(?:\w+\s*,\s*)?"([^"]+)"\s*,\s*"([^"]+)"')


def snippets(path, lang):
    lines = open(path).read().splitlines()
    out, seg = {}, []
    for line in lines:
        s = line.strip()
        if lang == "py" and (s.startswith(("def ", '"""')) or (seg and seg[-1].strip().startswith('"""') and not s)):
            seg = []
            continue
        if lang == "cpp" and (re.match(r"^(void|int|Plot|auto)\b.*\)\s*\{?$", line) or s in ("{", "}")):
            if s != "}" or not seg:
                seg = []
            continue
        seg.append(line)
        if "pm.AddPlot(" in s:
            code = "\n".join(seg)
            m = NAME.search(code)
            if m:
                body = [ln for ln in seg if not re.match(r"^\s*[{}]\s*$", ln)]
                while body and not body[0].strip():
                    body.pop(0)
                out[f"{m.group(2)}/{m.group(1)}"] = textwrap.dedent("\n".join(body)).rstrip()
            seg = []
    return out


py = snippets(sys.argv[1], "py")
cpp = snippets(sys.argv[2], "cpp")
print(json.dumps({k: {"py": py[k], "cpp": cpp.get(k, "")} for k in py}))
