#pragma once

#include "icemu/emu/types.h"
#include "icemu/emu/Emulator.h"

template <class _Arch>
class AsyncWriteBackCache {

  using arch_addr_t = typename _Arch::riscv_addr_t;

 private:

  icemu::Emulator &emu;

 public:

  AsyncWriteBackCache(icemu::Emulator &emu)
      : emu(emu) {}

  void powerFailure() {
    // TODO:
    //  1. For each dirty block, write 0xDEADBEEF back to aid failure detection.
    //  2. Clear entire state, all cells are now empty and clean.
  }

  void tick(unsigned int cycles_passed) {
    // TODO:
    //  1. decrease pending writeback cycles
    //  2. if pending writeback cycles == 0, clear dirty bit
  }

  void handleRequest(address_t address, enum icemu::HookMemory::memory_type mem_type,
                     address_t *value_req, const address_t size_req) {
    // TODO:
    //  1. find destination cell
    //  2. evict if necessary
    //  3. if read, copy value from emu memory into cache and value_req
    //  4. if write, copy value from value_req into cache, and mark dirty
  }

  void handleReplay(arch_addr_t address, arch_addr_t value, const address_t size) {
    // TODO:
    //  1. use common write logic
  }

  /**
   * @brief Wait for all cache contents to be written back to memory.
   * @return The number of cycles spent waiting for all writebacks to complete.
   */
  unsigned int fence() {
    // TODO:
    //  1. for all dirty entries that are not pending to be written back, start to write them back
    //  2. find the maximum number of cycles required to write back all dirty entries
    //  3. clear all dirty bits
    //  4. return the number from step 2
    return 0;
  }

 private:

  // TODO: add methods to handle reads and writes given a resolved cache entry
};
