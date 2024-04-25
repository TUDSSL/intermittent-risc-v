#include "RegisterPressureAwareRegionPartitioning.h"
#include "llvm/InitializePasses.h"
#include "llvm/CodeGen/RegisterPressure.h"
#include <algorithm>

using namespace llvm;

#define PASS_NAME "ReplayCache_RegisterPressureAwareRegionPartitioning"

char RegisterPressureAwareRegionPartitioning::ID = 5;

raw_ostream &output_rparp = llvm::outs();

INITIALIZE_PASS_BEGIN(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE, PASS_NAME,
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LiveIntervalExtensionAnalysis)
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_END(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE,
                    PASS_NAME, false, false)

RegisterPressureAwareRegionPartitioning::RegisterPressureAwareRegionPartitioning() : MachineFunctionPass(ID)
{}

void RegisterPressureAwareRegionPartitioning::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequiredTransitive<LiveIntervalExtensionAnalysis>();
    AU.addRequiredTransitive<ReplayCacheRegionAnalysis>();
    AU.addRequired<LiveIntervals>();
    AU.addRequired<SlotIndexes>();
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
}

void RegisterPressureAwareRegionPartitioning::releaseMemory()
{
    
}

bool RegisterPressureAwareRegionPartitioning::runOnMachineFunction(MachineFunction &MF)
{
    static constexpr unsigned NUM_INTERVAL_THRESHOLD = 31;

    LIEA_ = &getAnalysis<LiveIntervalExtensionAnalysis>();
    RRA_ = &getAnalysis<ReplayCacheRegionAnalysis>();
    LIS_ = &getAnalysis<LiveIntervals>();
    SIS_ = &getAnalysis<SlotIndexes>();

    MachineBasicBlock *PrevMBB;
    IntervalPressure RP;
    RegPressureTracker RPT(RP);

    unsigned NumLiveIntervals = 0;
    unsigned NumLiveIns = 0;

    std::vector<LiveInterval *> LiveIntervalVector;

    for (auto &Region : *RRA_)
    {
        for (auto Instr = Region.begin(); Instr != Region.end(); Instr++)
        {
            // TODO:
            // 1. Ignore debug instrs.
            // 2. Get #live-ins of MBB if MBB changed.
            // 3. For each instr, loop over defs. Add each deffed register live range to a list. If there are no defs, continue.
            // 4. For each entry in the list, check if it is live at this instr. If it is not, remove from list.
            // 5. Calculate extension pressure.
            // 6. Add length of list + extension pressure to get total pressure.
            // 7. If total pressure > threshold, insert region.
            
            auto MBB = Instr.getMBB();

            if (IsRC(*Instr))
            {
                continue;
            }

            /* If there are no register definitions in this instruction, it has no effect on the distribution
             * of regions so we ignore it.
             */
            if (Instr->getNumExplicitDefs() == 0)
            {
                continue;
            }

            // output_rparp << "Instr: ";
            // Instr->print(output_rparp);
            // output_rparp.flush();

            /* Add all intervals for register definitions to the vector. */
            for (auto &Def : Instr->defs())
            {
                auto Reg = Def.getReg();

                // output_rparp << "REG: " << Reg << "\n";
                // output_rparp.flush();

                /* Check if this register has an associated interval. */
                if (!Reg.isPhysicalRegister(Reg) && LIS_->hasInterval(Reg))
                {
                    // output_rparp << "HasInterval!\n";
                    // output_rparp.flush();

                    auto &LI = LIS_->getInterval(Reg);

                    /* Add this interval to the vector if it doesn't already exist. */
                    if (std::find(LiveIntervalVector.begin(), LiveIntervalVector.end(), &LI) == LiveIntervalVector.end())
                    {
                        LiveIntervalVector.push_back(&LI);
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

            unsigned ExtensionPressure = LIEA_->getExtensionPressureAt(*Instr);
            output_rparp << "Extensions: " << ExtensionPressure << "\n";
            output_rparp.flush();

            if (ExtensionPressure > 0 && NumLiveIntervals + ExtensionPressure > NUM_INTERVAL_THRESHOLD)
            {
                output_rparp << NumLiveIntervals + ExtensionPressure << " - Threshold exceeded, inserting region...\n";
                output_rparp.flush();
                /* Insert new region. */
                RRA_->createRegionBefore(&Region, Instr.getMBBIt(), Instr.getInstrIt(), SIS_);
                /* Trim active extensions to terminate at new region boundary. */
                LIEA_->recomputeActiveExtensions(LIS_->getInstructionIndex(*Instr));
                /* Stop iterating over the current region. */
                // Instr.stop();
                output_rparp << "Inserted region.\n";
                output_rparp.flush();
                goto goto_continue_to_next_region;
            }

            // output_rparp << "----------------------\n";
            // output_rparp << "NumLive: " << NumLiveIntervals << "\n";
            // output_rparp << "NumEXt:  " << ExtensionPressure << "\n";
            // output_rparp << "Total:   " << NumLiveIntervals + ExtensionPressure << "\n";
            // output_rparp.flush();

            PrevMBB = MBB;
        }
        goto_continue_to_next_region:;
    }
    


    // Loop over all defs in the region
    // For each def, check register pressure + extension pressure
    // If > threshold, insert region before.

    return false;
}


FunctionPass *llvm::createRegisterPressureAwareRegionPartitioningPass() {
    return new RegisterPressureAwareRegionPartitioning();
}

// INITIALIZE_PASS(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE, PASS_NAME, false, true)

