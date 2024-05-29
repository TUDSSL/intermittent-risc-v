// #include "RISCVSubtarget.h"
// #include "RISCVTargetMachine.h"
// #include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
// #include "ReplayCacheRegionInserter.h"
// #include "llvm/CodeGen/MachineInstrBuilder.h"
// #include <iostream>

// using namespace llvm;


// #define DEBUG_TYPE "replaycache"
// #define PASS_NAME "ReplayCache_RegionInserter"

// char ReplayCacheRegionInserter::ID = 7;

// raw_ostream &output_rins = llvm::outs();

// bool ReplayCacheRegionInserter::runOnMachineFunction(MachineFunction &MF) 
// {
//     for (auto &MBB : MF) 
//     {
//         for (auto &MI : MBB)
//         {
//             if (hasRegionBoundaryBefore(MI))
//             {

//                 output_rins << "Has region boundary before!\n";
//                 BuildRC(MBB, MI, MI.getDebugLoc(), ReplayCacheInstruction::FENCE);
//                 BuildRC(MBB, MI, MI.getDebugLoc(), ReplayCacheInstruction::START_REGION);
//             }
//         }
//     }

//     return true;
// }

// FunctionPass *llvm::createReplayCacheRegionInserterPass() {
//     return new ReplayCacheRegionInserter();
// }

// INITIALIZE_PASS(ReplayCacheRegionInserter, DEBUG_TYPE, PASS_NAME, false, false)
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "ReplayCacheRegionInserter.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_RegionInserter"


raw_ostream &output_rins = llvm::outs();

char ReplayCacheRegionInserter::ID = 7;

void ReplayCacheRegionInserter::getAnalysisUsage(AnalysisUsage &AU) const
{
    // AU.setPreservesCFG();
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheRegionInserter::runOnMachineFunction(MachineFunction &MF) {
  // for (auto &MBB : MF) {
  //   for (auto &MI : MBB) {
  //     if (isStoreInstruction(MI))
  //     {
  //       for (const auto *Op : MI.memoperands()) {
  //         if (const PseudoSourceValue *PVal = Op->getPseudoValue())
  //         {
  //           if (PVal->kind() == PseudoSourceValue::Stack || PVal->kind() == PseudoSourceValue::FixedStack)
  //           {
  //             output_rins << "Stack spilled: " << MI;
  //           }
  //         }
  //       }
  //     }
  //     if (isLoadInstruction(MI))
  //     {
  //       for (const auto *Op : MI.memoperands()) {
  //         if (const PseudoSourceValue *PVal = Op->getPseudoValue())
  //         {
  //           if (PVal->kind() == PseudoSourceValue::Stack || PVal->kind() == PseudoSourceValue::FixedStack)
  //           {
  //             output_rins << "Stack loaded: " << MI;
  //           }
  //         }
  //       }
  //     }
  //   }
  // }

  // output0 << "INITIAL REGION START\n";

  // output0 << "============================================\n";
  // output0 << "=                INIT                      =\n";
  // output0 << "============================================\n";

  // SLIS = &getAnalysis<SlotIndexes>();

  // // Skip naked functions
  // // TODO: determine eligible functions in an earlier pass
  // if (MF.getFunction().hasFnAttribute(Attribute::Naked))
  //   return false;

  // // TODO: make sure this only runs once

  // auto &TII = *MF.getSubtarget().getInstrInfo();

  // for (auto &MBB : MF) {

  //   if (MBB.isEntryBlock())
  //   {
  //     StartRegionInBB(MBB, START_REGION, true);
  //     // SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
  //     // output0 << "Entry block region started!\n";
  //   }
      

  //   for (auto &MI : MBB) {
  //     // Skip any instructions that were already inserted by this pass
  //   //   if (IsRC(MI))
  //   //     continue;

  //     auto NextMI = MI.getNextNode();

  //     if (MI.isCall()) {
  //       // Start a new region when callee returns
  //       if (NextMI) {
  //         InsertRegionBoundaryBefore(MBB, *NextMI, START_REGION_RETURN, true);
  //         // BuildRC(MBB, NextMI, NextMI->getDebugLoc(), FENCE);
  //         // BuildRC(MBB, NextMI, NextMI->getDebugLoc(), START_REGION_RETURN);
  //         // SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
  //         // output0 << "NEW region started (callee return)!\n";
  //       } else {
  //         // If there is no next instruction, the callee returns to the
  //         // fallthrough
  //         auto Fallthrough = MBB.getFallThrough();
  //         if (Fallthrough) {
  //           StartRegionInBB(*Fallthrough, START_REGION_RETURN, true);
  //           // SLIS->repairIndexesInRange(Fallthrough, Fallthrough->begin(), Fallthrough->end());
  //         }
  //         // If there is no fallthrough, this is the last block in the function
  //         // and the call is probably a tail call.
  //       }
  //     } else if (MI.isReturn()) {
  //       // No need to 'explicitly' end a region when returning,
  //       // because the caller is expected to do that already
  //     } else if (MI.isConditionalBranch()) {
  //       // output0 << MBB;
  //       // output0 << MI;
  //       // output0 << "---------------------------------------------\n";
  //       // Create boundaries BEFORE branches
  //       InsertRegionBoundaryBefore(MBB, MI, START_REGION_BRANCH, true);
  //       // SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());

  //       // Create boundaries at the start of branch basic blocks

  //       auto TrueDest = TII.getBranchDestBlock(MI);
  //       MachineBasicBlock *FalseDest = nullptr;

  //       if (NextMI) {
  //         // If the next instruction is an unconditional branch, where it jumps
  //         // to should be interpreted as the false branch
  //         if (NextMI->isUnconditionalBranch()) {
  //           FalseDest = TII.getBranchDestBlock(*NextMI);
  //         }
  //         // else: fall through within basic block.
  //         // else {
  //         //   BuildRC(MBB, NextMI, NextMI->getDebugLoc(), FENCE);
  //         //   BuildRC(MBB, NextMI, NextMI->getDebugLoc(), START_REGION_BRANCH_DEST);
  //         //   // SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
  //         // }
  //       } else {
  //         // If there is no next instruction, the false branch is the fallthrough
  //         FalseDest = MBB.getFallThrough();
  //       }
          
  //       StartRegionInBB(*TrueDest, START_REGION_BRANCH_DEST, true);
  //       // SLIS->repairIndexesInRange(TrueDest, TrueDest->begin(), TrueDest->end());
  //       if (FalseDest)
  //       {
  //         StartRegionInBB(*FalseDest, START_REGION_BRANCH_DEST, true);
  //         // SLIS->repairIndexesInRange(FalseDest, FalseDest->begin(), FalseDest->end());
  //       }
  //     }
  //   }


  //   // SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
  // }
  
  // for (auto &MBB : MF) {
  //   for (auto &MI : MBB) {
  //     if (MI.isConditionalBranch()) {
  //       output0 << MBB;
  //       output0 << "--------------------\n";
  //     }
  //   }
  // }

  return true;
}

FunctionPass *llvm::createReplayCacheRegionInserterPass() {
  return new ReplayCacheRegionInserter();
}

INITIALIZE_PASS(ReplayCacheRegionInserter, DEBUG_TYPE, PASS_NAME, false, false)
