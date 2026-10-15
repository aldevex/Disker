#pragma once
#include "../../mem/mem.h"

// Layout types are used to index into layout-specific functions therefore
//   one type may include multiple specific layouts handled with similar algorithms
//   (e.g. only FAT for FAT32/16/12)

// A layout type greater than or equal to extension-begin (EXTBEGIN)
//   is registered and used dynamically at runtime

// PT means partition table
typedef enum layout_Type
{
    LAYOUT_TYPE_NULL,
    LAYOUT_TYPE_UNKNOWN,

    LAYOUT_TYPE_DISKBEGIN,
    LAYOUT_TYPE_DISK_REAL_DISK,
    LAYOUT_TYPE_DISK_RAW_IMAGE,
    LAYOUT_TYPE_DISKEND,

    LAYOUT_TYPE_PTBEGIN,
    LAYOUT_TYPE_PT_MBR,
    LAYOUT_TYPE_PT_GPT,
    LAYOUT_TYPE_PTEND,

    LAYOUT_TYPE_FSBEGIN,
    LAYOUT_TYPE_FS_FAT,
    LAYOUT_TYPE_FSEND,

    // LAYOUT_TYPE_SPECIALBEGIN,
    // LAYOUT_TYPE_SPECIAL_IDK,
    // LAYOUT_TYPE_SPECIALEND,

    LAYOUT_TYPE_EXTBEGIN
} layout_Type;

typedef struct layout_Info
{
    layout_Type type;
    void* pMetadata;
} layout_Info;
DARRAY_DEF(Dlinfo, dlinfo, layout_Info)

typedef struct layout_DiskInfo
{
    String8 path;
    handle_t handle;
    // Edited any layouts in the disk info object
    bool edited;
    // Disk format (e.g. raw disk, .vhd)
    layout_Info diskLayout;
    // Disk-wide layout (e.g. MBR, FAT16 unpartitioned)
    layout_Info globalLayout;
    // Partition-wide layout (e.g. FAT32 in a partition)
    Dlinfo partLayouts;
} layout_DiskInfo;

#define LAYOUT_DISKINFO_DEFAULT (layout_DiskInfo){ .handle = HANDLE_NONE }



// Bad prototype, just take argsStartI and pErrorText from it:

// es_t layout_diskInfoInit(layout_DiskInfo* pInfo, String8* pErrorText,
//                 String8* pAdoptedPath, const String8* pCmdText, const Dview* pCmdSegs, size_t argsStartI);

// es_t layout_diskInfoFree(layout_DiskInfo* pInfo);
