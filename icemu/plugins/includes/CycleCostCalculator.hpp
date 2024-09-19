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

class CycleCost {
 public:
  CycleCost() {}

  ~CycleCost() = default;

  static constexpr unsigned int CACHE_READ_COST = 2;
  static constexpr unsigned int CACHE_WRITE_COST = 2;
  static constexpr unsigned int NVM_READ_COST = 6;
  static constexpr unsigned int NVM_WRITE_COST = 6;

  static unsigned int cache_read_cycles;
  static unsigned int cache_write_cycles;
  static unsigned int nvm_read_cycles;
  static unsigned int nvm_write_cycles;

  unsigned int modifyCost(RiscvE21Pipeline *Pipeline, enum CostSpecification type,
                  address_t size) {

    // Round up to nearest word access. We count the cost per word
    size = (address_t)ceil((double)size/4.0);

    unsigned int cycles = 0;

    switch (type) {
      case CACHE_READ:
        cycles = CACHE_READ_COST * size;
        cache_read_cycles += cycles;
        break;
      case CACHE_WRITE:
        cycles = CACHE_WRITE_COST * size;
        cache_write_cycles += cycles;
        break;
      case NVM_READ:
        cycles = NVM_READ_COST * size;
        nvm_read_cycles += cycles;
        break;
      case NVM_WRITE:
        cycles = NVM_WRITE_COST * size;
        nvm_write_cycles += cycles;
        break;
      default:
        assert(false);
        break;
    }

    Pipeline->addToCycles(cycles);
    return cycles;
  }
};

unsigned int CycleCost::cache_read_cycles = 0;
unsigned int CycleCost::cache_write_cycles = 0;
unsigned int CycleCost::nvm_read_cycles = 0;
unsigned int CycleCost::nvm_write_cycles = 0;