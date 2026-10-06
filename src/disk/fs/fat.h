#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../utils/utils.h"
#pragma pack(push, 1)

// Basic FAT partition architecture:
// 1. Reserved region:
//   Sector 0: VBR
//   Sector 1: FSInfo
//   Sector 6: Backup VBR (points to primary FSInfo)
//   Sector 7: Backup FSInfo (updated only at format time because our Microsoft overlords say so)
// FAT region:
//   Sector IDK: FAT1 (File Allocation Table 1)
//   Sector IDK: FAT2 (optional backup of FAT1)
// Data region (divided into clusters):
//   Cluster #2 (first available after 0 and 1 which aren't mapped): Root Directory
//   Other clusters: file data & subdirectories

// A FAT12 volume can't contain more than 4,084 clusters
// A FAT16 volume can't contain less than 4,085 clusters or more than 65,524 clusters
// A FAT32 volume can't contain less than 65,524 clusters
// And the correct way to know which FS is being used is by cluster count ☠️

// All FAT fields that are counts or indices are relative to the start of the partition
//   EXCEPT hidden sector count is obviously the amount of sectors before the partition

#define _fat_SHARED_BPB_START_FIELDS\
    /* FAT BIOS Parameter Block (BPB) */\
    /* x86 jump instruction */\
    uint8_t jmp[3];\
    /* Name of tool or OS that created the partition */\
    uint8_t OEMName[8];\
    \
    /* Must be n = power of 2 where (512 =< n <= 4096) */\
    uint16_t bytesPerSector;\
    /* Must be n = power of 2 where (1 =< n <= 128) */\
    uint8_t sectorsPerCluster;\
    /* Reserved region sector count (relative to partition start) (n > 0) */\
    /* Used for data area (cluster #2) cluster alignment */\
    uint16_t reservedSectorCount;\
    \
    /* Number of FAT copies (commonly set to 2 for og + backup) */\
    uint8_t FATCount;\
    /* FAT12/FAT16 directory entry count in root directory */\
    /* In FAT16: recommended 512 */\
    /* In FAT12: recommended depends on the disk size */\
    /* In FAT32: always 0 */\
    uint16_t rootEntryCount;\
    /* FAT12/FAT16 sector count (includes all 4 FAT regions) */\
    /* In FAT12/FAT16: if count > 1MiB must be 0 and totalSectors32 used instead */\
    /* In FAT32: always 0 */\
    uint16_t totalSectors16;\
    /* Disk type (0xF8 fixed, 0xF0 removable (the entire disk is FAT, no MBR/GPT partition table)) */\
    /* (values 0xF9-0xFF are allowed but uncommon and may be replaced with 0xF0) */\
    uint8_t mediaDescriptor;\
    /* FAT12/FAT16 FAT size in sectors */\
    /* In FAT32: always 0 */\
    uint16_t sectorsPerFAT16;\
    /* = */\
    uint16_t sectorsPerTrack;\
    /* = */\
    uint16_t headsCount;\
    /* Count of sectors before the partition */\
    uint32_t hiddenSectors;\
    /* 32-bit sector count (includes all 4 FAT regions) */\
    /* In FAT12/FAT16: used if count > 1MiB (must be n > 0) and totalSectors16 is 0 */\
    /* In FAT32: always n > 0 */\
    uint32_t totalSectors32;
    // Everything after this differs between FAT12/FAT16 and FAT32
    //   (except some values before the ending are the same, only offsets differ)

#define _fat_SHARED_BPB_MIDDLE_FIELDS\
        /* BIOS int 0x13 drive number (0x80 booted-from fixed disk, 0x0 booted-from removable disk) */\
        /* (fixed and removable disks explained in previous comments) */\
        uint8_t driveNumber;\
        /* = */\
        uint8_t reserved1;\
        \
        /* Extended boot signature (0x29 = one of the following 2 fields aren't 0 and the following 3 exist) */\
        uint8_t extSignature;\
        /* Volume serial number ID (this and label are for volume tracking on removable (non-partitioned) disks) */\
        /* Equals current date and time combined into one 32-bit value */\
        uint32_t volumeID;\
        /* Volume label (must match label in root directory) */\
        /* Default value must be "NO NAME    " */\
        uint8_t volumeLabel[11];\
        /* Useless FS type string that may not be used to detect FAT filesystem type 💀 */\
        /*   but you still have to write it */\
        /* In FAT12/FAT16: Set to "FAT12   " or "FAT16   " or "FAT     " */\
        /* In FAT32: Set to "FAT32   " */\
        uint8_t typeName[8];

typedef struct fat_SharedBPBStart
{
    _fat_SHARED_BPB_START_FIELDS
} fat_SharedBPBStart;

typedef struct fat_SharedBPBMiddle
{
    _fat_SHARED_BPB_MIDDLE_FIELDS
} fat_SharedBPBMiddle;



typedef struct fat1216_VBR
{
    // Shared start fields
    _fat_SHARED_BPB_START_FIELDS
    // Shared middle fields
    _fat_SHARED_BPB_MIDDLE_FIELDS
    // Bootloader
    uint8_t bootloader[448];
    uint16_t signature; // 0xAA55
} fat1216_VBR;

typedef struct fat32_VBR
{
    // Shared start fields
    _fat_SHARED_BPB_START_FIELDS

    // FAT32 FAT size in sectors
    uint32_t sectorsPerFAT32;
    // Extended flags
    // Bits 0-3: Active FAT index (zero based so 0 = FAT1)
    //   (only valid when mirroring is disabled)
    // Bits 4-6: Reserved
    // Bit 7: FAT mirroring flag (0 = enabled, 1 = disabled)
    //   (enabled = system mirrors updates to all FATs)
    //   (disabled = only one FAT copy is active (the one in the active FAT index))
    // Bits 8-15: Reserved
    uint16_t extFlags;
    // FAT32 fs version (high byte major, low byte minor) (always 0)
    uint16_t FSVersion;

    // Start cluster number of root directory
    // Must be 2 or the first usable cluster after it
    uint32_t rootCluster;
    // The ***PRIMARY*** FSInfo sector number relative to partition (de facto standard at 1)
    uint16_t fsInfo;
    // Backup VBR sector number (0 if no backup, 6 pointing at the backup VBR and FSInfo)
    uint16_t backupBootSector;
    // =
    uint8_t reserved[12];

    // Shared middle fields
    _fat_SHARED_BPB_MIDDLE_FIELDS

    // Bootloader
    uint8_t bootloader[420];
    uint16_t signature; // 0xAA55
} fat32_VBR;

#pragma pack(pop)

typedef enum fat_Type
{
    FAT_TYPE_NULL, FAT_TYPE_INVALID,
    FAT_TYPE_FAT12, FAT_TYPE_FAT16, FAT_TYPE_FAT32
} fat_Type;

typedef struct fat_Variant
{
    fat_Type type;
    union {
        fat1216_VBR fat1216VBR;
        fat32_VBR fat32VBR;
    } data;
} fat_Variant;

// extern fat_Type fat_guessTypeBecauseMicrosoftSucks(void* pVBR, size_t sVBR);
