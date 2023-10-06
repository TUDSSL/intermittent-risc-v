#!/bin/bash

VER="16.0.2"
DOWNLOAD="wget"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
LLVM_SRC_DIR="$DIR/llvm-$VER/llvm"
LLVM_BIN_DIR="$DIR/llvm-$VER-bin"
LLVM_REF_DIR="$DIR/llvm-$VER-ref"

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
rm -f .download_llvm_$VER

rm -rf "$LLVM_BIN_DIR" *.tar.xz
$DOWNLOAD "https://github.com/llvm/llvm-project/releases/download/llvmorg-$VER/clang+llvm-$VER-x86_64-linux-gnu-ubuntu-22.04.tar.xz"
tar -xf "clang+llvm-$VER-x86_64-linux-gnu-ubuntu-22.04.tar.xz"
rm "clang+llvm-$VER-x86_64-linux-gnu-ubuntu-22.04.tar.xz"
mv "clang+llvm-$VER-x86_64-linux-gnu-ubuntu-22.04" "$LLVM_BIN_DIR"

mkdir -p "$LLVM_REF_DIR"
pushd "$LLVM_REF_DIR"
download_extract "llvm"
popd

pushd "$LLVM_SRC_DIR/tools"
download_extract "clang"
download_extract "lld"
popd

pushd "$LLVM_SRC_DIR/projects"
download_extract "compiler-rt"
download_extract "libcxx"
download_extract "libcxxabi"
download_extract "libunwind"
download_extract "openmp"
download_extract "polly"
popd

# Address https://discourse.llvm.org/t/unable-to-package-llvm-version-12-0-0-1-as-rpm-in-rhel7/4874 (see https://github.com/llvmenv/llvmenv/issues/115)
#ln -s "$LLVM_SRC_DIR/projects/libunwind/include/mach-o" "$LLVM_SRC_DIR/projects/lld/MachO/mach-o"


echo "Downloaded llvm components @$(date)" > .download_llvm_$VER
