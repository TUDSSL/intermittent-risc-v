#ifndef _STATS
#define _STATS

#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>

using namespace std;
using namespace icemu;

struct CacheStats {
    uint32_t misses;
    uint32_t hits;
    uint32_t reads;
    uint32_t writes;
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
};

struct MiscStats {
    uint32_t hints_given;
    float max_dirty_ratio;
};

class Stats {
  private:
    char separator = ' ';
    int text_width = 26;
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
        logger << left << setw(text_width) << setfill(separator) << t;
    }

    template<typename T> void logText(T t, ofstream &logger)
    {
        logger << left << setw(text_width) << setfill(separator) << t;
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

    Stats() = default;
    ~Stats() = default;

    void printStats() {
        cout << "\n-------------------------------------";
        print("Cache misses:", cache.misses);
        print("Cache hits:", cache.hits);
        print("Cache reads:", cache.reads);
        print("Cache writes:", cache.writes);

        print("NVM reads w/o cache:", nvm.nvm_reads_without_cache);
        print("NVM writes w/o cache:", nvm.nvm_writes_without_cache);
        print("NVM reads:", nvm.nvm_reads);
        print("NVM writes:", nvm.nvm_writes);

        print("Checkpoints:", checkpoint.checkpoints);
        print("Checkpoints due to WAR:", checkpoint.due_to_war);
        print("Checkpoints due to dirty:", checkpoint.due_to_dirty);
        print("Checkpoints due to period:", checkpoint.due_to_period);

        print("Misc: hints given:", misc.hints_given);
        cout << "\n-------------------------------------" << endl;
    }

    void logStats(ofstream &logger) {
        log("Cache misses:", cache.misses, logger);
        log("Cache hits:", cache.hits, logger);
        log("Cache reads:", cache.reads, logger);
        log("Cache writes:", cache.writes, logger);

        log("NVM reads w/o cache:", nvm.nvm_reads_without_cache, logger);
        log("NVM writes w/o cache:", nvm.nvm_writes_without_cache, logger);
        log("NVM reads:", nvm.nvm_reads, logger);
        log("NVM writes:", nvm.nvm_writes, logger);

        log("Checkpoint:", checkpoint.checkpoints, logger);
        log("Checkpoint WAR:", checkpoint.due_to_war, logger);
        log("Checkpoint dirty:", checkpoint.due_to_dirty, logger);
        log("Checkpoint period:", checkpoint.due_to_period, logger);

        log("Misc: hints given:", misc.hints_given, logger);
        logger << endl;
    }

    void update_dirty_ratio(float ratio)
    {
        if (ratio > misc.max_dirty_ratio)
            misc.max_dirty_ratio = ratio;   
    }
};

#endif /* _STATS */