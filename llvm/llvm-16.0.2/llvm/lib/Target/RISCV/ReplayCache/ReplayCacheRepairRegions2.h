#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREPAIRREGIONS2_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREPAIRREGIONS2_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class ReplayCacheRepairRegions2 : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheRepairRegions2() : MachineFunctionPass(ID) {}

private:
  SlotIndexes *SLIS = nullptr;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // namespace llvm

#endif
