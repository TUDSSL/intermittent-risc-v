#!/bin/bash

VER="9.0.1"
DOWNLOAD="wget"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
LLVM_SRC_DIR="$DIR/llvm-9.0.1/llvm"

function download_extract {
    fn="$1-$VER.src"
    tarn="$fn.tar.xz"
    $DOWNLOAD "https://github.com/llvm/llvm-project/releases/download/llvmorg-$VER/$tarn"
    echo "Untar $tarn"
    tar -xf "$tarn"
    rm "$tarn"
    rm -rf $1
    mv "$fn" "$1"
}

# File that marks that we downloaded and extracted all components
rm -f .download_llvm

pushd "$LLVM_SRC_DIR/tools"
download_extract "clang"
# Add the ability to specify ld.lld as the linker using -fuse-ld=lld
# https://reviews.llvm.org/D74704
cp "$DIR/patch/riscv-fuse-ld.patch" ./
patch -p0 -i riscv-fuse-ld.patch
rm ./riscv-fuse-ld.patch

popd

pushd "$LLVM_SRC_DIR/projects"
download_extract "compiler-rt"
download_extract "libcxx"
download_extract "libcxxabi"
download_extract "libunwind"
download_extract "lld"
download_extract "openmp"
download_extract "polly"

# Apply glibc patch for compiler-rt
# https://bugs.gentoo.org/708430
cp "$DIR/patch/compiler-rt-glibc.patch" ./
patch -p0 -i compiler-rt-glibc.patch
rm ./compiler-rt-glibc.patch

popd

echo "Downloaded llvm components @$(date)" > .download_llvm
