/**
 * CLWB inserted
 * 
 * Inserts CLWB instructions after every store instruction.
 * 
 * Runs at the very end of all passes.
 */
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "ReplayCacheCLWBInserter.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;


#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_CLWBInserter"

char ReplayCacheCLWBInserter::ID = 2;

raw_ostream &output_clwb = llvm::outs();

bool ReplayCacheCLWBInserter::runOnMachineFunction(MachineFunction &MF) 
{
    for (auto &MBB : MF) 
    {
        for (auto MI = MBB.begin(); MI != MBB.end(); MI++)
        {
            if (isStoreInstruction(*MI))
            {
                MBB.insertAfter(MI, BuildRC(MBB, *MI, MI->getDebugLoc(), CLWB));
            }
        }
    }
    return true;
}

FunctionPass *llvm::createReplayCacheCLWBInserterPass() {
    return new ReplayCacheCLWBInserter();
}

INITIALIZE_PASS(ReplayCacheCLWBInserter, DEBUG_TYPE, PASS_NAME, false, false)
