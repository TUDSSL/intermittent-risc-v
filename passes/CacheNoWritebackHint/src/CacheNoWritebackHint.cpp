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

#if 0
// Collect instructions that need to be instrumented
CacheNoWritebackHint::CandidatesTy
CacheNoWritebackHint::analyze(Noelle &N, DependencyAnalysis &DA, Module &M) {
  CandidatesTy Candidates;

  auto FM = N.getFunctionsManager();
  auto PCF = FM->getProgramCallGraph();

  // Go through all the functions
  for (auto Node : PCF->getFunctionNodes()) {
    Function *F = Node->getFunction();
    assert(F != nullptr);
    if (F->isIntrinsic())
      continue;

    // Analyze the function
    analyzeFunction(N, DA, *F, Candidates);
  }

  return Candidates;
}
#endif

CacheNoWritebackHint::CandidatesTy
CacheNoWritebackHint::analyzeFunction(Noelle &N, DependencyAnalysis &DA, Function &F) {
  CandidatesTy Candidates;

  auto FunctionName = F.getName();
  dbg() << "Analyzing function: " << FunctionName << "\n";

  if (FunctionName == "__cache_hint") {
    dbg() << "Skipping function\n";
    return Candidates;
  }

  if (FunctionName != "main") {
    dbg() << "Skipping function\n";
    return Candidates;
  }

  // Get the Reach analysis for the function
  auto DFA = N.getDataFlowAnalyses();
  auto DFR = DFA.runReachableAnalysis(&F);

  // Find all the read instructions
  for (auto &BB : F) {
    for (auto &I : BB) {
      analyzeInstruction(N, DA, *DFR, I, Candidates);
    }
  }

  return Candidates;
}

void CacheNoWritebackHint::analyzeInstruction(Noelle &N,
                                              DependencyAnalysis &DA,
                                              DataFlowResult &DFReach,
                                              Instruction &I,
                                              CandidatesTy &Candidates) {
  errs() << "Analyzing instruction: " << I << "\n";
  if (isa<LoadInst>(&I)) {

    CandidateTy C;
    C.I = &I;

    // Load instruction
    dbgs() << " found candidate instruction: " << I << "\n";
 
    // Get the candidate RAR dependencies
    auto rar_deps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_RAR, &I);
    if (rar_deps != nullptr) {
      dbgs() << "  candidate RAR dependencies:\n";
      for (auto dep : *rar_deps) {
        dbgs() << "   -> Dependent Instruction: " << *dep.Instruction 
               << " type: " << ((dep.IsMust) ? "Must" : "May") << "\n";

        // If MAY or MUST we add it to the candidate
        C.RARs.insert(dep.Instruction);
      }
    } else {
      dbgs() << "  candidate has no RAR dependencies\n";
    }
 
    // Get the candidate WAR dependencies
    auto war_deps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, &I);
    if (war_deps != nullptr) {
      dbgs() << "  candidate WAR dependencies:\n";
      for (auto dep : *war_deps) {
        dbgs() << "   -> Dependent Instruction: " << *dep.Instruction 
               << " type: " << ((dep.IsMust) ? "Must" : "May") << "\n";

        // If MUST we add it to the candidate
        if (dep.IsMust)
          C.WARs.insert(dep.Instruction);
      }
    } else {
      dbgs() << "  candidate has no WAR dependencies\n";
    }

    // Instructions reachable by the WAR writes
    set<Value *> WARWriteReach;
    for (auto &Store : C.WARs) {
      auto &ReachedByWarWrite = DFReach.OUT(Store);
      for (auto R : ReachedByWarWrite) {
        WARWriteReach.insert(R); // Add the set
      }
    }

    // Instructions reachable by the RAR reads
    set<Value *> RARReadReach;
    for (auto &Load : C.RARs) {
      auto &ReachedByRarRead = DFReach.OUT(Load);
      for (auto R : ReachedByRarRead) {
        RARReadReach.insert(R); // Add the set
      }
    }

    // Instructions reachable by the Candidate instruction (Load)
    auto &CandidateReach = DFReach.OUT(&I);
    
    // Instructions that are candidates (a subset of CandidateReach)
    set<Instruction *> CandidateInstructions;

    // Contains helper
    auto contains = [](set<Value *> ValueSet, Value *Instr) {
      if (ValueSet.find(Instr) != ValueSet.end()) {
        return true;
      } else {
        return false;
      }
    };

    // Go through the candidates
    dbgs() << "  candidate Reachable instructions:\n";
    for (auto CandidateInstr : CandidateReach) {
      dbgs() << "    analyzing hint location for candidate: " << *CandidateInstr << "\n";

      // If it is in the RARReadReach, then it's not a candidate (it's after a RAR read)
      if (contains(RARReadReach, CandidateInstr)) {
        dbgs() << "      NO_HINT_LOC: Candidate is in the RARReachReads\n";
        continue;
      }

      // If it is in the WARWriteReach, then it's not a candidate (it's after a WAR write)
      if (contains(WARWriteReach, CandidateInstr)) {
        dbgs() << "      NO_HINT_LOC: Candidate is in the WARWriteReach\n";
        continue;
      }

      // If any reachable instructions FROM THIS CANDIDATE can reach any RAR read, 
      // then it's not a candidate (it can lead to the RAR read)
      auto &ReachableByCandidate = DFReach.OUT(dyn_cast<Instruction>(CandidateInstr));
      bool CanReachRARRead = false;
      for (auto &RARRead : C.RARs) {
        if (contains(ReachableByCandidate, RARRead)) {
          CanReachRARRead = true;
          break;
        }
      }
      if (CanReachRARRead) {
        dbgs() << "      NO_HINT_LOC: Candidate can reach a RAR read\n";
        continue;
      }

      // Otherwise, it's a candidate
      dbgs() << "      HINT_LOC: Valid hint location!\n";
    }

#if 0
    dbgs() << "  candidate Reachable instructions:\n";
    auto &ReachedByCandidate = DFReach.OUT(&I);
    for (auto CandidateInstr : ReachedByCandidate) {
      if (isa<IntrinsicInst>(CandidateInstr)) continue;

      dbgs() << "   -> Reaches: " << *CandidateInstr << "\n";


      bool IsC = true;
      auto &CandidateReaches = DFReach.OUT(dyn_cast<Instruction>(CandidateInstr));

      // If a RAR read is reachable, it is not a candidate
      if (RARReadReach.find(CandidateInstr) != RARReadReach.end()) {
        dbgs() << "   candidate can reach an instruction reached by a RAR read\n";
        IsC = false;
      }

      // If any of the instructions reachable by the WAR write can be reached,
      // it is not a candidate
      if (WARWriteReach.find(CandidateInstr) != WARWriteReach.end()) {
        dbgs() << "   candidate can reach an instruction reached by a WAR write\n";
        IsC = false;
      }

      for (auto ReachInstr : CandidateReaches) {
        Instruction *ri = dyn_cast<Instruction>(ReachInstr);
        if (isa<ReturnInst>(ri)) continue;

        dbgs() << "     -> Reaches: " << *ri << "\n";

        // If a RAR read is reachable, it is not a candidate
        if (RARReadReach.find(ri) != RARReadReach.end()) {
          dbgs() << "   candidate can reach an instruction reached by a RAR read\n";
          IsC = false;
          break;
        }

        // If any of the instructions reachable by the WAR write can be reached,
        // it is not a candidate
        if (WARWriteReach.find(ri) != WARWriteReach.end()) {
          dbgs() << "   candidate can reach an instruction reached by a WAR write\n";
          IsC = false;
          break;
        }
      }
      if (IsC)
        CandidateInstructions.insert(dyn_cast<Instruction>(CandidateInstr));
    }

    for (auto &CI : CandidateInstructions) {
        dbgs() << "$ CANDIDATE: insertion point" << *CI << "\n" ;
    }

    // Add the candidate
    Candidates.push_back(C);

    dbgs() << "\n";
#endif

#if 0
    // Get the candidate dependencies
    auto rar_deps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_RAR, &I);
    if (rar_deps != nullptr) {
      dbgs() << "  candidate RAR dependencies:\n";
      for (auto dep : *rar_deps) {
        dbgs() << "   -> Dependent Instruction: " << *dep.Instruction 
               << " type: " << ((dep.IsMust) ? "Must" : "May") << "\n";
      }
    } else {
      dbgs() << "  candidate has no dependencies\n";
    }

    // Get the candidate dependencies
    auto war_deps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, &I);
    if (war_deps != nullptr) {
      dbgs() << "  candidate WAR dependencies:\n";
      for (auto dep : *war_deps) {
        dbgs() << "   -> Dependent Instruction: " << *dep.Instruction 
               << " type: " << ((dep.IsMust) ? "Must" : "May") << "\n";
      }
    } else {
      dbgs() << "  candidate has no dependencies\n";
    }
#endif
  }
}

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
    dbg() << "Instrumenting candidate: " << *C.I << "\n";

    // Insert the call
    insertHintFunctionCall(N, M, "__cache_hint", C.I);
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
    CandidatesTy AnalyzedCandidates = analyzeCandidates(N, DA, Candidates);

    // Instrument the candidates
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
