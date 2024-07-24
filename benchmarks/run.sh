#!/bin/bash

# Threads to use
threads=$(($(nproc) - 1))

echo "Running benchmarks with $threads threads"

#
# Run all the benchmarks and store the logs
#

# Continuous power benchmarks
# make -f make-run.mk run-targets -j$threads |& tee "run-log.txt"
# make -f make-run.mk run-targets-replay-cache -j$threads |& tee "run-rc-log.txt"
# make -f make-run.mk run-targets-replay-cache-baseline -j$threads |& tee "run-rcb-log.txt"
make -f make-run.mk run-opt-targets -j$threads |& tee "run-opt-log.txt"

# Power failure benchmarks
# make -f make-run.mk run-pf-targets -j$threads |& tee "run-pf-log.txt"

echo "Done running benchmarks"
