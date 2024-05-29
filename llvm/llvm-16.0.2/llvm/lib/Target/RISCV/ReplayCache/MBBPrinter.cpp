#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "MBBPrinter.h"
#include <iostream>

using namespace llvm;


#define DEBUG_TYPE "replaycache"
#define PASS_NAME "MBBPrinter"

char MBBPrinter::ID = 20;

raw_ostream &output_mbbp = llvm::outs();

bool MBBPrinter::runOnMachineFunction(MachineFunction &MF) 
{
    for (auto &MBB : MF) 
    {
        output_mbbp << MBB;
    }
    return true;
}

FunctionPass *llvm::createMBBPrinterPass() {
    return new MBBPrinter();
}

INITIALIZE_PASS(MBBPrinter, DEBUG_TYPE, PASS_NAME, false, false)
