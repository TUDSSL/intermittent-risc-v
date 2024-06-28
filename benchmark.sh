set -e
export MAKEFLAGS="$MAKEFLAGS -j$(nproc)"
# Ensure the LLVM toolchain is compiled (incremental)
# make llvm nodownload=1
# Ensure ICEmu plugins are built (incremental)
# Assuming ICEmu itself does not need recompilation
cmake --build icemu/plugins/build
# Ensure benchmarks are built (incremental)
# make -C benchmarks clean
# make -C benchmarks clean build
# Run a benchmark
bench=$1
exec run-elf \
    -p replay_cache_plugin.so \
    -a hash-method=0 \
    -a cache-size=8192 \
    -a cache-lines=2 \
    -a writeback-queue-size=8 \
    -a writeback-parallelism=1 \
    -a log-file=/tmp/replay-cache-$bench-log \
    -a on-duration=250000 \
    ./benchmarks/$bench/build-replay-cache/$bench.elf

