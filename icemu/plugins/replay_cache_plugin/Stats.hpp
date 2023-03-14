#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>

#include "../includes/Utils.hpp"

struct CacheStats {
  int misses;
  int hits;
  int evictions;
  int reads;
  int writes;
  int clean_evictions;
  int dirty_evictions;
};

struct NVMStats {
  int writes;
  int reads;
};

struct CheckpointStats {
  int checkpoints;
  int due_to_period;
  int due_to_explicit;
  int restores;
};

struct ReplayCacheStats {
  // TODO
};

struct MiscStats {
  float max_dirty_ratio;
  float dirty_ratio;
  uint64_t on_duration;
};

class Stats {

 private:

  CacheStats cache;
  NVMStats nvm;
  CheckpointStats checkpoint;
  MiscStats misc;

 public:

  /* Cache statistics updates */

  void incCacheMisses() { cache.misses++; }
  void incCacheHits() { cache.hits++; }
  void incCacheEvictions() { cache.evictions++; }
  void incCacheReads(int n) { cache.reads += n; }
  void incCacheWrites(int n) { cache.writes += n; }
  void incCacheCleanEvictions() { cache.clean_evictions++; }
  void incCacheDirtyEvictions() { cache.dirty_evictions++; }

  /* NVM statistics updates */

  void incNVMReads(int n = 1) { nvm.reads += n; }
  void incNVMWrites(int n = 1) { nvm.writes += n; }

  /* Checkpoint statistics updates */

  void incCheckpoints() { checkpoint.checkpoints++; }
  void incCheckpointsDueToPeriod() { checkpoint.due_to_period++; }
  void incCheckpointsDueToExplicit() { checkpoint.due_to_explicit++; }
  void incRestores() { checkpoint.restores++; }

  /* ReplayCache updates */

  // TODO

  /* Misc statistics updates */

  void updateDirtyRatio(float ratio) {
    misc.dirty_ratio = ratio;
    if (misc.dirty_ratio > misc.max_dirty_ratio)
      misc.max_dirty_ratio = misc.dirty_ratio;
  }

  void updateOnDuration(uint64_t duration) { misc.on_duration = duration; }

  /* Print statistics */

  void printAll() {
    std::cout << "\n-------------------------------------" << std::endl;

    std::cout << "CACHE STATS" << std::endl;
    std::cout << " Misses: " << cache.misses << std::endl;
    std::cout << " Hits: " << cache.hits << std::endl;
    std::cout << " Evictions: " << cache.evictions << std::endl;
    std::cout << " Reads: " << cache.reads << std::endl;
    std::cout << " Writes: " << cache.writes << std::endl;
    std::cout << " Clean evictions: " << cache.clean_evictions << std::endl;
    std::cout << " Dirty evictions: " << cache.dirty_evictions << std::endl;

    std::cout << "NVM STATS" << std::endl;
    std::cout << " Reads: " << nvm.reads << std::endl;
    std::cout << " Writes: " << nvm.writes << std::endl;

    std::cout << "CHECKPOINT STATS" << std::endl;
    std::cout << " Checkpoints: " << checkpoint.checkpoints << std::endl;
    std::cout << " Due to period: " << checkpoint.due_to_period << std::endl;
    std::cout << " Due to explicit: " << checkpoint.due_to_explicit << std::endl;
    std::cout << " Restores: " << checkpoint.restores << std::endl;

    std::cout << "REPLAY CACHE STATS" << std::endl;
    std::cout << " TODO" << std::endl;

    std::cout << "MISC STATS" << std::endl;
    std::cout << " Max dirty ratio: " << misc.max_dirty_ratio << std::endl;
    std::cout << " Dirty ratio (last): " << misc.dirty_ratio << std::endl;
    std::cout << " On duration: " << misc.on_duration << std::endl;

    std::cout << "-------------------------------------" << std::endl;
  }

  void logAll(std::ofstream &out) {
    out << "cache_miss:" << cache.misses << std::endl;
    out << "cache_hit:" << cache.hits << std::endl;
    out << "cache_eviction:" << cache.evictions << std::endl;
    out << "cache_read:" << cache.reads << std::endl;
    out << "cache_write:" << cache.writes << std::endl;
    out << "cache_clean_eviction:" << cache.clean_evictions << std::endl;
    out << "cache_dirty_eviction:" << cache.dirty_evictions << std::endl;

    out << "nvm_reads:" << nvm.reads << std::endl;
    out << "nvm_writes:" << nvm.writes << std::endl;

    out << "checkpoint:" << checkpoint.checkpoints << std::endl;
    out << "checkpoint_period:" << checkpoint.due_to_period << std::endl;
    out << "checkpoint_explicit:" << checkpoint.due_to_explicit << std::endl;
    out << "restore:" << checkpoint.restores << std::endl;

    // TODO: ReplayCache stats

    out << "max_dirty_ratio:" << misc.max_dirty_ratio << std::endl;
    out << "dirty_ratio:" << misc.dirty_ratio << std::endl;
    out << "on_duration_cycles:" << misc.on_duration << std::endl;
  }

};
