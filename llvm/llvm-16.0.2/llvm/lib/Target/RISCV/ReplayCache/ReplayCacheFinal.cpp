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
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), START_REGION);
}

bool ReplayCacheFinal::runOnMachineFunction(MachineFunction &MF) {
  // Skip naked functions
  // TODO: determine eligible functions in an earlier pass
  if (MF.getFunction().hasFnAttribute("naked"))
    return false;

  auto &TII = *MF.getSubtarget().getInstrInfo();

  for (auto &MBB : MF) {

    if (MBB.isEntryBlock())
      StartRegionInBB(MBB);

    for (auto &MI : MBB) {

      if (IsRC(MI))
        continue;

      if (MI.isCall()) {
        BuildRC(MBB, MI, MI.getDebugLoc(), FENCE);
        auto Next = MI.getNextNode();
        if (Next) {
          BuildRC(MBB, Next, Next->getDebugLoc(), START_REGION);
        }
      } else if (MI.isReturn()) {
        BuildRC(MBB, MI, MI.getDebugLoc(), FENCE);
      } else if (MI.isBranch()) {
        BuildRC(MBB, MI, MI.getDebugLoc(), FENCE);
        MachineBasicBlock *TBB = nullptr, *FBB = nullptr;
        SmallVector<MachineOperand, 4> Cond;
        if (!TII.analyzeBranch(MBB, TBB, FBB, Cond)) {
          if (TBB)
            StartRegionInBB(*TBB);
          if (FBB)
            StartRegionInBB(*FBB);
        } else {
          // TODO: is this is a problen?
        }
      }

      switch (MI.getOpcode()) {
      default:
        break;
      case RISCV::SW:
      case RISCV::SH:
      case RISCV::SB:
      case RISCV::C_SW:
      case RISCV::C_SWSP:
        BuildRC(MBB, MI.getNextNode(), MI.getDebugLoc(), CLWB);
        break;
      }
    }
  }

  return true;
}

FunctionPass *llvm::createReplayCacheFinalPass() {
  return new ReplayCacheFinal();
}

INITIALIZE_PASS(ReplayCacheFinal, DEBUG_TYPE, PASS_NAME, false, false)
