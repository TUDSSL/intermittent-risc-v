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

}