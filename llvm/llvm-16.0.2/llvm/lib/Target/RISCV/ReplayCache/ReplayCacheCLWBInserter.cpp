#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheCLWBInserter.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_CLWBInserter"

char ReplayCacheCLWBInserter::ID = 2;

bool ReplayCacheCLWBInserter::runOnMachineFunction(MachineFunction &MF) 
{
    for (auto &MBB : MF) 
    {
        for (auto &MI : MBB)
        {
            auto NextMI = MI.getNextNode();

            if (NextMI && isStoreInstruction(MI))
            {
                BuildRC(MBB, *NextMI, NextMI->getDebugLoc(), CLWB);
            }
        }
    }
    return true;
}

FunctionPass *llvm::createReplayCacheCLWBInserterPass() {
    return new ReplayCacheCLWBInserter();
}

INITIALIZE_PASS(ReplayCacheCLWBInserter, DEBUG_TYPE, PASS_NAME, false, false)
