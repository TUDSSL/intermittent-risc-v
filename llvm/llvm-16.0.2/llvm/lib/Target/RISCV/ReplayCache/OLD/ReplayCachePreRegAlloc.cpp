#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "ReplayCacheShared.h"
#include "ReplayCachePreRegAlloc.h"
#include "llvm/InitializePasses.h"
#include "llvm/CodeGen/MachineOperand.h"
#include <iostream>

using namespace llvm;

#define PASS_NAME "ReplayCache_PreRegAlloc"

INITIALIZE_PASS_BEGIN(ReplayCachePreRegAlloc, DEBUG_TYPE, PASS_NAME,
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LiveIntervals)
INITIALIZE_PASS_END(ReplayCachePreRegAlloc, DEBUG_TYPE,
                PASS_NAME, false, false)

raw_ostream &output = llvm::outs();

char ReplayCachePreRegAlloc::ID = 99;
ReplayCachePreRegAlloc::ReplayCachePreRegAlloc() : MachineFunctionPass(ID) {}

void ReplayCachePreRegAlloc::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<LiveIntervals>();

    MachineFunctionPass::getAnalysisUsage(AU);
}

bool ReplayCachePreRegAlloc::runOnMachineFunction(MachineFunction &MF) {
    // Skip naked functions
    // TODO: determine eligible functions in an earlier pass
    if (MF.getFunction().hasFnAttribute(Attribute::Naked))
        return false;

    // TODO: make sure this only runs once

    auto &TII = *MF.getSubtarget().getInstrInfo();

    for (auto &MBB : MF) {

        if (MBB.isEntryBlock())
        {
        StartRegionInBB(MBB);
        output << "Entry block region started!\n";
        }
        

        for (auto &MI : MBB) {

        // Skip any instructions that were already inserted by this pass
        if (IsRC(MI))
            continue;

        auto NextMI = MI.getNextNode();

        if (MI.isCall()) {
            // Start a new region when callee returns
            if (NextMI) {
            BuildRC(MBB, NextMI, NextMI->getDebugLoc(), FENCE);
            BuildRC(MBB, NextMI, NextMI->getDebugLoc(), START_REGION);
            output << "NEW region started (callee return)!\n";
            } else {
            // If there is no next instruction, the callee returns to the
            // fallthrough
            auto Fallthrough = MBB.getFallThrough();
            if (Fallthrough)
                StartRegionInBB(*Fallthrough);
            // If there is no fallthrough, this is the last block in the function
            // and the call is probably a tail call.
            }
        } else if (MI.isReturn()) {
            // No need to 'explicitly' end a region when returning,
            // because the caller is expected to do that already
        } else if (MI.isConditionalBranch()) {
            // Create boundaries around branches
            auto TrueDest = TII.getBranchDestBlock(MI);
            MachineBasicBlock *FalseDest = nullptr;

            if (NextMI) {
            // If the next instruction is an unconditional branch, where it jumps
            // to should be interpreted as the false branch
            if (NextMI->isUnconditionalBranch()) {
                FalseDest = TII.getBranchDestBlock(*NextMI);
            }
            // else: nothing of interest (?)
            } else {
            // If there is no next instruction, the false branch is the fallthrough
            FalseDest = MBB.getFallThrough();
            }

            StartRegionInBB(*TrueDest);
            if (FalseDest)
            StartRegionInBB(*FalseDest);
        } else {
            // Insert CLWB after stores
            if (NextMI) {
            switch (MI.getOpcode()) {
            default:
                break;
            case RISCV::SW:
            case RISCV::SH:
            case RISCV::SB:
            case RISCV::C_SW:
            case RISCV::C_SWSP:
                BuildRC(MBB, NextMI, MI.getDebugLoc(), CLWB);
                output << "Add CLWB instruction!\n";
                break;
            }
            }
        }
        }
    }
    // MachineInstr *PrevInstr = nullptr;
    // LIS = &getAnalysis<LiveIntervals>();

    // output << "REGION PARTITIONING START!!!\n";

    // for (auto &MBB : MF) {
    //     for (auto &MI : MBB) {
    //         PrevInstr = &MI;

    //         /* Check if we have hit a region boundary. If so, we do not extend the live intervals past this boundary. */
    //         if (IsRC(MI))
    //         {
    //             ReplayCacheInstruction instr = static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm());
    //             if (instr == ReplayCacheInstruction::FENCE)
    //             {
    //                 output << "Found FENCE\n";

    //                 SlotIndex index = LIS->getInstructionIndex(MI).getRegSlot();

    //                 for (LiveInterval* LI : Intervals)
    //                 {
    //                     if (LI->find(index) == LI->end())
    //                     {
    //                         LIS->extendToIndices(*LI, {});
    //                     }
    //                 }
    //             }
    //         }
    //         /* Only extend live interval of registers used in store instructions. */
    //         if (isStoreInstruction(MI))
    //         {
    //             auto MO = MI.getOperand(0);
    //             Register Reg = MO.getReg();
    //             LiveInterval &LI = LIS->getInterval(Reg);

    //             if (std::find(Intervals.begin(), Intervals.end(), &LI) == Intervals.end())
    //             {
    //                 if (Intervals.size() >= NUM_STORE_REGISTERS)
    //                 {
    //                     /* Insert a region boundary before the current instruction. */
    //                     BuildRC(MBB, MI.getPrevNode(), MI.getDebugLoc(), FENCE);
    //                     BuildRC(MBB, MI.getPrevNode(), MI.getDebugLoc(), START_REGION);
    //                     /* Deactivate all current intervals. */
    //                     Intervals.clear();
    //                 }

    //                 output << "Push interval!\n";

    //                 Intervals.push_back(&LI);

    //                 // IntervalsMap[LI_ptr] = IntervalsMapEntry{ .active = true, .index = LIS->getInstructionIndex(MI).getRegSlot()};
    //             }

    //             /* Check if live interval is not in the map yet. */
    //             // if (IntervalsMap.count(LI_ptr) == 0)
    //             // {
    //             //     output << "Active intervals: " << getActiveLiveIntervalsCount() << "\n";

    //             //     /* Put a region boundary if we exceed the number of allocated registers. */
    //             //     if (getActiveLiveIntervalsCount() >= NUM_STORE_REGISTERS)
    //             //     {
    //             //         /* Insert a region boundary before the current instruction. */
    //             //         BuildRC(MBB, MI.getPrevNode(), MI.getDebugLoc(), FENCE);
    //             //         BuildRC(MBB, MI.getPrevNode(), MI.getDebugLoc(), START_REGION);

    //             //         /* Deactivate all current intervals. */
    //             //         deactivateLiveIntervals(MI);
    //             //     }
                    
    //             //     IntervalsMap[LI_ptr] = IntervalsMapEntry{ .active = true, .index = LIS->getInstructionIndex(MI).getRegSlot()};
    //             // }
    //         }
    //     }

    //     /* Add MBB end index to active live interval extension map entries. */
    //     // if (PrevInstr != nullptr)
    //     // {
    //     //     extendLiveIntervalsToInstr(*PrevInstr);
    //     //     PrevInstr = nullptr;
    //     // }
    // }

    // // TODO: extend live intervals
    // // LIS->extendToIndices(LI, NewIndices);
    // // LI.verify();
    // for (auto &[key, value] : IntervalsMap)
    // {
        // key->print(output);
        // LiveInterval &LI = LIS->getInterval(value.reg);
        // output << "LIVE INTERVAL: " << *key << "\n";
        // if (value.indices.size() > 0 && key->find(value.indices.back()) == key->end())
        // {
        //     // LICalc->extend(*key, value.indices.back(), 0, {});
        //     LIS->extendToIndices(*key, {value.indices.back()});
        // }
    // }

    // printIntervalsMapContents();

    return false;
}

FunctionPass *llvm::createReplayCachePreRegAllocPass() {
    return new ReplayCachePreRegAlloc();
}

// INITIALIZE_PASS(ReplayCacheRegisterRegionPartitioning, DEBUG_TYPE, PASS_NAME, false, true)
