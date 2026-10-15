#pragma once
#include "../../mem/mem.h"

// Layout types are used to index into layout-specific functions therefore
//   one type may include multiple specific layouts handled with similar algorithms
//   (e.g. only FAT for FAT32/16/12)

// A layout type greater than or equal to extension-begin (EXTBEGIN)
//   is registered and used dynamically at runtime

// Disk format (e.g. raw .img file, .vhd file, a real disk)
typedef enum layout_DiskType
{
    LAYOUT_DISKTYPE_NULL,
    LAYOUT_DISKTYPE_UNKNOWN,

    LAYOUT_DISKTYPE_REAL_DISK,
    LAYOUT_DISKTYPE_RAW_IMAGE,
} layout_DiskType;

// In-volume layout (e.g. MBR, GPT, FAT)
typedef enum layout_VolType
{
    LAYOUT_VOLTYPE_NULL,
    LAYOUT_VOLTYPE_UNKNOWN,

    LAYOUT_VOLTYPE_PTBEGIN,
    LAYOUT_VOLTYPE_PT_MBR,
    LAYOUT_VOLTYPE_PT_GPT,
    LAYOUT_VOLTYPE_PTEND,

    LAYOUT_VOLTYPE_FSBEGIN,
    LAYOUT_VOLTYPE_FS_FAT,
    LAYOUT_VOLTYPE_FSEND,

    LAYOUT_VOLTYPE_EXTBEGIN
} layout_VolType;

typedef struct layout_VolInfo
{
    layout_VolType type;
    void* pMetadata;
} layout_VolInfo;
DARRAY_DEF(Dlvinfo, dlvinfo, layout_VolInfo)

typedef struct layout_DiskInfo
{
    String8 path;
    handle_t handle;
    // Edited any layouts in the disk info object
    bool edited;
    // Disk format layout (e.g. raw disk, .vhd)
    struct {
        layout_DiskType type;
        void* pMetadata;
    } diskLayout;
    // Disk-wide layout (e.g. MBR, FAT16 unpartitioned)
    layout_VolInfo globalLayout;
    // Partition-wide layout (e.g. FAT32 in a partition)
    Dlvinfo partLayouts;
} layout_DiskInfo;

#define LAYOUT_DISKINFO_DEFAULT (layout_DiskInfo){ .handle = HANDLE_NONE }
