#include "RegisterPressureAwareRegionPartitioning.h"
#include "llvm/InitializePasses.h"
#include "llvm/CodeGen/RegisterPressure.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "RISCV.h"
#include <algorithm>

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_RegisterPressureAwareRegionPartitioning"

char RegisterPressureAwareRegionPartitioning::ID = 5;

raw_ostream &output_rparp = llvm::outs();

INITIALIZE_PASS_BEGIN(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE, PASS_NAME,
                      false, false)
// INITIALIZE_PASS_DEPENDENCY(LiveIntervalExtensionAnalysis)
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_END(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE,
                    PASS_NAME, false, false)

RegisterPressureAwareRegionPartitioning::RegisterPressureAwareRegionPartitioning() : MachineFunctionPass(ID)
{}

void RegisterPressureAwareRegionPartitioning::getAnalysisUsage(AnalysisUsage &AU) const
{
    // AU.addRequiredTransitive<LiveIntervalExtensionAnalysis>();
    AU.addRequired<ReplayCacheRegionAnalysis>();
    AU.addRequired<LiveIntervals>();
    AU.addRequired<SlotIndexes>();
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
}

bool RegisterPressureAwareRegionPartitioning::runOnMachineFunction(MachineFunction &MF)
{
    // output_rparp << "START REGION PARTITIONING\n";

    static constexpr unsigned NUM_INTERVAL_THRESHOLD = 26;

    RRA_ = &getAnalysis<ReplayCacheRegionAnalysis>();
    LIS_ = &getAnalysis<LiveIntervals>();
    SIS_ = &getAnalysis<SlotIndexes>();

    MachineBasicBlock *PrevMBB;
    unsigned NumLiveIntervals = 0;
    std::vector<LiveInterval *> LiveIntervalVector;
    // MachineRegisterInfo &MRI = MF.getRegInfo();

    LIS_->computeExtensions(RRA_);

    for (auto &Region : *RRA_)
    {
        for (auto Instr = Region.begin(); Instr != Region.end(); Instr++)
        {
            auto MBB = Instr.getMBB();

            if (IsRC(*Instr) || Instr->isDebugInstr())
            {
                continue;
            }

            /* If there are no register definitions in this instruction, it has no effect on the distribution
             * of regions so we ignore it.
             */
            if (Instr->getNumDefs() == 0)
            {
                continue;
            }

            /* Add all intervals for register definitions to the vector. */
            for (auto &Def : Instr->all_defs())
            {
                auto Reg = Def.getReg();

                /* Check if this register has an associated interval. */
                if (Reg.isVirtual() && LIS_->hasInterval(Reg))
                {
                    auto &LI = LIS_->getInterval(Reg);

                    /* Add this interval to the vector if it doesn't already exist. */
                    if (std::find(LiveIntervalVector.begin(), LiveIntervalVector.end(), &LI) == LiveIntervalVector.end())
                    {
                        LiveIntervalVector.push_back(&LI);
                        // output_rparp << *Instr << "\n";
                        // output_rparp.flush();
                    }
                }
            }
            
            /* Compute number of live intervals. */
            NumLiveIntervals = 0;
            for (auto *LI : LiveIntervalVector)
            {
                if (LI->liveAt(LIS_->getInstructionIndex(*Instr)))
                {
                    NumLiveIntervals++;
                }
            }

            unsigned ExtensionPressure = LIS_->getExtensionPressureAt(*Instr);
            // output_rparp << "EXT_PRES: " << ExtensionPressure << "\n";

            if (ExtensionPressure > 0 && NumLiveIntervals > NUM_INTERVAL_THRESHOLD)
            {
                /* Insert new region. */
                RRA_->createRegionBefore(&Region, Instr.getMBBIt(), Instr.getInstrIt(), SIS_);
                /* Trim active extensions to terminate at new region boundary. */
                LIS_->recomputeActiveExtensions(LIS_->getInstructionIndex(*Instr));
                /* Stop iterating over the current region. */
                goto goto_continue_to_next_region;
            }

            PrevMBB = MBB;
        }

        goto_continue_to_next_region:;
    }

    /* Enable extension of live intervals. */
    // MRI.setExtendsLiveIntervals();

    return true;
}


FunctionPass *llvm::createRegisterPressureAwareRegionPartitioningPass() {
    return new RegisterPressureAwareRegionPartitioning();
}

// INITIALIZE_PASS(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE, PASS_NAME, false, true)

