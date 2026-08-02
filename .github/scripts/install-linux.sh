#!/usr/bin/env bash

set -euxo pipefail

sudo apt-get update

sudo apt-get install -y \
    libboost-dev \
    libboost-program-options-dev \
    libfmt-dev \
    pybind11-dev
