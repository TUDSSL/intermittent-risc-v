#include "LiveIntervalExtensionAnalysis.h"
#include "../ReplayCacheShared.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define PASS_NAME "ReplayCache_LiveIntervalExtensionAnalysis"

char LiveIntervalExtensionAnalysis::ID = 5;

raw_ostream &output_llex = llvm::outs();

INITIALIZE_PASS_BEGIN(LiveIntervalExtensionAnalysis, DEBUG_TYPE, PASS_NAME,
                      false, true)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
INITIALIZE_PASS_END(LiveIntervalExtensionAnalysis, DEBUG_TYPE,
                    PASS_NAME, false, true)

LiveIntervalExtensionAnalysis::LiveIntervalExtensionAnalysis() : MachineFunctionPass(ID)
{}

void LiveIntervalExtensionAnalysis::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequiredTransitive<LiveIntervals>();
    AU.addRequiredTransitive<SlotIndexes>();
    AU.addRequiredTransitive<ReplayCacheRegionAnalysis>();
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
}

void LiveIntervalExtensionAnalysis::releaseMemory()
{
    ExtensionAllocator_.Reset();
    ExtensionMap_.clear();
}

bool LiveIntervalExtensionAnalysis::runOnMachineFunction(MachineFunction &MF)
{
    LIS_ = &getAnalysis<LiveIntervals>();
    SLIS_ = &getAnalysis<SlotIndexes>();
    RRA_ = &getAnalysis<ReplayCacheRegionAnalysis>();

    for (auto &Region : *RRA_)
    {
        for (auto &MI : Region)
        {
            CurrentRegion_ = &Region;

            if (isStoreInstruction(MI))
            {
                auto Reg = MI.getOperand(0).getReg();
                LiveInterval &LI = LIS_->getInterval(Reg);

                computeExtensionFromLI(MI, LI);
            }
        }
    }
    
    return true;
}

void LiveIntervalExtensionAnalysis::computeExtensionFromLI(MachineInstr &MI, LiveInterval &LI)
{
    /* Extension already in map, skip. */
    if (ExtensionMap_.find(LI.reg()) != ExtensionMap_.end())
    {
        return;
    }
    
    /* Create the extension. */
    auto *Extension = new (ExtensionAllocator_) ExtendedLiveInterval(LI);

    /* Find the slot intervals of all basic blocks until the next region boundary, starting from MI (a store instruction). */
    auto SlotIntervals = getSlotIntervalsInRegionFrom(MI);

    /* Remove slot intervals that overlap with ranges in LI. We do this so the extension and LI are disjoint from one another.
     * This is required to get the correct values for register allocation. */
    auto NonOverlappingSlotIntervals = removeOverlappingIntervals(LI, SlotIntervals);

    /* Add all slot intervals to the extension and add the extension to the extension map. */
    Extension->setSlotIntervals(NonOverlappingSlotIntervals);
    ExtensionMap_.insert(std::pair(LI.reg(), Extension));
}

std::vector<SlotInterval> LiveIntervalExtensionAnalysis::getSlotIntervalsInRegionFrom(MachineInstr &MI)
{
    std::vector<SlotInterval> SlotIntervals;
    MachineBasicBlock *PrevMBB = nullptr;
    bool MIFound = false;

    for (auto Instr = CurrentRegion_->begin(); Instr != CurrentRegion_->end(); Instr++)
    {
        auto MBB = Instr.getMBB();

        if (!MIFound)
        {
            if (&*Instr == &MI)
            {
                SlotIntervals.push_back(Instr.getSlotIntervalInCurrentMBB(*SLIS_, &MI));

                MIFound = true;
            }
        }
        else
        {
            if (MBB != PrevMBB)
            {
                /* Entered a new basic block, add the slot intervals. */
                auto SlotInterval = Instr.getSlotIntervalInCurrentMBB(*SLIS_);
                if (SlotInterval.first.isValid() && SlotInterval.last.isValid())
                {
                    SlotIntervals.push_back(SlotInterval);
                }   
            }
        }

        PrevMBB = MBB;
    }

    return SlotIntervals;
}

std::vector<SlotInterval> LiveIntervalExtensionAnalysis::removeOverlappingIntervals(LiveInterval &LI, std::vector<SlotInterval> &SlotIntervals)
{
    std::vector<SlotInterval> NonOverlappingIntervals;

    /* For each slot interval in the array, we need to check if this range is (partially) overlapped by the ranges in LI. */

    for (auto &SI : SlotIntervals)
    {
        /* If there is no overlap, add the slot interval. */
        if (!LI.overlaps(SI.first, SI.last))
        {
            NonOverlappingIntervals.push_back(SI);
            continue;
        }
        
        /* There is overlap, we need to know exactly where. Find the segment that contains the first slot index and iterate from there. */
        auto Seg = LI.find(SI.first);

        /* Increment segment until we reach the end of the LI, or until we find the last slot index in the interval. */
        while (Seg != LI.end() && !Seg->contains(SI.last))
        {
            ++Seg;
        }

        if (Seg != LI.end())
        {
            /* Complete overlap, ignore slot interval. */
            continue;
        }
        else
        {
            /* Partial overlap, add trimmed slot interval. */
            NonOverlappingIntervals.push_back({ .first = LI.endIndex(), .last = SI.last });
        }
    }

    return NonOverlappingIntervals;  
}

ExtendedLiveInterval &LiveIntervalExtensionAnalysis::getExtensionFromLI(LiveInterval &LI)
{

}

void LiveIntervalExtensionAnalysis::recomputeActiveExtensions(SlotIndex SI)
{
    for (const auto &[reg, ex] : ExtensionMap_)
    {
        if (ex->isLiveAt(SI))
        {
            auto &lastSlotInterval = ex->SITS_.back();
            lastSlotInterval.last = SI;
        }
    }
}

unsigned LiveIntervalExtensionAnalysis::getExtensionPressureAt(MachineInstr &MI)
{
    auto SI = SLIS_->getInstructionIndex(MI).getRegSlot();
    unsigned Pressure = 0;

    for (const auto &[reg, ex] : ExtensionMap_)
    {
        if (ex->isLiveAt(SI))
        {
            Pressure++;
        }
    }
    
    return Pressure;
}

// FunctionPass *llvm::createLiveIntervalExtensionAnalysisPass() {
//     return new LiveIntervalExtensionAnalysis();
// }

// INITIALIZE_PASS(LiveIntervalExtensionAnalysis, DEBUG_TYPE, PASS_NAME, false, true)

