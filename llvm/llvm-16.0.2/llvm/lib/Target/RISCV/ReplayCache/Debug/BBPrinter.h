#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_BBPRINTER_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_BBPRINTER_H

#include "RISCV.h"
#include "llvm/Pass.h"

namespace llvm {
class BBPrinter : public FunctionPass {
public:
  static char ID;

  BBPrinter() : FunctionPass(ID) {}

private:
  bool runOnFunction(Function &F) override;
};
} // namespace llvm

#endif
