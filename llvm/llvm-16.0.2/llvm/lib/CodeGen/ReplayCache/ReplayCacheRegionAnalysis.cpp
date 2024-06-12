#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "llvm/CodeGen/ReplayCache/ReplayCacheRegionAnalysis.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_RegionAnalysis"

char ReplayCacheRegionAnalysis::ID = 5;

raw_ostream &output1 = llvm::outs();

void ReplayCacheRegionAnalysis::releaseMemory()
{
    RegionAllocator_.Reset();
    
    Head = nullptr;
    Tail = nullptr;
    RegionsSize = 0;
}

void ReplayCacheRegionAnalysis::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
}

/* Create regions at region start instructions, or if the instruction should have a region before it. */
bool ReplayCacheRegionAnalysis::runOnMachineFunction(MachineFunction &MF) 
{
    // output1 << "REGION ANALYSIS\n";
    // output1.flush();

    /* Create regions at START_REGION instructions. */
    for (auto MBB = MF.begin(); MBB != MF.end(); MBB++)
    {
        for (auto MI = MBB->begin(); MI != MBB->end(); MI++)
        {
            auto PrevMI = MI->getPrevNode();

            if (hasRegionBoundaryBefore(*MI) || (PrevMI && IsStartRegion(*PrevMI)))
            {
                // output1 << "Has region boundary before!\n";
                createRegionAtBoundary(MBB, MI);
            }
        }
    }

    return false;
}

/* Create a region before the given instruction. */
void ReplayCacheRegionAnalysis::createRegionBefore(ReplayCacheRegion* Region, ReplayCacheRegion::RegionBlock MBB, ReplayCacheRegion::RegionInstr MI, SlotIndexes *SLIS)
{
    /* Insert instructions. */
    InsertRegionBoundaryBefore(*MBB, *MI, START_REGION_EXTENSION, false);
    SLIS->repairIndexesInRange(&(*MBB), MBB->begin(), MBB->end());
    // createRegionAtBoundary(MBB, MI, Region);
}

/* Create a new region at a region boundary.
 * The new region automatically runs to the start of the next region,
 * or to the end of the current MachineFunction.
 */
void ReplayCacheRegionAnalysis::createRegionAtBoundary(ReplayCacheRegion::RegionBlock StartRegionBlock, ReplayCacheRegion::RegionInstr StartRegionInstr, ReplayCacheRegion* PrevRegion)
{
    ReplayCacheRegion *NewRegion = new (RegionAllocator_) ReplayCacheRegion(RegionsSize, StartRegionInstr, StartRegionBlock);
    // output1 << "Start instr: " << *StartRegionInstr << "\n";
    // output1.flush();

    if (PrevRegion == nullptr)
    {
        if (Head == nullptr)
        {
            Head = NewRegion;
            Tail = NewRegion;
        }
        else
        {
            assert(Tail != nullptr);

            /* Insert into linked list. */
            NewRegion->setPrev(Tail);
            Tail->setNext(NewRegion);

            /* Set end instr and block for previous region. */
            Tail->terminateAt(StartRegionInstr, ++StartRegionBlock);
            
            /* Set tail of linked list to new region. */
            Tail = NewRegion;
        }
    }
    else
    {
        /* Insert into linked list. Assumes there is always a previous region. */
        NewRegion->setPrev(PrevRegion);
        if (PrevRegion->hasNext())
        {
            NewRegion->setNext(PrevRegion->getNext());
            PrevRegion->getNext()->setPrev(NewRegion);
        }
        else
        {
            Tail = NewRegion;
        }
        PrevRegion->setNext(NewRegion);

        /* Set end instr and block for new region. */
        NewRegion->terminateAt(PrevRegion->InstrEnd_, PrevRegion->BlockEnd_);

        /* Set end instr and block for previous region. */
        PrevRegion->terminateAt(StartRegionInstr, ++StartRegionBlock);
    }

    RegionsSize++;
}

INITIALIZE_PASS(ReplayCacheRegionAnalysis, DEBUG_TYPE, PASS_NAME, false, true)
