#!/bin/bash

# For now libgcc is used when compiling the ARM elf files
libgcc_loc=$(arm-none-eabi-gcc -print-libgcc-file-name)
libgcc_loc=$(dirname "$libgcc_loc")
export CMAKE_LIBGCC_ARM_BASE_DIR="$libgcc_loc"
