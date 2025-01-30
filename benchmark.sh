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

# Benchmark parameters
bench=$1
opt_lvl=$2
cache=512
lines=2
on_duration=0

# Run a benchmark (NACHO)
exec run-elf \
        -p custom_cache_plugin.so \
        -a hash-method=0 \
        -a cache-size=$cache \
        -a cache-lines=$lines \
        -a custom-cache-log-file=/tmp/uninstrumented-$bench-log \
        -a enable-pw-bit=1 \
        -a enable-stack-tracking=2 \
        -a opt-level=$opt_lvl \
        ./benchmarks/$bench/build-uninstrumented-$opt_lvl/$bench.elf

# Run a benchmark (ReplayCache)
# exec run-elf \
#     -p replay_cache_plugin.so \
#     -a hash-method=0 \
#     -a cache-size=$cache \
#     -a cache-lines=$lines \
#     -a writeback-queue-size=8 \
#     -a writeback-parallelism=1 \
#     -a log-file=/tmp/replay-cache-$bench-log \
#     -a on-duration=$on_duration \
#     ./benchmarks/$bench/build-replay-cache-$opt_lvl/$bench.elf

