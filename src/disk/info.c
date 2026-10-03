#include <stdio.h>
#include "../utils/utils.h"
#include "scheme/mbr.h"
#include "scheme/scheme.h"
#include "utils/base.h"
#include "utils/file.h"
#include "./info.h"

utils_ErrorState diskInfoOpenRead(String8* pPath, uint64_t rawImgSectorSize, uint64_t rawImgAlignment,
                                    DiskInfo* pDiskInfo, bool* pCreatedNewFile)
{
    // Check if got uncleared disk info
    if (pDiskInfo->handle != UTILS_SYSHANDLE_NONE)
    {
        fprintf(stderr, "failed to open already opened disk \"%s\"\n",
                        string8NT(pPath));
        return UTILS_ERRORSTATE_FAILURE;
    }
    // Open and lock disk/image
    utils_SysHandle handle = utils_openFile(&vwstr(pPath), pCreatedNewFile);
    if (handle == UTILS_SYSHANDLE_NONE)
        return UTILS_ERRORSTATE_FAILURE;
    // Get size
    const uint64_t size = utils_getFileSize(&vwstr(pPath));
    if (size == UTILS_SIZESIG_NO_NUMBER)
    {
        utils_closeFile(&vwstr(pPath), &handle);
        return UTILS_ERRORSTATE_FAILURE;
    }
    // Set size
    pDiskInfo->og.format.data.raw = raw_dataMake(size, rawImgSectorSize, rawImgSectorSize, rawImgAlignment);

    // Set MBR
    if (size == 0)
    {
        pDiskInfo->og.scheme.type = SCHEME_TYPE_NULL;
    }
    else if (size < sizeof(mbr_Data))
    {
        pDiskInfo->og.scheme.type = SCHEME_TYPE_UNKNOWN;
    }
    else
    {
        mbr_Data* pMbrBuffer = &pDiskInfo->og.scheme.mbrData;
        utils_readFile(&vwstr(pPath), handle, 0, sizeof(mbr_Data), (void**)&pMbrBuffer);
        if (pMbrBuffer->signature != 0xAA55) pDiskInfo->og.scheme.type = SCHEME_TYPE_UNKNOWN;
        else pDiskInfo->og.scheme.type = SCHEME_TYPE_MBR;
    }

    // Copy original disk into target
    pDiskInfo->target = pDiskInfo->og;
    // Set handle and adopt path
    pDiskInfo->handle = handle;
    pDiskInfo->path = *pPath;
    *pPath = (String8){0};
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState diskInfoCloseReset(DiskInfo* pDiskInfo)
{
    // Close disk/image
    utils_ErrorState es = utils_closeFile(&vwstr(&pDiskInfo->path), &pDiskInfo->handle);
    if (es != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
    // Clear object
    *pDiskInfo = DISKINFO_DEFAULT;
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState diskInfoWrite(DiskInfo* pDiskInfo)
{
    const View8 path = vwstr(&pDiskInfo->path);
    // Get disk size
    const uint64_t size = utils_getFileSize(&path);
    if (size == UTILS_SIZESIG_NO_NUMBER) return UTILS_ERRORSTATE_FAILURE;

    // Write MBR
    {
        void* pBuffer = &pDiskInfo->target.scheme.mbrData;
        utils_writeFile(&path, pDiskInfo->handle, 0, sizeof(mbr_Data), pBuffer);
    }
    
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}
