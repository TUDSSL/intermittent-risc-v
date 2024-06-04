#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_MBBPRINTER_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_MBBPRINTER_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class MBBPrinter : public MachineFunctionPass {
public:
  static char ID;

  MBBPrinter() : MachineFunctionPass(ID) {}

private:
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace llvm

#endif
