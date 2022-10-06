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

#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>

#include "icemu/emu/Architecture.h"
#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"

#include "../includes/Checkpoint.hpp"
#include "../includes/CycleCostCalculator.hpp"
#include "../includes/DetectWAR.h"
#include "../includes/LocalMemory.hpp"
#include "../includes/Logger.hpp"
#include "../includes/Stats.hpp"
#include "../includes/Utils.hpp"
#include "PluginArgumentParsing.h"
#include "Riscv32E21Pipeline.hpp"

using namespace std;
using namespace icemu;

class Clank {
 private:
  static int NoMemCost(cs_insn *insn) {
    (void)insn;
    return 0;
  }

 public:
  RiscvE21Pipeline Pipeline;
  Checkpoint registerCheckpoint;

  DetectWAR war;
  Logger log;
  Stats stats;
  LocalMemory nvm;
  CycleCost cost;
  address_t last_chp = 0;

  // Settings
  bool double_bufferd_checkpoints = true;

  // We control the memory access cost our selves using the CycleCostCalculator
  // in order to keep it the same for all systems
  Clank(Emulator &emu) : Pipeline(emu, &NoMemCost, &NoMemCost), registerCheckpoint(emu) {
    Pipeline.setVerifyJumpDestinationGuess(false);
    Pipeline.setVerifyNextInstructionGuess(false);
  }

  void runInstruction(address_t address, size_t size) {
    Pipeline.add(address, size);
    stats.updateCurrentCycle(Pipeline.getTotalCycles());
  }

  void runMemory(address_t address, enum HookMemory::memory_type type,
                 address_t value, size_t size) {
    switch (type) {
      case HookMemory::MEM_READ:
        cost.modifyCost(&Pipeline, NVM_READ, size);
        stats.incNVMReads(size);
        break;

      case HookMemory::MEM_WRITE:
        cost.modifyCost(&Pipeline, NVM_WRITE, size);
        stats.incNVMWrites(size);

        nvm.localWrite(address, value, size);
        break;
    }

    if (war.isWAR(address, size, type)) {
      p_debug << "Creating checkpoint #" << stats.checkpoint.checkpoints << endl;
      // Create a checkpoint
      createCheckpoint(CHECKPOINT_DUE_TO_WAR);
    }
  }

  void createCheckpoint(enum CheckpointReason reason) {
    // cout << "Creating checkpoint" << endl;
    // Create a register checkpoint
    int reg_cp_size = registerCheckpoint.create() * 4; // Checkpoint size in bytes

    // Write the checkpoint to NVM
    stats.incNVMWrites(reg_cp_size);
    cost.modifyCost(&Pipeline, NVM_WRITE, reg_cp_size);

    // Double buffer
    if (double_bufferd_checkpoints) {
      // Read the checkpoint back
      stats.incNVMReads(reg_cp_size);
      cost.modifyCost(&Pipeline, NVM_READ, reg_cp_size);

      // Double buffered final write
      stats.incNVMWrites(reg_cp_size);
      cost.modifyCost(&Pipeline, NVM_WRITE, reg_cp_size);
    }

    // Only place where checkpoints are incremented
    stats.incCheckpoints();

    // Update the cause to the stats
    stats.updateCheckpointCause(reason);

    // Increment based on reasons
    switch (reason) {
      case CHECKPOINT_DUE_TO_WAR:
        stats.incCheckpointsDueToWAR();
        // p_debug << " due to WAR" << endl;
        break;
      case CHECKPOINT_DUE_TO_DIRTY:
        assert(false && "Clank should not have a 'dirty' checkpoint cause");
        break;
      case CHECKPOINT_DUE_TO_PERIOD:
        stats.incCheckpointsDueToPeriod();
        // p_debug << " due to period" << endl;
        break;
    }

    last_chp = Pipeline.getTotalCycles();

    log.printCheckpointStats(stats);
    stats.updateLastCheckpointCycle(last_chp);

    war.reset();
  }

  void restoreCheckpoint() {
    // Restore the registers
    int reg_cp_size = registerCheckpoint.restore() * 4; // Checkpoint size in bytes

    // Increment NVM writes
    stats.incNVMReads(reg_cp_size);
    cost.modifyCost(&Pipeline, NVM_READ, reg_cp_size);

    // Increment counter
    stats.incRestores();
  }

  void resetProcessor() {
    p_debug << "Resetting processor" << endl;
    restoreCheckpoint();
  }
};

class HookInstructionCount : public HookCode {
 public:
  Clank clank;

  // Power failure emulation
  uint64_t on_duration = 0;
  uint64_t reset_cycle_target = 0;
  uint64_t last_reset_checkpoint_count = 0;

  // Periodic checkpoints
  uint64_t checkpoint_period = 0;

  HookInstructionCount(Emulator &emu) : HookCode(emu, "icnt-ratio"), clank(emu) {
    // Get the on duration from the arguments
    auto arg_on_duration = PluginArgumentParsing::GetArguments(getEmulator(), "on-duration=");
    if (arg_on_duration.size())
      on_duration = std::stoul(arg_on_duration[0]);

    // Configure the next reset_cycle_target
    reset_cycle_target += on_duration;

    // Get the checkpoint cycle threshold
    auto arg_checkpoint_period = PluginArgumentParsing::GetArguments(
        getEmulator(), "checkpoint-period=");
    if (arg_checkpoint_period.size())
      checkpoint_period = std::stoul(arg_checkpoint_period[0]);

    p_debug << "On-duration: " << on_duration << endl;
    p_debug << "checkpoint period: " << checkpoint_period << endl;
    p_debug << "reset cycle target: " << reset_cycle_target << endl;

    // Set constant stats
    clank.stats.misc.on_duration = on_duration;
    clank.stats.misc.checkpoint_period = checkpoint_period;
  }

  ~HookInstructionCount() {}

  void run(hook_arg_t *arg) {
    // Reset the processor after a defined number of cycles (re-execution
    // benchmark) If the reset_cycle_target = 0, then no resets happen
    if (reset_cycle_target > 0 &&
        clank.Pipeline.getTotalCycles() >= reset_cycle_target) {

      // Check if there were checkpoints since the last reset (to detect the
      // lack of forward progress)
      if (last_reset_checkpoint_count == clank.registerCheckpoint.count) {
        cout << "NO FORWARD PROGRESS" << endl;
        assert(false && "No forward progress is made, abort execution");
      }
      last_reset_checkpoint_count = clank.registerCheckpoint.count;

      // Set the next reset target
      reset_cycle_target += on_duration;

      // Reset the processor
      clank.resetProcessor();

      // Skip the other steps
      return;
    }

    // Check if we need to create a periodic checkpoint
    // if the checkpoint_period = 0, then there are no periodic checkpoints
    if (checkpoint_period > 0 &&
        clank.Pipeline.getTotalCycles() >= (clank.stats.getLastCheckpointCycle() + checkpoint_period)) {
      // cout << "Periodic checkpoint" << endl;
      // Create a periodic checkpoint
      clank.createCheckpoint(CHECKPOINT_DUE_TO_PERIOD);
    }

    clank.runInstruction(arg->address, arg->size);
  }
};

class MemoryAccess : public HookMemory {
 public:
  HookInstructionCount *hook_instr_cnt;
  Clank *clank;

  // Register checkpoint
  Checkpoint registerCheckpoint;

  MemoryAccess(Emulator &emu) : HookMemory(emu, "memory-access-ratio"), registerCheckpoint(emu) {
    hook_instr_cnt = new HookInstructionCount(emu);
    clank = &hook_instr_cnt->clank;

    string filename;

    auto arg1_val = PluginArgumentParsing::GetArguments(getEmulator(), "clank-log-file=");
    if (arg1_val.size())
      filename = arg1_val[0];

    filename += "-" + std::to_string(hook_instr_cnt->checkpoint_period) + "-" +
                std::to_string(hook_instr_cnt->on_duration);
    clank->log.init(filename);
    clank->nvm.initMem(&getEmulator().getMemory());
  }

  ~MemoryAccess() {
    clank->stats.printStats();
    clank->log.printEndStats(clank->stats);
  }

  void run(hook_arg_t *arg) {
    clank->runMemory(arg->address, arg->mem_type, arg->value, arg->size);
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
