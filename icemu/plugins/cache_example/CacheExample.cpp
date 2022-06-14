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
#include "icemu/emu/Function.h"

// Local includes
#include "../includes/DetectWAR.h"
#include "../includes/CacheSkewAssociative.hpp"
// #include "../includes/Cache.hpp"
#include "../includes/CycleCounter.h"

using namespace std;
using namespace icemu;


uint64_t instruction_count = 0;
uint64_t return_instr_count()
{
  return instruction_count;
}

ofstream logger;

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
  private:
    // Config
    Cache *obj;

  public:
    uint64_t pc = 0;
    uint64_t count = 0;

  explicit HookInstructionCount(Emulator &emu) : HookCode(emu, "stack-war")
  {

  }

  ~HookInstructionCount() override
  {

  }

  void register_cache(Cache *ext_obj)
  {
    obj = ext_obj;
  }

  void log_cache_content()
  {
    obj->log_all_cache_contents(logger);
  }

  void run(hook_arg_t *arg)
  {
    instruction_count += 1;
  }

};

// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
  public:
    HookInstructionCount *hook_instr_cnt;

    // Create cache object
    Cache CacheObj = Cache();

    // Create WAR detection object
    DetectWAR war;

    string filename = "log/default_log";

  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru") {
    hook_instr_cnt = new HookInstructionCount(emu);

    // Register the cache object - could be done with OOPs but I like C style :(
    hook_instr_cnt->register_cache(&CacheObj);

    // Parse optional args
    parseLogArguements();
    parseCacheArguements();

    // Means to provide the instruction count to the cache
    CacheObj.register_instr_count_fn(&return_instr_count);

    // Open the file for logging
    logger.open(filename.c_str(), ios::out | ios::trunc);
  }

  ~MemoryAccess() {
    cout << "End of the cache" << endl;
  }

  void run(hook_arg_t *arg) {
    armaddr_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    armaddr_t value = arg->value;

    // Call the cache
    CacheObj.run(address, mem_type, &value);
    // Log if needed
    CacheObj.log_all_cache_contents(logger);
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
