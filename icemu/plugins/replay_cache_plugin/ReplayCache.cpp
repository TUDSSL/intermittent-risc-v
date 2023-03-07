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
  std::vector<insn_t> replay_instructions;
  arch_addr_t last_region_register_value = 0;

  std::shared_ptr<_Cache> cache;

  _Pipeline pipeline;
  Checkpoint checkpoint;
  _PFG power_failure_generator;

 public:
  explicit ReplayCacheIntrinsics(Emulator &emu, const std::shared_ptr<_Cache> &cache)
      : HookCode(emu, "replay_cache"),
        cache(cache),
        pipeline(emu, &NoMemCost, &NoMemCost),
        checkpoint(emu),
        power_failure_generator(emu) {
    pipeline.setVerifyJumpDestinationGuess(false);
    pipeline.setVerifyNextInstructionGuess(false);
  }

  ~ReplayCacheIntrinsics() {
    std::cout << printLeader() << " total cycle count: " << pipeline.getTotalCycles() << std::endl;
  }

  void run(hook_arg *arg) {
    if (power_failure_generator.shouldReset(pipeline, checkpoint)) {
      std::cout << printLeader() << " power failure at cycle " << pipeline.getTotalCycles() << std::endl;

      createCheckpoint();
      resetProcessorAndCache();
      restoreCheckpoint();
      // Don't clear the replay instructions; they are still valid

      return;
    }

    const auto cyclesBefore = pipeline.getTotalCycles();
    pipeline.add(arg->address, arg->size);
    const auto pipelineCycleEst = pipeline.getTotalCycles() - cyclesBefore;

    cache->tick(pipelineCycleEst);

    processInstructions(arg->address, arg->size);
  }

 private:
  void processInstructions(address_t address, address_t size) {
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

    checkInstruction(std::move(insn));
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
            case 1: p_debug << "end region" << std::endl;
              // End a region without starting a new one.
              // Issued before a branch, but after the fence.
              endRegion();
              break;
            case 2: p_debug << "CLWB" << std::endl;
              executeCLWB();
              break;
            case 3: p_debug << "FENCE" << std::endl;
              executeFence();
              break;
            case 4: p_debug << "power failure next" << std::endl;
              power_failure_generator.failNext();
              break;
            default:
              assert(false && "AUIPC immediate for region register not recognized");
              break;
          }
          // Adjust for any effects that this special AUIPC instruction might have had
          setRegisterValue(_Arch::Register::REG_X31, last_region_register_value);
          break;
        }
        break;
      }
      case RISCV_INS_SW:
      case RISCV_INS_SH:
      case RISCV_INS_SB:
      case RISCV_INS_C_SW:
      case RISCV_INS_C_SWSP: {
        // TODO: in any case, make sure writes are delayed and not written back to NVM immediately
        if (is_region_active) {
          // Store the instruction for replay
          replay_instructions.emplace_back(std::move(insn));
          p_debug << printLeader() << " stored instruction for replay" << std::endl;
          return true;
        }
        break;
      }
      default:
        break;
    }
    return false;
  }

  void endRegion() {
    assert(is_region_active);
    is_region_active = false;
    replay_instructions.clear();
  }

  void startRegion() {
    assert(!is_region_active);
    is_region_active = true;
    assert(replay_instructions.empty());
  }

  void executeCLWB() {
    // TODO
    /*
    The CLWB instruction should always follow a store instruction, that is why we do not have an
    explicit reference to the relevant memory address.
    1. Get last store instruction.
    2. Find out the memory address it would write to.
    3. Enqueue a write-back operation to that address.
    */
  }

  void executeFence() {
    const auto fence_cycles = cache->fence();
    // TODO: increase cycle count with the returned value
  }

  void createCheckpoint() {
    checkpoint.create();
    // TODO: increase cycle count with the returned checkpoint size
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
  }

  void replay(const StoreParsed &store) {
    p_debug << printLeader() << " replaying " << registerNameFriendly(store.r_src)
                             << " to " << registerNameFriendly(store.r_base) << " + " << store.offset
                             << " (size " << store.size << ")" << std::endl;

    // Get the register values
    // TODO: get these register values from the QuickRecall area,
    //       as the processor should have been reset and all registers will be zero
    // TODO: does this incur an NVM write?
    // TODO: does this populate the cache?
    const auto src_value = getRegisterValue(store.r_src);
    const auto base_value = getRegisterValue(store.r_base);

    // Calculate the actual destination address, respecting the offset
    const auto addr = base_value + store.offset;

    p_debug << printLeader() << " replay 0x" << std::hex << std::setw(8) << std::setfill('0') << src_value
            << " to 0x" << std::hex << std::setw(8) << std::setfill('0') << addr << std::dec << std::endl;

    // Inform the cache about the replay
    cache->handleReplay(addr, src_value, store.size);

    // TODO: increase the cycle count with the correct amount of cycles
  }

  void quickRecallRestore() {
    checkpoint.restore();

    // TODO: Increase the cycle count with the correct amount of cycles.
    // TODO: is QuickRecall fully deterministic? Do its NVM reads go through the cache?
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
  auto cache = std::make_shared<_Cache>(emu);

  // Register the hooks with ICEmu
  HM.add(new ReplayCacheIntrinsics(emu, cache));
  HM.add(new ReplayCacheMemoryAccess(emu, cache));
}

// Class that is used by ICEmu to find the register function
// NB.  * MUST BE NAMED "RegisterMyHook"
//      * MUST BE global
RegisterHook RegisterMyHook(registerMyCodeHook);
