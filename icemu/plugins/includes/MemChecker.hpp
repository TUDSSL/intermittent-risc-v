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

class MemChecker {
  public:
    unordered_set<address_t> writes;
    MemChecker() = default;
    ~MemChecker() = default;
};