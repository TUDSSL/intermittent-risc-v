#ifndef _STATS
#define _STATS

#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>

// Local includes
#include "../includes/Utils.hpp"

using namespace std;
using namespace icemu;

struct CacheStats {
    uint32_t misses;
    uint32_t hits;
    uint32_t reads;
    uint32_t writes;
    uint32_t clean_evictions;
    uint32_t dirty_evictions;
    uint32_t cache_cuckoo;
    uint32_t cache_checkpoint;
};

struct NVMStats {
    uint32_t nvm_writes;
    uint32_t nvm_reads;
    uint32_t nvm_reads_without_cache;
    uint32_t nvm_writes_without_cache;
};

struct CheckpointStats {
    uint32_t checkpoints;
    uint32_t due_to_war;
    uint32_t due_to_dirty;
    uint32_t due_to_period;
    uint64_t last_checkpoint_cycle;
    uint64_t cycles_between_checkpoints;
    enum CheckpointReason cause;
    
};

struct MiscStats {
    uint32_t hints_given;
    float max_dirty_ratio;
    float dirty_ratio;
    uint64_t current_cycle;
    uint32_t no_of_cuckoos;
};

class Stats {
  private:    
    char separator = ' ';
    int text_width = 30;
    int num_width = 10;
    
    template<typename T> void printNum(T t)
    {
        cout << left << setw(text_width) << setfill(separator) << t;
    }

    template<typename T> void printText(T t)
    {
        cout << left << setw(text_width) << setfill(separator) << t;
    }

    template<typename T, typename U> void print(T t1, U t2)
    {
        cout << endl;
        printText(t1);
        printNum(t2);
    }

    template<typename T> void logNum(T t, ofstream &logger)
    {
        logger << left << t;
    }

    template<typename T> void logText(T t, ofstream &logger)
    {
        logger << left << t;
    }

    template<typename T, typename U> void log(T t1, U t2, ofstream &logger)
    {
        logger << endl;
        logText(t1, logger);
        logNum(t2, logger);
    }

  public:
    struct CacheStats cache;
    struct NVMStats nvm;
    struct CheckpointStats checkpoint;
    struct MiscStats misc;
    
    Stats() {
        cache = {};
        nvm = {};
        checkpoint = {};
        misc = {};
    }
    ~Stats() = default;

    void printStats() {
        cout << "\n-------------------------------------";
        print("Cache misses:", cache.misses);
        print("Cache hits:", cache.hits);
        print("Cache reads:", cache.reads);
        print("Cache writes:", cache.writes);
        print("Cache Cuckoo:", cache.cache_cuckoo);
        print("Cache Checkpoints:", cache.cache_checkpoint);

        print("NVM reads w/o cache:", nvm.nvm_reads_without_cache);
        print("NVM writes w/o cache:", nvm.nvm_writes_without_cache);
        print("NVM reads:", nvm.nvm_reads);
        print("NVM writes:", nvm.nvm_writes);

        print("Checkpoints:", checkpoint.checkpoints);
        print("Checkpoints due to WAR:", checkpoint.due_to_war);
        print("Checkpoints due to dirty:", checkpoint.due_to_dirty);
        print("Checkpoints due to period:", checkpoint.due_to_period);
        print("Max cycles b/ checkpoints: ", checkpoint.cycles_between_checkpoints);

        print("Misc: hints given:", misc.hints_given);
        print("Misc: Max ratio", misc.max_dirty_ratio);
        print("Misc: No of cuckoo iter:", misc.no_of_cuckoos);
        cout << "\n-------------------------------------" << endl;
    }

    void logStats(ofstream &logger) {
        log("cache_miss:", cache.misses, logger);
        log("cache_hit:", cache.hits, logger);
        log("cache_read:", cache.reads, logger);
        log("cache_write:", cache.writes, logger);
        log("cache_cuckoo:", cache.cache_cuckoo, logger);
        log("cache_checkpoint:", cache.cache_checkpoint, logger);

        log("nvm_reads_no_cache:", nvm.nvm_reads_without_cache, logger);
        log("nvm_writes_no_cache:", nvm.nvm_writes_without_cache, logger);
        log("nvm_reads:", nvm.nvm_reads, logger);
        log("nvm_writes:", nvm.nvm_writes, logger);

        log("checkpoint:", checkpoint.checkpoints, logger);
        log("checkpoint_war:", checkpoint.due_to_war, logger);
        log("checkpoint_dirty:", checkpoint.due_to_dirty, logger);
        log("checkpoint_period:", checkpoint.due_to_period, logger);
        log("checkpoint_max_cycles: ", checkpoint.cycles_between_checkpoints, logger);

        log("hints_given:", misc.hints_given, logger);
        log("max_dirty_ratio:", misc.max_dirty_ratio, logger);
        log("cuckoo_iter:", misc.no_of_cuckoos, logger);
        log("cycle:", misc.current_cycle, logger);
        logger << endl;
    }

    void updateDirtyRatio(float ratio)
    {
        misc.dirty_ratio = ratio;
        if (misc.dirty_ratio > misc.max_dirty_ratio)
            misc.max_dirty_ratio = misc.dirty_ratio;   
    }

    // Update cache stats
    void incCacheHits() { cache.hits++; }
    void incCacheMiss() { cache.misses++; }
    void incCacheReads(uint64_t size) { cache.reads += size; }
    void incCacheWrites(uint64_t size) { cache.writes += size; }
    void incCacheCleanEvictions() { cache.clean_evictions++; }
    void incCacheDirtyEvictions() { cache.dirty_evictions++; }
    void incCacheCuckoo(uint64_t size) { cache.cache_cuckoo += size;}
    void incCacheCheckpoint(uint64_t size) { cache.cache_checkpoint += size;}

    // Update NVM stats
    void incNVMReads(uint64_t size) { nvm.nvm_reads += size; }
    void incNVMWrites(uint64_t size) { nvm.nvm_writes += size; }
    void incNonCacheNVMReads(uint64_t size) { nvm.nvm_reads_without_cache += size; }
    void incNonCacheNVMWrites(uint64_t size) { nvm.nvm_writes_without_cache += size; }
    
    // Update misc stats
    void incHintsGiven() { misc.hints_given++; }
    void updateCurrentCycle(uint64_t cycle) { misc.current_cycle = cycle; }
    uint64_t getCurrentCycle() { return misc.current_cycle; }
    void incCuckooIterations() { misc.no_of_cuckoos++; }
    
    // Update checkpoint stats
    void incCheckpoints() { checkpoint.checkpoints++; }
    void updateCheckpointCause(enum CheckpointReason cause) { checkpoint.cause = cause; }
    void incCheckpointsDueToWAR() { checkpoint.due_to_war++; }
    void incCheckpointsDueToPeriod() { checkpoint.due_to_period++; }
    void incCheckpointsDueToDirty() { checkpoint.due_to_dirty++; }
    void updateLastCheckpointCycle(uint64_t cycle) { checkpoint.last_checkpoint_cycle = cycle; }
    uint64_t getLastCheckpointCycle() { return checkpoint.last_checkpoint_cycle; }
};

#endif /* _STATS */