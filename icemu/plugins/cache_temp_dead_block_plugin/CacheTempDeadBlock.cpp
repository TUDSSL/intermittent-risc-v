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
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>
#include <chrono>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Function.h"
#include "../includes/DetectWAR.h"
#include "../includes/Cache.hpp"

using namespace std;
using namespace icemu; 

uint64_t global_count = 0;

uint64_t return_instr_count()
{
  return global_count;
}

ofstream logger;

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
  private:
   // Config
   uint64_t instruction_count = 0;

  public:
    uint64_t pc = 0;

    explicit HookInstructionCount(Emulator &emu) : HookCode(emu, "stack-war")
    {
     
    }

    ~HookInstructionCount() override
    {

    }

    void run(hook_arg_t *arg)
    {
      (void)arg;  // Don't care
      instruction_count += 1;
      global_count++;
    }
};

// Struct to store the current instruction state
struct InstructionState {
  uint64_t pc;
  uint64_t icount;
  armaddr_t mem_address;
  armaddr_t mem_size;
};

// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
  public:
    HookInstructionCount *hook_instr_cnt;
    Cache CacheObj = Cache();
    DetectWAR war;
    string filename = "log/default_log";

  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru") {
      hook_instr_cnt = new HookInstructionCount(emu);
      parseLogArguements();
      parseCacheArguements();
      CacheObj.register_instr_count_fn(&return_instr_count);
      logger.open(filename.c_str(), ios::out | ios::app);
      cout << "Start of the cache" << endl;
  }

  ~MemoryAccess() {
      cout << "End of the cache" << endl;
      CacheObj.print_stats(logger);
  }

  void run(hook_arg_t *arg) {
    armaddr_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    armaddr_t value = arg->value;

    // Call the cache
    CacheObj.run(address, mem_type, &value);
  }
    
  void parseLogArguements() { 
      string argument_name = "log-file=";
      for (const auto &a : getEmulator().getPluginArguments().getArgs()) {
        auto pos = a.find(argument_name);
        if (pos != string::npos) {
          filename = "log/" + a.substr(pos+argument_name.length());
        }
      }

  }
  
  void parseCacheArguements() {
      uint32_t size = 512, lines = 2;
      string arg1 = "cache-size=", arg2 = "cache-lines=";
      for (const auto &a : getEmulator().getPluginArguments().getArgs()) {
        auto pos1 = a.find(arg1);
        if (pos1 != string::npos) {
          size = std::stoul(a.substr(pos1+arg1.length()));
        }

        auto pos2 = a.find(arg2);
        if (pos2 != string::npos) {
          lines = std::stoul(a.substr(pos2+arg2.length()));
        }
      }

      CacheObj.init(size, lines, LRU);
      filename += "_" + std::to_string(size) + "_" + std::to_string(lines);
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
