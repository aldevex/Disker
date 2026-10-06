#include <stdio.h>
#include "../utils/utils.h"
#include "scheme/mbr.h"
#include "scheme/scheme.h"
#include "utils/base.h"
#include "utils/file.h"
#include "./info.h"

utils_ErrorState diskInfoOpenRead(String8* pPath, uint64_t rawImgSectorSize,
                                    DiskInfo* pDiskInfo, bool* pCreatedNewFile)
{
    // Caller must reset *pDiskInfo beforehand
    if (pDiskInfo->handle != UTILS_SYSHANDLE_NONE)
    {
        fprintf(stderr, "pDiskInfo->handle == %llu != UTILS_SYSHANDLE_NONE given to %s\n",
                        (uint64_t)pDiskInfo->handle, __func__);
        exit(EXIT_FAILURE);
    }
    // Open and lock disk/image
    utils_SysHandle handle = utils_openFile(&vwstr(pPath), pCreatedNewFile);
    if (handle == UTILS_SYSHANDLE_NONE)
        return UTILS_ERRORSTATE_FAILURE;
    // Get size & sparsity
    uint64_t size = 0;
    bool sparse = false;
    if (utils_getFileInfo(&vwstr(pPath), handle, &size, &sparse) != UTILS_ERRORSTATE_SUCCESS)
    {
        utils_closeFile(&vwstr(pPath), &handle);
        return UTILS_ERRORSTATE_FAILURE;
    }
    pDiskInfo->og.format.data.raw.size = size;
    // Get sector size
    pDiskInfo->og.format.data.raw.sectorSize = rawImgSectorSize;
    // Get alignment
    uint64_t alignment = 0;
    // *************** THIS IS TEMPORARY BECAUSE CODE ONLY OPERATES ON MBR *************
    alignment = UTILS_SIZESIG_NO_NUMBER;
    pDiskInfo->og.format.data.raw.alignment = alignment;
    // Get scheme
    if (size == 0)
    {
        pDiskInfo->og.scheme.type = SCHEME_TYPE_NULL;
    }
    else if (size >= sizeof(mbr_Data))
    {
        mbr_Data* pMbrBuffer = &pDiskInfo->og.scheme.mbrData;
        if (utils_readFile(&vwstr(pPath), handle, 0, sizeof(mbr_Data), (void**)&pMbrBuffer)
        != UTILS_ERRORSTATE_SUCCESS)
        {
            utils_closeFile(&vwstr(pPath), &handle);
            return UTILS_ERRORSTATE_FAILURE;
        }
        if (pMbrBuffer->signature == 0xAA55) pDiskInfo->og.scheme.type = SCHEME_TYPE_MBR;
        else pDiskInfo->og.scheme.type = SCHEME_TYPE_UNKNOWN;
    }
    else
    {
        pDiskInfo->og.scheme.type = SCHEME_TYPE_UNKNOWN;
    }
    // Set initial selected scheme
    pDiskInfo->selectedScheme = SCHEME_TYPE_MBR;
    // Set initial selected partition number
    pDiskInfo->selectedPartNum = 0;
    // Set initial "edited" booleans
    pDiskInfo->editedAnything = false;
    pDiskInfo->editedFormat = false;
    pDiskInfo->editedScheme = false;
    pDiskInfo->editedFSes = false;
    // Copy original disk into target
    pDiskInfo->target = pDiskInfo->og;
    // Set handle
    pDiskInfo->handle = handle;
    // Adopt path
    pDiskInfo->path = *pPath;
    *pPath = (String8){0};
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState diskInfoCloseReset(DiskInfo* pDiskInfo)
{
    // Caller must check if *pDiskInfo is already clear beforehand
    if (pDiskInfo->handle == UTILS_SYSHANDLE_NONE)
    {
        fprintf(stderr, "pDiskInfo->handle == UTILS_SYSHANDLE_NONE given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    // Close disk/image
    utils_ErrorState es = utils_closeFile(&vwstr(&pDiskInfo->path), &pDiskInfo->handle);
    if (es != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
    // Free path string
    string8Free(&pDiskInfo->path);
    // Clear object
    *pDiskInfo = DISKINFO_DEFAULT;
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState diskInfoWrite(DiskInfo* pDiskInfo)
{    
    const View8 path = vwstr(&pDiskInfo->path);
    utils_ErrorState es = UTILS_ERRORSTATE_FAILURE;
    // Caller must check if the disk in *pDiskInfo isn't opened beforehand
    if (pDiskInfo->handle == UTILS_SYSHANDLE_NONE)
    {
        fprintf(stderr, "pDiskInfo->handle == UTILS_SYSHANDLE_NONE given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    // Set disk sparse (early before size because why not)
    if (pDiskInfo->editedSparsity
    && pDiskInfo->target.sparsity != pDiskInfo->og.sparsity)
    {
        es = utils_setFileSparsity(&path, pDiskInfo->handle, pDiskInfo->target.sparsity);
        // Print warning and move on 🤯
        // if (es != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
    }
    // Set disk size
    if (pDiskInfo->editedFormat
    && pDiskInfo->target.format.data.raw.size != pDiskInfo->og.format.data.raw.size)
    {
        es = utils_resizeFile(&path, pDiskInfo->handle, pDiskInfo->target.format.data.raw.size);
        if (es != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
    }
    // Set sector size
    /* **************************
        No operation currently
    ************************ */
    // Set alignment
    /* **************************
        No operation currently
    ************************ */
    // Set scheme
    if (pDiskInfo->editedScheme)
    {
        if (pDiskInfo->target.scheme.type == SCHEME_TYPE_MBR)
        {
            void* pBuffer = &pDiskInfo->target.scheme.mbrData;
            es = utils_writeFile(&path, pDiskInfo->handle, 0, sizeof(mbr_Data), pBuffer);
            if (es != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
        }
    }
    // Reset "edited" booleans
    pDiskInfo->editedAnything = false;
    pDiskInfo->editedFormat = false;
    pDiskInfo->editedScheme = false;
    pDiskInfo->editedFSes = false;
    // Set original disk object to the written target disk
    pDiskInfo->og = pDiskInfo->target;
    // Return success
    return UTILS_ERRORSTATE_SUCCESS;
}
