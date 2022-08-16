#pragma once

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include "noelle/core/SystemHeaders.hpp"
#include "noelle/core/DataFlowResult.hpp"
#include "noelle/core/DataFlowEngine.hpp"

#include "SetManipulation.hpp"

namespace llvm::noelle {

// Extended dataflow engine with intersection option
class DataFlowEngineExt : public DataFlowEngine {

 public:
  DataFlowEngineExt() : DataFlowEngine() {};

  enum DataFlowMeet {
    DFE_MEET_UNION,
    DFE_MEET_INTERSECT,
  };

  // Copy of applyBackward from:
  //  noelle/src/core/dataflow/src/DataFlowEngine.cpp
  // With changes so we can perform an intersection instead of a union
  DataFlowResult *applyBackward(
    Function *f,
    std::function<void(Instruction *, DataFlowResult *)> computeGEN,
    std::function<void(Instruction *, DataFlowResult *)> computeKILL,
    std::function<void(std::set<Value *> &IN,
                       Instruction *inst,
                       DataFlowResult *df)> computeIN,
    std::function<void(std::set<Value *> &OUT,
                       Instruction *successor,
                       DataFlowResult *df)> computeOUT,
    DataFlowMeet meet) {
    /*
     * Compute the GENs and KILLs
     */
    auto df = new DataFlowResult{};
    computeGENAndKILL(f, computeGEN, computeKILL, df);

    /*
     * Compute the IN and OUT
     *
     * Create the working list by adding all basic blocks to it.
     */
    std::unordered_set<BasicBlock *> computedOnce;
    std::list<BasicBlock *> workingList;
    for (auto &bb : *f) {
      workingList.push_front(&bb);
    }

    /*
     * Compute the INs and OUTs iteratively until the working list is empty.
     */
    while (!workingList.empty()) {

      /*
       * Fetch a basic block that needs to be processed.
       */
      auto bb = workingList.front();
      workingList.pop_front();

      /*
       * Fetch the last instruction of the current basic block.
       */
      auto inst = bb->getTerminator();
      assert(inst != nullptr);

      /*
       * Fetch IN[inst], OUT[inst], GEN[inst], and KILL[inst]
       */
      auto &inSetOfInst = df->IN(inst);
      auto &outSetOfInst = df->OUT(inst);

      /*
       * Collect all OUT[successor]
       */
      std::vector<std::set<Value *>> OUTs;

      for (auto successorBB : successors(bb)) {
        /*
         * Fetch the current successor of "inst".
         */
        auto successorInst = &*successorBB->begin();

        /*
         * Compute OUT[inst]
         */
        std::set<Value *> successorOUT;
        computeOUT(successorOUT, successorInst, df);

        /*
         * Add the OUT to OUTs
         */
        OUTs.push_back(successorOUT);
      }

      /*
       * Use the meet operation to create the final outSetOfInst
       */
      switch (meet) {
        case DFE_MEET_INTERSECT:
          {
            /*
             * Create an Intersection of all OUT sets from all the successors
             */
            auto meetOut = SetManipulation::Intersection<Value *>(OUTs);
            outSetOfInst.insert(meetOut.begin(), meetOut.end());
            break;
          }
        case DFE_MEET_UNION:
          {
            /*
             * Create a Union of all OUT sets from all the successors
             */
            auto meetOut = SetManipulation::Union<Value *>(OUTs);
            outSetOfInst.insert(meetOut.begin(), meetOut.end());
            break;
          }
        default:
          assert(false && "Unhandled MEET operation for DFE");
      }
 
      /*
       * Compute IN[inst]
       */
      auto oldSize = inSetOfInst.size();
      computeIN(inSetOfInst, inst, df);

      /*
       * Check if IN[inst] changed.
       */
      if (false || (inSetOfInst.size() > oldSize)
          || (computedOnce.find(bb) == computedOnce.end())) {

        /*
         * Remember that we have now computed this basic block.
         */
        computedOnce.insert(bb);

        /*
         * Propagate the new IN[inst] to the rest of the instructions of the
         * current basic block.
         */
        BasicBlock::iterator iter(inst);
        auto succI = cast<Instruction>(inst);
        while (iter != bb->begin()) {

          /*
           * Move the iterator.
           */
          iter--;

          /*
           * Fetch the current instruction.
           */
          auto i = &*iter;

          /*
           * Compute OUT[i]
           */
          auto &outSetOfI = df->OUT(i);
          computeOUT(outSetOfI, succI, df);

          /*
           * Compute IN[i]
           */
          auto &inSetOfI = df->IN(i);
          computeIN(inSetOfI, i, df);

          /*
           * Update the successor.
           */
          succI = i;
        }

        /*
         * Add predecessors of the current basic block to the working list.
         */
        for (auto predBB : predecessors(bb)) {
          workingList.push_back(predBB);
        }
      }
    }

    return df;
  }

  // Copy of applyCustomizableForwardAnalysis from:
  //  noelle/src/core/dataflow/src/DataFlowEngine.cpp
  // With changes so we can perform an intersection instead of a union
  DataFlowResult *applyCustomizableForwardAnalysis(
    Function *f,
    std::function<void(Instruction *, DataFlowResult *)> computeGEN,
    std::function<void(Instruction *, DataFlowResult *)> computeKILL,
    std::function<void(Instruction *inst, std::set<Value *> &IN)> initializeIN,
    std::function<void(Instruction *inst, std::set<Value *> &OUT)>
        initializeOUT,
    std::function<void(Instruction *inst,
                       std::set<Value *> &IN,
                       Instruction *predecessor,
                       DataFlowResult *df)> computeIN,
    std::function<void(Instruction *inst,
                       std::set<Value *> &OUT,
                       DataFlowResult *df)> computeOUT,
    std::function<void(std::list<BasicBlock *> &workingList, BasicBlock *bb)>
        appendBB,
    std::function<Instruction *(BasicBlock *bb)> getFirstInstruction,
    std::function<Instruction *(BasicBlock *bb)> getLastInstruction,
    DataFlowMeet meet) {

    /*
     * Initialize IN and OUT sets.
     */
    auto df = new DataFlowResult{};
    for (auto &bb : *f) {
      for (auto &i : bb) {
        auto &INSet = df->IN(&i);
        auto &OUTSet = df->OUT(&i);
        initializeIN(&i, INSet);
        initializeOUT(&i, OUTSet);
      }
    }

    /*
     * Compute the GENs and KILLs
     */
    computeGENAndKILL(f, computeGEN, computeKILL, df);

    /*
     * Compute the IN and OUT
     *
     * Create the working list by adding all basic blocks to it.
     */
    std::list<BasicBlock *> workingList;
    std::unordered_map<BasicBlock *, bool> worklingListContent;
    for (auto &bb : *f) {
      appendBB(workingList, &bb);
      worklingListContent[&bb] = true;
    }

    /*
     * Compute the INs and OUTs iteratively until the working list is empty.
     */
    std::unordered_map<Instruction *, bool> alreadyVisited;
    while (!workingList.empty()) {

      /*
       * Fetch a basic block that needs to be processed.
       */
      auto bb = workingList.front();
      assert(worklingListContent[bb] == true);

      /*
       * Remove the basic block from the workingList.
       */
      workingList.pop_front();
      worklingListContent[bb] = false;

      /*
       * Fetch the first instruction of the basic block.
       */
      auto inst = getFirstInstruction(bb);

      /*
       * Fetch IN[inst], OUT[inst], GEN[inst], and KILL[inst]
       */
      auto &inSetOfInst = df->IN(inst);
      auto &outSetOfInst = df->OUT(inst);

      /*
       * Collect all IN[predecessor]
       */
      std::vector<std::set<Value *>> INs;

      /*
       * Compute the IN of the first instruction of the current basic block.
       */
      for (auto predecessorBB : predecessors(bb)) {

        /*
         * Fetch the current predecessor of "inst".
         */
        auto predecessorInst = getLastInstruction(predecessorBB);

        /*
         * Compute IN[inst]
         */
        std::set<Value *> predecessorIN;
        computeIN(inst, predecessorIN, predecessorInst, df);
      }

      /*
       * Use the meet operation to create the final inSetOfInst
       */
      switch (meet) {
        case DFE_MEET_INTERSECT:
          {
            /*
             * Create an Intersection of all OUT sets from all the successors
             */
            auto meetIn = SetManipulation::Intersection<Value *>(INs);
            inSetOfInst.insert(meetIn.begin(), meetIn.end());
            break;
          }
        case DFE_MEET_UNION:
          {
            /*
             * Create a Union of all OUT sets from all the successors
             */
            auto meetIn = SetManipulation::Union<Value *>(INs);
            inSetOfInst.insert(meetIn.begin(), meetIn.end());
            break;
          }
        default:
          assert(false && "Unhandled MEET operation for DFE");
      }

      /*
       * Compute OUT[inst]
       */
      auto oldSize = outSetOfInst.size();
      computeOUT(inst, outSetOfInst, df);

      /* Check if the OUT of the first instruction of the current basic block
       * changed.
       */
      if (false || (!alreadyVisited[inst]) || (outSetOfInst.size() != oldSize)) {
        alreadyVisited[inst] = true;

        /*
         * Propagate the new OUT[inst] to the rest of the instructions of the
         * current basic block.
         */
        BasicBlock::iterator iter(inst);
        auto predI = inst;
        iter++;
        while (iter != bb->end()) {

          /*
           * Fetch the current instruction.
           */
          auto i = &*iter;

          /*
           * Compute IN[i]
           */
          auto &inSetOfI = df->IN(i);
          computeIN(i, inSetOfI, predI, df);

          /*
           * Compute OUT[i]
           */
          auto &outSetOfI = df->OUT(i);
          computeOUT(i, outSetOfI, df);

          /*
           * Update the predecessor.
           */
          predI = i;

          /*
           * Move the iterator.
           */
          iter++;
        }

        /*
         * Add successors of the current basic block to the working list.
         */
        for (auto succBB : successors(bb)) {
          if (worklingListContent[succBB] == true) {
            continue;
          }
          workingList.push_back(succBB);
          worklingListContent[succBB] = true;
        }
      }
    }

    return df;
  }


  /*
   * Below this comment are copies of the helpers in:
   * noelle/blob/master/src/core/dataflow/src/DataFlowEngine.cpp
   *
   * The only change is the addition of the DataFlowMeet argument to support the above
   * implementation of the meet operation (Union or Intersection)
   */

  DataFlowResult *applyForward(
      Function *f,
      std::function<void(Instruction *, DataFlowResult *)> computeGEN,
      std::function<void(Instruction *inst, std::set<Value *> &IN)> initializeIN,
      std::function<void(Instruction *inst, std::set<Value *> &OUT)>
          initializeOUT,
      std::function<void(Instruction *inst,
                         std::set<Value *> &IN,
                         Instruction *predecessor,
                         DataFlowResult *df)> computeIN,
      std::function<void(Instruction *inst,
                         std::set<Value *> &OUT,
                         DataFlowResult *df)> computeOUT,
      DataFlowMeet meet) {
  
    /*
     * Define an empty KILL set.
     */
    auto computeKILL = [](Instruction *, DataFlowResult *) { return; };
  
    /*
     * Run the data-flow analysis.
     */
    auto dfr = this->applyForward(f,
                                  computeGEN,
                                  computeKILL,
                                  initializeIN,
                                  initializeOUT,
                                  computeIN,
                                  computeOUT,
                                  meet);
  
    return dfr;
  }
  
  DataFlowResult *applyForward(
      Function *f,
      std::function<void(Instruction *, DataFlowResult *)> computeGEN,
      std::function<void(Instruction *, DataFlowResult *)> computeKILL,
      std::function<void(Instruction *inst, std::set<Value *> &IN)> initializeIN,
      std::function<void(Instruction *inst, std::set<Value *> &OUT)>
          initializeOUT,
      std::function<void(Instruction *inst,
                         std::set<Value *> &IN,
                         Instruction *predecessor,
                         DataFlowResult *df)> computeIN,
      std::function<void(Instruction *inst,
                         std::set<Value *> &OUT,
                         DataFlowResult *df)> computeOUT,
      DataFlowMeet meet) {
  
    /*
     * Define the customization.
     */
    auto appendBB = [](std::list<BasicBlock *> &workingList, BasicBlock *bb) {
      workingList.push_back(bb);
    };
  
    auto getFirstInst = [](BasicBlock *bb) -> Instruction * {
      return &*bb->begin();
    };
    auto getLastInst = [](BasicBlock *bb) -> Instruction * {
      return bb->getTerminator();
    };
  
    /*
     * Run the pass.
     */
    auto dfaResult = this->applyCustomizableForwardAnalysis(f,
                                                            computeGEN,
                                                            computeKILL,
                                                            initializeIN,
                                                            initializeOUT,
                                                            computeIN,
                                                            computeOUT,
                                                            appendBB,
                                                            getFirstInst,
                                                            getLastInst,
                                                            meet);
  
    return dfaResult;
  }
  
  DataFlowResult *applyBackward(
      Function *f,
      std::function<void(Instruction *, DataFlowResult *)> computeGEN,
      std::function<void(std::set<Value *> &IN,
                         Instruction *inst,
                         DataFlowResult *df)> computeIN,
      std::function<void(std::set<Value *> &OUT,
                         Instruction *successor,
                         DataFlowResult *df)> computeOUT,
      DataFlowMeet meet) {
  
    /*
     * Define an empty KILL set.
     */
    auto computeKILL = [](Instruction *, DataFlowResult *) { return; };
  
    /*
     * Run the data-flow analysis.
     */
    auto dfr =
        this->applyBackward(f, computeGEN, computeKILL, computeIN, computeOUT, meet);
  
    return dfr;
  }

};

}

