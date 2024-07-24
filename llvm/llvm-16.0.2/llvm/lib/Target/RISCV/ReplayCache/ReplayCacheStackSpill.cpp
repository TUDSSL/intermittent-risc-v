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
#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "ReplayCacheStackSpill.h"
#include <iostream>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "replaycache"
#define PASS_NAME "ReplayCache_StackSpill"


raw_ostream &output_ss = llvm::outs();

char ReplayCacheStackSpill::ID = 7;

INITIALIZE_PASS_BEGIN(ReplayCacheStackSpill, DEBUG_TYPE, PASS_NAME,
                      false, false)
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
INITIALIZE_PASS_END(ReplayCacheStackSpill, DEBUG_TYPE,
                    PASS_NAME, false, false)

void ReplayCacheStackSpill::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<ReplayCacheRegionAnalysis>();
    AU.addPreserved<ReplayCacheRegionAnalysis>();
    AU.setPreservesAll();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheStackSpill::runOnMachineFunction(MachineFunction &MF) {
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
                    if (Reg == storereg0 || Reg == storereg1)
                    {
                        InsertRegionBoundaryBefore(*MBB, I, START_REGION_STACK_SPILL, true);
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
