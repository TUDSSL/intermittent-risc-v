#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"

/* Build and insert a new ReplayCache instruction. */
MachineInstrBuilder BuildRC(MachineBasicBlock &MBB, MachineInstr &MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  return BuildMI(*MBB.getParent(), DL, TII.get(RISCV::C_LI), RISCV::X31) // TODO: missing isIndirect?
      .addImm(Instr);
}
MachineInstrBuilder BuildRC(MachineBasicBlock &MBB, MachineInstr *MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  return BuildMI(*MBB.getParent(), DL, TII.get(RISCV::C_LI), RISCV::X31) // TODO: missing isIndirect?
      .addImm(Instr);
}

/* Insert region boundary instructions before */
void InsertRegionBoundaryBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr, bool insertInstr)
{
  MI.setFlag(MachineInstr::MIFlag::HasRegionBoundaryBefore);
  MI.StartRegionInstr = (unsigned)Instr;
  if (insertInstr)
  {
    MBB.insert(MI, BuildRC(MBB, MI, MI.getDebugLoc(), ReplayCacheInstruction::FENCE));
    MBB.insert(MI, BuildRC(MBB, MI, MI.getDebugLoc(), Instr));
  }
}

/**
 * Check if the instruction is a ReplayCache instruction.
 * Ideally this would've been done by a special MIFlag, but those flags are
 * stored in uint16_t and there are already 16 flags defined.
 */
bool IsRC(const MachineInstr &MI) {
  auto &TII = *MI.getParent()->getParent()->getSubtarget().getInstrInfo();
  return MI.getOpcode() == TII.get(RISCV::C_LI).getOpcode() &&
         MI.getOperand(0).getReg() == RISCV::X31;
}

/* Check if instruction is start region. */
bool IsStartRegion(const MachineInstr &MI)
{
  return IsRC(MI) && (static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm()) < ReplayCacheInstruction::FENCE);
}
/* Check if instruction is fence. */
bool IsFence(const MachineInstr &MI)
{
  return IsRC(MI) && static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm()) == ReplayCacheInstruction::FENCE;
}

/* Check if instruction should have a region boundary before it. */
bool hasRegionBoundaryBefore(const MachineInstr &MI)
{
  return MI.getFlag(MachineInstr::MIFlag::HasRegionBoundaryBefore);
}

/* Remove the region boundary before flag. */
void removeRegionBoundaryBefore(MachineInstr &MI)
{
  MI.clearFlag(MachineInstr::MIFlag::HasRegionBoundaryBefore);
}

/* Start region in the specified MBB.
 * If the specified MBB is empty, go to the next
 * non-empty MBB.
 */
void StartRegionInBB(MachineBasicBlock &MBB, const ReplayCacheInstruction Instr, bool insertInstr) {
  auto FirstNonPHI = MBB.getFirstNonPHI();
  while (FirstNonPHI != MBB.instr_end() && FirstNonPHI->isDebugInstr())
  {
    FirstNonPHI++;
  }

  /* If MBB is empty, go on to next MBB if it exists. */
  if (FirstNonPHI == MBB.instr_end()) {
    MachineBasicBlock *next = MBB.getNextNode();
    if (next)
    {
      return StartRegionInBB(*next, Instr, insertInstr);
    }
    else
    {
      return;
    }
  }
    
  auto &EntryInstr = *FirstNonPHI;
  if (IsRC(EntryInstr))
    return;
  InsertRegionBoundaryBefore(MBB, EntryInstr, Instr, insertInstr);
}

/* Check if instruction is store instruction. */
bool isStoreInstruction(MachineInstr &MI)
{
    switch (MI.getOpcode()) {
        case RISCV::SW:
        case RISCV::SH:
        case RISCV::SB:
        case RISCV::C_SW:
        case RISCV::C_SWSP:
            return true;
    }

    return false;
}

/* Check if instruction is load instruction. */
bool isLoadInstruction(MachineInstr &MI)
{
    switch (MI.getOpcode()) {
        case RISCV::LW:
        case RISCV::LH:
        case RISCV::LB:
        case RISCV::C_LW:
        case RISCV::C_LWSP:
            return true;
    }

    return false;
}
