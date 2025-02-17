#pragma once

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

#include "noelle/core/Noelle.hpp"

#include <unordered_set>
#include <list>


class DependencyAnalysis {
public:
  struct Dependence {
    Instruction *Instruction;
    bool IsMust;

    bool operator==(const Dependence &rhs) const {
      return Instruction == rhs.Instruction;
    }
    bool operator==(const class Value *rhs) const { return Instruction == rhs; }
  };

  typedef map<Instruction *, list<Dependence>> InstructionDependencyMapTy;

  struct InstructionDependencies {
    InstructionDependencyMapTy WARs;
    InstructionDependencyMapTy RAWs;
    InstructionDependencyMapTy WAWs;
    InstructionDependencyMapTy RARs;
  };

  enum DependencyType { DEPTYPE_WAR, DEPTYPE_RAW, DEPTYPE_WAW, DEPTYPE_RAR };

private:
  // Analysis works on *either* a function or a module
  Function *F = nullptr; // Only set if we analyze only a function

  // Noelle (used for the actual analysis)
  Noelle &N;

  // Instruction dependencies
  InstructionDependencies _InstructionDependenciesFrom;
  InstructionDependencies _InstructionDependenciesTo;

  void analyzeModule();
  void analyzeFunction(Function *F);

  inline InstructionDependencyMapTy &getMap(InstructionDependencies &ID,
                                            DependencyType T) {
    switch (T) {
    case DEPTYPE_WAR:
      return ID.WARs;
    case DEPTYPE_RAW:
      return ID.RAWs;
    case DEPTYPE_WAW:
      return ID.WAWs;
    case DEPTYPE_RAR:
      return ID.RARs;
    }
    assert(false && "Unknown dependency type");
  }

  inline bool isRealCallInst(Instruction &I) {
      return isa<CallInst>(I) && (!isa<IntrinsicInst>(I));
  }

public:
  DependencyAnalysis(Noelle &N) : N(N) { analyzeModule(); }

  DependencyAnalysis(Noelle &N, Function *F) : N(N), F(F) {
    analyzeFunction(F);
  }

  bool isModuleAnalysis() { return F == nullptr; }
  bool isFunctionAnalysis() { return !isModuleAnalysis(); }

  InstructionDependencies &getInstructionDependenciesFrom() {
    return _InstructionDependenciesFrom;
  }

  InstructionDependencies &getInstructionDependenciesTo() {
    return _InstructionDependenciesTo;
  }

  list<Dependence> *getInstructionDependenciesTo(DependencyType T,
                                                 Instruction *I) {
    auto &map = getMap(_InstructionDependenciesTo, T);
    if (map.find(I) != map.end())
      return &map[I];
    else
      return nullptr;
  }

  list<Dependence> *getInstructionDependenciesFrom(DependencyType T,
                                                   Instruction *I) {
    auto &map = getMap(_InstructionDependenciesFrom, T);
    if (map.find(I) != map.end())
      return &map[I];
    else
      return nullptr;
  }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const DependencyAnalysis::Dependence &D);
