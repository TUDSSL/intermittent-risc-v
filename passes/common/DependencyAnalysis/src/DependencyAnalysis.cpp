#include "DependencyAnalysis.hpp"
#include "PassUtils.hpp"

/*
 * Can be compiled with debug information
 */
#define DEBUG_PRINT
#ifdef DEBUG_PRINT
#define dbg() errs()
#else
#define dbg() if (true) {} else errs()
#endif

void DependencyAnalysis::analyzeModule() {
  auto FM = N.getFunctionsManager();
  auto PCF = FM->getProgramCallGraph();

  // Go through all the functions
  for (auto Node : PCF->getFunctionNodes()) {
    Function *F = Node->getFunction();
    assert(F != nullptr);
    if (F->isIntrinsic())
      continue;

    // Analyze the function
    analyzeFunction(F);
  }
}

void DependencyAnalysis::analyzeFunction(Function *F) {
  dbg() << "DependencyAnalysis: Analyzing function: " << F->getName() << "\n";

  auto FDG = N.getFunctionDependenceGraph(F);
  auto DT = N.getDominators(F)->DT;

  list<Dependence> war_deps;
  list<Dependence> raw_deps;
  list<Dependence> waw_deps;
  list<Dependence> rar_deps;

  Instruction *I;

  auto iterDep = [&](Value *src, DGEdge<Value> *dependence) -> bool {
    if (dependence == nullptr) return false;
    if (src == nullptr) return false;

    Instruction *src_instr = dyn_cast<Instruction>(src);
    assert(src_instr != nullptr);

    switch (dependence->dataDependenceType()) {
      case DG_DATA_WAR:
        dbg() << "             needs [WAR]";
        dbg() << "  " << *src << "\n";
        war_deps.push_back(Dependence{src_instr, dependence->isMustDependence()});
        break;
      case DG_DATA_RAW:
        dbg() << "             needs [RAW]";
        dbg() << "  " << *src << "\n";
        raw_deps.push_back(Dependence{src_instr, dependence->isMustDependence()});
        break;
      case DG_DATA_WAW:
        dbg() << "             needs [WAW]";
        dbg() << "  " << *src << "\n";
        waw_deps.push_back(Dependence{src_instr, dependence->isMustDependence()});
        break;
      case DG_DATA_RAR:
        dbg() << "             needs [RAR]";
        dbg() << "  " << *src << "\n";
        rar_deps.push_back(Dependence{src_instr, dependence->isMustDependence()});
        break;
      case DG_DATA_NONE:
        break;
      default:
        dbg() << "             needs [UNKNOWN]";
    }
    return false;
  };

  // Iterate over all the instructions in the function
  for (auto &I : instructions(F)) {
    if (isa<StoreInst>(I) || isa<LoadInst>(I) || isRealCallInst(I)) {
      dbg() << "Instruction" << I << " *to* dependencies:\n";

      // Collect the dependencies *to* the instruction
      FDG->iterateOverDependencesTo(&I, false, true, false, iterDep);

      // Only add to the map if there are dependencies
      if (war_deps.size() > 0) _InstructionDependenciesTo.WARs[&I] = war_deps;
      if (raw_deps.size() > 0) _InstructionDependenciesTo.RAWs[&I] = raw_deps;
      if (waw_deps.size() > 0) _InstructionDependenciesTo.WAWs[&I] = waw_deps;
      if (rar_deps.size() > 0) _InstructionDependenciesTo.RARs[&I] = rar_deps;
 
      // Clear for the next instruction
      war_deps.clear();
      raw_deps.clear();
      waw_deps.clear();
      rar_deps.clear();

      dbg() << "Instruction" << I << " *from* dependencies:\n";

      // Collect the dependencies *from* the instruction
      FDG->iterateOverDependencesFrom(&I, false, true, false, iterDep);

      // Only add to the map if there are dependencies
      if (war_deps.size() > 0) _InstructionDependenciesFrom.WARs[&I] = war_deps;
      if (raw_deps.size() > 0) _InstructionDependenciesFrom.RAWs[&I] = raw_deps;
      if (waw_deps.size() > 0) _InstructionDependenciesFrom.WAWs[&I] = waw_deps;
      if (rar_deps.size() > 0) _InstructionDependenciesFrom.RARs[&I] = rar_deps;

      // Clear for the next instruction
      war_deps.clear();
      raw_deps.clear();
      waw_deps.clear();
      rar_deps.clear();
    }
  }
}

