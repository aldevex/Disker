#include "./raw_img.h"
#include "../../../base/file.h"

typedef struct Metadata
{
    uint64_t diskSize;
    uint64_t sectorSize;
    uint64_t alignment;
    bool isSparse;
} Metadata;

#include <windows.h>

es_t lrawimgCmdCreate(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo)
{
    // Check if given unclosed disk
    if (pDiskInfo->handle != HANDLE_NONE)
    {
        base_fatalErrorf("Unclosed disk info given to %s.\n",
                        __func__);
        base_halt(ES_FAILURE);
    }

    // Check next args
    // sectsize argument parsing should be added here 🤯

    // Open & lock file
    handle_t handle = base_openFile(view8NT(&pCmdInfo->createDisk.path), BASE_OFMETHOD_CREATE_ALWAYS, NULL);
    if (handle == HANDLE_NONE)
    {
        base_stdErrorf("Failed to create file \"%s\".\n",
                        view8NT(&pCmdInfo->createDisk.path));
        base_printf("errr %u\n", GetLastError());
        return ES_FAILURE;
    }

    // Attempt to make it sparse by default
    bool isSparse = (bool)base_setFileSparsity(handle, true);

    // Set disk layout
    pInfo->pDiskInfo->diskLayout.type = LAYOUT_TYPE_DISK_RAW_IMAGE;
    pInfo->pDiskInfo->diskLayout.pMetadata = base_realloc(NULL, sizeof(Metadata));
    if (pInfo->pDiskInfo->diskLayout.pMetadata == NULL)
    {
        base_fatalErrorf("Failed to allocate memory for raw disk image metadata.\n");
        base_halt(ES_FAILURE);
    }
    *(Metadata*)pInfo->pDiskInfo->diskLayout.pMetadata = (Metadata){
        .diskSize    = 0,
        .sectorSize  = 0,
        .alignment   = 0,
        .isSparse   = isSparse
    };

    // Print
    base_printf("Created new raw image \"%s\" (sparsity = %s).\n",
            view8NT(&pCmdInfo->createDisk.path), utils_boolText(isSparse));

    // Adopt handle and path
    pInfo->pDiskInfo->handle = handle;
    String8* pPathString = cmdInfoCreateDiskReleasePath(pCmdInfo);
    pInfo->pDiskInfo->path = *pPathString;
    *pPathString = (String8){0};

    // Return
    return ES_SUCCESS;
}

es_t lrawimgCmdClose(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo)
{
    // Check if given closed disk
    if (pInfo->pDiskInfo->handle == HANDLE_NONE)
    {
        base_fatalErrorf("Closed disk info given to %s.\n",
                        __func__);
        base_halt(ES_FAILURE);
    }

    // Check next args
    // (no arguments should be after "close <filename>")

    // Reset disk layout
    pInfo->pDiskInfo->diskLayout.type = LAYOUT_TYPE_DISK_RAW_IMAGE;
    base_realloc(pInfo->pDiskInfo->diskLayout.pMetadata, 0);
    pInfo->pDiskInfo->diskLayout.pMetadata = NULL;

    // Close file (also sets handle to HANDLE_NONE, even on failure)
    es_t closeES = base_closeFile(&pInfo->pDiskInfo->handle);
    if (!closeES)
    {
        base_stdErrorf("Failed to close file \"%s\".\n",
                        string8NT(&pInfo->pDiskInfo->path));
    }
    // Print
    else base_printf("Closed disk \"%s\".\n",
                    string8NT(&pInfo->pDiskInfo->path));

    // Free path
    string8Free(&pInfo->pDiskInfo->path);

    // Return
    return ES_SUCCESS;
}

void lrawimgExit(const ConstSharedInfo* pInfo)
{
    // Close file
    if (pInfo->pDiskInfo->handle != HANDLE_NONE)
        base_closeFile(&pInfo->pDiskInfo->handle);

    // Free layout metadata
    base_realloc(pInfo->pDiskInfo->diskLayout.pMetadata, 0);

    // Free path
    string8Free(&pInfo->pDiskInfo->path);
}
