#pragma once

#include "noelle/core/Noelle.hpp"
#include "DependencyAnalysis.hpp"

namespace CacheNoWritebackHintNS {

class CacheNoWritebackHint : public ModulePass {
public:

  struct CandidateTy {
    Instruction *I;
    set<Instruction *> Reachable;
    set<Instruction *> RARs;
    set<Instruction *> WARs;
  };

  //typedef std::vector<Instruction *> CandidatesTy;
  typedef std::vector<CandidateTy> CandidatesTy;

  static char ID;

  CacheNoWritebackHint();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  std::string prefixString;

  bool run(Noelle &N, Module &M);

  //CandidatesTy analyze(Noelle &N, DependencyAnalysis &DA, Module &M);
  CandidatesTy analyzeFunction(Noelle &N, DependencyAnalysis &DA, Function &F);
  void analyzeInstruction(Noelle &N, DependencyAnalysis &DA, DataFlowResult & DFReach, Instruction &I, CandidatesTy &Candidates);

  void Instrument(Noelle &N, Module &M, CandidatesTy &Candidates);

  void insertHintFunctionCall(Noelle &N, Module &M, std::string FunctionName, Instruction *I);

  CandidatesTy analyzeCandidates(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates);
  //void analyzeCandidate(Noelle &N, DependencyAnalysis &DA, CandidatesTy &Candidates);

};

} // namespace CacheNoWritebackHintNS
