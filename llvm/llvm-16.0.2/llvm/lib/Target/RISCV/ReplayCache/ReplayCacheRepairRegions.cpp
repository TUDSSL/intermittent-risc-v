/**
 * Repair regions
 * 
 * Add region boundary instructions BEFORE instructions that have the
 * hasRegionBoundaryBefore flag.
 * 
 * Runs after register allocation.
 */
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "ReplayCacheRepairRegions.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;


#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_RepairRegions"

char ReplayCacheRepairRegions::ID = 8;

raw_ostream &output_repair = llvm::outs();

void ReplayCacheRepairRegions::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheRepairRegions::runOnMachineFunction(MachineFunction &MF) 
{
    SLIS = &getAnalysis<SlotIndexes>();
    MachineInstr *PrevMI = nullptr;

    for (auto &MBB : MF) 
    {
        for (MachineInstr &MI : MBB) 
        {
            if (hasRegionBoundaryBefore(MI) && PrevMI != nullptr && !IsStartRegion(*PrevMI))
            {
                ReplayCacheInstruction startRegionInstr = (ReplayCacheInstruction) MI.StartRegionInstr;
                if (startRegionInstr != START_REGION_BRANCH_DEST && startRegionInstr != START_REGION)
                {
                    InsertRegionBoundaryBefore(MBB, MI, startRegionInstr, true);
                }
                else
                {
                    /* Flags for BRANH_DEST are removed, because these are
                     * moved around later. If we keep them, the flags will either be
                     * removed regardless, or they will be put somewhere where we don't want them.
                     */
                    removeRegionBoundaryBefore(MI);
                }
            }

            PrevMI = &MI;
        }
    }
    return true;
}

FunctionPass *llvm::createReplayCacheRepairRegionsPass() {
    return new ReplayCacheRepairRegions();
}

INITIALIZE_PASS(ReplayCacheRepairRegions, DEBUG_TYPE, PASS_NAME, false, false)
