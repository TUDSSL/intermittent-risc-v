#ifndef _UTILS
#define _UTILS

#include <string>
#include <chrono>

#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"

using namespace std;

static const bool MIMIC_CHECKPOINT_TAKEN = true;
static const float DIRTY_RATIO_THRESHOLD = 0.6;
static const bool NVM_STATS_PER_BYTE = true;

static const uint64_t FREQ = 50 * 1000 * 1000; // MHz
static const double TIME_FOR_CHECKPOINT_THRESHOLD = 50 / 1000; // in ms
static const uint64_t CYCLE_COUNT_CHECKPOINT_THRESHOLD = TIME_FOR_CHECKPOINT_THRESHOLD * FREQ; // checkpoint after these cycles

// The cache block size in number of bytes
#define NO_OF_CACHE_BLOCKS 4 // Equates to the cache block size of 4 bytes
#define CACHE_BLOCK_SIZE NO_OF_CACHE_BLOCKS
#define NUM_BITS(_n) ((uint32_t)log2(_n))
#define GET_MASK(_n) ((uint32_t) ((uint64_t)1 << (_n)) - 1)

enum replacement_policy {
  LRU,
  MRU
};

enum CheckpointReason {
  CHECKPOINT_DUE_TO_WAR,
  CHECKPOINT_DUE_TO_DIRTY,
  CHECKPOINT_DUE_TO_PERIOD
};

enum CostSpecification {
  CACHE_READ,
  CACHE_WRITE,
  NVM_READ,
  NVM_WRITE,
  EVICT,
  CHECKPOINT,
  HINTS
};

enum CacheBits {
  VALID,
  READ_DOMINATED,
  WRITE_DOMINATED,
  POSSIBLE_WAR,
  DIRTY
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
  bool read_dominated;
  bool write_dominated;
  bool possible_war;

  // Overload the < operator to enable LRU/MRU
  bool operator<(const CacheLine &other) const {
    return blocks.last_used < other.blocks.last_used;
  }
};

struct CacheSet {
  std::vector<CacheLine> lines;
};

static inline uint64_t CurrentTime_nanoseconds()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>
         (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#endif /* _UTILS */