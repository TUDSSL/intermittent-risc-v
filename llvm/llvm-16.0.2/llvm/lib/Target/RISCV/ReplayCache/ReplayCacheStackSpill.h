#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHESTACKSPILL_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHESTACKSPILL_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheRegionAnalysis.h"

namespace llvm {
class ReplayCacheStackSpill : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheStackSpill() : MachineFunctionPass(ID) {}

private:
  ReplayCacheRegionAnalysis *RRA = nullptr;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void checkRegionForStackSpill(MachineFunction &MF, MachineInstr &MI);

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // namespace llvm

#endif
