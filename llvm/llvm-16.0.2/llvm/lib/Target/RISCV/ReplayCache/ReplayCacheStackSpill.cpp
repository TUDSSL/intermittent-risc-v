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
// INITIALIZE_PASS_DEPENDENCY(LiveIntervalExtensionAnalysis)
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
// INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
// INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
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
            if (isStoreInstruction(MI))
            {
                storeRegs.push_back(MI.getOperand(0).getReg());
            }

            if (MI.getNumDefs() > 0)
            {
                /* Add all intervals for register definitions to the vector. */
                for (auto &Def : MI.all_defs())
                {
                    auto Reg = Def.getReg();

                    /* Add this interval to the vector if it doesn't already exist. */
                    if (std::find(storeRegs.begin(), storeRegs.end(), Reg) != storeRegs.end())
                    {
                        InsertRegionBoundaryBefore(*(MI.getParent()), MI, START_REGION_STACK_SPILL, true);
                        storeRegs.clear();
                        break;
                    }
                }
                // for (const auto *Op : MI.memoperands()) {
                //     if (const PseudoSourceValue *PVal = Op->getPseudoValue())
                //     {
                //         if (PVal->kind() == PseudoSourceValue::Stack || PVal->kind() == PseudoSourceValue::FixedStack)
                //         {
                //             if (std::find(storeRegs.begin(), storeRegs.end(), MI.getOperand(0).getReg()) != storeRegs.end())
                //             {
                //                 InsertRegionBoundaryBefore(*(MI.getParent()), MI, START_REGION_STACK_SPILL, true);
                //                 storeRegs.clear();
                //             }
                //             output_ss << "Stack load: " << MI;
                //         }
                //     }
                // }
            }
        }

        storeRegs.clear();
    }
    return false;
}

FunctionPass *llvm::createReplayCacheStackSpillPass() {
    return new ReplayCacheStackSpill();
}

// INITIALIZE_PASS(ReplayCacheStackSpill, DEBUG_TYPE, PASS_NAME, false, false)
