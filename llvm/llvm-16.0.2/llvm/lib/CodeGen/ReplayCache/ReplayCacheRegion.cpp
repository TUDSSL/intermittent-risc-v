#include "llvm/CodeGen/ReplayCache/ReplayCacheRegion.h"

namespace llvm
{

ReplayCacheRegion::ReplayCacheRegion(unsigned ID, RegionInstr StartRegionInstr, RegionBlock StartRegionBlock) :
    Next_(nullptr),
    Prev_(nullptr),
    MF_(StartRegionBlock->getParent()),
    InstrStart_(StartRegionInstr),
    InstrEnd_((--(MF_->end()))->end()),
    BlockStart_(StartRegionBlock),
    BlockEnd_(MF_->end()),
    ID_(ID)
{
    for (auto BlockIt = BlockStart_; BlockIt != BlockEnd_; BlockIt++)
    {
        if (InstrEnd_ == BlockIt->end())
        {
            InstrEndIsSentinel_ = true;
        }
    }
}

void ReplayCacheRegion::terminateAt(RegionInstr FenceInstr, RegionBlock FenceBlock)
{
    InstrEnd_ = FenceInstr;
    BlockEnd_ = FenceBlock;
}

/* Check if this region contains the given instruction. */
bool ReplayCacheRegion::containsInstr(MachineInstr &MI)
{
    for (auto &MI_Region : *this)
    {
        if (&MI_Region == &MI)
        {
            return true;
        }
        
    }

    return false;
}

bool ReplayCacheRegion::hasMBB(MachineBasicBlock &MBB)
{
    for (auto BlockIt = BlockStart_; BlockIt != BlockEnd_; BlockIt++)
    {
        if (&*BlockIt == &MBB)
        {
            return true;
        }
    }

    return false;
}

/* Check if this region points to a next region in the linked list.*/
bool ReplayCacheRegion::hasNext()
{
    return this->Next_ != nullptr;
}
/* Check if this region points to a previous region in the linked list.*/
bool ReplayCacheRegion::hasPrev()
{
    return this->Prev_ != nullptr;
}

/* Set the next pointer of this region. */
void ReplayCacheRegion::setNext(ReplayCacheRegion *Region)
{
    this->Next_ = Region;
}
/* Set the previous pointer of this region.*/
void ReplayCacheRegion::setPrev(ReplayCacheRegion *Region)
{
    this->Prev_ = Region;
}

/* Get the next pointer in this region. */
ReplayCacheRegion *ReplayCacheRegion::getNext()
{
    return this->Next_;
}
/* Get the prev pointer in this region. */
ReplayCacheRegion *ReplayCacheRegion::getPrev()
{
    return this->Prev_;
}


std::pair<SlotInterval, bool> ReplayCacheRegion::getSlotIntervalInMBB(const SlotIndexes &SLIS, MachineBasicBlock *MBB, MachineInstr *MIStart) const
{
    SlotInterval SI;
    bool foundFence = false;

    /* Return invalid SlotIndices if MBB is empty.*/
    if (MBB->empty())
    {
        return std::make_pair(SI, foundFence);
    }

    if (MIStart == nullptr)
    {
        SI.first = SLIS.getMBBStartIdx(MBB);
    }
    else
    {
        /* Region contains custom start instruction. */
        assert(MIStart->getParent() == MBB);
        SI.first = SLIS.getInstructionIndex(*MIStart).getRegSlot();
    }

    /* If the final instruction is in this MBB, that should be the end of the range. */
    if (!InstrEndIsSentinel_ && InstrEnd_->getParent() == MBB)
    {
        /* Get non-debug final instr (using debug instr crashes compiler). */
        RegionInstr RealInstrFinal = InstrEnd_;
        while (RealInstrFinal != MBB->begin() && RealInstrFinal->isDebugInstr())
        {
            RealInstrFinal--;
        }

        if (RealInstrFinal == MBB->begin())
        {
            /* Invalid slot interval because start == end. */
            return std::make_pair(SI, foundFence);
        }
        else {
            SI.last = SLIS.getInstructionIndex(*RealInstrFinal).getRegSlot();
        }

        foundFence = true;
    }
    else
    {
        SI.last = SLIS.getMBBEndIdx(MBB);
    }

    assert(SI.first <= SI.last && "Inverse interval!");
    
    return std::make_pair(SI, foundFence);
}

}