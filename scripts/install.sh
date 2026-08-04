#!/bin/bash

(return 0 2>/dev/null) && {
    echo "The SciRooPlot installer must not be sourced."
    return 1
}

if [[ -d "${SCIROOPLOT_SOURCE_DIR:-}" ]]; then
    cd "${SCIROOPLOT_SOURCE_DIR}" || exit 1
else
    echo "SciRooPlot source directory not found: ${SCIROOPLOT_SOURCE_DIR:-<unset>}"
    exit 1
fi
if [[ ! -f "CMakeLists.txt" ]]; then
    echo "Error: Not in SciRooPlot source dir."
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
    echo "Press enter for default install location [${INSTALL_DIR}]"
    echo -n "or specify a different path: "
    read -r USER_INSTALL_DIR
    INSTALL_DIR="${USER_INSTALL_DIR:-$INSTALL_DIR}"
else
    echo "Skipping prompts and reusing existing configuration."
    echo "To reconfigure the installation run with argument --reinstall."
fi

# update
echo
echo "Updating repository..."
git pull
echo

# build
mkdir -p build
cd build || exit 1
if [[ "${UPDATE_MODE}" -eq 0 ]]; then
    cmake -S .. -B . "${CMAKE_FLAGS}"
fi
cmake --build .
cmake --install . >/dev/null 2>&1

# source env
ENV_SCRIPT="${INSTALL_DIR}/share/scirooplot/scirooplot-env.sh"
if [[ -f "${ENV_SCRIPT}" ]]; then
    echo
    echo "Make sure to add the following line to your ~/.bashrc or ~/.zshrc:"
    echo "source ${ENV_SCRIPT}"
else
    echo "Warning: Environment script not found at ${ENV_SCRIPT}"
fi
