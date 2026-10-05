#!/usr/bin/env bash

set -euxo pipefail

if [ -z "${SRP_TEST_INSTALL:-}" ]; then
    echo "SRP_TEST_INSTALL is not set"
    exit 1
fi
if [ -n "${CONDA_PREFIX:-}" ]; then
    export CMAKE_PREFIX_PATH="${CONDA_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
fi
# shellcheck disable=SC1091
source "${SRP_TEST_INSTALL}/share/scirooplot/env.sh"

WORKDIR=$(mktemp -d)
trap 'rm -rf "${WORKDIR}"' EXIT
export SCIROOPLOT_CONFIG_PATH="${WORKDIR}/config"  # keep the user's own projects out of reach of the test (it ends with srp reset)

echo "Testing in ${WORKDIR}"
cd "${WORKDIR}"

echo "=== Testing C++ skeleton ==="
srp init-cpp Skeleton_CPP
plot myGroup myFirstPlot pdf
test -s Skeleton_CPP/output/myGroup/myFirstPlot.pdf

echo "=== Testing Python skeleton ==="
srp init-py Skeleton_PY
plot myGroup myFirstPlot pdf
test -s Skeleton_PY/output/myGroup/myFirstPlot.pdf

echo "=== Testing C++ examples ==="
srp example-cpp Example_CPP
plot '.+' '.+' pdf
test -s Example_CPP/output/higgs/diphoton/massSpectrum.pdf

echo "=== Testing Python examples ==="
srp example-py Example_PY
plot '.+' '.+' pdf
test -s Example_PY/output/higgs/diphoton/massSpectrum.pdf

echo "=== C++ and Python templates must define the same plots ==="
diff "$(srp confdir Skeleton_CPP)/plots.info" "$(srp confdir Skeleton_PY)/plots.info"
diff "$(srp confdir Example_CPP)/plots.info" "$(srp confdir Example_PY)/plots.info"

echo "=== remove and reset delete the project directories in the config path ==="
removed_dir="$(srp confdir Skeleton_PY)"
srp remove Skeleton_PY
test ! -e "${removed_dir}"
test -s Skeleton_PY/DefinePlots.py  # user code is kept
remaining_dir="$(srp confdir Example_PY)"
srp reset
test ! -e "${remaining_dir}"

echo "SciRooPlot integration test successful"
