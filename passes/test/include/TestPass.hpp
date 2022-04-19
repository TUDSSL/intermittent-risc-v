#pragma once

#include "noelle/core/Noelle.hpp"

namespace TestPassNS {

class TestPass : public ModulePass {
public:
  static char ID;

  TestPass();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  std::string prefixString;

  bool run(Noelle &noelle, Module &M);
};

} // namespace TestPassNS
