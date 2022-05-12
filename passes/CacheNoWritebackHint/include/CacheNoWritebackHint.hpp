#pragma once

#include "noelle/core/Noelle.hpp"
#include "DependencyAnalysis.hpp"

namespace CacheNoWritebackHintNS {

class CacheNoWritebackHint : public ModulePass {
public:

  typedef std::vector<Instruction *> CandidatesTy;

  static char ID;

  CacheNoWritebackHint();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  std::string prefixString;

  bool run(Noelle &N, Module &M);

  CandidatesTy analyze(Noelle &N, DependencyAnalysis &DA, Module &M);
  void analyzeFunction(Noelle &N, DependencyAnalysis &DA, Function &F, CandidatesTy &Candidates);
  void analyzeInstruction(Noelle &N, DependencyAnalysis &DA, Instruction &I, CandidatesTy &Candidates);

  void Instrument(Noelle &N, Module &M, CandidatesTy &Candidates);

  void insertHintFunctionCall(Noelle &N, Module &M, std::string FunctionName, Instruction *I);
};

} // namespace CacheNoWritebackHintNS
