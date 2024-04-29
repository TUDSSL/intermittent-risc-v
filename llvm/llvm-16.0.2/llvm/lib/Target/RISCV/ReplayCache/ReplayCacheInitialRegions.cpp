#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheInitialRegions.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_InitialRegions"


raw_ostream &output0 = llvm::outs();

char ReplayCacheInitialRegions::ID = 0;

void ReplayCacheInitialRegions::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesCFG();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheInitialRegions::runOnMachineFunction(MachineFunction &MF) {
  output0 << "INITIAL REGION START\n";

  // Skip naked functions
  // TODO: determine eligible functions in an earlier pass
  if (MF.getFunction().hasFnAttribute(Attribute::Naked))
    return false;

  // TODO: make sure this only runs once

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
          BuildRC(MBB, NextMI, NextMI->getDebugLoc(), START_REGION_RETURN);
          output0 << "NEW region started (callee return)!\n";
        } else {
          // If there is no next instruction, the callee returns to the
          // fallthrough
          auto Fallthrough = MBB.getFallThrough();
          if (Fallthrough)
            StartRegionInBB(*Fallthrough, START_REGION_RETURN);
          // If there is no fallthrough, this is the last block in the function
          // and the call is probably a tail call.
        }
      } else if (MI.isReturn()) {
        // No need to 'explicitly' end a region when returning,
        // because the caller is expected to do that already
      } else if (MI.isConditionalBranch()) {
        // Create boundaries BEFORE branches
        BuildRC(MBB, MI, MI.getDebugLoc(), FENCE);
        BuildRC(MBB, MI, MI.getDebugLoc(), START_REGION_BRANCH);

        // Create boundaries at the start of branch basic blocks

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
          
        StartRegionInBB(*TrueDest, START_REGION_BRANCH_DEST);
        if (FalseDest)
          StartRegionInBB(*FalseDest, START_REGION_BRANCH_DEST);
      }
    }
  }
  output0 << "INITIAL REGION END\n";

  return true;
}

FunctionPass *llvm::createReplayCacheInitialRegionsPass() {
  return new ReplayCacheInitialRegions();
}

INITIALIZE_PASS(ReplayCacheInitialRegions, DEBUG_TYPE, PASS_NAME, false, false)
