#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "TestPass.hpp"
#include "Configurations.hpp"
#include "PassUtils.hpp"

using namespace llvm::noelle ;

namespace TestPassNS {

TestPass::TestPass() :
  ModulePass(ID),
  prefixString{"TestPass: "}
{
  return ;
}

bool TestPass::run(Noelle &noelle, Module &M) {
  auto modified = false;

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

/*
 * Pass Registration
 */

bool TestPass::doInitialization (Module &M) {
  /*
   * Check the options.
   */
  return false;
}

bool TestPass::runOnModule(Module &M) {

  /*
   * Fetch NOELLE
   */
  auto &noelle = getAnalysis<Noelle>();
  errs() << prefixString << "The program has "
         << noelle.numberOfProgramInstructions() << " instructions\n";

  /*
   * Expand
   */
  auto modified = this->run(noelle, M);

  /*
   * Verify
   */
  if (NoVerify == false) {
    PassUtils::Verify(M);
  }

  return modified;
}

void TestPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<Noelle>();
}

// Next there is code to register your pass to "opt"
char TestPass::ID = 0;
static RegisterPass<TestPass> X("testpass", "Test pass");

// Next there is code to register your pass to "clang"
static TestPass * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new TestPass());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new TestPass()); }}); // ** for -O0

} // namespace TestPassNS
