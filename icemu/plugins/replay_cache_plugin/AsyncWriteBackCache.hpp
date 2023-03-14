#pragma once

#include <list>
#include <vector>

#include "icemu/emu/types.h"
#include "icemu/emu/Emulator.h"

#include "PluginArgumentParsing.h"

#include "../includes/LocalMemory.hpp"
#include "../includes/Utils.hpp"

template <class _Arch>
class AsyncWriteBackCache {

  using arch_addr_t = typename _Arch::riscv_addr_t;

  struct WriteBackRequest {
    unsigned int pending_cycles;
    arch_addr_t addr;
    address_t data;
    address_t size;

    WriteBackRequest(unsigned int pending_cycles, arch_addr_t addr, address_t data, address_t size)
        : pending_cycles(pending_cycles), addr(addr), data(data), size(size) {}
  };

 private:

  icemu::Emulator &emu;
  LocalMemory nvm;

  /* Configuration */

  enum replacement_policy policy;
  const enum CacheHashMethod hash_method;
  const uint32_t capacity; // in bytes
  uint32_t no_of_sets;
  const uint32_t no_of_lines;
  const unsigned int writeback_delay; // in cycles

  /* Processing */

  float dirty_ratio = 0.0f;
  CacheReq req;
  std::vector<CacheSet> sets;
  std::list<WriteBackRequest> writeback_queue;

 public:

  AsyncWriteBackCache(icemu::Emulator &emu,
                      enum replacement_policy policy,
                      enum CacheHashMethod hash_method,
                      uint32_t capacity,
                      uint32_t no_of_lines,
                      unsigned int writeback_delay)
      : emu(emu),
        policy(policy),
        hash_method(hash_method),
        capacity(capacity),
        no_of_lines(no_of_lines),
        writeback_delay(writeback_delay) {
    nvm.initMem(&emu.getMemory());

    p_debug << "Hash method: " << hash_method << endl;
    if (hash_method == SKEW_ASSOCIATIVE)
      policy = SKEW;

    // Number of cache sets = capacity / (ways * block size)
    no_of_sets = capacity / (no_of_lines * NO_OF_CACHE_BLOCKS);

    // Initialize the cache
    p_debug << "CAPACITY: " << capacity << endl;
    p_debug << "SETS: " << no_of_sets << endl;
    p_debug << "Lines: " << no_of_lines << endl;
    zeroCacheContent();
  }

  AsyncWriteBackCache(const AsyncWriteBackCache &) = delete;

  AsyncWriteBackCache(AsyncWriteBackCache &&o)
      : emu(o.emu),
        nvm(std::move(o.nvm)),
        policy(o.policy),
        hash_method(o.hash_method),
        capacity(o.capacity),
        no_of_sets(o.no_of_sets),
        no_of_lines(o.no_of_lines),
        writeback_delay(o.writeback_delay),
        dirty_ratio(o.dirty_ratio),
        req(o.req),
        sets(std::move(o.sets)),
        writeback_queue(std::move(o.writeback_queue)) {
    o.req = {};
  }

  static AsyncWriteBackCache fromImplicitConfig(icemu::Emulator &emu) {
    const auto arg_cache_hash_method_val = PluginArgumentParsing::GetArguments(emu, "hash-method=");
    const auto arg_cache_size_val = PluginArgumentParsing::GetArguments(emu, "cache-size=");
    const auto arg_cache_lines_val = PluginArgumentParsing::GetArguments(emu, "cache-lines=");
    const auto arg_writeback_delay_val = PluginArgumentParsing::GetArguments(emu, "writeback-delay=");

    const auto arg_cache_hash_method = arg_cache_hash_method_val.empty()
        ? SET_ASSOCIATIVE
        : (enum CacheHashMethod)std::stoul(arg_cache_hash_method_val[0]);
    const auto arg_cache_size = arg_cache_size_val.empty()
        ? 512
        : std::stoul(arg_cache_size_val[0]);
    const auto arg_cache_lines = arg_cache_lines_val.empty()
        ? 2
        : std::stoul(arg_cache_lines_val[0]);
    const auto arg_writeback_delay = arg_writeback_delay_val.empty()
        ? 2
        : std::stoul(arg_writeback_delay_val[0]);

    return AsyncWriteBackCache(emu, LRU, arg_cache_hash_method, arg_cache_size, arg_cache_lines, arg_writeback_delay);
  }

  ~AsyncWriteBackCache() {
    // TODO: stats & reporting
  }

  /**
   * @brief Incur a power failure on the cache.
   * This will ensure the emulator's memory is in inconsistent state for dirty cache lines.
   * The cache will be cleared and all pending writebacks will be discarded.
   */
  void powerFailure() {
    // Write back 0xDEADBEEF to all dirty blocks to aid failure detection
    for (CacheSet &s : sets) {
      for (CacheLine &line : s.lines) {
        if (!line.valid) continue;
        if (!line.dirty) continue;
        nvm.emulatorWrite(reconstructAddress(line), 0xDEADBEEF, line.blocks.size);
      }
    }

    // Clear the cache
    zeroCacheContent();

    // Clear the writeback queue, discarding all pending writebacks
    writeback_queue.clear();
  }

  /**
   * @brief Advance time by the given number of cycles.
   * This may complete some pending writeback requests, or otherwise decrease the amount of
   * pending cycles.
   */
  void tick(unsigned int cycles_passed) {
    // Decrease the pending cycles of all writeback requests
    size_t n_ready = 0;
    for (auto &wb: writeback_queue) {
      if (wb.pending_cycles > cycles_passed) {
        wb.pending_cycles -= cycles_passed;
      } else {
        wb.pending_cycles = 0;
      }
      if (wb.pending_cycles == 0) ++n_ready;
    }

    // Complete and remove all writeback requests that are ready (i.e. have no pending cycles)
    size_t n_completed = 0;
    auto it = writeback_queue.begin();
    while (it != writeback_queue.end()) {
      auto &wb = *it;
      if (wb.pending_cycles != 0) break;
      completeWriteback(wb);
      ++n_completed;
      it = writeback_queue.erase(it);
    }

    ASSERT(n_ready == n_completed);
  }

  void handleRequest(address_t address, enum icemu::HookMemory::memory_type mem_type,
                     address_t *value_req, const address_t size_req) {
    configureRequest(address, *value_req, mem_type, size_req);
    processRequest();
  }

  void handleReplay(arch_addr_t address, arch_addr_t value, const address_t size) {
    configureRequest(address, value, icemu::HookMemory::MEM_WRITE, size);
    processRequest();
  }

  /**
   * @brief Perform a Cache Line Write Back (CLWB) operation on the given address and size.
   * If the corresponding cache line is not found, this will cause an assertion error.
   */
  void clwb(const arch_addr_t address, const address_t size) {
    // Simulate a read request so we can find the correct cache line
    configureRequest(address, 0, icemu::HookMemory::MEM_READ, size);

    // Find the cache line that contains the requested address
    bool hit, miss;
    unsigned int collisions;
    auto line = scanLines(hit, miss, collisions);

    // The cache line must be found, because CLWB may only be executed on
    //  cache lines that would be valid for reading
    ASSERT(line);
    ASSERT(hit);
    ASSERT(!miss);
    ASSERT(collisions == 0);

    updateCacheLastUsed(*line);
    enqueueWriteback(*line);
  }

  /**
   * @brief Wait for all cache contents to be written back to memory.
   * @return The number of cycles it would cost to complete all pending writebacks.
   */
  unsigned int fence() {
    // Tracker for how many cycles were spent waiting for writebacks at most.
    // Even if there are multiple writebacks, we only need to wait for the longest one.
    unsigned int max_cycles = 0;

    for (auto &wb: writeback_queue) {
      max_cycles = std::max(max_cycles, wb.pending_cycles);
      wb.pending_cycles = 0;
      completeWriteback(wb);
    }
    writeback_queue.clear();

    return max_cycles;
  }

 private:

  /**
   * @brief Zero out and reset the cache content.
   */
  void zeroCacheContent() {
    if (sets.empty()) {
      sets.resize(no_of_sets);
    } else {
      std::fill(sets.begin(), sets.end(), CacheSet());
      assert(sets.size() == no_of_sets);
    }

    for (auto &set: sets) {
      set.lines.resize(no_of_lines);
    }
  }

  void configureRequest(address_t address, address_t value, enum icemu::HookMemory::memory_type mem_type,
                        const address_t size) {
    memset(&req, 0, sizeof(req));

    req.addr = address;
    req.size = size;
    req.value = value;
    req.type = mem_type;
    req.mem_id.tag = req.addr >> (address_t)(log2(CACHE_BLOCK_SIZE) + log2(no_of_sets));
    req.mem_id.offset = req.addr & GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE));
    req.mem_id.index =
        (req.addr &
         GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE) + NUM_BITS(no_of_sets)) &
         ~(GET_MASK(NUM_BITS(CACHE_BLOCK_SIZE)))) >>
        NUM_BITS(CACHE_BLOCK_SIZE);

    p_debug << "REQ: " << hex << req.addr << " VALUE: " << req.value << dec
            << " OFFSET: " << req.mem_id.offset << " SIZE: " << req.size
            << endl;
  }

  void processRequest() {
    // Dirty ratio should never go negative
    ASSERT(dirty_ratio >= 0);
    ASSERT(req.mem_id.index <= no_of_sets);

    bool hit, miss;
    unsigned int collisions;
    auto line = scanLines(hit, miss, collisions);

    ASSERT(collisions <= no_of_lines);

    if (line) {
      updateCacheLastUsed(*line);
      if (hit) handleHit(*line);
      else if (miss) handleMiss(*line);
      else ASSERT(false);
    }

    // Handle eviction in case all lines are occupied
    else if (collisions == no_of_lines) {
      auto evicted_line = evictLine();
      updateCacheLastUsed(evicted_line);
      handleMiss(evicted_line);
    }

    else ASSERT(false);

    shadowCheck();
  }

  /**
   * @brief Scan the entire cache for a cache line to use.
   * @param[out] hit Whether the data was already in the cache.
   *             If true, the data can be read from the returned cache line immediately
   *             or new data can immediately be written.
   * @param[out] miss Whether the cache line was not found in the cache, but a free line was found.
   *             If true, a new cache entry can be created in the returned cache line.
   * @param[out] collisions The number of cache lines that were checked that caused a collision.
   *             If this is equal to the number of cache lines, all cache lines are occupied,
   *             and some cache line must be evicted. This implies \p hit and \p miss are false.
   * @return The cache line to use, or nullptr if all cache lines are occupied.
   */
  CacheLine *scanLines(bool &hit, bool &miss, unsigned int &collisions) {
    hit = false;
    miss = false;
    collisions = 0;

    // Look for any entry in the Cache line marked by "index"
    for (uint32_t i = 0; i < no_of_lines; i++) {
      // Based on the type of hashing, fetch the location of the line
      address_t hashed_index =
          cacheHash(req.mem_id.tag, req.mem_id.index, hash_method, i);
      CacheLine &line = sets.at(hashed_index).lines[i];

      p_debug << "Checking IDX: " << hex << hashed_index << dec << " WAY: " << i
              << endl;

      // New cache entry for the line.
      if (line.valid == false) {
        miss = true;
        p_debug << "Cache create, stored at IDX: " << hex << hashed_index << dec
                << " WAY: " << i << endl;
        return &line;
      }
      // Data is already present in the cache so there might
      // be either hit or collision.
      else {
        if (reconstructAddress(line) ==
            reconstructAddress(req.mem_id.tag, req.mem_id.index)) {
          hit = true;
          p_debug << "Cache hit, stored at IDX: " << hex << hashed_index << dec
                  << " WAY: " << i << endl;
          return &line;
        } else {
          p_debug << "Cache miss for Addr: " << hex << req.addr << dec << endl;
          collisions++;
        }
      }
    }

    return nullptr;
  }

  void handleHit(CacheLine &line) {
    switch (req.type) {
      case HookMemory::MEM_READ:
        // TODO: stats & costs

        p_debug << "Cache read req, read DATA: " << line.blocks.data << endl;
        break;

      case HookMemory::MEM_WRITE:
        setBit(DIRTY, line);

        p_debug << "Cache before write: " << hex << line.blocks.data << dec
                << endl;

        writeToCache(line);

        p_debug << "Cache write req, written DATA: " << hex << line.blocks.data
                << dec << endl;
        p_debug << "Data at NVM: " << hex
                << nvm.localRead(reconstructAddress(line), 4) << dec << endl;
        p_debug << "Data at EMULATOR: " << hex
                << nvm.emulatorRead(reconstructAddress(line), 4) << dec << endl;

        // TODO: stats & costs
        break;
    }
  }

  void handleMiss(CacheLine &line) {
    setBit(VALID, line);
    cacheStoreAddress(line, req);

    auto address = reconstructAddress(req.mem_id.tag, req.mem_id.index);
    line.blocks.data = nvm.localRead(address, 4);

    switch (req.type) {
      case HookMemory::MEM_READ:
        // TODO: stats & costs

        // We read the whole line
        line.blocks.size = 4;

        p_debug << "Cache read req, read DATA: " << line.blocks.data << endl;
        break;
      case HookMemory::MEM_WRITE:
        setBit(DIRTY, line);

        // If we don't write a full cache line, we first need to read it to fill
        // in the missing bytes
        if (req.size != 4) {
          // TODO: additional costs
        }

        // TODO: stats & costs

        p_debug << "Cache before write: " << hex << line.blocks.data << dec
                << endl;

        // In case of a write, copy the value from the CPU to the data
        writeToCache(line);

        p_debug << "Cache write req, written DATA: " << hex << line.blocks.data
                << dec << endl;
        p_debug << "Data at NVM: " << hex
                << nvm.localRead(reconstructAddress(line), 4) << dec << endl;
        p_debug << "Data at EMULATOR: " << hex
                << nvm.emulatorRead(reconstructAddress(line), 4) << dec << endl;

        break;
    }
  }

  /**
   * Evict and get that line, using the configured policy.
   * @return The line that was evicted, which has been invalidated.
   */
  CacheLine &evictLine() {
    CacheLine *evicted_line = nullptr;
    address_t hashed_index;

    // Perform the eviction based on the policy.
    switch (policy) {
      case LRU:
        hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, SET_ASSOCIATIVE, 0);
        evicted_line = &(*std::min_element(sets.at(hashed_index).lines.begin(),
                                           sets.at(hashed_index).lines.end()));
        break;
      case MRU:
        hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, SET_ASSOCIATIVE, 0);
        evicted_line = &(*std::max_element(sets.at(hashed_index).lines.begin(),
                                           sets.at(hashed_index).lines.end()));
        break;
      case SKEW:
        p_err << "SKEW policy not implemented yet" << endl;
        ASSERT(false);
        break;
    }

    ASSERT(evicted_line != nullptr);
    ASSERT(evicted_line->valid == true);

    // Reconstruct the address
    auto evict_address = reconstructAddress(*evicted_line);

    p_debug << "Evicting address: " << std::hex << evict_address << std::dec << std::endl;

    // Perform the eviction
    if (evicted_line->dirty) {
      // Enqueue a writeback request
      enqueueWriteback(*evicted_line);

      nvm.localWrite(evict_address, evicted_line->blocks.data,
                     evicted_line->blocks.size);

      // TODO: stats.incCacheDirtyEvictions();
    } else {
      // TODO: stats: incCacheCleanEvictions();
    }

    clearBit(VALID, *evicted_line);
    return *evicted_line;
  }

  void enqueueWriteback(CacheLine &line) {
    // Writing back a line that is not dirty is illegal
    ASSERT(line.dirty);
    clearBit(DIRTY, line);

    const auto addr = reconstructAddress(line);

    // Enqueue the new writeback request
    writeback_queue.emplace_back(writeback_delay, addr, line.blocks.data, line.blocks.size);
  }

  void completeWriteback(const WriteBackRequest &wb) {
    ASSERT(wb.pending_cycles == 0);

    p_debug << "Completing writeback for address: "
            << std::hex << wb.addr << std::dec << std::endl;

    nvm.localWrite(wb.addr, wb.data, wb.size);
  }

  /********************************************
   * METHODS (FULLY) COPIED FROM CacheMem.hpp *
   ********************************************/

  /**
   * @brief Write the value received from the CPU to the cache.
   * This has to be done taking into account the offset and the
   * size of the data received.
   */
  void writeToCache(CacheLine &line) {
    // The destination is the data in the cache
    uint8_t *d = (uint8_t *)&line.blocks.data;

    // The source is the data from the request
    uint8_t *s = (uint8_t *)&req.value;

    // Copy the data from the request to the cache
    for (size_t i = 0; i < req.size; i++) {
      d[i + req.mem_id.offset] = s[i];
    }

    // The size of the value stored in data is always 4 bytes
    line.blocks.size = CACHE_BLOCK_SIZE;
  }

  /**
   * @brief Read a value from a cache data block.
   * This has to be done taking into account the offset and the
   * size of the data requested.
   */
  uint32_t readFromCache(uint64_t dataToRead, address_t offset,
                         address_t size) {
    uint32_t data = 0;
    uint32_t *_data_half = ((uint32_t *)&dataToRead);
    uint8_t *d = (uint8_t *)&_data_half[0];

    // Go to the offset in the cache block
    d += offset;

    // Copy the new data into respective bits
    memcpy(&data, d, size);

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
  void shadowCheck() {
    // Shadow checking is only relevant for reads
    if (req.type != HookMemory::MEM_READ)
      return;

    address_t valueFromEmuMem;
    bool foundData = false;
    uint64_t data = 0;

    valueFromEmuMem = readFromCache(
        nvm.emulatorRead(reconstructAddress(req.mem_id.tag, req.mem_id.index), 4),
        req.mem_id.offset, req.size);

    /**
     * @note Find the data associated with the current request. At this point
     * of time the cache must have performed all eviction and replacement
     * which means that the data has to be there in the cache.
     */
    for (uint32_t i = 0; i < no_of_lines; i++) {
      address_t hashed_index = cacheHash(req.mem_id.tag, req.mem_id.index, hash_method, i);
      CacheLine &line = sets.at(hashed_index).lines[i];

      if (line.valid) {
        if (reconstructAddress(line) ==
            reconstructAddress(req.mem_id.tag, req.mem_id.index)) {
          // This is the data to be compared
          data = readFromCache(line.blocks.data, req.mem_id.offset, req.size);

          // This is the actual data from emulator memory which is the shadow
          // memory conceptually.
          foundData = true;

          // Debug prints
          p_debug << "Data from emulator memory, at ADDR: " << hex << req.addr
                  << " DATA: " << valueFromEmuMem << dec << endl;
          p_debug << "Data stored in cache, at ADDR: " << hex << req.addr
                  << " DATA: " << data << dec << endl;
          p_debug << "Data from local NVM memory, at ADDR: " << hex
                  << req.addr
                  << " DATA: " << nvm.localRead(reconstructAddress(line), 4)
                  << dec << endl
                  << endl;
          break;
        }
      }
    }

    // If this fails then something is really wrong with the code, like
    // reaaaaaaaaaally PROWL does not need to end up with a read in the cache
    // if it has a cycle (see comment below)
    ASSERT(foundData == true || hash_method == 1);

    // If the data from the cache and the value from Emulator memory is not
    // same then there is a fault somewhere
    if (data != valueFromEmuMem) {
      if (hash_method == 1) {
        // For PRWOL the algorithm the data might not actually end up in the
        // cache IF the access is a Read, and the cuckooing forms a cycle back
        // to this Read, then PROWL might evict the cache entry corresponding
        // to this Read, as it's not Dirty. And PROWL simply looks for the
        // first non-dirty entry to kick out. In the PROWL paper, Algorithm 1,
        // line 28 to 33.
        //
        // Therefore, for PROWL, we also check the Memory to compare the value
        // to if the value was not found in the cache.

        // Fetch from local memory
        address_t mem_data = readFromCache(
            nvm.localRead(reconstructAddress(req.mem_id.tag, req.mem_id.index), 4),
            req.mem_id.offset, req.size);

        if (mem_data != valueFromEmuMem) {
          p_err << hex << "From local: " << mem_data
                << " From emulator: " << valueFromEmuMem
                << " at addr: " << req.addr << dec << " size:" << req.size
                << endl;
          ASSERT(false);
        }
      } else {
        p_err << hex << "From local: " << data
              << " From emulator: " << valueFromEmuMem
              << " at addr: " << req.addr << dec << " size:" << req.size
              << endl;
        ASSERT(false);
      }
    }
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
   * @note index is only used for SET ASSOCIATIVE and addr is only used for SKEW
   * ASSOCIATIVE
   */
//#define PRIME_MOD 524309UL
#define PRIME_MOD 2148007997UL
  address_t cacheHash(address_t tag, address_t index, enum CacheHashMethod type,
                      uint32_t line_number) {
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
        //      p is a prime number > the memory size (here total address, so base
        //      + size) n is the number of sets in the cache
        //
        //  In PROWL a and b are randomly generated every run, here we have set
        //  them constant for repeatability.
        ASSERT(no_of_lines == 2);
        if (line_number == 0)
          hash = ((3 * hash_addr + 0) % PRIME_MOD) % no_of_sets;
        else
          hash = ((9 * hash_addr + 3) % PRIME_MOD) % no_of_sets;
        break;
      default:
        ASSERT(false);
        break;
    }

    return hash;
  }

  /**
   * @brief Reconstruct and return the address from a cache line.
   * The address will be the address corresponding to the
   * main memory.
   *
   * @param line Cache line containing the data
   * @return reconstructed address
   */
  address_t reconstructAddress(CacheLine &line) {
    address_t tag = line.blocks.bits.tag;
    address_t index = line.blocks.bits.index;

    address_t address =
        tag << (NUM_BITS(no_of_sets) + NUM_BITS(CACHE_BLOCK_SIZE)) |
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
  address_t reconstructAddress(address_t tag, address_t index) {
    address_t address =
        tag << (NUM_BITS(no_of_sets) + NUM_BITS(CACHE_BLOCK_SIZE)) |
        index << (NUM_BITS(CACHE_BLOCK_SIZE));
    return address;
  }

  /**
   * @brief Set the specified bit with associated conditions
   * and actions. Update the dirty ratio if the dirty bit is
   * modified.
   *
   * @param bit The bit to be handled @see enum CacheBits
   * @param line The line whose metadata needs to be modified
   */
  void setBit(enum CacheBits bit, CacheLine &line) {
    switch (bit) {
      case VALID:
        line.valid = true;
        break;
      case DIRTY:
        ASSERT(line.valid == true);
        // Check if not already set, set the bit and update the dirty ratio
        if (line.dirty == false) {
          line.dirty = true;
          dirty_ratio = (dirty_ratio * (capacity / CACHE_BLOCK_SIZE) + 1) /
                        (capacity / CACHE_BLOCK_SIZE);
          ASSERT(dirty_ratio <= 1.0);
        }
        // p_debug << "Setting dirty bit: " << line.blocks.set_bits << " to " <<
        // dirty_ratio << endl;
        break;
      default:
        p_err << "Only VALID and DIRTY should be set through here" << std::endl;
        ASSERT(false);
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
  void clearBit(enum CacheBits bit, CacheLine &line) {
    switch (bit) {
      case VALID:
        // Should never clear a valid bit twice
        ASSERT(line.valid == true);
        line.valid = false;
        break;
      case DIRTY:
        // Check if not already cleared, clear the bit and update the dirty ratio
        if (line.dirty == true) {
          line.dirty = false;
          dirty_ratio = (dirty_ratio * (capacity / CACHE_BLOCK_SIZE) - 1) /
                        (capacity / CACHE_BLOCK_SIZE);
          ASSERT(dirty_ratio >= 0.0);
        }
        break;
      default:
        p_err << "Only VALID and DIRTY are supported through here" << std::endl;
        ASSERT(false);
        break;
    }
  }
};
