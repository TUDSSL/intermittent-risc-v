#ifndef _UTILS
#define _UTILS

#include <string>
#include <chrono>

using namespace std;

static const bool MIMIC_CHECKPOINT_TAKEN = true;
static const float DIRTY_RATIO_THRESHOLD = 0.6;
static const bool NVM_STATS_PER_BYTE = true;

static const uint64_t FREQ = 50 * 1000 * 1000;
static const double TIME_FOR_CHECKPOINT_THRESHOLD = 50 / 1000;
static const uint64_t CYCLE_COUNT_CHECKPOINT_THRESHOLD = TIME_FOR_CHECKPOINT_THRESHOLD * FREQ;



static inline uint64_t CurrentTime_nanoseconds()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>
         (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

enum replacement_policy {
  LRU,
  MRU
};

enum CheckpointReason {
  CHECKPOINT_DUE_TO_WAR,
  CHECKPOINT_DUE_TO_DIRTY,
  CHECKPOINT_DUE_TO_PERIOD
};

#endif /* _UTILS */