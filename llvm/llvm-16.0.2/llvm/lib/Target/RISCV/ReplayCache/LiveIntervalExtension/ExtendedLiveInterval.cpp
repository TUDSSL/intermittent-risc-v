#include "ExtendedLiveInterval.h"

namespace llvm
{

ExtendedLiveInterval::ExtendedLiveInterval(LiveInterval &LI) : LI_(LI)
{}

LiveInterval &ExtendedLiveInterval::getInterval()
{
    return LI_;
}

void ExtendedLiveInterval::addSlotInterval(SlotInterval SIT)
{
    SITS_.push_back(SIT);
}

void ExtendedLiveInterval::setSlotIntervals(std::vector<SlotInterval> &SITS)
{
    SITS_ = SITS;
}

void ExtendedLiveInterval::clearSlotIntervals()
{
    SITS_.clear();
}

unsigned ExtendedLiveInterval::getSize() const
{
    unsigned Size = 0;

    for (auto &SIT : SITS_)
    {
        Size += SIT.getSize();
    }
    
    return Size;
}

bool ExtendedLiveInterval::isLiveAt(SlotIndex &SI)
{
    for (auto &SIT : SITS_)
    {
        if (SIT.contains(SI))
        {
            return true;
        }
    }

    return false;
}

}