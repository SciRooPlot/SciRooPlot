#!/bin/bash
# Records the terminal demo of the web page (tab completion, a plot window opening, several plots chosen by a regular expression
# and browsed with s) as GIF.
# Needs: Xvfb openbox xterm xdotool xsetroot ffmpeg gifsicle and the DejaVu fonts, an installed SciRooPlot (env.sh) and the example project.
# usage: bash record-demo.sh <srp prefix> <config dir with the example project "ex"> <example project dir> <out dir>
# Run by build-site.sh, so the GIF always shows the current plots.
set -euo pipefail
PREFIX=$1 CFG=$2 EXDIR=$3 OUT=$4
D=:55; export DISPLAY=$D
WORK=$(mktemp -d)
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
Xvfb $D -screen 0 1280x720x24 -nolisten tcp >/dev/null 2>&1 & XV=$!
sleep 1.5
openbox --sm-disable --config-file "$WORK/openbox/rc.xml" >/dev/null 2>&1 &
sleep 1.5
xsetroot -solid '#16233A'
xterm -geometry 64x24+536+56 -fa "DejaVu Sans Mono" -fs 13 -bg '#16233A' -fg '#E8EDF4' -cr '#E0A526' -b 18 -bw 0 -T terminal \
  -xrm 'XTerm*cursorBlink: false' -e bash --rcfile "$WORK/rc.sh" -i >/dev/null 2>&1 &
sleep 3
TERMW=$(xdotool search --name terminal | head -1)
term() { xdotool windowactivate --sync "$TERMW"; xdotool mousemove 1270 710; }
t() { xdotool type --delay 85 "$1"; }
waitwin() { for _ in $(seq 40); do xdotool search --name "$1" >/dev/null 2>&1 && return; sleep 0.25; done; }
term
ffmpeg -y -loglevel error -f x11grab -draw_mouse 0 -framerate 15 -video_size 1280x720 -i $D -c:v libx264 -preset veryfast -crf 18 -pix_fmt yuv444p "$WORK/raw.mp4" & FF=$!
sleep 1.2
# the browse keys are pressed in the terminal (they work there as in the plot window)
# one plot found by tab completion
t 'plot sp'; sleep 0.4; xdotool key Tab; sleep 0.9
xdotool key Tab; sleep 0.15; xdotool key Tab; sleep 1.8
t 'ptS'; sleep 0.3; xdotool key Tab; sleep 1.0
xdotool key Return; waitwin 'ptSpectrum:spectra'; term; sleep 3.0
xdotool key q; sleep 0.6; term; sleep 0.8
# several plots at once with a regular expression, browsed with s
t 'plot spectra multiplicity.*'; sleep 0.8
xdotool key Return; waitwin 'multiplicity'; term; sleep 2.4
xdotool key s; sleep 0.5; term; sleep 2.4
xdotool key s; sleep 0.5; term; sleep 2.6
xdotool key q; sleep 0.6; term; sleep 1.0
kill -INT $FF; wait $FF || true
kill $XV
SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" screenscale 1 >/dev/null
mkdir -p "$OUT"
ffmpeg -y -loglevel error -i "$WORK/raw.mp4" -vf "fps=12,scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle" "$WORK/demo.gif"
gifsicle -O3 --lossy=30 "$WORK/demo.gif" -o "$OUT/demo.gif"
rm -rf "$WORK"
