#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheStackSpillPrevention.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_StackSpillPrevention"

char ReplayCacheStackSpillPrevention::ID = 3;

bool ReplayCacheStackSpillPrevention::runOnMachineFunction(MachineFunction &MF) {
    return false;
}

FunctionPass *llvm::createReplayCacheStackSpillPreventionPass() {
    return new ReplayCacheStackSpillPrevention();
}

INITIALIZE_PASS(ReplayCacheStackSpillPrevention, DEBUG_TYPE, PASS_NAME, false, false)
