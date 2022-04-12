#!/bin/bash

# Update the submodules
git submodule update --init --recursive

# Build ICEmu
echo "Building ICEmu"
cd icemu
mkdir build && cd build && cmake ../ && make -j"$(nproc)"
echo "Done building ICEmu"
