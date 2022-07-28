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
#include "icemu/emu/Memory.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Architecture.h"

// Local includes
#include "../includes/DetectWAR.h"
#include "../includes/ShadowMemory.hpp"
#include "../includes/Stats.hpp"
#include "../includes/CycleCostCalculator.hpp"

using namespace std;
using namespace icemu;

// The cache block size in number of bytes
#define NO_OF_CACHE_BLOCKS 8 // Equates to the cache block size of 8 bytes
#define CACHE_BLOCK_SIZE NO_OF_CACHE_BLOCKS
#define NUM_BITS(_n) ((uint32_t)log2(_n))
#define GET_MASK(_n) ((uint32_t) ((uint64_t)1 << (_n)) - 1)

static const bool MIMIC_CHECKPOINT_TAKEN = true;
static const float DIRTY_RATIO_THRESHOLD = 10.0;

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

enum CheckpointReason {
  CHECKPOINT_DUE_TO_WAR,
  CHECKPOINT_DUE_TO_DIRTY,
  CHECKPOINT_DUE_TO_PERIOD
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

class Cache {
  private:    
    // Cache metadata
    enum replacement_policy policy;
    uint32_t capacity;  // in bytes
    uint32_t no_of_sets;
    uint8_t no_of_lines;
    float dirty_ratio;

    // Cache sets
    std::vector<CacheSet> sets;

    // Helper stuffs
    Stats stats;
    ShadowMemory nvm;
    icemu::Memory *mem;
    CycleCost cost;

  public:
  // No need for constructor now
  Cache() = default;

  ~Cache()
  {

    // Evict all the writes that are a possible war and reset the bits
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            if (l.dirty) {
              cacheNVMwrite(l.blocks.addr, l.blocks.data, l.blocks.size, false);
            }
        }
    }

    cout << "Final check: \t";
    nvm.compareMemory(); // a final check

    stats.printStats();
    cout << "Exact cycle count: " << cost.calculateCost(stats, 0) << endl;
  }

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
    dirty_ratio = 0;
  
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

    // Initialize the shadow mem
    nvm.initMem(mem);
  }

  void createCheckpoint(enum CheckpointReason reason)
  {
    stats.checkpoint.checkpoints++;

    switch (reason)
    {
      case CHECKPOINT_DUE_TO_WAR:
        stats.checkpoint.due_to_war++;
        break;
      case CHECKPOINT_DUE_TO_DIRTY:
        stats.checkpoint.due_to_dirty++;
        break;
      case CHECKPOINT_DUE_TO_PERIOD:
        stats.checkpoint.due_to_period++;
        break;
    }

    checkpointEviction();
  }

  // Function to be called on eviction of the cache on checkpoint
  void checkpointEviction()
  {
    // Evict all the writes that are a possible war and reset the bits
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            if (l.dirty) {
                // Perform the actual write to the memory
                cacheNVMwrite(l.blocks.addr, l.blocks.data, l.blocks.size, true);
            }
            
            // Reset all bits
            clearDirty(l);
            l.was_read = false;
            l.possible_war = false;
        }
    }

    nvm.compareMemory();
  }

  // Set the dirty bit and also update the dirty ratio.
  void setDirty(CacheLine &l)
  {
    if (!l.dirty) {
      l.dirty = true;
      dirty_ratio = ((dirty_ratio * capacity) + 1) / capacity;
    }
  }

  void clearDirty(CacheLine &l)
  {
    if (l.dirty) {
      l.dirty = false;
      dirty_ratio = ((dirty_ratio * capacity) - 1) / capacity;
    }
  }

  bool checkDirtyRatio()
  {
    stats.update_dirty_ratio(dirty_ratio);

    if (dirty_ratio > DIRTY_RATIO_THRESHOLD) {
      createCheckpoint(CHECKPOINT_DUE_TO_DIRTY);
      return true;
    }

    return false;
  }

  void cacheNVMwrite(address_t address, address_t value, address_t size, bool doesCountForStats)
  {
    if (doesCountForStats)
      stats.nvm.nvm_writes++;
    
    nvm.shadowWrite(address, value, size);
  }

  void normalNVMAccess(enum HookMemory::memory_type type)
  {
    switch(type) {
      case HookMemory::MEM_READ:
        stats.nvm.nvm_reads_without_cache++;
        break;
      case HookMemory::MEM_WRITE:
        stats.nvm.nvm_writes_without_cache++;
        break;
    }
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

      // cout << "dirty_ratio:\t" << dirty_ratio * capacity << endl;
      if (checkDirtyRatio())
        cout << "Creating checkpoint due to dirty ratio" << endl;

      // Dirty ratio should never go negative
      assert(dirty_ratio >= 0);

      // stats for normal nvm
      normalNVMAccess(type);

      // Checks if there are any collisions. If yes, then something has to be
      // evicted before putting in the current mem access.
      uint8_t collisions = 0;

      // Look for any entry in the Cache line marked by "index"
      for (int i = 0; i < no_of_lines; i++) {
          CacheLine &line = sets[index].lines[i];

          if (line.valid) {
              if (line.blocks.tag_bits == tag && line.blocks.offset_bits == offset) {
                  line.blocks.last_used = CurrentTime_nanoseconds();
                  stats.cache.hits++;
                  
                  switch (type) {
                    case HookMemory::MEM_READ:
                      stats.cache.reads++;
                      line.was_read = true;
                      break;
 
                    case HookMemory::MEM_WRITE:
                      setDirty(line);
                      stats.cache.writes++;

                      line.blocks.data = *value;
                      line.blocks.size = size;
                      
                      if (line.was_read == true)
                          line.possible_war = true;

                      break;
                  }

                  break;
              } else
                collisions++;

          } else {
              stats.cache.misses++;

              // Store the values. Only place where valid is set true
              line.valid = true;
              line.blocks.offset_bits = offset;
              line.blocks.set_bits = index;
              line.blocks.tag_bits = tag;
              line.blocks.last_used = CurrentTime_nanoseconds();
              line.blocks.addr = addr;

              // Store the data and the size
              line.blocks.data = *value;
              line.blocks.size = size;

              cacheResetEntry(line, type);

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

  void cacheResetEntry(CacheLine &line, enum HookMemory::memory_type type)
  {
    switch (type) {
      case HookMemory::MEM_READ:
        line.was_read = true;
        stats.nvm.nvm_reads++;
        break;

      case HookMemory::MEM_WRITE:
        setDirty(line);
        stats.cache.writes++;

        assert(line.possible_war == false);
        assert(line.was_read == false);
        break;
    }
  }

  // Handle collisions.
  void evict(address_t addr, address_t offset, address_t tag, address_t index,
             address_t *value, enum HookMemory::memory_type type, address_t size)
  {
    // If needs to evict, then the misses needs to be incremented.
    stats.cache.misses++;

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

    // Perform the eviction
    if (evicted_line->dirty) {
      if (evicted_line->possible_war)
        createCheckpoint(CHECKPOINT_DUE_TO_WAR);
      else {
        cacheNVMwrite(evicted_line->blocks.addr, evicted_line->blocks.data, evicted_line->blocks.size, true);
        clearDirty(*evicted_line);
      }
    }

    // Now that the eviction has been done, perform the replacement.
    evicted_line->blocks.offset_bits = offset;
    evicted_line->blocks.tag_bits = tag;
    evicted_line->blocks.set_bits = index;
    evicted_line->blocks.last_used = CurrentTime_nanoseconds();
    evicted_line->blocks.addr = addr;

    // If evicted then reset all the flags
    evicted_line->was_read = false;
    evicted_line->possible_war = false;

    // Replace the data in the memory
    evicted_line->blocks.data = *value;
    evicted_line->blocks.size = size;

    // Again the same logic. If the memory access is a READ then set the was_read
    // flag to true.
    cacheResetEntry(*evicted_line, type);
  }

  void printStats(ofstream &logger)
  {
      (void)logger;
      // stats.logStats(logger);
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

      // Search for the cache line where the hint has to be given. If found
      // then reset the possibleWAR and the was_read flag. This will ensure that
      // eviction of the given memory causes no checkpoint.
      for (CacheSet &s : sets) {
          for (CacheLine &line : s.lines) {
            if (line.valid) {
                if (line.blocks.tag_bits == tag && line.blocks.offset_bits == offset) {
                    clearDirty(line);
                    line.possible_war = false;
                    line.was_read = false;
                }
            }
          }
      }

      // Do we need this?
      stats.misc.hints_given++;
  }
};

#endif /* _CACHE_MEM_HPP_ */