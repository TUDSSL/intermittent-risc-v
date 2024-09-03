/**
 * Repair Regions 2
 * 
 * Repairs regions to be in the correct place by recomputing
 * the initial regions for the rearranged instructions.
 * 
 * Runs at the end of all passes, but BEFORE the stack spill pass.
 */
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
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

/**
 * Repairs region boundaries by recomputing the initial regions.
 * Inserts region boundary instructions for initial regions that are not in the code yet.
 */
bool ReplayCacheRepairRegions2::runOnMachineFunction(MachineFunction &MF) 
{
    SLIS = &getAnalysis<SlotIndexes>();
    MachineInstr *PrevMI = nullptr;
    auto &TII = *MF.getSubtarget().getInstrInfo();

    /* Recompute initial regions, and add flags to all instructions
     * that need region boundaries before them.
     */
    for (auto &MBB : MF)
    {
        for (auto &MI : MBB)
        {
            auto NextMI = MI.getNextNode();
            
            if (MI.isConditionalBranch())
            {
                auto TrueDest = TII.getBranchDestBlock(MI);
                MachineBasicBlock *FalseDest = nullptr;

                if (NextMI) 
                {
                    if (NextMI->isUnconditionalBranch()) 
                    {
                        FalseDest = TII.getBranchDestBlock(*NextMI);
                    }
                    else 
                    {
                        assert(false && "Branch should not have instruction that is not jump.");
                    }
                } 
                else 
                {
                    FalseDest = MBB.getFallThrough();
                }

                StartRegionInBB(*TrueDest, START_REGION_BRANCH_DEST, false);
                if (FalseDest != nullptr)
                {
                    StartRegionInBB(*FalseDest, START_REGION_BRANCH_DEST, false);
                }
            } 
            else if (MI.isCall()) 
            {
                if (NextMI) {
                    InsertRegionBoundaryBefore(MBB, *NextMI, START_REGION_RETURN, false);
                } 
                else 
                {
                    auto Fallthrough = MBB.getFallThrough();
                    if (Fallthrough) 
                    {
                        StartRegionInBB(*Fallthrough, START_REGION_RETURN, false);
                    }
                }
            }
        }
    }

    /* Add region boundaries before instructions that are flagged. */
    for (auto &MBB : MF) 
    {
        /* Add new region for entry block, because new instructions may have been
         * inserted before the original entry block region in previous passes.
         *
         * This could lead to duplicate regions, but one of those regions will be
         * empty and thus would not contribute to any latency/slowdown.
         */
        if (MBB.isEntryBlock() && !IsStartRegion(*MBB.begin()))
        {
            StartRegionInBB(MBB, START_REGION, true);
            SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
        }

        for (auto &MI : MBB) 
        {
            if (hasRegionBoundaryBefore(MI) && PrevMI != nullptr && !IsStartRegion(*PrevMI) && !IsStartRegion(MI))
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
