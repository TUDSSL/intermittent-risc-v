#!/bin/bash

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export PATH="$ROOT_DIR/bin/:$PATH"
export PATH="$ROOT_DIR/host-bin/:$PATH"
export PATH="$ROOT_DIR/llvm/llvm-9.0.1/install/bin:$PATH"
export PATH="$ROOT_DIR/noelle/noelle/install/bin:$PATH"
export PATH="$ROOT_DIR/icemu/icemu/bin:$PATH"
export PATH="$ROOT_DIR/chipyard/verilator/bin:$PATH"

export LLVM_ROOT="$ROOT_DIR/llvm/llvm-9.0.1/install/"
export NOELLE="$ROOT_DIR/noelle/noelle/install"
export ICLANG_ROOT="$ROOT_DIR"
export VERILATOR_ROOT="$ROOT_DIR/chipyard/verilator"
export ICEMU_PLUGIN_PATH="$ROOT_DIR/icemu/plugins/build/plugins"

# For now libgcc is used when compiling the ARM elf files
libgcc_loc=$(arm-none-eabi-gcc -print-libgcc-file-name)
libgcc_loc=$(dirname "$libgcc_loc")
export CMAKE_LIBGCC_ARM_BASE_DIR="$libgcc_loc"

