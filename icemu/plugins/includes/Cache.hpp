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

// Try to make the instr based checkpioints implemented properly
// so that it counts the stats too. This might lead to reduction in the
// NVM writes and also the number of checkpoints.

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
    cout << "SETS: " << no_of_sets << endl;
    cout << "Lines: " << no_of_lines << endl;

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
  }

  uint32_t* run(armaddr_t address, enum HookMemory::memory_type type, armaddr_t *value)
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
    
        // To get an accurate statistic of the number of write evictions
        // without a read before, have to simulate some form of checkpoint.
        // At a checkpoint previous history is cleared.
        instr_count++;
        if (instr_count >= ESTIMATED_CHECKPOINT_INSTR) {
            // Evict all the writes that are a possible war and reset the bits
            for (CacheSet &s : sets) {
                for (CacheLine &l : s.lines) {
                    if (l.possible_war) {
                        l.possible_war = false;
                        l.was_read = false;
                        stats.nvm_writes++;
                        stats.dead_block_nvm_writes++;

                        // Store the memory that has been evicted. This shall be then checked
                        // later to check if a memory that is being written to was previously read
                        // and evicted - which can be then reduced the count from the eviction count
                        evicted_mem[l.blocks.addr] = get_instr_count();
                    }
                }
            }
            instr_count = 0;
        }

        uint8_t collisions = 0;

        // Look for any entry in the Cache line marked by "index"
        for (int i = 0; i < no_of_lines; i++) {
            CacheLine *line = &sets[index].lines[i];

            if (line->valid) {
                if (line->blocks.tag_bits == tag) {
                    line->blocks.last_used = CurrentTime_nanoseconds();
                    stats.hits++;
                    
                    if (type == HookMemory::MEM_READ)
                        stats.cache_reads++;
                    else
                        stats.cache_writes++;

                    break;
                }
                else {
                    collisions++;
                }
            }
            else {
                // cout << "Cache Miss" << endl;
                stats.misses++;
                line->valid = true;
                line->blocks.offset_bits = offset;
                line->blocks.set_bits = index;
                line->blocks.tag_bits = tag;
                line->blocks.last_used = CurrentTime_nanoseconds();
                line->blocks.addr = addr;
                line->type = type;

                if (type == HookMemory::MEM_READ) {
                    line->was_read = true;
                    stats.nvm_reads++;
                } else {
                    line->was_read = false;
                    stats.nvm_writes++;
                    stats.dead_block_nvm_writes++;
                }

                break;
            }
        }

        // Currently hardcoded to just 2 ways; extend it later if needed;
        if (collisions == no_of_lines) {
            evict(addr, offset, tag, index, type);
        }

        // If there is a read, then put in the last read set
        // if there is a write then check if the read was present in the last read
        // set and if yes then remove it from the last read set
        cout << "INSTR COUNT: " << get_instr_count() << endl; 
        if (type == HookMemory::MEM_READ) {
            cout << "READ: " << hex << addr << dec << endl;
            last_reads[addr] = get_instr_count();
        } else { // if a write
            bool is_in_reads = !!last_reads.count(addr);
            bool is_in_evicted = !!evicted_mem.count(addr);

            cout << "WRTE: " << hex << addr << dec << "(" << is_in_reads << ", " << is_in_evicted << ")" << endl; 

            if (is_in_reads && !is_in_evicted) {
                last_reads.erase(addr);
            }
            else if (!is_in_reads && is_in_evicted) {
                evicted_mem.erase(addr);
            }
            else if (is_in_reads && is_in_evicted) {
                cout << "IN READS (" << last_reads[addr] << ") and EVICTED(" << evicted_mem[addr] << ")" << endl;
                
                if (last_reads[addr] > evicted_mem[addr]) {
                    evicted_mem.erase(addr);
                } else if (evicted_mem[addr] - last_reads[addr] >= DEAD_BLOCK_INSTR_WINDOW) {
                    cout << "ERASING from both" << endl << "----------" << endl;
                    last_reads.erase(addr);
                    evicted_mem.erase(addr);
                    stats.dead_block_nvm_writes--;
                }

            }

        }

        // Return NULL as of now; can be extended to return the data when needed.
        return NULL;
  }

  void evict(uint32_t addr, uint32_t offset, uint32_t tag, uint32_t index, enum HookMemory::memory_type type)
  {
    stats.misses++;

    // Get the least used block
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
        stats.nvm_reads++;
    } else { // if write then evict all writes and increment a checkpoint
        stats.write_evictions++;
        stats.nvm_writes++;
        stats.dead_block_nvm_writes++;

        if (evicted_line->possible_war) {
            stats.checkpoints++;

            // Evict all the writes that are a possible war and reset the bits
            for (CacheSet &s : sets) {
                for (CacheLine &l : s.lines) {
                    if (l.possible_war) {
                        l.possible_war = false;
                        l.was_read = false;
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
    }

    // Add the entry to the evicted memory map here also
    evicted_mem[evicted_line->blocks.addr] = get_instr_count();

    // Replace the values with new data
    evicted_line->blocks.offset_bits = offset;
    evicted_line->blocks.tag_bits = tag;
    evicted_line->blocks.set_bits = index;
    evicted_line->blocks.last_used = CurrentTime_nanoseconds();
    evicted_line->blocks.addr = addr;
    evicted_line->type = type;

    // Possible WAR if there was a read somewhere and currently storing write there
    if (evicted_line->was_read == true && type == HookMemory::MEM_WRITE)
        evicted_line->possible_war = true;
    
    // Again, if the new entered memory is a read, then set the bit
    if (evicted_line->type == HookMemory::MEM_READ)
        evicted_line->was_read = true;
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
};

#endif /* _CACHE_HPP_ */