#!/bin/bash

# Update the submodules
git submodule update --init --recursive icemu

# Build ICEmu
echo "Building ICEmu"
cd icemu
# Build the dependencies
./setup-lib.sh
# Build ICEmu
mkdir -p build && cd build && cmake ../ && make -j"$(nproc)"
echo "Done building ICEmu"
