#pragma once

#include "noelle/core/Noelle.hpp"
#include "DependencyAnalysis.hpp"

namespace CacheNoWritebackHintNS {

class CacheNoWritebackHint : public ModulePass {
public:

  typedef std::set<Instruction *> HintLocationsTy;

  struct CandidateTy {
    Instruction *I = nullptr;
    HintLocationsTy PossibleHintLocations;
    HintLocationsTy SelectedHintLocations;
  };

  typedef std::vector<CandidateTy> CandidatesTy;

  static char ID;

  CacheNoWritebackHint();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  std::string prefixString;

  struct WAR {
    llvm::Instruction *Read;
    llvm::Instruction *Write;
  };

  bool run(Noelle &N, Module &M);

  //CandidatesTy analyze(Noelle &N, DependencyAnalysis &DA, Module &M);
  CandidatesTy analyzeFunction(Noelle &N, DependencyAnalysis &DA, Function &F);
  std::tuple<bool, CandidateTy> analyzeInstruction(Noelle &N, DependencyAnalysis &DA, DataFlowResult & DFReach, DomTreeSummary &DT, Instruction &I);

  void Instrument(Noelle &N, Module &M, CandidatesTy &Candidates);

  void insertHintFunctionCall(Noelle &N, Module &M, std::string FunctionName, Instruction *I, Instruction *HintLocation);

  void selectHintLocations(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates);

  DataFlowResult *writeAfterReadReachDFA(DataFlowEngine &DFE, Function &F, std::vector<WAR> &WARs);

  //CandidatesTy analyzeCandidates(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates);
  //void analyzeCandidate(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates);

};

} // namespace CacheNoWritebackHintNS
