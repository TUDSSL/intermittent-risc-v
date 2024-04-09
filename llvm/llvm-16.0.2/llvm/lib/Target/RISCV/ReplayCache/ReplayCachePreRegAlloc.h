#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEPREREGALLOC_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEPREREGALLOC_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/LiveIntervals.h"

namespace llvm {
class ReplayCachePreRegAlloc : public MachineFunctionPass {
public:
  static char ID;

  ReplayCachePreRegAlloc();

private:
  static constexpr int NUM_STORE_REGISTERS = 2;
  
  std::vector<LiveInterval*> Intervals;

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // namespace llvm

#endif
