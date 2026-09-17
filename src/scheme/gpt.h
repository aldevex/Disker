#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../utils/generic.h"
#pragma pack(push, 1)

// Basic GPT disk architecture:
// Sector 0: Protective MBR (contains 1 specifica partiton entry)
// Sector 1: Primary GPT Header
// Sector 2-33: GPT Partition Entry Array
// Sector 34-(N-34): Usable LBA space:
//   Sector 34-2047: Alignment Gap (for performance)
//   Sector 2048+: Data
// Sector (N-33)-(N-2): Backup GPT Partition Entry Array
// Sector (N-1): Backup GPT Header

// Partition type GUIDs
#define GPT_PTGUID_ESP (utils_msguidMakeCopyNT("C12A7328-F81F-11D2-BA4B-00A0C93EC93B"))
#define GPT_PTGUID_FAT32 (utils_msguidMakeCopyNT("E7C63C1E-8EDF-11D2-A2C9-00A0C93EC93B"))

// Partition Entry Attribute Flags
typedef uint64_t gpt_PEAFlags;
// Critical system-required partition
#define GPT_PEAFLAG_PLATFORM_REQUIRED ((gpt_PEAFlags)(1ULL << 0))
// EFI ignoes partition for bootloader search
#define GPT_PEAFLAG_EFI_IGNORE ((gpt_PEAFlags)(1ULL << 1))
// Legacy BIOS bootable (for partition with BIOS stage 2 code)
#define GPT_PEAFLAG_ACTIVE ((gpt_PEAFlags)(1ULL << 2))

typedef struct gpt_PartitionEntry
{
    utils_MSGUID partitionTypeGUID;
    utils_MSGUID uniquePartitionGUID;
    uint64_t firstLBA;
    uint64_t lastLBA;
    uint64_t attributeFlags;
    uint16_t partitionName16[36]; // Null-terminated, spec allows 36 characters without null but that's dangerous
} gpt_PartitionEntry;
/*
    GPTPartitionEntry() = default;
    GPTPartitionEntry(const char16_t* partitionName_UTF16_max35, utils_MSGUID partitionGUID, utils_MSGUID partitionTypeGUID,
                    uint64_t firstLBA, uint64_t lastLBA, uint64_t attributeFlags)
    {
        this->partitionTypeGUID = partitionTypeGUID;
        this->uniquePartitionGUID = partitionGUID;
        this->firstLBA = firstLBA;
        this->lastLBA = lastLBA;
        this->attributeFlags = attributeFlags;
        if (partitionName_UTF16_max35 != nullptr)
            for (size_t i = 0; i < 36; i++)
            {
                if (i == 35 || partitionName_UTF16_max35[i] == '\0')
                {
                    for (; i < 36; i++) this->partitionName16[i] = u'\0';
                    break;
                }
                else this->partitionName16[i] = partitionName_UTF16_max35[i];
            }
    }
*/

typedef struct gpt_Header
{
    uint64_t signature; // Must be set to "EFI PART"
    uint32_t revision; // GPT version 1.0
    uint32_t headerSize; // 92 bytes
    uint32_t headerCRC32; // Must be set to CRC32 checksum (HAS TO BE 0 INITIALLY)
    uint32_t reserved;
    uint64_t myLBA; // Must be set to header LBA
    uint64_t alternateLBA; // Must be set to header opposite LBA (backup for og, og for backup)
    uint64_t firstUsableLBA; // Must be set to first LBA usable by partitions
    uint64_t lastUsableLBA; // Must be set to (Total -34)
    utils_MSGUID diskGUID; // Must be set to a random GUID (must be identical in backup header)
    uint64_t partitionEntryLBA; // Must be set to parition entry array start
    uint32_t numberOfPartitionEntries; // Commonly 128 but can be anything
    uint32_t sizeOfPartitionEntry; // Can actually be >128 but that's for niche custom usage
    uint32_t partitionEntryArrayCRC32; // Must be set to partiton array CRC32 checksum
} gpt_Header;

/*
func has to set:
    uint64_t signature; // Must be set to "EFI PART"
    uint32_t reserved = 0;
    uint32_t revision = 0x10000; // GPT version 1.0
    uint32_t headerSize = 92;
    uint32_t sizeOfPartitionEntry = 128; // Can actually be >128 but that's for niche custom usage

    GPTHeader() = default;
    GPTHeader(utils_MSGUID diskGUID, bool backupHeader, std::span<const PartitionEntry> sPartitionEntries,
            uint64_t sectorSize, uint64_t totalSectors, uint64_t firstUsableSectorLBA)
    {
        if (sPartitionEntries.data() == nullptr || sPartitionEntries.size() == 0)
        {
            std::cerr << "null/empty sPartitionEntries given to GPT Header constructor\n";
            exit(EXIT_FAILURE);
        }
        // Partition Entry Array Sector Count
        uint64_t PEArraySectorCount = (sPartitionEntries.size() *128) / sectorSize;
        memcpy(&signature, "EFI PART", 8);

        myLBA = (!backupHeader)? 1  : totalSectors -1;
        alternateLBA = (!backupHeader)? totalSectors -1  : 1;

        firstUsableLBA = firstUsableSectorLBA;
        lastUsableLBA = totalSectors -1 -PEArraySectorCount -1; // -header -array -1

        this->diskGUID = diskGUID;
        partitionEntryLBA = (!backupHeader)? 2  : totalSectors -1 -PEArraySectorCount; // -header -array
        numberOfPartitionEntries = sPartitionEntries.size();

        partitionEntryArrayCRC32 = Utils::crc32(sPartitionEntries.data(),
                                                sPartitionEntries.size() *128);
        headerCRC32 = 0;
        headerCRC32 = Utils::crc32(this, 92);
    }
*/

DARRAY_DEF(Dgpt, dgpt, gpt_PartitionEntry)

typedef struct gpt_Data
{
    gpt_Header header;
    Dgpt partitionEntries;
} gpt_Data;

#pragma pack(pop)    
