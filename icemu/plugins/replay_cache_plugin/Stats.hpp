#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <numeric>

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
  std::vector<unsigned int> checkpoints; //< Cycle count per checkpoint
  int due_to_period;
  int due_to_explicit;
  int restores;
  int restore_cycles;
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
  RiscvE21Pipeline &pipeline;

 public:

  /* Cache statistics updates */
  Stats(RiscvE21Pipeline &pipeline) : pipeline(pipeline) {}

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

  void incCheckpoints(unsigned int cycle_count) { checkpoint.checkpoints.emplace_back(cycle_count); }
  void incCheckpointsDueToPeriod() { checkpoint.due_to_period++; }
  void incCheckpointsDueToExplicit() { checkpoint.due_to_explicit++; }
  void incRestores() { checkpoint.restores++; }
  void incRestoreCycles(int cycles) { checkpoint.restore_cycles += cycles; }

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

  void printAll(std::ostream &out) const {
    out << "CACHE STATS" << std::endl;
    out << " Misses: " << cache.misses << std::endl;
    out << " Hits: " << cache.hits << std::endl;
    out << " Evictions: " << cache.evictions << std::endl;
    out << " Reads: " << cache.reads << std::endl;
    out << " Writes: " << cache.writes << std::endl;
    out << " Clean evictions: " << cache.clean_evictions << std::endl;
    out << " Dirty evictions: " << cache.dirty_evictions << std::endl;
    out << " CLWB: " << cache.clwb << std::endl;
    out << " Writebacks enqueued: " << cache.writebacks_enqueued << std::endl;
    out << " Writebacks completed: " << cache.writebacks_completed << std::endl;
    out << " Writebacks completed before FENCE: " << cache.writebacks_completed_before_fence << " [" << ((double)cache.writebacks_completed_before_fence / (double)cache.writebacks_completed) * 100.0 << "%]" << std::endl;
    out << " FENCE: " << cache.fence << std::endl;
    out << " Total FENCE wait cycles: " << std::accumulate(cache.fence_wait_cycles.begin(), cache.fence_wait_cycles.end(), 0) << " [" << ((double)std::accumulate(cache.fence_wait_cycles.begin(), cache.fence_wait_cycles.end(), 0) / (double)pipeline.getTotalCycles()) * 100.0 << "% of total]" << std::endl;
    // out << " FENCE wait cycles: ";
    // for (auto cycles : cache.fence_wait_cycles)
    //   out << cycles << " ";
    // out << std::endl;

    out << "NVM STATS" << std::endl;
    out << " Reads: " << nvm.reads << std::endl;
    out << " Writes: " << nvm.writes << std::endl;

    out << "CHECKPOINT STATS" << std::endl;
    out << " Checkpoints: " << checkpoint.checkpoints.size() << std::endl;
    out << " Due to period: " << checkpoint.due_to_period << std::endl;
    out << " Due to explicit: " << checkpoint.due_to_explicit << std::endl;
    out << " Restores: " << checkpoint.restores << std::endl;
    out << " Overhead cycles (checkpoint + restore): " << checkpoint.restore_cycles << " [" << ((double) checkpoint.restore_cycles / (double)pipeline.getTotalCycles()) * 100.0 << "% of total]" << std::endl;

    out << "REPLAYCACHE STATS" << std::endl;
    out << " Region starts: " << replay_cache.region_starts << std::endl;
    out << " Region ends: " << replay_cache.region_ends << std::endl;
    // out << " Region sizes: ";
    // for (auto size : replay_cache.region_sizes)
    //   out << size << " ";
    // out << std::endl;
    // out << " Stores per region: ";
    // for (auto stores : replay_cache.stores_per_region)
    //   out << stores << " ";
    // out << std::endl;
    out << " Replays: " << replay_cache.replays << std::endl;

    out << "MISC STATS" << std::endl;
    out << " Max dirty ratio: " << misc.max_dirty_ratio << std::endl;
    out << " Dirty ratio (last): " << misc.dirty_ratio << std::endl;
    out << " On duration: " << misc.on_duration << std::endl;
  }

  void logAll(std::ofstream &out) const {
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

    out << "checkpoint:" << checkpoint.checkpoints.size() << std::endl;
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

  std::string getContinuousLine() const {
    std::string res;

    // "checkpoints"
    res += std::to_string(checkpoint.checkpoints.size());
    res += ',';
    // "cycle count"
    res += std::to_string(checkpoint.checkpoints.back());
    res += ',';
    // "last checkpoint" (difference in cycles since last checkpoint, or the total cycle count if this is the first checkpoint)
    if (checkpoint.checkpoints.size() > 1) {
      res += std::to_string(checkpoint.checkpoints.back() - checkpoint.checkpoints[checkpoint.checkpoints.size() - 2]);
    } else {
      res += std::to_string(checkpoint.checkpoints.back());
    }
    res += ',';
    // "dirty ratio"
    res += std::to_string(misc.dirty_ratio);
    res += ',';
    // "cause", always 3 for ReplayCache
    res += "3";

    return res;
  }

};
