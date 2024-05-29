#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "BBPrinter.h"
#include <iostream>

using namespace llvm;


#define DEBUG_TYPE "replaycache"
#define PASS_NAME "BBPrinter"

char BBPrinter::ID = 20;

raw_ostream &output_bbp = llvm::outs();

bool BBPrinter::runOnFunction(Function &F) 
{
    for (auto &BB : F) 
    {
        output_bbp << BB;
    }
    return true;
}

FunctionPass *llvm::createBBPrinterPass() {
    return new BBPrinter();
}

INITIALIZE_PASS(BBPrinter, DEBUG_TYPE, PASS_NAME, false, false)
