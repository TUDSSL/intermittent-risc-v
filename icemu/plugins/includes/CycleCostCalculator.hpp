#ifndef _CYCLE_COST_CALCULATOR
#define _CYCLE_COST_CALCULATOR

#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>

// Local includes
#include "../includes/Stats.hpp"

using namespace std;

// Cycle costs per byte of access
#define CACHE_READ_COST 1
#define CACHE_WRITE_COST 1
#define NVM_READ_COST 2
#define NVM_WRITE_COST 2

// Non-access related costs
#define CACHE_CLEAN_EVICTION_COST 1
#define CACHE_DIRTY_EVICTION_COST 2
#define CACHE_CHECKPOINT_COST 4
#define PROCESSING_HINTS_COST 1

class CycleCost {
  private:
    uint64_t final_cycle_count;

  public:

    CycleCost()
    {
        final_cycle_count = 0;
    }

    ~CycleCost() = default;

    uint64_t calculateCost(Stats &stats, uint64_t cycle_count)
    {
        final_cycle_count = cycle_count
                                // Remove NVM mem accesses
                                - stats.nvm.nvm_reads * NVM_READ_COST
                                - stats.nvm.nvm_writes * NVM_READ_COST
                                // Add cache mem accesses
                                + stats.cache.reads * CACHE_READ_COST
                                + stats.cache.writes * CACHE_READ_COST
                                // Due to checkpoints
                                + stats.checkpoint.checkpoints * CACHE_CHECKPOINT_COST
                                // Due to evictions
                                + stats.cache.clean_evictions * CACHE_CLEAN_EVICTION_COST
                                + stats.cache.dirty_evictions * CACHE_DIRTY_EVICTION_COST
                                // Due to other factors
                                + stats.misc.hints_given * PROCESSING_HINTS_COST;

        return final_cycle_count;
    }
};

#endif /* _CYCLE_COST_CALCULATOR */