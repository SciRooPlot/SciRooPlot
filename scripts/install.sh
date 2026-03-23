#!/bin/bash

# prevent sourcing
if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    echo "Error: This script must be executed, not sourced."
    return 1 2>/dev/null || exit 1
fi

# move to repo root
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    cd "$(git rev-parse --show-toplevel)"
else
    echo "Error: Not in SciRooPlot project root."
fi

# validate
if [[ ! -f "CMakeLists.txt" ]]; then
    echo "Error: Not in SciRooPlot project root."
    exit 1
fi

BUILD_DIR="$(pwd)/build"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"

# auto-enable update mode if cache exists
UPDATE_MODE=0
if [[ -f "$CACHE_FILE" ]]; then
    UPDATE_MODE=1
    for arg in "$@"; do
        case "$arg" in
            --reinstall)
                rm "$CACHE_FILE"
                UPDATE_MODE=0
                ;;
        esac
    done
fi

# interactive setup
INSTALL_DIR="${BUILD_DIR}"
CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}"
if [[ "${UPDATE_MODE}" -eq 0 ]]; then
    echo -n "Press enter for default install location [${INSTALL_DIR}] or specify a different path: "
    read USER_INSTALL_DIR
    INSTALL_DIR="${USER_INSTALL_DIR:-$INSTALL_DIR}"

    echo "Choose console color mode:"
    echo "1) Light (default)"
    echo "2) Dark"
    echo "3) Off"
    echo -n "Enter choice [1-3]: "
    read COLOR_CHOICE

    case "${COLOR_CHOICE}" in
        2)
            CMAKE_FLAGS="${CMAKE_FLAGS} -DDARK_COLORS=ON"
            ;;
        3)
            CMAKE_FLAGS="${CMAKE_FLAGS} -DDISABLE_COLORS=ON"
            ;;
        *)
            ;;
    esac
else
    echo "Skipping prompts and reusing existing configuration."
    echo "To reconfigure the installation run with argument --reinstall."
fi

# update
echo "Updating repository..."
git pull

# build
mkdir -p build
cd build
if [[ "${UPDATE_MODE}" -eq 0 ]]; then
    cmake ${CMAKE_FLAGS} ..
fi
make
make install >/dev/null 2>&1

# source env
ENV_SCRIPT="${INSTALL_DIR}/share/scirooplot/scirooplot-env.sh"
if [[ -f "${ENV_SCRIPT}" ]]; then
    echo
    echo "Sourcing environment script..."
    source "${ENV_SCRIPT}"
    echo
    echo "Make sure to add the following line to your ~/.bashrc or ~/.zshrc:"
    echo "source ${ENV_SCRIPT}"
else
    echo "Warning: Environment script not found at ${ENV_SCRIPT}"
fi
