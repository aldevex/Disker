#pragma once
#include "./format/format.h"
#include "./scheme/scheme.h"
#include "../utils/utils.h"
#include "./fs/fs.h"
#include "../mem/mem.h"

typedef struct DiskLayout
{
    df_Variant format;
    scheme_Data scheme;
    Dfs fileSystems;
} DiskLayout;

typedef struct DiskInfo
{
    String8 path;
    utils_SysHandle handle;

    DiskLayout og, target;
} DiskInfo;

#define DISKINFO_DEFAULT (DiskInfo){\
    .path = {0},\
    .handle = UTILS_SYSHANDLE_NONE,\
    .og = {0},\
    .target = {0}\
}

// Takes ownership of pPath data and nulls out the given object's data
// Opens and locks disk/image, and reads layout data into the object
// Also prints errors on failure
// If pCreatedNewFile == NULL or *pCreatedNewFile == false: won't create a new file on failure to open
//   otherwise: sets *pCreatedNewFile to true or false if created a new file
extern utils_ErrorState diskInfoOpenRead(String8* pPath, uint64_t rawImgSectorSize, uint64_t rawImgAlignment,
                                        DiskInfo* pDiskInfo, bool* pCreatedNewFile);
// Unlocks and closes disk/image and resets the object data to default
// Also prints errors on failure
extern utils_ErrorState diskInfoCloseReset(DiskInfo* pDiskInfo);
// Writes object disk layout and fs updates to disk/image
// Also prints errors on failure
extern utils_ErrorState diskInfoWrite(DiskInfo* pDiskInfo);
