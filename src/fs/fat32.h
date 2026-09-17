#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/generic.h"
#pragma pack(push, 1)

// A Cluster is the smallest space the system can allocate for a single file (minimum = 1 sector per cluster)
// FAT32 requires minimum 65,525 clusters, if less you must use FAT16
//   Most disks are too big for that minimum limit anyway sooo it's mostly irrelevant

// Basic FAT32 partition architecture:
// 1. Reserved Region:
//   Sector 0: VBR
//   Sector 1: FSInfo
//   Sector 6: Backup VBR
//   Sector 7: Backup FSInfo
// FAT Region:
//   Sector IDK: FAT1 (File Allocation Table 1)
//   Sector IDK: FAT2 (optional backup of FAT1)
// Data Region (divided into clusters):
//   Cluster 2 (first available after 0 and 1 which aren't mapped): Root Directory
//   Other clusters: file data & subdirectories

// Every sector count or index field is relative to partition EXCEPT hiddenSectors
typedef struct fat32_VBR
{
    // Jump instruction & OEM ID
    uint8_t jmp[3];
    uint8_t OEMId[8]; // Name of tool or OS that created the partition

    // FAT32 extended BIOS Parameter Block (BPB)
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster; // Must be power of 2
    uint16_t reservedSectorCount; // Reserved sectors before FAT1 (includes VBR)
    uint8_t FATCount; // Number of FAT copies (2 for og + backup)
    uint16_t rootEntryCount; // 0 on FAT32
    uint16_t totalSectors16; // 0 because 32-bit totalSectors32 is used
    uint8_t mediaDescriptor; // Disk type (0xF8 fixed, 0xF0 removable) (USB flash drive is considered fixed)
    uint16_t sectorsPerFAT16; // 0 because 32-bit sectorsPerFAT32 is used
    uint16_t sectorsPerTrack;
    uint16_t headsCount;
    uint32_t hiddenSectors; // Hidden sectors before the ENTIRE partition
    uint32_t totalSectors32;
    uint32_t sectorsPerFAT32; // Size of one FAT table (in sectors)
    // Bits 0-3: Active FAT index (starts at 0 so 0 = FAT1) (used when mirroring is disabled)
    // Bits 4-6: Reserved
    // Bit 7: FAT mirroring flag (0 = enabled, 1 = disabled)
    //   (enabled = OS will copy updates to FAT1 tp all other FATs)
    //   (disabled = only one FAT copy is active and getting updated)
    // Bits 8-15: Reserved
    uint16_t extFlags;
    uint16_t FSVersion; // File system version (0.0)
    uint32_t rootCluster; // Start cluster of root directory
    uint16_t FSInfo; // FSInfo sector number
    uint16_t backupBootSector; // Backup VBR sector number
    uint8_t reserved[12];
    uint8_t driveNumber; // BIOS drive number (0x80 hard disk, 0x0 floppy)
    uint8_t reserved1; // Used by Windows
    uint8_t bootSignature; // Extended boot signature (0x29 = all 3 fields valid, 0x28 = fileSystemType only)

    // Extended fields
    uint32_t volumeId; // Unique volume serial number
    uint8_t volumeLabel[11]; // Volume name padded with spaces
    uint8_t fileSystemType[8]; // File system string ("FAT32   ")

    // Bootloader
    uint8_t bootCode[420]; // VBR bootloader code
    uint16_t signature; // Boot sector signature (0xAA55)
} fat32_VBR;

#pragma pack(pop)

/*
    fat32_vbrMake(// Interface details
        const uint8_t volumeLabel[11], uint32_t volumeId,
        // Segmentation details
        uint16_t bytesPerSector, uint8_t sectorsPerCluster,
        uint32_t hiddenSectors, uint16_t reservedSectorCount,
        uint16_t sectorsPerTrack, uint16_t headsCount,
        uint32_t totalSectors,
        // Internal details
        bool isFloppy, uint8_t mediaDescriptor,
        uint16_t FSInfo, uint16_t backupBootSector,
        uint8_t FATCount, uint32_t rootCluster,        
        uint8_t activeFatIndex, bool disableFatMirroring,
        // Bootloader
        const uint8_t* jmpInstruction3, const uint8_t inBootCode[420], bool bootable)
    {
    }
*/
