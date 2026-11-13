"""plot runs of one project at the same time and interrupted: two runs into one output folder at once (one writing Plots.root, one the
pictures of mode png) both end with exit status 0 and write all they make (every plot in Plots.root, every picture complete); Ctrl-C (SIGINT
to the process group, as a terminal sends it) ends a long run within seconds, in mode file and png, and leaves the Plots.root of an earlier
run as it was (it is written at the end of a run).
Covers: two runs at once, Ctrl-C during a long run."""

import os
import pty
import select
import signal
import subprocess
import time

from srptest import PLOT, ROOT, Project, S, check, finish

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
finish()
