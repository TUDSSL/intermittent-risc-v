#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>

// Local includes
#include "../includes/Stats.hpp"
#include "../includes/Utils.hpp"
#include "Riscv32E21Pipeline.hpp"

using namespace std;

// Cycle costs per byte of access
#define CACHE_READ_COST 1
#define CACHE_WRITE_COST CACHE_READ_COST
#define NVM_READ_COST 5
#define NVM_WRITE_COST 5

// Non-access related costs
#define CACHE_CLEAN_EVICTION_COST 1
#define CACHE_DIRTY_EVICTION_COST 2
#define CACHE_CHECKPOINT_COST 4
#define PROCESSING_HINTS_COST 1

// PROWL related costs
#define CUCKOO_ITERATION_COST 1

class CycleCost {
 private:
  uint64_t final_cycle_count;

 public:
  CycleCost() { final_cycle_count = 0; }

  ~CycleCost() = default;

  uint64_t calculateCost(Stats &stats, uint64_t cycle_count) {
    final_cycle_count =
        cycle_count
        // Remove NVM mem accesses
        - stats.nvm.nvm_reads * NVM_READ_COST -
        stats.nvm.nvm_writes * NVM_READ_COST
        // Add cache mem accesses
        + stats.cache.reads * CACHE_READ_COST +
        stats.cache.writes * CACHE_READ_COST
        // Due to checkpoints
        + stats.checkpoint.checkpoints * CACHE_CHECKPOINT_COST
        // Due to evictions
        + stats.cache.clean_evictions * CACHE_CLEAN_EVICTION_COST +
        stats.cache.dirty_evictions * CACHE_DIRTY_EVICTION_COST
        // Due to other factors
        + stats.misc.hints_given * PROCESSING_HINTS_COST;

    return final_cycle_count;
  }

  void modifyCost(RiscvE21Pipeline *Pipeline, enum CostSpecification type,
                  address_t size) {
    switch (type) {
      case CACHE_READ:
      case CACHE_WRITE:
        /* do nothing as this is the default case */
        break;
      case CACHE_ACCESS:
        Pipeline->addToCycles(CACHE_READ_COST * size);
        break;
      case NVM_READ:
        Pipeline->addToCycles(NVM_READ_COST * size - CACHE_READ_COST);
        break;
      case NVM_WRITE:
      case EVICT:
        Pipeline->addToCycles(NVM_WRITE_COST * size - CACHE_READ_COST);
        break;
      case CHECKPOINT:
        Pipeline->addToCycles(CACHE_CHECKPOINT_COST);
        break;
      case HINTS:
        Pipeline->addToCycles(PROCESSING_HINTS_COST);
        break;
      case CUCKOO_ITER:
        Pipeline->addToCycles(CUCKOO_ITERATION_COST);
        break;
    }
  }
};
