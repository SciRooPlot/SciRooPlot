#!/bin/bash
# Records the terminal demos of the web page as videos, one per scene, which the start page plays one after the other:
#   1 a plot found by tab completion, opening in a window
#   2 all plots of a group at once, browsed with s
#   3 the plot definition changed in an editor: plot runs the program again and shows the change
#   4 the plots of a group written as pdf files
# Needs: Xvfb openbox xterm xdotool nano imagemagick ffmpeg bc and the DejaVu fonts, and an installed SciRooPlot (env.sh).
# usage: bash record-demo.sh <srp prefix> <config dir> <out dir>
# The scenes use the example project, made as ~/my-analysis for the recording (paths in the output look like a user's) and removed after.
# Writes <out dir>/demo-<n>.mp4, demo-1.png (shown before the first video plays) and demos.json (file, title, text, command per scene).
# Run by build-site.sh, so the videos always show the current plots.
set -euo pipefail
PREFIX=$1 CFG=$2 OUT=$3
EXDIR=$HOME/my-analysis
if [[ -e $EXDIR ]]; then echo "$EXDIR exists: not recording the demos into it" >&2; exit 1; fi
D=:55; export DISPLAY=$D
W=1600 H=900  # the screen; the page shows it at about 1150 px
WORK=$(mktemp -d)
# on any exit, also a failed step: stop Xvfb, openbox, xterm and ffmpeg, or the next run finds display :55 taken
trap 'kill $(jobs -p) 2>/dev/null || true; rm -rf "${WORK}" "${EXDIR}"; SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" remove my-analysis >/dev/null 2>&1 || true' EXIT  # kill fails when nothing is left to stop
# shellcheck disable=SC1091  # env.sh of the installation
( source "$PREFIX/share/scirooplot/env.sh"; export SCIROOPLOT_CONFIG_PATH=$CFG SCIROOPLOT_DATA_DIR=$PREFIX/share/scirooplot; srp example-py my-analysis "$EXDIR" >/dev/null )

# the look of the desktop: a dark blue gradient, windows with a flat title bar and three round buttons on the left
THEME=$WORK/home/.themes/srp/openbox-3
mkdir -p "$THEME" "$WORK/home/.config/openbox"
for b in close iconify max; do
  printf '#define %s_width 12\n#define %s_height 12\nstatic unsigned char %s_bits[] = {\n 0xf0,0x00,0xfc,0x03,0xfe,0x07,0xfe,0x07,0xff,0x0f,0xff,0x0f,\n 0xff,0x0f,0xff,0x0f,0xfe,0x07,0xfe,0x07,0xfc,0x03,0xf0,0x00 };\n' "$b" "$b" "$b" > "$THEME/$b.xbm"
done
cat > "$THEME/themerc" <<'THEME'
border.width: 1
padding.width: 12
padding.height: 7
window.handle.width: 0
window.client.padding.width: 0
window.client.padding.height: 0
window.active.border.color: #3a4d6e
window.inactive.border.color: #3a4d6e
window.active.title.bg: flat solid
window.active.title.bg.color: #24344f
window.inactive.title.bg: flat solid
window.inactive.title.bg.color: #24344f
window.active.label.bg: parentrelative
window.inactive.label.bg: parentrelative
window.active.label.text.color: #e8edf4
window.inactive.label.text.color: #e8edf4
window.label.text.justify: center
window.active.button.unpressed.bg: parentrelative
window.inactive.button.unpressed.bg: parentrelative
window.active.button.close.unpressed.image.color: #ff5f57
window.inactive.button.close.unpressed.image.color: #ff5f57
window.active.button.iconify.unpressed.image.color: #febc2e
window.inactive.button.iconify.unpressed.image.color: #febc2e
window.active.button.max.unpressed.image.color: #28c840
window.inactive.button.max.unpressed.image.color: #28c840
menu.border.width: 0
THEME
cat > "$WORK/home/.config/openbox/rc.xml" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_config xmlns="http://openbox.org/3.4/rc">
  <theme><name>srp</name><titleLayout>CIML</titleLayout>
    <font place="ActiveWindow"><name>DejaVu Sans</name><size>11</size><weight>bold</weight></font>
    <font place="InactiveWindow"><name>DejaVu Sans</name><size>11</size><weight>bold</weight></font></theme>
  <placement><policy>Smart</policy></placement>
  <margins><top>36</top><left>36</left><right>36</right><bottom>36</bottom></margins>
</openbox_config>
XML
convert -size "${W}x${H}" radial-gradient:'#2a4166'-'#101a2b' "$WORK/desktop.png"

cat > "$WORK/rc.sh" <<RC
source $PREFIX/share/scirooplot/env.sh
export SCIROOPLOT_CONFIG_PATH=$CFG SCIROOPLOT_DATA_DIR=$PREFIX/share/scirooplot EDITOR=nano
srp select my-analysis >/dev/null 2>&1
cd $EXDIR
PS1='\[\e[38;2;94;196;214m\]~/my-analysis\[\e[0m\] \[\e[38;2;224;165;38;1m\]\$\[\e[0m\] '
clear
RC
# nano as a plain editor: calm colours (comments grey, strings green, keywords cyan), long lines wrapped at blanks (the edited line stays
# readable), no line numbers and no help lines
cat > "$WORK/nanorc" <<'NANORC'
set nohelp
set softwrap
set atblanks
set titlecolor bold,white,black
syntax python "\.py$"
color lightblack "#.*$"
color green ""([^"\\]|\\.)*""
color cyan "\<(def|for|in|if|else|elif|return|import|from|as|with|not|and|or)\>"
NANORC
cat >> "$WORK/rc.sh" <<RC
alias nano='nano --rcfile $WORK/nanorc'
RC

SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" screenscale 0.85 >/dev/null
Xvfb $D -screen 0 ${W}x${H}x24 -nolisten tcp >/dev/null 2>&1 &
XV=$!
sleep 1.5
HOME=$WORK/home openbox --sm-disable >/dev/null 2>&1 &
sleep 1.5
display -window root "$WORK/desktop.png" || true  # sets the background, but exits with 1
COLS=64 ROWS=36
# -l: a log of everything shown in the terminal, to check that no line is longer than the terminal is wide
xterm -geometry ${COLS}x${ROWS}-36+36 -fa "DejaVu Sans Mono" -fs 14 -bg '#16233A' -fg '#E8EDF4' -cr '#E0A526' -b 16 -bw 0 \
  -T "Terminal" -xrm 'XTerm*cursorBlink: false' -xrm 'XTerm*allowTitleOps: false' -xrm 'XTerm*allowSendEvents: true' -l -lf "$WORK/terminal.log" -e bash --rcfile "$WORK/rc.sh" -i >/dev/null 2>&1 &
sleep 3
TERMW=$(xdotool search --name '^Terminal$' | head -1)
# keys are sent to the terminal without raising it (xterm takes them with allowSendEvents), so the plot windows stay in front
t() { xdotool type --window "$TERMW" --delay 70 "$1"; }
key() { xdotool key --window "$TERMW" "$@"; }
waitwin() { for _ in $(seq 100); do xdotool search --name "$1" >/dev/null 2>&1 && return; sleep 0.1; done; }
waitlog() { for _ in $(seq 100); do grep -q "$1" "$WORK/terminal.log" 2>/dev/null && return; sleep 0.1; done; }
clean() { xdotool type --window "$TERMW" --delay 0 'clear'; key Return; sleep 0.5; }  # between scenes, not recorded
FF=0
SCENE=0 T0=0
start() { SCENE=$1; ffmpeg -y -loglevel error -f x11grab -draw_mouse 0 -framerate 20 -video_size ${W}x${H} -i $D -c:v libx264 -preset veryfast -crf 16 -pix_fmt yuv444p "$WORK/$1.mp4" & FF=$!; T0=$(date +%s.%N); sleep 1.0; }
# the start-up of plot (ROOT, reading the inputs) is cut from the video to 0.4 s: Return, then the window or the output
now() { echo "$(date +%s.%N) - $T0" | bc; }
# (the video starts a moment after T0: the end of the cut is taken a little earlier, so nothing of the window is lost)
run() { key Return; local from to; from=$(echo "$(now) + 0.4" | bc); "$@"; to=$(echo "$(now) - 0.3" | bc); (( $(echo "$to > $from" | bc) )) && echo "between(t,$from,$to)" >> "$WORK/cut-$SCENE"; return 0; }
stop() { kill -INT $FF; wait $FF || true; }

# the browse keys are pressed in the terminal (they work there as in the plot window)
start 1  # one plot, found by tab completion
t 'plot hi'; sleep 0.6; key Tab; sleep 1.2   # the group higgs (with its subgroups)
key Tab; sleep 0.2; key Tab; sleep 2.5       # its plots
t 'br'; sleep 0.5; key Tab; sleep 1.2
run waitwin 'brazilPlot:higgs'; sleep 4.5
key q; sleep 0.8; sleep 1.0
stop; clean

start 2  # all plots of a group, browsed with s
t "plot higgs '.+'"; sleep 0.8
run waitwin 'massSpectrum:higgs'; sleep 3.0
for _ in 1 2 3; do key s; sleep 0.6; sleep 2.6; done
key q; sleep 0.8; sleep 1.0
stop; clean

start 3  # the definition changed in an editor: plot runs the program again
line=$(grep -n '"massSpectrum", "higgs/fourLepton"' "$EXDIR/DefinePlots.py" | cut -d: -f1)
code=$(sed -n "$((line + 2))p" "$EXDIR/DefinePlots.py")
GAP=${code#*kAzure - 9}; GAP=${GAP%%kAzure - 9*}  # text between the two colours
t "nano +$((line + 2)),$(( $(sed -n "$((line + 2))p" "$EXDIR/DefinePlots.py" | grep -bo 'kAzure - 9' | head -1 | cut -d: -f1) + 1 )) DefinePlots.py"; sleep 0.6
key Return; sleep 2.5
# the fill and the outline of the background: kAzure - 9 becomes kOrange - 4 in both places
for k in 1 2; do
  [[ $k == 2 ]] && for _ in $(seq ${#GAP}); do key Right; sleep 0.04; done
  for _ in $(seq 10); do key Delete; done; sleep 0.3
  t 'kOrange - 4'; sleep 0.8
done
sleep 0.6
key ctrl+s; sleep 1.0; key ctrl+x; sleep 0.8
t 'plot higgs/fourLepton massSpectrum'; sleep 0.6
run waitwin 'massSpectrum:higgs'; sleep 4.5
key q; sleep 0.8; sleep 1.0
stop; clean

start 4  # files for a paper or slides
t "plot higgs '.+' pdf"; sleep 0.6; run waitlog 'brazilPlot from group'; sleep 2.0
t 'ls output/higgs/*'; sleep 0.5; key Return; sleep 4.5
stop

kill $XV
# a line longer than the terminal breaks in the middle of the video: say which (not in nano, which cuts lines at the width)
long=$(perl -0pe 's/\e\[\?1049h.*?\e\[\?1049l//gs' "$WORK/terminal.log" | sed 's/\x1b\[[0-9;?]*[A-Za-z]//g; s/\x1b[()][A-Z0-9]//g; s/\r$//' | tr '\r' '\n' | awk -v w="$COLS" 'length($0) > w' || true)
if [[ -n $long ]]; then
  echo "::warning::lines of the terminal demos are wider than the terminal (${COLS} columns):" >&2
  echo "$long" >&2
fi
SCIROOPLOT_CONFIG_PATH=$CFG "$PREFIX/bin/srp" screenscale 1 >/dev/null
mkdir -p "$OUT"
titles=("Show a plot" "Browse a group" "Change a plot" "Save for a paper")
# in the texts `...` is shown as code and {...} as a key
texts=("Type \`plot\`, then the group and the name of a plot. {Tab} completes both, so you never need to remember the exact names."
  "Group and name are patterns: \`.+\` stands for every plot of the group. Press {s} for the next plot and {a} to go back."
  "Plots are defined in a short program, here in Python. Change it, run \`plot\` again: the new version is drawn."
  "A mode as third word writes files instead: \`pdf\`, \`png\`, \`svg\`, a ROOT macro and more, one per plot, in folders by group.")
commands=("plot higgs brazilPlot" "plot higgs '.+'" "plot higgs/fourLepton massSpectrum" "plot higgs '.+' pdf")
echo "[" > "$OUT/demos.json"
for n in 1 2 3 4; do
  # small files for the web: 1280 px wide, 4:2:0 colours (every browser), the moov atom first so playback starts at once
  cut=""
  [[ -f "$WORK/cut-$n" ]] && cut="select='not($(paste -sd+ "$WORK/cut-$n"))',setpts=N/FRAME_RATE/TB,"
  ffmpeg -y -loglevel error -i "$WORK/$n.mp4" -vf "${cut}scale=1280:-2:flags=lanczos" -c:v libx264 -preset slow -crf 22 -pix_fmt yuv420p -movflags +faststart -an "$OUT/demo-$n.mp4"
  printf '  {"file": "demo-%d.mp4", "title": "%s", "text": "%s", "command": "%s"}%s\n' "$n" "${titles[n - 1]}" "${texts[n - 1]}" "${commands[n - 1]}" "$([[ $n -lt 4 ]] && echo ,)" >> "$OUT/demos.json"
done
echo "]" >> "$OUT/demos.json"
ffmpeg -y -loglevel error -sseof -4 -i "$OUT/demo-1.mp4" -frames:v 1 "$OUT/demo-1.png"  # before the first video plays: the plot of scene 1
