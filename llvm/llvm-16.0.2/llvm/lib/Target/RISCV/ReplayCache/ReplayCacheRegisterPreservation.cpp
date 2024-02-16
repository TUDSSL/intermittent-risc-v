#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheRegisterPreservation.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_RegisterPreservation"

char ReplayCacheRegisterPreservation::ID = 2;

bool ReplayCacheRegisterPreservation::runOnMachineFunction(MachineFunction &MF) {
    return false;
}

FunctionPass *llvm::createReplayCacheRegisterPreservationPass() {
    return new ReplayCacheRegisterPreservation();
}

INITIALIZE_PASS(ReplayCacheRegisterPreservation, DEBUG_TYPE, PASS_NAME, false, false)
