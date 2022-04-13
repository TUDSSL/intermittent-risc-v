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
#include <set>
#include <map>
#include <unordered_set>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Function.h"
#include "../includes/CycleCounter.h"

using namespace std;
using namespace icemu;

static const uint32_t CHECKPOINT_INTERVAL = 200000;
ofstream file;

class DetectWAR{
  private:
    unordered_set<armaddr_t> reads, writes;

  public:
    uint32_t checkpoints;

    DetectWAR()
    {
      reads.clear();
      writes.clear();
      checkpoints = 0;
    }

    ~DetectWAR() = default;

    bool isWAR(armaddr_t addr, HookMemory::memory_type type)
    {
      bool possibleWAR = false;
      auto readLoc = reads.find(addr);
      auto writeLoc = writes.find(addr);

      // Check if the reads is in the reads list
      bool isInReads = (readLoc != reads.end());
      // Check if the reads is in the writes list
      bool isInWrites = (writeLoc != writes.end());

      switch (type) {
        // If the current access type is read, just put it in the list
        case HookMemory::MEM_READ:
          if (isInReads)
            reads.erase(readLoc);
          reads.insert(addr);
          break;

        // If the current type is write, do stuffs
        case HookMemory::MEM_WRITE:
          // WAR scenario - R...W access
          if (isInReads && !isInWrites) {
            cout << "R...W" << endl;
            possibleWAR = true;
            reset();
          }
          // Safe scenario - W...W or W..R..W access
          else if (isInReads || isInWrites) {
            cout << "W...W or W..R..W" << endl;
            possibleWAR = false;
            writes.erase(writeLoc);
          }
          // Neither in read or write - new entry.
          else {
            cout << "Just W" << endl;
            possibleWAR = false;
          }

          // Insert the write
          writes.insert(addr);
          break;
      }

      return possibleWAR;
    }

    void reset()
    {
      checkpoints++;
      reads.clear();
      writes.clear();
    }
};

// TODO: Need a way to get information from other hooks
class HookInstructionCount : public HookCode {
  private:
   // Config
   bool use_cycle_count = true;
   const char func_type = 2; // magic ELF function type
   uint64_t instruction_count = 0;

   CycleCounter cycleCounter;

  public:
    uint64_t pc = 0;
    uint64_t count = 0;

    const string unknown_function_str = "UNKNOWN_FUNCTION";
    struct FunctionFrame {
      const string *function_name = nullptr;
      armaddr_t function_address = 0;
      uint64_t function_entry_icount = 0;
      armaddr_t sp_function_entry = 0;
      armaddr_t LR = 0;
    };
    list<FunctionFrame> callstack;

    armaddr_t estack;

    // A boolean that is true on the first instruction of a new function
    // NB. This is hacky, but new_function acts like an ISR flag
    // it needs to be manually reset after reading
    bool new_function = true;
    list<FunctionFrame> function_entries; // is cleared by the HookIdempotencyStatistics class

    void clearNewFunction() {
      new_function = false;
      function_entries.clear();
    }

    // Stores addresses found in a function
    map<armaddr_t, const string *> function_map;
    // Stores the function and the starting address
    map<armaddr_t, const string *> function_entry_map;

    // A map holding ALL executed instructions and the number of times they have
    // been executed
    const bool track_instruction_execution = true;
    map<armaddr_t, uint64_t> instruction_execution_map;

    explicit HookInstructionCount(Emulator &emu) : HookCode(emu, "stack-war"),
                                          cycleCounter(emu)
    {
      auto &symbols = getEmulator().getMemory().getSymbols();
      for (const auto &sym : symbols.symbols) {
        if (sym.type == func_type) {
          // cout << "Func addr: " << sym.address << " - " << sym.getFuncAddr() << endl;
          // cout << "Func name: " << sym.name << endl;
          // cout << "Func size: " << sym.size << endl;

          // To check for function entries only
          function_entry_map[sym.getFuncAddr()] = &sym.name;


          // Add ALL the possible addresses in the function to the map
          // Return the function name if it's in any of the addresses in the
          // function.
          // Assmumtions:
          //  * All opcodes are 16-bit (2 bytes) or more in multiple
          //  * Functions are continious (we use the size for functions)
          for (armaddr_t faddr = sym.getFuncAddr();
               faddr < sym.getFuncAddr() + sym.size;
               faddr += 2) {
            function_map[faddr] = &sym.name;
          }
        }
      }

      auto estack_sym = symbols.get("_estack");
      estack = estack_sym->address;
      cout << "Estack at: " << estack << endl;
    }

    ~HookInstructionCount() override
    {
      cout << "Call stack size at the end of the program: " << callstack.size()
           << " (should be 2)" << endl;
    }

    // Return the current instruction count
    uint64_t getInstructionCount()
    {
      if (use_cycle_count == true)
        return cycleCounter.cycleCount();

      return instruction_count;
    }

    // Return the name of the function that the addr is the start of
    const string *isFunctionEntry(armaddr_t addr)
    {
      auto f = function_entry_map.find(addr);
      if (f != function_entry_map.end())
        return f->second;

      return nullptr;
    }

    // Is the instruction at the address a function?
    const string *inFunction(armaddr_t addr)
    {
      auto f = function_map.find(addr);
      if (f == function_map.end())
        return nullptr;

      return f->second;
    }

    void trackFunctions(armaddr_t addr)
    {
      // Check if this is a function entry
      // If so, we mark it as such
      const string *function_entry = isFunctionEntry(addr);

      if (function_entry != nullptr) {
        // cout << "Address: " << addr << " Function entry: " << *function_entry << endl;

        struct FunctionFrame callframe;

        callframe.function_name = function_entry;
        callframe.function_address = addr;
        callframe.function_entry_icount = getInstructionCount();
        callframe.sp_function_entry = getEmulator().getRegisters().get(Registers::SP);
        callframe.LR = getEmulator().getRegisters().get(Registers::LR) & ~0x1;

        callstack.push_back(callframe);
        //cout << "callframe size: " << callstack.size() << endl;
        //cout << "LR: " << callframe.LR << endl;

        // Set new function flag
        function_entries.push_back(callframe);
        new_function = true;

        return;
      }

      if (callstack.back().LR == addr) {
        // We left the function and are back at the LR
        // We pop stack frames until we are the current function
        // This is because a function can use a "normal" branch to jump to the
        // start of the function. If that's the case we DO detect it in
        // the function entry check. So we need to unwind here if that happened.
        const string *function_current = inFunction(addr);
        while (callstack.back().function_name != function_current)
          callstack.pop_back();

        // cout << "Function exit to: " << *callstack.back().function_name << endl;
        // cout << "callframe size: " << callstack.size() << endl;
      }

      // Else we are somewhere in the same function
    }

    void run(hook_arg_t *arg)
    {
      (void)arg;  // Don't care
      instruction_count += 1;
      if (use_cycle_count)
        cycleCounter.add(arg->address, arg->size);

      pc = arg->address;

      // Track the functions
      trackFunctions(arg->address);

      // Track the number of times this instruction was executed
      if (track_instruction_execution)
        instruction_execution_map[pc] += 1;
    }
};

struct InstructionState {
  uint64_t pc;
  uint64_t icount;
  armaddr_t mem_address;
  armaddr_t mem_size;
};

// TODO: Need a way to get information from other hooks
class MemoryAccess : public HookMemory {
  public:
    HookInstructionCount *hook_instr_cnt;
    DetectWAR war; bool first = true;

    enum MemAccessType {
      UNKNOWN = 0,
      MEM_NONE,
      MEM_LOCAL,
      MEM_STACK,
      MEM_GLOBAL,
    };

    int stats[5] = {0,0,0,0,0};
    int stats_no_war[5] = {0,0,0,0,0};


  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru") \
  {
      hook_instr_cnt = new HookInstructionCount(emu);
      file.open("plugins/stats/stats_memory_tracker/war_stack", ios::out | ios::app);
      cout << "Start of the cache" << endl;
  }

  ~MemoryAccess()
  {
      cout << "======== WARs at different regions ========" << endl;
      cout << "Unknown:  " << stats[0] << endl;
      cout << "None:     " << stats[1] << endl;
      cout << "Local:    " << stats[2] << endl;
      cout << "Stack:    " << stats[3] << endl;
      cout << "Global:   " << stats[4] << endl;
      cout << "CPs:      " << war.checkpoints << endl;
      cout << "End of the cache" << endl;

    
      file << "Unknown:  " << stats_no_war[0] << "\t" << stats[0] << endl;
      file << "None:     " << stats_no_war[1] << "\t" << stats[1] << endl;
      file << "Local:    " << stats_no_war[2] << "\t" << stats[2] << endl;
      file << "Stack:    " << stats_no_war[3] << "\t" << stats[3] << endl;
      file << "Global:   " << stats_no_war[4] << "\t" << stats[4] << endl;
      file << "CPs:      " << war.checkpoints << endl;

      // for (auto &a : hook_instr_cnt->function_entry_map)
      //   cout << *a.second << "::" << hex << a.first << dec << endl;
  }

  enum MemAccessType getMemAccessType(InstructionState &istate)
  {
    auto address = istate.mem_address;
    auto estack_sp = hook_instr_cnt->estack;
    auto f_entry_sp = hook_instr_cnt->callstack.back().sp_function_entry;
    auto f_current_sp = getEmulator().getRegisters().get(Registers::SP);

    // Memory access is local if the address is larger than the current stack
    // pointer, but below the entry sp
    if (address >= f_current_sp && address < f_entry_sp)
      return MEM_LOCAL;

    // Memory access is stack if it's larger than the current stack and pointer
    // but below the _estack (end of the stack)
    else if (address >= f_current_sp && address < estack_sp)
      return MEM_STACK;

    // Other memory accesses we regard as "Global"
    else
      return MEM_GLOBAL;
  }

// 

  void run(hook_arg_t *arg)
  {
    if (first) {
      first = false;
      return;
    }

    armaddr_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    armaddr_t value = arg->value;

    InstructionState istate = {hook_instr_cnt->pc,
                               hook_instr_cnt->getInstructionCount(),
                               arg->address,
                               arg->size
                              };


    auto type = getMemAccessType(istate);
    // cout << "Mem access type: " << type << endl;

    if (hook_instr_cnt->getInstructionCount() % CHECKPOINT_INTERVAL == 0)
      war.reset();

    stats_no_war[type]++;
    if (war.isWAR(address, mem_type)) {
       cout << "WAR detected" << endl;
       stats[type]++;
    }
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
