#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEINITIALREGIONS_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEINITIALREGIONS_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/SlotIndexes.h"

namespace llvm {
class ReplayCacheInitialRegions : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheInitialRegions() : MachineFunctionPass(ID) {}

private:
  SlotIndexes *SLIS = nullptr;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // namespace llvm

#endif
