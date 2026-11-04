#!/usr/bin/env bash
# Checks that the legend symbols show what is drawn for every data type, drawing option and style (see legend-symbols/compare.py).

set -euo pipefail

if [ -z "${SRP_TEST_INSTALL:-}" ]; then
    echo "SRP_TEST_INSTALL is not set"
    exit 1
fi
# shellcheck disable=SC1091
source "${SRP_TEST_INSTALL}/share/scirooplot/env.sh"

SCRIPTS=$(cd "$(dirname "${BASH_SOURCE[0]}")/legend-symbols" && pwd)
WORKDIR=$(mktemp -d)
trap 'rm -rf "${WORKDIR}"' EXIT
export SCIROOPLOT_CONFIG_PATH="${WORKDIR}/config"
export LEGEND_TEST_DATA="${WORKDIR}/data.root"

python3 "${SCRIPTS}/MakeData.py" "${LEGEND_TEST_DATA}"
cp "${SCRIPTS}/DefinePlots.py" "${WORKDIR}/"
srp add legendSymbols "${WORKDIR}/DefinePlots.py" "${WORKDIR}/output"
plot legend '.+' root
python3 "${SCRIPTS}/compare.py" "${WORKDIR}/output"
