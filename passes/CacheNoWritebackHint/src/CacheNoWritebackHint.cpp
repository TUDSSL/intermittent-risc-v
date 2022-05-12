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
CacheNoWritebackHint::CandidatesTy
CacheNoWritebackHint::analyze(Noelle &N, DependencyAnalysis &DA, Module &M) {
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
    analyzeFunction(N, DA, *F, InstructionsToInstrument);
  }

  return InstructionsToInstrument;
}

void CacheNoWritebackHint::analyzeFunction(Noelle &N, DependencyAnalysis &DA,
                                           Function &F,
                                           CandidatesTy &Candidates) {
  auto FunctionName = F.getName();
  dbg() << "Analyzing function: " << FunctionName << "\n";

  if (FunctionName == "__cache_hint") {
    dbg() << "Skipping function\n";
    return;
  }

  // Find all the read instructions
  for (auto &BB : F) {
    for (auto &I : BB) {
      analyzeInstruction(N, DA, I, Candidates);
    }
  }
}

void CacheNoWritebackHint::analyzeInstruction(Noelle &N, DependencyAnalysis &DA,
                                              Instruction &I,
                                              CandidatesTy &Candidates) {
  errs() << "Analyzing instruction: " << I << "\n";
  if (isa<LoadInst>(&I)) {
    // Load instruction
    dbgs() << " found candidate instruction: " << I << "\n";
    Candidates.push_back(&I);

    // Get the candidate dependencies
    auto war_deps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, &I);
    if (war_deps != nullptr) {
      dbgs() << "  candidate WAR dependencies:\n";
      for (auto dep : *war_deps) {
        dbgs() << " Dep instr: " << dep.Instruction << "\n";
      }
    } else {
      dbgs() << "  candidate has no dependencies\n";
    }
  }
}

void CacheNoWritebackHint::insertHintFunctionCall(Noelle &N, Module &M,
                                                  std::string FunctionName,
                                                  Instruction *I) {

  // Get the function
  Function *InsertFunction = PassUtils::GetMethod(&M, FunctionName);
  assert(!!InsertFunction &&
         "CacheNoWritebackHint: Can't find function");
  FunctionType *InsertFunctionType = InsertFunction->getFunctionType();
  FunctionCallee InsertFunctionCallee =
      M.getOrInsertFunction(InsertFunction->getName(), InsertFunctionType);
  Value *InsertFunctionValue = InsertFunctionCallee.getCallee();

  // Greate the builder
  auto *BB = I->getParent();
  auto *F = BB->getParent();
  auto Builder = PassUtils::GetBuilder(F, BB);
  Builder.SetInsertPoint(I);

  // Get the context
  LLVMContext &Ctx = F->getContext();

  // Get the Load source address
  LoadInst *Load = dyn_cast<LoadInst>(I);
  assert(Load != nullptr && "CacheNoWritebackHint: Expected LoadInst");
  Value *LoadPtr =  Load->getPointerOperand(); // Get the pointer (src address)

  // Configure the function arguments
  Value *LoadSrcCast = Builder.CreateBitOrPointerCast(LoadPtr, Type::getInt8PtrTy(Ctx));
  Value *InsertFunctionArgs[] = {LoadSrcCast};

  // Insert the function call
  CallInst *CI =
      Builder.CreateCall(InsertFunctionValue, InsertFunctionArgs); // Create the function call
  CI->setCallingConv(InsertFunction->getCallingConv());
}

void CacheNoWritebackHint::Instrument(Noelle &N, Module &M, CandidatesTy &Candidates) {
  // Iterate over all the candidates
  for (auto &C : Candidates) {
    dbg() << "Instrumenting candidate: " << *C << "\n";

    // Insert the call
    insertHintFunctionCall(N, M, "__cache_hint", C);
  }
}

bool CacheNoWritebackHint::run(Noelle &N, Module &M) {
  // Analyze all the instruction dependencies
  DependencyAnalysis DA = DependencyAnalysis(N);
  
  // Get the candidates
  CandidatesTy Candidates = analyze(N, DA, M);

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
