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
    // assert(IsStartRegion(*InstrStart_) && "Start instruction is not START_REGION");

    // InstrStart_++;
}

void ReplayCacheRegion::terminateAt(RegionInstr FenceInstr, RegionBlock FenceBlock)
{
    // assert(IsFence(*FenceInstr) && "End instruction is not FENCE");

    InstrEnd_ = FenceInstr;
    BlockEnd_ = FenceBlock;
}

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

bool ReplayCacheRegion::hasNext()
{
    return this->Next_ != nullptr;
}
bool ReplayCacheRegion::hasPrev()
{
    return this->Prev_ != nullptr;
}

void ReplayCacheRegion::setNext(ReplayCacheRegion *Region)
{
    this->Next_ = Region;
}
void ReplayCacheRegion::setPrev(ReplayCacheRegion *Region)
{
    this->Prev_ = Region;
}

ReplayCacheRegion *ReplayCacheRegion::getNext()
{
    return this->Next_;
}
ReplayCacheRegion *ReplayCacheRegion::getPrev()
{
    return this->Prev_;
}

}