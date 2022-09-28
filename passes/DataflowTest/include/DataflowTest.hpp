#pragma once

#include "noelle/core/Noelle.hpp"

namespace DataflowTestNS {

class DataflowTest : public ModulePass {
public:
  static char ID;

  DataflowTest();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  void reachUse(Function *mainF);
  void postponableExpressions(Function *mainF);

private:
  std::string prefixString;

  bool run(Noelle &noelle, Module &M);
};

} // namespace DataflowTestNS
