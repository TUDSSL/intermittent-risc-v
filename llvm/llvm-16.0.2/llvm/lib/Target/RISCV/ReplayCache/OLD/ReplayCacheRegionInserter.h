// #ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGIONINSERTER_H
// #define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGIONINSERTER_H

// #include "RISCV.h"
// #include "llvm/CodeGen/MachineFunctionPass.h"

// namespace llvm {
// class ReplayCacheRegionInserter : public MachineFunctionPass {
// public:
//   static char ID;

//   ReplayCacheRegionInserter() : MachineFunctionPass(ID) {}

// private:
//   bool runOnMachineFunction(MachineFunction &MF) override;
// };
// } // namespace llvm

// #endif
#ifndef LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGIONINSERTER_H
#define LLVM_LIB_TARGET_RISCV_REPLAYCACHE_REPLAYCACHEREGIONINSERTER_H

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/SlotIndexes.h"

namespace llvm {
class ReplayCacheRegionInserter : public MachineFunctionPass {
public:
  static char ID;

  ReplayCacheRegionInserter() : MachineFunctionPass(ID) {}

private:
  SlotIndexes *SLIS = nullptr;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // namespace llvm

#endif
