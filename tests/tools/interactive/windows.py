"""The windows of modes show and spread, driven like a user on a virtual display with a window manager (Xvfb and openbox, keys and
window actions by xdotool and wmctrl): browsing with s and a, d moving the shown plot into a window of its own (in front, the browsing
window going on with the next plot), q closing one window, x all, the window manager closing or minimizing windows, and the keys typed
in the terminal. Prints PASS/FAIL per step; screenshots of the screen after some steps are in the work folder (ImageMagick's import).
Not run by CTest: timing-dependent (a slow machine needs --wait 2) and it needs the tools below.
Needs: Xvfb, openbox, xdotool, wmctrl (apt-get install xvfb openbox xdotool wmctrl imagemagick).

usage: SRP_PREFIX=<build or install> SRP_TESTS_WORK=<folder> PYTHONPATH=tests python3 tests/tools/interactive/windows.py [--wait 1]
"""

import os
import pty
import random
import select
import shutil
import subprocess
import sys
import time

wait = float(sys.argv[sys.argv.index("--wait") + 1]) if "--wait" in sys.argv else 1.0
import srptest as S  # noqa: E402
from srptest import check, finish  # noqa: E402

missingTools = [tool for tool in ("Xvfb", "openbox", "xdotool", "wmctrl") if not shutil.which(tool)]
if missingTools:
    S.skip(f"needs {', '.join(missingTools)}")
number = random.randrange(100, 30000)
server = subprocess.Popen(["Xvfb", f":{number}", "-screen", "0", "1920x1080x24", "-nolisten", "tcp"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
os.environ["DISPLAY"] = f":{number}"
time.sleep(1)
manager = subprocess.Popen(["openbox", "--sm-disable"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)  # without --sm-disable it fails
time.sleep(2)

p = S.Project("windows")
p.inputs()
PLOTS = ("p1", "p2", "p3", "p4", "p5")
for name in PLOTS:
    plot = S.S.Plot(name, "g", "1d")
    plot[1].AddData("hist", "inputs", name)
    plot[1].AddLegend()
    p.pm.AddPlot(plot)
p.plot()  # saves and selects the project


def sh(command):
    return subprocess.run(["bash", "-c", command], capture_output=True, text=True, check=False).stdout


def windows():
    """the titles of the windows on screen (without the window manager's)"""
    return {line.split(None, 3)[3] for line in sh("wmctrl -l").splitlines() if len(line.split(None, 3)) == 4 and "Openbox" not in line}


def active():
    wid = sh("xdotool getactivewindow 2>/dev/null").strip()
    return next((line.split(None, 3)[3] for line in sh("wmctrl -l").splitlines() if wid and int(line.split()[0], 16) == int(wid)), None)


def key(title, keys):
    """a key typed into a window: ROOT takes keys only with its window active and the pointer inside it"""
    wid = next((line.split()[0] for line in sh("wmctrl -l").splitlines() if line.split(None, 3)[3:] == [title]), None)
    if wid is None:
        return False
    sh(f"xdotool windowactivate --sync {wid}; eval $(xdotool getwindowgeometry --shell {wid}); xdotool mousemove $((X + WIDTH / 2)) $((Y + HEIGHT / 2))")
    time.sleep(0.3)
    sh(f"xdotool key {keys}")
    time.sleep(2 * wait)
    return True


def screenshot(name):
    if shutil.which("import"):
        sh(f'import -window root "{p.path(name)}.png"')


def titles(*names):
    return {f"{name}:g" for name in names}


def start(mode):
    log = open(p.path(f"{mode}.log"), "w")
    proc = subprocess.Popen([S.PLOT, "g", ".+", mode], stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
    time.sleep(8 * wait)
    return proc, log


def ended(proc, seconds=10):
    try:
        return proc.wait(seconds)
    except subprocess.TimeoutExpired:
        proc.kill()
        return None


def step(what, expected, front=None):
    shown = windows()
    check(f"{what}: windows {sorted(expected)}", shown == expected, f"shown {sorted(shown)}")
    if front:
        check(f"{what}: {front} in front", active() == front, f"active {active()}")


# show: browsing, detaching, closing one by one
proc, log = start("show")
step("show starts with the first plot", titles("p1"))
key("p1:g", "d")
step("d: the plot in a window of its own, the browsing window on the next", titles("p1", "p2"), front="p1:g")
key("p1:g", "s")
step("s in the window of its own: nothing changes (browsing keys only in the browsing window)", titles("p1", "p2"))
key("p2:g", "d")
step("d again: the second plot in a window of its own too", titles("p1", "p2", "p3"), front="p2:g")
key("p3:g", "s")
step("s in the browsing window: the next plot", titles("p1", "p2", "p4"))
key("p4:g", "a")
step("a: the plot before (plots in windows of their own are not browsed)", titles("p1", "p2", "p3"))
screenshot("show")
key("p1:g", "q")
step("q in a window of its own closes only it", titles("p2", "p3"))
key("p3:g", "q")
step("q in the browsing window ends browsing, the window of its own stays", titles("p2"))
key("p2:g", "q")
step("q in the last window", set())
check("show ends normally once every window is closed", ended(proc) == 0, f"exit status {proc.returncode}")
log.close()
text = open(p.path("show.log")).read()
check("show waits for the windows of their own after browsing ends, and says so once", text.count("Waiting until all windows are closed") == 1, text[-400:])

proc, log = start("show")
key("p1:g", "d")
key("p2:g", "x")
step("x closes every window", set())
check("show ends normally after x", ended(proc) == 0, f"exit status {proc.returncode}")
log.close()
check("no waiting message when no window is left", "Waiting until all windows are closed" not in open(p.path("show.log")).read())

# spread: every plot in a window, closed by the window manager, by q, minimized, by x
proc, log = start("spread")
step("spread opens a window per plot", titles(*PLOTS))
screenshot("spread")
sh("xdotool windowminimize $(wmctrl -l | awk '$NF == \"p5:g\" {print $1}')")
sh("wmctrl -c p1:g")
time.sleep(wait)
key("p2:g", "q")
step("closed by the window manager and by q (one minimized)", titles("p3", "p4", "p5"))
key("p3:g", "x")
step("x closes every window, also a minimized one", set())
check("spread ends normally", ended(proc) == 0, f"exit status {proc.returncode}")
log.close()

# keys typed in the terminal: plot in the foreground of a terminal
pid, fd = pty.fork()
if pid == 0:
    os.execvp("bash", ["bash", "-c", f'"{S.PLOT}" g ".+" show; echo "EXIT $?"; stty -a'])
output = b""


def pump(seconds):
    global output
    end = time.time() + seconds
    while time.time() < end:
        if select.select([fd], [], [], 0.1)[0]:
            try:
                output += os.read(fd, 4096)
            except OSError:
                return


pump(8 * wait)
for typed, expected, what in (("s", titles("p2"), "s in the terminal: the next plot"), ("a", titles("p1"), "a in the terminal: the plot before"), ("d", titles("p1", "p2"), "d in the terminal: the plot in a window of its own")):
    os.write(fd, typed.encode())
    pump(3 * wait)
    step(what, expected)
os.write(fd, b"x")
pump(4 * wait)
step("x in the terminal closes every window", set())
text = output.decode(errors="replace")
check("plot ends normally, the terminal set back (canonical mode, echo)", "EXIT 0" in text and " icanon" in text and " echo " in text, text[-600:])
manager.terminate()
server.terminate()
finish()
