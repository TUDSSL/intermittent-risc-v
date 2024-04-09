#pragma once

#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include <vector>

namespace llvm
{

struct SlotInterval
{
    SlotIndex first;
    SlotIndex last;

    unsigned getSize() { return first.distance(last); }
}

class ExtendedLiveInterval
{
public:
    std::vector<SlotInterval> SITS_;
    LiveInterval &LI_;

public:
    ExtendedLiveInterval(LiveInterval &LI);

    LiveInterval &getInterval();

    void addSlotInterval(SlotInterval SIT);
    void clearSlotIntervals();

    unsigned getSize() const;
};

} // namespace llvm
