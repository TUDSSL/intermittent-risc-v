#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGISTERPRESERVATION_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGISTERPRESERVATION_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {
class ReplayCacheRegisterPreservation : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheRegisterPreservation() : MachineFunctionPass(ID) {}

private:
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace llvm

#endif
