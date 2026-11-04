#!/bin/bash
# Records the terminal demos of the web page as GIFs, one per scene, which the start page shows one after the other:
#   1 a plot found by tab completion, opening in a window
#   2 several plots chosen by a regular expression, browsed with s
#   3 the plotting program changed: plot runs it again and the window shows the change
#   4 a look into an input file with srp print, then a plot of what is in it
# Needs: Xvfb openbox xterm xdotool xsetroot ffmpeg ffprobe gifsicle and the DejaVu fonts, an installed SciRooPlot (env.sh) and the example project.
# usage: bash record-demo.sh <srp prefix> <config dir with the example project "ex"> <example project dir> <out dir>
# Writes <out dir>/demo-<n>.gif and demos.json (file, title, caption, seconds per scene). Run by build-site.sh, so the GIFs always show
# the current plots.
set -euo pipefail
PREFIX=$1 CFG=$2 EXDIR=$3 OUT=$4
D=:55; export DISPLAY=$D
WORK=$(mktemp -d)
# on any exit, also a failed step: stop Xvfb, openbox, xterm and ffmpeg, or the next run finds display :55 taken
trap 'kill $(jobs -p) 2>/dev/null || true; rm -rf "${WORK}"' EXIT  # kill fails when nothing is left to stop
cat > "$WORK/rc.sh" <<RC
source $PREFIX/share/scirooplot/env.sh
export SCIROOPLOT_CONFIG_PATH=$CFG SCIROOPLOT_DATA_DIR=$PREFIX/share/scirooplot
srp select ex >/dev/null 2>&1
cd $EXDIR
PS1='\[\e[38;2;28;148;165;1m\]\$\[\e[0m\] '
clear
RC
mkdir -p "$WORK/openbox" && cat > "$WORK/openbox/rc.xml" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_config xmlns="http://openbox.org/3.4/rc"><placement><policy>Smart</policy></placement>
<applications><application class="XTerm"><decor>no</decor></application></applications></openbox_config>
XML
SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" screenscale 0.63 >/dev/null
cp "$EXDIR/DefinePlots.py" "$WORK/DefinePlots.py"  # scene 3 changes the program: put back afterwards
Xvfb $D -screen 0 1280x720x24 -nolisten tcp >/dev/null 2>&1 & XV=$!
sleep 1.5
openbox --sm-disable --config-file "$WORK/openbox/rc.xml" >/dev/null 2>&1 &
sleep 1.5
xsetroot -solid '#16233A'
COLS=64
# -l: a log of everything shown in the terminal, to check that no line is longer than the terminal is wide
xterm -geometry ${COLS}x24+536+56 -fa "DejaVu Sans Mono" -fs 13 -bg '#16233A' -fg '#E8EDF4' -cr '#E0A526' -b 18 -bw 0 -T terminal \
  -xrm 'XTerm*cursorBlink: false' -l -lf "$WORK/terminal.log" -e bash --rcfile "$WORK/rc.sh" -i >/dev/null 2>&1 &
sleep 3
TERMW=$(xdotool search --name terminal | head -1)
term() { xdotool windowactivate --sync "$TERMW"; xdotool mousemove 1270 710; }
t() { xdotool type --delay 85 "$1"; }
waitwin() { for _ in $(seq 40); do xdotool search --name "$1" >/dev/null 2>&1 && return; sleep 0.25; done; }
clean() { xdotool type --delay 0 'clear'; xdotool key Return; sleep 0.5; }  # between scenes, not recorded
term
FF=0
start() { ffmpeg -y -loglevel error -f x11grab -draw_mouse 0 -framerate 15 -video_size 1280x720 -i $D -c:v libx264 -preset veryfast -crf 18 -pix_fmt yuv444p "$WORK/$1.mp4" & FF=$!; sleep 1.2; }
stop() { kill -INT $FF; wait $FF || true; }

# the browse keys are pressed in the terminal (they work there as in the plot window)
start 1  # one plot found by tab completion
t 'plot sp'; sleep 0.4; xdotool key Tab; sleep 0.9
xdotool key Tab; sleep 0.15; xdotool key Tab; sleep 1.8
t 'ptS'; sleep 0.3; xdotool key Tab; sleep 1.0
xdotool key Return; waitwin 'ptSpectrum:spectra'; term; sleep 3.0
xdotool key q; sleep 0.6; term; sleep 0.8
stop; clean

start 2  # several plots at once with a regular expression, browsed with s
t 'plot spectra multiplicity.*'; sleep 0.8
xdotool key Return; waitwin 'multiplicity'; term; sleep 2.4
xdotool key s; sleep 0.5; term; sleep 2.4
xdotool key s; sleep 0.5; term; sleep 2.6
xdotool key q; sleep 0.6; term; sleep 1.0
stop; clean

start 3  # the program changed: plot runs it again before drawing
t "sed -i '/(\"ptSpec\"), \"sim/s/\$/.SetLineColor(kBlue)/' *.py"; sleep 0.5; xdotool key Return; sleep 1.0
t 'plot spectra ptSpectrum'; sleep 0.5
xdotool key Return; waitwin 'ptSpectrum:spectra'; term; sleep 3.5
xdotool key q; sleep 0.6; term; sleep 0.8
stop; clean
cp "$WORK/DefinePlots.py" "$EXDIR/DefinePlots.py"

# the wider canvas of energyScan (with its colour key) must not cover the terminal
SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" screenscale 0.5 >/dev/null
start 4  # what is in an input file, then a plot of it
# shellcheck disable=SC2016  # typed into the terminal, expanded there
t 'srp print $SCIROOPLOT_DATA_DIR/ExampleData/spectra.root'; sleep 0.5; xdotool key Return; sleep 3.5
t 'plot spectra energyScan'; sleep 0.5
xdotool key Return; waitwin 'energyScan:spectra'; term; sleep 3.5
xdotool key q; sleep 0.6; term; sleep 0.8
stop

kill $XV
# a line longer than the terminal breaks in the middle of the GIF: say which
long=$(sed 's/\x1b\[[0-9;?]*[A-Za-z]//g; s/\x1b[()][A-Z0-9]//g; s/\r$//' "$WORK/terminal.log" | tr '\r' '\n' | awk -v w="$COLS" 'length($0) > w' || true)
if [[ -n $long ]]; then
  echo "::warning::lines of the terminal demos are wider than the terminal (${COLS} columns):" >&2
  echo "$long" >&2
fi
SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" screenscale 1 >/dev/null
mkdir -p "$OUT"
titles=("Find a plot" "Several plots at once" "Change the code, plot again" "Look into the inputs")
captions=("plot sp, Tab: groups and plots are completed, the plot opens in a window"
  "group and name are regular expressions: multiplicity.* matches three plots, s browses them"
  "the plotting program changed: plot runs it again before drawing, the simulation is drawn in blue"
  "srp print lists what is in a file, here the folders and histograms of the example data")
echo "[" > "$OUT/demos.json"
for n in 1 2 3 4; do
  ffmpeg -y -loglevel error -i "$WORK/$n.mp4" -vf "fps=12,scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle" "$WORK/demo-$n.gif"
  gifsicle -O3 --lossy=30 --no-loopcount "$WORK/demo-$n.gif" -o "$OUT/demo-$n.gif"  # played once: the page moves on to the next scene
  seconds=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$WORK/$n.mp4")
  printf '  {"file": "demo-%d.gif", "title": "%s", "caption": "%s", "seconds": %.1f}%s\n' "$n" "${titles[n - 1]}" "${captions[n - 1]}" "$seconds" "$([[ $n -lt 4 ]] && echo ,)" >> "$OUT/demos.json"
done
echo "]" >> "$OUT/demos.json"
