#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>

#include "capstone/capstone.h"

#include "icemu/emu/types.h"
#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookCode.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"

#include "Riscv32E21Pipeline.hpp"

#include "../includes/Checkpoint.hpp"
#include "../includes/CycleCostCalculator.hpp"
#include "ReplayCacheBaselineCache.hpp"
#include "PowerFailureGenerator.hpp"
#include "Stats.hpp"

using _Pipeline = RiscvE21Pipeline;
using _PFG = PowerFailureGenerator<_Pipeline>;
using _Arch = icemu::ArchitectureRiscv32;
using arch_addr_t = _Arch::riscv_addr_t;
using _Cache = ReplayCacheBaselineCache<_Arch>;
using insn_t = std::unique_ptr<cs_insn, std::function<void(cs_insn *)>>;
using _Stats = ReplayCacheStats::Stats;

#include "util.ipp"

static int NoMemCost(cs_insn *insn) {
  (void)insn;
  return 0;
}

class ReplayCacheBaselineIntrinsics : public HookCode {

 private:
  static std::string printLeader() {
    return "[replay_cache_baseline]";
  }

  bool is_region_active = false;
  unsigned int region_instruction_count = 0;
  unsigned int store_count_after_fence = 0;
  std::vector<insn_t> replay_instructions;
  std::set<_Arch::Register> region_store_operand_registers;
  arch_addr_t last_region_register_value = 0;
  arch_addr_t last_store_address = 0;
  address_t last_store_size = 0;
  bool last_cycle_was_pf = false;

  std::shared_ptr<_Cache> cache;

  struct Restore {
    arch_addr_t region_addr;
    arch_addr_t pc_addr;
  };
  std::vector<Restore> restores;
  std::unordered_map<arch_addr_t, std::set<arch_addr_t>> stores_map;

  CycleCost cost;

  _Pipeline pipeline;
  _Stats stats;
  Checkpoint checkpoint;
  _PFG power_failure_generator;

  std::ofstream logger_cont, logger_final;

  bool double_bufferd_checkpoints = true;

 public:
  explicit ReplayCacheBaselineIntrinsics(Emulator &emu, const std::shared_ptr<_Cache> &cache)
      : HookCode(emu, "replay_cache_baseline"),
        cache(cache),
        pipeline(emu, &NoMemCost, &NoMemCost),
        stats(pipeline),
        checkpoint(emu),
        power_failure_generator(emu, stats) {
    pipeline.setVerifyJumpDestinationGuess(false);
    pipeline.setVerifyNextInstructionGuess(false);
    cache->setPipeline(&pipeline);
    cache->setStats(&stats);

    std::string filename_base = "replay_cache_log";
    std::string opt_level = "-O3";
    const auto args = PluginArgumentParsing::GetArguments(emu, "log-file=");
    const auto args2 = PluginArgumentParsing::GetArguments(emu, "opt-level=");
    if (args.size())
      filename_base = args[0];
    if (args2.size())
      opt_level = args2[0];
    filename_base += "-" + std::to_string(cache->getCapacity());
    filename_base += "-" + std::to_string(cache->getNoOfLines());
    filename_base += "-0"; // checkpoint period unused, for compatibility
    filename_base += "-" + std::to_string(power_failure_generator.getOnDuration());
    filename_base += opt_level;
    std::cout << printLeader() << " Log file: " << filename_base << std::endl;

    logger_cont.open(filename_base + "-cont", std::ios::out | std::ios::trunc);
    logger_cont << "checkpoints,cycle count,last checkpoint,dirty ratio,cause\n";
    logger_final.open(filename_base + "-final", std::ios::out | std::ios::trunc);
  }

  ~ReplayCacheBaselineIntrinsics() {    
    stats.updateTotalCycles(pipeline.getTotalCycles());

    // Print stats & config summary to stdout
    std::cout << "\n-------------------------------------" << std::endl;
    cache->printConfig(std::cout);
    std::cout << "-------------------------------------" << std::endl;
    stats.printAll(std::cout);
    std::cout << "-------------------------------------" << std::endl;
    std::cout << printLeader() << " total cycle count: " << pipeline.getTotalCycles() << std::endl;

    // Formally store stats in the log file
    stats.logAll(logger_final);
  }

  void run(hook_arg *arg) {
    if (power_failure_generator.shouldReset(pipeline, checkpoint)) {
      std::cout << printLeader() << " power failure at cycle " << pipeline.getTotalCycles() << std::endl;

      createCheckpoint();
      resetProcessorAndCache();
      restoreCheckpoint();
    }
    
    const auto cyclesBefore = pipeline.getTotalCycles();
    pipeline.add(arg->address, arg->size);
    const auto pipelineCycleEst = pipeline.getTotalCycles() - cyclesBefore;
    stats.incInstrCycles(pipelineCycleEst);

    return;
  }

private:
void resetProcessorAndCache() {
    // Zero out all regular registers
    setRegisterValue(_Arch::REG_X0, 0);
    setRegisterValue(_Arch::REG_X1, 0);
    setRegisterValue(_Arch::REG_X2, 0);
    setRegisterValue(_Arch::REG_X3, 0);
    setRegisterValue(_Arch::REG_X4, 0);
    setRegisterValue(_Arch::REG_X5, 0);
    setRegisterValue(_Arch::REG_X6, 0);
    setRegisterValue(_Arch::REG_X7, 0);
    setRegisterValue(_Arch::REG_X8, 0);
    setRegisterValue(_Arch::REG_X9, 0);
    setRegisterValue(_Arch::REG_X10, 0);
    setRegisterValue(_Arch::REG_X11, 0);
    setRegisterValue(_Arch::REG_X12, 0);
    setRegisterValue(_Arch::REG_X13, 0);
    setRegisterValue(_Arch::REG_X14, 0);
    setRegisterValue(_Arch::REG_X15, 0);
    setRegisterValue(_Arch::REG_X16, 0);
    setRegisterValue(_Arch::REG_X17, 0);
    setRegisterValue(_Arch::REG_X18, 0);
    setRegisterValue(_Arch::REG_X19, 0);
    setRegisterValue(_Arch::REG_X20, 0);
    setRegisterValue(_Arch::REG_X21, 0);
    setRegisterValue(_Arch::REG_X22, 0);
    setRegisterValue(_Arch::REG_X23, 0);
    setRegisterValue(_Arch::REG_X24, 0);
    setRegisterValue(_Arch::REG_X25, 0);
    setRegisterValue(_Arch::REG_X26, 0);
    setRegisterValue(_Arch::REG_X27, 0);
    setRegisterValue(_Arch::REG_X28, 0);
    setRegisterValue(_Arch::REG_X29, 0);
    setRegisterValue(_Arch::REG_X30, 0);
    setRegisterValue(_Arch::REG_X31, 0);

    // Incur power failure on the cache
    cache->powerFailure();
  }


  arch_addr_t getRegisterValue(_Arch::Register reg) {
    return getEmulator().getArchitecture().getRiscv32Architecture().registerGet(reg);
  }

  void setRegisterValue(_Arch::Register reg, arch_addr_t value) {
    getEmulator().getArchitecture().getRiscv32Architecture().registerSet(reg, value);
  }

  void createCheckpoint() {
    // cout << "Creating checkpoint" << endl;
    // Create a register checkpoint
    int reg_cp_size = checkpoint.create() * 4; // Checkpoint size in bytes

    // Write the checkpoint to NVM
    stats.incNVMWrites(reg_cp_size);
    cost.modifyCost(&pipeline, NVM_WRITE, reg_cp_size);

    // Double buffer
    if (double_bufferd_checkpoints) {
      // Read the checkpoint back
      stats.incNVMReads(reg_cp_size);
      cost.modifyCost(&pipeline, NVM_READ, reg_cp_size);

      // Double buffered final write
      stats.incNVMWrites(reg_cp_size);
      cost.modifyCost(&pipeline, NVM_WRITE, reg_cp_size);
    }

    // Only place where checkpoints are incremented
    stats.incCheckpoints(pipeline.getTotalCycles());
  }

  void restoreCheckpoint() {
    // Restore the registers
    int reg_cp_size = checkpoint.restore() * 4; // Checkpoint size in bytes

    // Increment NVM writes
    stats.incNVMReads(reg_cp_size);
    cost.modifyCost(&pipeline, NVM_READ, reg_cp_size);

    // Increment counter
    stats.incRestores();
  }
};

class ReplayCacheBaselineMemoryAccess : public HookMemory {
 private:

  std::shared_ptr<_Cache> cache;

 public:
  ReplayCacheBaselineMemoryAccess(Emulator &emu, const std::shared_ptr<_Cache> &cache)
      : HookMemory(emu, "cache-none"),
        cache(cache) {
  }

  void run(hook_arg_t *arg) {
    const auto address = arg->address;
    const auto mem_type = arg->mem_type;
    auto value = arg->value;

    if (arg->mem_type == MEM_READ) {
      const auto *read_value = getEmulator().getMemory().at(arg->address);
      memcpy(&arg->value, read_value, arg->size);
    }

    const auto main_memory_start = getEmulator().getMemory().entrypoint;

    // Process only valid memory
    if (!(address >= main_memory_start))
      return;

    cache->handleRequest(address, mem_type, &value, arg->size);
  }
};

// Function that registers the hook
static void registerMyCodeHook(Emulator &emu, HookManager &HM) {
  // Create the cache, which is shared between the two hooks
  auto cache = std::make_shared<_Cache>(_Cache::fromImplicitConfig(emu));

  // Register the hooks with ICEmu
  HM.add(new ReplayCacheBaselineIntrinsics(emu, cache));
  HM.add(new ReplayCacheBaselineMemoryAccess(emu, cache));
}

// Class that is used by ICEmu to find the register function
// NB.  * MUST BE NAMED "RegisterMyHook"
//      * MUST BE global
RegisterHook RegisterMyHook(registerMyCodeHook);
