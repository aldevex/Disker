#pragma once
#include "./geo/geo.h"
#include "./scheme/scheme.h"
#include "./utils/generic.h"
#include "./fs/fs.h"
#include "./mem/mem.h"

typedef struct DiskInfo
{
    String8 path;
    utils_SysHandle handle;

    geo_Variant geometry;
    scheme_Data scheme;
    Dfs fileSystems;
} DiskInfo;

static inline DiskInfo diskInfoMakeDefault()
{
    return (DiskInfo){
        .path = {0},
        .handle = UTILS_HANDLE_NONE,
        // .geometry = {0},
        // .scheme = {0},
        .fileSystems = {0}
    };
}

// Takes ownership of string data into disk info and nulls out the object
// Doesn't take *pPath on failure
// Also prints errors
utils_ErrorState openLockReadDisk(DiskInfo* pDiskInfo, String8* pPath, bool* pCreatedNewFile);

// typedef struct WriteImageInfo
// {
//     const uint8_t* pBuffer;
//     size_t bufferSize;
//     size_t fileWriteOffset;
// } WriteImageInfo;

// DARRAY_DEF(Dwii, dwii, WriteImageInfo)

// utils_ErrorState writeDisk(DiskInfo* pDiskInfo, const WriteImageInfo* pWriteInfo);

// Also prints errors
//utils_ErrorState closeDisk(DiskInfo* pDiskInfo);
