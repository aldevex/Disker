#pragma once
#include "../../mem/mem.h"
#include "../utils/utils.h"
#include "../layout/layout.h"

typedef enum CmdType
{
    CMDTYPE_NULL,

    CMDTYPE_SET_AUTO_YES,
    CMDTYPE_SET_ALWAYS_BINARY,

    CMDTYPE_CLS,
    CMDTYPE_HELP,
    CMDTYPE_VERSION,

    CMDTYPE_CREATE_DISK,
    // CMDTYPE_SELECT_DISK,
    // CMDTYPE_SELECT_PART,

    // CMDTYPE_SAVE,
    CMDTYPE_CLOSE,
    CMDTYPE_EXIT,

    // CMDTYPE_LIST_SYS,
    // CMDTYPE_LIST_DISK,
    // CMDTYPE_LIST_PART,
    // CMDTYPE_LIST_BOOT,

    // CMDTYPE_EDIT_DISK,
    // CMDTYPE_EDIT_PART,
    // CMDTYPE_EDIT_BOOT,

    // In the comments FSI stands for File System Item (file or directory)

    // CMDTYPE_FS_CWD, // Change or print CWD
    // CMDTYPE_FS_LIST, // List FSIs in the CWD
    // CMDTYPE_FS_TREE, // List all FSIs in the CWD including nested ones
    // CMDTYPE_FS_FIND, // Find a specific file
    // CMDTYPE_FS_CREATE, // Create an FSI
    // CMDTYPE_FS_DELETE, // Delete an FSI
    // CMDTYPE_FS_PRINT, // Print the contents of a file
    // CMDTYPE_FS_ATTRIBUTES, // Print FSI attributes
    // CMDTYPE_FS_RENAME, // Rename an FSI
    // CMDTYPE_FS_COPY, // Copy an FSI
    
    // EG stands for Easter Egg

    CMDTYPE_EG_444,
    CMDTYPE_EG_DIFF_VHD,
    CMDTYPE_EG_BIG_K,
    CMDTYPE_EG_BIG_J,
    CMDTYPE_EG_BIG_P,
    CMDTYPE_EG_BIG_M,
    CMDTYPE_EG_UEFI_SPEC,
    CMDTYPE_EG_FRANCAIS,
    CMDTYPE_EG_IGHRIQI,
    CMDTYPE_EG_SERBIAN,
    CMDTYPE_EG_DUTCHLANDIAN,
} CmdType;

// The command struct is only for
//   the beginning of a command that is parsed in core(),
//   everything that isn't parsed (after the beginning) gets parsed in layout-specific functions

typedef struct CmdInfoCreateDisk
{
    View8 path;
    layout_DiskType type;
} CmdInfoCreateDisk;

// typedef struct CmdInfoSelectDisk
// {
//     View8 path;
//     df_Type type;
//     uint64_t rawImgSectorSize;
// } CmdInfoSelectDisk;

// typedef struct CmdInfoEditDisk
// {
//     uint64_t size, sectorSize, alignment, partCount;
//     scheme_Type scheme;
//     utils_Tribool sparse;
// } CmdInfoEditDisk;

// typedef struct CmdInfoEditPart
// {
//     uint64_t partNum;
//     uint64_t size;
//     fs_Type type;
// } CmdInfoEditPart;

typedef struct CmdInfo
{
    String8 _textBuffers[2];
    union {
        bool switchValue;
        CmdInfoCreateDisk createDisk;
        // CmdInfoSelectDisk selectDisk;
        // CmdInfoEditDisk editDisk;
        // CmdInfoEditPart editPart;
    };
} CmdInfo;

static inline void cmdInfoFree(CmdInfo* pInfo)
{
    string8Free(&pInfo->_textBuffers[0]);
    string8Free(&pInfo->_textBuffers[1]);
}
// Takes ownership of the given string memory and nulls out the pointed-to string
//   so no need to free string memory after call
static inline void cmdInfoCreateDiskAdoptPath(CmdInfo* pInfo, String8* pPathString)
{
    pInfo->_textBuffers[0] = *pPathString;
    *pPathString = (String8){0};
    pInfo->createDisk.path = view8MakeCopyS(&pInfo->_textBuffers[0]);
}
// Gives direct pointer to path string for another object to take ownership of it
static inline String8* cmdInfoCreateDiskReleasePath(CmdInfo* pInfo)
{
    return &pInfo->_textBuffers[0];
}

typedef struct Cmd
{
    CmdType type;
    CmdInfo info;
} Cmd;

static inline Cmd cmdMake(CmdType type, CmdInfo info)
{
    Cmd result;
    result.type = type;
    result.info = info;
    return result;
}
