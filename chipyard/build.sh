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

# Two chipyard generators use relative path symbolic links for the .gitignore
# This creates warnings during 'git status'
#   warning: unable to access 'behav/BramDwc/.gitignore': Too many levels of symbolic links
#   warning: unable to access 'synth/BramDwc/.gitignore': Too many levels of symbolic links
# Resolve it by copying the .gitignore files
pushd generators/cva6/src/main/resources/vsrc/cva6/src/fpga-support/behav/BramDwc
mv ../common/gitignore .gitignore
popd
pushd generators/cva6/src/main/resources/vsrc/cva6/src/fpga-support/synth/BramDwc
mv ../common/gitignore .gitignore
popd

# Done
popd

