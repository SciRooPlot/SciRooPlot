#!/usr/bin/env bash
# Builds the web page into <out folder>: renders its plots with an installed SciRooPlot (the tour, the drawing option gallery,
# the example project), records the terminal demo, runs the plain ROOT macros of docs/slides/root-examples, extracts the
# example code, the palettes and the example data, and fills the template (build.py).
# usage: docs/site/build-site.sh <out folder>   (with env.sh of the installed SciRooPlot sourced; needs python3 with numpy and Pillow
# and the tools of record-demo.sh)
set -euo pipefail
SITE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "${SITE}/../.." && pwd)
mkdir -p "$1"
OUT=$(cd "$1" && pwd)
# shellcheck disable=SC1091
source "${SCIROOPLOT_DATA_DIR}/env.sh"   # srp and plot are shell functions
DATA="${SCIROOPLOT_DATA_DIR}/ExampleData"
WORK=$(mktemp -d)
trap 'rm -rf "${WORK}"' EXIT
export SCIROOPLOT_CONFIG_PATH="${WORK}/config"   # projects of this build only
cd "${WORK}"
mkdir -p "${OUT}/img/compare" gen

# the example project: its plots and, for the data browser, its data sources and plot definitions
srp example-py ex
srp select ex
plot '.+' '.+' png
# an animation of the example is one gif made from the frames in its group, with the mode its code names in a comment
grep -o "plot animations/[A-Za-z0-9_]* '\.+' gif+[0-9]*" "${REPO}/templates/example/DefinePlots.py" | while read -r _ animGroup _ gifMode; do
  plot "${animGroup}" '.+' "${gifMode}" </dev/null
  gifs=(ex/output/"${animGroup}"/*.gif)
  mv "${gifs[0]}" "ex/output/${animGroup}.gif"
  rm -r "ex/output/${animGroup:?}"   # the frames: the page shows the animation only
done
cp -r ex/output "${OUT}/img/ex"

# the terminal demo of the start page, recorded on a virtual screen (needs Xvfb, openbox, xterm, xdotool, ffmpeg and gifsicle)
bash "${SITE}/record-demo.sh" "$(cd "${SCIROOPLOT_DATA_DIR}/../.." && pwd)" "${SCIROOPLOT_CONFIG_PATH}" "${WORK}/ex" "${WORK}/demo"
cp demo/demo.gif "${OUT}/img/demo.gif"

# the plots of the tour and the drawing option gallery
srp add site "${SITE}/DefineSitePlots.py" "${WORK}/site-out"
srp select site
plot '.+' '.+' png
cp -r site-out/tour "${OUT}/img/tour"
cp -r site-out/gallery "${OUT}/img/gallery"

# the plain ROOT macros the vs. ROOT page compares with (the SciRooPlot side shows the example plots)
for macro in "${REPO}"/docs/slides/root-examples/*.C; do
  name=$(basename "${macro}" .C)
  root -l -b -q "${macro}(\"${DATA}\", \"${OUT}/img/compare/${name}_root.png\")"
done

python3 "${SITE}/extract_examples.py" "${REPO}/templates/example/DefinePlots.py" "${REPO}/templates/example/DefinePlots.cpp" > gen/examples.json
python3 "${SITE}/extract_palettes.py" > gen/palettes.json
python3 "${SITE}/extract_data.py" "${DATA}" "${SCIROOPLOT_CONFIG_PATH}/ex" "${OUT}/datajson" > gen/data.json
python3 "${SITE}/build.py" gen "${OUT}"
