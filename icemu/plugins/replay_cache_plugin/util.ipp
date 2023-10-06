static const char *registerNameFriendly(ArchitectureRiscv32::Register reg) {
  switch (reg) {
    case ArchitectureRiscv32::REG_X0: return "zero";
    case ArchitectureRiscv32::REG_X1: return "ra";
    case ArchitectureRiscv32::REG_X2: return "sp";
    case ArchitectureRiscv32::REG_X3: return "gp";
    case ArchitectureRiscv32::REG_X4: return "tp";
    case ArchitectureRiscv32::REG_X5: return "t0";
    case ArchitectureRiscv32::REG_X6: return "t1";
    case ArchitectureRiscv32::REG_X7: return "t2";
    case ArchitectureRiscv32::REG_X8: return "s0";
    case ArchitectureRiscv32::REG_X9: return "s1";
    case ArchitectureRiscv32::REG_X10: return "a0";
    case ArchitectureRiscv32::REG_X11: return "a1";
    case ArchitectureRiscv32::REG_X12: return "a2";
    case ArchitectureRiscv32::REG_X13: return "a3";
    case ArchitectureRiscv32::REG_X14: return "a4";
    case ArchitectureRiscv32::REG_X15: return "a5";
    case ArchitectureRiscv32::REG_X16: return "a6";
    case ArchitectureRiscv32::REG_X17: return "a7";
    case ArchitectureRiscv32::REG_X18: return "s2";
    case ArchitectureRiscv32::REG_X19: return "s3";
    case ArchitectureRiscv32::REG_X20: return "s4";
    case ArchitectureRiscv32::REG_X21: return "s5";
    case ArchitectureRiscv32::REG_X22: return "s6";
    case ArchitectureRiscv32::REG_X23: return "s7";
    case ArchitectureRiscv32::REG_X24: return "s8";
    case ArchitectureRiscv32::REG_X25: return "s9";
    case ArchitectureRiscv32::REG_X26: return "s10";
    case ArchitectureRiscv32::REG_X27: return "s11";
    case ArchitectureRiscv32::REG_X28: return "t3";
    case ArchitectureRiscv32::REG_X29: return "t4";
    case ArchitectureRiscv32::REG_X30: return "t5";
    case ArchitectureRiscv32::REG_X31: return "t6";
    default: return "unknown";
  }
}

// Map the disassembled capstone register to ICEmu
static ArchitectureRiscv32::Register fromCapstone(unsigned int capstone_reg) {
  // Only map general purpose registers for now
  ArchitectureRiscv32::Register reg;
  switch (capstone_reg) {
    case RISCV_REG_X0: reg = ArchitectureRiscv32::REG_X0; break;
    case RISCV_REG_X1: reg = ArchitectureRiscv32::REG_X1; break;
    case RISCV_REG_X2: reg = ArchitectureRiscv32::REG_X2; break;
    case RISCV_REG_X3: reg = ArchitectureRiscv32::REG_X3; break;
    case RISCV_REG_X4: reg = ArchitectureRiscv32::REG_X4; break;
    case RISCV_REG_X5: reg = ArchitectureRiscv32::REG_X5; break;
    case RISCV_REG_X6: reg = ArchitectureRiscv32::REG_X6; break;
    case RISCV_REG_X7: reg = ArchitectureRiscv32::REG_X7; break;
    case RISCV_REG_X8: reg = ArchitectureRiscv32::REG_X8; break;
    case RISCV_REG_X9: reg = ArchitectureRiscv32::REG_X9; break;
    case RISCV_REG_X10: reg = ArchitectureRiscv32::REG_X10; break;
    case RISCV_REG_X11: reg = ArchitectureRiscv32::REG_X11; break;
    case RISCV_REG_X12: reg = ArchitectureRiscv32::REG_X12; break;
    case RISCV_REG_X13: reg = ArchitectureRiscv32::REG_X13; break;
    case RISCV_REG_X14: reg = ArchitectureRiscv32::REG_X14; break;
    case RISCV_REG_X15: reg = ArchitectureRiscv32::REG_X15; break;
    case RISCV_REG_X16: reg = ArchitectureRiscv32::REG_X16; break;
    case RISCV_REG_X17: reg = ArchitectureRiscv32::REG_X17; break;
    case RISCV_REG_X18: reg = ArchitectureRiscv32::REG_X18; break;
    case RISCV_REG_X19: reg = ArchitectureRiscv32::REG_X19; break;
    case RISCV_REG_X20: reg = ArchitectureRiscv32::REG_X20; break;
    case RISCV_REG_X21: reg = ArchitectureRiscv32::REG_X21; break;
    case RISCV_REG_X22: reg = ArchitectureRiscv32::REG_X22; break;
    case RISCV_REG_X23: reg = ArchitectureRiscv32::REG_X23; break;
    case RISCV_REG_X24: reg = ArchitectureRiscv32::REG_X24; break;
    case RISCV_REG_X25: reg = ArchitectureRiscv32::REG_X25; break;
    case RISCV_REG_X26: reg = ArchitectureRiscv32::REG_X26; break;
    case RISCV_REG_X27: reg = ArchitectureRiscv32::REG_X27; break;
    case RISCV_REG_X28: reg = ArchitectureRiscv32::REG_X28; break;
    case RISCV_REG_X29: reg = ArchitectureRiscv32::REG_X29; break;
    case RISCV_REG_X30: reg = ArchitectureRiscv32::REG_X30; break;
    case RISCV_REG_X31: reg = ArchitectureRiscv32::REG_X31; break;
    default:
      assert(false && "ReplayCache: Unknown register mapping, fixme");
      break;
  }
  return reg;
}

struct StoreParsed {
  _Arch::Register r_src;
  _Arch::Register r_base;
  int64_t offset = 0;
  address_t size = 0;

  static StoreParsed parse(const insn_t &insn) {
    StoreParsed store;

    assert(insn->detail->riscv.op_count >= 2);

    const auto o_src = insn->detail->riscv.operands[0];
    assert(o_src.type == RISCV_OP_REG);
    store.r_src = fromCapstone(o_src.reg);

    if (insn->id == RISCV_INS_SW || insn->id == RISCV_INS_SH || insn->id == RISCV_INS_SB) {
      assert(insn->detail->riscv.op_count == 2);

      const auto o_mem = insn->detail->riscv.operands[1];
      assert(o_mem.type == RISCV_OP_MEM);

      store.r_base = fromCapstone(o_mem.mem.base);
      store.offset = o_mem.mem.disp;
      if (insn->id == RISCV_INS_SW) {
        store.size = 4;
      } else if (insn->id == RISCV_INS_SH) {
        store.size = 2;
      } else if (insn->id == RISCV_INS_SB) {
        store.size = 1;
      }
    } else if (insn->id == RISCV_INS_C_SW || insn->id == RISCV_INS_C_SWSP) {
      assert(insn->detail->riscv.op_count == 3);

      const auto o_offset = insn->detail->riscv.operands[1];
      const auto o_base = insn->detail->riscv.operands[2];
      assert(o_base.type == RISCV_OP_REG);
      assert(o_offset.type == RISCV_OP_IMM);

      store.r_base = fromCapstone(o_base.reg);
      store.offset = o_offset.imm;
      store.size = 4;
    } else {
      assert(false && "invalid store instruction to decode");
    }

    assert(store.size > 0);

    return store;
  }
};
