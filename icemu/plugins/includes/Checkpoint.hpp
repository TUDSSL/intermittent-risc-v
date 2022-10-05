#pragma once
/*
 * Create and restore a checkpoint within ICEmu
 */

#include "icemu/emu/Emulator.h"
#include "Utils.hpp"

class Checkpoint {
 private:
   icemu::Emulator &emu;

   static const size_t checkpoint_size = 32+2;
   uint32_t SavedRegisters[checkpoint_size];
  
   icemu::memseg_t *main_memseg;
   size_t memory_size;
   uint8_t *SavedMemory;

 public:
  size_t count = 0;

  Checkpoint(icemu::Emulator &emu) : emu(emu) {
    main_memseg = emu.getMemory().find("RWMEM");
    assert((main_memseg != nullptr) && "Could not find memory segment RWMEM");

    memory_size = main_memseg->length;

    // Allocate space
    SavedMemory = new uint8_t[main_memseg->allocated_length];
  }

  ~Checkpoint() {
    delete[] SavedMemory;
  }

  void print() {
    print(SavedRegisters);
  }

  void print(uint32_t *cp) {
    for (int i=0; i<32; i++) {
      std::cout << "REG_" << i << cp[i] << std::endl;
    }
    std::cout << "REG_PC" << cp[32] << std::endl;
    std::cout << "REG_MSTATUS" << cp[33] << std::endl;
  }

  /*
   * Because the emulator memory is our shadow memory, we also need to checkpoint and restore
   * it to verify OUR memory (which should automatically restore)
   */
  void createShadowMemoryCheckpoint() {
    memcpy(SavedMemory, main_memseg->data, memory_size);
  }
  void restoreShadowMemoryCheckpoint() {
    memcpy(main_memseg->data, SavedMemory, memory_size);
  }

  /*
   * Create a checkpoint and return the size of the checkpoint
   */
  int create() {
    p_debug << "Checkpoint" << std::endl;
    ++count;
    createShadowMemoryCheckpoint();
    return create(SavedRegisters);
  }

  int create(uint32_t *dst) {
    auto arch = emu.getArchitecture().getRiscv32Architecture();

    // Save all the registers
    dst[0] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X0);
    dst[1] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X1);
    dst[2] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X2);
    dst[3] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X3);
    dst[4] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X4);
    dst[5] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X5);
    dst[6] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X6);
    dst[7] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X7);
    dst[8] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X8);
    dst[9] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X9);
    dst[10] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X10);
    dst[11] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X11);
    dst[12] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X12);
    dst[13] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X13);
    dst[14] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X14);
    dst[15] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X15);
    dst[16] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X16);
    dst[17] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X17);
    dst[18] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X18);
    dst[19] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X19);
    dst[20] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X20);
    dst[21] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X21);
    dst[22] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X22);
    dst[23] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X23);
    dst[24] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X24);
    dst[25] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X25);
    dst[26] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X26);
    dst[27] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X27);
    dst[28] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X28);
    dst[29] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X29);
    dst[30] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X30);
    dst[31] = arch.registerGet(icemu::ArchitectureRiscv32::REG_X31);

    auto reg_pc   = arch.registerGet(icemu::ArchitectureRiscv32::REG_PC);
    auto reg_mstatus   = arch.registerGet(icemu::ArchitectureRiscv32::REG_MSTATUS);
    dst[32] = reg_pc;
    dst[33] = reg_mstatus;

    //std::cout << "RegisterCheckpoint at address: 0x" << std::hex << reg_pc << std::dec << std::endl;

    return checkpoint_size;
  }

  /*
   * Restore a checkpoint and return the size of the checkpoint
   */
  int restore() {
    restoreShadowMemoryCheckpoint();
    return restore(SavedRegisters);
  }

  int restore(uint32_t *src) {
    auto arch = emu.getArchitecture().getRiscv32Architecture();

    // Restore all the registers
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X0,  src[0]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X1,  src[1]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X2,  src[2]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X3,  src[3]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X4,  src[4]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X5,  src[5]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X6,  src[6]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X7,  src[7]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X8,  src[8]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X9,  src[9]); 
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X10, src[10]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X11, src[11]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X12, src[12]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X13, src[13]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X14, src[14]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X15, src[15]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X16, src[16]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X17, src[17]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X18, src[18]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X19, src[19]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X20, src[20]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X21, src[21]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X22, src[22]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X23, src[23]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X24, src[24]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X25, src[25]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X26, src[26]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X27, src[27]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X28, src[28]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X29, src[29]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X30, src[30]);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_X31, src[31]);

    auto reg_pc = src[32];
    auto reg_mstatus = src[33];

    arch.registerSet(icemu::ArchitectureRiscv32::REG_PC, reg_pc);
    arch.registerSet(icemu::ArchitectureRiscv32::REG_MSTATUS, reg_mstatus);

    return checkpoint_size;
  }

};
