#include "llvm/CodeGen/ReplayCache/LiveIntervalExtensionAnalysis.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "llvm/InitializePasses.h"

using namespace llvm;


#define DEBUG_TYPE "replaycache"
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

                // output_llex << "Store instr: " << MI;
                // output_llex << "Operand:     " << MI.getOperand(0) << "\n";
                // output_llex << "LI:          " << LI << "\n";
                // output_llex.flush();

                computeExtensionFromLI(MI, LI);
            }
        }
    }

    addExtensionsToIntervals();

    // output_llex << "Added " << ExtensionMap_.size() << " extensions.\n";
    // for (const auto &[reg, ex] : ExtensionMap_)
    // {
    //     output_llex << "LI:          " << ex->getInterval() << "\n";
    // }
    // output_llex.flush(); 
    
    return true;
}

void LiveIntervalExtensionAnalysis::computeExtensionFromLI(MachineInstr &MI, LiveInterval &LI)
{
    /* Extension already in map, skip. */
    if (ExtensionMap_.find(LI.reg()) != ExtensionMap_.end())
    {
        output_llex << "Extension already exists\n";
        output_llex.flush();
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

ExtendedLiveInterval *LiveIntervalExtensionAnalysis::getExtensionFromLI(const LiveInterval &LI)
{
    auto it = ExtensionMap_.find(LI.reg());

    if (it == ExtensionMap_.end())
    {
        return nullptr;
    }

    return it->second;
}

void LiveIntervalExtensionAnalysis::recomputeActiveExtensions(SlotIndex FinalIndex)
{
    LiveRangeUpdater LRU;

    for (const auto &[reg, ex] : ExtensionMap_)
    {
        if (ex->isLiveAt(FinalIndex))
        {
            LiveRange &LR = ex->getInterval();

            /* Remove all extension intervals from live range. */
            for (auto &SI : ex->SITS_)
            {
                LR.removeSegment(SI.first, SI.last, true);
            }

            /* Trim extension interval to stop at the FinalIndex. */
            bool found = false;

            // for (auto &SlotInterval : ex->SITS_)
            for (auto it = ex->SITS_.begin(); it != ex->SITS_.end(); it++)
            {
                if (found)
                {
                    ex->SITS_.erase(it);
                }

                if (it->contains(FinalIndex))
                {
                    it->last = FinalIndex;
                    found = true;
                }
            }

            /* Re-add extension to live range. */
            addExtensionToInterval(LRU, ex);
        }
    }

    LRU.flush();
}

unsigned LiveIntervalExtensionAnalysis::getExtensionPressureAt(MachineInstr &MI)
{
    auto SI = SLIS_->getInstructionIndex(MI).getRegSlot();
    unsigned Pressure = 0;

    for (const auto &[reg, ex] : ExtensionMap_)
    {
        // output_llex << "First, last: ";
        // ex->SITS_.front().print(output_llex);
        // ex->SITS_.back().print(output_llex);
        // output_llex << "\n";
        // output_llex.flush();
        if (ex->isLiveAt(SI))
        {
            Pressure++;
        }
    }
    
    return Pressure;
}

void LiveIntervalExtensionAnalysis::addExtensionsToIntervals()
{
    LiveRangeUpdater LRU;
    for (const auto &[reg, ex] : ExtensionMap_)
    {
        addExtensionToInterval(LRU, ex);
    }
    
    LRU.flush();
}

void LiveIntervalExtensionAnalysis::addExtensionToInterval(LiveRangeUpdater &LRU, ExtendedLiveInterval *ELR)
{
    LiveRange &LR = ELR->getInterval();
    LRU.setDest(&LR);

    for (auto &Seg : ELR->SITS_)
    {
        if (Seg.first < Seg.last)
        {
            VNInfo *VN = LR.getNextValue(Seg.first, LIS_->getVNInfoAllocator());
            LRU.add(Seg.first, Seg.last, VN);
        }
    }
}

// FunctionPass *llvm::createLiveIntervalExtensionAnalysisPass() {
//     return new LiveIntervalExtensionAnalysis();
// }

// INITIALIZE_PASS(LiveIntervalExtensionAnalysis, DEBUG_TYPE, PASS_NAME, false, true)

