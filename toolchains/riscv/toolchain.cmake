# CMake toolchain file for Clang

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# libgcc is required to compile with clang/llvm for now
set(LIBGCC_BASE_DIR  $ENV{CMAKE_LIBGCC_RISCV_BASE_DIR})

set(CONFIG_DIR "$ENV{ICLANG_ROOT}/toolchains/riscv")

set(OUTPUT_SUFFIX ".elf" CACHE STRING "")
set(LINKER_SCRIPT "${CONFIG_DIR}/linkerscript.ld")

set(STARTUP_CODE "${CONFIG_DIR}/crt.S")
set(SYSCALL_CODE "${CONFIG_DIR}/syscalls.c")
set(PRINTF_CODE "${CONFIG_DIR}/printf.c")

set(ALL_TOOLCHAIN_CODE 
    "${STARTUP_CODE}"
    "${SYSCALL_CODE}"
    "${PRINTF_CODE}"
    )

set(CMAKE_C_COMPILER    "clang")
#set(CMAKE_CXX_COMPILER  "clang++")
set(CMAKE_AR            "llvm-ar")
set(CMAKE_LINKER        "ld.lld")
set(CMAKE_NM            "llvm-nm")
set(CMAKE_OBJDUMP       "llvm-objdump")
set(CMAKE_STRIP         "llvm-strip")
set(CMAKE_RANLIB        "llvm-ranlib")
set(CMAKE_SIZE          "llvm-size")

# General compiler flags
add_compile_options(
    --target=riscv64
    -march=rv64gc
    -mcmodel=medium
    -I${CONFIG_DIR}
    -I${CONFIG_DIR}/lib/include
    -fno-common
    -ffunction-sections
    -fdata-sections
    )

# Toolchain include directories
 include_directories(
     )

# Device linker flags
add_link_options(
    --target=riscv64
    -march=rv64gc
    -fuse-ld=lld
    -nostartfiles
    -nodefaultlibs
    -nostdlib
    -static
    -T${CONFIG_DIR}/linkerscript.ld

    ## Add the config dir
    -L${CONFIG_DIR}
    # Add gcc lib
    #-L${CONFIG_DIR}/lib/gcc
    # Add standard lib
    #-L${CONFIG_DIR}/lib/libc

    # Remove unused functions
    -Wl,--gc-sections

    # Link libraries
    -Wl,--Bstatic
    #-Wl,-lnosys
    #-Wl,-lgcc
    )

