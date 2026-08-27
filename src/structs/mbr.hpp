#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include "../algos/utils.hpp"
// MBR (Namespace)
namespace MBRns {
#pragma pack(push, 1)

enum class BootIndicator : uint8_t
{
    Inactive = 0x0,
    Active = 0x80
};

enum class PartitionType : uint8_t
{
    None = 0x00, // Unused / Empty

    FAT32LBA = 0x0C, // FAT32 (LBA addressing)
    
    GPTProtectiveMBR = 0xEE, // GPT Protective MBR
    EFISystemPartition = 0xEF, // EFI System Partition (ESP)
};

struct PartitionEntry
{
    uint8_t bootIndicator;
    // CHS order:
    // CHS[0]: Head (8 bits)
    // CHS[1]: Sector (6 bits 0-5), Cylinder high (2 bits 6-7)
    // CHS[2]: Cylinder low (8 bits)
    uint8_t startingCHS[3];
    uint8_t partitionType;
    uint8_t endingCHS[3];
    uint32_t startingLBA;
    uint32_t sectorCountLBA;

    PartitionEntry() = default;
    PartitionEntry(PartitionType type, bool bootable,
                    uint32_t startingLBA, uint32_t sectorCountLBA,
                    uint8_t startingSector, uint8_t startingHead, uint16_t startingCylinder,
                    uint8_t endingSector, uint8_t endingHead, uint16_t endingCylinder)
    {
        bootIndicator = (bootable)? (uint8_t)BootIndicator::Active : (uint8_t)BootIndicator::Inactive;

        startingCHS[0] = startingHead;
        startingCHS[1] = (startingSector & 0b111111) | (uint8_t(startingCylinder >> 2) & 0b11000000);
        startingCHS[2] = (startingCylinder & 0xFF);

        partitionType = (uint8_t)type;

        endingCHS[0] = endingHead;
        endingCHS[1] = (endingSector & 0b111111) | (uint8_t(endingCylinder >> 2) & 0b11000000);
        endingCHS[2] = (endingCylinder & 0xFF);
        
        this->startingLBA = startingLBA;
        this->sectorCountLBA = sectorCountLBA;
    }
};

struct MBRData
{
    uint8_t code[446] = {};
    PartitionEntry partitionTable[4] = {};
    uint16_t signature = 0xAA55;

    MBRData(const uint8_t* pCode446, const PartitionEntry* pPartitionEntries4)
    {
        if (pCode446 != nullptr) memcpy(code, pCode446, 446);
        
        if (pPartitionEntries4 == nullptr)
        {
            std::cerr << "pPartitionEntries4 = nullptr given to MBRData constructor\n";
            exit(EXIT_FAILURE);
        }
        else memcpy(partitionTable, pPartitionEntries4, sizeof(PartitionEntry) *4);
    }
};

#pragma pack(pop)
}
