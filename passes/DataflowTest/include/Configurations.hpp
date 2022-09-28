
#pragma once

#include "llvm/Support/CommandLine.h"

using namespace llvm;

/*
 * Command line options for pass
 */
extern cl::opt<bool> NoVerify;

extern cl::opt<bool> Debug;

#ifdef DEBUG_PRINT
#define dbg() errs()
#else
#define dbg() if (Debug == false) {} else errs()
#endif
