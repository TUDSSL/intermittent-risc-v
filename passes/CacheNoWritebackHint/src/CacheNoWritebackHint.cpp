#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "CacheNoWritebackHint.hpp"
#include "Configurations.hpp"
#include "PassUtils.hpp"

using namespace llvm::noelle ;

namespace CacheNoWritebackHintNS {

CacheNoWritebackHint::CacheNoWritebackHint() :
  ModulePass(ID),
  prefixString{"CacheNoWritebackHint: "}
{
  return ;
}

// Collect instructions that need to be instrumented
CacheNoWritebackHint::CandidatesTy CacheNoWritebackHint::analyze(Noelle &N,
                                                                   Module &M) {
  CandidatesTy InstructionsToInstrument;

  auto FM = N.getFunctionsManager();
  auto PCF = FM->getProgramCallGraph();

  // Go through all the functions
  for (auto Node : PCF->getFunctionNodes()) {
    Function *F = Node->getFunction();
    assert(F != nullptr);
    if (F->isIntrinsic())
      continue;

    // Analyze the function
    analyzeFunction(N, *F, InstructionsToInstrument);
  }

  return InstructionsToInstrument;
}

void CacheNoWritebackHint::analyzeFunction(
    Noelle &N, Function &F, CandidatesTy &Candidates) {
  auto FunctionName = F.getName();
  dbg() << "Analyzing function: " << FunctionName << "\n";

  if (FunctionName == "__cache_hint") {
    dbg() << "Skipping function\n";
    return;
  }

  // Get the first non-phy instruction
  auto *InsertInstruction = F.getEntryBlock().getFirstNonPHIOrDbg();
  Candidates.push_back(InsertInstruction);
}

void CacheNoWritebackHint::Instrument(Noelle &N, Module &M, CandidatesTy &Candidates) {
  // Get the function declaration we want to insertcwinsert_function
  std::string InsertFunctionName = "__cache_hint";
  Function *InsertFunction = PassUtils::GetMethod(&M, InsertFunctionName);
  FunctionType *InsertFunctionType = InsertFunction->getFunctionType();
  FunctionCallee InsertFunctionCallee = M.getOrInsertFunction(InsertFunction->getName(), InsertFunctionType);
  Value *InsertFunctionValue = InsertFunctionCallee.getCallee();

  // Iterate over all the candidates
  for (auto &C : Candidates) {
    dbg() << "Instrumenting candidate: " << *C << "\n";

    // Get a builder to insert the function
    auto *BB = C->getParent(); 
    auto *F = BB->getParent();
    auto Builder = PassUtils::GetBuilder(F, BB);
    Builder.SetInsertPoint(C);

    // Insert the call
    CallInst *CI = Builder.CreateCall(InsertFunctionValue); // Create the function call
    CI->setCallingConv(InsertFunction->getCallingConv());
  }
}

bool CacheNoWritebackHint::run(Noelle &N, Module &M) {
  // Get the candidates
  CandidatesTy Candidates = analyze(N, M);

  // Instrument the candidates
  Instrument(N, M, Candidates);

  return true;
}

/*
 * Pass Registration
 */

bool CacheNoWritebackHint::doInitialization (Module &M) {
  /*
   * Check the options.
   */
  return false;
}

bool CacheNoWritebackHint::runOnModule(Module &M) {

  /*
   * Fetch NOELLE
   */
  auto &N = getAnalysis<Noelle>();
  errs() << prefixString << "The program has "
         << N.numberOfProgramInstructions() << " instructions\n";

  /*
   * Apply the transformation
   */
  auto modified = this->run(N, M);

  /*
   * Verify
   */
  if (NoVerify == false) {
    PassUtils::Verify(M);
  }

  return modified;
}

void CacheNoWritebackHint::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<Noelle>();
}

// Next there is code to register your pass to "opt"
char CacheNoWritebackHint::ID = 0;
static RegisterPass<CacheNoWritebackHint> X("cache-no-writeback-hint", "Test pass");

// Next there is code to register your pass to "clang"
static CacheNoWritebackHint * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CacheNoWritebackHint());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CacheNoWritebackHint()); }}); // ** for -O0

} // namespace CacheNoWritebackHintNS
