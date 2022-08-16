#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "CacheNoWritebackHint.hpp"
#include "Configurations.hpp"
#include "PassUtils.hpp"

#include "SetManipulation.hpp"

using namespace llvm::noelle;

namespace CacheNoWritebackHintNS {

CacheNoWritebackHint::CacheNoWritebackHint() :
  ModulePass(ID),
  prefixString{"CacheNoWritebackHint: "}
{
  return ;
}

CacheNoWritebackHint::CandidatesTy
CacheNoWritebackHint::analyzeFunction(Noelle &N, DependencyAnalysis &DA, Function &F) {
  CandidatesTy Candidates;

  if (F.isIntrinsic())
    return Candidates;

  auto FunctionName = F.getName();
  dbgs() << "Analyzing function: " << FunctionName << "\n";

  if (F.getInstructionCount() == 0)
    return Candidates;

  //if (F.hasExternalLinkage()) {
  //  dbgs() << "External\n";
  //  return Candidates;
  //}

  if (FunctionName == "__cache_hint") {
    dbgs() << "Skipping function\n";
    return Candidates;
  }

  // Get the Reach analysis for the function
  auto DFA = N.getDataFlowAnalyses();
  auto DFR = DFA.runReachableAnalysis(&F);

  // Get the Dominator Tree analysis
  auto DT = N.getDominators(&F)->DT;

  //struct WAR {
  //  Instruction *Read;
  //  Instruction *Write;
  //};

  // Get all the WAR violations in the function
  vector<WAR> WARs;
  for (auto &I : instructions(F)) {
    auto WARDeps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, &I);
    if (WARDeps == nullptr) continue;
    set<Instruction *> WARWrites;
    for (auto &WARDep : *WARDeps) {
      if (WARDep.IsMust) {
        WARs.push_back(WAR{.Read = &I, .Write = WARDep.Instruction});
      }
    }
  }

  dbgs() << "WARs in function:\n";
  for (auto &WAR : WARs) {
    dbgs() << "WAR:\n  read: " << *WAR.Read << "\n  write: " << *WAR.Write << "\n";
  }

  // Create the DFA
  auto DFE = N.getDataFlowEngine();
  
  auto CustomDFA = writeAfterReadReachDFA(DFE, F, WARs);

  dbgs() << "\n";
  for (auto &I : instructions(F)) {
    dbgs() << "Instruction: " << I << "\n";
    auto IOUT = CustomDFA->OUT(&I);
    dbgs() << " OUT:\n";
    for (auto &II : IOUT) {
      dbgs() << "  " << *II << "\n";
    }
  }

  return Candidates;
}


void CacheNoWritebackHint::insertHintFunctionCall(Noelle &N, Module &M,
                                                  std::string FunctionName,
                                                  Instruction *I, Instruction *HintLocation) {

  // Greate the builder
  auto *BB = I->getParent();
  auto *F = BB->getParent();
  auto Builder = PassUtils::GetBuilder(F, BB);
  Builder.SetInsertPoint(HintLocation);

  // Get the context
  LLVMContext &Ctx = F->getContext();

  FunctionCallee InsertFunctionCallee =
    M.getOrInsertFunction("__cache_hint", Type::getVoidTy(Ctx), Type::getInt8PtrTy(Ctx));

  // Get the function
  //Function *InsertFunction = PassUtils::GetMethod(&M, FunctionName);
  //assert(!!InsertFunction &&
  //       "CacheNoWritebackHint: Can't find function");
  //FunctionType *InsertFunctionType = InsertFunction->getFunctionType();

  //FunctionCallee InsertFunctionCallee =
  //    M.getOrInsertFunction(InsertFunction->getName(), InsertFunctionType);
  Value *InsertFunctionValue = InsertFunctionCallee.getCallee();


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
  CI->setCallingConv(F->getCallingConv());
}

void CacheNoWritebackHint::Instrument(Noelle &N, Module &M, CandidatesTy &Candidates) {
  // Iterate over all the candidates
  for (auto &C : Candidates) {
    dbg() << "Instrumenting candidate: " << *C.I << "\n";

    for (auto &H : C.SelectedHintLocations) {
      dbg() << "  Inserting hint before: " << *H << "\n";
      // Insert the call
      insertHintFunctionCall(N, M, "__cache_hint", C.I, H);
    }
  }
}

bool CacheNoWritebackHint::run(Noelle &N, Module &M) {
  auto FM = N.getFunctionsManager();
  auto PCF = FM->getProgramCallGraph();

  // Analyze all the instruction dependencies
  DependencyAnalysis DA = DependencyAnalysis(N);

  // Debug
  dbgs() << "#WARs: " << DA.getInstructionDependenciesTo().WARs.size() << "\n";

  // Go through all the functions
  for (auto Node : PCF->getFunctionNodes()) {
    Function *F = Node->getFunction();
    assert(F != nullptr);
    if (F->isIntrinsic())
      continue;

    // Analyze the function
    CandidatesTy Candidates = analyzeFunction(N, DA, *F);

    // Print the candidates and possible locations
    dbgs() << "CacheNoWritebackHint candidates for function: " << F->getName() << "\n";
    for (auto &C : Candidates) {
      dbgs() << "  Candidate instruction: " << *C.I << "\n";
      dbgs() << "    Possible hint locations: \n";
      for (auto &PHL : C.PossibleHintLocations) {
        dbgs() << "    " << *PHL << "\n";
      }
      dbgs() << "    Selected hint locations: \n";
      for (auto &SHL : C.SelectedHintLocations) {
        dbgs() << "    " << *SHL << "\n";
      }
    }

    // Instrument the 
    Instrument(N, M, Candidates);
  }

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

  // For unit tests
  dbgs() << "#instructions: " << N.numberOfProgramInstructions() << "\n";

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
