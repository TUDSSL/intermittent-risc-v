/*
 * CacheMem: Cache implementation for a real-life cache system.
 *
 * Cache obj: Primary cache object.
 * obj.run(): Primary entry point, needs to be called with the address and the type of the memory.
 */

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

// ICEmu includes
#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"
#include "icemu/hooks/HookFunction.h"
#include "icemu/hooks/HookManager.h"
#include "icemu/hooks/RegisterHook.h"
#include "icemu/emu/Architecture.h"

// Local includes
#include "../includes/DetectWAR.h"
#include "../includes/LocalMemory.hpp"
#include "../includes/Stats.hpp"
#include "../includes/CycleCostCalculator.hpp"
#include "../includes/Logger.hpp"
#include "../includes/Utils.hpp"
#include "Riscv32E21Pipeline.hpp"
#include "../includes/MemChecker.hpp"

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
    LocalMemory nvm;
    icemu::Memory *EmuMem;
    CycleCost cost;
    Logger log;
    uint64_t last_checkpoint_cycle;
    MemChecker check_mem;
    bool enable_pw;

  public:
    RiscvE21Pipeline *Pipeline;
  // No need for constructor now
  Cache() = default;

  ~Cache()
  {
    uint32_t non_used_cache_blocks = 0;

    // Evict all the writes that are a possible war and reset the bits
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            if (!l.valid) {
              non_used_cache_blocks++;
              continue;
            }
            
            if (l.dirty) {
              cacheNVMwrite(reconstructAddress(l), l.blocks.data, l.blocks.size, false);
            }
        }
    }

    // This needs to pass
    nvm.compareMemory(); 

    // Do any logging/printing
    stats.printStats();
    log.printEndStats(stats);

    // Perform final checks
    ASSERT(stats.cache.writes == stats.nvm.nvm_writes_without_cache);
    ASSERT(stats.cache.reads + stats.nvm.nvm_reads == stats.nvm.nvm_reads_without_cache);

    cout << "Cache lines not used: " << non_used_cache_blocks << endl;
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
            icemu::Memory &emu_mem, string filename, enum CacheHashMethod hash_method, bool enable_pw)
  {
    // Initialize meta stuffs
    EmuMem = &emu_mem;
    capacity = size;
    no_of_lines = ways;
    policy = p;
    dirty_ratio = 0;
    cout << "Hash method: " << hash_method << endl;
    this->hash_method = hash_method;
    this->enable_pw = enable_pw;

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
        line->read_dominated = false;
        line->write_dominated = false;
        line->possible_war = false;
        line->dirty = false;
      }
    }

    // Initialize the shadow mem
    nvm.initMem(EmuMem);

    // Initialize the logger
    log.init(filename, hash_method);
  }

  address_t reconstructAddress(CacheLine &line)
  {
    address_t tag = line.blocks.tag_bits;
    address_t index = line.blocks.set_bits;
    address_t offset = line.blocks.offset_bits;

    address_t address =   tag << (NUM_BITS(no_of_sets) + NUM_BITS(CACHE_BLOCK_SIZE))
                        | index << (NUM_BITS(CACHE_BLOCK_SIZE))
                        | offset;
    return address;
  }

  uint32_t* run(address_t addr, enum HookMemory::memory_type type, address_t *value, const address_t size)
  {
      // This is the data tha needs to be returned to the CPU from the cache.
      uint64_t data = 0;

      // Only supports 32 bit now, will ASSERT for 64bit
      ASSERT(size == 1 || size == 2 || size == 4);

      // Fetch the tag, set and offset from the mem address
      const address_t offset = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      const address_t index = (addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)))) >> NUM_BITS(CACHE_BLOCK_SIZE);
      const address_t tag = addr >> (address_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));

      ASSERT(index <= no_of_sets);

      // Create checkpoints due to period and dirty ratio
      checkDirtyRatioAndCreateCheckpoint();
      checkCycleCountAndCreateCheckpoint();

      // Dirty ratio should never go negative
      ASSERT(dirty_ratio >= 0);

      // stats for normal nvm
      normalNVMAccess(type, size);

      // Extra checks for the shadow memory
      if (type == HookMemory::MEM_WRITE)
        check_mem.writes.insert(addr);

      // Checks if there are any collisions. If yes, then something has to be
      // evicted before putting in the current mem access.
      uint8_t collisions = 0;

      // Look for any entry in the Cache line marked by "index"
      for (uint32_t i = 0; i < no_of_lines; i++) {
          // Based on the type of hashing, fetch the location of the line
          address_t hashed_index = cacheHash(index, addr, hash_method, i);
          CacheLine &line = sets.at(hashed_index).lines[i];

          // New cache entry for the line.
          if (line.valid == false) {
              stats.incCacheMiss();

              setBit(VALID, line);
              cacheStoreAddress(line, tag, offset, index);
              updateCacheLastUsed(line);
              cacheCreateEntry(line, addr, type, size, *value);

              // In case of read, get the data form the cache
              if (type == HookMemory::MEM_READ)
                data = line.blocks.data;

              break;
          } else {
              // Condition for a cache hit
              if (addr == reconstructAddress(line)) {
                  updateCacheLastUsed(line);
                  stats.incCacheHits();
                  
                  // Perform hit actions - note that these are slightly different
                  // from putting a new element in an empty cache line
                  switch (type) {
                    case HookMemory::MEM_READ:
                      setBit(READ_DOMINATED, line);
                      stats.incCacheReads(size);
                      cost.modifyCost(Pipeline, CACHE_READ, size);
                  
                      // Fetch the data from the cache
                      data = line.blocks.data;
                      break;

                    case HookMemory::MEM_WRITE:
                      setBit(DIRTY, line);
                      setBit(WRITE_DOMINATED, line);
                      setBit(POSSIBLE_WAR, line);

                      // In case of a read, the value from the CPU is stored in
                      // the cache.
                      line.blocks.data = *value;
                      line.blocks.size = size;

                      stats.incCacheWrites(size);
                      cost.modifyCost(Pipeline, CACHE_WRITE, size);
                      break;
                  }

                  break;
              } else {
                collisions++;
              }
          }

      }

      // Oh ho no no can't do this shit, must panic
      ASSERT(collisions <= no_of_lines);

      // Handle the collisions if any
      if (collisions == no_of_lines)
          data = evict(addr, offset, tag, index, value, type, size);

      // If it is a read, then see if our implementation provides same as emulator NVM
      if (type == HookMemory::MEM_READ) {
        address_t valueFromEmuMem;
        valueFromEmuMem = nvm.emulatorRead(addr, size);

        data = 0;
        bool foundData = false;

        for (uint32_t i = 0; i < no_of_lines; i++) {
          // Based on the type of hashing, fetch the location of the line
          address_t hashed_index = cacheHash(index, addr, hash_method, i);
          CacheLine &line = sets.at(hashed_index).lines[i];

          if (line.valid) {
              if (addr == reconstructAddress(line)) {
                data = line.blocks.data;
                valueFromEmuMem = nvm.emulatorRead(addr, line.blocks.size);
                foundData = true;
                break;
              }
          }
        }

        ASSERT(foundData == true);

        if (data != valueFromEmuMem) {
          cout << "---DIFFERS---";
          cout << hex << "From local: " << data << " From emulator: " << valueFromEmuMem << " addr: " << addr << dec << endl;
        }
      }
      return NULL;
  }

  // Handle collisions.
  uint64_t evict(address_t addr, address_t offset, address_t tag, address_t index,
             address_t *value, enum HookMemory::memory_type type, const address_t size)
  {
      // If needs to evict, then the misses needs to be incremented.
      stats.incCacheMiss();

      // Get the line to be evicted using the given replacement policy.
      CacheLine *evicted_line = nullptr;
      address_t hashed_index;

      // Perform the eviction based on the policy.
      switch (policy) {
          case LRU:
            hashed_index = cacheHash(index, 0, SET_ASSOCIATIVE, 0);
            evicted_line = &(*std::min_element(sets.at(hashed_index).lines.begin(), sets.at(hashed_index).lines.end()));
            break;
          case MRU:
            hashed_index = cacheHash(index, 0, SET_ASSOCIATIVE, 0);
            evicted_line = &(*std::max_element(sets.at(hashed_index).lines.begin(), sets.at(hashed_index).lines.end()));
            break;
          case SKEW:
            // Perform the cuckoo hashing
            return cuckooHashing(addr, offset, tag, index, value, type, size);
      }
      
      ASSERT(evicted_line != nullptr);
      ASSERT(evicted_line->valid == true);
    
      // Perform the eviction
      if (evicted_line->dirty) {
          if (evicted_line->possible_war || enable_pw == false)
              createCheckpoint(CHECKPOINT_DUE_TO_WAR);
          else {
              cacheNVMwrite(reconstructAddress(*evicted_line), evicted_line->blocks.data, evicted_line->blocks.size, true);
              clearBit(DIRTY, *evicted_line);
              stats.incCacheDirtyEvictions();
          }
      } else
          stats.incCacheCleanEvictions();

      // If evicted then reset all the flags (except VALID)
      clearBit(READ_DOMINATED, *evicted_line);
      clearBit(WRITE_DOMINATED, *evicted_line);
      clearBit(POSSIBLE_WAR, *evicted_line);
      clearBit(DIRTY, *evicted_line);

      // Now that the eviction has been done, perform the replacement.
      cacheStoreAddress(*evicted_line, tag, offset, index);
      updateCacheLastUsed(*evicted_line);

      // Again the same logic. If the memory access is a READ then set the was_read
      // flag to true.
      cacheCreateEntry(*evicted_line, addr, type, size, *value);

      return evicted_line->blocks.data;
  }

  uint64_t cuckooHashing(address_t addr, address_t offset, address_t tag, address_t index,
                     address_t *value, enum HookMemory::memory_type type, address_t size)
  {
        uint64_t data = 0;

        // Algorithm based on https://ieeexplore-ieee-org.tudelft.idm.oclc.org/stamp/stamp.jsp?tp=&arnumber=9224198 (Section C)
        int line_used = 0;
        address_t cuckoo_hash;
        CacheLine cuckoo_incoming = {}, *cuckoo_cache = nullptr, cuckoo_temp = {};

        // This line will be cuckooed around - this is the data that has to be inserted
        setBit(VALID, cuckoo_incoming);

        // Now that the eviction has been done, perform the replacement.
        cacheStoreAddress(cuckoo_incoming, tag, offset, index);
        updateCacheLastUsed(cuckoo_incoming);
        cacheCreateEntry(cuckoo_incoming, addr, type, size, *value);

        // Data that needs to be retured to CPU
        data = cuckoo_incoming.blocks.data;

        // cout << "Handling addr #" << hex << addr << "with data #" << *value << dec << endl;

        // Until we find a place or till max number of times
        for (int cuckoo_iter = 0; cuckoo_iter < CUCKOO_MAX_ITER; cuckoo_iter++) {
          // Get the line already in the cache based on the address of the incoming line
          cuckoo_hash = cacheHash(0, reconstructAddress(cuckoo_incoming), SKEW_ASSOCIATIVE, line_used);
          cuckoo_cache = &sets.at(cuckoo_hash).lines.at(line_used);
          
          // cout << "iter #" << cuckoo_iter << endl;
          // cout << "cuckoo_cache with index #" << cuckoo_incoming.blocks.set_bits << " line #" << line_used << " hash #" << cuckoo_hash << endl;
          stats.incCuckooIterations();
          cost.modifyCost(Pipeline, CUCKOO_ITER, 0);

          // If the current line is dirty, then we need to move around stuff
          if (cuckoo_cache->valid && cuckoo_cache->dirty) {
            // Each unsuccessful iteration adds 2 cache reads and 2 cache writes = 4 cache accesses
            stats.incCacheCuckoo(cuckoo_cache->blocks.size * 2);
            stats.incCacheCuckoo(cuckoo_incoming.blocks.size * 2);
            // Apart from the access, compute the extra cycle cost as well.
            cost.modifyCost(Pipeline, CACHE_ACCESS, cuckoo_cache->blocks.size * 2);
            cost.modifyCost(Pipeline, CACHE_ACCESS, cuckoo_incoming.blocks.size * 2);

            // Backup the line already in the cache
            copyCacheLines(cuckoo_temp, *cuckoo_cache);
            // store the incoming line in the cache
            copyCacheLines(*cuckoo_cache, cuckoo_incoming);
            // The removed line becomes the incoming element
            copyCacheLines(cuckoo_incoming, cuckoo_temp);
          } else {
            // In case successful Cuckoo, it is 1 cache write
            stats.incCacheCuckoo(cuckoo_incoming.blocks.size);
            cost.modifyCost(Pipeline, CACHE_READ, size);

            // If found a nice place, just put it there
            copyCacheLines(*cuckoo_cache, cuckoo_incoming);
            // eviction is done, return
            return data;
          }

          // Alternate the line use for every iteration
          line_used = (line_used + 1) % 2;
        }

        ASSERT(cuckoo_incoming.valid == true);
        ASSERT(cuckoo_incoming.dirty == true);

        // write the dangling cuckoo back to nvm
        cacheNVMwrite(reconstructAddress(cuckoo_incoming), cuckoo_incoming.blocks.data, cuckoo_incoming.blocks.size, true);
        clearBit(DIRTY, cuckoo_incoming);

        stats.incCheckpoints();
        stats.incCheckpointsDueToWAR(); 
        
        // cout << "Creating checkpoint #" << stats.checkpoint.checkpoints << endl;
        cost.modifyCost(Pipeline, CHECKPOINT, 0);
        for (CacheSet &s : sets) {
            for (CacheLine &l : s.lines) {
                if (l.valid) {
                  ASSERT(l.blocks.size != 0);
                  // This is the line that was inserted in this iteration. This should not be evicted
                  // during the current checkpoint. The checkpoint should be placed before the write
                  // instruction and this write should not be placed.
                  if (addr == reconstructAddress(l))
                    continue;
                  
                  // Note: Now there is a corner case for which the above if check will not work. It is
                  // the corner case where the cuckoo forms a circle (which should not occur IMO). For
                  // this case, the "addr" is a valid eviction and this results in a shadowMem failure.

                  if (l.dirty) {
                      // Perform the actual write to the memory
                      cacheNVMwrite(reconstructAddress(l), l.blocks.data, l.blocks.size, true);
                      stats.incCacheCheckpoint(l.blocks.size);
                  }
                  
                  clearBit(DIRTY, l);
                }
            }
        }
 
        nvm.compareMemory();

        return data;
  }

  void copyCacheLines(struct CacheLine &dst, struct CacheLine &src)
  {
    dst.blocks.data = src.blocks.data;
    dst.blocks.set_bits = src.blocks.set_bits;
    dst.blocks.tag_bits = src.blocks.tag_bits;
    dst.blocks.offset_bits = src.blocks.offset_bits;
    dst.blocks.size = src.blocks.size;
    dst.blocks.last_used = src.blocks.last_used;
    dst.valid = src.valid;
    dst.dirty = src.dirty;
    dst.possible_war = src.possible_war;
    dst.read_dominated = src.read_dominated;
    dst.write_dominated = src.write_dominated;
  }

  // Apply compiler hints
  void applyCompilerHints(uint32_t address)
  {
      // Offset the main memory start
      auto addr = address;

      // Fetch the tag, set and offset from the mem address
      auto index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);

      // Search for the cache line where the hint has to be given. If found
      // then reset the possibleWAR and the was_read flag. This will ensure that
      // eviction of the given memory causes no checkpoint.
      for (CacheSet &s : sets) {
          for (CacheLine &line : s.lines) {
            if (line.valid) {
                if (addr == reconstructAddress(line)) {
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
  // Note that @index is only used for SET ASSOCIATIVE and @addr is only used for SKEW ASSOCIATIVE
  address_t cacheHash(address_t index, address_t addr, enum CacheHashMethod type, uint32_t line_number)
  {
    switch (type) {
      case SET_ASSOCIATIVE:
        return index;
      case SKEW_ASSOCIATIVE:
        // Only support 2-way skew associative as of now
        ASSERT(no_of_lines == 2);
        if (line_number == 0)
          return ((2 * addr) % 103) % no_of_sets;
        else
          return ((9 * addr + 3) % 103) % no_of_sets;
    }

    // No no, you should not come here
    ASSERT(false);
    return 0;
  }

  // Create a checkpoint with proper reason
  void createCheckpoint(enum CheckpointReason reason)
  {
    // Only place where checkpoints are incremented
    stats.incCheckpoints();
    cost.modifyCost(Pipeline, CHECKPOINT, 0);

    // Update the cause to the stats
    stats.updateCheckpointCause(reason);

    // cout << "Creating checkpoint #" << stats.checkpoint.checkpoints;

    // Increment based on reasons
    switch (reason)
    {
      case CHECKPOINT_DUE_TO_WAR:
        stats.incCheckpointsDueToWAR();
        // cout << " due to WAR" << endl;
        break;
      case CHECKPOINT_DUE_TO_DIRTY:
        stats.incCheckpointsDueToDirty();
        // cout << " due to ditry ratio" << endl;
        break;
      case CHECKPOINT_DUE_TO_PERIOD:
        stats.incCheckpointsDueToPeriod();
        // cout << " due to period" << endl;
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
    for (CacheSet &s : sets) {
        for (CacheLine &l : s.lines) {
            if (l.valid) {
              if (l.dirty) {
                  // Perform the actual write to the memory
                  cacheNVMwrite(reconstructAddress(l), l.blocks.data, l.blocks.size, true);
                  stats.incCacheCheckpoint(l.blocks.size);
              }
              
              // Reset all bits
              clearBit(READ_DOMINATED, l);
              clearBit(WRITE_DOMINATED, l);
              clearBit(POSSIBLE_WAR, l);
              clearBit(DIRTY, l);
            }
        }
    }

    // for (auto &w : check_mem.writes)
    //   cout << "Writes not evicted #" << hex << w << dec << endl;

    // assert(check_mem.writes.size() == 0);

    // Sanity check - MUST pass
    nvm.compareMemory(false);
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
        line.valid = true;
        break;
      case READ_DOMINATED:
        ASSERT(line.valid == true);
        if (line.write_dominated == false)
          line.read_dominated = true;
        break;
      case WRITE_DOMINATED:
        ASSERT(line.valid == true);
        if (line.read_dominated == false)
          line.write_dominated = true;
        break;
      case POSSIBLE_WAR:
        ASSERT(line.valid == true);
        ASSERT(line.dirty == true);
        if (line.read_dominated == true)
          line.possible_war = true;
        break;
      case DIRTY:
        ASSERT(line.valid == true);
        // Check if not already set, set the bit and update the dirty ratio
        if (line.dirty == false) {
          line.dirty = true;
          dirty_ratio = (dirty_ratio * (capacity / CACHE_BLOCK_SIZE) + 1) / (capacity / CACHE_BLOCK_SIZE);
          ASSERT(dirty_ratio <= 1.0);
        }
        // cout << "Setting dirty bit: " << line.blocks.set_bits << " to " << dirty_ratio << endl;
        break;
    }
  }

  // Clear the "bit" from "line"
  void clearBit(enum CacheBits bit, CacheLine &line)
  {
    switch (bit) {
      case VALID:
        // Should never clear a valid bit twice
        ASSERT(line.valid == true);
        line.valid = false;
        break;
      case READ_DOMINATED:
        line.read_dominated = false;
        break;
      case WRITE_DOMINATED:
        line.write_dominated = false;
        break;
      case POSSIBLE_WAR:
        line.possible_war = false;
        break;
      case DIRTY:
        // Check if not already cleared, clear the bit and update the dirty ratio
        if (line.dirty == true) {
          line.dirty = false;
          dirty_ratio = (dirty_ratio * (capacity / CACHE_BLOCK_SIZE) - 1) / (capacity / CACHE_BLOCK_SIZE);
          ASSERT(dirty_ratio >= 0.0);
        }
        // cout << "Clearing dirty bit: " << line.blocks.set_bits << " to " << dirty_ratio << endl;
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
    uint64_t diff = stats.getCurrentCycle() - stats.getLastCheckpointCycle();
   
    if (diff > stats.checkpoint.cycles_between_checkpoints)
      stats.checkpoint.cycles_between_checkpoints = diff;

    // A value of 0 disables the periodic checkpoint
    if (CYCLE_COUNT_CHECKPOINT_THRESHOLD == 0)
      return false;

    // If the difference between the current cycle and the last checkpoint is
    // higher that the threshold, then create a checkpoint.
    if (diff > CYCLE_COUNT_CHECKPOINT_THRESHOLD) {
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
    
    check_mem.writes.erase(address);
    nvm.localWrite(address, value, size);
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
    }
  }

  // Fill a cache entry either for the first time or after an eviction (both of these actions
  // perform the same set of steps.)
  void cacheCreateEntry(CacheLine &line, address_t addr, enum HookMemory::memory_type type, const address_t size, address_t value)
  {
    switch (type) {
      case HookMemory::MEM_READ:
        setBit(READ_DOMINATED, line);
      
        // In case of a read, read the value from the local NVM copy
        line.blocks.data = nvm.localRead(addr, size);
        line.blocks.size = size;
        stats.incNVMReads(size);
        cost.modifyCost(Pipeline, NVM_READ, size);
        break;

      case HookMemory::MEM_WRITE:
        setBit(DIRTY, line);
        setBit(WRITE_DOMINATED, line);

        // In case of a write, copy the value from the CPU to the data
        line.blocks.data = value;
        line.blocks.size = size;
        
        stats.incCacheWrites(size);
        cost.modifyCost(Pipeline, CACHE_WRITE, size);
        ASSERT(line.possible_war == false);
        ASSERT(line.read_dominated == false);
        break;
    }
  }

};

#endif /* _CACHE_MEM_HPP_ */