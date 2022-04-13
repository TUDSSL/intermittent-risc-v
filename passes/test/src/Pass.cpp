#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <algorithm>

#include "TestPass.hpp"

using namespace llvm::noelle ;

namespace timesqueezer {

static cl::opt<int> MaximumFunctionSize("bs-max-function", cl::ZeroOrMore, cl::Hidden, cl::desc("Maximum number of instructions for each function"));
static cl::opt<int> MaximumLoopSize("bs-max-loop", cl::ZeroOrMore, cl::Hidden, cl::desc("Maximum number of instructions for each loop"));
static cl::opt<int> MaximumLoopUnrollFactor("bs-max-loop-unroll", cl::ZeroOrMore, cl::Hidden, cl::desc("Maximum loop unroll factor"));

bool TestPass::doInitialization (Module &M) {

  /*
   * Check the options.
   */
  auto maxFunc = MaximumFunctionSize.getValue();
  if (maxFunc != 0){
    this->maxFunctionSize = maxFunc;
  }
  auto maxLoop = MaximumLoopSize.getValue();
  if (maxLoop != 0){
    this->maxLoopSize = maxLoop;
  }
  auto maxLoopUnroll = MaximumLoopUnrollFactor.getValue();
  if (maxLoopUnroll != 0){
    this->maxUnrollFactor = maxLoopUnroll;
  }

  return false;
}

bool TestPass::runOnModule (Module &M) {

  /*
   * Fetch NOELLE
   */
  auto& noelle = getAnalysis<Noelle>();
  errs() << prefixString << "The program has " << noelle.numberOfProgramInstructions() << " instructions\n";

  /*
   * Expand
   */
  auto modified = this->run(noelle, M);

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

}
