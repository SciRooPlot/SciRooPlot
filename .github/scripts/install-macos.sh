#!/usr/bin/env bash

set -euxo pipefail

brew update

brew install \
    boost \
    fmt \
    pybind11 \
    root
