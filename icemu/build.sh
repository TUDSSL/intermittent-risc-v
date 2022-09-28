#!/bin/bash

set -e

# Update the submodules
#git submodule update --init --recursive icemu

# Build ICEmu
echo "Building ICEmu"
pushd icemu
# Build the dependencies
./setup-lib.sh
# Build ICEmu
rm -rf build
rm -rf plugins/build
mkdir build && cd build && cmake ../ && make -j"$(nproc)"
echo "Done building ICEmu"
popd

echo "Building ICEmu plugins"
pushd plugins
echo "$(pwd)"
rm -rf build
./build.sh
popd
