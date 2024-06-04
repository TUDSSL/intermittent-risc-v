#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheMarkBranches.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include <iostream>

using namespace llvm;


#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_MarkBranches"

char ReplayCacheMarkBranches::ID = 9;

raw_ostream &output_markbranches = llvm::outs();


void ReplayCacheMarkBranches::releaseMemory()
{
    MarkedBlocks.clear();
}

bool ReplayCacheMarkBranches::runOnMachineFunction(MachineFunction &MF) 
{
    MachineInstr *PrevMI = nullptr;

    for (auto &MBB : MF) 
    {
        for (auto &MI : MBB)
        {
            if (MI.isConditionalBranch() && hasRegionBoundaryBefore(MI) && PrevMI != nullptr && !IsRC(*PrevMI))
            {
                output_markbranches << "Has branch without boundary!\n";
            }

            PrevMI = &MI;
        }
    }
    return true;
}

FunctionPass *llvm::createReplayCacheMarkBranchesPass() {
    return new ReplayCacheMarkBranches();
}

INITIALIZE_PASS(ReplayCacheMarkBranches, DEBUG_TYPE, PASS_NAME, false, false)
