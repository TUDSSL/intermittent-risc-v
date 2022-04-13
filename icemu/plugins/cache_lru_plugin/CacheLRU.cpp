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

/**
 * Next to do
 * 1. Evict all the dirty writes at once.
 * 2. Get the stats for the number of reads/writes to NVM
 * 3. Create a nice stats class
 * 4. Get the number of checkpoints
 */

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>
#include <chrono>

#include "icemu/emu/Emulator.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Function.h"
#include "../includes/DetectWAR.h"

using namespace std;
using namespace icemu;

// The cache block size in number of bytes
#define CACHE_BLOCK_SIZE 4
#define NUM_BITS(_n) ((uint32_t)log2(_n))
#define GET_MASK(_n) ((uint32_t) ((uint64_t)1 << (_n)) - 1)

// Number of instructions after which a checkpoint is mimicked
#define ESTIMATED_CHECKPOINT_INSTR 0xffffffff

static inline uint64_t CurrentTime_nanoseconds()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>
         (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
} 

/*
 * The smallest unit of a cache that stores the data.
 * A group of blocks form a cache line (or way)
 * A group of cache lines form a cache set
 * A group of cache sets form a cache
 *
 * @tag_bits: Bits to represent the significat memory bits
 * @set_bits: Bits to decide which set of the cache is being used
 * @offset_bits: Bits to decide the byte/word being referred to inside a set
 * @data: The block of data to be stored
 * @last_used: Time it was last used
 *
 * SUM_OF_BITS(tag_bits, set_bits, offset_bits) = Memory address lines. (32 bit in this case)
 */
struct CacheBlock {
  uint32_t tag_bits;
  uint32_t set_bits;
  uint32_t offset_bits;
  uint32_t data[CACHE_BLOCK_SIZE / 4]; // divided by 4 as it is uint32_t
  uint64_t last_used;
};

/* 
 * A cache line that consists of blocks. A 2-way set associative cache will have
 * 2 blocks 
 */
struct CacheLine {
  CacheBlock blocks;
  
  // For cache management
  bool valid;
  bool dirty;
  enum HookMemory::memory_type type;
  bool was_read;
  bool possible_war;
  
  // Overload the < operator to enable LRU
  bool operator<(const CacheLine &other) const {
    return blocks.last_used < other.blocks.last_used;
  }
};

struct CacheSet {
  std::vector<CacheLine> lines;
};

struct CacheStats {
  uint32_t misses;
  uint32_t hits;
  uint32_t read_evictions;
  uint32_t write_evictions;
  uint32_t safe_write_evictions;
  uint32_t war_write_evictions;
  uint32_t nvm_writes;
  uint32_t nvm_reads;
  uint32_t checkpoints;
  uint32_t cache_reads;
  uint32_t cache_writes;
};

ofstream logger;

class Cache {
  public:
    uint32_t capacity;  // in bytes
    uint32_t no_of_sets;
    uint8_t no_of_lines;
    
    std::vector<CacheSet> sets;

    armaddr_t main_memory_start = 0x10000000;
    armaddr_t main_memory_size = 0x60000;

    CacheStats stats;
    uint32_t instr_count = 0;

  Cache() {} // Empty constructor

  // size in bytes and ways as a number
  void init(uint32_t size, uint32_t ways)
  {
    capacity = size;
    no_of_lines = ways;
  
    // Number of cache sets = capacity / (ways * block size)
    no_of_sets = capacity / (no_of_lines * CACHE_BLOCK_SIZE);

    // Initialize the cache
    sets.resize(no_of_sets);
    for (uint32_t i = 0; i < no_of_sets; i++) {
      sets[i].lines.resize(no_of_lines);
      
      for (auto j = 0; j < no_of_lines; j++) {
        auto line = &sets[i].lines[j];
        memset(&line->blocks, 0, sizeof(struct CacheBlock));
        line->valid = false;
        line->dirty = false;
        line->was_read = false;
        line->possible_war = false;
      }
    }

    // Initialize the stats
    memset(&stats, 0, sizeof(struct CacheStats));
  }

  ~Cache()
  {
    // Printout any stats if there
    cout << "\n\n== STATS ==" << endl;
    cout << "Misses\t\t: " << stats.misses << endl;
    cout << "Hits\t\t: " << stats.hits << endl;
    cout << "NVM reads\t: " << stats.nvm_reads << endl;
    cout << "NVM writes\t: " << stats.nvm_writes << endl;
    cout << "Cache reads\t: " << stats.cache_reads << endl;
    cout << "Cache writes\t: " << stats.cache_writes << endl;
    cout << "Checkpoints\t: " << stats.checkpoints << endl;


    logger << "Misses\t\t: " << stats.misses << endl;
    logger << "Hits\t\t: " << stats.hits << endl;
    logger << "NVM reads\t: " << stats.nvm_reads << endl;
    logger << "NVM writes\t: " << stats.nvm_writes << endl;
    logger << "Cache reads\t: " << stats.cache_reads << endl;
    logger << "Cache writes\t: " << stats.cache_writes << endl;
    logger << "Checkpoints\t: " << stats.checkpoints << endl;
 

  }

  uint32_t* LRU2WaySetAssociativeCache(armaddr_t address, enum HookMemory::memory_type type, armaddr_t *value)
  {
      // Process only valid memory
      if (!((address >= main_memory_start) && (address <= (main_memory_start + main_memory_size))))
        return 0;

      if (value) {} // Use it if and when needed

      // Offset the main memory start
      auto addr = address - main_memory_start;

      // Fetch the tag, set and offset from the mem address
      auto offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      auto index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);
      auto tag = addr >> (uint32_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));

      // Debug stuffs
      cout << endl << "Address: " << (addr) << "\t";
  
      // To get an accurate statistic of the number of write evictions
      // without a read before, have to simulate some form of checkpoint.
      // At a checkpoint previous history is cleared.
      instr_count++;
      if (instr_count >= ESTIMATED_CHECKPOINT_INSTR) {
        for (uint32_t i = 0; i < no_of_sets; i++) {
          for (auto j = 0; j < no_of_lines; j++) {
            auto line = &sets[i].lines[j];
            line->was_read = false;
          }
        }
        instr_count = 0;
        // Also need to reset the Memory tracker here
      }

      uint8_t collisions = 0;

      // Look for any entry in the Cache line marked by "index"
      for (int i = 0; i < no_of_lines; i++) {
        CacheLine *line = &sets[index].lines[i];

        cout << "Accessing index: " << index << " way: " << i << endl;

        if (line->valid) {
          if (line->blocks.tag_bits == tag) {
            cout << "Cache Hit" << endl;
            line->blocks.last_used = CurrentTime_nanoseconds();
            stats.hits++;
            
            if (type == HookMemory::MEM_READ)
              stats.cache_reads++;
            else
              stats.cache_writes++;

            break;
          }
          else {
            cout << "Way: " << i << " already has data" << endl;
            collisions++;
          }
        }
        else {
          cout << "Cache Miss" << endl;
          stats.misses++;
          line->valid = true;
          line->blocks.offset_bits = offset;
          line->blocks.set_bits = index;
          line->blocks.tag_bits = tag;
          line->blocks.last_used = CurrentTime_nanoseconds();
          line->type = type;

          if (type == HookMemory::MEM_READ) {
            line->was_read = true;
            stats.nvm_reads++;
          } else {
            line->was_read = false;
            stats.nvm_writes++;
          }

          break;
        }
      }

      // Currently hardcoded to just 2 ways; extend it later if needed;
      if (collisions == no_of_lines) {
        cout << "Cache collision" << endl;
        stats.misses++;

        // Get the least used block
        CacheLine *LRU_line = &(*std::max_element(sets[index].lines.begin(), sets[index].lines.end()));

        // If the memory to be evicted is a READ then just evict and be done with it.
        if (LRU_line->type == HookMemory::MEM_READ) {
          stats.read_evictions++;
          stats.nvm_reads++;
        } else { // if write then evict all writes and increment a checkpoint
          stats.write_evictions++;
          stats.nvm_writes++;

          if (LRU_line->possible_war) {
            stats.checkpoints++;

            // Evict all the writes that are a possible war and reset the bits
            for (CacheSet &s : sets) {
              for (CacheLine &l : s.lines) {
                if (l.possible_war) {
                  l.possible_war = false;
                  l.was_read = false;
                  stats.nvm_writes++;
                }
              }
            }
          }
        }

        // Replace the values with new data
        LRU_line->blocks.offset_bits = offset;
        LRU_line->blocks.tag_bits = tag;
        LRU_line->blocks.set_bits = index;
        LRU_line->blocks.last_used = CurrentTime_nanoseconds();
        LRU_line->type = type;

        // Possible WAR if there was a read somewhere and currently storing write there
        if (LRU_line->was_read == true && type == HookMemory::MEM_WRITE)
          LRU_line->possible_war = true;
        
        // Again, if the new entered memory is a read, then set the bit
        if (LRU_line->type == HookMemory::MEM_READ)
          LRU_line->was_read = true;
      }

      // Return NULL as of now; can be extended to return the data when needed.
      return NULL;
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

// Struct to store the current instruction state
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
    Cache CacheObj = Cache();
    DetectWAR war;
    string filename = "log/default_log";

    enum MemAccessType {
      UNKNOWN = 0,
      MEM_NONE,
      MEM_LOCAL,
      MEM_STACK,
      MEM_GLOBAL,
    };

  MemoryAccess(Emulator &emu) : HookMemory(emu, "cache-lru") {
      hook_instr_cnt = new HookInstructionCount(emu);
      parseLogArguements();
      parseCacheArguements();
      logger.open(filename.c_str(), ios::out | ios::trunc);
      cout << "Start of the cache" << endl;
  }

  ~MemoryAccess() {
      cout << "End of the cache" << endl;
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

  void run(hook_arg_t *arg) {
    armaddr_t address = arg->address;
    enum memory_type mem_type = arg->mem_type;
    armaddr_t value = arg->value;

    InstructionState istate = {hook_instr_cnt->pc,
                              hook_instr_cnt->getInstructionCount(),
                              arg->address,
                              arg->size
                            };

    // Call the cache
    CacheObj.LRU2WaySetAssociativeCache(address, mem_type, &value);

  }
    
  void parseLogArguements() { 
      string argument_name = "log-file=";
      for (const auto &a : getEmulator().getPluginArguments().getArgs()) {
        auto pos = a.find(argument_name);
        if (pos != string::npos) {
          filename = "log/" + a.substr(pos+argument_name.length());
        }
      }

  }
  
  void parseCacheArguements() {
      uint32_t size = 512, lines = 2;
      string arg1 = "cache-size=", arg2 = "cache-lines=";
      for (const auto &a : getEmulator().getPluginArguments().getArgs()) {
        auto pos1 = a.find(arg1);
        if (pos1 != string::npos) {
          size = std::stoul(a.substr(pos1+arg1.length()));
        }

        auto pos2 = a.find(arg2);
        if (pos2 != string::npos) {
          lines = std::stoul(a.substr(pos2+arg2.length()));
        }
      }

      CacheObj.init(size, lines);
      filename += "_" + std::to_string(size) + "_" + std::to_string(lines);
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
