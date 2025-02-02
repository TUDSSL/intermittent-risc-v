#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

LLVM_SRC_DIR="$DIR/llvm-16.0.2"
BUILD_DIR="$LLVM_SRC_DIR/build"
INSTALL_DIR="$LLVM_SRC_DIR/install"

echo "Building LLVM"
echo "Build dir: $BUILD_DIR"
echo "Install dir: $INSTALL_DIR"

# Configure step (runs quickly on follow-up runs)
echo "Configuring LLVM"
CXX=clang++-12 CC=clang-12 cmake  \
    -Wno-dev \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_TARGETS_TO_BUILD="ARM;X86;RISCV" \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_BUILD_TESTS=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_BUILD_BENCHMARKS=OFF \
    -DLLVM_BUILD_DOCS=OFF \
    -DLLVM_PARALLEL_LINK_JOBS=4 \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
    -DOPENMP_ENABLE_LIBOMPTARGET=OFF \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    -S "$LLVM_SRC_DIR"/llvm \
    -B "$BUILD_DIR"

# Address https://github.com/llvmenv/llvmenv/issues/115
ln -sf "$LLVM_SRC_DIR/llvm/projects/libunwind/include/mach-o" \
       "$LLVM_SRC_DIR/llvm/tools/lld/MachO/mach-o"

# build step
echo "Building LLVM"
cmake --build "$BUILD_DIR" --target RISCVCommonTableGen
cmake --build "$BUILD_DIR"
cmake --install "$BUILD_DIR"
