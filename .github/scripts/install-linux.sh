#!/usr/bin/env bash

set -euxo pipefail

sudo apt-get update

# APT_ARCHIVES: folder for the downloaded .deb files, kept between runs by actions/cache (the runners' mirror is at times very slow)
APT_OPTS=()
if [[ -n "${APT_ARCHIVES:-}" ]]; then
    mkdir -p "${APT_ARCHIVES}/partial"
    APT_OPTS=(-o "Dir::Cache::Archives=${APT_ARCHIVES}")
fi

sudo apt-get install -y --no-install-recommends "${APT_OPTS[@]}" \
    libboost-dev \
    libboost-program-options-dev \
    libfmt-dev \
    pybind11-dev \
    xvfb

# apt leaves its partial folder to the _apt user: make everything readable for saving the cache
if [[ -n "${APT_ARCHIVES:-}" ]]; then
    sudo chown -R "$(id -u):$(id -g)" "${APT_ARCHIVES}"
fi
