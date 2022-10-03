/**
 *  ICEmu loadable plugin (library)
 *
 * An example ICEmu plugin that is dynamically loaded.
 * This example prints the disassembled instructions
 *
 * Should be compiled as a shared library, i.e. using `-shared -fPIC`
 */
#include <iostream>

#include "capstone/capstone.h"

#include "icemu/emu/types.h"
#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookCode.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"

#include "Riscv32E21Pipeline.hpp"
#include "PluginArgumentParsing.h"

using namespace std;
using namespace icemu;

class MemClockCycles : public HookCode {
private:
  RiscvE21Pipeline Pipeline;

public:
  MemClockCycles(Emulator &emu)
      : HookCode(emu, "memory_stats_cycles"), Pipeline(emu) {
    Pipeline.setVerifyJumpDestinationGuess(false);
    Pipeline.setVerifyNextInstructionGuess(false);
  }

  ~MemClockCycles() {}

  void run(hook_arg *arg) {
    Pipeline.add(arg->address, arg->size);
  }

  uint64_t getCycleCount() {
    return Pipeline.getTotalCycles();
  }
};

class MemoryStats : public HookMemory {
 private:
  std::string printLeader() {
    return "[memory_stats]";
  }

  // Config
  std::string log_file = "memory_stats_log_file";

  // Data
  uint64_t bytes_written = 0;
  uint64_t bytes_read = 0;

  MemClockCycles &CC;

 public:
  // Always execute
  MemoryStats(Emulator &emu, MemClockCycles &CC) : HookMemory(emu, "memory_stats"), CC(CC) {

    // Get the log_file argument
    auto arg_log_file = PluginArgumentParsing::GetArguments(getEmulator(), "memory-stats-log-file=");
    if (arg_log_file.size())
      log_file = arg_log_file[0];

    log_file += "-final"; // Finish the log file
    cout << printLeader() << " using log file: " << log_file << endl;
  }

  ~MemoryStats() {
    // Write the log file
    ofstream log(log_file);
    if (!log.is_open()) {
      cout << printLeader() << " failed to open log file: " << log_file << endl;
      return;
    }

    // Print the stats

    cout << printLeader() << " memory statistics:" << endl;
    cout << "mem_reads:" << bytes_read << endl;
    cout << "mem_writes:" << bytes_written << endl;
    cout << "cycles:" << CC.getCycleCount() << endl;

    // Log the stats
    log << "mem_reads:" << bytes_read << endl;
    log << "mem_writes:" << bytes_written << endl;
    log << "cycles:" << CC.getCycleCount() << endl;
  }

  // Hook run
  void run(hook_arg_t *arg) {
    switch(arg->mem_type) {
      case MEM_READ:
        bytes_read += arg->size;
        break;
      case MEM_WRITE:
        bytes_written += arg->size;
        break;
    }
  }
};

// Function that registers the hook
static void registerMyCodeHook(Emulator &emu, HookManager &HM) {
  auto *CC = new MemClockCycles(emu);
  assert(CC != nullptr);
  auto *MS = new MemoryStats(emu, *CC);
  assert(MS != nullptr);

  HM.add(CC); // Add cycles first as we use it in the MS distructor
  HM.add(MS);
}

// Class that is used by ICEmu to finf the register function
// NB.  * MUST BE NAMED "RegisterMyHook"
//      * MUST BE global
RegisterHook RegisterMyHook(registerMyCodeHook);
