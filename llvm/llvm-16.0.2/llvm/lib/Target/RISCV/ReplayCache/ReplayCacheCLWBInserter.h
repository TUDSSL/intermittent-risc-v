#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHECLWBINSERTER_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHECLWBINSERTER_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class ReplayCacheCLWBInserter : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheCLWBInserter() : MachineFunctionPass(ID) {}

private:
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace llvm

#endif
