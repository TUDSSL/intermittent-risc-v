#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCacheInitialRegions.h"
#include "llvm/InitializePasses.h"
#include "ReplayCacheRegisterRegionPartitioning.h"
#include "llvm/CodeGen/MachineOperand.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_RegisterRegionPartitioning"

char ReplayCacheRegisterRegionPartitioning::ID = 1;

INITIALIZE_PASS_BEGIN(ReplayCacheRegisterRegionPartitioning, DEBUG_TYPE, PASS_NAME,
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_DEPENDENCY(SlotIndexes)
INITIALIZE_PASS_DEPENDENCY(ReplayCacheRegionAnalysis)
// INITIALIZE_PASS_DEPENDENCY(ReplayCacheInitialRegions)
INITIALIZE_PASS_END(ReplayCacheRegisterRegionPartitioning, DEBUG_TYPE,
                PASS_NAME, false, false)

raw_ostream &output = llvm::outs();

ReplayCacheRegisterRegionPartitioning::ReplayCacheRegisterRegionPartitioning() : MachineFunctionPass(ID) {}

void ReplayCacheRegisterRegionPartitioning::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesCFG();
    // AU.addRequired<ReplayCacheInitialRegions>();
    AU.addPreserved<ReplayCacheInitialRegions>();
    AU.addRequired<LiveIntervals>();
    AU.addRequired<ReplayCacheRegionAnalysis>();
    AU.addRequired<SlotIndexes>();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCacheRegisterRegionPartitioning::runOnMachineFunction(MachineFunction &MF) {
    // MachineInstr *PrevInstr = nullptr;
    output << "REGION PARTITIONING START!!!\n";
    Intervals.clear();

    LIS = &getAnalysis<LiveIntervals>();
    SIS = &getAnalysis<SlotIndexes>();
    RRS = &getAnalysis<ReplayCacheRegionAnalysis>();

    // MachineInstr *last_instruction = nullptr;
    // MachineBasicBlock *last_mbb = nullptr;

    // for (auto Region = RRS->begin(); Region != RRS->end(); Region++)
    // {
    //     assert(*Region != nullptr);
    //     (*Region)->print(output);
    //     output.flush();
    // }
    // int ctr = 0;

    // for (auto Region : *RRS)
    // {
    //     // Region->print(output);
    //     // output.flush();
        
    //     for (auto MI : *Region)
    //     {
    //         // output << "New MI\n";
    //         // if (ctr == 0)
    //         // {
    //         //     MI->getParent()->print(output);
    //         //     ctr++;
    //         // }
    //         // MI->print(output);
    //         // output.flush();
    //     }

    //     ctr = 0;
    // }

    // for (auto &MBB : MF) {
    //     // output << "Entering block\n" << MBB;
    //     // output.flush();

    //     for (auto &MI : MBB) {
    //         // output << "Processing instruction: " << MI;
    //         // output.flush();
    //         // PrevInstr = &MI;

    //         /* Check if we have hit a region boundary. If so, we do not extend the live intervals past this boundary. */
    //         if (IsRC(MI) && Intervals.size() > 0)
    //         {
    //             ReplayCacheInstruction instr = static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm());
    //             if (instr == ReplayCacheInstruction::FENCE && MI.getPrevNode())
    //             {
    //                 output << "Found FENCE\n";

    //                 MachineInstr *extension_instr = MI.getPrevNode() ? MI.getPrevNode() : &MI;
    //                 MachineBasicBlock *extension_block = &MBB;

    //                 while (extension_instr->getPrevNode() && (extension_instr->isPHI() || extension_instr->isDebugInstr()))
    //                 {
    //                     extension_instr = extension_instr->getPrevNode();
    //                 }

    //                 if (!extension_instr->getPrevNode())
    //                 {
    //                     output << "No prev node found\n";
    //                     output.flush();
    //                     extension_block = last_mbb;
    //                     extension_instr = &*(last_mbb->getLastNonDebugInstr());
    //                 }

    //                 // output << "Original instr: " << MI << "\n";
    //                 // output << "Extension instr: " << *extension_instr << "\n";
                    

    //                 output.flush();
    //                 extendLiveIntervalsToInstr(MF, *extension_block, *extension_instr);
    //                 Intervals.clear();
    //             }
    //         }
    //         /* Only extend live interval of registers used in store instructions. */
    //         else if (isStoreInstruction(MI))
    //         {
    //             auto MO = MI.getOperand(0);
    //             Register Reg = MO.getReg();
    //             LiveInterval &LI = LIS->getInterval(Reg);

    //             if (std::find(Intervals.begin(), Intervals.end(), std::pair<LiveInterval *, Register>(&LI, Reg)) == Intervals.end())
    //             {
    //                 if (Intervals.size() >= NUM_STORE_REGISTERS)
    //                 {
    //                     /* Insert a region boundary before the current instruction. */
    //                     BuildRC(MBB, MI, MI.getDebugLoc(), FENCE);
    //                     BuildRC(MBB, MI, MI.getDebugLoc(), START_REGION);
    //                     SIS->repairIndexesInRange(&MBB, MBB.begin(), MBB.end());

    //                     /**/
    //                     extendLiveIntervalsToInstr(MF, MBB, *MI.getPrevNode());

    //                     Intervals.clear();
    //                 }

    //                 output << "Push interval!\n";

    //                 Intervals.push_back(std::pair<LiveInterval *, Register>(&LI, Reg));

    //             }
    //         }

    //         last_instruction = &MI;
    //     }

    //     last_mbb = &MBB;
    // }

    /* Extend remaining live intervals to the end of the function. */
    // if (last_instruction && last_mbb)
    // {
    //     extendLiveIntervalsToInstr(MF, *last_mbb, *last_instruction);
    // }

    output << "REGION PARTITIONING END!!!\n";

    return true;
}

void ReplayCacheRegisterRegionPartitioning::extendLiveIntervalsToInstr(MachineFunction &MF, MachineBasicBlock &MBB, MachineInstr &MI)
{
    SlotIndex index = LIS->getInstructionIndex(MI).getRegSlot();

    output << "Extending " << Intervals.size() << " intervals...\n";

    for (auto interval : Intervals)
    {
        auto LI = interval.first;
        auto Reg = interval.second;

        if (LI != nullptr && LI->find(index) == LI->end())
        {
            output << "Extend LI to ";
            index.print(output);
            output << "\n";
            output.flush();

            MachineInstrBuilder(MF, MI).addUse(Reg, RegState::Implicit, 0);
            LIS->extendToIndices(*LI, {index});

            // TODO: travel back up the tree, for each predecessor of MBB,
            // check if there is a Def for the new use. If there isn't, add an implicit def.


            // for (auto MBBpred : MBB.predecessors())
            // {
            //     bool found_def = false;

            //     for (auto &MIpred : *MBBpred) {
            //         if (MIpred.findRegisterDefOperandIdx(Reg) != -1)
            //         {
            //             output << "Found def for " << Reg << "\n";
            //             found_def = true;
            //             break;
            //         }
            //     }

            //     if (!found_def)
            //     {
            //         output << "Did not find def for " << Reg << " in block " << *MBBpred << "\n";
            //         MachineInstrBuilder(MF, MBBpred->instr_front()).addDef(Reg, RegState::Implicit, 0);
            //     }
            // }
            
            // LIS->extendToIndices(*LI, {index});
        }
        else {
            // Shrink live interval
            // Use Kill with PruneValue?
        }
    }
}

FunctionPass *llvm::createReplayCacheRegisterRegionPartitioningPass() {
    return new ReplayCacheRegisterRegionPartitioning();
}

// INITIALIZE_PASS(ReplayCacheRegisterRegionPartitioning, DEBUG_TYPE, PASS_NAME, false, true)
