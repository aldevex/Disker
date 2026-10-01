#pragma once
#include "./geo/geo.h"
#include "./scheme/scheme.h"
#include "../utils/utils.h"
#include "./fs/fs.h"
#include "../mem/mem.h"

typedef struct DiskInfo
{
    String8 path;
    utils_SysHandle handle;

    geo_Variant geometry;
    scheme_Data scheme;
    Dfs fileSystems;
} DiskInfo;

#define DISKINFO_DEFAULT (DiskInfo){\
    .path = {0},\
    .handle = UTILS_SYSHANDLE_NONE,\
    .geometry = {0},\
    .scheme = {0},\
    .fileSystems = {0}\
}

// Takes ownership of pPath data and nulls out the given object's data
// Opens and locks disk/image, and reads layout data into the object
// Also prints errors on failure
// If pCreatedNewFile == NULL or *pCreatedNewFile == false: won't create a new file on failure to open
//   otherwise: sets *pCreatedNewFile to true or false if created a new file
extern utils_ErrorState diskInfoOpenRead(String8* pPath, DiskInfo* pDiskInfo, bool* pCreatedNewFile);
// Unlocks and closes disk/image and clears out the object data
// Also prints errors on failure
extern utils_ErrorState diskInfoCloseClear(DiskInfo* pDiskInfo);
// Writes object disk layout and fs updates to disk/image
// Also prints errors on failure
extern utils_ErrorState diskInfoWrite(DiskInfo* pDiskInfo);
