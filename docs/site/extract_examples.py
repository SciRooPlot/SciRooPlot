"""Cuts the example programs into one snippet per plot: {"group/name": {"py": code, "cpp": code}}; an animation (a loop of frames
in a subgroup of animations/) is one snippet under the name of its group, with "gif": the mode that makes it.
Usage: extract_examples.py <DefinePlots.py> <DefinePlots.cpp> > examples.json"""
import json
import re
import sys
import textwrap

NAME = re.compile(r'Plot(?:\s+\w+)?\s*\(\s*(?:\w+\s*,\s*)?"([^"]+)"\s*,\s*"([^"]+)"')
# an animation: frames named in a loop, in a subgroup of animations/, with the command making the gif in a comment
ANIM = re.compile(r'Plot(?:\s+\w+)?\s*\([^;\n]*?,\s*"(animations/[^"]+)"')
GIF = re.compile(r"plot (animations/\S+) '\.\+' (gif\+\d+)")


def snippets(path, lang):
    lines = open(path).read().splitlines()
    out, seg = {}, []
    for line in lines:
        s = line.strip()
        if lang == "py" and (s.startswith(("def ", '"""')) or (seg and seg[-1].strip().startswith('"""') and not s)):
            seg = []
            continue
        if lang == "cpp" and (re.match(r"^(void|int|Plot|auto)\b.*\)\s*\{?$", line) or (s in ("{", "}") and not seg)):
            seg = []
            continue
        seg.append(line)
        if "pm.AddPlot(" in s:
            code = "\n".join(seg)
            anim, m = ANIM.search(code), NAME.search(code)
            key = anim.group(1) if anim else f"{m.group(2)}/{m.group(1)}" if m else None
            if key:
                body = list(seg)
                while body and (not body[0].strip() or body[0].lstrip().startswith("}")):  # the end of the block before
                    body.pop(0)
                outer = 0  # indentation of the block around the plot (C++): its own braces are not shown
                if len(body) > 1 and re.match(r"^\s*\{\s*//", body[0]):  # the opening of the block: its comment stays, indented as the block
                    outer = len(body[0]) - len(body[0].lstrip())
                    body[0] = body[1][: len(body[1]) - len(body[1].lstrip())] + body[0][body[0].index("//"):]
                body = [ln for ln in body if not (re.match(r"^\s*[{}]\s*$", ln) and len(ln) - len(ln.lstrip()) <= outer)]
                # a plot added inside a loop (an animation): the loop is closed as in the program
                depth = sum(ln.count("{") - ln.count("}") for ln in body) if lang == "cpp" else 0
                indent = len(body[-1]) - len(body[-1].lstrip()) if body else 0
                for i in range(depth):
                    body.append(" " * max(0, indent - 2 * (i + 1)) + "}")
                out[key] = textwrap.dedent("\n".join(body)).rstrip()
            seg = []
    return out


py = snippets(sys.argv[1], "py")
cpp = snippets(sys.argv[2], "cpp")

def entry(k):
    e = {"py": py[k], "cpp": cpp.get(k, "")}
    gif = GIF.search(py[k])
    if gif:
        e["gif"] = gif.group(2)  # the example is an animation: one gif of the frames of its group
    return e


print(json.dumps({k: entry(k) for k in py}))
