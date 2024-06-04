#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"

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

void InsertStartRegionBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr)
{
  MBB.insert(MI, BuildRC(MBB, MI, MI.getDebugLoc(), Instr));
}

void InsertRegionBoundaryBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr, bool insertInstr)
{
  MI.setFlag(MachineInstr::MIFlag::HasRegionBoundaryBefore);
  MI.setFlag(MachineInstr::MIFlag::NoMerge);
  MI.StartRegionInstr = (unsigned)Instr;
  if (insertInstr)
  {
    MBB.insert(MI, BuildRC(MBB, MI, MI.getDebugLoc(), ReplayCacheInstruction::FENCE).setMIFlag(MachineInstr::MIFlag::NoMerge));
    MBB.insert(MI, BuildRC(MBB, MI, MI.getDebugLoc(), Instr).setMIFlag(MachineInstr::MIFlag::NoMerge));
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

bool IsStartRegion(const MachineInstr &MI)
{
  return IsRC(MI) && (static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm()) < ReplayCacheInstruction::FENCE);
}
bool IsFence(const MachineInstr &MI)
{
  return IsRC(MI) && static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm()) == ReplayCacheInstruction::FENCE;
}

bool hasRegionBoundaryBefore(const MachineInstr &MI)
{
  return MI.getFlag(MachineInstr::MIFlag::HasRegionBoundaryBefore);
}

void removeRegionBoundaryBefore(MachineInstr &MI)
{
  MI.clearFlag(MachineInstr::MIFlag::HasRegionBoundaryBefore);
}

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
