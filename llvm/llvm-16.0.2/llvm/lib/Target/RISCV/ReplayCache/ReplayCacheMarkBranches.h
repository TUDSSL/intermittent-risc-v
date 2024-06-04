#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEMARKBRANCHES_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEMARKBRANCHES_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include <vector>

namespace llvm {
class ReplayCacheMarkBranches : public MachineFunctionPass {
public:
  static char ID;

  std::vector<MachineBasicBlock*> MarkedBlocks;

  ReplayCacheMarkBranches() : MachineFunctionPass(ID) {}

private:
  void releaseMemory() override;
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // namespace llvm

#endif
