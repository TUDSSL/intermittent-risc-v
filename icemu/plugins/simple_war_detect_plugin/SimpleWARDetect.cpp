/**
 *  ICEmu loadable plugin (library)
 *
 * An example ICEmu plugin that is dynamically loaded.
 * This example prints the address of each instruction that is executed.
 *
 * Should be compiled as a shared library, i.e. using `-shared -fPIC`
 */


/**
 * Different stats that can be fetched
 * - Cache hits
 * - Cache misses
 * - No of Read-evictions
 * - No of Write-evictions
 * - No of write-evictions that had a read
 */

/**
 * Next to do
 * 1. Evict all the dirty writes at once.
 * 2. Get the stats for the number of reads/writes to NVM
 * 3. Create a nice stats class
 * 4. Get the number of checkpoints
 */

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>
#include <chrono>

#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Architecture.h"

#include "../includes/DetectWAR.h"
#include "../includes/Stats.hpp"
#include "../includes/Logger.hpp"
#include "PluginArgumentParsing.h"

using namespace std;
using namespace icemu;

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
 public:
  uint64_t count = 0;

  HookInstructionCount(Emulator &emu) : HookCode(emu, "icnt-ratio") {
  }

  ~HookInstructionCount() {
  }

  void run(hook_arg_t *arg) {
    (void)arg;  // Don't care
    ++count;
  }
};


// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
 public:
  HookInstructionCount *hook_instr_cnt;
  DetectWAR war;
  Logger log;
  Stats stats;
  

  MemoryAccess(Emulator &emu) : HookMemory(emu, "memory-access-ratio") {
    hook_instr_cnt = new HookInstructionCount(emu);
    string filename;
    

    auto arg1_val = PluginArgumentParsing::GetArguments(getEmulator(), "clank-log-file=");
      if (arg1_val.size())
        filename = arg1_val[0];
    
    log.init(filename, SET_ASSOCIATIVE);
  }

  ~MemoryAccess() {
      stats.printStats();
      log.printEndStats(stats);
  }

  void run(hook_arg_t *arg) {
    address_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    address_t value = arg->value;

    switch (mem_type) {
      case MEM_READ:
        stats.incNVMReads(arg->size);
        break;
      
      case MEM_WRITE:
        stats.incNVMWrites(arg->size);
        break;
    }

    if (war.isWAR(address, mem_type)) {
      stats.incCheckpoints();
      stats.incCheckpointsDueToWAR();
      war.reset();
    }
  }
};

// Function that registers the hook
static void registerMyCodeHook(Emulator &emu, HookManager &HM) {
  auto mf = new MemoryAccess(emu);
  if (mf->getStatus() == Hook::STATUS_ERROR) {
    delete mf->hook_instr_cnt;
    delete mf;
    return;
  }
  HM.add(mf->hook_instr_cnt);
  HM.add(mf);
}

// Class that is used by ICEmu to finf the register function
// NB.  * MUST BE NAMED "RegisterMyHook"
//      * MUST BE global
RegisterHook RegisterMyHook(registerMyCodeHook);
