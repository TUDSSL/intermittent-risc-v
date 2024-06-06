#pragma once

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include "ReplayCacheShared.h"
#include <vector>
#include <iterator>

namespace llvm 
{

class ReplayCacheRegion
{
private:
    ReplayCacheRegion *Next_;
    ReplayCacheRegion *Prev_;

public:
    
    MachineFunction *MF_;
    using RegionInstr = MachineBasicBlock::iterator;
    using RegionBlock = MachineFunction::iterator;

    /* Start instruction of the region. */
    RegionInstr InstrStart_;
    /* End instruction of the region. Can be in a different MBB, 
     * so we need to keep track of MBBs.
     */
    RegionInstr InstrEnd_;

    /* Start MBB of the region. */
    RegionBlock BlockStart_;
    /* End MBB of the region. */
    RegionBlock BlockEnd_;
public:
    /* Iterator for iterating over instructions in a region.
     * Copied from https://github.com/mishermasher/llvm/tree/idempotence-extensions and slightly modified.
     */
    template <typename MachineInstrTy>
    class rcr_inst_iterator : public std::iterator<std::forward_iterator_tag, MachineInstrTy *> {
    public:
        // Constructors.
        explicit rcr_inst_iterator(const ReplayCacheRegion &Region, const bool isEnd = false) :
            Region_(&Region), 
            Valid_(!isEnd && Region.InstrStart_ != Region.InstrEnd_),
            InstrFinalIsSentinel_(false), 
            InstrStart_(Region.InstrStart_),
            InstrEnd_(isEnd ? Region.InstrEnd_ : Region.InstrStart_),
            InstrIt_(isEnd ? Region.InstrEnd_ : Region.InstrStart_), 
            InstrFinal_(Region.InstrEnd_),
            BlockStart_(Region.BlockStart_),
            BlockEnd_(Region.BlockEnd_),
            BlockIt_(isEnd ? Region.BlockEnd_ : Region.BlockStart_)
        {
            /* Figure out if the final instruction is a sentinel. */
            for (auto BlockIt = BlockStart_; BlockIt != BlockEnd_; BlockIt++)
            {
                if (InstrFinal_ == BlockIt->end())
                {
                    InstrFinalIsSentinel_ = true;
                }
            }

            /* Ignore empty blocks at the start. */
            // if (!isEnd)
            // {
            //     bool advanced = false;

            //     while (BlockIt_ != BlockEnd_ && BlockIt_->empty())
            //     {
            //         ++BlockIt_;
            //         advanced = true;
            //     }

            //     if (advanced)
            //     {
            //         InstrEnd_ = BlockIt_->begin();
            //         InstrIt_ = BlockIt_->begin();
            //     }
            // }

            /* Make InstrEnd_ go to the end of the current basic block. */
            if (!isEnd)
            {
                InstrEnd_ = BlockIt_->end();
            }
        }

        // Return whether the iterator is valid.  False implies the end condition
        // has been met.
        bool isValid() { return Valid_; };

        // Comparison.
        bool operator==(const rcr_inst_iterator &X) const;
        bool operator!=(const rcr_inst_iterator &X) const { return !operator==(X); };

        // Pre-increment and post-increment.
        rcr_inst_iterator<MachineInstrTy> &operator++();
        rcr_inst_iterator<MachineInstrTy> operator++(int) { rcr_inst_iterator Tmp = *this; ++*this; return Tmp; };

        // Dereference.
        MachineInstrTy &operator*() const { assert(InstrIt_ != InstrEnd_ && InstrIt_ != InstrFinal_); return *InstrIt_; };
        MachineInstrTy *operator->() const { assert(InstrIt_ != InstrEnd_ && InstrIt_ != InstrFinal_); return &*InstrIt_; };
        RegionInstr getInstrIt() const { return InstrIt_; };

        MachineBasicBlock *getMBB() const { return Valid_ ? InstrIt_->getParent() : nullptr; };
        RegionBlock getMBBIt() const { return BlockIt_; };

        MachineBasicBlock::iterator getIterator() const { return InstrIt_; };

        SlotInterval getSlotIntervalInCurrentMBB(const SlotIndexes &SLIS, MachineInstr *MIStart = nullptr) const;

        void stop();

        // Debugging.
        void print(raw_ostream &OS) const { (*InstrIt_).print(OS); };

    private:
        const ReplayCacheRegion *Region_;
        bool Valid_;

        bool InstrFinalIsSentinel_;
        
        MachineBasicBlock::iterator InstrStart_;
        MachineBasicBlock::iterator InstrEnd_;
        MachineBasicBlock::iterator InstrIt_;
        MachineBasicBlock::iterator InstrFinal_;
        
        MachineFunction::iterator BlockStart_;
        MachineFunction::iterator BlockEnd_;
        MachineFunction::iterator BlockIt_;
    };

    typedef rcr_inst_iterator<MachineInstr> inst_iterator;
    typedef rcr_inst_iterator<const MachineInstr> const_inst_iterator;

    inst_iterator       begin()       { return inst_iterator(*this); }
    inst_iterator       end()         { return inst_iterator(*this, true); }
    const_inst_iterator begin() const { return const_inst_iterator(*this); }
    const_inst_iterator end()   const { return const_inst_iterator(*this, true); }

public:
    unsigned ID_;

    /* Start the region at an instruction. */
    ReplayCacheRegion(unsigned ID, RegionInstr StartRegionInstr, RegionBlock StartRegionBlock);

    /* Terminate the region at this instruction. */
    void terminateAt(RegionInstr FenceInstr, RegionBlock FenceBlock);

    bool containsInstr(MachineInstr MI);

    SlotInterval getSlotIntervalInCurrentMBB(const SlotIndexes &SLIS, MachineInstr *MIStart = nullptr) const;

    bool hasNext();
    bool hasPrev();

    void setNext(ReplayCacheRegion *Region);
    void setPrev(ReplayCacheRegion *Region);

    ReplayCacheRegion *getNext();
    ReplayCacheRegion *getPrev();

    unsigned getSize(const SlotIndexes &SLIS) { return SLIS.getInstructionIndex(*InstrStart_).distance(SLIS.getInstructionIndex(*InstrEnd_)); }

    void print(raw_ostream &OS) const { OS << "Region ID: " << ID_ << "\n"; };
};

template <typename MachineInstrTy>
bool
ReplayCacheRegion::rcr_inst_iterator<MachineInstrTy>::operator==(const rcr_inst_iterator &X) const
{
    return InstrIt_ == X.InstrIt_;
}

template <typename MachineInstrTy>
ReplayCacheRegion::rcr_inst_iterator<MachineInstrTy> &
ReplayCacheRegion::rcr_inst_iterator<MachineInstrTy>::operator++()
{
    assert(Valid_ && "iterating past end condition");
    assert(BlockIt_ != Region_->MF_->end());
    assert(InstrIt_ != BlockIt_->end());
    
    /* Increment instruction iterator. */
    ++InstrIt_;

    /* Return if the final instruction in the region has been reached. */
    if (InstrIt_ == InstrFinal_)
    {
        Valid_ = false;
    }
    /* We reached the end of the current basic block, increment basic block and find
     * next instruction.
     */
    else if (InstrIt_ == InstrEnd_)
    {
        assert(BlockIt_ != BlockEnd_);
        ++BlockIt_;

        /* Find next non-empty MBB. */
        while (BlockIt_ != BlockEnd_ && BlockIt_->empty())
        {
            ++BlockIt_;
        }

        if (BlockIt_ != BlockEnd_)
        {
            assert(BlockIt_ != Region_->MF_->end());
            assert(!BlockIt_->empty());

            /* Set Instr iterator to first instruction in new MBB. */
            InstrIt_ = BlockIt_->begin();

            /* Set InstrEnd to end instruction of MBB or final instruction, whichever comes first. */
            InstrEnd_ = BlockIt_->begin();
            while (InstrEnd_ != BlockIt_->end() && InstrEnd_ != InstrFinal_)
            {
                ++InstrEnd_;
            }
        }
        /* Final basic block reached, region ends. */
        else
        {
            Valid_ = false;
        }
    }
    
    return *this;
}

/* Get the slot index interval of the region in the current MBB. */
template <typename MachineInstrTy>
SlotInterval ReplayCacheRegion::rcr_inst_iterator<MachineInstrTy>::getSlotIntervalInCurrentMBB(const SlotIndexes &SLIS, MachineInstr *MIStart) const
{
    assert(Valid_ && "Iterator invalid");

    MachineBasicBlock *MBB = getMBB();
    SlotInterval SI;

    /* Return invalid SlotIndices if MBB is empty.*/
    if (MBB->empty())
    {
        return SI;
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
    if (!InstrFinalIsSentinel_ && InstrFinal_->getParent() == MBB)
    {
        /* Get non-debug final instr (using debug instr crashes compiler). */
        RegionInstr RealInstrFinal = InstrFinal_;
        while (RealInstrFinal != MBB->end() && RealInstrFinal->isDebugInstr())
        {
            RealInstrFinal--;
        }

        if (RealInstrFinal == MBB->begin())
        {
            SI.last = SLIS.getMBBStartIdx(MBB);
        }
        else {
            SI.last = SLIS.getInstructionIndex(*RealInstrFinal).getRegSlot();
        }
    }
    else
    {
        SI.last = SLIS.getMBBEndIdx(MBB);
    }

    assert(SI.first <= SI.last && "Inverse interval!");
    
    return SI;
}

// template <typename MachineInstrTy>
// void ReplayCacheRegion::rcr_inst_iterator<MachineInstrTy>::stop()
// {
//     InstrIt_ = --InstrFinal_;
//     InstrEnd_ = ++InstrFinal_;
// }

}