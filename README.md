# intermittent-risc-v

TODO: Come up with a better name

This repository contains the source code and benchmark results accompanying the NACHO paper.

## Repository Structure

- `.devcontainer/`: VS Code Dev Container configuration.
- `.vscode/`: VS Code (plugin) configuration.
- `benchmarks/`: a directory per benchmark.
  Benchmark configurations are specified in the .mk files.
- `bin/`: helper scripts that are put on `PATH` by `sourceme.sh`.
- `docker/`: Dockerfiles, e.g. for the Dev Container.
- `figures/`: figures rendered by the plotting notebooks.
- `host-bin/`: not used.
- `icemu/`: Intermittent Computing Emulator.
  - `icemu/`: upstream submodule.
  - `plugins/`: various plugins to extend ICEmu's behavior.
- `llvm/`:
  - `llvm-16.0.2/`: source code with out patches for the ReplayCache compiler.
  - `llvm-16.0.2-ref/`: reference source code of LLVM 16.0.2, used to diff against.
  - `llvm-16.0.2-bin/`: binary distribution of LLVM 16.0.2, used to build all non-ReplayCache benchmarks.
  - `diff.sh`: script to compute ReplayCache patch diffs.
- `plotting/`: Jupyter notebooks to analyze and plot benchmark outputs.
- `riscv/`: not used.
- `toolchain/`: CMake toolchain support, only `riscv32` is used.

## Development

Development is intended to happen primarily through VS Code in a Dev Container.
Assuming the Docker daemon is already running, you can open the repository root with VS Code and select the "Reopen in Container" option (which should be prompted).
This configures all relevant plugins and settings such that you can almost immediately start developing.

Building the project is primarily done through Make, which usually dispatches to CMake/Ninja again.
Before calling Make, you should first source the files `sourceme.sh` and `setup.sh` to prepare your (termina) environment:

```shell
$ . sourceme.sh
$ . setup.sh
```

Intially, run `make download` once to populate the LLVM binary distribution and setup the Git submodule for ICEmu.
After that, you can just run `make` (or `make all`) to build _and run_ everything.

In practice, during development, it is not necessary to (re)compile everything and run all benchmarks
For example, you could use the following custom script to run a single benchmark with ReplayCache:

```bash
set -e
export MAKEFLAGS="$MAKEFLAGS -j$(nproc)"
# Ensure the LLVM toolchain is compiled (incremental)
make llvm nodownload=1
# Ensure ICEmu plugins are built (incremental)
# Assuming ICEmu itself does not need recompilation
cmake --build icemu/plugins/build
# Ensure benchmarks are built (incremental)
make -C benchmarks clean build
# Run a benchmark
bench=aes
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
```
