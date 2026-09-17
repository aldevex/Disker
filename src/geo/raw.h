#pragma once
#include <stdint.h>
#include "../mem/mem.h"

// Raw image/actual disk geometry
typedef struct raw_Data
{
    uint64_t size, sectorSize, sectorCount;
    uint64_t physicalSectorSize;

    uint64_t alignment; // In bytes
} raw_Data;

static inline raw_Data raw_dataMakeNull()
{
    return (raw_Data){
        .size = 0,
        .sectorSize = 0,
        .sectorCount = 0,
        .physicalSectorSize = 0,
        .alignment = 0
    };
}

static inline raw_Data raw_dataMake(uint64_t size, uint64_t sectorSize, uint64_t physicalSectorSize, uint64_t alignment)
{
    return (raw_Data){
        .size = size,
        .sectorSize = sectorSize,
        .sectorCount = size / sectorSize,
        .physicalSectorSize = physicalSectorSize,
        .alignment = alignment
    };
}
