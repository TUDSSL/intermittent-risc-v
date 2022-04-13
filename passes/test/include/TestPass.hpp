#pragma once

#include "noelle/core/Noelle.hpp"

namespace timesqueezer {

class TestPass : public ModulePass {
 public:
  static char ID;

  TestPass();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

 private:
  std::string prefixString;
  std::uint64_t maxLoopSize;
  std::uint64_t maxFunctionSize;
  std::uint64_t maxUnrollFactor;

  bool run(Noelle &noelle, Module &M);
};

}  // namespace timesqueezer
