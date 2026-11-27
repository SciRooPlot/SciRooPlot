"""plot runs of one project at the same time and interrupted: two runs into one output folder at once (one writing Plots.root, one the
pictures of mode png) both end with exit status 0 and write all they make (every plot in Plots.root, every picture complete); a run whose
write of Plots.root is cut short leaves the Plots.root of the earlier run as it was; Ctrl-C (SIGINT
to the process group, as a terminal sends it) ends a long run within seconds, in mode file and png, and leaves the Plots.root of an earlier
run as it was (it is written at the end of a run); a Plots.root that is a symbolic link is written where it points, the link kept (also
a link to a file not written yet, through another link).
The UserData.root of a project is replaced only when the program saves the project, and the files a process writes the in-memory inputs
to are gone when it ends (also those of a process ended by a signal, once another one writes in-memory inputs).
SCIROOPLOT_OUTDIR sends the files of one process to another folder, the output folder of the project and its stored path untouched; one
naming a variable that is not set writes nothing.
Covers: two runs at once, a write of Plots.root cut short, Ctrl-C during a long run, a Plots.root linked elsewhere, in-memory inputs."""

import os
import pty
import select
import signal
import socket
import subprocess
import time

from srptest import PLOT, ROOT, Project, S, check, config_folder, finish, python

N = 300  # plots of the project: a run in mode file takes several seconds
p = Project("runs", bases=("1d",))
h = ROOT.TH1D("h", "", 50, 0.0, 1.0)
for b in range(1, 51):
    h.SetBinContent(b, b)
p.source("s", {"h": h})
for i in range(N):
    plot = S.Plot(f"p{i}", "g", "1d")
    plot[1].AddData("h", "s")
    p.pm.AddPlot(plot)
first = p.plot("g", "p0")  # registers and selects the project
check("the project plots", not first.errors() and first.canvas("p0", "g"), " | ".join(first.errors()[:3]))
out = p.path("out")
# SCIROOPLOT_OUTDIR: the files of one process go to that folder, the output folder of the project and its stored path stay as they were
elsewhere = config_folder("outdir_of_process")
moved = p.plot("g", "p1", mode="png", env={"SCIROOPLOT_OUTDIR": elsewhere})
storedOut = p.srp("get", "runs", "outdir").stdout.strip()
check(
    "SCIROOPLOT_OUTDIR: the pictures written there, none in the output folder of the project, its stored path unchanged",
    moved.rc == 0 and os.path.isfile(os.path.join(elsewhere, "g", "p1.png")) and not os.path.exists(os.path.join(out, "g", "p1.png")) and storedOut == out,
    f"stored {storedOut} | {moved.log[-300:]}",
)
# a variable in it that is not set: said, and nothing written (not into the output folder of the project either)
os.environ.pop("SRP_TEST_UNSET", None)
unexpanded = p.plot("g", "p2", mode="png", env={"SCIROOPLOT_OUTDIR": "$SRP_TEST_UNSET/x"})
check(
    "SCIROOPLOT_OUTDIR with a variable that is not set: said, no picture written anywhere",
    unexpanded.rc > 0 and "SRP_TEST_UNSET, which is not set" in unexpanded.log and not os.path.exists(os.path.join(out, "g", "p2.png")),
    f"exit status {unexpanded.rc} | {unexpanded.log[-300:]}",
)


def start(name, mode):
    """a plot run in the background, its output into a terminal of its own and in a process group of its own (to be interrupted as by Ctrl-C)"""
    master, slave = pty.openpty()
    proc = subprocess.Popen([PLOT, "g", name, mode], stdin=slave, stdout=slave, stderr=slave, close_fds=True, start_new_session=True)
    os.close(slave)
    return proc, master


def finished(proc, master, timeout):
    """waits for a run: (exit status or None if it is still running after timeout seconds, its output)"""
    chunks, deadline = [], time.time() + timeout
    while time.time() < deadline:
        ready, _, _ = select.select([master], [], [], max(deadline - time.time(), 0))
        if not ready:
            break
        try:
            chunk = os.read(master, 65536)
        except OSError:  # the terminal is closed once the program ended
            break
        if not chunk:
            break
        chunks.append(chunk)
    try:
        status = proc.wait(max(deadline - time.time(), 1))
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
        status = None
    os.close(master)
    return status, b"".join(chunks).decode(errors="replace")


def plots_in(path):
    """the plots in a Plots.root, or None if it cannot be read"""
    f = ROOT.TFile.Open(path)
    if not f or f.IsZombie():
        return None
    group = f.Get("g")
    names = sorted(key.GetName() for key in group.GetListOfKeys()) if group else []
    f.Close()
    return names


# two runs at once into one output folder
pictures = sorted(f"p{i}" for i in range(N) if str(i).startswith("1"))
runs = [start(".+", "file")]
time.sleep(1.0)
runs.append(start("p1.*", "png"))
(fileStatus, fileOutput), (pngStatus, pngOutput) = (finished(proc, master, 600) for proc, master in runs)
check("two runs at once into one output folder: both end with exit status 0", fileStatus == 0 and pngStatus == 0, f"{fileStatus} {pngStatus}: {fileOutput[-300:]} || {pngOutput[-300:]}")
found = plots_in(os.path.join(out, "Plots.root"))
check("two runs at once: the run in mode file wrote every plot into Plots.root", found == sorted(f"p{i}" for i in range(N)), f"{len(found or [])} plots")
written = sorted(f[:-4] for f in os.listdir(os.path.join(out, "g")) if f.endswith(".png")) if os.path.isdir(os.path.join(out, "g")) else []
broken = [name for name in written if not (img := ROOT.TImage.Open(os.path.join(out, "g", f"{name}.png"))) or img.GetWidth() == 0]
check("two runs at once: the run in mode png wrote every picture, each complete", written == pictures and not broken, f"{len(written)} of {len(pictures)}, unreadable {broken[:3]}")

# a run whose write of Plots.root is cut short (here by a file size limit, as by a full disk) leaves the Plots.root of the earlier run
size = os.path.getsize(os.path.join(out, "Plots.root"))
cut = subprocess.run(["bash", "-c", f"ulimit -f {size // 2048}; exec {PLOT} g '.+' file"], capture_output=True, text=True)
check("a write of Plots.root cut short: the run fails", cut.returncode != 0, f"exit status {cut.returncode}")
check("a write of Plots.root cut short: the Plots.root of the earlier run is left as it was", plots_in(os.path.join(out, "Plots.root")) == found and os.path.getsize(os.path.join(out, "Plots.root")) == size, f"{len(plots_in(os.path.join(out, 'Plots.root')) or [])} plots")
for leftover in [f for f in os.listdir(out) if f.endswith(".tmp")]:  # the temporary file of the run killed while writing
    os.remove(os.path.join(out, leftover))

# Ctrl-C during a long run
before = os.path.getmtime(os.path.join(out, "Plots.root"))
for mode in ("file", "png"):
    proc, master = start(".+", mode)
    time.sleep(3.0)
    os.killpg(proc.pid, signal.SIGINT)
    interrupted = time.time()
    status, output = finished(proc, master, 30)
    check(f"Ctrl-C during a run in mode {mode}: it ends within seconds", status is not None and time.time() - interrupted < 10, f"status {status} after {time.time() - interrupted:.1f} s: {output[-200:]}")
    check(f"Ctrl-C during a run in mode {mode}: the Plots.root of the earlier run is left as it was", os.path.getmtime(os.path.join(out, "Plots.root")) == before and plots_in(os.path.join(out, "Plots.root")) == found, "changed")

# a Plots.root that is a symbolic link: written where it points, the link kept
target = p.path("elsewhere", "Plots.root")
os.makedirs(os.path.dirname(target), exist_ok=True)
os.replace(os.path.join(out, "Plots.root"), target)
os.symlink(target, os.path.join(out, "Plots.root"))
linked = subprocess.run([PLOT, "g", "p0", "file"], capture_output=True, text=True)
check(
    "a Plots.root that is a link: written where it points, the link kept",
    linked.returncode == 0 and os.path.islink(os.path.join(out, "Plots.root")) and plots_in(target) == ["p0"] and not [f for f in os.listdir(os.path.dirname(target)) if f.endswith(".tmp")],
    f"exit status {linked.returncode}, link {os.path.islink(os.path.join(out, 'Plots.root'))}, target {plots_in(target)}",
)
# a link to a file not written yet (its folder exists), also through a second link: written where it points, the links kept
os.remove(os.path.join(out, "Plots.root"))
dangling = p.path("elsewhere", "later", "Plots.root")
os.makedirs(os.path.dirname(dangling))
os.symlink(os.path.relpath(dangling, p.path("elsewhere")), p.path("elsewhere", "hop.root"))
os.symlink(p.path("elsewhere", "hop.root"), os.path.join(out, "Plots.root"))
linked = subprocess.run([PLOT, "g", "p0", "file"], capture_output=True, text=True)
check(
    "a Plots.root that is a link to a file not written yet: written where it points, the links kept",
    linked.returncode == 0 and os.path.islink(os.path.join(out, "Plots.root")) and os.path.islink(p.path("elsewhere", "hop.root")) and plots_in(dangling) == ["p0"],
    f"exit status {linked.returncode}, link {os.path.islink(os.path.join(out, 'Plots.root'))}, target {plots_in(dangling) if os.path.exists(dangling) else 'missing'}",
)

# the in-memory inputs of a program: its project's UserData.root replaced only once the program saves the project (a second run of the
# program at the same time reads the stored data meanwhile), the files of the process gone when it ends, also those of an ended process
# of this host, but not those of another host sharing the config path (its processes cannot be seen from here)
config = config_folder("userdata_config")
deadPid = 4194304 + 1  # beyond the largest process id of Linux
leftover = os.path.join(config, f"UserData_{socket.gethostname()}_{deadPid}_123.root")
otherHost = os.path.join(config, f"UserData_other-host.example.org_{deadPid}_123.root")
program = f"""
import ROOT, sys
import SciRooPlot as S
content = float(sys.argv[1])
pm = S.PlotManager("mem")
h = ROOT.TH1D("h", "", 1, 0.0, 1.0)
h.SetBinContent(1, content)
pm.AddDataSource("s", [h])
def stored():
    import os
    if not os.path.exists({os.path.join(config, "mem", "UserData.root")!r}):
        return None
    f = ROOT.TFile.Open({os.path.join(config, "mem", "UserData.root")!r})
    value = f.Get("s/h").GetBinContent(1) if f and not f.IsZombie() and f.Get("s/h") else None
    if f:
        f.Close()
    return value
print("BEFORE", stored())
pm.SaveProject()
print("AFTER", stored())
"""
first = python(program.replace("sys.argv[1]", "'1'"), config=config)
open(leftover, "w").close()
open(otherHost, "w").close()
second = python(program.replace("sys.argv[1]", "'2'"), config=config)
check("in-memory inputs: UserData.root of the project is written by SaveProject", "AFTER 1.0" in first.stdout, first.tail())
check("in-memory inputs: a second run leaves UserData.root as it was until it saves the project", "BEFORE 1.0" in second.stdout and "AFTER 2.0" in second.stdout, second.tail())
processFiles = [f for f in os.listdir(config) if f.startswith("UserData_")]
check("in-memory inputs: no file of a process left when it ended, also of an ended process of this host", processFiles == [os.path.basename(otherHost)], str(processFiles))
finish()
