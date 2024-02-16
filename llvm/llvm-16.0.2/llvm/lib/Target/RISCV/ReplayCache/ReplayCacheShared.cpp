#include "ReplayCacheShared.h"

void BuildRC(MachineBasicBlock &MBB, MachineInstr &MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  BuildMI(MBB, MI, DL, TII.get(RISCV::C_LI), RISCV::X31) // TODO: missing isIndirect?
      .addReg(RISCV::X31)
      .addImm(Instr);
}

void BuildRC(MachineBasicBlock &MBB, MachineInstr *MI,
                    const DebugLoc &DL, const ReplayCacheInstruction Instr) {
  auto &TII = *MBB.getParent()->getSubtarget().getInstrInfo();
  BuildMI(MBB, MI, DL, TII.get(RISCV::C_LI), RISCV::X31) // TODO: missing isIndirect?
      .addReg(RISCV::X31)
      .addImm(Instr);
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

void StartRegionInBB(MachineBasicBlock &MBB) {
  auto FirstNonPHI = MBB.getFirstNonPHI();
  if (FirstNonPHI == MBB.instr_end())
    return;
  auto &EntryInstr = *FirstNonPHI;
  if (IsRC(EntryInstr))
    return;
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), FENCE);
  BuildRC(MBB, EntryInstr, EntryInstr.getDebugLoc(), START_REGION);
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
