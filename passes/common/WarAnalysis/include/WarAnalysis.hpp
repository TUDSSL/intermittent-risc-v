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

class WarAnalysis {
 public:

  typedef struct {
    llvm::Instruction *read;
    llvm::Instruction *write;
  } ReadWritePairTy;
  typedef std::vector<ReadWritePairTy> ReadWritePairsTy;

  typedef std::vector<Instruction *> PathTy;
  typedef std::list<PathTy> PathsTy;

  typedef std::unordered_set<Instruction *> CutsTy;

  typedef std::map<const ReadWritePairTy *, const PathTy *> WarPathMapTy;

  struct Dependence {
    Value *Value;
    bool IsMust;

    bool operator==(const Dependence &rhs) const {
      return Value == rhs.Value;
    }
    bool operator==(const class Value *rhs) const {
      return Value == rhs;
    }
  };

  typedef map<Instruction *, list<Dependence>> InstructionDependencyMapTy;

 private:
  Noelle &N;
  Function &F;

  InstructionDependencyMapTy WarDepMap;
  InstructionDependencyMapTy RawDepMap;

  ReadWritePairsTy AllWars;
  ReadWritePairsTy UncutWars;
  ReadWritePairsTy PrecutWars;

  // A map from a WAR to the resulting Path
  WarPathMapTy WarPathMap;

  CutsTy ForcedCuts;
  PathsTy Paths;

  bool forcesCut(Instruction &I);
  bool hasUncutPath(CutsTy &Cuts, Instruction *From, Instruction *To);

  void collectInstructionDependencies();
  void collectForcedCuts();
  void collectUncutWars();
  void collectDominatingPaths();

  void collectWarDependencies();

 public:
  WarAnalysis(Noelle &N, Function &F) : N(N), F(F) {}
  PathsTy &run();

  /*
   * Getters
   */
  ReadWritePairsTy &getUncutWars() { return UncutWars; }
  CutsTy &getForcedCuts() { return ForcedCuts; }
  PathsTy &getPaths() { return Paths; }

  const WarPathMapTy &getWarPathMap() { return WarPathMap; }
  const InstructionDependencyMapTy &getRawDepMap() { return RawDepMap; }
  const InstructionDependencyMapTy &getWarDepMap() { return WarDepMap; }
};

/*
* Print Support
*/
static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const WarAnalysis::ReadWritePairTy &RW) {
  os << "R:" << *RW.read << " - W:" << *RW.write;
  return os;
}

