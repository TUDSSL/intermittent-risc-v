#ifndef _CYCLE_COST_CALCULATOR
#define _CYCLE_COST_CALCULATOR

#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>

// Local includes
#include "../includes/Stats.hpp"

using namespace std;

#define CACHE_READ_COST
#define CACHE_WRITE_COST
#define NVM_READ_COST
#define NVM_WRITE_COST
#define CACHE_CLEAN_EVICTION_COST
#define CACHE_DIRTY_EVICTION_COST
#define CACHE_CHECKPOINT_COST
#define PROCESSING_HINTS_COST

class CycleCost {
  private:
    uint64_t final_cycle_count;

    void CacheAccessCosts(Stats &stats, uint64_t cycle_count)
    {
        final_cycle_count = cycle_count;
    }

    void CacheEvictionCosts(Stats &stats, uint64_t cycle_count)
    {
        final_cycle_count = cycle_count;
    }

    void HintsCosts(Stats &stats, uint64_t cycle_count)
    {
        final_cycle_count = cycle_count;
    }


  public:
    struct CacheStats cache;
    struct NVMStats nvm;
    struct CheckpointStats checkpoint;
    struct MiscStats misc;

    CycleCost()
    {
        final_cycle_count = 0;
    }

    ~CycleCost() = default;

    uint64_t calculateCost(Stats &stats, uint64_t cycle_count)
    {
        CacheAccessCosts(stats, cycle_count);
        CacheEvictionCosts(stats, cycle_count);
        HintsCosts(stats, cycle_count);

        return final_cycle_count;
    }
};

#endif /* _CYCLE_COST_CALCULATOR */