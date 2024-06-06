#include "llvm/CodeGen/ReplayCache/ExtendedLiveInterval.h"

namespace llvm
{

ExtendedLiveInterval::ExtendedLiveInterval(LiveInterval &LI) : LI_(LI)
{}

/* Get the live interval belonging to the extension. */
LiveInterval &ExtendedLiveInterval::getInterval()
{
    return LI_;
}

/* Add a slot interval to the vector. */
void ExtendedLiveInterval::addSlotInterval(SlotInterval SIT)
{
    SITS_.push_back(SIT);
}

/* Replace the slot interval vector with a new one. */
void ExtendedLiveInterval::setSlotIntervals(std::vector<SlotInterval> &SITS)
{
    SITS_ = SITS;
}

/* Clear the slot interval vector. */
void ExtendedLiveInterval::clearSlotIntervals()
{
    SITS_.clear();
}

/* Get the size of the extension. This measures the number of slots
 * from the first slot to the last in the interval vector.
 */
unsigned ExtendedLiveInterval::getSize() const
{
    unsigned Size = 0;

    for (auto &SIT : SITS_)
    {
        Size += SIT.getSize();
    }
    
    return Size;
}

/* Check if the extension is live at the given slot index.*/
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

void ExtendedLiveInterval::print(raw_ostream &stream)
{
    for (auto &SIT : SITS_)
    {
        stream << "[" << SIT.first << ", " << SIT.last << ")";
    }
    stream << "\n";
}

}