#pragma once
#include <stdint.h>
#include "../../mem/mem.h"

// Raw image/actual disk geometry
typedef struct raw_Data
{
    uint64_t size, sectorSize;
    uint64_t alignment; // In bytes
} raw_Data;
