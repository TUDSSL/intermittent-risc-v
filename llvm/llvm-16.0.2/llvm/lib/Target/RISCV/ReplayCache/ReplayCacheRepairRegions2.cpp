#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "ReplayCacheRepairRegions2.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;


#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_RepairRegions2"

char ReplayCacheRepairRegions2::ID = 8;

raw_ostream &output_repair2 = llvm::outs();

void ReplayCacheRepairRegions2::getAnalysisUsage(AnalysisUsage &AU) const
{
    // AU.setPreservesCFG();
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheRepairRegions2::runOnMachineFunction(MachineFunction &MF) 
{
    SLIS = &getAnalysis<SlotIndexes>();
    MachineInstr *PrevMI = nullptr;
    auto &TII = *MF.getSubtarget().getInstrInfo();

    for (auto &MBB : MF)
    {
        for (auto &MI : MBB)
        {
            auto NextMI = MI.getNextNode();
            
            if (MI.isConditionalBranch())
            {
                auto TrueDest = TII.getBranchDestBlock(MI);
                MachineBasicBlock *FalseDest = nullptr;

                if (NextMI) {
                    // If the next instruction is an unconditional branch, where it jumps
                    // to should be interpreted as the false branch
                    if (NextMI->isUnconditionalBranch()) {
                        FalseDest = TII.getBranchDestBlock(*NextMI);
                    }
                    else {
                        assert(false && "Branch should not have instruction that is not jump.");
                    }
                } else {
                    // If there is no next instruction, the false branch is the fallthrough
                    FalseDest = MBB.getFallThrough();
                }

                StartRegionInBB(*TrueDest, START_REGION_BRANCH_DEST, false);
                if (FalseDest != nullptr)
                {
                    StartRegionInBB(*FalseDest, START_REGION_BRANCH_DEST, false);
                }
                
            } else if (MI.isCall()) {
                if (NextMI) {
                    InsertRegionBoundaryBefore(MBB, *NextMI, START_REGION_RETURN, false);
                } else {
                    auto Fallthrough = MBB.getFallThrough();
                    if (Fallthrough) {
                        StartRegionInBB(*Fallthrough, START_REGION_RETURN, false);
                    }
                }
            }
        }
    }

    for (auto &MBB : MF) 
    {
        if (MBB.isEntryBlock())
        {
            StartRegionInBB(MBB, START_REGION, true);
            SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
        }

        for (auto &MI : MBB) 
        {
            if (hasRegionBoundaryBefore(MI) && PrevMI != nullptr && !IsStartRegion(*PrevMI))
            {
                InsertRegionBoundaryBefore(MBB, MI, (ReplayCacheInstruction) MI.StartRegionInstr, true);
                SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
            }

            PrevMI = &MI;
        }
    }
    return true;
}

FunctionPass *llvm::createReplayCacheRepairRegions2Pass() {
    return new ReplayCacheRepairRegions2();
}

INITIALIZE_PASS(ReplayCacheRepairRegions2, DEBUG_TYPE, PASS_NAME, false, false)
