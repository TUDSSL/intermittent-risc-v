/**
 *  An example cache implementation.
 *  ~ Sourav Mohapatra
 */

// C++ includes
#include <map>
#include <iostream>
#include <fstream>
#include <string>
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

// ICEmu includes
#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Architecture.h"

#include "Riscv32E21Pipeline.hpp"
#include "PluginArgumentParsing.h"

// Local includes
#include "../includes/DetectWAR.h"
#include "../includes/CacheMem.hpp"

using namespace std;
using namespace icemu;

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
  private:
    // Config
    Cache *obj;
    RiscvE21Pipeline Pipeline;

  public:
    uint64_t pc = 0;
    uint64_t count = 0;

  explicit HookInstructionCount(Emulator &emu) : HookCode(emu, "stack-war"), Pipeline(emu)
  {
    Pipeline.setVerifyJumpDestinationGuess(false);
    Pipeline.setVerifyNextInstructionGuess(false);
  }

  ~HookInstructionCount() override
  {
    cout << "Total cycle count: " << Pipeline.getTotalCycles() << endl; 
  }

  void register_cache(Cache *ext_obj)
  {
    obj = ext_obj;
  }

  void run(hook_arg_t *arg)
  {
    (void)arg;
    Pipeline.add(arg->address, arg->size);
    obj->updateCycleCount(Pipeline.getTotalCycles());
  }

};

// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
  public:
    HookInstructionCount *hook_instr_cnt;

    // Create cache object
    Cache CacheObj = Cache();

    string filename = "log/default_log";

  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru") {
    hook_instr_cnt = new HookInstructionCount(emu);

    // Register the cache object - could be done with OOPs but I like C style :(
    hook_instr_cnt->register_cache(&CacheObj);

    // Parse optional args
    parseLogArguements();
    parseCacheArguements();
  }

  ~MemoryAccess() {
    cout << "End of the cache" << endl;
  }

  void run(hook_arg_t *arg) { 
    address_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    address_t value = arg->value;

    if (arg->mem_type == MEM_READ) {
      char *read_value = getEmulator().getMemory().at(arg->address);
      memcpy(&arg->value, read_value, arg->size); 
    }

    address_t main_memory_start = getEmulator().getMemory().entrypoint;

    // Process only valid memory
    if (!((address >= main_memory_start)))
        return;

    // cout << "Memory type: " << arg->mem_type << "\t size: " << arg->size << "\t address: " << arg->address << "\tData: " << arg->value << endl;
    // return;

    // Call the cache
    CacheObj.run(address, mem_type, &value, arg->size);
  }
    
  void parseLogArguements() {
      if (PluginArgumentParsing::GetArguments(getEmulator(), "custom-cache-log-file=", ".log").args.size())
        filename = PluginArgumentParsing::GetArguments(getEmulator(), "custom-cache-log-file=", ".log").args[0];
  }
  
  void parseCacheArguements() {
      // Default values
      uint32_t size = 512, lines = 2;
      string arg1 = "cache-size=", arg2 = "cache-lines=";
      
      if (PluginArgumentParsing::GetArguments(getEmulator(), arg1).args.size())
        size = std::stoul(PluginArgumentParsing::GetArguments(getEmulator(), arg1).args[0]);

      if (PluginArgumentParsing::GetArguments(getEmulator(), arg2).args.size())
        lines = std::stoul(PluginArgumentParsing::GetArguments(getEmulator(), arg2).args[0]);

      filename += "_" + std::to_string(size) + "_" + std::to_string(lines);
      CacheObj.init(size, lines, LRU, getEmulator().getMemory(), filename);
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
