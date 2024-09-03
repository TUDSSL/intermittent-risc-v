/**
 * Register pressure-aware region partitioning
 * 
 * For every definition, checks if the register pressure (including extensions)
 * is above a certain threshold. If it is, a new region boundary is inserted.
 * 
 * Runs after initial regions and before register allocation.
 */
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
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_END(RegisterPressureAwareRegionPartitioning, DEBUG_TYPE,
                    PASS_NAME, false, false)

RegisterPressureAwareRegionPartitioning::RegisterPressureAwareRegionPartitioning() : MachineFunctionPass(ID)
{}

void RegisterPressureAwareRegionPartitioning::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<ReplayCacheRegionAnalysis>();
    AU.addRequired<LiveIntervals>();
    AU.addRequired<SlotIndexes>();
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
}

/**
 * Inserts region boundaries when the number of available registers isn't enough to
 * satisfy current register pressure.
 * If an instruction contains a definition, the live interval for this definition is
 * added to the vector.
 * Then the number of live intervals is checked; if it exceeds the register threshold,
 * a region boundary is added and the live interval extensions are recomputed.
 */
bool RegisterPressureAwareRegionPartitioning::runOnMachineFunction(MachineFunction &MF)
{
    static constexpr unsigned NUM_INTERVAL_THRESHOLD = 26;

    RRA_ = &getAnalysis<ReplayCacheRegionAnalysis>();
    LIS_ = &getAnalysis<LiveIntervals>();
    SIS_ = &getAnalysis<SlotIndexes>();

    unsigned NumLiveIntervals = 0;
    std::vector<LiveInterval *> LiveIntervalVector;

    /* Compute initial live interval extensions. */
    LIS_->computeExtensions(MF, RRA_);

    // We can use regions here because ALL live intervals in the current MachineFunction
    // are added to the vector and aren't remove until done.
    for (auto &Region : *RRA_)
    {
        for (auto Instr = Region.begin(); Instr != Region.end(); Instr++)
        {
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
                    }
                }
            }
            
            /* Compute number of live intervals, INCLUDING extensions! */
            NumLiveIntervals = 0;
            auto InstrIndex = LIS_->getInstructionIndex(*Instr);
            for (auto *LI : LiveIntervalVector)
            {
                if (LI->liveAt(InstrIndex))
                {
                    NumLiveIntervals++;
                }
            }

            /* Compute number of extensions that are live. */
            unsigned ExtensionPressure = LIS_->getExtensionPressureAt(*Instr);

            /* We only insert a region when there is at least one live interval with an
             * extension, otherwise the new region would be useless.
             */
            if (ExtensionPressure > 0 && NumLiveIntervals > NUM_INTERVAL_THRESHOLD)
            {
                /* Flag instruction to insert new region boundary instruction later. */
                RRA_->createRegionBefore(&Region, Instr.getMBBIt(), Instr.getInstrIt(), SIS_);
                /* Trim active extensions to terminate at new region boundary. */
                LIS_->recomputeActiveExtensions(LIS_->getInstructionIndex(*Instr));
            }
        }
    }

    return true;
}

FunctionPass *llvm::createRegisterPressureAwareRegionPartitioningPass() {
    return new RegisterPressureAwareRegionPartitioning();
}
