#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../utils/generic.h"
#pragma pack(push, 1)

typedef enum mbr_BootIndicator
{
    MBR_BOOTINDICATOR_INACTIVE = 0x0,
    MBR_BOOTINDICATOR_ACTIVE = 0x80
} mbr_BootIndicator;

typedef enum mbr_PartitionType
{
    MBR_PARTITIONTYPE_NONE = 0x00, // Unused / Empty

    MBR_PARTITIONTYPE_FAT32_LBA = 0x0C, // FAT32 (LBA addressing)
    
    MBR_PARTITIONTYPE_GPT_PROTECTIVE_MBR = 0xEE, // GPT Protective MBR
    MBR_PARTITIONTYPE_ESP = 0xEF, // EFI System Partition (ESP)
} mbr_PartitionType;

typedef struct mbr_PartitionEntry
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

} mbr_PartitionEntry;
/*
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
*/
typedef struct mbr_Data
{
    uint8_t code[446];
    mbr_PartitionEntry partitionTable[4];
    uint16_t signature; // 0xAA55
} mbr_Data;
/*
    MBRData() = default;
    MBRData(const uint8_t* pCode446, const std::vector<PartitionEntry>& partitionEntries4max)
    {
        if (pCode446 != nullptr) memcpy(code, pCode446, 446);
        
        if (partitionEntries4max.size() > 4)
        {
            std::cerr << "partitionEntries4max.size() > 4 given to MBRData constructor\n";
            exit(EXIT_FAILURE);
        }
        else for (size_t i = 0; i < partitionEntries4max.size(); i++)
            partitionTable[i] = partitionEntries4max[i];
    }
    MBRData(const uint8_t* pMBR512Bytes)
    {
        if (pMBR512Bytes == nullptr)
        {
            std::cerr << "pMBR512Bytes == nullptr given to MBRData constructor\n";
            exit(EXIT_FAILURE);
        }
        else memcpy(this, pMBR512Bytes, 512);
    }
*/
#pragma pack(pop)
