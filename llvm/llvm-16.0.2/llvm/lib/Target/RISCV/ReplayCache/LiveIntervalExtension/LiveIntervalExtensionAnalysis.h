#pragma once

#include "ExtendedLiveInterval.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/ADT/DenseMap.h"

class LiveIntervalExtensionAnalysis : public MachineFunctionPass
{
private:
    using ExtendedLiveIntervalMap = DenseMap<unsigned, ExtendedLiveInterval *>;

    BumpPtrAllocator ExtensionAllocator_;
    ExtendedLiveIntervalMap ExtensionMap_;

    void computeExtensionFromLI(LiveInterval &LI);
public:
    static char ID;

    LiveIntervalExtensionAnalysis();

    void getAnalysisUsage(AnalysisUsage &AU) const override;
    void releaseMemory() override;
    bool runOnMachineFunction(MachineFunction &MF) override;

    ExtendedLiveInterval &getExtensionFromLI(LiveInterval &LI);
    void recomputeExtension(ExtendedLiveInterval &EX);
};
