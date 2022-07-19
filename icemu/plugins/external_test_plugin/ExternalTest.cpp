/**
 *  ICEmu loadable plugin (library)
 *
 * An example ICEmu plugin that is dynamically loaded.
 * This example prints the address of each instruction that is executed.
 *
 * Should be compiled as a shared library, i.e. using `-shared -fPIC`
 */
#include <iostream>
#include <map>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Architecture.h"

using namespace std;
using namespace icemu;

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
 public:
  uint64_t count = 0;

  HookInstructionCount(Emulator &emu) : HookCode(emu, "icnt-ratio") {
  }

  ~HookInstructionCount() {
  }

  void run(hook_arg_t *arg) {
    (void)arg;  // Don't care
    ++count;
  }
};


// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
 public:
  HookInstructionCount *hook_instr_cnt;

  MemoryAccess(Emulator &emu) : HookMemory(emu, "memory-access-ratio") {
    hook_instr_cnt = new HookInstructionCount(emu);
  }

  ~MemoryAccess() {
      cout << "Ending the memory plugin" << endl;
  }

  void run(hook_arg_t *arg) {
    address_t address = arg->address;
  }
};

// Function that registers the hook
static void registerMyCodeHook(Emulator &emu, HookManager &HM) {
  auto mf = new MemoryAccess(emu);
  if (mf->getStatus() == Hook::STATUS_ERROR) {
    delete mf->hook_instr_cnt;
    delete mf;
    return;
  }
  HM.add(mf->hook_instr_cnt);
  HM.add(mf);
}

// Class that is used by ICEmu to finf the register function
// NB.  * MUST BE NAMED "RegisterMyHook"
//      * MUST BE global
RegisterHook RegisterMyHook(registerMyCodeHook);
