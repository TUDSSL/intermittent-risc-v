#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGISTERREGIONPARTITIONING_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGISTERREGIONPARTITIONING_H

#include "RISCV.h"
#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/LiveIntervals.h"
#include "llvm/CodeGen/SlotIndexes.h"
// #include "llvm/CodeGen/LiveIntervalCalc.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "Region/ReplayCacheRegionAnalysis.h"
#include <map>
#include <vector>
#include <utility>

namespace llvm {
class ReplayCacheRegisterRegionPartitioning : public MachineFunctionPass {
public:
  static char ID;
  ReplayCacheRegisterRegionPartitioning();
  // ~ReplayCacheRegisterRegionPartitioning();

private:
  struct IntervalsMapEntry
  {
    bool active;
    SlotIndex index;
  };

  static constexpr int NUM_STORE_REGISTERS = 15;

  LiveIntervals *LIS;
  SlotIndexes *SIS;
  ReplayCacheRegionAnalysis *RRS;
  // LiveIntervalCalc *LICalc;
  std::vector<std::pair<LiveInterval*, Register>> Intervals;
  std::map<LiveInterval*, IntervalsMapEntry> IntervalsMap;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  void deactivateLiveIntervals(MachineInstr &MI);
  void extendLiveIntervalsToInstr(MachineFunction &MF, MachineBasicBlock &MBB, MachineInstr &MI);
  int getActiveLiveIntervalsCount();
  void printIntervalsMapContents();
};
} // namespace llvm

#endif
