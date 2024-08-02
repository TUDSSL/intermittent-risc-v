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
#define CACHE_READ_COST 2
#define CACHE_WRITE_COST 2
#define NVM_READ_COST 8
#define NVM_WRITE_COST 8

class CycleCost {
 public:
  CycleCost() {}

  ~CycleCost() = default;

  void modifyCost(RiscvE21Pipeline *Pipeline, enum CostSpecification type,
                  address_t size) {

    // Round up to nearest word access. We count the cost per word
    size = (address_t)ceil((float)size/4.0);

    switch (type) {
      case CACHE_READ:
        Pipeline->addToCycles(CACHE_READ_COST * size);
        break;
      case CACHE_WRITE:
        Pipeline->addToCycles(CACHE_WRITE_COST * size);
        break;
      case NVM_READ:
        Pipeline->addToCycles(NVM_READ_COST * size);
        break;
      case NVM_WRITE:
        Pipeline->addToCycles(NVM_WRITE_COST * size);
        break;
    }
  }
};
