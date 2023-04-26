#!/bin/bash

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export PATH="$ROOT_DIR/host-bin/:$PATH"

export MAKEFLAGS="$MAKEFLAGS -j$(nproc)"

export CMAKE_GENERATOR=Ninja
