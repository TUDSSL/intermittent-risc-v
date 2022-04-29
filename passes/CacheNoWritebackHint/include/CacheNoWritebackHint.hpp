#pragma once

#include "noelle/core/Noelle.hpp"

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

  CandidatesTy analyze(Noelle &N, Module &M);
  void analyzeFunction(Noelle &N, Function &F, CandidatesTy &Candidates);

  void Instrument(Noelle &N, Module &M, CandidatesTy &Candidates);
};

} // namespace CacheNoWritebackHintNS
