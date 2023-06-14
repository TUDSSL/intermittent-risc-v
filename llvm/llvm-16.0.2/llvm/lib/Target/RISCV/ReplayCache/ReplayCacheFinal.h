#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEFINAL_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEFINAL_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class ReplayCacheFinal : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheFinal() : MachineFunctionPass(ID) {}

private:
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace llvm

#endif
