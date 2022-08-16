#include "Configurations.hpp"

/*
 * Command line options for pass
 */

cl::opt<bool> NoVerify(
  "dfintersect-fno-verify",
  cl::init(false),
  cl::desc("No verification of runtime methods or transformations")
);

cl::opt<bool> Debug(
  "dfintersect-debug",
  cl::init(false),
  cl::desc("Turn on debugging outputs/prints")
);

