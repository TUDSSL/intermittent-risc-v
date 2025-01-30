#!/bin/bash

# Check input arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <benchmark> <optimization_level> <system>"
    echo "Example: $0 aes O0 nacho"
    echo "Systems: nacho, replaycache"
    echo "Important: Please ensure that ICEmu plugins and LLVM toolchain are compiled"
    exit 1
fi

set -e
export MAKEFLAGS="$MAKEFLAGS -j$(nproc)"

# Ensure the LLVM toolchain is compiled (incremental)
make llvm nodownload=1

# Ensure ICEmu plugins are built (incremental)
# Assuming ICEmu itself does not need recompilation
cmake --build icemu/plugins/build

# Ensure benchmarks are built (incremental)
make -C benchmarks clean
make -C benchmarks clean build

# Assign command-line arguments
bench=$1
opt_lvl=$2
system=$3

# Common parameters but we can make it as args if needed
cache=512
lines=2
on_duration=0

# Select the appropriate system and plugin
if [ "$system" == "nacho" ]; then
    plugin="custom_cache_plugin.so"
    log_file="/tmp/uninstrumented-$bench-log"
    extra_args="-a enable-pw-bit=1 -a enable-stack-tracking=2"
elif [ "$system" == "replaycache" ]; then
    plugin="replay_cache_plugin.so"
    log_file="/tmp/replay-cache-$bench-log"
    extra_args="-a writeback-queue-size=8 -a writeback-parallelism=1 -a on-duration=$on_duration"
else
    echo "Error: Invalid system '$system'. Choose either 'nacho' or 'replaycache'."
    exit 1
fi

# Run the benchmark
exec run-elf \
    -p $plugin \
    -a hash-method=0 \
    -a cache-size=$cache \
    -a cache-lines=$lines \
    -a log-file=$log_file \
    -a opt-level=$opt_lvl \
    $extra_args \
    ./benchmarks/$bench/build-replay-cache-$opt_lvl/$bench.elf
