#pragma once

#include "icemu/emu/Emulator.h"

#include "PluginArgumentParsing.h"

#include "../includes/Checkpoint.hpp"
#include "Stats.hpp"

using _Stats = ReplayCacheStats::Stats;

template <class _Pipeline>
class PowerFailureGenerator {

  bool first_power_failure = true;

  // Periodic power failure emulation
  uint64_t on_duration = 0;
  uint64_t reset_cycle_target = 0;
  uint64_t last_reset_checkpoint_count = 0;

  // Explicit power failure support
  bool fail_next = false;

 public:
  PowerFailureGenerator(icemu::Emulator &emu, _Stats &stats) {
    // Get the on duration from the arguments
    auto arg_on_duration =
        PluginArgumentParsing::GetArguments(emu, "on-duration=");
    if (arg_on_duration.size())
      on_duration = std::stoul(arg_on_duration[0]);

    // Configure the next reset_cycle_target
    reset_cycle_target += on_duration;

    stats.updateOnDuration(on_duration);
  }

  auto getOnDuration() const {
    return on_duration;
  }

  bool shouldReset(_Pipeline &pipeline, Checkpoint &checkpoint) {
    if (fail_next ||
        (reset_cycle_target > 0 && pipeline.getTotalCycles() >= reset_cycle_target)) {
      fail_next = false;

      // Check if there were checkpoints since the last reset (to detect the
      // lack of forward progress)
      if (!first_power_failure && last_reset_checkpoint_count == checkpoint.count) {
        cout << "NO FORWARD PROGRESS" << endl;
        assert(false && "No forward progress is made, abort execution");
      }
      last_reset_checkpoint_count = checkpoint.count;
      first_power_failure = false;

      // Set the next reset target
      reset_cycle_target += on_duration;

      // Indicate that a reset should be performed
      return true;
    }

    return false;
  }

  void failNext() {
    fail_next = true;
  }
};
