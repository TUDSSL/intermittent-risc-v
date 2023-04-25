#!/bin/bash

set -e

# Update the submodules
#git submodule update --init --recursive icemu

# Build ICEmu
echo "Building ICEmu"
pushd icemu
# Build the dependencies
CMAKE_GENERATOR= ./setup-lib.sh
# Build ICEmu
rm -rf build
rm -rf plugins/build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
echo "Done building ICEmu"
popd

echo "Building ICEmu plugins"
pushd plugins
echo "$(pwd)"
rm -rf build
CMAKE_GENERATOR= ./build.sh
popd
