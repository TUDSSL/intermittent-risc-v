#pragma once

#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include "llvm/Support/raw_ostream.h"
#include "ReplayCacheShared.h"
#include <vector>

namespace llvm
{

class ExtendedLiveInterval
{
public:
    std::vector<SlotInterval> SITS_;
    LiveInterval &LI_;

public:
    ExtendedLiveInterval(LiveInterval &LI);

    LiveInterval &getInterval();

    void addSlotInterval(SlotInterval SIT);
    void setSlotIntervals(std::vector<SlotInterval> &SITS);
    void clearSlotIntervals();

    bool isLiveAt(SlotIndex &SI);

    unsigned getSize() const;
    void print(raw_ostream &stream);
};

} // namespace llvm
