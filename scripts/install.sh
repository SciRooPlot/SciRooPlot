#!/bin/bash

(return 0 2>/dev/null) && {
    echo "The SciRooPlot installer must not be sourced."
    return 1
}

# support execution with zsh in case the interpreter is explicitly chosen
# shellcheck disable=SC2296
SOURCE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]:-${(%):-%N}}")/.." >/dev/null 2>&1 && pwd)"
cd "${SOURCE_DIR}" || exit 1

if [[ ! -f "CMakeLists.txt" ]]; then
    echo "Error: Not in SciRooPlot source directory."
    exit 1
fi

BUILD_DIR="${SOURCE_DIR}/build"
DEFAULT_PREFIX="${SOURCE_DIR}/install"
INSTALL_PREFIX="${DEFAULT_PREFIX}"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"

REINSTALL=0
PREFIX_SET=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix)
            if [[ -z "${2:-}" || "${2}" == -* ]]; then
                echo "Error: --prefix requires a directory."
                exit 1
            fi
            INSTALL_PREFIX="$2"
            PREFIX_SET=1
            shift 2
            ;;
        --reinstall)
            REINSTALL=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--prefix <directory>] [--reinstall]"
            echo
            echo "Options:"
            echo "  --prefix <directory>  Set installation prefix (default is <source_dir>/install)"
            echo "  --reinstall           Reconfigure CMake from scratch"
            echo "  --help, -h            Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--prefix <directory>] [--reinstall]"
            exit 1
            ;;
    esac
done

if [[ "${REINSTALL}" -eq 1 ]]; then
    rm -f "${CACHE_FILE}"
fi

# determine installation prefix
if [[ -f "${CACHE_FILE}" ]]; then
    CACHED_PREFIX=$(grep -m1 '^CMAKE_INSTALL_PREFIX:PATH=' "${CACHE_FILE}" | cut -d= -f2)

    if [[ -z "${CACHED_PREFIX}" ]]; then
        echo "Error: Could not determine CMake install prefix."
        exit 1
    fi

    CACHED_ENV_SCRIPT="${CACHED_PREFIX}/share/scirooplot/env.sh"

    if [[ -f "${CACHED_ENV_SCRIPT}" ]]; then
        # the CMake build has previously been installed.
        if [[ "${PREFIX_SET}" -eq 1 && "${CACHED_PREFIX}" != "${INSTALL_PREFIX}" ]]; then
            echo "Error: Existing installation uses a different install prefix:"
            echo
            echo "  Current:   ${CACHED_PREFIX}"
            echo "  Requested: ${INSTALL_PREFIX}"
            echo
            echo "Run with --reinstall to configure with the new prefix."
            exit 1
        fi

        INSTALL_PREFIX="${CACHED_PREFIX}"

        echo "Existing SciRooPlot installation detected."
        echo "Updating installation:"
        echo "  ${INSTALL_PREFIX}"
    else
        # there is a CMake build, but it has never been installed.
        echo "Development build detected."
        echo "No existing SciRooPlot installation found."
        echo "Installation prefix from CMake:"
        echo "  ${CACHED_PREFIX}"
        echo
        echo "Run ./scripts/install.sh to install SciRooPlot."
        exit 1
    fi
else
    echo "Installing SciRooPlot to:"
    echo "  ${INSTALL_PREFIX}"
fi

echo
echo "Source directory : ${SOURCE_DIR}"
echo "Build directory  : ${BUILD_DIR}"
echo "Install prefix   : ${INSTALL_PREFIX}"
echo

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Updating repository..."
    git pull || exit 1
    echo
fi

mkdir -p "${BUILD_DIR}"

if [[ ! -f "${CACHE_FILE}" ]]; then
    cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
fi

cmake --build "${BUILD_DIR}" >/dev/null 2>&1
cmake --install "${BUILD_DIR}" >/dev/null 2>&1

ENV_SCRIPT="${INSTALL_PREFIX}/share/scirooplot/env.sh"

echo "========================================"
echo "SciRooPlot installation complete."
echo
echo "Install prefix:"
echo "  ${INSTALL_PREFIX}"
echo

if [[ -f "${ENV_SCRIPT}" ]]; then
    echo "Activate SciRooPlot in the current shell with:"
    echo
    echo "  source ${ENV_SCRIPT}"
    echo
    echo "To enable it automatically in future shells,"
    echo "add this line to ~/.bashrc or ~/.zshrc:"
    echo
    echo "  source ${ENV_SCRIPT}"
else
    echo "Warning: Environment script not found:"
    echo "  ${ENV_SCRIPT}"
fi

echo
echo "Verify the installation with:"
echo
echo "  srp help"
echo
echo "========================================"
