linker = common/test.ld

CFLAGS32 = -Icommon/ \
    -ffreestanding \
    -D__riscv_xlen=32 \
    -mcmodel=medany \
    -march=rv32gc \
    -mabi=ilp32d \
    -static -std=gnu99 -O2 \
    -ffast-math -fno-common \
    -fno-builtin-printf \
    -fno-tree-loop-distribute-patterns

CFLAGS64 = -Icommon/ \
    -ffreestanding \
    -D__riscv_xlen=64 \
    -mcmodel=medany \
    -march=rv64gc \
    -mabi=ilp64d \
    -static -std=gnu99 -O2 \
    -ffast-math -fno-common \
    -fno-builtin-printf \
    -fno-tree-loop-distribute-patterns

LDFLAGS = -static -nostdlib -nostartfiles -lgcc -lm -T $(linker)

riscv32:
	@mkdir -p build32
	riscv32-unknown-elf-gcc $(CFLAGS32) -c common/crt.S -o build32/crt.o
	riscv32-unknown-elf-gcc $(CFLAGS32) -c common/syscalls.c -o build32/syscalls.o
	riscv32-unknown-elf-gcc $(CFLAGS32) -c test.c -o build32/test.o
	riscv32-unknown-elf-gcc build32/crt.o build32/syscalls.o build32/test.o -o build32/test $(LDFLAGS) 

riscv64:
	@mkdir -p build64
	riscv64-unknown-elf-gcc $(CFLAGS64) -c common/crt.S -o build64/crt.o
	riscv64-unknown-elf-gcc $(CFLAGS64) -c common/syscalls.c -o build64/syscalls.o
	riscv64-unknown-elf-gcc $(CFLAGS64) -c test.c -o build64/test.o
	riscv64-unknown-elf-gcc build64/crt.o build64/syscalls.o build64/test.o -o build64/test $(LDFLAGS)

clean:
	rm -rf build64/* build32/*
