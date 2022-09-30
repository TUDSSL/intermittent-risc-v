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
// #include "../includes/CacheProwl.hpp"

using namespace std;
using namespace icemu;

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
  private:
    // Config
    Cache *obj;
    RiscvE21Pipeline Pipeline;

    // Power failure emulation
    uint64_t on_duration = 0;
    uint64_t reset_cycle_target = 0;
    uint64_t last_reset_checkpoint_count = 0;

    // Periodic checkpoints
    uint64_t checkpoint_period = 0;

  public:
    uint64_t pc = 0;
    uint64_t count = 0;

  explicit HookInstructionCount(Emulator &emu) : HookCode(emu, "stack-war"), Pipeline(emu)
  {
    Pipeline.setVerifyJumpDestinationGuess(false);
    Pipeline.setVerifyNextInstructionGuess(false);

    // Get the on duration from the arguments
    auto arg_on_duration = PluginArgumentParsing::GetArguments(getEmulator(), "on-duration=");
    if (arg_on_duration.size())
      on_duration = std::stoul(arg_on_duration[0]);

    // Configure the next reset_cycle_target
    reset_cycle_target += on_duration;

    // Get the checkpoint cycle threshold
    auto arg_checkpoint_period = PluginArgumentParsing::GetArguments(getEmulator(), "checkpoint-period=");
    if (arg_checkpoint_period.size())
      checkpoint_period = std::stoul(arg_checkpoint_period[0]);
  }

  ~HookInstructionCount() override
  {
    cout << "Total cycle count: " << Pipeline.getTotalCycles() << endl; 
  }

  void registerCache(Cache *ext_obj)
  {
    obj = ext_obj;
    obj->Pipeline = &Pipeline;

    // Set constant stats
    obj->stats.misc.on_duration = on_duration;
    obj->stats.misc.checkpoint_period = checkpoint_period;
  }

  void resetProcessor() {
    cout << "Resetting processor" << endl;
    obj->restoreCheckpoint();
  }

  void run(hook_arg_t *arg) override
  {
    auto PC = getEmulator().getArchitecture().registerGet(icemu::Architecture::Register::REG_PC);
    cout << "Instruction hook PC: " << PC << endl;
    // Reset the processor after a defined number of cycles (re-execution benchmark)
    // If the reset_cycle_target = 0, then no resets happen
    if (reset_cycle_target > 0 
        && Pipeline.getTotalCycles() >= reset_cycle_target) {

      // Check if there were checkpoints since the last reset (to detect the lack of forward progress)
      if (last_reset_checkpoint_count == obj->registerCheckpoint.count) {
        cout << "NO FORWARD PROGRESS" << endl;
        assert(false && "No forward progress is made, abort execution");
      }
      last_reset_checkpoint_count = obj->registerCheckpoint.count;

      // Set the next reset target
      reset_cycle_target += on_duration;

      // Reset the processor
      resetProcessor();

      // Skip the other steps
      return;
    }

    // Check if we need to create a periodic checkpoint
    // if the checkpoint_period = 0, then there are no periodic checkpoints
    if (checkpoint_period > 0
        && obj->stats.getCurrentCycle() >= (obj->stats.getLastCheckpointCycle() + checkpoint_period)) {
      //cout << "Periodic checkpoint" << endl;
      // Create a periodic checkpoint
      obj->createCheckpoint(CHECKPOINT_DUE_TO_PERIOD);
    }

    Pipeline.add(arg->address, arg->size);
    obj->updateCycleCount(Pipeline.getTotalCycles());

    // Track the stack pointer
    obj->stackTracker.run();

    // Detect cache hints
    address_t hint_memory = 0;
    bool is_hint = obj->cacheHints.run(arg->address, &hint_memory);
    if (is_hint) {
      // Apply the hint
      obj->applyCompilerHints(hint_memory);
    }
  }

};

// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
  public:
    HookInstructionCount *hook_instr_cnt;

    // Create cache object
    Cache CacheObj;

    string filename = "log/default_log";

  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru"), CacheObj(emu) {
    hook_instr_cnt = new HookInstructionCount(emu);

    // Register the cache object - could be done with OOPs but I like C style :(
    hook_instr_cnt->registerCache(&CacheObj);

    // Parse optional args
    parseLogArguements();
    parseCacheArguements();
  }

  ~MemoryAccess() {
    cout << "End of the cache" << endl;
  }

  void run(hook_arg_t *arg) { 
    auto PC = getEmulator().getArchitecture().registerGet(icemu::Architecture::Register::REG_PC);
    cout << "Memory hook PC: " << PC << endl;
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

    // if (address == 0x80070634) {
    //   auto PC = getEmulator().getArchitecture().registerGet(icemu::Architecture::REG_PC);
    //   cout  << "PC: " << hex << PC << dec << " Memory type: " << arg->mem_type << "\t size: " << arg->size << hex << "\t address: " << arg->address << "\tData: " << value << dec << endl;
    // }

    // Call the cache
    CacheObj.run(address, mem_type, &value, arg->size);
  }
    
  void parseLogArguements() {
      auto args = PluginArgumentParsing::GetArguments(getEmulator(), "custom-cache-log-file=");
      if (args.size())
        filename = args[0];
      
      cout << "Log file: " << filename << endl;
  }
  
  void parseCacheArguements() {
      // Default values
      uint32_t size = 512, lines = 2, hash_method = 0;
      bool enable_pw = false;
      int enable_stack_tracking = 0;
      string arg1 = "cache-size=", arg2 = "cache-lines=", arg3 = "hash-method=", arg4="enable-pw-bit=", arg5="enable-stack-tracking=";
      
      auto arg1_val = PluginArgumentParsing::GetArguments(getEmulator(), arg1);
      if (arg1_val.size())
        size = std::stoul(arg1_val[0]);

      auto arg2_val = PluginArgumentParsing::GetArguments(getEmulator(), arg2);
      if (arg2_val.size())
        lines = std::stoul(arg2_val[0]);

      auto arg3_val = PluginArgumentParsing::GetArguments(getEmulator(), arg3);
      if (arg3_val.size())
        hash_method = std::stoul(arg3_val[0]);

      auto arg4_val = PluginArgumentParsing::GetArguments(getEmulator(), arg4);
      if (arg4_val.size())
        enable_pw = !!(std::stoul(arg4_val[0]));

      auto arg5_val = PluginArgumentParsing::GetArguments(getEmulator(), arg5);
      if (arg5_val.size())
        enable_stack_tracking = std::stoul(arg5_val[0]);

      // Arguments used in HookInstructionCount 
      // TODO: Merge the use, because now we look for them here AND in the HookInstructionCount plugin
      // Here we only need them for the filename
      uint64_t on_duration = 0;
      auto arg_on_duration = PluginArgumentParsing::GetArguments(getEmulator(), "on-duration=");
      if (arg_on_duration.size())
        on_duration = std::stoul(arg_on_duration[0]);

      // Get the checkpoint cycle threshold
      uint64_t checkpoint_period = 0;
      auto arg_checkpoint_period = PluginArgumentParsing::GetArguments(getEmulator(), "checkpoint-period=");
      if (arg_checkpoint_period.size())
        checkpoint_period = std::stoul(arg_checkpoint_period[0]);

      filename += "-" + std::to_string(size) + "-" + std::to_string(lines) 
                + "-" + std::to_string(checkpoint_period) + "-" + std::to_string(on_duration);

      cout << "Lines from outside " << lines << endl;
      CacheObj.init(size, lines, LRU, getEmulator().getMemory(), filename, (enum CacheHashMethod)hash_method, enable_pw, enable_stack_tracking);
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
