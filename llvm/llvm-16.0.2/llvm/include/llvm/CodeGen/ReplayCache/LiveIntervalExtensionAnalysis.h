// #pragma once

// #include "ExtendedLiveInterval.h"
// #include "llvm/CodeGen/MachineFunctionPass.h"
// #include "llvm/ADT/DenseMap.h"
// #include "llvm/CodeGen/SlotIndexes.h"
// #include "llvm/CodeGen/LiveIntervals.h"
// #include "ReplayCacheRegionAnalysis.h"

// class LiveIntervalExtensionAnalysis : public MachineFunctionPass
// {
// private:
//     using ExtendedLiveIntervalMap = DenseMap<unsigned, ExtendedLiveInterval *>;

//     BumpPtrAllocator ExtensionAllocator_;
//     ExtendedLiveIntervalMap ExtensionMap_;

//     ReplayCacheRegionAnalysis *RRA_;
//     LiveIntervals *LIS_;
//     SlotIndexes *SLIS_;

//     ReplayCacheRegion *CurrentRegion_;

//     void computeExtensionFromLI(MachineInstr &MI, LiveInterval &LI);
//     std::vector<SlotInterval> getSlotIntervalsInRegionFrom(MachineInstr &MI);
//     std::vector<SlotInterval> removeOverlappingIntervals(LiveInterval &LI, std::vector<SlotInterval> &SlotIntervals);
//     void addExtensionToInterval(LiveRangeUpdater &LRU, ExtendedLiveInterval *ELR);
// public:
//     static char ID;

//     LiveIntervalExtensionAnalysis();

//     void getAnalysisUsage(AnalysisUsage &AU) const override;
//     void releaseMemory() override;
//     bool runOnMachineFunction(MachineFunction &MF) override;

//     ExtendedLiveInterval *getExtensionFromLI(const LiveInterval &LI);
//     void recomputeActiveExtensions(SlotIndex SI);

//     void addExtensionsToIntervals();

//     unsigned getExtensionPressureAt(MachineInstr &MI);
// };
