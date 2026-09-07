#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace Disk {

struct Disk
{
    std::string path = "";
    bool isRealDisk = true; // True default for safety
    
    uint64_t size = 0, sectorSize = 0, sectorCount = 0;
    uint64_t physicalSectorSize = 0;

    uint64_t alignment = 0; // In bytes

    Disk() = default;
    Disk(std::string_view path, bool isRealDisk,
        uint64_t size, uint64_t sectorSize,
        uint64_t physicalSectorSize, uint64_t alignment)

        : path(path), isRealDisk(isRealDisk),
        size(size), sectorSize(sectorSize), sectorCount(size/sectorSize),
        physicalSectorSize(physicalSectorSize), alignment(alignment)
    {}
};

enum class Scheme
{
    MBR, GPT
};

}
