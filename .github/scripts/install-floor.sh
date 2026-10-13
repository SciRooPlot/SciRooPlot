#!/usr/bin/env bash
# Installs the minimum versions of CMake, pybind11, fmt and Boost required in CMakeLists.txt
# (ROOT and Python come from the conda environment). Usage: install-floor.sh <prefix>

set -euxo pipefail

PREFIX="$(realpath -m "$1")"
SOURCE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
WORKDIR=$(mktemp -d)
trap 'rm -rf "${WORKDIR}"' EXIT

# pad to major.minor.patch
full_version() {
  local version="$1"
  while [ "$(tr -cd . <<< "${version}" | wc -c)" -lt 2 ]; do version="${version}.0"; done
  echo "${version}"
}
required() {
  full_version "$(sed -n "s/^set(REQUIRED_$1_VERSION \(.*\))$/\1/p" "${SOURCE_DIR}/CMakeLists.txt")"
}

CMAKE_VERSION=$(full_version "$(sed -n 's/^cmake_minimum_required(VERSION \([0-9.]*\)).*/\1/p' "${SOURCE_DIR}/CMakeLists.txt")")
PYBIND11_VERSION=$(required PYBIND11)
FMT_VERSION=$(required FMT)
BOOST_VERSION=$(required BOOST)

python -m pip install "cmake==${CMAKE_VERSION}" "pybind11==${PYBIND11_VERSION}"
test "$(cmake --version | head -1)" = "cmake version ${CMAKE_VERSION}"

cd "${WORKDIR}"

curl -fsSL -o fmt.zip "https://github.com/fmtlib/fmt/releases/download/${FMT_VERSION}/fmt-${FMT_VERSION}.zip"
unzip -q fmt.zip
cmake -S "fmt-${FMT_VERSION}" -B fmt-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DFMT_TEST=OFF -DFMT_DOC=OFF -DCMAKE_INSTALL_PREFIX="${PREFIX}"
cmake --build fmt-build --parallel
cmake --install fmt-build

BOOST_DIR="boost_${BOOST_VERSION//./_}"
curl -fsSL -o boost.tar.bz2 "https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_DIR}.tar.bz2"
tar xjf boost.tar.bz2
cd "${BOOST_DIR}"
./bootstrap.sh --with-libraries=program_options --prefix="${PREFIX}"
./b2 install -d0 -j"$(nproc)" link=shared variant=release cxxflags=-std=c++17
