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
#include "AsyncWriteBackCache.hpp"
#include "PowerFailureGenerator.hpp"
#include "Stats.hpp"

using _Pipeline = RiscvE21Pipeline;
using _PFG = PowerFailureGenerator<_Pipeline>;
using _Arch = icemu::ArchitectureRiscv32;
using arch_addr_t = _Arch::riscv_addr_t;
using _Cache = AsyncWriteBackCache<_Arch>;
using insn_t = std::unique_ptr<cs_insn, std::function<void(cs_insn *)>>;
using _Stats = ReplayCacheStats::Stats;

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
  int instructions_from_last_store_to_boundary = 0;
  std::set<_Arch::Register> region_store_operand_registers;
  arch_addr_t last_region_register_value = 0;
  arch_addr_t last_store_address = 0;
  address_t last_store_size = 0;
  bool last_cycle_was_pf = false;
  bool was_cache_instr = false;

  std::shared_ptr<_Cache> cache;

  struct Restore {
    arch_addr_t region_addr;
    arch_addr_t pc_addr;
  };
  std::vector<Restore> restores;
  std::unordered_map<arch_addr_t, std::set<arch_addr_t>> stores_map;

  _Pipeline pipeline;
  _Stats stats;
  Checkpoint checkpoint;
  _PFG power_failure_generator;

  std::ofstream logger_cont, logger_final;

 public:
  explicit ReplayCacheIntrinsics(Emulator &emu, const std::shared_ptr<_Cache> &cache)
      : HookCode(emu, "replay_cache"),
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

  ~ReplayCacheIntrinsics() {
    /* Execute fence to wait for any pending cache requests. */
    executeFence();
    endRegion();
    addPendingRestoreCycles();
    
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

      ASSERT(!last_cycle_was_pf && "Immediate power failure after reset, restoration might take too long");
      last_cycle_was_pf = true;

      /* Power failure triggered, create checkpoint and restore using QuickRecall. */
      cache->setIsInCheckpoint(true);
      createCheckpoint();
      resetProcessorAndCache();
      restoreCheckpoint();
      cache->setIsInCheckpoint(false);
      // Don't clear the region instruction count and replay instruction list; they are still valid

      return;
    }

    if (is_region_active) ++region_instruction_count;

    const auto cyclesBefore = pipeline.getTotalCycles();
    pipeline.add(arg->address, arg->size);
    const auto pipelineCycleEst = pipeline.getTotalCycles() - cyclesBefore;

    // Only tick the cache if a non-cache related instruction was executed
    if (!processInstructions(arg->address, arg->size))
      cache->tick(pipelineCycleEst);

    /* ReplayCache authors said that a CLWB or region boundary instruction creates no runtime overhead,
     * so we subtract their pipeline cycles here.
     */
    if (was_cache_instr)
    {
      pipeline.addToCycles(-pipelineCycleEst);
    }
    else
    {
      stats.incInstrCycles(pipelineCycleEst);
    }

    was_cache_instr = false;
    last_cycle_was_pf = false;
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

  std::string insnDebugLeader(const insn_t &insn) {
    std::stringstream ss;
    const auto pc = getRegisterValue(_Arch::Register::REG_PC);
    ss << printLeader() << " ";
    ss << std::hex << std::setfill('0') << std::setw(8) << pc << "/";
    ss << std::hex << std::setfill('0') << std::setw(8) << insn->address << " ";
    ss << insn->mnemonic << " " << insn->op_str;
    return ss.str();
  }

  bool checkInstruction(insn_t insn) {
    // Verify that the registers that this instruction writes to are not used in any of the
    // instructions that were stored for replay
    // In RISCV, for all instructions other than stores & branches, if the first operand is a register,
    // it is the destination register
    if (insn->detail->riscv.op_count > 0 && insn->detail->riscv.operands[0].type == RISCV_OP_REG &&
        insn->id != RISCV_INS_SW &&
        insn->id != RISCV_INS_SH &&
        insn->id != RISCV_INS_SB &&
        insn->id != RISCV_INS_C_SW &&
        insn->id != RISCV_INS_C_SWSP && 
        insn->id != RISCV_INS_FSW &&
        insn->id != RISCV_INS_BEQ &&
        insn->id != RISCV_INS_BGE &&
        insn->id != RISCV_INS_BGEU &&
        insn->id != RISCV_INS_BLT &&
        insn->id != RISCV_INS_BLTU &&
        insn->id != RISCV_INS_BNE &&
        insn->id != RISCV_INS_C_BNEZ &&
        insn->id != RISCV_INS_C_BEQZ) {
      const auto rd = fromCapstone(insn->detail->riscv.operands[0].reg);
      if (region_store_operand_registers.find(rd) != region_store_operand_registers.end()) {
        std::cerr << insnDebugLeader(insn) << ": instruction writes to register '" << registerNameFriendly(rd)
                  << "' which is used as a store operand in the current region before this instruction" << std::endl;
        assert(false);
      }
    }

    bool was_store_instr = false;
    const auto rvinsn = (riscv_insn)insn->id;
    switch (rvinsn) {
      case RISCV_INS_C_LI: {
        // Extract the operands
        const auto rd = insn->detail->riscv.operands[0];
        const auto imm = insn->detail->riscv.operands[1];
        assert(rd.type == RISCV_OP_REG);
        assert(imm.type == RISCV_OP_IMM);

        // Process in case rd is x31 (t6), this is the reserved ReplayCache register
        if (rd.reg == RISCV_REG_X31) {
          // Read the immediate value
          const auto imm_value = imm.imm;
          p_debug << printLeader() << " C.LI x31, " << imm_value << ": ";

          switch (imm_value) {
            case 0:
              /* This case happens at the start, where all values are set to 0. */
              break;
            case 1: // START_REGION
              stats.incStartRegionCount();
              executeRegionBoundary();
              was_cache_instr = true;
              break;
            case 2:  // START_REGION_RETURN
              stats.incStartRegionReturnCount();
              executeRegionBoundary();
              was_cache_instr = true;
              break;
            case 3:  // START_REGION_EXTENSION
              stats.incStartRegionExtensionCount();
              executeRegionBoundary();
              was_cache_instr = true;
              break;
            case 4:  // START_REGION_BRANCH (not used)
              stats.incStartRegionBranchCount();
              executeRegionBoundary();
              was_cache_instr = true;
              break;
            case 5:  // START_REGION_BRANCH_DEST
              stats.incStartRegionBranchDestCount();
              executeRegionBoundary();
              was_cache_instr = true;
              break;
            case 6: // START_REGION_STACK_SPILL
              stats.incStartRegionStackSpillCount();
              executeRegionBoundary();
              was_cache_instr = true;
              break;
            case 8: // CLWB
              executeCLWB();
              was_cache_instr = true;
              break;
            case 9: p_debug << "power failure next" << std::endl;
              power_failure_generator.failNext();
              break;
            default:
              assert(false && "C.LI immediate for region register not recognized");
              break;
          }
          // Adjust for any effects that this special instruction might have had
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
        const auto store = StoreParsed::parse(insn);
        recordLastStore(store);
        if (is_region_active) {
          // Increase the store counter.
          // Only when a region is active, to exclude stores that occurred before the first
          // region was activated
          ++store_count_after_fence;

          // Mark the operands as not being allowed to be modified in any further instruction
          region_store_operand_registers.insert(store.r_src);
          region_store_operand_registers.insert(store.r_base);

          // Store the instruction for replay
          replay_instructions.emplace_back(std::move(insn));
          p_debug << printLeader() << " stored instruction for replay" << std::endl;
          instructions_from_last_store_to_boundary = 0;
        }
        was_store_instr = true;
        break;
      }
      default:
        break;
    }

    /* Keep track of the number of instructions from the last store
     * in the current region to the next region boundary.
     */
    if (!was_store_instr)
    {
      instructions_from_last_store_to_boundary++;
    }

    return false;
  }

  void executeRegionBoundary()
  {
      p_debug << "start region" << std::endl;
      executeFence();
      // Store the PC for verification
      last_region_register_value = getRegisterValue(_Arch::Register::REG_PC);
      p_debug << printLeader() << " stored PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << last_region_register_value << std::dec << std::endl;
      // End the region if it is active, and start a new one
      if (is_region_active)
        endRegion();
      startRegion();
  }

  void recordLastStore(const StoreParsed &store) {
    // Get only the base value, the source register is irrelevant
    const auto base_value = getRegisterValue(store.r_base);

    // Calculate the actual destination address, respecting the offset
    const auto addr = base_value + store.offset;

    // Persist the address and size for the CLWB instruction
    last_store_address = addr;
    last_store_size = store.size;

    // Record the store.
    stores_map[last_region_register_value].insert(getRegisterValue(_Arch::Register::REG_PC));
  }

  void endRegion() {
    assert(is_region_active);

    stats.incRegionEnds();
    stats.addRegionSize(region_instruction_count);
    stats.addStoresPerRegion(replay_instructions.size());
    stats.addDistanceBeforeBoundary(instructions_from_last_store_to_boundary);

    is_region_active = false;
    region_instruction_count = 0;
    instructions_from_last_store_to_boundary = 0;
    replay_instructions.clear();
    region_store_operand_registers.clear();
  }

  void startRegion() {
    assert(!is_region_active);

    stats.incRegionStarts();

    is_region_active = true;
    assert(replay_instructions.empty());

    // Sanity check for compiler-generated code.
    // Any ReplayCache region can only safely be started when no stores have been
    // performed since the last region boundary
    assert(store_count_after_fence == 0);
  }

  void executeCLWB() {
    const auto clwb_cycles = cache->clwb(last_store_address, last_store_size, last_cycle_was_pf);
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

    /* Create QuickRecall checkpoint by backing up all registers. */
    stats.incNVMWrites(n_bytes_transferred);
    auto cost = NVM_WRITE_COST * (n_bytes_transferred + 1); // NVM writes (one extra for QuickRecall checkpoint flag)
    auto instr_cost = n_registers_checkpointed + 1; // Optimistic cost of instructions in pipeline, store instructions + voltage comparison. Assume power off after 1 comparison.
    pipeline.addToCycles(cost + instr_cost);
    stats.incNVMWriteCycles(cost);
    stats.incInstrCycles(instr_cost);
    stats.incRestoreCycles(cost + instr_cost);

    /* Push restore to list. */
    restores.push_back({.region_addr = last_region_register_value, .pc_addr = getRegisterValue(_Arch::Register::REG_PC)});
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
    auto cost = NVM_READ_COST * 4 + 1  // Add cycles for reading region register. Also done by QuickRecall.
              + NVM_READ_COST * 4 + 1  // Add cycles for lookup of CM map entry & storing it in register.
              + NVM_READ_COST * 4 + 1; // Add cycles for lookup of recovery code address in RC map & storing in PC.
    pipeline.addToCycles(cost);
    stats.incRestoreCycles(cost);
    stats.incNVMReads(4 * 3);
    // Cost for looking up SC in binary search are added later.

    // Replay all stores that were issued in this region so far
    std::cout << printLeader() << " replaying " << replay_instructions.size() << " instructions" << std::endl;
    for (auto &insn : replay_instructions) {
      const auto store = StoreParsed::parse(insn);
      replay(store);
    }

    // After ReplayCache has replayed all instructions, populating the cache,
    // all registers can be restored to the values before power failure.
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
        NVM_READ_COST * store.size + 1; // NVM read of the source register
    pipeline.addToCycles(cost_read_replay_value);
    cache->tick(cost_read_replay_value);
    stats.incRestoreCycles(cost_read_replay_value);

    // Inform the cache about the replay-store instruction
    cache->handleReplay(addr, src_value, store.size);

    // Call CLWB on the replayed store
    // NOTE: This is *not* mentioned in the paper, but the authors have confirmed
    //       that it was merely omitted from Figure 6 for brevity.
    auto cache_clwb_cycles = 1 + cache->clwb(addr, store.size);
    pipeline.addToCycles(cache_clwb_cycles);
    stats.incRestoreCycles(cache_clwb_cycles);

    // Account for post-replay instructions
    const auto cost_post_replay =
        1  // decrementing the replay counter
      + 1; // checking if the replay counter is zero
    pipeline.addToCycles(cost_post_replay);
    cache->tick(cost_post_replay);
    stats.incRestoreCycles(cost_post_replay);
  }

  void quickRecallRestore() {
    const auto n_registers_restored = checkpoint.restore();
    const auto n_bytes_transferred = n_registers_restored * 4;

    // Initialization overhead is not taken into account because it is not quickrecall-dependent.
    /* Restore by transferring saved registers from NVM to register file. */
    stats.incNVMReads(n_bytes_transferred);
    stats.incNVMWrites(1);
    stats.incNVMWriteCycles(1 * NVM_READ_COST);
    const auto cost_quick_recall = 
      (NVM_READ_COST + 1) * (n_bytes_transferred) // NVM reads, assuming reads did not go through the cache
    + NVM_WRITE_COST + 1  // NVM write to reset QuickRecall checkpoint flag
    + 1; // Compare Vdd > Vtrig, assume true
    pipeline.addToCycles(cost_quick_recall);
    stats.incRestoreCycles(cost_quick_recall);
    cache->tick(cost_quick_recall);
  }

  void addPendingRestoreCycles()
  {
    /* Add pending restore cycles from looking up store counter in the SC table. */
    /* Assume average-case complexity with 9 cycles per iteration. 9 cycles/iteration is based on https://mhdm.dev/posts/sb_lower_bound/ */
    int cost_restore_pending = 0;
    for (auto &restore : restores)
    {
      auto &stores_set = stores_map[restore.region_addr];
      auto len = stores_set.size();
      cost_restore_pending += 1; // Store size
      if (len > 0)
      {
        cost_restore_pending += (int)std::round(9.0 * log10((double)len)); // Calculate worst case complexity, with 9 cycles per iteration.
      }
    }

    pipeline.addToCycles(cost_restore_pending);
    stats.incRestoreCycles(cost_restore_pending);
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
