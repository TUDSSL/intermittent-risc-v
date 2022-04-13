#include "TestPass.hpp"

namespace timesqueezer {

TestPass::TestPass() :
    ModulePass(ID)
  , prefixString{"TestPass: "}
  , maxLoopSize{800000}
  , maxFunctionSize{5000}
  , maxUnrollFactor{100}
{
  return ;
}

bool TestPass::run (
  Noelle &noelle,
  Module &M
  ){
  auto modified = false;

  // Test the idea of inlining functions that occure only N times and take
  // at least one pointer argument
  auto FM = noelle.getFunctionsManager();
  auto PCF = FM->getProgramCallGraph();

  set<Function *> Candidates;
  set<Function *> NotCandidates;

  for (auto Node : PCF->getFunctionNodes()) {
    Function *F = Node->getFunction();
    assert(F != nullptr);

    errs() << "\nFunction: " << F->getName() << "\n";

  }

  modified = false;
  return modified;
}


}
