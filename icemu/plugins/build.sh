#!/bin/bash

echo "Building ICEmu plugins"
cmake -S . -B build
cmake --build build
