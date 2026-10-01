#include <stdio.h>
#include "../utils/utils.h"
#include "scheme/mbr.h"
#include "scheme/scheme.h"
#include "utils/base.h"
#include "utils/file.h"
#include "./info.h"

utils_ErrorState diskInfoOpenRead(String8* pPath, DiskInfo* pDiskInfo, bool* pCreatedNewFile)
{
    // Check if previous data isn't cleared
    if (pDiskInfo->handle != UTILS_SYSHANDLE_NONE)
    {
        fprintf(stderr, "written disk info object given to %s (path = %s)\n",
                        __func__, string8NT(pPath));
        exit(EXIT_FAILURE);
    }
    // Open and lock disk/image
    utils_SysHandle handle = utils_openFile(&vwstr(pPath), pCreatedNewFile);
    if (handle == UTILS_SYSHANDLE_NONE)
        return UTILS_ERRORSTATE_FAILURE;
    // Get size
    const uint64_t size = utils_getFileSize(&vwstr(pPath));
    if (size == UTILS_SIZESIG_NO_NUMBER)
        return UTILS_ERRORSTATE_FAILURE;
    // Read MBR
    if (size < sizeof(mbr_Data))
    {
        pDiskInfo->scheme.type = SCHEME_TYPE_UNKNOWN;
    }
    else
    {
        void* pBuffer = &pDiskInfo->scheme.mbrData;
        utils_readFile(&vwstr(pPath), handle, 0, sizeof(mbr_Data), &pBuffer);
        pDiskInfo->scheme.type = SCHEME_TYPE_MBR;
    }
    // Adopt path
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
    // Get size
    const uint64_t size = utils_getFileSize(&path);
    if (size == UTILS_SIZESIG_NO_NUMBER)
        return UTILS_ERRORSTATE_FAILURE;
    // Write MBR
    if (size < sizeof(mbr_Data))
        return UTILS_ERRORSTATE_SUCCESS;
    else
    {
        void* pBuffer = &pDiskInfo->scheme.mbrData;
        utils_writeFile(&path, pDiskInfo->handle, 0, sizeof(mbr_Data), pBuffer);
    }
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}
