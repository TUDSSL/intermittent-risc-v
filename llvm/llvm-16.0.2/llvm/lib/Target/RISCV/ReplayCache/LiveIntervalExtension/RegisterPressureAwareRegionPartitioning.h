#pragma once

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/ReplayCache/LiveIntervalExtensionAnalysis.h"

class RegisterPressureAwareRegionPartitioning : public MachineFunctionPass
{
private:
    LiveIntervalExtensionAnalysis *LIEA_;
    ReplayCacheRegionAnalysis *RRA_;
    LiveIntervals *LIS_;
    SlotIndexes *SIS_;
public:
    static char ID;

    RegisterPressureAwareRegionPartitioning();

    void getAnalysisUsage(AnalysisUsage &AU) const override;
    void releaseMemory() override;
    bool runOnMachineFunction(MachineFunction &MF) override;
};
