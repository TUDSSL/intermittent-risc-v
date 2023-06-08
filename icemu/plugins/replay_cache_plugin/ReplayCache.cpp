#include <functional>
#include <iostream>
#include <memory>

#include "capstone/capstone.h"

#include "icemu/emu/types.h"
#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookCode.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"

#include "Riscv32E21Pipeline.hpp"

#include "../includes/Checkpoint.hpp"
#include "AsyncWriteBackCache.hpp"
#include "PowerFailureGenerator.hpp"
#include "Stats.hpp"

using _Pipeline = RiscvE21Pipeline;
using _PFG = PowerFailureGenerator<_Pipeline>;
using _Arch = icemu::ArchitectureRiscv32;
using arch_addr_t = _Arch::riscv_addr_t;
using _Cache = AsyncWriteBackCache<_Arch>;
using insn_t = std::unique_ptr<cs_insn, std::function<void(cs_insn *)>>;

#include "util.ipp"

static int NoMemCost(cs_insn *insn) {
  (void)insn;
  return 0;
}

class ReplayCacheIntrinsics : public HookCode {

 private:
  static std::string printLeader() {
    return "[replay_cache]";
  }

  bool is_region_active = false;
  unsigned int region_instruction_count = 0;
  unsigned int store_count_after_fence = 0;
  std::vector<insn_t> replay_instructions;
  arch_addr_t last_region_register_value = 0;
  arch_addr_t last_store_address = 0;
  address_t last_store_size = 0;
  bool last_cycle_was_pf = false;

  std::shared_ptr<_Cache> cache;

  _Pipeline pipeline;
  Stats stats;
  Checkpoint checkpoint;
  _PFG power_failure_generator;

  std::ofstream logger_cont, logger_final;

 public:
  explicit ReplayCacheIntrinsics(Emulator &emu, const std::shared_ptr<_Cache> &cache)
      : HookCode(emu, "replay_cache"),
        cache(cache),
        pipeline(emu, &NoMemCost, &NoMemCost),
        checkpoint(emu),
        power_failure_generator(emu, stats) {
    pipeline.setVerifyJumpDestinationGuess(false);
    pipeline.setVerifyNextInstructionGuess(false);
    cache->setPipeline(&pipeline);
    cache->setStats(&stats);

    std::string filename_base = "replay_cache_log";
    const auto args = PluginArgumentParsing::GetArguments(emu, "log-file=");
    if (args.size())
      filename_base = args[0];
    filename_base += "-" + std::to_string(cache->getCapacity());
    filename_base += "-" + std::to_string(cache->getNoOfLines());
    filename_base += "-" + std::to_string(power_failure_generator.getOnDuration());
    filename_base += "-0"; // checkpoint period unused, for compatibility
    std::cout << printLeader() << " Log file: " << filename_base << std::endl;

    logger_cont.open(filename_base + "-cont", std::ios::out | std::ios::trunc);
    logger_cont << "checkpoints,cycle count,last checkpoint,dirty ratio,cause\n";
    logger_final.open(filename_base + "-final", std::ios::out | std::ios::trunc);
  }

  ~ReplayCacheIntrinsics() {
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

      ASSERT(!last_cycle_was_pf && "Immediate power failure after reset, restoration might take too long");
      last_cycle_was_pf = true;

      createCheckpoint();
      resetProcessorAndCache();
      restoreCheckpoint();
      // Don't clear the region instruction count and replay instruction list; they are still valid

      return;
    }

    last_cycle_was_pf = false;
    if (is_region_active) ++region_instruction_count;

    const auto cyclesBefore = pipeline.getTotalCycles();
    pipeline.add(arg->address, arg->size);
    const auto pipelineCycleEst = pipeline.getTotalCycles() - cyclesBefore;

    // Only tick the cache if a non-cache related instruction was executed
    if (!processInstructions(arg->address, arg->size))
      cache->tick(pipelineCycleEst);
  }

 private:
  bool processInstructions(address_t address, address_t size) {
    // Process the instruction
    uint8_t instruction[size];

    bool ok = getEmulator().readMemory(address, (char *)instruction, size);
    if (!ok) {
      std::cerr << printLeader() << " failed to read memory for instruction at address " << address << std::endl;
      assert(false);
    }

    insn_t insn = {cs_malloc(*getEmulator().getCapstoneEngine()),
                   [](cs_insn *insn) { cs_free(insn, 1); }};
    const uint8_t *insn_ptr = instruction;
    address_t size_remaining = size;
    address_t address_next = address;
    if (!cs_disasm_iter(*getEmulator().getCapstoneEngine(), &insn_ptr, &size_remaining, &address_next, insn.get())) {
      std::cerr << printLeader() << " failed to disassemble instruction at address " << address << std::endl;
      assert(false);
    }

    return checkInstruction(std::move(insn));
  }

  bool checkInstruction(insn_t insn) {
    const auto rvinsn = (riscv_insn)insn->id;
    switch (rvinsn) {
      case RISCV_INS_AUIPC: {
        // Extract the operands
        const auto rd = insn->detail->riscv.operands[0];
        const auto imm20 = insn->detail->riscv.operands[1];
        assert(rd.type == RISCV_OP_REG);
        assert(imm20.type == RISCV_OP_IMM);

        // Process in case rd is x31 (t6)
        if (rd.reg == RISCV_REG_X31) {
          // Read the immediate value
          const auto imm1_value = imm20.imm;
          p_debug << printLeader() << " AUIPC x31, " << imm1_value << ": ";

          bool was_cache_instr = false;
          switch (imm1_value) {
            case 0: p_debug << "start region" << std::endl;
              // Store the PC for verification
              last_region_register_value = getRegisterValue(_Arch::Register::REG_PC);
              p_debug << printLeader() << " stored PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << last_region_register_value << std::dec << std::endl;
              // End the region if it is active, and start a new one
              if (is_region_active)
                endRegion();
              startRegion();
              break;
            case 1: p_debug << "CLWB" << std::endl;
              executeCLWB();
              was_cache_instr = true;
              break;
            case 2: p_debug << "FENCE" << std::endl;
              executeFence();
              was_cache_instr = true;
              break;
            case 3: p_debug << "power failure next" << std::endl;
              power_failure_generator.failNext();
              break;
            default:
              assert(false && "AUIPC immediate for region register not recognized");
              break;
          }
          // Adjust for any effects that this special AUIPC instruction might have had
          setRegisterValue(_Arch::Register::REG_X31, last_region_register_value);
          if (was_cache_instr)
            return true;
          break;
        }
        break;
      }
      case RISCV_INS_SW:
      case RISCV_INS_SH:
      case RISCV_INS_SB:
      case RISCV_INS_C_SW:
      case RISCV_INS_C_SWSP: {
        recordLastStore(insn);
        if (is_region_active) {
          // Increase the store counter.
          // Only when a region is active, to exclude stores that occurred before the first
          // region was activated
          ++store_count_after_fence;

          // Store the instruction for replay
          replay_instructions.emplace_back(std::move(insn));
          p_debug << printLeader() << " stored instruction for replay" << std::endl;
        }
        break;
      }
      default:
        break;
    }
    return false;
  }

  void recordLastStore(const insn_t &insn) {
    const auto store = StoreParsed::parse(insn);

    // Get only the base value, the source register is irrelevant
    const auto base_value = getRegisterValue(store.r_base);

    // Calculate the actual destination address, respecting the offset
    const auto addr = base_value + store.offset;

    // Persist the address and size for the CLWB instruction
    last_store_address = addr;
    last_store_size = store.size;
  }

  void endRegion() {
    assert(is_region_active);

    stats.incRegionEnds();
    stats.addRegionSize(region_instruction_count);
    stats.addStoresPerRegion(replay_instructions.size());

    is_region_active = false;
    region_instruction_count = 0;
    replay_instructions.clear();
  }

  void startRegion() {
    assert(!is_region_active);

    stats.incRegionStarts();

    is_region_active = true;
    assert(replay_instructions.empty());

    // Sanity check for compiler-generated code.
    // Any ReplayCache region can only safely be started when no stores have been
    // performed since the last FENCE instruction
    assert(store_count_after_fence == 0);
  }

  void executeCLWB() {
    const auto clwb_cycles = cache->clwb(last_store_address, last_store_size);
    p_debug << printLeader() << " CLWB cycles: " << clwb_cycles << std::endl;
    pipeline.addToCycles(clwb_cycles);
  }

  void executeFence() {
    const auto fence_cycles = cache->fence();
    p_debug << printLeader() << " FENCE cycles: " << fence_cycles << std::endl;
    pipeline.addToCycles(fence_cycles);
    store_count_after_fence = 0;
  }

  void createCheckpoint() {
    stats.incCheckpoints(pipeline.getTotalCycles());
    logger_cont << stats.getContinuousLine() << "\n"; // "\n" instead of std::endl to avoid immediate flushing

    const auto n_registers_checkpointed = checkpoint.create();
    const auto n_bytes_transferred = n_registers_checkpointed * 4;

    stats.incNVMWrites(n_bytes_transferred);
    pipeline.addToCycles(6 * n_bytes_transferred); // NVM writes
    pipeline.addToCycles(2 * n_registers_checkpointed); // QuickRecall logic overhead (TODO: currently just a guess)
  }

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

  void restoreCheckpoint() {
    // Replay all stores that were issued in this region so far
    std::cout << printLeader() << " replaying " << replay_instructions.size() << " instructions" << std::endl;
    for (auto &insn : replay_instructions) {
      const auto store = StoreParsed::parse(insn);
      replay(store);
    }

    // After ReplayCache has replayed all instructions, populating the cache,
    //  all registers can be restored to the values before power failure.
    // This happens with QuickRecall.
    quickRecallRestore();

    stats.incRestores();
  }

  void replay(const StoreParsed &store) {
    p_debug << printLeader() << " replaying " << registerNameFriendly(store.r_src)
                             << " to " << registerNameFriendly(store.r_base) << " + " << store.offset
                             << " (size " << store.size << ")" << std::endl;

    stats.incReplays();

    // Get the register values from the QuickRecall storage area
    stats.incNVMReads(4); // CPU cycles are accounted for later
    const auto src_value = getCheckpointedRegisterValue(store.r_src);
    stats.incNVMReads(4); // CPU cycles are accounted for later
    const auto base_value = getCheckpointedRegisterValue(store.r_base);

    // Calculate the actual destination address, respecting the offset
    const auto addr = base_value + store.offset;

    p_debug << printLeader() << " replay 0x" << std::hex << std::setw(8) << std::setfill('0') << src_value
            << " to 0x" << std::hex << std::setw(8) << std::setfill('0') << addr << std::dec << std::endl;

    // Tick the cache with the amount of cycles spent on reading the to-be-replayed value
    const auto cost_read_replay_value =
        6 * store.size // NVM read of the source register
      + 6 * 4;         // NVM read of the base register
    pipeline.addToCycles(cost_read_replay_value);
    cache->tick(cost_read_replay_value);

    // Inform the cache about the replay-store instruction
    cache->handleReplay(addr, src_value, store.size);

    // Call CLWB on the replayed store
    // NOTE: This is *not* mentioned in the paper, but the authors have confirmed
    //       that it was merely omitted from Figure 6 for brevity.
    pipeline.addToCycles(1 + cache->clwb(addr, store.size));

    // Account for post-replay instructions
    const auto cost_post_replay =
        1  // decrementing the replay counter
      + 1; // checking if the replay counter is zero
    pipeline.addToCycles(cost_post_replay);
    cache->tick(cost_post_replay);
  }

  void quickRecallRestore() {
    const auto n_registers_restored = checkpoint.restore();
    const auto n_bytes_transferred = n_registers_restored * 4;

    stats.incNVMReads(n_bytes_transferred);
    pipeline.addToCycles(6 * n_bytes_transferred); // NVM reads, assuming reads did not go through the cache
    pipeline.addToCycles(2 * n_registers_restored); // QuickRecall logic overhead (TODO: currently just a guess)
  }

  arch_addr_t getCheckpointedRegisterValue(_Arch::Register reg) {
    return checkpoint.getRegister(reg);
  }

  arch_addr_t getRegisterValue(_Arch::Register reg) {
    return getEmulator().getArchitecture().getRiscv32Architecture().registerGet(reg);
  }

  void setRegisterValue(_Arch::Register reg, arch_addr_t value) {
    getEmulator().getArchitecture().getRiscv32Architecture().registerSet(reg, value);
  }
};

class ReplayCacheMemoryAccess : public HookMemory {
 private:

  std::shared_ptr<_Cache> cache;

 public:
  ReplayCacheMemoryAccess(Emulator &emu, const std::shared_ptr<_Cache> &cache)
      : HookMemory(emu, "cache-lru-async"),
        cache(cache) {
    //
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
  HM.add(new ReplayCacheIntrinsics(emu, cache));
  HM.add(new ReplayCacheMemoryAccess(emu, cache));
}

// Class that is used by ICEmu to find the register function
// NB.  * MUST BE NAMED "RegisterMyHook"
//      * MUST BE global
RegisterHook RegisterMyHook(registerMyCodeHook);
