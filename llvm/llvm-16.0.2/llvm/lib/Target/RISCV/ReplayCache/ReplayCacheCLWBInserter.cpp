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
    // output_clwb << "============================================\n";
    // output_clwb << "=                CLWB                      =\n";
    // output_clwb << "============================================\n";
    for (auto &MBB : MF) 
    {
        // output_clwb << MBB;
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
