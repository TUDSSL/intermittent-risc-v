#!/bin/bash

# Build Verilator
pushd verilator
autoconf && ./configure && make -j$(nproc)
popd

# Initialize and build the chipyard submodule
pushd chipyard
./scripts/init-submodules-no-riscv-tools.sh && \
./scripts/build-toolchains.sh riscv-tools


# A submodule of QEMU has a branch named HEAD, which causes a lot of warnings in
# 'git status' (warning: refname 'HEAD' is ambiguous.)
# So we rename the branch to renamed-HEAD to get rid of them
pushd chipyard/toolchains/qemu/roms/edk2
git branch -m HEAD renamed-HEAD 2> /dev/null
popd

popd

