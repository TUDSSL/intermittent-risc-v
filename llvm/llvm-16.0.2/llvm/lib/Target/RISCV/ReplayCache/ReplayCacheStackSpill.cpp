/**
 * Stack spill
 * 
 * Handle any stack spills by checking for any registers
 * that are used by stack spill stores and other instructions.
 * 
 * If a register is used by both, then we need to insert a
 * region boundary.
 * 
 * Runs after region repair and before CLWB insertion.
 */
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheStackSpill.h"
#include <iostream>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_StackSpill"


raw_ostream &output_ss = llvm::outs();

char ReplayCacheStackSpill::ID = 7;

void ReplayCacheStackSpill::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<SlotIndexes>();
    AU.addPreserved<SlotIndexes>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheStackSpill::runOnMachineFunction(MachineFunction &MF) {
    SLIS = &getAnalysis<SlotIndexes>();

    for (auto &MBB : MF){
        for (auto &MI : MBB) {
            if (isStoreInstruction(MI))
            {
                checkRegionForStackSpill(MF, MI);
            }
        }
    }
    return false;
}


void ReplayCacheStackSpill::checkRegionForStackSpill(MachineFunction &MF, MachineInstr &MI)
{
    auto storereg0 = MI.getOperand(0).getReg();
    auto storereg1 = MI.getOperand(1).getReg();

    std::vector<Register> storeRegs;
    storeRegs.push_back(storereg0);
    storeRegs.push_back(storereg1);

    auto *MBB = MI.getParent();
    bool foundFence = false;
    std::vector<MachineBasicBlock*> Visited;
    bool FirstIter = true;

    do {
        if (std::find(Visited.begin(), Visited.end(), MBB) != Visited.end())
        {
          break;
        }

        Visited.push_back(MBB);

        for (auto &I : *MBB)
        {
          if (!FirstIter && isStoreInstruction(I))
          {
            storeRegs.push_back(I.getOperand(0).getReg());
            storeRegs.push_back(I.getOperand(1).getReg());
          }

          /* Start iteration from MI. */
          if (FirstIter)
          {
            if (&I == &MI)
            {
              FirstIter = false;
            }
          }
          else if (IsStartRegion(I) || hasRegionBoundaryBefore(I))
          {
            foundFence = true;
            break;
          }
          else
          {
            if (I.getNumDefs() > 0)
            {
                for (auto &Def : I.all_defs())
                {
                    auto Reg = Def.getReg();

                    /* Check if the defining register is redefined. If it is, we need to
                     * insert a region boundary.
                     */
                    if (std::find(storeRegs.begin(), storeRegs.end(), Reg) != storeRegs.end())
                    {
                        InsertRegionBoundaryBefore(*MBB, I, START_REGION_STACK_SPILL, true);
                        SLIS->repairIndexesInRange(MBB, MBB->begin(), MBB->end());
                        foundFence = true;
                        break;
                    }
                }

                if (foundFence)
                {
                    break;
                }
            }
          }
        }

        if (MBB->succ_size() == 1)
        {
          MBB = *MBB->succ_begin();
        }
        else
        {
          MBB = nullptr;
        }

        FirstIter = false;
    }
    while (MBB != nullptr && !foundFence);
}

FunctionPass *llvm::createReplayCacheStackSpillPass() {
    return new ReplayCacheStackSpill();
}


INITIALIZE_PASS(ReplayCacheStackSpill, DEBUG_TYPE, PASS_NAME, false, false)