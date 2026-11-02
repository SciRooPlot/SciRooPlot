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
    --help | -h)
      echo "Usage: $0 [--prefix <directory>] [--reinstall]"
      echo
      echo "Options:"
      echo "  --prefix <directory>  Set installation prefix (default is <source_dir>/install)"
      echo "  --reinstall           Reconfigure CMake from scratch"
      echo "  --help, -h            Show this help message"
      exit 0
      ;;
    *)
      echo "Unknown option: $1."
      echo "Usage: $0 [--prefix <directory>] [--reinstall]"
      exit 1
      ;;
  esac
done

# on a terminal: colours, steps that turn into check marks, a progress bar for the build
if [[ -t 1 ]]; then
  YELLOW=$'\033[93m'
  CYAN=$'\033[96m'
  GREEN=$'\033[92m'
  RED=$'\033[91m'
  RESET=$'\033[0m'
  CLEAR_LINE=$'\r\033[K'
else
  YELLOW="" CYAN="" GREEN="" RED="" RESET="" CLEAR_LINE=""
fi
if [[ "${LC_ALL:-${LC_CTYPE:-${LANG:-}}}" == *[Uu][Tt][Ff]* ]]; then
  OK="✓" FAILED="✗" BAR_FULL="█" BAR_EMPTY="░"
else
  OK="+" FAILED="x" BAR_FULL="#" BAR_EMPTY="-"
fi
BAR_WIDTH=30

heading() {
  echo
  echo "${YELLOW}$1${RESET}"
}
row() {
  printf '  %s%-9s%s  %s\n' "${CYAN}" "$1" "${RESET}" "$2"
}
step_running() {
  [[ -t 1 ]] && printf '%s    %s ...' "${CLEAR_LINE}" "$1"
}
step_ok() {
  printf '%s  %s%s%s %s\n' "${CLEAR_LINE}" "${GREEN}" "${OK}" "${RESET}" "$1"
}
step_failed() {
  printf '%s  %s%s%s %s\n' "${CLEAR_LINE}" "${RED}" "${FAILED}" "${RESET}" "$1"
}
show_log() {
  echo
  tail -n 30 "${LOG_FILE}"
  echo
  echo "Full log: ${LOG_FILE}"
}
# run_step <label> <command...>: output goes into the log
run_step() {
  local label="$1"
  shift
  step_running "${label}"
  if "$@" >>"${LOG_FILE}" 2>&1; then
    step_ok "${label}"
  else
    step_failed "${label}"
    return 1
  fi
}
bar() {
  local filled=$(($1 * BAR_WIDTH / 100)) text="" i
  for ((i = 0; i < BAR_WIDTH; i++)); do
    if ((i < filled)); then text+="${BAR_FULL}"; else text+="${BAR_EMPTY}"; fi
  done
  printf '%s' "${text}"
}
# reads the build output, writes it into the log and draws the progress from its [ 42%] (make) or [12/40] (ninja) prefixes
build_progress() {
  local line inner value percent=-1
  while IFS= read -r line; do
    printf '%s\n' "${line}" >>"${LOG_FILE}"
    [[ -t 1 && "${line}" == \[* ]] || continue
    inner="${line#\[}"
    inner="${inner%%\]*}"
    inner="${inner// /}"
    case "${inner}" in
      *%) value="${inner%\%}" ;;
      */*) value="${inner%/*}/${inner#*/}" ;;
      *) continue ;;
    esac
    case "${value}" in
      */*)
        case "${value}" in *[!0-9/]* | /* | */) continue ;; esac
        ((${value#*/} > 0)) || continue
        value=$((100 * ${value%/*} / ${value#*/}))
        ;;
      "" | *[!0-9]*) continue ;;
    esac
    ((value > percent)) || continue
    percent=${value}
    printf '%s    Building  %s %3d%%' "${CLEAR_LINE}" "$(bar "${percent}")" "${percent}"
  done
}
build() {
  local status_file="${BUILD_DIR}/.install-status"
  step_running "Building"
  { cmake --build "${BUILD_DIR}" --parallel 2>&1; echo $? >"${status_file}"; } | build_progress
  local status
  status=$(cat "${status_file}")
  rm -f "${status_file}"
  if [[ "${status}" -ne 0 ]]; then
    step_failed "Building"
    return 1
  fi
  local warnings
  warnings=$(grep -cE '^[^ ]+:[0-9]+:[0-9]+: warning:' "${LOG_FILE}")  # file:line:column: warning: (GCC and Clang)
  if [[ "${warnings}" -eq 1 ]]; then
    step_ok "Building  (1 compiler warning, see ${LOG_FILE})"
  elif [[ "${warnings}" -gt 1 ]]; then
    step_ok "Building  (${warnings} compiler warnings, see ${LOG_FILE})"
  else
    step_ok "Building"
  fi
}

if [[ "${REINSTALL}" -eq 1 ]]; then
  rm -f "${CACHE_FILE}"
fi

DO_INSTALL=1
# determine installation prefix
if [[ -f "${CACHE_FILE}" ]]; then
  CACHED_PREFIX=$(grep -m1 '^CMAKE_INSTALL_PREFIX:PATH=' "${CACHE_FILE}" | cut -d= -f2)

  if [[ -z "${CACHED_PREFIX}" ]]; then
    echo "Error: Could not determine CMake install prefix."
    exit 1
  fi

  CACHED_ENV_SCRIPT="${CACHED_PREFIX}/share/scirooplot/env.sh"

  if [[ "${PREFIX_SET}" -eq 1 && "${CACHED_PREFIX}" != "${INSTALL_PREFIX}" ]]; then
    echo "Error: The build directory is configured for another install prefix:"
    echo
    echo "  Configured: ${CACHED_PREFIX}"
    echo "  Requested:  ${INSTALL_PREFIX}"
    echo
    echo "Run with --reinstall to configure with the requested prefix."
    exit 1
  fi

  if [[ -f "${CACHED_ENV_SCRIPT}" || "${PREFIX_SET}" -eq 1 ]]; then
    # the CMake build has previously been installed (or the prefix to update was given)
    INSTALL_PREFIX="${CACHED_PREFIX}"
    heading "Updating SciRooPlot"
  else
    # there is a CMake build, but it has never been installed
    heading "Updating the SciRooPlot development build (not installed)"
    DO_INSTALL=0
  fi
else
  heading "Installing SciRooPlot"
fi

row "source" "${SOURCE_DIR}"
row "build" "${BUILD_DIR}"
if [[ "${DO_INSTALL}" -eq 1 ]]; then
  row "prefix" "${INSTALL_PREFIX}"
fi
echo

mkdir -p "${BUILD_DIR}"
LOG_FILE="${BUILD_DIR}/install.log"
: >"${LOG_FILE}"

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  if git rev-parse --abbrev-ref --symbolic-full-name '@{u}' >/dev/null 2>&1; then
    if ! run_step "Updating the repository" git pull; then
      echo "  Could not update the repository: building the current checkout (see ${LOG_FILE})." >&2
    fi
  else
    echo "  No upstream branch (detached HEAD or local branch): building the current checkout."
  fi
fi

if [[ ! -f "${CACHE_FILE}" ]]; then
  if ! run_step "Configuring" cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"; then
    rm -f "${CACHE_FILE}"
    show_log
    exit 1
  fi
fi

build || {
  show_log
  exit 1
}
if [[ "${DO_INSTALL}" -eq 1 ]]; then
  run_step "Installing" cmake --install "${BUILD_DIR}" || {
    show_log
    exit 1
  }
fi

if [[ "${DO_INSTALL}" -eq 1 ]]; then
  ENV_SCRIPT="${INSTALL_PREFIX}/share/scirooplot/env.sh"
  heading "SciRooPlot is installed"
else
  ENV_SCRIPT="${BUILD_DIR}/share/scirooplot/env.sh"
  heading "The development build is updated"
fi
echo "  Activate it in this shell (and in new shells by adding the line to ~/.bashrc or ~/.zshrc):"
echo
echo "    ${CYAN}source ${ENV_SCRIPT}${RESET}"
echo
echo "  Then try ${CYAN}srp help${RESET}."
echo
