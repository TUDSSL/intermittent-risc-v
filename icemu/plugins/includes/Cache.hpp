#ifndef _CACHE_HPP_
#define _CACHE_HPP_

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
#include "icemu/emu/Function.h"
#include "../includes/DetectWAR.h"

using namespace std;
using namespace icemu;

// The cache block size in number of bytes
#define CACHE_BLOCK_SIZE 4
#define NUM_BITS(_n) ((uint32_t)log2(_n))
#define GET_MASK(_n) ((uint32_t) ((uint64_t)1 << (_n)) - 1)

// Number of instructions after which a checkpoint is mimicked
#define ESTIMATED_CHECKPOINT_INSTR 2500000

// Window size of the dead block detection part
#define DEAD_BLOCK_INSTR_WINDOW 500

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
  uint32_t tag_bits;
  uint32_t set_bits;
  uint32_t offset_bits;
  uint32_t data[CACHE_BLOCK_SIZE / 4]; // divided by 4 as it is uint32_t
  uint64_t last_used;
  uint32_t addr;

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
        std::size_t h1 = std::hash<uint32_t>()(block.offset_bits);
        std::size_t h2 = std::hash<uint32_t>()(block.set_bits);
        std::size_t h3 = std::hash<uint32_t>()(block.tag_bits);
 
        return h1 ^ h2 ^ h3;
    }
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
  int64_t dead_block_nvm_writes;
  uint32_t hint_violations;
  uint32_t hint_given;
};

class Cache {
  private:
    // For initializing
    enum replacement_policy policy;
    uint32_t capacity;  // in bytes
    uint32_t no_of_sets;
    uint8_t no_of_lines;
    
    // Where actual stuffs are stored
    std::vector<CacheSet> sets;
    
    // An unordered map of addr:instr map for reads and writes
    std::unordered_map<uint32_t, uint64_t> last_reads, evicted_mem;
    unordered_set<armaddr_t> hint_reads;

    // Function pointer to be registered back to plugin class
    uint64_t (*get_instr_count)();

  public:
    armaddr_t main_memory_start = 0x10000000;
    armaddr_t main_memory_size = 0x60000;
    CacheStats stats;
    uint32_t instr_count = 0;

  // size in bytes and ways as a number
  void init(uint32_t size, uint32_t ways, enum replacement_policy p)
  {
    capacity = size;
    no_of_lines = ways;
    policy = p;
  
    // Number of cache sets = capacity / (ways * block size)
    no_of_sets = capacity / (no_of_lines * CACHE_BLOCK_SIZE);

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

    // Set to a dummy function. It will be replaced later if the instruction count
    // function is registerd using the appropriate call
    get_instr_count = dummy;
  }

  ~Cache()
  {
    // Printout any stats if there
    cout << "\n\n== STATS ==" << endl;
    cout << "Misses\t\t: " << stats.misses << endl;
    cout << "Hits\t\t: " << stats.hits << endl;
    cout << "NVM reads\t: " << stats.nvm_reads << endl;
    cout << "NVM writes\t: " << stats.nvm_writes << endl;
    cout << "Opt NVM writes\t:" << stats.dead_block_nvm_writes << endl;
    cout << "Cache reads\t: " << stats.cache_reads << endl;
    cout << "Cache writes\t: " << stats.cache_writes << endl;
    cout << "Checkpoints\t: " << stats.checkpoints << endl;
    cout << "Hint violations\t: " << stats.hint_violations << endl;
    cout << "Hints given\t: " << stats.hint_given << endl;
  }

  // Function to be called on eviction of the cache on checkpoint
  void checkpointEviction()
  {
    stats.checkpoints++;

    // Clear the hint sets as the hint does not matter if the
    // checkpoint is between the R and the next R or W.
    hint_reads.clear();

    // Evict all the writes that are a possible war and reset the bits
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            // Clear the was_read for all the lines. This is because in a checkpoint
            // there can be no scenario where the previous read can cause a WAR. So
            // the possibleWar bit not get activated in this case.
            l.was_read = false;
            l.dirty = false;

            // Now if there is an actual case of possibleWar then we need to evict
            // the bit and clear the flag. This should increment the NVM writes.
            if (l.possible_war) {
                l.possible_war = false;
                stats.nvm_writes++;
                stats.dead_block_nvm_writes++;

                // Store the memory that has been evicted. This shall be then checked
                // later to check if a memory that is being written to was previously read
                // and evicted - which can be then reduced the count from the eviction count
                evicted_mem[l.blocks.addr] = get_instr_count();
            }
        }
    }
  }

  // Function to perform the mock cache hint thingy.
  void performMockHintCalculation(armaddr_t addr, enum HookMemory::memory_type type)
  {
      // If there is a read, then put in the last read set
      // if there is a write then check if the read was present in the last read
      // set and if yes then remove it from the last read set
      if (type == HookMemory::MEM_READ) {
          last_reads[addr] = get_instr_count();
      } else {
          bool is_in_reads = !!last_reads.count(addr);
          bool is_in_evicted = !!evicted_mem.count(addr);

          if (is_in_reads && !is_in_evicted)
              last_reads.erase(addr);

          else if (!is_in_reads && is_in_evicted)
              evicted_mem.erase(addr);

          else if (is_in_reads && is_in_evicted) {
              if (last_reads[addr] > evicted_mem[addr])
                  evicted_mem.erase(addr);
              else if (evicted_mem[addr] - last_reads[addr] >= DEAD_BLOCK_INSTR_WINDOW) {
                  last_reads.erase(addr);
                  evicted_mem.erase(addr);
                  stats.dead_block_nvm_writes--;
              }
          }
      }
  }

  uint32_t* run(armaddr_t address, enum HookMemory::memory_type type, armaddr_t *value)
  {
      // Process only valid memory
      if (!((address >= main_memory_start) && (address <= (main_memory_start + main_memory_size))))
          return 0;

      // Offset the main memory start
      auto addr = address - main_memory_start;

      // Fetch the tag, set and offset from the mem address
      auto offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      auto index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);
      auto tag = addr >> (uint32_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));

      // To get an accurate statistic of the number of write evictions
      // without a read before, have to simulate some form of checkpoint.
      // At a checkpoint previous history is cleared.
      instr_count++;
      if (instr_count >= ESTIMATED_CHECKPOINT_INSTR) {
          checkpointEviction();
          instr_count = 0;
      }

      // Checks if there are any collisions. If yes, then something has to be
      // evicted before putting in the current mem access.
      uint8_t collisions = 0;

      // Special check to see if the hints are actually working or not. The logic
      // is that if we saw a READ and the next access to that is a READ - that
      // should be a faulty prediction. In case it is a write then the pattern
      // matches - so correct logic.
      if (type == HookMemory::MEM_READ) {
          if (hint_reads.count(addr) != 0)
            stats.hint_violations++;
      } else {
          if (hint_reads.count(addr) != 0)
            hint_reads.erase(addr);
      }

      // Look for any entry in the Cache line marked by "index"
      for (int i = 0; i < no_of_lines; i++) {
          CacheLine *line = &sets[index].lines[i];

          // In case the cache location has something already, check
          // if it is a hit or a miss. In case it is a miss, increment the
          // collisions. This will enable the eviction to be handled.
          if (line->valid) {
              // Check if the cache actually stores the given memory. If yes
              // then increment the hit stats and perform READ/WRITE conditional
              // logic.
              if (line->blocks.tag_bits == tag) {
                  line->blocks.last_used = CurrentTime_nanoseconds();
                  stats.hits++;
                  
                  // If the memory is a READ, then increment the cache read stats
                  // and set the was_read flag. This flag will then be checked later
                  // to see if the possibleWar can occur or not.
                  if (type == HookMemory::MEM_READ) {
                      stats.cache_reads++;
                      line->was_read = true;
                  } 
                  // Otherwise, mark the cache as dirty. This means that this line
                  // was written to after getting a hit.
                  else {
                      line->dirty = true;
                      stats.cache_writes++;

                      // If this line was previously stored a READ and now it is
                      // storing a WRITE, that means there is a case of possible WAR
                      // Set the bit to indicate that.
                      if (line->was_read == true)
                          line->possible_war = true;
                  }

                  // No need to continue searching if there is a hit
                  break;
              } else
                  collisions++;

          } else {
              // This will be executed only when the cache line is empty, so
              // only the first time. From the next time onwards, only the hits
              // would be executed.
              stats.misses++;

              // Since the cache is empty, store the value.
              line->valid = true;
              line->blocks.offset_bits = offset;
              line->blocks.set_bits = index;
              line->blocks.tag_bits = tag;
              line->blocks.last_used = CurrentTime_nanoseconds();
              line->blocks.addr = addr;
              line->type = type;

              // Same logic as above. If it is a READ then set the was read flag.
              // But no need to check for the possibleWAR flag as this is bound to
              // be just the first read/write.
              if (type == HookMemory::MEM_READ) {
                  line->was_read = true;
                  stats.nvm_reads++;
              } else {
                  line->dirty = true;

                  // A cache write has taken place as the value is written to
                  // the cache and not the nvm.
                  stats.cache_writes++;
                  stats.dead_block_nvm_writes++;
              }

              // No need to keep looking now
              break;
          }
      }

      // Handle the collisions if any
      if (collisions == no_of_lines)
          evict(addr, offset, tag, index, type);

      performMockHintCalculation(addr, type);

      // Return NULL as of now; can be extended to return the data when needed.
      return NULL;
  }

  // Handle collisions.
  void evict(uint32_t addr, uint32_t offset, uint32_t tag, uint32_t index, enum HookMemory::memory_type type)
  {
    // If needs to evict, then the misses needs to be incremented.
    stats.misses++;

    // Get the line to be evicted using the given replacement policy.
    CacheLine *evicted_line;

    switch (policy) {
      case LRU:
        evicted_line = &(*std::min_element(sets[index].lines.begin(), sets[index].lines.end()));
        break;
      case MRU:
        evicted_line = &(*std::max_element(sets[index].lines.begin(), sets[index].lines.end()));
        break;
    }

    // If the memory to be evicted is a READ then just evict and be done with it.
    if (evicted_line->type == HookMemory::MEM_READ) {
        stats.read_evictions++;
    } else {
        // If the line that is being evicted has possible WAR bit set then
        // we need to create a checkpoint.
        if (evicted_line->possible_war)
            checkpointEviction();
    }

    // Add the entry to the evicted memory map here also
    evicted_mem[evicted_line->blocks.addr] = get_instr_count();

    // Now that the eviction has been done, perform the replacement.
    evicted_line->blocks.offset_bits = offset;
    evicted_line->blocks.tag_bits = tag;
    evicted_line->blocks.set_bits = index;
    evicted_line->blocks.last_used = CurrentTime_nanoseconds();
    evicted_line->blocks.addr = addr;
    evicted_line->type = type;

    // Again the same logic. If the memory access is a READ then set the was_read
    // flag to true. Otherwise, check if the access is a write, check if there was
    // a read access earlier. In that case, set the possibleWAR flag to true.
    if (evicted_line->type == HookMemory::MEM_READ) {
      evicted_line->was_read = true;
      stats.nvm_reads++;
    } else {
      evicted_line->dirty = true;
      stats.cache_writes++;

      if (evicted_line->was_read)
        evicted_line->possible_war = true;
    }
  }

  void register_instr_count_fn(uint64_t (*func)())
  {
      get_instr_count = func;
  }

  void print_stats(ofstream &logger)
  {
    stats.dead_block_nvm_writes -= evicted_mem.size();
    logger << "Misses\t\t: " << stats.misses << endl;
    logger << "Hits\t\t: " << stats.hits << endl;
    logger << "NVM reads\t: " << stats.nvm_reads << endl;
    logger << "NVM writes\t: " << stats.nvm_writes << endl;
    logger << "Cache reads\t: " << stats.cache_reads << endl;
    logger << "Cache writes\t: " << stats.cache_writes << endl;
    logger << "Checkpoints\t: " << stats.checkpoints << endl;
    logger << "Opt NVM Writes\t: " << stats.dead_block_nvm_writes << endl;
    logger << "Hint violations\t: " << stats.hint_violations << endl;
    logger << "------------------------------------------------" << endl;
  }

  void log_all_cache_contents(ofstream &logger)
  {
    for (uint32_t i = 0; i < sets.size(); i++) {
      for (auto j = 0; j < sets[i].lines.size(); j++) {
        auto line = &sets[i].lines[j];
        logger << hex << line->blocks.addr << dec << ",";
      }
    }
    logger << endl;
  }

  void applyCompilerHints(uint32_t address)
  {
      // Process only valid memory
      if (!((address >= main_memory_start) && (address <= (main_memory_start + main_memory_size))))
          return;

      // Offset the main memory start
      auto addr = address - main_memory_start;

      // Fetch the tag, set and offset from the mem address
      auto offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      auto index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);
      auto tag = addr >> (uint32_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));

      if (hint_reads.count(addr) == 0)
        hint_reads.insert(addr);
      else
        return;

      // Search for the cache line where the hint has to be given. If found
      // then reset the possibleWAR and the was_read flag. This will ensure that
      // eviction of the given memory causes no checkpoint.
      for (int i = 0; i < no_of_lines; i++) {
          CacheLine *line = &sets[index].lines[i];

          if (line->valid) {
              if (line->blocks.tag_bits == tag) {
                  line->dirty = false;
                  line->possible_war = false;
                  line->was_read = false;
              }
          }
      }

      stats.hint_given++;
  }
};

#endif /* _CACHE_HPP_ */