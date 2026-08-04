#!/usr/bin/env bash

set -euxo pipefail

if [ -z "${SRP_TEST_INSTALL:-}" ]; then
    echo "SRP_TEST_INSTALL is not set"
    exit 1
fi

echo "CONDA_PREFIX=$CONDA_PREFIX"
echo "CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH"

find "$CONDA_PREFIX" -name "VdtConfig.cmake"
find "$CONDA_PREFIX" -name "ROOTConfig.cmake"

source "${SRP_TEST_INSTALL}/share/scirooplot/scirooplot-env.sh"

WORKDIR=$(mktemp -d)
trap 'rm -rf "${WORKDIR}"' EXIT

echo "Testing in ${WORKDIR}"
cd "${WORKDIR}"

echo "=== Testing C++ ==="
# initialize a project and make a plot with cpp user code
srp init-cpp TestProject_CPP
plot examples ptSpec pdf
test -s TestProject_CPP/output/examples/ptSpec.pdf

srp reset

echo "=== Testing Python ==="
# initialize a project and make a plot with python user code
srp init-py TestProject_PY
plot examples ptSpec pdf
test -s TestProject_PY/output/examples/ptSpec.pdf


echo "SciRooPlot integration test successful"
