/*
 * CacheMem: Cache implementation for a real-life cache system.
 *
 * Cache obj: Primary cache object.
 * obj.run(): Primary entry point, needs to be called with the address and the type of the memory.
 */

#ifndef _CACHE_PROWL_HPP_
#define _CACHE_PROWL_HPP_

#include <map>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>

// ICEmu includes
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
#include "../includes/Logger.hpp"
#include "../includes/Utils.hpp"
#include "Riscv32E21Pipeline.hpp"

using namespace std;
using namespace icemu;

class Cache {
  private:    
    // Cache metadata
    enum replacement_policy policy;
    enum CacheHashMethod hash_method;
    uint32_t capacity;  // in bytes
    uint32_t no_of_sets;
    uint32_t no_of_lines;
    float dirty_ratio;

    // Cache sets
    std::vector<CacheSet> sets;

    // Helper stuffs
    Stats stats;
    ShadowMemory nvm;
    icemu::Memory *mem;
    CycleCost cost;
    Logger log;
    uint64_t last_checkpoint_cycle;

  public:
    RiscvE21Pipeline *Pipeline;
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

    // This needs to pass
    nvm.compareMemory(); 

    // Do any logging/printing
    stats.printStats();
    log.printEndStats(stats);

    // Perform final checks
    assert(stats.cache.writes == stats.nvm.nvm_writes_without_cache);
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
  void init(uint32_t size, uint32_t ways, enum replacement_policy p,
            icemu::Memory &emu_mem, string filename, enum CacheHashMethod hash_method)
  {
    // Initialize meta stuffs
    mem = &emu_mem;
    capacity = size;
    no_of_lines = ways;
    policy = p;
    dirty_ratio = 0;
    cout << "Hash method: " << hash_method << endl;
    this->hash_method = hash_method;

    if (hash_method == SKEW_ASSOCIATIVE)
      policy = SKEW;

    // Number of cache sets = capacity / (ways * block size)
    no_of_sets = capacity / (no_of_lines * NO_OF_CACHE_BLOCKS);

    // Initialize the cache
    sets.resize(no_of_sets);
    cout << "CAPACITY: " << capacity << endl;
    cout << "SETS: " << no_of_sets << endl;
    cout << "Lines: " << no_of_lines << endl;

    for (uint32_t i = 0; i < no_of_sets; i++) {
      sets[i].lines.resize(no_of_lines);
      
      for (uint32_t j = 0; j < no_of_lines; j++) {
        auto line = &sets[i].lines[j];
        memset(&line->blocks, 0, sizeof(struct CacheBlock));
        line->valid = false;
        line->dirty = false;
        // line->read_dominated = false;
        // line->write_dominated = false;
        // line->possible_war = false;
      }
    }

    // Initialize the shadow mem
    nvm.initMem(mem);

    // Initialize the logger
    log.init(filename);
  }

  uint32_t* run(address_t addr, enum HookMemory::memory_type type, address_t *value, address_t size)
  {
      // Only supports 32 bit now, will assert for 64bit
      assert(size == 1 || size == 2 || size == 4);

      // Fetch the tag, set and offset from the mem address
      address_t offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      address_t index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);
      address_t tag = addr >> (address_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));

      assert(index <= no_of_sets);
      // cout << "Address: " << addr << "\tTag: " << tag << "\tIndex: " << index << "\tOffset: " << offset << endl;
      // cout << "Memory type: " << type << "\t size: " << size << hex << "\t address: " << address << dec << "\tData: " << *value;

      // cout << "Dirty ratio: " << dirty_ratio << endl;

    //   checkDirtyRatioAndCreateCheckpoint();
    //   checkCycleCountAndCreateCheckpoint();

      // Dirty ratio should never go negative
      assert(dirty_ratio >= 0);

      // stats for normal nvm
      normalNVMAccess(type, size);

      // Checks if there are any collisions. If yes, then something has to be
      // evicted before putting in the current mem access.
      uint8_t collisions = 0;

      // Look for any entry in the Cache line marked by "index"
      for (int i = 0; i < no_of_lines; i++) {
          address_t hashed_index = cacheHash(index, hash_method, i);
          CacheLine &line = sets.at(hashed_index).lines[i];

          if (line.valid) {
              if (line.blocks.tag_bits == tag && line.blocks.offset_bits == offset) {
                  line.blocks.last_used = CurrentTime_nanoseconds();
                  stats.incCacheHits();
                  cout << "HIT in set: " << hashed_index << " line: " << i << endl;
                  switch (type) {
                    case HookMemory::MEM_READ:
                      stats.incCacheReads(size);
                      cost.modifyCost(Pipeline, CACHE_READ, size);
                    //   setBit(READ_DOMINATED, line);
                      break;
 
                    case HookMemory::MEM_WRITE:
                      setBit(DIRTY, line);
                    //   setBit(WRITE_DOMINATED, line);
                    //   setBit(POSSIBLE_WAR, line);
                      stats.incCacheWrites(size);
                      cost.modifyCost(Pipeline, CACHE_WRITE, size);
                      line.blocks.data = *value;
                      line.blocks.size = size;
                      break;
                  }

                  break;
              } else
                collisions++;

          } else {
              stats.incCacheMiss();
              cout << "INIT in set: " << hashed_index << " line: " << i << endl;
              // Store the values. Only place where valid is set true
              setBit(VALID, line);
              line.blocks.offset_bits = offset;
              line.blocks.set_bits = index;
              line.blocks.tag_bits = tag;
              line.blocks.last_used = CurrentTime_nanoseconds();
              line.blocks.addr = addr;

              // Store the data and the size
              line.blocks.data = *value;
              line.blocks.size = size;

              cacheResetEntry(line, type, size);

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
    stats.incCacheMiss();

    // Get the line to be evicted using the given replacement policy.
    CacheLine *evicted_line = nullptr;
    cout << "MISS in set: " << cacheHash(index, SET_ASSOCIATIVE, 0) << endl;
    switch (policy) {
      case LRU:
        evicted_line = &(*std::min_element(sets.at(cacheHash(index, SET_ASSOCIATIVE, 0)).lines.begin(), sets.at(cacheHash(index, SET_ASSOCIATIVE, 0)).lines.end()));
        break;
      case MRU:
        evicted_line = &(*std::max_element(sets.at(cacheHash(index, SET_ASSOCIATIVE, 0)).lines.begin(), sets.at(cacheHash(index, SET_ASSOCIATIVE, 0)).lines.end()));
        break;
      case SKEW:
        if (sets.at(cacheHash(index, SKEW_ASSOCIATIVE, 0)).lines.at(0).blocks.last_used < sets.at(cacheHash(index, SKEW_ASSOCIATIVE, 1)).lines.at(1).blocks.last_used)
          evicted_line = &sets.at(cacheHash(index, SKEW_ASSOCIATIVE, 0)).lines.at(0);
        else
          evicted_line = &sets.at(cacheHash(index, SKEW_ASSOCIATIVE, 1)).lines.at(1);
    }

    assert(evicted_line != nullptr);

    // Perform the eviction - NO POSSIBLE WAR CHECK, just evict and create checkpoint
    if (evicted_line->dirty)
        createCheckpoint(CHECKPOINT_DUE_TO_WAR);
    else
      stats.incCacheCleanEvictions();

    // Now that the eviction has been done, perform the replacement.
    evicted_line->blocks.offset_bits = offset;
    evicted_line->blocks.tag_bits = tag;
    evicted_line->blocks.set_bits = index;
    evicted_line->blocks.last_used = CurrentTime_nanoseconds();
    evicted_line->blocks.addr = addr;

    // If evicted then reset all the flags (except VALID)
    // clearBit(READ_DOMINATED, *evicted_line);
    // clearBit(WRITE_DOMINATED, *evicted_line);
    // clearBit(POSSIBLE_WAR, *evicted_line);
    clearBit(DIRTY, *evicted_line);

    // Replace the data in the memory
    evicted_line->blocks.data = *value;
    evicted_line->blocks.size = size;

    // Again the same logic. If the memory access is a READ then set the was_read
    // flag to true.
    cacheResetEntry(*evicted_line, type, size);
  }

  // Apply compiler hints
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
                  clearBit(READ_DOMINATED, line);
                  clearBit(WRITE_DOMINATED, line);
                  clearBit(POSSIBLE_WAR, line);
                  clearBit(DIRTY, line);
                }
            }
          }
      }

      // Do we need this?
      stats.incHintsGiven();
      cost.modifyCost(Pipeline, HINTS, 0);
  }

  // Perform the hashing which fetches the set from the cache.
  address_t cacheHash(address_t index, enum CacheHashMethod type, uint32_t line_number)
  {
    return 15;
    address_t idx_to_return, idx;
    switch (type) {
      case SET_ASSOCIATIVE:
        idx_to_return = index;
        break;
      case SKEW_ASSOCIATIVE:
        // Only support 2-way skew associative as of now
        assert(no_of_lines == 2);
        if (line_number == 0)
          idx_to_return = ((2 * index) % 71) % no_of_sets;
        else if (line_number == 1)
          idx_to_return = ((9 * index + 3) % 71) % no_of_sets;
        break;
    }

    // cout << "INDEX: " << idx_to_return << endl;

    // No no, you should not come here
    assert(idx_to_return <= no_of_sets);
    return idx_to_return;
  }

  // Create a checkpoint with proper reason
  void createCheckpoint(enum CheckpointReason reason)
  {
    // Only place where checkpoints are incremented
    stats.incCheckpoints();
    cost.modifyCost(Pipeline, CHECKPOINT, 0);

    // Update the cause to the stats
    stats.updateCheckpointCause(reason);

    cout << "Creating checkpoint #" << stats.checkpoint.checkpoints;

    // Increment based on reasons
    switch (reason)
    {
      case CHECKPOINT_DUE_TO_WAR:
        stats.incCheckpointsDueToWAR();
        cout << " due to WAR" << endl;
        break;
      case CHECKPOINT_DUE_TO_DIRTY:
        stats.incCheckpointsDueToDirty();
        cout << " due to ditry ratio" << endl;
        break;
      case CHECKPOINT_DUE_TO_PERIOD:
        stats.incCheckpointsDueToPeriod();
        cout << " due to period" << endl;
        break;
    }

    // Perform the actual evictions due to checkpoints
    checkpointEviction();

    // Print the log line
    log.printCheckpointStats(stats);

    // And then update when this checkpoint was created
    stats.updateLastCheckpointCycle(stats.getCurrentCycle());
  }

  // Function to be called on eviction of the cache on checkpoint
  void checkpointEviction()
  {
    // Evict all the writes that are a possible war and reset the bits
    // for (CacheSet &s : sets) {
        for (CacheLine &l : sets[15].lines) {
            if (l.dirty) {
                // Perform the actual write to the memory
                cacheNVMwrite(l.blocks.addr, l.blocks.data, l.blocks.size, true);
            }
            
            // Reset all bits
            // clearBit(READ_DOMINATED, l);
            // clearBit(WRITE_DOMINATED, l);
            // clearBit(POSSIBLE_WAR, l);
            clearBit(DIRTY, l);
        }
    // }

    // Sanity check - MUST pass
    nvm.compareMemory();
  }

  // Update the cycle count being stored in stats - callback function
  // to be used in the instruction hook
  void updateCycleCount(uint64_t cycle_count)
  {
    stats.updateCurrentCycle(cycle_count);
  }

  // Set the specified bit with associated conditions and actions
  void setBit(enum CacheBits bit, CacheLine &line)
  {
    switch (bit) {
      case VALID:
        // Should never set a valid bit twice
        assert(line.valid == false);
        line.valid = true;
        break;
      case READ_DOMINATED:
        assert(false);
        if (line.write_dominated == false)
          line.read_dominated = true;
        break;
      case WRITE_DOMINATED:
        assert(false);
        if (line.read_dominated == false)
          line.write_dominated = true;
        break;
      case POSSIBLE_WAR:
        assert(false);
        if (line.read_dominated == true)
          line.possible_war = true;
        break;
      case DIRTY:
        // Check if not already set, set the bit and update the dirty ratio
        if (line.dirty == false) {
          line.dirty = true;
          dirty_ratio = (dirty_ratio * (capacity / CACHE_BLOCK_SIZE) + 1) / (capacity / CACHE_BLOCK_SIZE);
        }
        break;
    }
  }

  // Clear the "bit" from "line"
  void clearBit(enum CacheBits bit, CacheLine &line)
  {
    switch (bit) {
      case VALID:
        // Should never clear a valid bit twice
        assert(line.valid == true);
        line.valid = false;
        break;
      case READ_DOMINATED:
        assert(false);
        line.read_dominated = false;
        break;
      case WRITE_DOMINATED:
        assert(false);
        line.write_dominated = false;
        break;
      case POSSIBLE_WAR:
        assert(false);
        line.possible_war = false;
        break;
      case DIRTY:
        // Check if not already cleared, clear the bit and update the dirty ratio
        if (line.dirty == true) {
          line.dirty = false;
          dirty_ratio = (dirty_ratio * (capacity / CACHE_BLOCK_SIZE) - 1) / (capacity / CACHE_BLOCK_SIZE);
        }
        break;
    }
  }

  // Check and create checkpoints if dirty bit is present
  bool checkDirtyRatioAndCreateCheckpoint()
  {
    stats.updateDirtyRatio(dirty_ratio);

    // Create checkpoint on exceeding the threshold
    if (dirty_ratio > DIRTY_RATIO_THRESHOLD) {
      createCheckpoint(CHECKPOINT_DUE_TO_DIRTY);
      return true;
    }

    return false;
  }

  // Check and create checkpoints in a period.
  bool checkCycleCountAndCreateCheckpoint()
  {
    if (CYCLE_COUNT_CHECKPOINT_THRESHOLD == 0)
      return false;

    if (stats.getCurrentCycle() - stats.getLastCheckpointCycle() > CYCLE_COUNT_CHECKPOINT_THRESHOLD) {
      createCheckpoint(CHECKPOINT_DUE_TO_PERIOD);
      return true;
    }

    return false;
      
  }

  // Write back the cache content to the shadow memory with option to increase the NVM reads/writes
  void cacheNVMwrite(address_t address, address_t value, address_t size, bool doesCountForStats)
  {
    if (doesCountForStats) {
      stats.incNVMWrites(size);
      cost.modifyCost(Pipeline, NVM_WRITE, size);
    }
    
    nvm.shadowWrite(address, value, size);
  }

  // Increments stats as if there was no cache
  void normalNVMAccess(enum HookMemory::memory_type type, address_t size)
  {
    switch(type) {
      case HookMemory::MEM_READ:
        stats.incNonCacheNVMReads(size);
        break;
      case HookMemory::MEM_WRITE:
        stats.incNonCacheNVMWrites(size);
        break;
        break;
    }
  }

  // Fill a cache entry either for the first time or after an eviction (both of these actions
  // perform the same set of steps.)
  void cacheResetEntry(CacheLine &line, enum HookMemory::memory_type type, address_t size)
  {
    switch (type) {
      case HookMemory::MEM_READ:
        // setBit(READ_DOMINATED, line);
        stats.incNVMReads(size);
        cost.modifyCost(Pipeline, NVM_READ, size);
        break;

      case HookMemory::MEM_WRITE:
        setBit(DIRTY, line);
        // setBit(WRITE_DOMINATED, line);
        stats.incCacheWrites(size);
        cost.modifyCost(Pipeline, CACHE_WRITE, size);
        // assert(line.possible_war == false);
        // assert(line.read_dominated == false);
        break;
    }
  }

};

#endif /* _CACHE_MEM_HPP_ */