/**
 *  ICEmu loadable plugin (library)
 *
 * An example ICEmu plugin that is dynamically loaded.
 * This example prints the address of each instruction that is executed.
 *
 * Should be compiled as a shared library, i.e. using `-shared -fPIC`
 */


/**
 * Different stats that can be fetched
 * - Cache hits
 * - Cache misses
 * - No of Read-evictions
 * - No of Write-evictions
 * - No of write-evictions that had a read
 */


#include <iostream>
#include <map>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>
#include <chrono>
#include <queue>
#include <list>
#include <map>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Function.h"

using namespace std;
using namespace icemu;

#define MEMORY_WINDOW 10000

enum mem_state {
  STATE_NEW,
  STATE_W1,
  STATE_R
};

enum mem_state state = STATE_NEW;
map<uint32_t, pair<enum mem_state, uint32_t>> m_state;
list<pair<HookMemory::memory_type, uint32_t>> mem;
ofstream file;

void detectWRWmemoryAccess(armaddr_t address, HookMemory::memory_type type, armaddr_t value)
{
  if (m_state.count(address) == 0)
    m_state.insert(pair<uint32_t, pair<enum mem_state, uint32_t>>(address, pair<enum mem_state, uint32_t>(STATE_NEW, 0)));

  if (mem.size() == MEMORY_WINDOW)
    mem.pop_front();

  mem.push_back(make_pair(type, address));

  for (auto &x : mem) {
    HookMemory::memory_type t = x.first;
    uint32_t addr = x.second;

    switch (m_state.at(addr).first) {
      case STATE_NEW:
        if (t == HookMemory::MEM_WRITE)
          m_state.at(addr).first = STATE_W1;
        break;
      case STATE_W1:
        if (t == HookMemory::MEM_READ)
          m_state.at(addr).first = STATE_R;
        break;
      case STATE_R:
        if (t == HookMemory::MEM_WRITE) {
          m_state.at(addr).second++;
          m_state.at(addr).first = STATE_NEW;
        }
        break;
    }


  }
      
  uint32_t global_count = 0;
  for (auto &x : m_state)
  {
//    cout << hex << x.first  << dec << ':' << x.second.second << endl;
    global_count += x.second.second;
    x.second.second = 0;
  }

  cout << "Current window: " << global_count << endl;

  // file.open("wrw_count.txt", ios::out | ios::app);
  // file << "Current window: " << global_count << endl;
  // file.close();
}

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

  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru") {
      hook_instr_cnt = new HookInstructionCount(emu);
      cout << "Start of the cache" << endl;
  }

  ~MemoryAccess() {
      cout << "End of the cache" << endl;

      //for (auto t : mem)
      //  cout << "Mem: " << t.first << hex << " Address: " << t.second << dec << endl;
      
      // for (auto const& x : m_state)
      // {
          // cout << hex << x.first  << dec << ':' << x.second.second << endl;
      // }

  }

  void run(hook_arg_t *arg) {
    armaddr_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    armaddr_t value = arg->value;

    detectWRWmemoryAccess(address, mem_type, value);
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
