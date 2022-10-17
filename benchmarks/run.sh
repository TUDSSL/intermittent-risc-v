#!/bin/bash

# Threads to use
threads=$(($(nproc) - 1))

echo "Running benchmarks with $threads threads"

#
# Run all the benchmarks and store the logs
#

# Continuous power benchmarks
make -f make-run.mk run-targets -j$threads |& tee "run-log.txt"

# Power failure benchmarks
make -f make-run.mk run-pf-targets -j$threads |& tee "run-pf-log.txt"

echo "Done running benchmarks"
