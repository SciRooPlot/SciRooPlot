"""The settings commands of srp (verbosity, color, plotmode, matchmode, matchcase, screenscale, bitmapscale, cache) store every value srp help
documents, shown by srp settings and kept in settings.info, and refuse anything else with exit status 1 and one error, changing nothing.
Without a value they print the current value, changing nothing. srp cache on keeps the copies in the cache folder of the config folder,
srp cache on <folder> in a folder SciRooPlot of its own inside <folder>; srp settings shows that folder.
Without a settings file the documented defaults hold (verbosity log). SCIROOPLOT_VERBOSITY sets the verbosity of one process: the stored
one stays as it is, an invalid value is reported once and ignored. A settings.info edited by hand with an unknown key or an invalid value is reported once
per entry and written again without it, keeping the valid entries."""

import os
import re

from srptest import SRP, WORK, cases, check, config_files, config_folder, describe, finish, python, run

MODES = ["show", "spread", "pdf", "eps", "ps", "svg", "png", "jpg", "gif", "gif+20", "gif+20++", "gif+20++3", "root", "html", "json", "xml", "macro", "file", "data", "print", "list"]
VALID = {
    "verbosity": ["debug", "hint", "log", "info", "warning", "error", "silent"],
    "color": ["bright", "dark", "off"],
    "plotmode": MODES,
    "matchmode": ["exact", "contains"],
    "matchcase": ["sensitive", "insensitive"],
    "screenscale": ["1", "1.5", "2", "0.25"],
    "bitmapscale": ["1", "1.5", "2", "0.25"],
    "cache": ["off", "on"],
}
INVALID = {
    "verbosity": ["loud"],
    "color": ["red"],
    "plotmode": ["nosuchmode", "gif+", "gif++", "pdf png"],
    "matchmode": ["regex"],
    "matchcase": ["maybe"],
    "screenscale": ["0", "-1", "abc", "nan", "inf", "1.5x"],
    "bitmapscale": ["0", "-2", "abc", "nan", "inf", "2x"],
    "cache": ["relative/folder", "yes", "/tmp/srp-cache"],
}
DEFAULTS = {"verbosity": "log", "plotmode": "show", "matchmode": "exact", "matchcase": "sensitive", "screenscale": "1", "bitmapscale": "1", "cache": "off", "current project": "none"}
# the value srp settings shows for a stored value (the cache: on with the folder of the copies)
SHOWN = {("cache", "on"): lambda config: f"on ({os.path.join(config, 'cache')})"}


def read_text(path):
    with open(path) as f:
        return f.read()


def shown(config):
    """the settings srp settings shows, by name"""
    rows = run(SRP, "settings", config=config).stdout.splitlines()[1:]
    pairs = [re.split(r"\s{2,}", row.strip(), maxsplit=1) for row in rows if row.strip()]
    return {pair[0]: pair[-1] for pair in pairs}


config = config_folder("config_defaults")
got = shown(config)
check("defaults without a settings file", all(got.get(k) == v for k, v in DEFAULTS.items()), f"{got}")
# SCIROOPLOT_VERBOSITY (the tests run with hint) sets the verbosity of one process: srp shows and keeps the stored one
run(SRP, "verbosity", "warning", config=config)
out = run(SRP, "verbosity", config=config, env={"SCIROOPLOT_VERBOSITY": "debug"})
check("SCIROOPLOT_VERBOSITY leaves the stored verbosity as it is", out.stdout.split()[-1:] == ["warning"] and "verbosity warning" in read_text(os.path.join(config, "settings.info")), out.tail())
out = run(SRP, "verbosity", config=config, env={"SCIROOPLOT_VERBOSITY": "loud"})
check("an invalid SCIROOPLOT_VERBOSITY is reported once and ignored", out.rc == 0 and out.text.count("Ignoring invalid SCIROOPLOT_VERBOSITY 'loud'") == 1, out.tail())

todo = cases({"setting": list(VALID), "value": sorted({v for values in list(VALID.values()) + list(INVALID.values()) for v in values})}, valid=lambda c: c["value"] in VALID[c["setting"]] + INVALID[c["setting"]], always=lambda c: True)
for i, case in enumerate(todo):
    what = describe(case)
    config = config_folder(f"config{i}")
    run(SRP, "verbosity", "info", config=config)  # a settings file to compare
    before = config_files(config)
    out = run(SRP, case["setting"], case["value"], config=config)
    if case["value"] in VALID[case["setting"]]:
        stored = read_text(os.path.join(config, "settings.info")).splitlines()
        check(
            f"stored and shown: {what}",
            out.rc == 0 and not out.errors() and shown(config).get(case["setting"]) == SHOWN.get((case["setting"], case["value"]), lambda c, value=case["value"]: value)(config) and (f"{case['setting']} {case['value']}" in stored or f'{case["setting"]} "{case["value"]}"' in stored),
            f"rc {out.rc}, shown {shown(config).get(case['setting'])}: {out.tail()}",
        )
    else:
        check(f"refused with one error, nothing changed: {what}", out.rc == 1 and len(out.errors()) == 1 and config_files(config) == before, f"rc {out.rc}: {out.tail()}")

# without a value: the current value, nothing changed
for setting in VALID:
    config = config_folder(f"show_{setting}")
    run(SRP, "verbosity", "info", config=config)
    before = config_files(config)
    out = run(SRP, setting, config=config)
    current = shown(config).get(setting)
    check(f"srp {setting} without a value prints its value, changing nothing", out.rc == 0 and not out.errors() and out.stdout.strip() == current and config_files(config) == before, f"rc {out.rc}, printed {out.stdout.strip()!r}, shown {current!r}: {out.tail()}")

# srp cache on <folder>: the copies in <folder>/SciRooPlot; anything else after on or off is refused
home = os.path.expanduser("~")
for i, (folder, stored_folder) in enumerate((("/a b/c", "/a b/c"), ("~/srp-cache-test", os.path.join(home, "srp-cache-test")), ("/tmp//x/../y", "/tmp/y"))):
    config = config_folder(f"cache_folder{i}")
    out = run(SRP, "cache", "on", folder, config=config)
    stored = read_text(os.path.join(config, "settings.info")).splitlines() if out.rc == 0 else []
    want = f"on ({os.path.join(stored_folder, 'SciRooPlot')})"
    check(f"srp cache on {folder}: stored, shown with the folder of the copies", out.rc == 0 and not out.errors() and shown(config).get("cache") == want and f'cache "on {stored_folder}"' in stored, f"rc {out.rc}, shown {shown(config).get('cache')!r}, stored {stored}: {out.tail()}")
    off = run(SRP, "cache", "off", config=config)
    check(f"srp cache off after srp cache on {folder}", off.rc == 0 and shown(config).get("cache") == "off", f"rc {off.rc}, shown {shown(config).get('cache')!r}: {off.tail()}")
for args in (("on", "relative/folder"), ("off", "/tmp/x"), ("on", "/a", "/b")):
    config = config_folder(f"cache_bad{len(os.listdir(WORK))}")
    run(SRP, "verbosity", "info", config=config)
    before = config_files(config)
    out = run(SRP, "cache", *args, config=config)
    check(f"srp cache {' '.join(args)} refused with one error, nothing changed", out.rc == 1 and len(out.errors()) == 1 and config_files(config) == before, f"rc {out.rc}: {out.tail()}")
# a settings file edited by hand
config = config_folder("config_edited")
with open(os.path.join(config, "settings.info"), "w") as f:
    f.write("verbosity loud\nfoo bar\nmatchmode contains\nbitmapscale -3\n")
out = run(SRP, "settings", config=config)
reports = [line for line in out.text.splitlines() if "settings.info" in line]
check("an unknown key and invalid values in settings.info are reported once each", len(reports) == 3 and all(any(w in line for line in reports) for w in ("foo", "loud", "-3")), out.tail())
check("the valid entries of an edited settings.info are kept", shown(config).get("matchmode") == "contains", f"{shown(config)}")
again = run(SRP, "settings", config=config)
text = read_text(os.path.join(config, "settings.info"))
check("an edited settings.info is written again without the bad entries", not [line for line in again.text.splitlines() if "settings.info" in line] and "foo" not in text and "loud" not in text and "-3" not in text and "matchmode contains" in text, again.tail() + text)
# a program setting a locale with a decimal comma (Python's locale.setlocale) reads and keeps the scales (std::stod read 1.5 as 1, and saved that)
comma = config_folder("comma")
run(SRP, "screenscale", "1.5", config=comma)
kept = python("import locale\ntry:\n    locale.setlocale(locale.LC_ALL, 'de_DE.UTF-8')\nexcept locale.Error:\n    print('NO_LOCALE')\nfrom SciRooPlot import *\npm = PlotManager('c')\npm.SaveProject()", config=comma)
if "NO_LOCALE" in kept.text:
    print("[SKIP] the locale de_DE.UTF-8 is not installed: the decimal-comma case is not run")
else:
    text = read_text(os.path.join(comma, "settings.info"))
    check("a program with a decimal-comma locale keeps the scale settings", "screenscale 1.5" in text, kept.tail() + text)
finish()
