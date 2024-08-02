/**
 * Initial regions
 * 
 * Inserts regions in the following places:
 * - At the start of a function.
 * - After a function call.
 * - Before a branch instruction.
 * - At each branch destination (including fallthrough).
 * 
 * Runs before all other replaycache passes.
 */
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
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
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheInitialRegions::runOnMachineFunction(MachineFunction &MF)
{
  SLIS = &getAnalysis<SlotIndexes>();

  // Skip naked functions
  if (MF.getFunction().hasFnAttribute(Attribute::Naked))
    return false;

  auto &TII = *MF.getSubtarget().getInstrInfo();

  for (auto &MBB : MF) {

    if (MBB.isEntryBlock())
    {
      StartRegionInBB(MBB, START_REGION, false);
      SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
    }
      

    for (auto &MI : MBB) {

      // Skip any instructions that were already inserted by this pass
      if (IsRC(MI))
        continue;

      auto NextMI = MI.getNextNode();

      if (MI.isCall()) {
        // Start a new region when callee returns
        if (NextMI) {
          InsertRegionBoundaryBefore(MBB, *NextMI, START_REGION_RETURN, false);
          SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
        } else {
          // If there is no next instruction, the callee returns to the
          // fallthrough
          auto Fallthrough = MBB.getFallThrough();
          if (Fallthrough) {
            StartRegionInBB(*Fallthrough, START_REGION_RETURN, false);
            SLIS->repairIndexesInRange(Fallthrough, Fallthrough->begin(), Fallthrough->end());
          }
          // If there is no fallthrough, this is the last block in the function
          // and the call is probably a tail call.
        }
      } else if (MI.isReturn()) {
        // No need to 'explicitly' end a region when returning,
        // because the caller is expected to do that already
      } else if (MI.isConditionalBranch()) {
        // Create boundaries BEFORE branches
        // InsertRegionBoundaryBefore(MBB, MI, START_REGION_BRANCH, false);
        // SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());

        // Create boundaries at the start of branch basic blocks
        auto TrueDest = TII.getBranchDestBlock(MI);
        MachineBasicBlock *FalseDest = nullptr;

        if (NextMI) {
          // If the next instruction is an unconditional branch, where it jumps
          // to should be interpreted as the false branch
          if (NextMI->isUnconditionalBranch()) {
            FalseDest = TII.getBranchDestBlock(*NextMI);
          }
        } else {
          // If there is no next instruction, the false branch is the fallthrough
          FalseDest = MBB.getFallThrough();
        }
          
        StartRegionInBB(*TrueDest, START_REGION_BRANCH_DEST, false);
        SLIS->repairIndexesInRange(TrueDest, TrueDest->begin(), TrueDest->end());
        if (FalseDest)
        {
          StartRegionInBB(*FalseDest, START_REGION_BRANCH_DEST, false);
          SLIS->repairIndexesInRange(FalseDest, FalseDest->begin(), FalseDest->end());
        }
      }
    }


    SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
  }

  return true;
}

FunctionPass *llvm::createReplayCacheInitialRegionsPass() {
  return new ReplayCacheInitialRegions();
}

INITIALIZE_PASS(ReplayCacheInitialRegions, DEBUG_TYPE, PASS_NAME, false, false)
