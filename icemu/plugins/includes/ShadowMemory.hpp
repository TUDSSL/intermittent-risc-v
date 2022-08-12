#ifndef _SHADOW_MEM
#define _SHADOW_MEM

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

using namespace std;
using namespace icemu;

static const bool PRINT_MEMORY_DIFF = true;

class ShadowMemory{
  private:
    unordered_set<address_t> reads, writes;
    memseg_t *MainMemSegment = nullptr;
    uint8_t *ShadowMem = nullptr;
    icemu::Memory *mem;

  public:
    ShadowMemory() = default;
    ~ShadowMemory() {
        delete[] ShadowMem;
    }

    void initMem(icemu::Memory *mem)
    {
        this->mem = mem;
        auto code_entrypoint = mem->entrypoint;

        // Get the memory segment holding the main code (assume it also holds the RAM)
        MainMemSegment = mem->find(code_entrypoint);
        assert(MainMemSegment != nullptr);

        // Create shadow memory
        ShadowMem = new uint8_t[MainMemSegment->length];
        assert(ShadowMem != nullptr);

        // Populate the shadow memory
        memcpy(ShadowMem, MainMemSegment->data, MainMemSegment->length);
    }

    bool compareMemory()
    {
        // First do a fast memcmp (assume it's optimized)
        int compareValue = memcmp(ShadowMem, MainMemSegment->data, MainMemSegment->length);
        if (compareValue == 0) {
            // Memory is the same
            return true;
        }

        cout << "\tCompare value: " << (int)compareValue << endl;

        // Something is different according to `memcmp`, check byte-per-byte
        bool same = true;
        for (size_t i = 0; i < MainMemSegment->length; i++) {
            if (ShadowMem[i] != MainMemSegment->data[i]) {
                // Memory is different
                same = false;

                if (PRINT_MEMORY_DIFF) {
                    address_t addr = MainMemSegment->origin + i;
                    address_t emu_val = MainMemSegment->data[i];
                    address_t shadow_val = ShadowMem[i];
                    cerr << "[mem] memory location at 0x" << hex << addr
                        << " differ - Emulator: 0x" << emu_val << " Shadow: 0x"
                        << shadow_val << dec << endl;
                }
            }
        }

        return same;
    }

    bool compareMemory(bool assert)
    {
        // First do a fast memcmp (assume it's optimized)
        int compareValue = memcmp(ShadowMem, MainMemSegment->data, MainMemSegment->length);
        if (compareValue == 0) {
            // Memory is the same
            return true;
        }

        cout << "\tCompare value: " << (int)compareValue << endl;

        // Something is different according to `memcmp`, check byte-per-byte
        bool same = true;
        for (size_t i = 0; i < MainMemSegment->length; i++) {
            if (ShadowMem[i] != MainMemSegment->data[i]) {
                // Memory is different
                same = false;

                if (PRINT_MEMORY_DIFF) {
                    address_t addr = MainMemSegment->origin + i;
                    address_t emu_val = MainMemSegment->data[i];
                    address_t shadow_val = ShadowMem[i];
                    cerr << "[mem] memory location at 0x" << hex << addr
                        << " differ - Emulator: 0x" << emu_val << " Shadow: 0x"
                        << shadow_val << dec << endl;
                }
            }
        }

        if (assert)
            assert(compareValue == 0);
        
        return same;
    }

    void shadowWrite(address_t address, address_t value, address_t size) {
        address_t address_idx = address - MainMemSegment->origin;
        for (address_t i=0; i<size; i++) {
            uint64_t byte = (value >>(8*i)) & 0xFF; // Get the bytes
            ShadowMem[address_idx+i] = byte;
        }

        // cout << "[shadow write] wrote " << (address_t)ShadowMem[address_idx] << " to mem: " << hex << address_idx + MainMemSegment->origin << dec << endl;
    }

};

#endif /* _SHADOW_MEM */