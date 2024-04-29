#include "llvm/CodeGen/ReplayCache/ReplayCacheShared.h"
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"

void BuildRC(MachineBasicBlock &MBB, MachineInstr &MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  BuildMI(MBB, MI, DL, TII.get(RISCV::C_LI), RISCV::X31) // TODO: missing isIndirect?
      .addImm(Instr);
}

void BuildRC(MachineBasicBlock &MBB, MachineInstr *MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  BuildMI(MBB, MI, DL, TII.get(RISCV::C_LI), RISCV::X31) // TODO: missing isIndirect?
      .addImm(Instr);
}

void InsertStartRegionBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr)
{
  BuildRC(MBB, MI, MI.getDebugLoc(), Instr);
}

void InsertRegionBoundaryBefore(MachineBasicBlock &MBB, MachineInstr &MI, const ReplayCacheInstruction Instr)
{
  BuildRC(MBB, MI, MI.getDebugLoc(), ReplayCacheInstruction::FENCE);
  BuildRC(MBB, MI, MI.getDebugLoc(), Instr);
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
  return IsRC(MI) && (static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm()) <= ReplayCacheInstruction::START_REGION_BRANCH_DEST);
}
bool IsFence(const MachineInstr &MI)
{
  return IsRC(MI) && static_cast<ReplayCacheInstruction>(MI.getOperand(1).getImm()) == ReplayCacheInstruction::FENCE;
}

void StartRegionInBB(MachineBasicBlock &MBB, const ReplayCacheInstruction Instr) {
  auto FirstNonPHI = MBB.getFirstNonPHI();
  if (FirstNonPHI == MBB.instr_end())
    return;
  auto &EntryInstr = *FirstNonPHI;
  if (IsRC(EntryInstr))
    return;
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), FENCE);
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), Instr);
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
