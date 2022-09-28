#pragma once

#include "noelle/core/Noelle.hpp"
#include "noelle/core/DataFlowEngine.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "DependencyAnalysis.hpp"
#include "DataFlowEngineExt.hpp"

struct ReadReachDFA {

  static DataFlowResult *apply(DataFlowEngineExt &DFE, Function &F,
                               DependencyAnalysis &DA) {

    auto computeGEN = [&](Instruction *i, DataFlowResult *df) {
      if (!isa<StoreInst>(i)) {
        return;
      }
      auto &gen = df->GEN(i);

      return;
    };
    auto computeKILL = [&](Instruction *i, DataFlowResult *df) {
      if (!isa<LoadInst>(i)) {
        return;
      }

      auto &kill = df->KILL(i);

      return;
    };

    auto computeOUT = [](std::set<Value *> &OUT, Instruction *succ,
                         DataFlowResult *df) {
      auto &inS = df->IN(succ);
      OUT.insert(inS.begin(), inS.end());
      return;
    };

    auto computeIN = [](std::set<Value *> &IN, Instruction *inst,
                        DataFlowResult *df) {
      auto &genI = df->GEN(inst);
      auto &outI = df->OUT(inst);
      auto &killI = df->KILL(inst);

      // Add OUT[i]-KILL[i] to IN[i]
      //IN.insert(out_clean.begin(), out_clean.end());

      //// Add GEN[i] to IN[i]
      //IN.insert(genI.begin(), genI.end());

      return;
    };

    /*
     * Run the data flow analysis
     */
    auto DFR = DFE.applyBackward(&F, 
                                 computeGEN,
                                 computeKILL,
                                 computeIN,
                                 computeOUT,
                                 DataFlowEngineExt::DFE_MEET_INTERSECT);

    // Return the data flow result
    return DFR;
  }
};
