#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <math.h>
#include <bitset>
#include <string.h>
#include <chrono>
#include <queue>
#include <list>
#include <set>
#include <map>
#include <unordered_set>

#include "icemu/emu/Emulator.h"
#include "icemu/emu/Memory.h"

#include "../includes/Utils.hpp"

using namespace std;
using namespace icemu;

struct hashFunction
{
  size_t operator()(const tuple<address_t, address_t> &x) const
  {
    return get<0>(x) ^ get<1>(x);
  }
};

class MemChecker {
  public:
    unordered_set<address_t> writes;
    MemChecker() = default;
    ~MemChecker() = default;
};