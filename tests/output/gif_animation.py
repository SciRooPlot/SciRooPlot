"""Mode gif makes one animation of the selected plots that can be made, in the order they were defined: gif shows every frame 50
centiseconds and plays the animation once, gif+<cs> shows them cs centiseconds, gif+<cs>++ loops endlessly, gif+<cs>++<n> repeats it n
times (ROOT's spelling on the last frame); other spellings are refused. The repetition is right also when the last selected plots cannot be
made (missing data, missing base plot, failing while painted); a gif whose frames all fail is not reported as saved. The file is <group>.gif next to the folder of the deepest group all frames are in, animation.gif
in the output directory if they have none. Checked in the gif file itself (screen size, the size and delay of every frame, the loop count),
for frames of every data kind of srptest (each frame the size of its canvas, so the order shows); the canvases from mode file.
The gif has the size of its first frame (ROOT's): frames of another canvas size give one warning per gif, naming the first such plot."""

import os

from srptest import DATA_KINDS, Project, S, add_data, base_of, check, data_name, finish, gif_info

p = Project("gif_animation")
p.inputs()
KINDS = tuple(DATA_KINDS)


def add(name, group, kind="hist", base=None):
    plot = S.Plot(name, group, base or base_of(kind))
    add_data(plot[1], kind)
    p.pm.AddPlot(plot)


for i, kind in enumerate(KINDS):  # the base plots 1d and 2d have canvases of different widths
    add(f"kind{i:02d}", "kinds", kind)
for i in range(3):
    add(f"frame{i}", "modes")
plot = S.Plot("lastMissing", "modes", "1d")
plot[1].AddData("no_such_data", "inputs")
p.pm.AddPlot(plot)
plot = S.Plot("lastNoBase", "modes", "no_such_base")
add_data(plot[1], "hist")
p.pm.AddPlot(plot)
# a frame that fails only while it is painted (a 2d histogram divided by a 1d one), as the last one and alone
for name, group in (("lastPaintFails", "modes"), ("alone", "failing")):
    plot = S.Plot(name, group, "2d")
    plot[1].AddRatio(S.Data(data_name("hist2d"), "inputs"), S.Data(data_name("hist"), "inputs"))
    p.pm.AddPlot(plot)
# the names of the gif: groups of the frames -> file
GROUPS = {
    "deep": (["anim/rot", "anim/rot", "anim/rot"], "anim/rot.gif"),
    "common": (["anim/a", "anim/b/c"], "anim.gif"),
    "nested": (["anim/x", "anim/x/y"], "anim/x.gif"),
    "none": (["first", "second"], "animation.gif"),
    "single": (["solo"], "solo.gif"),
}
for scenario, (groups, _) in GROUPS.items():
    for i, group in enumerate(groups):
        add(f"{scenario}{i}", group)

reference = p.plot()  # mode file: the canvas sizes
sizes = {}
for i in range(len(KINDS)):
    canvas = reference.canvas(f"kind{i:02d}", "kinds")
    sizes[i] = (canvas.GetWw(), canvas.GetWh()) if canvas else None


def gifs():
    """every gif written, by path relative to the output directory"""
    found = {}
    for folder, _, names in os.walk(p.path("out")):
        for name in names:
            if name.endswith(".gif"):
                found[os.path.relpath(os.path.join(folder, name), p.path("out"))] = gif_info(os.path.join(folder, name))
    return found


def size_warnings(run):
    return [line for line in run.warnings() if "differ in size" in line]


# every data kind as frames: one frame per plot, in the order defined
run = p.plot("kinds", "kind.*", "gif")
written = gifs()
info = written.get("kinds.gif")
want = [sizes[i] for i in range(len(KINDS))]
check("frames of every data kind: one gif, no errors", list(written) == ["kinds.gif"] and info is not None and not run.errors(), f"{list(written)}, {run.errors()[:2]}")
if info:
    check("frames of every data kind: one frame per plot, in the order defined, each the size of its canvas", info["frames"] == want, f"{info['frames']}, expected {want}")
    check("frames of every data kind: the gif has the size of the first frame", info["size"] == want[0], f"{info['size']}, first frame {want[0]}")
other = next((i for i, size in enumerate(want) if size != want[0]), None)
warned = size_warnings(run)
check(
    "frames of different sizes: one warning, naming the first plot of another size and both sizes",
    other is not None and len(warned) == 1 and f"plot kind{other:02d} " in warned[0] and "{}x{}".format(*want[other]) in warned[0] and "{}x{}".format(*want[0]) in warned[0],
    f"{warned}",
)

# the modes: delay of every frame and repetition, also when the last selected plots cannot be made
MODES = {"gif": (50, None), "gif+20": (20, None), "gif+7++": (7, 0), "gif+20++3": (20, 3), "gif+1++1": (1, 1)}
for selection, what in (("frame.*", "plots that can all be made"), ("frame.*|last.*", "the last three selected plots cannot be made")):
    for mode, (delay, loops) in MODES.items():
        run = p.plot("modes", selection, mode)
        info = gifs().get("modes.gif")
        detail = f"{info}" if info else f"no modes.gif: {sorted(gifs())}, {run.errors()[:2]}"
        good = info is not None and info["delays"] == [delay] * 3 and len(info["frames"]) == 3 and info["loops"] == loops and not size_warnings(run)
        check(f"{mode}, {what}: 3 frames of {delay} cs, {'played once' if loops is None else ('looping' if loops == 0 else f'repeated {loops} times')}", good, detail)
run = p.plot("failing", "alone", "gif")
check("a single frame that fails while painted: no gif, none reported as saved", not gifs() and "Saved gif" not in run.log, f"{sorted(gifs())}")
for mode in ("gif++20", "gif+20+", "gif+", "gif+20++x", "gif20", "gif+-5", "gif+20+++", "gif+ 20", "GIF"):
    run = p.plot("modes", "frame.*", mode)
    check(f"{mode!r} refused: an error, no gif", not gifs() and any("Invalid plot mode" in line for line in run.lines("ERR")), f"{sorted(gifs())}, {run.lines('ERR')[:1]}")

# the name and place of the gif
for scenario, (groups, path) in GROUPS.items():
    run = p.plot(".+", f"{scenario}[0-9]", "gif+10")
    written = gifs()
    frames = len(written[path]["frames"]) if written.get(path) else 0
    check(f"frames of groups {', '.join(groups)}: {path}", list(written) == [path] and frames == len(groups) and not run.errors(), f"{ {k: len(v['frames']) for k, v in written.items()} }, {run.errors()[:1]}")
finish()
