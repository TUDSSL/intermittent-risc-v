#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHESTACKSPILLPREVENTION_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHESTACKSPILLPREVENTION_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class ReplayCacheStackSpillPrevention : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheStackSpillPrevention() : MachineFunctionPass(ID) {}

private:
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace llvm

#endif
