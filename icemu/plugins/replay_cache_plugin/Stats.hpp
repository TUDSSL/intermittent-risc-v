#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../includes/Utils.hpp"

struct CacheStats {
  int misses;
  int hits;
  int evictions;
  int reads;
  int writes;
  int clean_evictions;
  int dirty_evictions;
  int clwb;
  int writebacks_enqueued;
  int writebacks_completed;
  int writebacks_completed_before_fence;
  int fence;
  std::vector<int> fence_wait_cycles;
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
  int region_starts;
  int region_ends;
  std::vector<int> region_sizes;
  std::vector<int> stores_per_region;
  int replays;
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
  ReplayCacheStats replay_cache;
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
  void incCacheClwb() { cache.clwb++; }
  void incCacheWritebacksEnqueued() { cache.writebacks_enqueued++; }
  void incCacheWritebacksCompleted() { cache.writebacks_completed++; }
  void incCacheWritebacksCompletedBeforeFence(int n = 1) { cache.writebacks_completed_before_fence += n; }
  void incCacheFence() { cache.fence++; }
  void addCacheFenceWaitCycles(int cycles) { cache.fence_wait_cycles.push_back(cycles); }

  /* NVM statistics updates */

  void incNVMReads(int n = 1) { nvm.reads += n; }
  void incNVMWrites(int n = 1) { nvm.writes += n; }

  /* Checkpoint statistics updates */

  void incCheckpoints() { checkpoint.checkpoints++; }
  void incCheckpointsDueToPeriod() { checkpoint.due_to_period++; }
  void incCheckpointsDueToExplicit() { checkpoint.due_to_explicit++; }
  void incRestores() { checkpoint.restores++; }

  /* ReplayCache updates */

  void incRegionStarts() { replay_cache.region_starts++; }
  void incRegionEnds() { replay_cache.region_ends++; }
  void addRegionSize(int size) { replay_cache.region_sizes.push_back(size); }
  void addStoresPerRegion(int stores) { replay_cache.stores_per_region.push_back(stores); }
  void incReplays() { replay_cache.replays++; }

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
    std::cout << " CLWB: " << cache.clwb << std::endl;
    std::cout << " Writebacks enqueued: " << cache.writebacks_enqueued << std::endl;
    std::cout << " Writebacks completed: " << cache.writebacks_completed << std::endl;
    std::cout << " Writebacks completed before FENCE: " << cache.writebacks_completed_before_fence << std::endl;
    std::cout << " FENCE: " << cache.fence << std::endl;
    std::cout << " FENCE wait cycles: ";
    for (auto cycles : cache.fence_wait_cycles)
      std::cout << cycles << " ";
    std::cout << std::endl;

    std::cout << "NVM STATS" << std::endl;
    std::cout << " Reads: " << nvm.reads << std::endl;
    std::cout << " Writes: " << nvm.writes << std::endl;

    std::cout << "CHECKPOINT STATS" << std::endl;
    std::cout << " Checkpoints: " << checkpoint.checkpoints << std::endl;
    std::cout << " Due to period: " << checkpoint.due_to_period << std::endl;
    std::cout << " Due to explicit: " << checkpoint.due_to_explicit << std::endl;
    std::cout << " Restores: " << checkpoint.restores << std::endl;

    std::cout << "REPLAYCACHE STATS" << std::endl;
    std::cout << " Region starts: " << replay_cache.region_starts << std::endl;
    std::cout << " Region ends: " << replay_cache.region_ends << std::endl;
    std::cout << " Region sizes: ";
    for (auto size : replay_cache.region_sizes)
      std::cout << size << " ";
    std::cout << std::endl;
    std::cout << " Stores per region: ";
    for (auto stores : replay_cache.stores_per_region)
      std::cout << stores << " ";
    std::cout << std::endl;
    std::cout << " Replays: " << replay_cache.replays << std::endl;

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
    out << "cache_clwb:" << cache.clwb << std::endl;
    out << "cache_writeback_enqueued:" << cache.writebacks_enqueued << std::endl;
    out << "cache_writeback_completed:" << cache.writebacks_completed << std::endl;
    out << "cache_writeback_completed_before_fence:" << cache.writebacks_completed_before_fence << std::endl;
    out << "cache_fence:" << cache.fence << std::endl;
    out << "cache_fence_wait_cycles:";
    for (auto cycles : cache.fence_wait_cycles)
      out << cycles << " ";
    out << std::endl;

    out << "nvm_reads:" << nvm.reads << std::endl;
    out << "nvm_writes:" << nvm.writes << std::endl;

    out << "checkpoint:" << checkpoint.checkpoints << std::endl;
    out << "checkpoint_period:" << checkpoint.due_to_period << std::endl;
    out << "checkpoint_explicit:" << checkpoint.due_to_explicit << std::endl;
    out << "restore:" << checkpoint.restores << std::endl;

    out << "region_start:" << replay_cache.region_starts << std::endl;
    out << "region_end:" << replay_cache.region_ends << std::endl;
    out << "region_size:";
    for (auto size : replay_cache.region_sizes)
      out << size << " ";
    out << std::endl;
    out << "stores_per_region:";
    for (auto stores : replay_cache.stores_per_region)
      out << stores << " ";
    out << std::endl;
    out << "replay:" << replay_cache.replays << std::endl;

    out << "max_dirty_ratio:" << misc.max_dirty_ratio << std::endl;
    out << "dirty_ratio:" << misc.dirty_ratio << std::endl;
    out << "on_duration_cycles:" << misc.on_duration << std::endl;
  }

};
