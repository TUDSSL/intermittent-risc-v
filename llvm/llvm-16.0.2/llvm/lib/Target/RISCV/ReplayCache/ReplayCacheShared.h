#ifndef REPLAYCACHE_SHARED_H
#define REPLAYCACHE_SHARED_H

#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <iostream>

#define DEBUG_TYPE "replaycache"

using namespace llvm;

enum ReplayCacheInstruction {
  START_REGION = 0,
  CLWB = 1,
  FENCE = 2,
  POWER_FAILURE_NEXT = 3,
};

void BuildRC(MachineBasicBlock &MBB, MachineInstr &MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr);

void BuildRC(MachineBasicBlock &MBB, MachineInstr *MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr);

void InsertStartRegionBefore(MachineBasicBlock &MBB, MachineInstr &MI);
void InsertRegionBoundaryBefore(MachineBasicBlock &MBB, MachineInstr &MI);

/**
 * Check if the instruction is a ReplayCache instruction.
 * Ideally this would've been done by a special MIFlag, but those flags are
 * stored in uint16_t and there are already 16 flags defined.
 */
bool IsRC(const MachineInstr &MI);
bool IsStartRegion(const MachineInstr &MI);
bool IsFence(const MachineInstr &MI);

void StartRegionInBB(MachineBasicBlock &MBB);


bool isStoreInstruction(MachineInstr &MI);

#endif
