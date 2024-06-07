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
    RRA = &getAnalysis<ReplayCacheRegionAnalysis>();

    std::vector<Register> storeRegs;
    std::vector<Register> spillRegs;

    for (auto &Region : *RRA) {
        for (auto &MI : Region) {
            /* Push all registers used for every store instruction in the region
             * to a vector, to keep track of them.
             */
            if (isStoreInstruction(MI))
            {
                storeRegs.push_back(MI.getOperand(0).getReg());
            }

            if (MI.getNumDefs() > 0)
            {
                for (auto &Def : MI.all_defs())
                {
                    auto Reg = Def.getReg();

                    /* Check if the defining register is used in a store. If it is, we need to
                     * insert a region boundary.
                     */
                    if (std::find(storeRegs.begin(), storeRegs.end(), Reg) != storeRegs.end())
                    {
                        InsertRegionBoundaryBefore(*(MI.getParent()), MI, START_REGION_STACK_SPILL, true);
                        storeRegs.clear();
                        break;
                    }
                }
            }
        }

        /* Clear store register vector when switching regions. */
        storeRegs.clear();
    }
    return false;
}

FunctionPass *llvm::createReplayCacheStackSpillPass() {
    return new ReplayCacheStackSpill();
}
