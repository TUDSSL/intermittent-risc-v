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
    CacheReq req;

    // Cache sets
    std::vector<CacheSet> sets;

    // Helper stuffs
    Stats stats;
    LocalMemory nvm;
    icemu::Memory *EmuMem;
    CycleCost cost;
    Logger log;
    uint64_t last_checkpoint_cycle;
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
    nvm.compareMemory(true && hash_method == 0); 

    // Do any logging/printing
    stats.printStats();
    log.printEndStats(stats);

    // Perform final checks
    ASSERT(stats.cache.writes == stats.nvm.nvm_writes_without_cache);
    ASSERT(stats.cache.reads + stats.nvm.nvm_reads == stats.nvm.nvm_reads_without_cache);

    p_debug << "Cache lines not used: " << non_used_cache_blocks << endl;
  }

  /**
   * @brief Initialize the cache with required values
   *
   * @param size The size of the cache in bytes
   * @param ways The number of cache ways
   * @param p The replacement policy for cache @see enum replacement_policy
   * @param emu_mem Pointer to the parent emulator memory
   * @param filename Filename of the log file
   * @param hash_method The hashing method to use @see enum CacheHashMethod
   * @param enable_pw Enable/disable naive- version of NACHO
   */
  void init(uint32_t size, uint32_t ways, enum replacement_policy p,
            icemu::Memory &emu_mem, string filename, enum CacheHashMethod hash_method, bool enable_pw)
  {
    // Initialize meta stuffs
    EmuMem = &emu_mem;
    capacity = size;
    no_of_lines = ways;
    policy = p;
    dirty_ratio = 0;
    p_debug << "Hash method: " << hash_method << endl;
    this->hash_method = hash_method;
    this->enable_pw = enable_pw;

    if (hash_method == SKEW_ASSOCIATIVE)
      policy = SKEW;

    // Number of cache sets = capacity / (ways * block size)
    no_of_sets = capacity / (no_of_lines * NO_OF_CACHE_BLOCKS);

    // Initialize the cache
    sets.resize(no_of_sets);
    p_debug << "CAPACITY: " << capacity << endl;
    p_debug << "SETS: " << no_of_sets << endl;
    p_debug << "Lines: " << no_of_lines << endl;

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
    log.init(filename);
  }

  /**
   * @brief Primary function that runs when a cache request is made
   * by the CPU. The rough flow of the function is as such
   *      - Parse the cache req
   *      - Check if present in cache
   *            - Create new entry if not present
   *            - If present then if
   *                  - Cache hit, return data
   *                  - Cache miss, collision
   *      - If any collision, evict
   *      - Shadow Memory check
   *
   * @param address The memory address of the access
   * @param mem_type Memory access type - read or write
   * @param value_req Data associated with the memory access
   * @param size_req Size of the data associated with the access
   * @return Data if required
   */
  uint32_t* run(address_t address, enum HookMemory::memory_type mem_type, address_t *value_req, const address_t size_req)
  {
      // Only supports 32 bit now, will ASSERT for 64bit
      ASSERT(size_req == 1 || size_req == 2 || size_req == 4);

      // Populate the CacheReq with the data from the CPU.
      memset(&req, 0, sizeof(struct CacheReq));
      req.addr = address;
      req.size = size_req;
      req.value = *value_req;
      req.type = mem_type;
      req.mem_id.tag = req.addr >> (address_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));
      req.mem_id.offset = req.addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
      req.mem_id.index = (req.addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets))
                                   & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE))))
                                   >> NUM_BITS(CACHE_BLOCK_SIZE);

      p_debug << "REQ: " << hex << req.addr << " VALUE: " << req.value << dec << " OFFSET: " << req.mem_id.offset << " SIZE: " << req.size << endl;

      // Create checkpoints due to period and dirty ratio
      checkDirtyRatioAndCreateCheckpoint();
      checkCycleCountAndCreateCheckpoint();

      // Dirty ratio should never go negative
      ASSERT(dirty_ratio >= 0);
      ASSERT(req.mem_id.index <= no_of_sets);

      // stats for normal nvm
      normalNVMAccess(req.type, req.size);

      // Checks if there are any collisions. If yes, then something has to be
      // evicted before putting in the current mem access.
      uint8_t collisions = 0;

      // Look for any entry in the Cache line marked by "index"
      for (uint32_t i = 0; i < no_of_lines; i++) {
          // Based on the type of hashing, fetch the location of the line
          address_t hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, hash_method, i);
          CacheLine &line = sets.at(hashed_index).lines[i];

          p_debug << "Checking IDX: " << hex << hashed_index << dec << " WAY: " << i << endl;

          // New cache entry for the line.
          if (line.valid == false) {
              handleCacheMiss(line);
              p_debug << "Cache create, stored at IDX: " << hex << hashed_index << dec << " WAY: " << i << endl;
              break;
          }
          // Data is already present in the cache so there might
          // be either hit or collision.
          else {
              if (reconstructAddress(line) == reconstructAddress(req.mem_id.tag, req.mem_id.index)) {
                handleCacheHit(line);
                p_debug << "Cache hit, stored at IDX: " << hex << hashed_index << dec << " WAY: " << i << endl;
                break;
              }
              else {
                p_debug << "Cache miss for Addr: " << hex << req.addr << dec << endl;
                collisions++;
              }
          }

      }

      // Oh ho no no can't do this shit, must panic
      ASSERT(collisions <= no_of_lines);

      // Handle the collisions if any
      if (collisions == no_of_lines) {
          p_debug << "Cache eviction" << endl;
          evict();
      }

      // If it is a read, then see if our implementation provides same as emulator NVM
      performShadowCheck();
      return nullptr;
  }

  /**
   * @brief Reconstruct and return the address from a cache line.
   * The address will be the address corresponding to the
   * main memory.
   *
   * @param line Cache line containing the data
   * @return reconstructed address
   */
  address_t reconstructAddress(CacheLine &line)
  {
    address_t tag = line.blocks.bits.tag;
    address_t index = line.blocks.bits.index;

    address_t address =   tag << (NUM_BITS(no_of_sets) + NUM_BITS(CACHE_BLOCK_SIZE)) |
                          index << (NUM_BITS(CACHE_BLOCK_SIZE));
    return address;
  }

  /**
   * @brief Reconstruct and return the address from a cache line.
   * The address will be the address corresponding to the
   * main memory.
   *
   * @param line Cache line containing the data
   * @return reconstructed address
   */
  address_t reconstructAddress(address_t tag, address_t index)
  {
    address_t address =   tag << (NUM_BITS(no_of_sets) + NUM_BITS(CACHE_BLOCK_SIZE)) |
                          index << (NUM_BITS(CACHE_BLOCK_SIZE));
    return address;
  }

  /**
   * @brief Handle cache misses. Create the cache entry and set
   * all the metadata.
   *
   * @param line Cache line containing the data
   */
  void handleCacheMiss(CacheLine &line)
  {
      stats.incCacheMiss();

      setBit(VALID, line);
      cacheStoreAddress(line, req);
      updateCacheLastUsed(line);
      cacheCreateEntry(line, req);
  }

  /**
   * @brief Handle cache hits. Modify the cache entry and set
   * all the metadata.
   *
   * @param line Cache line containing the data
   */
  void handleCacheHit(CacheLine &line)
  {
    updateCacheLastUsed(line);
    stats.incCacheHits();
    
    // Perform hit actions - note that these are slightly different
    // from putting a new element in an empty cache line
    switch (req.type) {
      // For a read hit
      case HookMemory::MEM_READ:
        setBit(READ_DOMINATED, line);
        stats.incCacheReads(req.size);
        cost.modifyCost(Pipeline, CACHE_READ, req.size);
        p_debug << "Cache read req, read DATA: " << line.blocks.data << endl;
        break;
      
      // For a write hit
      case HookMemory::MEM_WRITE:
        setBit(DIRTY, line);
        setBit(WRITE_DOMINATED, line);
        setBit(POSSIBLE_WAR, line);

        writeToCache(line);

        p_debug << "Cache write req, written DATA: " << hex << line.blocks.data << dec << endl;
        p_debug << "Data at NVM: " << hex << nvm.localRead(reconstructAddress(line), 4) << dec << endl;
        p_debug << "Data at EMULATOR: " << hex << nvm.emulatorRead(reconstructAddress(line), 4) << dec << endl;

        stats.incCacheWrites(req.size);
        cost.modifyCost(Pipeline, CACHE_WRITE, req.size);
        break;
    }
  }

    /**
   * @brief Fill a cache entry either for the first time
   * or after an eviction (both of these actions perform
   * the same set of steps.)
   * 
   * @param line The cache line to create new block in
   * @param req The memory request from CPU
   */
  void cacheCreateEntry(CacheLine &line, const CacheReq req)
  {
    switch (req.type) {
      case HookMemory::MEM_READ:
        setBit(READ_DOMINATED, line);
      
        // In case of a read, read the value from the local NVM copy
        line.blocks.data = nvm.localRead(reconstructAddress(line), 4);
        line.blocks.size = 4;//req.size;
        stats.incNVMReads(req.size);
        cost.modifyCost(Pipeline, NVM_READ, req.size);
        p_debug << "Cache read req, read DATA: " << line.blocks.data << endl;
        break;

      case HookMemory::MEM_WRITE:
        setBit(DIRTY, line);
        setBit(WRITE_DOMINATED, line);

        // In case of a write, copy the value from the CPU to the data
        writeToCache(line);

        p_debug << "Cache write req, written DATA: " << hex << line.blocks.data << dec << endl;
        p_debug << "Data at NVM: " << hex << nvm.localRead(reconstructAddress(line), 4) << dec << endl;
        p_debug << "Data at EMULATOR: " << hex << nvm.emulatorRead(reconstructAddress(line), 4) << dec << endl;

        stats.incCacheWrites(req.size);
        cost.modifyCost(Pipeline, CACHE_WRITE, req.size);
        ASSERT(line.possible_war == false);
        ASSERT(line.read_dominated == false);
        break;
    }
  }

  /**
   * @brief Write the value received from the CPU to the cache.
   * This has to be done taking into account the offset and the
   * size of the data received.
   */
  void writeToCache(CacheLine &line)
  {
      // The destination is the data in the cache
      uint8_t *d = (uint8_t *) &line.blocks.data;

      // The source is the data from the request
      uint8_t *s = (uint8_t *) &req.value;

      // Copy the data from the request to the cache
      for (size_t i = 0; i < req.size; i++) {
        d[i + req.mem_id.offset] = s[i];
      }

      // The size of the value stored in data is always 4 bytes
      line.blocks.size = CACHE_BLOCK_SIZE;
  }

  /**
   * @brief Write the value received from the CPU to the cache.
   * This has to be done taking into account the offset and the
   * size of the data received.
   */
  uint32_t readFromCache(uint64_t dataToRead, address_t offset, address_t size)
  {
      uint32_t data = 0;
      uint32_t *_data_half = ((uint32_t *) &dataToRead);
      uint8_t *d = (uint8_t *) &_data_half[0];

      // Go to the offset in the cache block
      d += offset;

      // Copy the new data into respective bits
      memcpy(&data, d, size);

      return data;
  }

  /**
   * @brief Performs eviction of a cache block depending on the cache
   * replacement policy set. On eviction, it handles and places the
   * requested data into the cache as well. For PROWL, it handles
   * the cuckoo hashing part separately.
   *
   */
  uint64_t evict()
  {
      // If needs to evict, then the misses needs to be incremented.
      stats.incCacheMiss();

      // Get the line to be evicted using the given replacement policy.
      CacheLine *evicted_line = nullptr;
      address_t hashed_index;

      // Perform the eviction based on the policy.
      switch (policy) {
          case LRU:
            hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, SET_ASSOCIATIVE, 0);
            evicted_line = &(*std::min_element(sets.at(hashed_index).lines.begin(), sets.at(hashed_index).lines.end()));
            break;
          case MRU:
            hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, SET_ASSOCIATIVE, 0);
            evicted_line = &(*std::max_element(sets.at(hashed_index).lines.begin(), sets.at(hashed_index).lines.end()));
            break;
          case SKEW:
            // Perform the cuckoo hashing
            return cuckooHashing();
      }
      
      ASSERT(evicted_line != nullptr);
      ASSERT(evicted_line->valid == true);
    
      // Perform the eviction
      if (evicted_line->dirty) {
          if (evicted_line->possible_war || enable_pw == false)
              createCheckpoint(CHECKPOINT_DUE_TO_WAR);
          else { // This is the case where there is no need for checkpoint but still needs to be evicted
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
      evicted_line->blocks.data = nvm.localRead(reconstructAddress(req.mem_id.tag, req.mem_id.index), 4);
      cacheStoreAddress(*evicted_line, req);
      updateCacheLastUsed(*evicted_line);
      cacheCreateEntry(*evicted_line, req);

      return evicted_line->blocks.data;
  }

  /**
   * @brief Perform cuckoo hashing. This replaces and evicts
   * as per the hash functions defined.
   * 
   * @see [Section C](https://ieeexplore-ieee-org.tudelft.idm.oclc.org/stamp/stamp.jsp?tp=&arnumber=9224198)
   * 
   * @return uint64_t data that is put in the cache
   */
  uint64_t cuckooHashing()
  {
        uint64_t data = 0;
        int line_used = 0;
        address_t cuckoo_hash = 0;
        CacheLine cuckoo_incoming = {}, *cuckoo_cache = nullptr, cuckoo_temp = {};

        // I don't trust anything anymore, set all to zero pls
        memset(&cuckoo_incoming, 0, sizeof(struct CacheLine));
        memset(&cuckoo_temp, 0, sizeof(struct CacheLine));

        // This line will be cuckooed around - this is the data that has to be inserted
        setBit(VALID, cuckoo_incoming);

        cuckoo_incoming.blocks.data = nvm.localRead(reconstructAddress(req.mem_id.tag, req.mem_id.index), 4);
        cacheStoreAddress(cuckoo_incoming, req);
        updateCacheLastUsed(cuckoo_incoming);
        cacheCreateEntry(cuckoo_incoming, req);

        // Data that needs to be retured to CPU
        data = cuckoo_incoming.blocks.data;

        // Until we find a place or till max number of times
        for (int cuckoo_iter = 0; cuckoo_iter < CUCKOO_MAX_ITER; cuckoo_iter++) {
          // Get the line already in the cache based on the address of the incoming line
          cuckoo_hash = cacheHash(cuckoo_incoming.blocks.bits.tag, cuckoo_incoming.blocks.bits.index, SKEW_ASSOCIATIVE, line_used);
          cuckoo_cache = &sets.at(cuckoo_hash).lines.at(line_used);
          
          stats.incCuckooIterations();
          cost.modifyCost(Pipeline, CUCKOO_ITER, 0);

          p_debug << "Cuckoo iter : " << cuckoo_iter << hex 
                << " Addr: " << reconstructAddress(*cuckoo_cache) 
                << " Hash: " << cuckoo_hash 
                << dec << " Line: " << line_used << endl;

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
            cost.modifyCost(Pipeline, CACHE_READ, cuckoo_incoming.blocks.size);

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
        clearBit(VALID, cuckoo_incoming);

        stats.incCheckpoints();
        stats.incCheckpointsDueToWAR(); 
        
        cost.modifyCost(Pipeline, CHECKPOINT, 0);
        for (CacheSet &s : sets) {
            for (CacheLine &l : s.lines) {
                if (l.valid) {
                  /** 
                   * @warning This is the line that was inserted in this iteration. This should not be evicted
                   * during the current checkpoint. The checkpoint should be placed before the write
                   * instruction and this write should not be placed.
                   */
                  if (reconstructAddress(l) == reconstructAddress(req.mem_id.tag, req.mem_id.index))
                    continue;
                  
                  /**
                   * @note Now there is a corner case for which the above if check will not work. It is
                   * the corner case where the cuckoo forms a circle (which should not occur IMO). For
                   * this case, the "addr" is a valid eviction and this results in a shadowMem failure.
                   */
                  if (l.dirty) {
                      // Perform the actual write to the memory
                      cacheNVMwrite(reconstructAddress(l), l.blocks.data, l.blocks.size, true);
                      stats.incCacheCheckpoint(l.blocks.size);
                  }
                  
                  clearBit(DIRTY, l);
                }
            }
        }
 
        nvm.compareMemory(false);

        return data;
  }

  /**
   * @brief Perform sanity check based on shadow memory testing.
   * A read from the CPU must fetch the same data from the cache
   * as that from the emulator memory. The emulator memory acts as
   * the shadow memory in this case.
   * 
   * @see LocalMemory.hpp
   */
  void performShadowCheck()
  {
    if (req.type == HookMemory::MEM_READ) {
        address_t valueFromEmuMem;
        bool foundData = false;
        uint64_t data = 0;

        valueFromEmuMem = readFromCache(nvm.emulatorRead(
              reconstructAddress(req.mem_id.tag, req.mem_id.index), 4), req.mem_id.offset, req.size);

        /**
         * @note Find the data associated with the current request. At this point of time
         * the cache must have performed all eviction and replacement which means that
         * the data has to be there in the cache.
         */
        for (uint32_t i = 0; i < no_of_lines; i++) {
          address_t hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, hash_method, i);
          CacheLine &line = sets.at(hashed_index).lines[i];

          if (line.valid) {
              if (reconstructAddress(line) == reconstructAddress(req.mem_id.tag, req.mem_id.index)) {
                // This is the data to be compared
                data = readFromCache(line.blocks.data, req.mem_id.offset, req.size);

                // This is the actual data from emulator memory which is the shadow
                // memory conceptually.
                foundData = true;

                // Debug prints
                p_debug << "Data from emulator memory, at ADDR: " << hex << req.addr << " DATA: " << valueFromEmuMem << dec <<endl;
                p_debug << "Data stored in cache, at ADDR: " << hex << req.addr << " DATA: " << data << dec << endl;
                p_debug << "Data from local NVM memory, at ADDR: " << hex << req.addr << " DATA: " << nvm.localRead(reconstructAddress(line), 4) << dec << endl << endl;
                break;
              }
          }
        }

        // If this fails then something is really wrong with the code, like reaaaaaaaaaally
        // PROWL does not need to end up with a read in the cache if it has a cycle (see comment below)
        ASSERT(foundData == true || hash_method == 1);

        // If the data from the cache and the value from Emulator memory is not
        // same then there is a fault somewhere
        if (data != valueFromEmuMem) {
          if (hash_method == 1) {
            // For PRWOL the algorithm the data might not actually end up in the cache
            // IF the access is a Read, and the cuckooing forms a cycle back to this Read,
            // then PROWL might evict the cache entry corresponding to this Read, as it's not Dirty.
            // And PROWL simply looks for the first non-dirty entry to kick out.
            // In the PROWL paper, Algorithm 1, line 28 to 33.
            //
            // Therefore, for PROWL, we also check the Memory to compare the value to if the value
            // was not found in the cache.

            // Fetch from local memory
            address_t mem_data = readFromCache(nvm.localRead(
                  reconstructAddress(req.mem_id.tag, req.mem_id.index), 4), req.mem_id.offset, req.size);

            if (mem_data != valueFromEmuMem) {
              p_err << hex << "From local: " << mem_data <<
                            " From emulator: " << valueFromEmuMem <<
                            " at addr: " << req.addr << dec <<
                            " size:" << req.size << endl;
              ASSERT(false);
            }
          } else {
            p_err << hex << "From local: " << data <<
                          " From emulator: " << valueFromEmuMem <<
                          " at addr: " << req.addr << dec <<
                          " size:" << req.size << endl;
            ASSERT(false);
          }
        }
      }
  }

  /**
   * @brief Apply compiler hints when received from the compiler.
   * Reset all the bits for that particular address.
   * 
   * @param address The address at which the hint has to be applied.
   */
  void applyCompilerHints(uint32_t address)
  {
      // Offset the main memory start
      auto addr = address;

      // Fetch the tag, set and offset from the mem address
      auto index = addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) & ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)));
      index = index >> NUM_BITS(CACHE_BLOCK_SIZE);

      /**
       * @note Search for the cache line where the hint has to be given. If found
       * then reset the possibleWAR and the was_read flag. This will ensure that
       * eviction of the given memory causes no checkpoint.
       */
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

  /**
   * @brief Perform the hashing which fetches the set from the cache.
   * 
   * @param index The index bits from the address
   * @param addr The full address
   * @param type Type of memory access
   * @param line_number Which cache line is being used
   * 
   * @return A hashed value that can be used to access the location in cache
   * 
   * @note index is only used for SET ASSOCIATIVE and addr is only used for SKEW ASSOCIATIVE
   */
//#define PRIME_MOD 524309UL
#define PRIME_MOD 2148007997UL
  address_t cacheHash(address_t tag, address_t index, enum CacheHashMethod type, uint32_t line_number)
  {
    uint64_t hash;
    address_t hash_addr = reconstructAddress(tag, index);

    switch (type) {
      case SET_ASSOCIATIVE:
        hash = index;
        break;
      case SKEW_ASSOCIATIVE:
        // Only support 2-way skew associative as of now
        // PROWL uses a different random hash every time in the form of:
        //  hash(x) = ((a*x + b) mod p) mod n    where
        //      a and b are random, chosen at boot
        //      p is a prime number > the memory size (here total address, so base + size)
        //      n is the number of sets in the cache
        //
        //  In PROWL a and b are randomly generated every run, here we have set them constant
        //  for repeatability.
        ASSERT(no_of_lines == 2);
        if (line_number == 0)
          hash = ((3 * hash_addr + 0) % PRIME_MOD) % no_of_sets;
        else
          hash = ((9 * hash_addr + 3) % PRIME_MOD) % no_of_sets;
        break;
      default:
        // No no, you should not come here
        ASSERT(false);
    }

    return hash;
  }

  /**
   * @brief Invoke checkpoint with appropriate reason. Increment
   * required stats and write to the continuous log
   * 
   * @param reason The reason for which the checkpoint is created
   */
  void createCheckpoint(enum CheckpointReason reason)
  {
    // Only place where checkpoints are incremented
    stats.incCheckpoints();
    cost.modifyCost(Pipeline, CHECKPOINT, 0);

    // Update the cause to the stats
    stats.updateCheckpointCause(reason);

    // p_debug << "Creating checkpoint #" << stats.checkpoint.checkpoints;

    // Increment based on reasons
    switch (reason)
    {
      case CHECKPOINT_DUE_TO_WAR:
        stats.incCheckpointsDueToWAR();
        // p_debug << " due to WAR" << endl;
        break;
      case CHECKPOINT_DUE_TO_DIRTY:
        stats.incCheckpointsDueToDirty();
        // p_debug << " due to ditry ratio" << endl;
        break;
      case CHECKPOINT_DUE_TO_PERIOD:
        stats.incCheckpointsDueToPeriod();
        // p_debug << " due to period" << endl;
        break;
    }

    // Perform the actual evictions due to checkpoints
    checkpointEviction();

    // Print the log line
    log.printCheckpointStats(stats);

    // And then update when this checkpoint was created
    stats.updateLastCheckpointCycle(stats.getCurrentCycle());
  }

  /**
   * @brief Carry out the actual checkpointing. Iterate through the
   * cache and figure out which cache entry needs to be written out
   * to the NVM based on the dirty bit. Clear all bits of all cache
   * entries as a checkpoint means a clean slate.
   */
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

    // Sanity check - MUST pass
    nvm.compareMemory(false);
  }

  /**
   * @brief Update the cycle count in the stats. This will help
   * in the continuous logging and also to see the number of
   * cycles between two checkpoints.
   */
  void updateCycleCount(uint64_t cycle_count)
  {
    stats.updateCurrentCycle(cycle_count);
  }

  /**
   * @brief Set the specified bit with associated conditions
   * and actions. Update the dirty ratio if the dirty bit is
   * modified.
   * 
   * @param bit The bit to be handled @see enum CacheBits
   * @param line The line whose metadata needs to be modified
   */
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
        // p_debug << "Setting dirty bit: " << line.blocks.set_bits << " to " << dirty_ratio << endl;
        break;
    }
  }

  /**
   * @brief Clear the specified bit with associated conditions
   * and actions. Update the dirty ratio if the dirty bit is
   * modified.
   * 
   * @param bit The bit to be handled @see enum CacheBits
   * @param line The line whose metadata needs to be modified
   */
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
        // p_debug << "Clearing dirty bit: " << line.blocks.set_bits << " to " << dirty_ratio << endl;
        break;
    }
  }

  /**
   * @brief Check the value of the dirty ratio and invoke
   * checkpoint if the ratio is below a certain threshold.
   */
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

  /**
   * @brief Check the value of the dirty ratio and invoke
   * checkpoint if the number of cycles since last
   * checkpoint crosses a threshold.
   */
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

  /**
   * @brief Write back the cache content to the shadow memory
   * with option to increase the NVM reads/writes
   * 
   * @param address The address to write to
   * @param value The actual data that is being written
   * @param size The size of the data
   * @param doesCountForStats Can't describe better than the variable name
   */
  void cacheNVMwrite(address_t address, address_t value, address_t size, bool doesCountForStats)
  {
    if (doesCountForStats) {
      stats.incNVMWrites(size);
      cost.modifyCost(Pipeline, NVM_WRITE, size);
    }

    nvm.localWrite(address, value, size);
  }

  /**
   * @brief Increments stats as if there was no cache
   * 
   * @param type The type of the memory access
   * @param size The size of the data
   */
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

};

#endif /* _CACHE_MEM_HPP_ */
