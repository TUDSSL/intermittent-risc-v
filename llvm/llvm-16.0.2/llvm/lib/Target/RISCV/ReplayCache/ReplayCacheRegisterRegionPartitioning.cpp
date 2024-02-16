#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheRegisterRegionPartitioning.h"
#include "llvm/CodeGen/MachineOperand.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_RegisterRegionPartitioning"

char ReplayCacheRegisterRegionPartitioning::ID = 1;

raw_ostream &output = llvm::outs();

ReplayCacheRegisterRegionPartitioning::ReplayCacheRegisterRegionPartitioning() : MachineFunctionPass(ID) {}

void ReplayCacheRegisterRegionPartitioning::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<LiveIntervals>();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheRegisterRegionPartitioning::runOnMachineFunction(MachineFunction &MF) {

    MachineInstr *PrevInstr = nullptr;
    LIS = &getAnalysis<LiveIntervals>();

    // TODO: print LIS contents for debugging
    // LIS->print(output);

    // TODO: keep map LI --> (bool, [newIndex0, newIndex1, ...]) + total overlapping live intervals
    // Indices are end of every basic block along the path, plus the last index (end of generated or existing region)
    // TODO: after traversing all, extend all LI in the map to new indices

    // for (auto &MBB : MF) {
    //     for (auto &MI : MBB) {
    //         PrevInstr = &MI;

    //         /* Check if we have hit a region boundary. If so, we do not extend the live intervals past this boundary. */
    //         if (IsRC(MI))
    //         {
    //             ReplayCacheInstruction instr = static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm());
    //             if (instr == ReplayCacheInstruction::FENCE)
    //             {
    //                 deactivateLiveIntervals(MI);
    //             }
    //         }
    //         /* Only extend live interval of registers used in store instructions. */
    //         else if (isStoreInstruction(MI))
    //         {
    //             auto MO = MI.getOperand(0);
    //             Register Reg = MO.getReg();
    //             LiveInterval &LI = LIS->getInterval(Reg);
    //             LiveInterval *LI_ptr = &LI;

    //             if (IntervalsMap.count(LI_ptr) > 0)
    //             {
    //                 /* Put a region boundary if we exceed the number of allocated registers. */
    //                 if (IntervalsMap.size() >= NUM_STORE_REGISTERS)
    //                 {
    //                     /* Insert a region boundary before the current instruction. */
    //                     BuildRC(MBB, MI, MI.getDebugLoc(), FENCE);
    //                     BuildRC(MBB, MI, MI.getDebugLoc(), START_REGION);

    //                     /* Deactivate all current intervals. */
    //                     deactivateLiveIntervals(MI);
    //                 }
                    
    //                 IntervalsMap[LI_ptr] = std::make_pair(true, std::vector<SlotIndex>{});
    //             }
    //         }
    //     }

    //     /* Add MBB end index to active live interval extension map entries. */
    //     if (PrevInstr != nullptr)
    //     {
    //         extendLiveIntervalsToInstr(*PrevInstr);
    //         PrevInstr = nullptr;
    //     }
    // }

    // // TODO: extend live intervals
    // // LIS->extendToIndices(LI, NewIndices);
    // // LI.verify();

    // printIntervalsMapContents();

    return false;
}

void ReplayCacheRegisterRegionPartitioning::deactivateLiveIntervals(MachineInstr &MI)
{                            
    /* Append current index to all active entries and deactivate them. */
    for (auto &[key, value] : IntervalsMap)
    {
        if (value.first)
        {
            value.second.push_back(LIS->getInstructionIndex(MI).getRegSlot(MI.getOperand(0).isEarlyClobber()));
            value.first = false;
        }
    }
}

void ReplayCacheRegisterRegionPartitioning::extendLiveIntervalsToInstr(MachineInstr &MI)
{
    for (auto &[key, value] : IntervalsMap)
    {
        if (value.first)
        {
            value.second.push_back(LIS->getInstructionIndex(MI).getRegSlot(MI.getOperand(0).isEarlyClobber()));
        }
    }
}

void ReplayCacheRegisterRegionPartitioning::printIntervalsMapContents()
{
    output << "----------- INTERVAL MAP --------------\n";
    for (auto &[key, value] : IntervalsMap)
    {
        key->print(output);
        output << "\n";
    }
}

FunctionPass *llvm::createReplayCacheRegisterRegionPartitioningPass() {
    return new ReplayCacheRegisterRegionPartitioning();
}

INITIALIZE_PASS(ReplayCacheRegisterRegionPartitioning, DEBUG_TYPE, PASS_NAME, false, false)
