#pragma once

#include "icemu/emu/Emulator.h"
#include <math.h>

class StackTracker {
 private:
  icemu::Emulator &_emu;
  bool valid = false;
  address_t min_stack_address = 0; // N.B. the stack grows by decresing the
                                   // address, so this is the min address

  address_t memory_start_address;

 public:
  icemu::Emulator &getEmulator() { return _emu; }
  StackTracker(icemu::Emulator &emu) : _emu(emu) {
    memory_start_address = emu.getMemory().entrypoint;
  }

  address_t getMinStackAddress() {
    if (!valid) {
      assert(false && "StackTracker not initialized correctly (not valid)");
    }
    return min_stack_address;
  }

  address_t getStackPointer() {
    return getEmulator().getArchitecture().registerGet(
        icemu::Architecture::REG_SP);
  }

  void resetMinStackAddress() { min_stack_address = getStackPointer(); }

  bool isMemoryWriteNeeded(address_t evict_address) {
    auto min_stack_address = getMinStackAddress();
    auto cur_stack_address = getStackPointer();

    // TODO: check offset
    // if (evict_address+8 < min_stack_address || evict_address+8 >
    // cur_stack_address) {
    if (evict_address <= min_stack_address ||
        evict_address + 4 >= cur_stack_address) {
      return true;
    }

    return false;
  }

  void run() {
    // To avoid weird MAX_INT init which WILL break somehow
    if (!valid) {
      min_stack_address = getStackPointer();
      if (min_stack_address >= memory_start_address)
        valid = true;

      return;
    }

    // Track the minimum address of the stack, i.e., the max size of the stack
    auto current_stack_address = getStackPointer();
    if (current_stack_address < min_stack_address) {
      min_stack_address = current_stack_address;
    }
  }
};
