#pragma once

#include "noelle/core/Noelle.hpp"
#include "DependencyAnalysis.hpp"

#include "WPA/WPAPass.h"

namespace CacheNoWritebackHintNS {

class CacheNoWritebackHint : public ModulePass {
public:
  typedef std::map<Instruction *, std::set<Value *>> InstructionHintsMapTy;

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

  bool run(Noelle &N, WPAPass &WPA, Module &M);

  InstructionHintsMapTy analyzeFunction(Noelle &N, WPAPass &WPA, DependencyAnalysis &DA, Function &F);

  void Instrument(Noelle &N, Module &M, InstructionHintsMapTy &Candidates);

  void insertHintFunctionCall(Module &M, Instruction *I, Instruction *HintLocation);

  DataFlowResult *writeAfterReadReachDFA(DataFlowEngine &DFE, Function &F, std::vector<WAR> &WARs);

};

} // namespace CacheNoWritebackHintNS
