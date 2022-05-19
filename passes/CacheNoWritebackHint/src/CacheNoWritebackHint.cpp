#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "CacheNoWritebackHint.hpp"
#include "Configurations.hpp"
#include "PassUtils.hpp"

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

  auto FunctionName = F.getName();
  dbg() << "Analyzing function: " << FunctionName << "\n";

  if (FunctionName == "__cache_hint") {
    dbg() << "Skipping function\n";
    return Candidates;
  }

#if 0
  if (FunctionName != "main") {
    dbg() << "Skipping function\n";
    return Candidates;
  }
#endif

  // Get the Reach analysis for the function
  auto DFA = N.getDataFlowAnalyses();
  auto DFR = DFA.runReachableAnalysis(&F);

  // Find all the read instructions
  for (auto &BB : F) {
    for (auto &I : BB) {
      auto [IsCandidate, Candidate] = analyzeInstruction(N, DA, *DFR, I);
      if (IsCandidate) {
        Candidates.push_back(Candidate);
      }
    }
  }

  return Candidates;
}

std::tuple<bool, CacheNoWritebackHint::CandidateTy>
CacheNoWritebackHint::analyzeInstruction(Noelle &N, DependencyAnalysis &DA,
                                         DataFlowResult &DFReach,
                                         Instruction &I) {

  // Only Load instructions can be a candidate
  if (!isa<LoadInst>(&I))
    return {false, CandidateTy{}};

  // Load instruction
  dbgs() << "Analyzing possible candidate instruction: " << I << "\n";

  // Get the candidate RAR dependencies
  set<Instruction *> RARs;
  auto RARDeps =
      DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_RAR, &I);
  if (RARDeps != nullptr) {
    dbgs() << "  Candidate RAR dependencies:\n";
    for (auto Dep : *RARDeps) {
      dbgs() << "    " << Dep << "\n";

      // If MAY or MUST we add it to the candidate
      RARs.insert(Dep.Instruction);
    }
  } else {
    dbgs() << "  Candidate has no RAR dependencies\n";
  }

  // Get the candidate WAR dependencies
  set<Instruction *> WARs;
  auto WARDeps =
      DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, &I);
  if (WARDeps != nullptr) {
    dbgs() << "  candidate WAR dependencies:\n";
    for (auto Dep : *WARDeps) {
      dbgs() << "    " << Dep << "\n";

      // If MUST we add it to the candidate
      if (Dep.IsMust)
        WARs.insert(Dep.Instruction);
    }
  } else {
    dbgs() << "  Candidate has no WAR dependencies\n";
  }

  // Instructions reachable by the WAR writes
  set<Value *> WARWritesReach;
  for (auto &Store : WARs) {
    auto &ReachedByWarWrite = DFReach.OUT(Store);
    for (auto R : ReachedByWarWrite) {
      WARWritesReach.insert(R);
    }
  }

  // Instructions reachable by the RAR reads
  set<Value *> RARReadsReach;
  for (auto &Load : RARs) {
    auto &ReachedByRarRead = DFReach.OUT(Load);
    for (auto R : ReachedByRarRead) {
      RARReadsReach.insert(R);
    }
  }

  // Instructions reachable by the Candidate instruction (Load)
  auto &CandidateReach = DFReach.OUT(&I);

  // Instructions that are candidates (a subset of CandidateReach)
  set<Instruction *> CandidateHintLocations;

  // Contains helper
  auto contains = [](set<Value *> ValueSet, Value *InstrValue) {
    if (ValueSet.find(InstrValue) != ValueSet.end()) {
      return true;
    } else {
      return false;
    }
  };

  // The VALID possible hint locations
  HintLocationsTy PossibleHintLocations;

  // Go through the possible hint locations for the candidate
  dbgs() << "  Finding hint locations for candidate\n";
  for (auto &PossibleHintLocation : CandidateReach) {

    // Skip debug insructions
    if (isa<IntrinsicInst>(PossibleHintLocation))
      continue;

    dbgs() << "    Analyzing possible hint location: " << *PossibleHintLocation
           << "\n";

    // If it is a RAR read, then it's not a candidate
    bool IsRARRead = false;
    for (auto &Load : RARs) {
      if (Load == PossibleHintLocation) {
        IsRARRead = true;
        break;
      }
    }
    if (IsRARRead) {
      dbgs() << "      INVALID - Possible hint location is RAR read\n";
      continue;
    }

    // If it is in the RARReadReach, then it's not a candidate (it's after a RAR
    // read)
    if (contains(RARReadsReach, PossibleHintLocation)) {
      dbgs() << "      INVALID - Possible hint location is in the RARReachReads\n";
      continue;
    }

    // If it is in the WARWriteReach, then it's not a candidate (it's after a
    // WAR write)
    if (contains(WARWritesReach, PossibleHintLocation)) {
      dbgs() << "      INVALID - Possible hint location is in the WARWriteReach\n";
      continue;
    }

    // If ANY reachable instructions FROM THIS CANDIDATE can reach any RAR read,
    // then it's NOT a candidate (it can lead to the RAR read)
    auto &PossibleHintLocationReach =
        DFReach.OUT(dyn_cast<Instruction>(PossibleHintLocation));
    bool CanReachRARRead = false;
    for (auto &RARRead : RARs) {
      if (contains(PossibleHintLocationReach, RARRead)) {
        CanReachRARRead = true;
        break;
      }
    }
    if (CanReachRARRead) {
      dbgs() << "      INVALID - Possible hint location can reach a RAR read\n";
      continue;
    }

    // Otherwise, it's a candidate
    dbgs() << "      VALID - Valid hint location!\n";
    PossibleHintLocations.insert(dyn_cast<Instruction>(PossibleHintLocation));
  }

  // If the candidate instruction has NO possible hint locations, it's not a candidate
  if (PossibleHintLocations.size() == 0) {
    return {false, CandidateTy{}};
  }


  // The instruction I is a valid candidate, and has possible hint locations
  return {true,
          CandidateTy{.I = &I, .PossibleHintLocations = PossibleHintLocations}};
}

#if 0
CacheNoWritebackHint::CandidatesTy
CacheNoWritebackHint::analyzeCandidates(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates) {
  CandidatesTy AnalyzedCandidates;

  // Get reachable instructions


  for (auto &C : Candidates) {
    Instruction *I = C.I;
    // Get reachable instructions from candidate
    dbgs() << "Analyzing candidate: " << *I << "\n";

    // Get the candidate RARs
    auto rar_deps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_RAR, I);
    if (rar_deps != nullptr) {
      dbgs() << "  candidate RAR dependencies:\n";
      for (auto dep : *rar_deps) {
        dbgs() << "   -> Dependent Instruction: " << *dep.Instruction 
               << " type: " << ((dep.IsMust) ? "Must" : "May") << "\n";
      }
    } else {
      dbgs() << "  candidate has no dependencies\n";
    }
    
  }

  return AnalyzedCandidates;
}
#endif

void CacheNoWritebackHint::selectHintLocations(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates) {
  CandidatesTy HintLocations;

  for (auto &C : Candidates) {
    Instruction *I = C.I;
    // Get reachable instructions from candidate
    dbgs() << "Analyzing candidate: " << *I << "\n";

    // TODO: Select the best candidate
    Instruction *HI = *C.PossibleHintLocations.begin();
    dbgs() << "Selected instruction: " << *HI << "\n";

    C.SelectedHintLocations.insert(HI);
  }
}

void CacheNoWritebackHint::insertHintFunctionCall(Noelle &N, Module &M,
                                                  std::string FunctionName,
                                                  Instruction *I, Instruction *HintLocation) {

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
  Builder.SetInsertPoint(HintLocation);

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
    }

    // Find the ideal instructions to place hints
    selectHintLocations(N, DA, Candidates);

    //Instrument(N, M, Candidates);
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
