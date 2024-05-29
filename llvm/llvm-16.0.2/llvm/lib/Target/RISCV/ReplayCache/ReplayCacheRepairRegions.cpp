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
    // AU.setPreservesCFG();
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheRepairRegions::runOnMachineFunction(MachineFunction &MF) 
{
    SLIS = &getAnalysis<SlotIndexes>();

    for (auto &MBB : MF) 
    {
        if (MBB.isEntryBlock())
        {
            StartRegionInBB(MBB, START_REGION, true);
            SLIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());
        }
    }
    return true;
}

FunctionPass *llvm::createReplayCacheRepairRegionsPass() {
    return new ReplayCacheRepairRegions();
}

INITIALIZE_PASS(ReplayCacheRepairRegions, DEBUG_TYPE, PASS_NAME, false, false)
