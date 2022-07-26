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
#include <queue>
#include <list>
#include <set>
#include <map>
#include <unordered_set>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Function.h"
#include "../includes/DetectWAR.h"
#include "../includes/Cache.hpp"
#include "../includes/CycleCounter.h"

using namespace std;
using namespace icemu;

#define HINT_FUNCTION_NAME "__cache_hint"

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
    bool use_cycle_count = true;
    uint64_t instruction_count = 0;
    const char func_type = 2; // magic ELF function type
    Cache *obj; // pointer to cache object, comes from HookMemory
    CycleCounter cycleCounter;

  public:
    uint64_t pc = 0;
    uint64_t count = 0;

  const string unknown_function_str = "UNKNOWN_FUNCTION";
  struct FunctionFrame {
    const string *function_name = nullptr;
    armaddr_t function_address = 0;
    uint64_t function_entry_icount = 0;
    armaddr_t sp_function_entry = 0;
    armaddr_t LR = 0;
  };
  list<FunctionFrame> callstack;

  armaddr_t estack;

  // A boolean that is true on the first instruction of a new function
  // NB. This is hacky, but new_function acts like an ISR flag
  // it needs to be manually reset after reading
  bool new_function = true;
  list<FunctionFrame> function_entries; // is cleared by the HookIdempotencyStatistics class

  void clearNewFunction() {
    new_function = false;
    function_entries.clear();
  }

  // Stores addresses found in a function
  map<armaddr_t, const string *> function_map;
  // Stores the function and the starting address
  map<armaddr_t, const string *> function_entry_map;

  // A map holding ALL executed instructions and the number of times they have
  // been executed
  const bool track_instruction_execution = true;
  map<armaddr_t, uint64_t> instruction_execution_map;

  explicit HookInstructionCount(Emulator &emu) : HookCode(emu, "stack-war"),
                                                  cycleCounter(emu)
  {
    auto &symbols = getEmulator().getMemory().getSymbols();
    for (const auto &sym : symbols.symbols) {
      if (sym.type == func_type) {
        // To check for function entries only
        function_entry_map[sym.getFuncAddr()] = &sym.name;

        // Add ALL the possible addresses in the function to the map
        // Return the function name if it's in any of the addresses in the
        // function.
        // Assmumtions:
        //  * All opcodes are 16-bit (2 bytes) or more in multiple
        //  * Functions are continious (we use the size for functions)
        for (armaddr_t faddr = sym.getFuncAddr();
              faddr < sym.getFuncAddr() + sym.size;
              faddr += 2) {
          function_map[faddr] = &sym.name;
        }
      }
    }

    auto estack_sym = symbols.get("_estack");
    estack = estack_sym->address;
    cout << "Estack at: " << estack << endl;
  }

  ~HookInstructionCount() override
  {

  }

  // Return the current instruction count
  uint64_t getInstructionCount()
  {
    if (use_cycle_count == true)
      return cycleCounter.cycleCount();

    return instruction_count;
  }

  void register_cache(Cache *ext_obj)
  {
    obj = ext_obj;
  }

  void log_cache_content()
  {
    cout << "calling log all" << endl;
    obj->log_all_cache_contents(logger);
  }

  // Return the name of the function that the addr is the start of
  const string *isFunctionEntry(armaddr_t addr)
  {
    auto f = function_entry_map.find(addr);
    if (f != function_entry_map.end())
      return f->second;

    return nullptr;
  }

  // Is the instruction at the address a function?
  const string *inFunction(armaddr_t addr)
  {
    auto f = function_map.find(addr);
    if (f == function_map.end())
      return nullptr;

    return f->second;
  }

  void run(hook_arg_t *arg)
  {
    instruction_count += 1;
    global_count++;

    // See if the current instruction is a function call or not.
    auto fname = isFunctionEntry(arg->address);
    if (fname != nullptr) {
      // If the function is the cache hint, then parse the args.
      if (strcmp(fname->c_str(), HINT_FUNCTION_NAME) == 0) {
        auto regs = getEmulator().getRegisters();

        // Arg1 will have the address of the cache entry that needs to be marked.
        Function::Argument<uint32_t> farg1;
        Function::Arguments::parse(regs, farg1);

        obj->applyCompilerHints(farg1.arg);
      }
    }

    // Ignore executing the instructions the function is still executing.
    fname = inFunction(arg->address);
    if (fname != nullptr && strcmp(fname->c_str(), HINT_FUNCTION_NAME) == 0) {
      // How to tell the emulator to skip the instruction?
    }
  }

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

    // Register the cache object - could be done with OOPs but I like C style :(
    hook_instr_cnt->register_cache(&CacheObj);

    // Parse optional args
    parseLogArguements();
    parseCacheArguements();

    // Means to provide the instruction count to the cache
    CacheObj.register_instr_count_fn(&return_instr_count);
    logger.open(filename.c_str(), ios::out | ios::trunc);
    cout << "Start of the cache" << endl;
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
