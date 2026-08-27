#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../algos/utils.hpp"
namespace Generic {

enum class Format
{
    MBR, GPT
};

struct Disk
{
    std::string path = "";
    bool isRealDisk = true; // True default for safety
    
    uint64_t size = 0, sectorSize = 0, sectorCount = 0;
    uint64_t physicalSectorSize = 0;

    uint16_t sectorsPerTrack = 0;
    uint16_t headCount = 0; // headCount = heads per cylinder too
    uint32_t cylinderCount = 0;

    Format format = Format::MBR;
    uint64_t alignment = 0; // In bytes

    Disk(std::string_view path, bool isRealDisk,
        uint64_t size, uint64_t sectorSize, uint64_t sectorCount,
        uint64_t physicalSectorSize,
        uint16_t sectorsPerTrack, uint16_t headCount, uint32_t cylinderCount,
        Format format, uint64_t alignment)

        : path(path), isRealDisk(isRealDisk),
        size(size), sectorSize(sectorSize), sectorCount(sectorCount),
        physicalSectorSize(physicalSectorSize),
        sectorsPerTrack(sectorsPerTrack), headCount(headCount), cylinderCount(cylinderCount),
        format(format), alignment(alignment)
    {}
};

enum class PartitionType : uint64_t
{
    Unavailable, // Type not available here (format-specific or just invalid)
    FAT32, ESP
};

// CHS Address cut into segments
struct CutCHS
{
    uint16_t cylinder = 0;
    uint8_t head = 0;
    uint8_t sector = 0;
};

struct PartitionInfo
{
    PartitionType type;
    std::string label;
    uint64_t uniqueId[2] = {}; // ID or Serial idk

    uint64_t size = 0, sectorCount = 0;
    uint64_t startingSector = 0, endingSector = 0;
    CutCHS startingCHS = {}, endingCHS = {};
};

}
