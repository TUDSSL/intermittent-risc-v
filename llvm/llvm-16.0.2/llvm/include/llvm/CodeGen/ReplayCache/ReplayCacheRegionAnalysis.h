#pragma once

#include "ReplayCacheRegion.h"
#include "llvm/CodeGen/SlotIndexes.h"

namespace llvm
{

class ReplayCacheRegionAnalysis : public MachineFunctionPass
{
public:
    static char ID;
    
    unsigned int RegionsSize;
    ReplayCacheRegion *Head;
    ReplayCacheRegion *Tail;

    ReplayCacheRegionAnalysis() : MachineFunctionPass(ID), RegionsSize(0), Head(nullptr), Tail(nullptr) {}

    ReplayCacheRegion &createRegionBefore(ReplayCacheRegion* Region, ReplayCacheRegion::RegionBlock MBB, ReplayCacheRegion::RegionInstr MI, SlotIndexes *SLIS);

    /* Iterator for iterating over regions. */
    template <typename ReplayCacheRegionTy>
    class rcra_rr_iterator : public std::iterator<std::forward_iterator_tag, ReplayCacheRegionTy *> {
    public:
        // Constructors.
        explicit rcra_rr_iterator(const ReplayCacheRegionAnalysis &RR, const bool isEnd = false)    :
            RR_(&RR), 
            Valid_(!isEnd),
            Start_(RR.Head),
            It_(isEnd ? nullptr : RR.Head)
        {
        }

        // Return whether the iterator is valid.  False implies the end condition
        // has been met.
        bool isValid() { return It_ != nullptr; };

        // Comparison.
        bool operator==(const rcra_rr_iterator &X) const { return It_ == X.It_; };
        bool operator!=(const rcra_rr_iterator &X) const { return !operator==(X); };

        // Pre-increment and post-increment.
        rcra_rr_iterator<ReplayCacheRegionTy> &operator++() { It_ = It_->getNext(); Valid_ = It_ != nullptr; return *this; };
        rcra_rr_iterator<ReplayCacheRegionTy> operator++(int) { rcra_rr_iterator Tmp = *this; ++*this; return Tmp; };

        // Dereference.
        ReplayCacheRegionTy &operator*() const { assert(It_ != nullptr); return *It_; };

    private:
        const ReplayCacheRegionAnalysis *RR_;
        bool Valid_;
        
        ReplayCacheRegion* Start_;
        ReplayCacheRegion* It_;
    };

    typedef rcra_rr_iterator<ReplayCacheRegion> iterator;
    typedef rcra_rr_iterator<const ReplayCacheRegion> const_iterator;

    iterator       begin()       { return iterator(*this); }
    iterator       end()         { return iterator(*this, true); }
    const_iterator begin() const { return const_iterator(*this); }
    const_iterator end()   const { return const_iterator(*this, true); }
    ReplayCacheRegion *front() { return Head; }
    ReplayCacheRegion *back() { return Tail; }

private:
    BumpPtrAllocator RegionAllocator_;
    bool runOnMachineFunction(MachineFunction &MF) override;
    void releaseMemory() override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;

    ReplayCacheRegion &createRegionAtBoundary(ReplayCacheRegion::RegionBlock StartRegionBlock, ReplayCacheRegion::RegionInstr StartRegionInstr, ReplayCacheRegion* Region = nullptr);
};

}