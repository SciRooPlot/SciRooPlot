"""srp init-py, init-cpp, example-py and example-cpp (env.sh) make a registered, selected project in a new folder whose program has
run: plot makes its plots without warnings or errors. They refuse, with a message and without registering or creating anything, an
existing project name (whose program stays where it is), an existing folder, @current, a name that is no project name and a missing name.
C++ projects need cmake and a compiler (left out without them); the examples run in the full test only (they plot every example), also
in every mode writing files, without warnings (but the one that pdf files lack images with ROOT before 6.42, and mode html refused there).
Covers: init over an existing project from another folder."""

import os
import shutil

from srptest import FILE_MODES, FULL, ROOT, SHELLS, SRP, cases, check, config_files, config_folder, describe, finish, run, shell

ROOT_642 = ROOT.gROOT.GetVersionInt() >= 64200

hasCpp = bool(shutil.which("cmake") and (shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")))
if not hasCpp:
    print("no cmake or C++ compiler: the C++ projects are left out")
KINDS = ["init-py", "init-cpp", "example-py", "example-cpp"]
todo = cases({"shell": list(SHELLS), "kind": KINDS}, valid=lambda c: (hasCpp or "cpp" not in c["kind"]) and (FULL or "example" not in c["kind"]), named=[{"shell": "bash", "kind": "init-py", "why": "init over an existing project"}])
for i, case in enumerate(todo):
    what = describe(case)
    config = config_folder(f"config{i}")
    folder = config_folder(f"cwd{i}")
    out = shell(f"srp {case['kind']} proj", case["shell"], config=config, cwd=folder, timeout=1800)
    program = run(SRP, "get", "proj", "program", config=config).stdout.strip()
    where = os.path.join(folder, "proj")
    check(f"makes a selected project in a new folder: {what}", out.rc == 0 and program.startswith(where + "/") and "@current proj" in open(os.path.join(config, "projects.info")).read() and os.path.isfile(os.path.join(config, "proj", "plots.info")), out.tail())
    made = shell("plot '.+' '.+' file", case["shell"], config=config, timeout=1800)
    check(f"plot makes its plots without warnings or errors: {what}", made.rc == 0 and not made.warnings() and not made.errors() and "Created plot" in made.text, made.tail())
    if case["kind"].startswith("example"):  # the example in every mode writing files (html needs ROOT 6.42, pdf images too: said once)
        for mode in FILE_MODES:
            made = shell(f"plot '.+' '.+' {mode}", case["shell"], config=config, timeout=1800)
            if mode == "html" and not ROOT_642:
                check(f"mode html refused with ROOT before 6.42: {what}", made.rc != 0 and len(made.errors()) == 1 and "needs ROOT 6.42" in made.errors()[0], made.tail())
                continue
            expected = 1 if mode == "pdf" and not ROOT_642 else 0
            said = [w for w in made.warnings() if not (expected and "Images are missing in the pdf files" in w)]
            check(f"the example in mode {mode}: no warnings or errors{' but the one about images in pdf' if expected else ''}: {what}", made.rc == 0 and not said and len(made.warnings()) == expected and not made.errors(), made.tail())

    # refusals: nothing registered, nothing created, the existing project unchanged
    other = config_folder(f"cwd{i}_other")
    os.makedirs(os.path.join(other, "taken"))
    before = config_files(config)
    for args, reason in ((("proj",), "already exists"), (("taken",), "already exists"), (("@current",), "@current"), (("a/b",), "illegal character"), ((), "No project name")):
        out = shell(f"srp {case['kind']} {' '.join(args)}; echo status $?", case["shell"], config=config, cwd=other)
        created = sorted(set(os.listdir(other)) - {"taken"})
        check(f"refuses {args[0] if args else 'no name'} ({reason}): {what}", "status 0" not in out.stdout and reason in out.stderr and config_files(config) == before and not created, f"created {created}: {out.tail()}")
finish()
