#!/bin/bash

CFLAGS="--target=riscv64 -march=rv64gc -I /home/user/intermittent-risc-v/chipyard/chipyard/toolchains/riscv-tools/riscv-gnu-toolchain/riscv-newlib/newlib/libc/include -Iportme/ -I../../../ -mcmodel=medium -fno-common "

# Main.c
clang $CFLAGS -c towers/towers_main.c -o towers_main.o

# syscalls.c
clang $CFLAGS -c portme/syscalls.c -o syscalls.o

# crt
clang $CFLAGS -c portme/crt.S -o crt.o

# Link using gcc linker
#/home/user/intermittent-risc-v/chipyard/chipyard/riscv-tools-install/bin/riscv64-unknown-elf-gcc \
#crt.o syscalls.o towers_main.o \
#-nostartfiles -nostdlib -static -L/home/user/intermittent-risc-v/chipyard/chipyard/riscv-tools-install/riscv64-unknown-elf/lib/ \
#-T../../../linkerscript.ld \
#-lm -lgcc  -o towers.elf

# Link using clang
ld.lld crt.o syscalls.o towers_main.o  -T ../../../linkerscript.ld -o towers.lld.elf
