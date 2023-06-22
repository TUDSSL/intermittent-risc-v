#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheFinal.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache final"

char ReplayCacheFinal::ID = 0;

enum ReplayCacheInstruction {
  START_REGION,
  CLWB,
  FENCE,
  POWER_FAILURE_NEXT,
};

static void BuildRC(MachineBasicBlock &MBB, MachineInstr &MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  BuildMI(MBB, MI, DL, TII.get(RISCV::C_LI), RISCV::X31)
      .addImm(Instr);
}

static void BuildRC(MachineBasicBlock &MBB, MachineInstr *MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  BuildMI(MBB, MI, DL, TII.get(RISCV::C_LI), RISCV::X31)
      .addImm(Instr);
}

/**
 * Check if the instruction is a ReplayCache instruction.
 * Ideally this would've been done by a special MIFlag, but those flags are
 * stored in uint16_t and there are already 16 flags defined.
 */
static bool IsRC(const MachineInstr &MI) {
  auto &TII = *MI.getParent()->getParent()->getSubtarget().getInstrInfo();
  return MI.getOpcode() == TII.get(RISCV::C_LI).getOpcode() &&
         MI.getOperand(0).getReg() == RISCV::X31;
}

static void StartRegionInBB(MachineBasicBlock &MBB) {
  auto FirstNonPHI = MBB.getFirstNonPHI();
  if (FirstNonPHI == MBB.instr_end())
    return;
  auto &EntryInstr = *FirstNonPHI;
  if (IsRC(EntryInstr))
    return;
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), FENCE);
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), START_REGION);
}

bool ReplayCacheFinal::runOnMachineFunction(MachineFunction &MF) {
  // Skip naked functions
  // TODO: determine eligible functions in an earlier pass
  if (MF.getFunction().hasFnAttribute(Attribute::Naked))
    return false;

  auto &TII = *MF.getSubtarget().getInstrInfo();

  for (auto &MBB : MF) {

    if (MBB.isEntryBlock())
      StartRegionInBB(MBB);

    for (auto &MI : MBB) {

      // Skip any instructions that were already inserted by this pass
      if (IsRC(MI))
        continue;

      auto NextMI = MI.getNextNode();

      if (MI.isCall()) {
        // Start a new region when callee returns
        if (NextMI) {
          BuildRC(MBB, NextMI, NextMI->getDebugLoc(), FENCE);
          BuildRC(MBB, NextMI, NextMI->getDebugLoc(), START_REGION);
        } else {
          // If there is no next instruction, the callee returns to the
          // fallthrough
          auto Fallthrough = MBB.getFallThrough();
          if (Fallthrough)
            StartRegionInBB(*Fallthrough);
          // If there is no fallthrough, this is the last block in the function
          // and the call is probably a tail call.
        }
      } else if (MI.isReturn()) {
        // No need to 'explicitly' end a region when returning,
        // because the caller is expected to do that already
      } else if (MI.isConditionalBranch()) {
        // Create boundaries around branches
        auto TrueDest = TII.getBranchDestBlock(MI);
        MachineBasicBlock *FalseDest = nullptr;

        if (NextMI) {
          // If the next instruction is an unconditional branch, where it jumps
          // to should be interpreted as the false branch
          if (NextMI->isUnconditionalBranch()) {
            FalseDest = TII.getBranchDestBlock(*NextMI);
          }
          // else: nothing of interest (?)
        } else {
          // If there is no next instruction, the false branch is the fallthrough
          FalseDest = MBB.getFallThrough();
        }

        StartRegionInBB(*TrueDest);
        if (FalseDest)
          StartRegionInBB(*FalseDest);
      } else {
        // Insert CLWB after stores
        if (NextMI) {
          switch (MI.getOpcode()) {
          default:
            break;
          case RISCV::SW:
          case RISCV::SH:
          case RISCV::SB:
          case RISCV::C_SW:
          case RISCV::C_SWSP:
            BuildRC(MBB, NextMI, MI.getDebugLoc(), CLWB);
            break;
          }
        }
      }
    }
  }

  return true;
}

FunctionPass *llvm::createReplayCacheFinalPass() {
  return new ReplayCacheFinal();
}

INITIALIZE_PASS(ReplayCacheFinal, DEBUG_TYPE, PASS_NAME, false, false)
