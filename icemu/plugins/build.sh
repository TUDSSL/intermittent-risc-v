#!/bin/bash

echo "Building ICEmu plugins"
mkdir -p build
cd build
cmake ../ && make -j$(nproc)
