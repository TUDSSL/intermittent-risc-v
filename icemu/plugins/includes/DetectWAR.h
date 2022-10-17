#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>
#include <chrono>
#include <queue>
#include <list>
#include <set>
#include <map>
#include <unordered_set>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Architecture.h"

using namespace std;
using namespace icemu;

class DetectWAR {
 public:
  unordered_set<address_t> reads, writes;

  uint32_t checkpoints;

  DetectWAR() {
    reads.clear();
    writes.clear();
    checkpoints = 0;
  }

  ~DetectWAR() = default;

  bool isWAR(address_t addr, size_t size, HookMemory::memory_type type, bool can_set_write_dominated=true) {
    switch (type) {
    // If the current access type is read, just put it in the set
    case HookMemory::MEM_READ:
      return processRead(addr, size);
      break;

    // If the current access is a write
    //  - If the address is in 'writes' we are OK, it's write dominated
    //  - If the address is not in 'writes', but IS in 'reads', we have a WAR
    //  - If the address is not in 'writes' AND not in 'reads', we have a new
    //  access, add it to writes
    case HookMemory::MEM_WRITE:
      return processWrite(addr, size, can_set_write_dominated);
      break;
    }

    assert(false && "WAR detector, should not reach this");
  }

  void reset() {
    checkpoints++;
    reads.clear();
    writes.clear();
  }

 private:
  bool processRead(address_t addr, size_t size) {
    // Process all the bytes in the read access
    for (size_t i = 0; i < size; i++) {
      reads.insert(addr + i);
    }
    // Read can never cause a WAR
    return false;
  }

  bool processWrite(address_t addr, size_t size, bool can_set_write_dominated) {
    // Process all the bytes in the write access
    for (size_t i = 0; i < size; i++) {
      address_t byte_addr = addr + i;

      // Check if the address is in the reads
      bool isInReads = (reads.find(byte_addr) != reads.end());

      // Check if the addres is in the writes
      bool isInWrites = (writes.find(byte_addr) != writes.end());

      // Check for a WAR
      if (isInWrites) {
        // Write is already set. So it's write dominated and safe
        // (We only add to writes if it's the first access)
      } else if (isInReads) {
        // We have a WAR, addr is not in writes, but is in reads (and this is a
        // write)
        reset();
        return true;
      } else {
        // Is neither in writes or reads, new access. Add it to writes
        if (can_set_write_dominated)
          writes.insert(byte_addr);
      }
    }
    return false;
  }
};
