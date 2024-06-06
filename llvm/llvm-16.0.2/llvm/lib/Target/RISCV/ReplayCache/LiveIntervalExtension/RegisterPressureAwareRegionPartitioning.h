#pragma once

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/LiveIntervals.h"

class RegisterPressureAwareRegionPartitioning : public MachineFunctionPass
{
private:
    ReplayCacheRegionAnalysis *RRA_;
    LiveIntervals *LIS_;
    SlotIndexes *SIS_;
public:
    static char ID;

    RegisterPressureAwareRegionPartitioning();

    void getAnalysisUsage(AnalysisUsage &AU) const override;
    bool runOnMachineFunction(MachineFunction &MF) override;
};
