#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheInitialRegions.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_InitialRegions"


raw_ostream &output0 = llvm::outs();

char ReplayCacheInitialRegions::ID = 0;

bool ReplayCacheInitialRegions::runOnMachineFunction(MachineFunction &MF) {
  // Skip naked functions
  // TODO: determine eligible functions in an earlier pass
  if (MF.getFunction().hasFnAttribute(Attribute::Naked))
    return false;

  auto &TII = *MF.getSubtarget().getInstrInfo();

  for (auto &MBB : MF) {

    if (MBB.isEntryBlock())
    {
      StartRegionInBB(MBB);
      output0 << "Entry block region started!\n";
    }
      

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
          output0 << "NEW region started (callee return)!\n";
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
            output0 << "Add CLWB instruction!\n";
            break;
          }
        }
      }
    }
  }

  return true;
}

FunctionPass *llvm::createReplayCacheInitialRegionsPass() {
  return new ReplayCacheInitialRegions();
}

INITIALIZE_PASS(ReplayCacheInitialRegions, DEBUG_TYPE, PASS_NAME, false, false)
