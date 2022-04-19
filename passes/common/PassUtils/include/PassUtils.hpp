#pragma once

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include <functional>

#include "noelle/core/Noelle.hpp"

namespace PassUtils {

llvm::Function *GetMethod(llvm::Module *M, const std::string Name);

/*
 * Injection helpers
 */
llvm::IRBuilder<> GetBuilder(llvm::Function *F, llvm::Instruction *InsertionPoint);

llvm::IRBuilder<> GetBuilder(llvm::Function *F, llvm::BasicBlock *InsertionPoint);

bool IsInstrumentable(llvm::Function &F);

/*
 * Verification helpers
 */
bool Verify(llvm::Module &M);

/*
 * Metadata handlers
 */
void SetInstrumentationMetadata(llvm::Instruction *I, const std::string MDTypeString,
                                const std::string MDLiteral);

/*
 * Global Variable helper
 */
llvm::GlobalVariable *GetOrInsertGlobalInteger(llvm::Module *M,
                                               llvm::IntegerType *Type,
                                               const std::string Name,
                                               uint64_t Initial = 0,
                                               size_t Alignment = 4);

/*
 * Iteration Helper
 *
 * FunctionToInvokePerInstruction returns [bool, bool]
 * Stop, StopPath
 *
 * Function returns true if the iteration ends earlier.
 * It returns false otherwise.
 */
bool ReverseIterateOverInstructions(
    llvm::Instruction *From, llvm::Instruction *To,
    std::function<std::pair<bool, bool>(Instruction *I)>
        FucntionToInvokePerInstruction,
    bool DebugPrint = false);

}  // namespace PassUtils
