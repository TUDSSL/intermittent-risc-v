#pragma once

#include "noelle/core/Noelle.hpp"
#include "noelle/core/DataFlowEngine.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

#include "SetManipulation.hpp"

struct WriteAfterReadReachDFA {
  struct WAR {
    llvm::Instruction *Read;
    llvm::Instruction *Write;
  };

  static DataFlowResult *apply(DataFlowEngine &DFE, Function &F, std::vector<WAR> &WARs) {
    //
    // Define the data flow equations
    //

    // If the instruction is a WAR Write, then the GEN is the WAR read
    auto ComputeGEN = [&](Instruction *i, DataFlowResult *df) {
      auto &gen = df->GEN(i);
      for (auto &WAR : WARs) {
        if (i == WAR.Write) {
          gen.insert(WAR.Read);
        } else {
          //gen.insert(i);
        }
      }
      return;
    };
    // If the instruction is a WAR Read, then the KILL is the WAR Read
    auto ComputeKILL = [&](Instruction *i, DataFlowResult *df) {
      auto &kill = df->KILL(i);
      for (auto &WAR : WARs) {
        if (i == WAR.Read) {
          kill.insert(WAR.Read);
        }
      }
      return;
    };
    auto ComputeOUT = [](std::set<Value *> &OUT, Instruction *succ, DataFlowResult *df) {
      //auto &inS = df->IN(succ);
      //OUT.insert(inS.begin(), inS.end());

      // Missuse the OUT set to hold all the successors
      // Need to clean this up in 'computeIN'
      OUT.insert(succ);
      return;
    };
    auto ComputeIN = [](std::set<Value *> &IN, Instruction *inst,
                        DataFlowResult *df) {
      auto &genI = df->GEN(inst);
      auto &killI = df->KILL(inst);
      auto &outI = df->OUT(inst);

      // Clean up the OUT, it currently holds successors
      auto Successors(df->OUT(inst));
      outI.clear();
      //dbgs() << "Instruction: " << *inst << " has successors:\n";
      //for (auto &Successor : Successors) {
      //  dbgs() << "   " << *Successor << "\n";
      //}

      // Compute the new OUT
      vector<set<Value *>> OUTs;
      dbgs() << "Instruction: " << *inst << " has successors:\n";
      for (auto &Successor : Successors) {
        dbgs() << "   " << *Successor << "\n";
        auto &inS = df->IN(dyn_cast<Instruction>(Successor));
        dbgs() << "   Suc INs:\n";
        for (auto &inI : inS) {
          dbgs() << "     " << *inI << "\n";
        }
        OUTs.push_back(inS);
        //OUTs.push_back(df->IN(dyn_cast<Instruction>(Successor)));
      }

      dbgs() << "OUTs\n";
      for (auto &OUT : OUTs) {
        dbgs() << "  OUT:\n";
        for (auto &OutI : OUT) {
          dbgs() << "    " << *OutI << "\n";
        }
      }

      // Compute the intersection of all the OUTs
      //for (auto &OUT : OUTs) {
      //  set<Value *> IntersectOUT;
      //  //set_intersection(IntersectOUT.begin(), IntersectOUT.end(), OUT.begin(), OUT.end()); 
      //}
      auto IntersectOUT = SetManipulation::Intersection<Value *>(OUTs);

      // Update the OUT set for the instruction
      outI.insert(IntersectOUT.begin(), IntersectOUT.end());

      dbgs() << "Intersect OUT:\n";
      for (auto &Out : outI) {
          dbgs() << "   " << *Out << "\n";
      }

      // Perform the "normal" IN creation, combining the OUT set and the GEN set
      IN.insert(outI.begin(), outI.end());
      IN.insert(genI.begin(), genI.end());
      IN.erase(killI.begin(), killI.end());

      dbgs() << "Gen:\n";
      for (auto &i : genI) {
          dbgs() << "   " << *i << "\n";
      }

      dbgs() << "Kill:\n";
      for (auto &i : killI) {
          dbgs() << "   " << *i << "\n";
      }

      dbgs() << "Final IN:\n";
      for (auto &In : IN) {
          dbgs() << "   " << *In << "\n";
      }
      return;
    };

    // Apply the DFA
    auto CustomDFA = DFE.applyBackward(
      &F,
      ComputeGEN, 
      ComputeKILL, 
      ComputeIN, 
      ComputeOUT
      );

    return CustomDFA;
  }
};
