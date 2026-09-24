#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../mem/mem.h"
#include "../utils/generic.h"
#include "../scheme/scheme.h"
#include "../fs/fs.h"
#include "../diskinfo.h"

typedef enum CmdType
{
    // Parse/creation-level types (must not reach to applyCmd):

    CMDTYPE_NONE,

    CMDTYPE_SET_YES,
    CMDTYPE_SET_BINARY,

    // "select scheme" & "select disk" both will affect applyCmd indirectly
    //   by sending disk info, selected scheme type, and partition number
    //   to the applyCmd calls while parsing/creating commands

    CMDTYPE_SELECT_DISK,
    // CMDTYPE_SELECT_SCHEME,
    // CMDTYPE_SELECT_PART,

    CMDTYPE_SAVE,
    // CMDTYPE_CLOSE,
    CMDTYPE_EXIT,

    // CMDTYPE_UNDO,
    // CMDTYPE_REDO,

    CMDTYPE_HELP,
    // CMDTYPE_LIST_BOOT,
    // CMDTYPE_LIST_PART,
    // CMDTYPE_LIST_DISK,
    // CMDTYPE_LIST_SYS,


    // Apply-level types (reach to applyCmds in order and get applied there):

    CMDTYPE_EDIT_DISK,
    // CMDTYPE_EDIT_RESERVE,
    // CMDTYPE_EDIT_PART,
    // CMDTYPE_EDIT_BOOT,

    // FSI stands for file system item (file or directory)

    // CMDTYPE_FS_CWD, // Change or show CWD (cd, pwd)
    // CMDTYPE_FS_LIST, // List FSIs in the CWD (list, ls)
    // CMDTYPE_FS_TREE, // List all FSIs in the CWD including nested ones (tree)
    // CMDTYPE_FS_FIND, // Find a specific file (find)
    // CMDTYPE_FS_CREATE, // Create an FSI (touch. mkdir, md, create, make)
    // CMDTYPE_FS_DELETE, // Delete an FSI (rm, del, rmdir, rd)
    // CMDTYPE_FS_PRINT, // Print the contents of a file (type, echo)
    // CMDTYPE_FS_ATTRIBUTES, // Prints FSI attributes (attrib, stat)
    // CMDTYPE_FS_RENAME, // Rename an FSI (ren, rename)
    // CMDTYPE_FS_COPY, // Copy an FSI (copy, cp, move, mv)
} CmdType;

typedef struct CmdInfoSelectDisk
{
    View8 path;
    geo_Type type;
} CmdInfoSelectDisk;

typedef struct CmdInfoEditDisk
{
    uint64_t size, sectorSize, alignment, partCount;
    scheme_Type scheme;
} CmdInfoEditDisk;

typedef struct CmdInfo
{
    String8 _textBuffers[2];
    union {
        bool switchValue;
        CmdInfoSelectDisk selectDisk;
        CmdInfoEditDisk editDisk;
    };
} CmdInfo;

static inline void cmdInfoFree(CmdInfo* pInfo)
{
    string8Free(&pInfo->_textBuffers[0]);
    string8Free(&pInfo->_textBuffers[1]);
}
// Takes ownership of the given string memory and nulls out the pointed-to string
//   so no need to free string memory after call
static inline void cmdInfoSelectDiskAdoptPath(CmdInfo* pInfo, String8* pPathString)
{
    pInfo->_textBuffers[0] = *pPathString;
    *pPathString = (String8){0};
    pInfo->selectDisk.path = view8MakeCopyS(&pInfo->_textBuffers[0]);
}
// Gives direct pointer to path string for another object to take ownership of it
static inline String8* cmdInfoSelectDiskReleasePath(CmdInfo* pInfo)
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

DARRAY_DEF(Dcmd, dcmd, Cmd)

typedef struct ApplyCmdInfo
{
    Cmd* pCmd;
    DiskInfo* pDiskInfo;
    scheme_Type selectedScheme;
    uint64_t partNum;
} ApplyCmdInfo;

extern utils_ErrorState applyCmd(ApplyCmdInfo* pInfo);
