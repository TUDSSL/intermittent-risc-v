#ifndef REPLAYCACHE_SHARED_H
#define REPLAYCACHE_SHARED_H

#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/SlotIndexes.h"
#include <iostream>

using namespace llvm;

enum ReplayCacheInstruction {
  START_REGION = 1,
  START_REGION_RETURN = 2,
  START_REGION_EXTENSION = 3,
  START_REGION_BRANCH = 4,
  START_REGION_BRANCH_DEST = 5,
  START_REGION_STACK_SPILL = 6,
  FENCE = 7,
  CLWB = 8,
  POWER_FAILURE_NEXT = 9
};

struct SlotInterval
{
    SlotIndex first;
    SlotIndex last;

    unsigned getSize() const { return first.distance(last); }
    bool isEmpty() const { return getSize() == 0; }
    bool contains(SlotIndex &SI) const { return SI >= first && SI <= last; }
    void print(raw_ostream &s) const { s << first << ", " << last; }
};

MachineInstrBuilder BuildRC(MachineBasicBlock &MBB, MachineInstr &MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr);

MachineInstrBuilder BuildRC(MachineBasicBlock &MBB, MachineInstr *MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr);

void InsertStartRegionBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr = START_REGION);
void InsertRegionBoundaryBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr = START_REGION, bool insertInstr = false);

/**
 * Check if the instruction is a ReplayCache instruction.
 * Ideally this would've been done by a special MIFlag, but those flags are
 * stored in uint16_t and there are already 16 flags defined.
 */
bool IsRC(const MachineInstr &MI);
bool IsStartRegion(const MachineInstr &MI);
bool IsFence(const MachineInstr &MI);
bool hasRegionBoundaryBefore(const MachineInstr &MI);
void removeRegionBoundaryBefore(MachineInstr &MI);

void StartRegionInBB(MachineBasicBlock &MBB, const ReplayCacheInstruction Instr = START_REGION, bool insertInstr = false);


bool isStoreInstruction(MachineInstr &MI);
bool isLoadInstruction(MachineInstr &MI);

#endif
