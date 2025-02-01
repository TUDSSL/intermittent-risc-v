#!/bin/bash

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export LLVM_VERSION="16.0.2"

export PATH="$ROOT_DIR/bin/:$PATH"
export PATH="$ROOT_DIR/host-bin/:$PATH"
export PATH="$ROOT_DIR/icemu/icemu/bin:$PATH"

export LLVM_BIN_ROOT="$ROOT_DIR/llvm/llvm-$LLVM_VERSION-bin"
export LLVM_RC_ROOT="$ROOT_DIR/llvm/llvm-$LLVM_VERSION/install"
export NOELLE="$ROOT_DIR/noelle/noelle/install"
export ICLANG_ROOT="$ROOT_DIR"
export ICEMU_PLUGIN_PATH="$ROOT_DIR/icemu/plugins/build/plugins"