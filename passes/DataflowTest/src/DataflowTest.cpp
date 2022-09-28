#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "DataflowTest.hpp"
#include "Configurations.hpp"
#include "PassUtils.hpp"

#include "DataFlowEngineExt.hpp"

using namespace llvm::noelle ;

namespace DataflowTestNS {

DataflowTest::DataflowTest() :
  ModulePass(ID),
  prefixString{"DataflowTest: "}
{
  return ;
}

void DataflowTest::reachUse(Function *mainF) {
  /*
   * Fetch the data flow engine.
   */
  //auto dfe = N.getDataFlowEngine();
  auto dfe = DataFlowEngineExt(); // Modified version that allows for intersections

  /*
   * Define the data flow equations
   */
  auto computeGEN = [](Instruction *i, DataFlowResult *df) {
    if (!isa<LoadInst>(i)){
      return ;
    }
    auto& gen = df->GEN(i);
    gen.insert(i);
    return ;
  };
  auto computeKILL = [](Instruction *, DataFlowResult *) {
    return ;
  };
  auto computeOUT = [](std::set<Value *>& OUT, Instruction *succ, DataFlowResult *df) {
    auto& inS = df->IN(succ);
    OUT.insert(inS.begin(), inS.end());
    return ;
  } ;
  auto computeIN = [](std::set<Value *>& IN, Instruction *inst, DataFlowResult *df) {
    auto& genI = df->GEN(inst);
    auto& outI = df->OUT(inst);
    IN.insert(outI.begin(), outI.end());
    IN.insert(genI.begin(), genI.end());
    return ;
  };

  /*
   * Run the data flow analysis
   */
  auto customDfr = dfe.applyBackward(
    mainF,
    computeGEN, 
    computeKILL, 
    computeIN, 
    computeOUT,
    DataFlowEngineExt::DFE_MEET_UNION
    );

  /*
   * Print
   */
  for (auto& inst : instructions(mainF)){
    if (!isa<LoadInst>(&inst)){
      continue ;
    }
    auto insts = customDfr->OUT(&inst);
    errs() << " Next are the " << insts.size() << " instructions ";
    errs() << "that could read the value loaded by " << inst << "\n";

    std::string debug_str = "";
    for (auto possibleInst : insts){
      errs() << "   " << *possibleInst << "\n";
      debug_str += PassUtils::GetOperandString(*possibleInst) + " ";
    }

    if (debug_str != "") {
      debug_str.pop_back(); // Remove the last space
      errs() << "   Operand str:" << debug_str << "\n";
      errs() << "Instruction operand: " << PassUtils::GetOperandString(inst) << "\n";
      PassUtils::SetInstrumentationMetadata(&inst, debug_str, "could-read-value");
    }
  }
}

bool DataflowTest::run(Noelle &N, Module &M) {
  auto modified = false;

  auto FM = N.getFunctionsManager();
  auto PCF = FM->getProgramCallGraph();

    /*
   * Fetch the entry point.
   */
  auto fm = N.getFunctionsManager();
  auto mainF = fm->getEntryFunction();

  // Run ReachUse DFA
  reachUse(mainF);

#if 0
  for (auto Node : PCF->getFunctionNodes()) {
    Function *F = Node->getFunction();
    assert(F != nullptr);
    if (F->isIntrinsic()) continue;

    dbg() << "Function: " << F->getName() << "\n";

    for (auto &BB : *F) {
        for (auto &I : BB) {
            dbg() << "   " << I << "\n";
        }
    }
    dbg() << "\n";
  }
#endif

  return modified;
}

/*
 * Pass Registration
 */

bool DataflowTest::doInitialization (Module &M) {
  /*
   * Check the options.
   */
  return false;
}

bool DataflowTest::runOnModule(Module &M) {

  /*
   * Fetch NOELLE
   */
  auto &N = getAnalysis<Noelle>();
  errs() << "\n";
  errs() << prefixString << "The program has "
         << N.numberOfProgramInstructions() << " instructions\n";

  /*
   * Apply the transformation
   */
  auto modified = this->run(N, M);

  /*
   * Verify
   */
  if (NoVerify == false) {
    PassUtils::Verify(M);
  }

  return modified;
}

void DataflowTest::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<Noelle>();
}

// Next there is code to register your pass to "opt"
char DataflowTest::ID = 0;
static RegisterPass<DataflowTest> X("dfintersect", "Test pass");

// Next there is code to register your pass to "clang"
static DataflowTest * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new DataflowTest());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new DataflowTest()); }}); // ** for -O0

} // namespace DataflowTestNS
