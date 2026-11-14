"""C++ and Python have the same interface.

Every public method of the headers (as the API reference lists them, generated from the headers) has a Python binding of the same name
and parameter names, unless the reference marks it C++ only, and every binding is such a method. Operators are compared by their number
of parameters (Python cannot name them). The default values are the same (an empty optional, list or text {} of C++ is None, [] or '' of Python;
objects made by a constructor are not compared). The differences by design are listed below with their reason.
Covers: the differences by design between the C++ and the Python interface (listed below).
"""

import math

import re

import srptest as S

OPERATORS = {"__call__": "operator()", "__getitem__": "operator[]", "__iadd__": "operator+="}
# bindings without a method in the reference
PYTHON_ONLY = {
    ("*", "Print", ()): "the reference leaves Print out",
    ("Data", "GetDataSource", ()): "the reference lists only the getters giving access to parts of the definition",
    ("Data", "__init__", ("nameAndSource",)): "a list [name, data source] wherever a Data is taken",
    ("BasePlotShape", "__init__", ()): "the implicit constructor of C++",
    ("Plot", "operator[]", 1): "described in the remark of the class",
    ("Pad", "operator[]", 1): "described in the remark of the class",
    ("Plot", "operator+=", 1): "described in the remark of the class",
    ("Pad", "operator+=", 1): "described in the remark of the class",
}
# methods of the reference without a binding
CPP_ONLY = {
    ("PlotManager", "AddDataSource", ("dataSource", "inputs", "replace")): "one binding takes a path, a list of them, an object or a list of objects",
    ("PlotManager", "AddDataSource", ("dataSource", "input", "replace")): "one binding takes a path, a list of them, an object or a list of objects",
}


def key(cls, method, params):
    method = OPERATORS.get(method, method)
    return (cls, method, len(params) if method.startswith("operator") else tuple(params))


cppInterface, pyInterface = S.cpp_interface(), S.python_interface()
cpp = {key(cls, method, params) for cls, method, params, only, _ in cppInterface if not only}
py = {key(cls, method, [p[0] for p in params]) for cls, method, params in pyInterface}
for cls, method, params in sorted(cpp - py, key=str):
    reason = CPP_ONLY.get((cls, method, params))
    S.check(f"{cls}::{method}({params}) has a binding" + (f" (not needed: {reason})" if reason else ""), bool(reason), "no binding of these parameter names")
for cls, method, params in sorted(py - cpp, key=str):
    reason = PYTHON_ONLY.get((cls, method, params)) or PYTHON_ONLY.get(("*", method, params))
    S.check(f"Python {cls}.{method}({params}) is a method of the headers" + (f" (by design: {reason})" if reason else ""), bool(reason), "no method of these parameter names")
S.check("every method of the headers is compared", len(cpp & py) > 400, f"{len(cpp & py)} in both")


def default(text, lang):
    """a default value in a form comparable between the languages (None: not compared)"""
    text = text.strip()
    if lang == "py":
        text = re.sub(r"^<\w+\.(\w+): -?\d+>$", r"\1", text)  # an enumeration value
        if text.startswith(("'", '"')):
            return ("text", text[1:-1])
        return {"True": True, "False": False, "None": "empty", "[]": "empty"}.get(text, text)
    if text.startswith('"'):
        return ("text", text[1:-1])
    if text.endswith(")") or text.startswith("{{"):
        return None  # an object made by a constructor
    return {"true": True, "false": False, "{}": "empty"}.get(text, re.sub(r"(?<=\d)\.?f$|(?<=\d)\.$", "", text))


def same(a, b):
    if {a, b} <= {"empty", ("text", "")}:
        return True  # {} of C++ is also an empty text
    try:
        return math.isclose(float(a), float(b))
    except (TypeError, ValueError):
        return a == b


pyDefaults = {key(cls, method, [p[0] for p in params]): [p[2] for p in params] for cls, method, params in pyInterface}
for cls, method, params, only, defaults in cppInterface:
    k = key(cls, method, params)
    if only or k not in pyDefaults:
        continue
    for name, cppDefault, pyDefault in zip(params, defaults, pyDefaults[k]):
        a = default(cppDefault, "cpp") if cppDefault else None
        b = default(pyDefault, "py") if pyDefault else None
        if cppDefault and a is None:
            continue
        S.check(f"{cls}::{method}: default of {name} the same", same(a, b), f"C++ {cppDefault}, Python {pyDefault}")
S.finish()
