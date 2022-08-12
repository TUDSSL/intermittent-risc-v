#ifndef _LOGGER
#define _LOGGER

#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>

// Local includes
#include "../includes/Stats.hpp"
#include "../includes/Utils.hpp"

using namespace std;

ofstream logger_cont, logger_final;

class Logger {
  private:
    string continuous_logging_filename, final_logging_filename;

  public:
    Logger() = default;
    ~Logger() = default;
    
    void init(string filename, enum CacheHashMethod hash)
    {
        continuous_logging_filename = filename + "-cont";
        final_logging_filename = filename + "-final";

        // Open the file for logging
        logger_cont.open(continuous_logging_filename.c_str(), ios::out | ios::trunc);
        assert(logger_cont.is_open());
        logger_final.open(final_logging_filename.c_str(), ios::out | ios::trunc);
        assert(logger_final.is_open());
        
        logger_cont << "checkpoints,cycle count,last checkpoint,dirty ratio,cause" << endl;
    }

    void printCheckpointStats(Stats &stats) {
        logger_cont << stats.checkpoint.checkpoints << ","
                    << stats.misc.current_cycle << ","
                    << stats.misc.current_cycle - stats.checkpoint.last_checkpoint_cycle << ","
                    << stats.misc.dirty_ratio << ","
                    << stats.checkpoint.cause << endl;
    }

    void printEndStats(Stats &stats) {
        stats.logStats(logger_final);
    }
};

#endif /* _LOGGER */