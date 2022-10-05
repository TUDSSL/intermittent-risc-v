#pragma once

#include "icemu/emu/Emulator.h"
#include <math.h>

class CacheHints {
 private:
  icemu::Emulator &_emu;

  // cache hint function signature
  // void __cache(void *addr);
  std::string cache_hint_function_name = "__cache_hint";
  address_t cache_hint_function_address = 0;

 public:
  icemu::Emulator &getEmulator() { return _emu; }

  CacheHints(icemu::Emulator &emu) : _emu(emu) {
    try {
      cache_hint_function_address = getEmulator()
                                        .getMemory()
                                        .getSymbols()
                                        .get(cache_hint_function_name)
                                        ->address;
      cache_hint_function_address =
          emu.getArchitecture().getFunctionAddress(cache_hint_function_address);
    } catch (...) {
      std::cerr << "Failed to register function hook for: "
                << cache_hint_function_name << " (UNKNOWN ADDRESS)"
                << std::endl;
      cache_hint_function_address = 0;
      return;
    }

    std::cout << "[CacheHints] hint function address: 0x" << std::hex
              << cache_hint_function_address << std::dec << std::endl;
  }

  bool run(uint64_t instruction_address, uint64_t *memory_address) {
    if (cache_hint_function_address == 0)
      return false;

    // Check if this instruction is a hint call
    // If it's a hint, set memory_address to the hint address, return true
    // The caller can decide what to do with this information (probably skip the
    // call)
    if (instruction_address != cache_hint_function_address)
      return false;

    auto &arch = getEmulator().getArchitecture();
    address_t hintaddr =
        arch.functionGetArgument(0); // First argument is the address

    std::cout << "[CacheHints] found hint at address 0x" << std::hex
              << instruction_address << " for memory address: 0x" << hintaddr
              << std::dec << std::endl;

    // Set the memory address and return
    *memory_address = hintaddr;
    return true;
  }
};
