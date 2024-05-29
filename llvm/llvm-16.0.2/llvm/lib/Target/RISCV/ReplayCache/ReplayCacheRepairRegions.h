#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREPAIRREGIONS_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREPAIRREGIONS_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class ReplayCacheRepairRegions : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheRepairRegions() : MachineFunctionPass(ID) {}

private:
  SlotIndexes *SLIS = nullptr;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // namespace llvm

#endif
