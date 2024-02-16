#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGISTERREGIONPARTITIONING_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGISTERREGIONPARTITIONING_H

#include "RISCV.h"
#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include <map>
#include <vector>
#include <utility>

namespace llvm {
class ReplayCacheRegisterRegionPartitioning : public MachineFunctionPass {
public:
  static char ID;
  ReplayCacheRegisterRegionPartitioning();

private:
  static constexpr int NUM_STORE_REGISTERS = 15;

  LiveIntervals *LIS;
  std::map<LiveInterval*, std::pair<bool, std::vector<SlotIndex>>> IntervalsMap;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  void deactivateLiveIntervals(MachineInstr &MI);
  void extendLiveIntervalsToInstr(MachineInstr &MI);
  void printIntervalsMapContents();
};
} // namespace llvm

#endif
