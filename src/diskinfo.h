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

// Takes ownership of string data into disk info and nulls out the object
// Frees *pPath on failure
// Also prints errors
utils_ErrorState openLockReadImage(DiskInfo* pDiskInfo, String8* pPath, bool* pCreatedNewFile);
//utils_ErrorState openLockReadDisk(DiskInfo* pDiskInfo, String8* pDiskSymbol);

// typedef struct WriteImageInfo
// {
//     const uint8_t* pBuffer;
//     size_t bufferSize;
//     size_t fileWriteOffset;
// } WriteImageInfo;

// DARRAY_DEF(Dwii, dwii, WriteImageInfo)

// utils_ErrorState writeImage(DiskInfo* pDiskInfo, const WriteImageInfo* pWriteInfo);
// utils_ErrorState writeDisk(DiskInfo* pDiskInfo, const WriteImageInfo* pWriteInfo);

// Also prints errors
utils_ErrorState closeImage(DiskInfo* pDiskInfo);
//utils_ErrorState closeDisk(DiskInfo* pDiskInfo);
