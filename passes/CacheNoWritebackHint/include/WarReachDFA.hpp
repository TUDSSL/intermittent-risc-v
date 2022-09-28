#pragma once

#include "noelle/core/Noelle.hpp"
#include "noelle/core/DataFlowEngine.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "DependencyAnalysis.hpp"
#include "DataFlowEngineExt.hpp"

struct WarReachDFA {
  // TODO: Move this somewhere else
  struct WAR {
    llvm::Instruction *Read;
    llvm::Instruction *Write;
  };

  static DataFlowResult *apply(DataFlowEngineExt &DFE, Function &F,
                               DependencyAnalysis &DA) {

    // Gen:
    //  for instruction 'i' and all wars W with (read, write)
    //  GEN[i] = {r | (r,w) in W, w=i}
    //
    //  i.e., If this instruction is a write in a WAR, insert the corresponding
    //  read 'r' in GEN[i]
    auto computeGEN = [&](Instruction *i, DataFlowResult *df) {
      // TODO: make the store check more lenient, anything that causes a
      // write should be OK
      if (!isa<StoreInst>(i)) {
        return;
      }
      auto &gen = df->GEN(i);

      // TODO: make into a set
      //for (const auto &war : WARs) {
      //  if (war.Write == i) {
      //    gen.insert(war.Read);
      //  }
      //}

      // Get all the WAR dependencies TO this store instructions
      // i.e., reads
      auto WARDeps = DA.getInstructionDependenciesTo(DependencyAnalysis::DEPTYPE_WAR, i);
      if (WARDeps == nullptr) return;

      for (const auto &dep : *WARDeps) {
        // Add the read
        if (dep.IsMust) gen.insert(dep.Instruction);
      }

      return;
    };

    // Kill:
    //  for instruction 'i' and all wars W with (read, write)
    //  KILL[i] = {r | (r,w) in W, r=i}
    //
    //  i.e., If this instruction is a read in a WAR, insert the corresponding
    //  write 'w' in KILL[i] (i==w)
    //
    //  ADDITIONAL_NEED_TEST:
    //   Not only kill the directly related WAR read, but also all other WAR reads from the same load
    auto computeKILL = [&](Instruction *i, DataFlowResult *df) {
      if (!isa<LoadInst>(i)) {
        return;
      }


      auto &kill = df->KILL(i);

      auto ReadWARDeps = DA.getInstructionDependenciesFrom(DependencyAnalysis::DEPTYPE_WAR, i);
      if (ReadWARDeps == nullptr) return;

      // Writes that WAR with this Read
      for (const auto &war_write : *ReadWARDeps) {
        auto WriteWARDeps = DA.getInstructionDependenciesTo(DependencyAnalysis::DEPTYPE_WAR, war_write.Instruction);
        if (WriteWARDeps == nullptr) continue;

        // Reads from all the writes that WAR with the main read
        for (const auto &war_read : *WriteWARDeps) {
          // Add the read (May or Must)
          kill.insert(war_read.Instruction);
        }
      }

      //errs() << "KILL for instruction: " << *i << "\n";
      //for (const auto &k : kill) {
      //  errs() << "  " << *k << "\n";
      //}
    };

    // Compute out:
    //  for instruction 'i'
    //  OUT[i] = for all successors s of i, use union (meet operator)
    //  Here, collect the OUT of each successor
    auto computeOUT = [](std::set<Value *> &OUT, Instruction *succ,
                         DataFlowResult *df) {
      auto &inS = df->IN(succ);
      OUT.insert(inS.begin(), inS.end());
      return;
    };

    // Compute in:
    //  for instruction 'i'
    //  IN[i] = GEN[i] | (OUT[i]-KILL[i])
    auto computeIN = [](std::set<Value *> &IN, Instruction *inst,
                        DataFlowResult *df) {
      auto &genI = df->GEN(inst);
      auto &outI = df->OUT(inst);
      auto &killI = df->KILL(inst);

      // OUT[i]-KILL[i]
      std::set<Value *> out_clean;
      std::set_difference(outI.begin(), outI.end(),
                          killI.begin(), killI.end(),
                          std::inserter(out_clean, out_clean.end()));

      // Add OUT[i]-KILL[i] to IN[i]
      IN.insert(out_clean.begin(), out_clean.end());

      // Add GEN[i] to IN[i]
      IN.insert(genI.begin(), genI.end());

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

#if 0
  static DataFlowResult *apply(DataFlowEngineExt &DFE, Function &F,
                               std::vector<WAR> &WARs) {

    // Gen:
    //  for instruction 'i' and all wars W with (read, write)
    //  GEN[i] = {r | (r,w) in W, w=i}
    //
    //  i.e., If this instruction is a write in a WAR, insert the corresponding
    //  read 'r' in GEN[i]
    auto computeGEN = [&](Instruction *i, DataFlowResult *df) {
      // TODO: make the store check more lenient, anything that causes a
      // write should be OK
      if (!isa<StoreInst>(i)) {
        return;
      }
      auto &gen = df->GEN(i);

      // TODO: make into a set
      for (const auto &war : WARs) {
        if (war.Write == i) {
          gen.insert(war.Read);
        }
      }

      return;
    };

    // Kill:
    //  for instruction 'i' and all wars W with (read, write)
    //  KILL[i] = {r | (r,w) in W, r=i}
    //
    //  i.e., If this instruction is a read in a WAR, insert the corresponding
    //  write 'w' in KILL[i] (i==w)
    //
    //  ADDITIONAL_NEED_TEST:
    //   Not only kill the directly related WAR read, but also all other WAR reads from the same load
    auto computeKILL = [&](Instruction *i, DataFlowResult *df) {
      if (!isa<LoadInst>(i)) {
        return;
      }
      auto &kill = df->KILL(i);

      // TODO: make into a set
      for (const auto &war : WARs) {
        if (war.Read == i) {
          kill.insert(war.Read);
        }
      }
    };

    // Compute out:
    //  for instruction 'i'
    //  OUT[i] = for all successors s of i, use union (meet operator)
    //  Here, collect the OUT of each successor
    auto computeOUT = [](std::set<Value *> &OUT, Instruction *succ,
                         DataFlowResult *df) {
      auto &inS = df->IN(succ);
      OUT.insert(inS.begin(), inS.end());
      return;
    };

    // Compute in:
    //  for instruction 'i'
    //  IN[i] = GEN[i] | (OUT[i]-KILL[i])
    auto computeIN = [](std::set<Value *> &IN, Instruction *inst,
                        DataFlowResult *df) {
      auto &genI = df->GEN(inst);
      auto &outI = df->OUT(inst);
      auto &killI = df->KILL(inst);

      // OUT[i]-KILL[i]
      std::set<Value *> out_clean;
      std::set_difference(outI.begin(), outI.end(),
                          killI.begin(), killI.end(),
                          std::inserter(out_clean, out_clean.end()));

      // Add OUT[i]-KILL[i] to IN[i]
      IN.insert(out_clean.begin(), out_clean.end());

      // Add GEN[i] to IN[i]
      IN.insert(genI.begin(), genI.end());

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
#endif
};
