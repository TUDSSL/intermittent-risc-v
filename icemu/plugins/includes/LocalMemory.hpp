#pragma once

#include <bitset>
#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <math.h>
#include <queue>
#include <set>
#include <string.h>
#include <unordered_set>
#include <vector>

#include "../includes/Utils.hpp"
#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"

using namespace std;
using namespace icemu;

static const bool PRINT_MEMORY_DIFF = false;

class LocalMemory {
 private:
  unordered_set<address_t> reads, writes;
  memseg_t *MainMemSegment = nullptr;
  uint8_t *LocalMem = nullptr;
  uint8_t *EmuMem = nullptr;
  icemu::Memory *mem;

 public:
  LocalMemory() = default;
  ~LocalMemory() { delete[] LocalMem; }

  // Initialize the local memory and store a copy of the same
  void initMem(icemu::Memory *mem) {
    this->mem = mem;
    auto code_entrypoint = mem->entrypoint;

    // Get the memory segment holding the main code (assume it also holds the RAM)
    MainMemSegment = mem->find(code_entrypoint);
    assert(MainMemSegment != nullptr);

    // Create Local memory
    LocalMem = new uint8_t[MainMemSegment->length];
    assert(LocalMem != nullptr);

    // Store the emulator memory start address
    EmuMem = MainMemSegment->data;

    // Populate the shadow memory
    memcpy(LocalMem, MainMemSegment->data, MainMemSegment->length);

    compareMemory(true);
  }

  bool compareMemory(bool assert) {
    // First do a fast memcmp (assume it's optimized)
    int compareValue = memcmp(LocalMem, MainMemSegment->data, MainMemSegment->length);
    if (compareValue == 0) {
      // Memory is the same
      return true;
    }

    // Something is different according to `memcmp`, check byte-per-byte
    bool same = true;
    for (size_t i = 0; i < MainMemSegment->length; i++) {
      if (LocalMem[i] != MainMemSegment->data[i]) {
        // Memory is different
        same = false;

        if (PRINT_MEMORY_DIFF) {
          address_t addr = MainMemSegment->origin + i;
          address_t emu_val = MainMemSegment->data[i];
          address_t shadow_val = LocalMem[i];
          p_err << "[mem] memory location at 0x" << hex << addr
                << " differ - Emulator: 0x" << emu_val << " Shadow: 0x"
                << shadow_val << dec << endl;
        }
      }
    }

    if (assert)
      assert(compareValue == 0);

    return same;
  }

  // Write the value of the given size to the local memory copy
  void localWrite(address_t address, address_t value, address_t size) {
    p_debug << "[NVM Write] writing to addr: " << hex << address
            << " data: " << value << dec << endl;
    address_t address_idx = address - MainMemSegment->origin;
    for (address_t i = 0; i < size; i++) {
      uint64_t byte = (value >> (8 * i)) & 0xFF; // Get the bytes
      LocalMem[address_idx + i] = byte;
    }
    // cout << "[shadow write] wrote " << hex <<
    // (address_t)LocalMem[address_idx] << " to mem: " << hex << address_idx +
    // MainMemSegment->origin << dec << endl;
  }

  // Read the local memory copy from the address specified
  uint64_t localRead(address_t address, address_t size) {
    address_t address_idx = address - MainMemSegment->origin;
    uint64_t data = 0;
    for (address_t i = 0; i < size; i++) {
      uint64_t byte = (LocalMem[address_idx + i]);
      data = (byte << (8 * i)) | data;
    }

    return data;
  }

  // Read the value from the emulator memory
  uint64_t emulatorRead(address_t address, address_t size) {
    address_t address_idx = address - MainMemSegment->origin;
    uint64_t data = 0;
    for (address_t i = 0; i < size; i++) {
      uint64_t byte = (EmuMem[address_idx + i]);
      data = (byte << (8 * i)) | data;
    }

    return data;
  }
};
