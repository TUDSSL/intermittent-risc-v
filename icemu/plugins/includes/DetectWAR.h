#ifndef _DETECT_WAR
#define _DETECT_WAR

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
#include "CycleCounter.h"

using namespace std;
using namespace icemu;

class DetectWAR{
  public:
    unordered_set<address_t> reads, writes;

  public:
    uint32_t checkpoints;

    DetectWAR()
    {
      reads.clear();
      writes.clear();
      checkpoints = 0;
    }

    ~DetectWAR() = default;

    bool isWAR(address_t addr, HookMemory::memory_type type)
    {
      bool possibleWAR = false;
      auto readLoc = reads.find(addr);
      auto writeLoc = writes.find(addr);

      // Check if the reads is in the reads list
      bool isInReads = (readLoc != reads.end());
      // Check if the reads is in the writes list
      bool isInWrites = (writeLoc != writes.end());

      switch (type) {
        // If the current access type is read, just put it in the list
        case HookMemory::MEM_READ:
          if (isInReads)
            reads.erase(readLoc);
          reads.insert(addr);
          break;

        // If the current type is write, do stuffs
        case HookMemory::MEM_WRITE:
          // WAR scenario - R...W access
          if (isInReads && !isInWrites) {
            // cout << "R...W" << endl;
            possibleWAR = true;
            reset();
          }
          // Safe scenario - W...W or W..R..W access
          else if (isInReads || isInWrites) {
            // cout << "W...W or W..R..W" << endl;
            possibleWAR = false;
            writes.erase(writeLoc);
          }
          // Neither in read or write - new entry.
          else {
            // cout << "Just W" << endl;
            possibleWAR = false;
          }

          // Insert the write
          writes.insert(addr);
          break;
      }

      return possibleWAR;
    }

    void reset()
    {
      checkpoints++;
      reads.clear();
      writes.clear();
    }
};

#endif /* _DETECT_WAR */