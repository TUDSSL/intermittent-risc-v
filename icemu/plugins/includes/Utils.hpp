#ifndef _UTILS
#define _UTILS

#include <string>
#include <chrono>
#include <iostream>
#include <execinfo.h>

#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"

using namespace std;

#define PRINT_DEBUG
static const bool MIMIC_CHECKPOINT_TAKEN = true;
static const float DIRTY_RATIO_THRESHOLD = 1.1;
static const bool NVM_STATS_PER_BYTE = true;
static const uint8_t CUCKOO_MAX_ITER = 8;

static const double FREQ = 50 * 1000 * 1000; // MHz
static const double TIME_FOR_CHECKPOINT_THRESHOLD = 20.0 / 1000.0; // in ms
static const double CYCLE_COUNT_CHECKPOINT_THRESHOLD = TIME_FOR_CHECKPOINT_THRESHOLD * FREQ; // checkpoint after these cycles
static const int NO_OF_CACHE_BLOCKS = 4;

#define CACHE_BLOCK_SIZE NO_OF_CACHE_BLOCKS
#define NUM_BITS(_n) ((uint32_t)log2(_n))
#define GET_MASK(_n) ((uint32_t) ((uint64_t)1 << (_n)) - 1)

#define p_err cerr
#ifdef PRINT_DEBUG
#define p_debug cout
#else
std::ofstream p_debug("/dev/null");;
#endif

enum replacement_policy {
  LRU,
  MRU,
  SKEW
};

enum CacheHashMethod {
  SET_ASSOCIATIVE,
  SKEW_ASSOCIATIVE
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
  HINTS,
  CUCKOO_ITER,
  CACHE_ACCESS
};

enum CacheBits {
  VALID,
  READ_DOMINATED,
  WRITE_DOMINATED,
  POSSIBLE_WAR,
  DIRTY
};

/**
 * @brief The bits that identify the memory in a cache.
 *
 * @tag_bits: Bits to represent the significat memory bits
 * @set_bits: Bits to decide which set of the cache is being used
 * @index_bits: Bits to decide the byte/word being referred to inside a set
 */
struct MemId {
  address_t tag;
  address_t index;
  address_t offset;
};

struct CacheReq {
  MemId mem_id;
  address_t addr;
  enum HookMemory::memory_type type;
  address_t value;
  address_t size;
};

/**
 * @brief The smallest unit of a cache that stores the data.
 * A group of blocks form a cache line (or way)
 * A group of cache lines form a cache set
 * A group of cache sets form a cache
 *
 * @data: The block of data to be stored
 * @last_used: Time it was last used
 * @addr: Store the actual address - for easy reference
 *
 * SUM_OF_BITS(tag_bits, set_bits, offset_bits) = Memory address lines. (32 bit in this case)
 */
struct CacheBlock {
  address_t data; // actually store the data in blocks of bytes
  address_t last_used;
  address_t size;
  MemId bits;

  // Overload the = operator to compare evicted blocks
  bool operator==(const CacheBlock &other) const {
    return (bits.tag == other.bits.tag &&
           bits.index == other.bits.index &&
           bits.offset == other.bits.offset);
  }

};

// Hash function for the evicted_blocks unordered map
struct hash_fn
{
    size_t operator()(const CacheBlock &block) const
    {
        std::size_t h1 = std::hash<address_t>()(block.bits.offset);
        std::size_t h2 = std::hash<address_t>()(block.bits.index);
        std::size_t h3 = std::hash<address_t>()(block.bits.tag);
 
        return h1 ^ h2 ^ h3;
    }
};

/**
 * @brief A cache line that consists of blocks. A 2-way set associative cache will have
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

static inline void updateCacheLastUsed(CacheLine &line)
{
  line.blocks.last_used = std::chrono::duration_cast<std::chrono::nanoseconds>
                          (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#define ASSERT(condition) { \
          if (!(condition)) {  \
            std::cerr << "ASSERT FAILED: " << #condition << " @ " << __FILE__ << " (" << __LINE__ << ")" << std::endl; \
            print_trace(); \
            assert(condition); \
          } \
        }


void print_trace (void)
{
  void *array[10];
  size_t size;
  char **strings;

  size = backtrace(array, 10);
  strings = backtrace_symbols(array, size);

  printf("Obtained %zd stack frames.\n", size);

  for (size_t i = 0; i < size; i++)
     printf("%s\n", strings[i]);

  free(strings);
}

static inline void cacheStoreAddress(CacheLine &line, const CacheReq &req) {
  line.blocks.bits.tag = req.mem_id.tag;
  line.blocks.bits.offset = req.mem_id.offset;
  line.blocks.bits.index = req.mem_id.index;
}

void copyCacheLines(struct CacheLine &dst, const struct CacheLine &src)
{
  dst.blocks.data = src.blocks.data;
  dst.blocks.bits.index = src.blocks.bits.index;
  dst.blocks.bits.tag = src.blocks.bits.tag;
  dst.blocks.bits.offset = src.blocks.bits.offset;
  dst.blocks.size = src.blocks.size;
  dst.blocks.last_used = src.blocks.last_used;
  dst.valid = src.valid;
  dst.dirty = src.dirty;
  dst.possible_war = src.possible_war;
  dst.read_dominated = src.read_dominated;
  dst.write_dominated = src.write_dominated;
}



#endif /* _UTILS */
