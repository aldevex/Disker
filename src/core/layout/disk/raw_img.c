#include "./raw_img.h"
#include "../../../base/file.h"
#include "../../tui/parse.h"

typedef struct Metadata
{
    uint64_t diskSize;
    uint64_t sectorSize;
    uint64_t alignment;
    bool isSparse;
} Metadata;

es_t ldrawimg_create(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo)
{
    // Check if given unclosed disk
    if (pDiskInfo->handle != HANDLE_NONE)
    {
        base_fatalErrorf("Unclosed disk info given to %s.\n",
                        __func__);
        base_halt(ES_FAILURE);
    }

    // Check next args: nothing after beginning

    // Move this to edit command function later:
    // // Check next args: sectsize
    // ParseSharedInfo parseInfo = constToParseSharedInfo(pInfo);
    // Dadinfo decls = {0};
    // ExtractedAV sectSizeIn = {0};
    // dadinfoAppendV(&decls, adinfoMake("sectsize", false, false, (AVInfo[]){
    //     avinfoMakeNumeric("sector size", false, true, 512, &sectSizeIn)
    // }, 1));
    // if (parseCmd(pInfo->pCmdSegments, pInfo->afterBeginningI, &decls, &parseInfo, NULL)
    //     != ES_SUCCESS) return ES_FAILURE;

    // Open & lock file
    handle_t handle = base_openFile(view8NT(&pCmdInfo->createDisk.path), BASE_OFMETHOD_CREATE_ALWAYS, NULL);
    if (handle == HANDLE_NONE)
    {
        base_stdErrorf("Failed to create file \"%s\".\n",
                        view8NT(&pCmdInfo->createDisk.path));
        return ES_FAILURE;
    }

    // Attempt to make it sparse by default
    bool isSparse = (bool)base_setFileSparsity(handle, true);

    // Set disk layout
    pDiskInfo->diskLayout.type = LAYOUT_DISKTYPE_RAW_IMAGE;
    pDiskInfo->diskLayout.pMetadata = base_realloc(NULL, sizeof(Metadata));
    if (pDiskInfo->diskLayout.pMetadata == NULL)
    {
        base_fatalErrorf("Failed to allocate memory for raw disk image metadata.\n");
        base_halt(ES_FAILURE);
    }
    *(Metadata*)pDiskInfo->diskLayout.pMetadata = (Metadata){
        .diskSize    = 0,
        .sectorSize  = 0,
        .alignment   = 0,
        .isSparse   = isSparse
    };

    // Print
    base_printf("Created new raw image \"%s\" (sparsity = %s).\n",
            view8NT(&pCmdInfo->createDisk.path), utils_boolText(isSparse));

    // Adopt handle and path
    pDiskInfo->handle = handle;
    String8* pPathString = cmdInfoCreateDiskReleasePath(pCmdInfo);
    pDiskInfo->path = *pPathString;
    *pPathString = (String8){0};

    // Return
    return ES_SUCCESS;
}

es_t ldrawimg_close(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo)
{
    // Check if given closed disk
    if (pDiskInfo->handle == HANDLE_NONE)
    {
        base_fatalErrorf("Closed disk info given to %s.\n",
                        __func__);
        base_halt(ES_FAILURE);
    }

    // Check next args
    // (no arguments should be after "close <filename>")

    // Reset disk layout
    pDiskInfo->diskLayout.type = LAYOUT_DISKTYPE_RAW_IMAGE;
    base_realloc(pDiskInfo->diskLayout.pMetadata, 0);
    pDiskInfo->diskLayout.pMetadata = NULL;

    // Close file (also sets handle to HANDLE_NONE, even on failure)
    es_t closeES = base_closeFile(&pDiskInfo->handle);
    if (!closeES)
    {
        base_stdErrorf("Failed to close file \"%s\".\n",
                        string8NT(&pDiskInfo->path));
    }
    // Print
    else base_printf("Closed disk \"%s\".\n",
                    string8NT(&pDiskInfo->path));

    // Free path
    string8Free(&pDiskInfo->path);

    // Return
    return ES_SUCCESS;
}

void ldrawimg_free(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo)
{
    // Close file
    if (pDiskInfo->handle != HANDLE_NONE)
        base_closeFile(&pDiskInfo->handle);

    // Free layout metadata
    base_realloc(pDiskInfo->diskLayout.pMetadata, 0);

    // Free path
    string8Free(&pDiskInfo->path);
}
