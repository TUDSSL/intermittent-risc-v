#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "CacheNoWritebackHint.hpp"
#include "Configurations.hpp"
#include "PassUtils.hpp"

#include "WarReachDFA.hpp"

#  include "WPA/WPAPass.h"
#  include "Util/SVFModule.h"
#  include "Util/PTACallGraph.h"
#  include "WPA/Andersen.h"
#  include "MemoryModel/PointerAnalysis.h"
#  include "MSSA/MemSSA.h"

using namespace llvm::noelle;

namespace CacheNoWritebackHintNS {

CacheNoWritebackHint::CacheNoWritebackHint() :
  ModulePass(ID),
  prefixString{"CacheNoWritebackHint: "}
{
  return ;
}

CacheNoWritebackHint::InstructionHintsMapTy
CacheNoWritebackHint::analyzeFunction(Noelle &N, WPAPass &WPA, DependencyAnalysis &DA, Function &F) {
  InstructionHintsMapTy Candidates;

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

  auto NDFA = N.getDataFlowAnalyses();
  auto RA = NDFA.runReachableAnalysis(&F);

  // Get all the WAR violations in the function
  vector<WarReachDFA::WAR> WARs;
  for (auto &I : instructions(F)) {
    auto WARDeps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, &I);
    if (WARDeps == nullptr) continue;
    set<Instruction *> WARWrites;
    for (auto &WARDep : *WARDeps) {
      if (WARDep.IsMust) {
        WARs.push_back(WarReachDFA::WAR{.Read = &I, .Write = WARDep.Instruction});
      }
    }
  }

  dbgs() << "WARs in function:\n";
  for (auto &WAR : WARs) {
    dbgs() << "WAR:\n  read: " << *WAR.Read << "\n  write: " << *WAR.Write << "\n";
  }

  /*
   * Run the WarReachDFA
   *    Finds all chains that end in a Write.
   *    Meaning that, in those paths, we can forget the value from memory, as it MUST
   *    be written before being read (again, all paths end in a MUST write).
   *
   */

  // Create the data flow engine
  auto DFE = DataFlowEngineExt(); // Modified version that allows for intersections

  // Run the custom DFA
  auto warReachDFA = WarReachDFA();
  //auto CustomDFA = warReachDFA.apply(DFE, F, WARs);
  auto warReachDFAResult = warReachDFA.apply(DFE, F, DA);

  // Print the DFA output for debugging
  for (auto &I : instructions(F)) {
    auto IIN = warReachDFAResult->IN(&I);
    auto IOUT = warReachDFAResult->OUT(&I);

    // Add debug data to the instruction
    if (IIN.size()) {
      std::string in_string = "";
      for (auto &II : IIN) in_string += PassUtils::GetOperandString(*II) + " ";

      // Attach the metadata if it's not empty
      in_string.pop_back(); // Remove the last space if it's not empty
      PassUtils::SetInstrumentationMetadata(&I, in_string, "war-reach-dfa-IN");
    }

    // Printing
    dbgs() << "Instruction: " << I << "\n";
    dbgs() << " IN:\n";
    for (auto &II : IIN) dbgs() << "  " << *II << "\n";
    dbgs() << " OUT:\n";
    for (auto &II : IOUT) dbgs() << "  " << *II << "\n";
  }

  std::map<Instruction *, std::set<Value *>> instructionWarReachMap;
  std::map<Instruction *, std::set<Value *>> instructionReachableWarReachMap;
  std::map<Instruction *, std::set<Value *>> instructionHintMap;
  std::map<Instruction *, std::set<Value *>> noAliasInstructionHintMap;

  /*
   * Convert the DFA result to a map (to keep handling the same)
   * Also remove debug/intrinsic instructions
   */
  for (auto &I : instructions(F)) {
    if (isa<IntrinsicInst>(I)) continue;
    auto in_i = warReachDFAResult->IN(&I);
    instructionWarReachMap[&I].insert(in_i.begin(), in_i.end());
  }

  /*
   * Remove hint locations where the hint can not reach the hint location
   * i.e., the hint location if before the WAR read
   * (can happen with loop edges combined with the backwards WarReach DFA)
   */
  for (const auto & [I, hints] : instructionWarReachMap) {
    for (const auto &hint : hints) {
      // Can hint (i.e., WAR read) reach the hint location (i.e., I)
      auto hint_reach = RA->OUT(dyn_cast<Instruction>(hint));
      if (hint_reach.find(I) != hint_reach.end()) {
        // Hint location I is reachable from WAR Read, so we keep it
        instructionReachableWarReachMap[I].insert(hint);
      }
    }
  }

  /*
   * Select the best hint locations
   */
  for (const auto & [I, hints] : instructionReachableWarReachMap) {
    auto in_i = instructionWarReachMap[I];

    dbgs() << "I = " << *I << "\n";

    // Go through the hints for i
    for (const auto &hint : in_i) {
      bool best_hint = true;

      dbgs() << "  checking hint = " << *hint << "\n";
      // Go through the other instructions
      for (const auto & [II, _] : instructionWarReachMap) {
        if (I == II) continue; // Skip self compare
        dbgs() << "     checking in instruction = " << *II << "\n";

        auto other_hints = warReachDFAResult->IN(II);
        for (auto &other_hint : other_hints) {
          if (hint == other_hint) {
            dbgs() << "     found matching hint\n";
            // Found another location with the same hint
            // If our candidate hint is dominated by the other hint location
            // then there is a better hint, so we ignore this one
            if (DT.dominates(II, I)) {
              dbgs() << "     hint in:" << *II << " is better\n";
              //dbgs() << "II: " << II << " dominates I: " << I << "\n";
              best_hint = false;
            }
          }
        }
      }

      if (best_hint) {
        dbgs() << "Instruction: " << *I << "\n   has the best hint location for: " << *hint << "\n";
        instructionHintMap[I].insert(hint);
      }
    }
  }

  // Remove hints on the same line that MUST alias
  // Best would be to pick the closest one, but for now just pick one
  for (const auto & [inst, hints] : instructionHintMap) {
    if (hints.size() == 0) continue;

    std::vector<Value *> possible_hints(hints.begin(), hints.end());
    while (possible_hints.size()) {

      Value *hint = possible_hints.back();
      possible_hints.pop_back();

      // Add the hint to the map without aliases
      noAliasInstructionHintMap[inst].insert(hint);

      // Check if the selected hint aliases with any other hint
      // if so, we can also remove that hint
      auto mem_hint = MemoryLocation::get(dyn_cast<Instruction>(hint));

      auto it = possible_hints.begin();
      while (it != possible_hints.end()) {

        auto mem_other_hint = MemoryLocation::get(dyn_cast<Instruction>(*it));
        auto alias_res = WPA.alias(mem_hint, mem_other_hint);

        // If it MUST alias we know for sure we don't need the other hint
        // TODO: We can choose to also remove on May and see how if affects the result
        //if (alias_res == AliasResult::MustAlias) {
        if (alias_res == AliasResult::MustAlias || alias_res == AliasResult::MayAlias) {
          it = possible_hints.erase(it); // remove this element
        } else {
          ++it; // Next element
        }
      }
    }
  }

  // Add metadata for instructionHintMap
  for (const auto & [inst, hints] : instructionHintMap) {
    if (hints.size()) {
      std::string debug_string = "";
      for (const auto &hint : hints) debug_string += PassUtils::GetOperandString(*hint) + " ";

      // Attach the metadata
      debug_string.pop_back();
      PassUtils::SetInstrumentationMetadata(inst, debug_string, "hint-location");
    }
  }

  // Add metadata for noAliasInstructionHintMap
  for (const auto & [inst, hints] : noAliasInstructionHintMap) {
    if (hints.size()) {
      std::string debug_string = "";
      for (const auto &hint : hints) debug_string += PassUtils::GetOperandString(*hint) + " ";

      // Attach the metadata
      debug_string.pop_back();
      PassUtils::SetInstrumentationMetadata(inst, debug_string, "no-alias-hint-location");
    }
  }

  // Store the final candidates
  Candidates = noAliasInstructionHintMap;

  return Candidates;
}

void CacheNoWritebackHint::insertHintFunctionCall(Module &M, Instruction *I, Instruction *HintLocation) {

  assert((I != nullptr) && "insertHintFunctionCall: hint Load is nullptr");
  assert((HintLocation != nullptr) && "insertHintFunctionCall: hint location is nullptr");

  // Greate the builder
  auto *BB = I->getParent();
  auto *F = BB->getParent();
  auto Builder = PassUtils::GetBuilder(F, BB);
  Builder.SetInsertPoint(HintLocation);

  // Get the context
  LLVMContext &Ctx = F->getContext();

  // Get the function
  FunctionCallee InsertFunctionCallee =
    M.getOrInsertFunction("__cache_hint", Type::getVoidTy(Ctx), Type::getInt8PtrTy(Ctx));
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

void CacheNoWritebackHint::Instrument(Noelle &N, Module &M, InstructionHintsMapTy &Candidates) {
  // Iterate over all the candidates
  // Add hints to the code by calling _cache_hint(load_source_address) at the hint locations
  for (const auto & [inst, hints] : Candidates) {
    for (const auto &hint : hints) {
      dbgs() << "Adding hint call at:\n" << *inst << "\n  for:\n" << *hint << "\n";
      insertHintFunctionCall(M, dyn_cast<Instruction>(hint), inst);
    }
  }
}

bool CacheNoWritebackHint::run(Noelle &N, WPAPass &WPA, Module &M) {
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
    auto Candidates = analyzeFunction(N, WPA, DA, *F);
 
    dbgs() << "Instructions with hint locations:\n";
    for (const auto & [inst, hints] : Candidates) {
      // Printing
      dbgs() << *inst << " has hints:\n";
      for (const auto &hint : hints) {
        dbgs() << "    " << *hint << "\n";
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

  /*
   * Fetch WPA
   */
  auto &WPA = getAnalysis<WPAPass>();

  //SVFModule svfModule{ M };
  //auto pta = new AndersenWaveDiff();
  //pta->analyze(svfModule);
  //auto svfCallGraph = pta->getPTACallGraph();
  //auto mssa = new MemSSA((BVDataPTAImpl *)pta, false);

  // For unit tests
  dbgs() << "#instructions: " << N.numberOfProgramInstructions() << "\n";

  /*
   * Apply the transformation
   */
  auto modified = this->run(N, WPA, M);

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

  AU.addRequired<WPAPass>();
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
