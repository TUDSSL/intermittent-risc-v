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
}

void ReplayCacheRegion::terminateAt(RegionInstr FenceInstr, RegionBlock FenceBlock)
{
    InstrEnd_ = FenceInstr;
    BlockEnd_ = FenceBlock;
}

/* Check if this region contains the given instruction. */
bool ReplayCacheRegion::containsInstr(MachineInstr MI)
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

}