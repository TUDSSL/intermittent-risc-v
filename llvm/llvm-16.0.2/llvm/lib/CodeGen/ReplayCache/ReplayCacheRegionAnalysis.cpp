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

bool ReplayCacheRegionAnalysis::runOnMachineFunction(MachineFunction &MF) 
{
    // output1 << "REGION ANALYSIS\n";

    /* Create regions at START_REGION instructions. */
    for (auto MBB = MF.begin(); MBB != MF.end(); MBB++)
    {
        for (auto MI = MBB->begin(); MI != MBB->end(); MI++)
        {
            // if (hasRegionBoundaryBefore(*MI))
            // {
            //     // output1 << "Has region boundary before!\n";
            //     createRegionAtBoundary(MBB, MI);
            // }
            if (IsStartRegion(*MI))
            {
                // output1 << "CREATE REGION\n";
                createRegionAtBoundary(MBB, MI);
            }
        }
    }

    return false;
}

ReplayCacheRegion& ReplayCacheRegionAnalysis::createRegionBefore(ReplayCacheRegion* Region, ReplayCacheRegion::RegionBlock MBB, ReplayCacheRegion::RegionInstr MI, SlotIndexes *SLIS)
{
    /* Insert instructions. */
    InsertRegionBoundaryBefore(*MBB, *MI, START_REGION_EXTENSION, true);
    SLIS->repairIndexesInRange(&(*MBB), MBB->begin(), MBB->end());
    return createRegionAtBoundary(MBB, MI, Region);
}

ReplayCacheRegion& ReplayCacheRegionAnalysis::createRegionAtBoundary(ReplayCacheRegion::RegionBlock StartRegionBlock, ReplayCacheRegion::RegionInstr StartRegionInstr, ReplayCacheRegion* PrevRegion)
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

    return *NewRegion;
}

// FunctionPass *llvm::createReplayCacheRegionAnalysisPass() {
//     return new ReplayCacheRegionAnalysis();
// }

INITIALIZE_PASS(ReplayCacheRegionAnalysis, DEBUG_TYPE, PASS_NAME, false, true)
