#ifndef _CACHE_MEM_HPP_
#define _CACHE_MEM_HPP_

#include <map>
#include <unordered_map>
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
#include "icemu/emu/Architecture.h"
#include "../includes/DetectWAR.h"

using namespace std;
using namespace icemu;

// The cache block size in number of bytes
#define NO_OF_CACHE_BLOCKS 8 // Equates to the cache block size of 8 bytes
#define CACHE_BLOCK_SIZE NO_OF_CACHE_BLOCKS
#define NUM_BITS(_n) ((uint32_t)log2(_n))
#define GET_MASK(_n) ((uint32_t) ((uint64_t)1 << (_n)) - 1)

static const bool PRINT_MEMORY_DIFF = true;
static const bool MIMIC_CHECKPOINT_TAKEN = true;

static inline uint64_t CurrentTime_nanoseconds()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>
         (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

static inline uint64_t dummy() { return 0; }

enum replacement_policy {
  LRU,
  MRU
};

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
 * @addr: Store the actual address - for easy reference
 *
 * SUM_OF_BITS(tag_bits, set_bits, offset_bits) = Memory address lines. (32 bit in this case)
 */
struct CacheBlock {
  address_t tag_bits;
  address_t set_bits;
  address_t offset_bits;
  address_t data; // actually store the data in blocks of bytes
  address_t last_used;
  address_t addr;
  address_t size;

  // Overload the = operator to compare evicted blocks
  bool operator==(const CacheBlock &other) const {
    return (tag_bits == other.tag_bits &&
           set_bits == other.set_bits &&
           offset_bits == other.offset_bits);
  }

};

// Hash function for the evicted_blocks unordered map
struct hash_fn
{
    size_t operator()(const CacheBlock &block) const
    {
        std::size_t h1 = std::hash<address_t>()(block.offset_bits);
        std::size_t h2 = std::hash<address_t>()(block.set_bits);
        std::size_t h3 = std::hash<address_t>()(block.tag_bits);
 
        return h1 ^ h2 ^ h3;
    }
};

/* 
 * A cache line that consists of blocks. A 2-way set associative cache will have
 * 2 blocks. Each block will be then CACHE_BLOCK_SIZE big
 */
struct CacheLine {
  CacheBlock blocks;
  
  // For cache management
  bool valid;
  bool dirty;
  // enum HookMemory::memory_type type;
  bool was_read;
  bool possible_war;
  
  // Overload the < operator to enable LRU/MRU
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
  int64_t dead_block_nvm_writes;
  uint32_t hint_violations;
  uint32_t hint_given;
};

class icemu::Memory;

class Cache {
  private:
    enum replacement_policy policy;
    
    uint32_t capacity;  // in bytes
    uint32_t no_of_sets;
    uint8_t no_of_lines;

    std::vector<CacheSet> sets;
    
    // Function pointer to be registered back to plugin class
    uint64_t (*get_instr_count)();

    memseg_t *MainMemSegment = nullptr;
    uint8_t *ShadowMem = nullptr;

  public:
    CacheStats stats;
    address_t instr_count = 0;

    icemu::Memory *mem;

  // Size of the cache = 512 bytes
  // Each data block size = 8 bytes
  // So can store 512 / 8 bytes = 64 entries
  // now no of ways = 2
  // so no of sets = 32 - each set has 2 ways - each way has 8 bytes of data
  // Now address = 32 or 64 bits
  // First 3 bytes = doesn't matter becoz results in the block size
  // Next 5 bits = determine which of the 32 sets
  // Rest bits just used to compare
  void init(uint32_t size, uint32_t ways, enum replacement_policy p, icemu::Memory &emu_mem)
  {
    mem = &emu_mem;
    capacity = size;
    no_of_lines = ways;
    policy = p;
  
    // Number of cache sets = capacity / (ways * block size)
    no_of_sets = capacity / (no_of_lines * NO_OF_CACHE_BLOCKS);

    // Initialize the cache
    sets.resize(no_of_sets);
    // cout << "SETS: " << no_of_sets << endl;
    // cout << "Lines: " << no_of_lines << endl;

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

    // Initialize the shadow mem
    init_mem();

    // Set to a dummy function. It will be replaced later if the instruction count
    // function is registerd using the appropriate call
    get_instr_count = dummy;
  }

  void init_mem()
  {
    auto code_entrypoint = mem->entrypoint;

    // Get the memory segment holding the main code (assume it also holds the RAM)
    MainMemSegment = mem->find(code_entrypoint);
    assert(MainMemSegment != nullptr);

    // Create shadow memory
    ShadowMem = new uint8_t[MainMemSegment->length];
    assert(ShadowMem != nullptr);

    // Populate the shadow memory
    memcpy(ShadowMem, MainMemSegment->data, MainMemSegment->length);
  }

  ~Cache()
  {

    // Evict all the writes that are a possible war and reset the bits
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            if (l.dirty) {
              shadowWrite(l.blocks.addr, l.blocks.data, l.blocks.size);
            }
        }
    }

    compareMemory(); // a final check
    delete[] ShadowMem;
  }

  bool compareMemory() {
    // First do a fast memcmp (assume it's optimized)
    int compareValue = memcmp(ShadowMem, MainMemSegment->data, MainMemSegment->length);
    if (compareValue == 0) {
      // Memory is the same
      return true;
    }

    cout << "\tCompare value: " << (int)compareValue << endl;
    // Something is different according to `memcmp`, check byte-per-byte
    bool same = true;
    for (size_t i = 0; i < MainMemSegment->length; i++) {
      if (ShadowMem[i] != MainMemSegment->data[i]) {
        // Memory is different
        same = false;

        if (PRINT_MEMORY_DIFF) {
          address_t addr = MainMemSegment->origin + i;
          address_t emu_val = MainMemSegment->data[i];
          address_t shadow_val = ShadowMem[i];
          cerr << "[mem] memory location at 0x" << hex << addr
               << " differ - Emulator: 0x" << emu_val << " Shadow: 0x"
               << shadow_val << dec << endl;
        }
      }
    }
    return same;
  }

  void shadowWrite(address_t address, address_t value, address_t size) {
    stats.nvm_writes++;
    address_t address_idx = address - MainMemSegment->origin;
    for (address_t i=0; i<size; i++) {
      uint64_t byte = (value >>(8*i)) & 0xFF; // Get the bytes
      ShadowMem[address_idx+i] = byte;
    }
  }

  // Function to be called on eviction of the cache on checkpoint
  void checkpointEviction()
  {
    stats.checkpoints++;
    cout << "Creating checkpoint: " << stats.checkpoints << endl;

    // Evict all the writes that are a possible war and reset the bits
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            if (l.dirty) {
                // Perform the actual write to the memory
                shadowWrite(l.blocks.addr, l.blocks.data, l.blocks.size);
            }
            
            // Reset all bits
            l.dirty = false;
            l.was_read = false;
            l.possible_war = false;

        }
    }

    compareMemory();
  }

  uint32_t* run(address_t address, enum HookMemory::memory_type type, address_t *value, address_t size)
  {
      assert(size == 1 || size == 2 || size == 4 || size == 8);

      // Offset the main memory start
      address_t addr = address;// - main_memory_start;

      // Fetch the tag, set and offset from the mem address
      address_t offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      address_t index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);
      address_t tag = addr >> (address_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));

      assert(index <= no_of_sets);
      // cout << "Address: " << addr << "\tTag: " << tag << "\tIndex: " << index << "\tOffset: " << offset << endl;
      // cout << "Memory type: " << type << "\t size: " << size << hex << "\t address: " << address << dec << "\tData: " << *value;

      // compareMemory();

      // switch(type) {
      //   case HookMemory::MEM_READ:
      //     // Nothing
      //     break;
      //   case HookMemory::MEM_WRITE:
      //     shadowWrite(addr, *value, size);
      //     break;
      // }

      // Checks if there are any collisions. If yes, then something has to be
      // evicted before putting in the current mem access.
      uint8_t collisions = 0;

      // Look for any entry in the Cache line marked by "index"
      for (int i = 0; i < no_of_lines; i++) {
          CacheLine *line = &sets[index].lines[i];

          if (line->valid) {
              if (line->blocks.tag_bits == tag && line->blocks.offset_bits == offset) {
                  line->blocks.last_used = CurrentTime_nanoseconds();
                  stats.hits++;
                  
                  switch (type) {
                    case HookMemory::MEM_READ:
                      stats.cache_reads++;
                      line->was_read = true;
                      break;
 
                    case HookMemory::MEM_WRITE:
                      line->dirty = true;
                      stats.cache_writes++;

                      line->blocks.data = *value;
                      line->blocks.size = size;
                      
                      if (line->was_read == true)
                          line->possible_war = true;

                      break;
                  }

                  break;
              } else
                collisions++;

          } else {
              stats.misses++;

              line->valid = true;
              line->blocks.offset_bits = offset;
              line->blocks.set_bits = index;
              line->blocks.tag_bits = tag;
              line->blocks.last_used = CurrentTime_nanoseconds();
              line->blocks.addr = addr;

              line->blocks.data = *value;
              line->blocks.size = size;

              switch (type) {
                case HookMemory::MEM_READ:
                  line->was_read = true;
                  stats.nvm_reads++;
                  break;

                case HookMemory::MEM_WRITE:
                  line->dirty = true;
                  stats.cache_writes++;

                  // Since this is the first time the cache is being accessed
                  // this should never be possible.
                  if (line->was_read == true)
                    assert(false);

                  break;
              }

              break;
          }
      }

      assert(collisions <= no_of_lines);

      // Handle the collisions if any
      if (collisions == no_of_lines)
          evict(addr, offset, tag, index, value, type, size);

      // Return NULL as of now; can be extended to return the data when needed.
      return NULL;
  }

  // Handle collisions.
  void evict(address_t addr, address_t offset, address_t tag, address_t index,
             address_t *value, enum HookMemory::memory_type type, address_t size)
  {
    // If needs to evict, then the misses needs to be incremented.
    stats.misses++;

    // Get the line to be evicted using the given replacement policy.
    CacheLine *evicted_line = nullptr;

    switch (policy) {
      case LRU:
        evicted_line = &(*std::min_element(sets[index].lines.begin(), sets[index].lines.end()));
        break;
      case MRU:
        evicted_line = &(*std::max_element(sets[index].lines.begin(), sets[index].lines.end()));
        break;
    }

    assert(evicted_line != nullptr);


    if (evicted_line->dirty) {
      shadowWrite(evicted_line->blocks.addr, evicted_line->blocks.data, evicted_line->blocks.size);
      if (evicted_line->possible_war) {
        checkpointEviction();
      }
    }

    // switch (evicted_line->type) {
    //   case HookMemory::MEM_READ:
    //     stats.read_evictions++;
    //     break;
      
    //   case HookMemory::MEM_WRITE:
    //     stats.write_evictions++;
    //     if (evicted_line->possible_war) {
    //       checkpointEviction();
    //     } else {
    //       shadowWrite(evicted_line->blocks.addr, evicted_line->blocks.data, evicted_line->blocks.size);
    //     }
    //     break;
    // }

    // Now that the eviction has been done, perform the replacement.
    evicted_line->blocks.offset_bits = offset;
    evicted_line->blocks.tag_bits = tag;
    evicted_line->blocks.set_bits = index;
    evicted_line->blocks.last_used = CurrentTime_nanoseconds();
    evicted_line->blocks.addr = addr;
    // evicted_line->type = type;

    // If evicted then reset all the flags
    evicted_line->was_read = false;
    evicted_line->dirty = false;
    evicted_line->possible_war = false;

    // Replace the data in the memory
    evicted_line->blocks.data = *value;
    evicted_line->blocks.size = size;

    // Again the same logic. If the memory access is a READ then set the was_read
    // flag to true. Otherwise, check if the access is a write, check if there was
    // a read access earlier. In that case, set the possibleWAR flag to true.
    switch (type) {
      case HookMemory::MEM_READ:
        evicted_line->was_read = true;
        stats.nvm_reads++;
        break;

      case HookMemory::MEM_WRITE:
        evicted_line->dirty = true;
        stats.cache_writes++;

        if (evicted_line->was_read) {
          assert(false);
          evicted_line->possible_war = true;
        }
        break;
    }
  }

  void register_instr_count_fn(uint64_t (*func)())
  {
      get_instr_count = func;
  }

  void print_stats(ofstream &logger)
  {
      logger << "Print to log here";
  }

  void log_all_cache_contents(ofstream &logger)
  {
    for (address_t i = 0; i < sets.size(); i++) {
      for (address_t j = 0; j < sets[i].lines.size(); j++) {
        auto line = &sets[i].lines[j];
        logger << hex << line->blocks.addr << dec << ",";
      }
    }
    logger << endl;
  }

  void applyCompilerHints(uint32_t address)
  {
      // Offset the main memory start
      auto addr = address;

      // Fetch the tag, set and offset from the mem address
      auto offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      auto index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);
      auto tag = addr >> (uint32_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));
      offset = offset - offset + offset;

      // Search for the cache line where the hint has to be given. If found
      // then reset the possibleWAR and the was_read flag. This will ensure that
      // eviction of the given memory causes no checkpoint.
      for (CacheSet &s : sets) {
          for (CacheLine &line : s.lines) {
            if (line.valid) {
                if (line.blocks.tag_bits == tag) {
                    line.dirty = false;
                    line.possible_war = false;
                    line.was_read = false;
                }
            }
          }
      }

      // Do we need this?
      stats.hint_given++;
  }
};

#endif /* _CACHE_MEM_HPP_ */