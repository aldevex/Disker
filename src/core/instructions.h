// inss.hpp (instructions)
#pragma once
#include <stdint.h>
#include "../mem/mem.h"
#include "../scheme/scheme.h"

// Instruction type
typedef enum InsType
{
    INSTYPE_NONE,
    INSTYPE_INTERNAL_SKIP, // Should be converted to NONE before sending outside parsing code

    INSTYPE_SET_YES, // allyes/manyes
    INSTYPE_SET_BINARY, // binary/decimal instructions (for gb/mb/etc. units)

    INSTYPE_OPEN_DISK, // openvd/openphd
    INSTYPE_SET_SCHEME, // Set disk scheme (MBR/GDT) and partition count
    INSTYPE_SET_PART, // Set partition details
    INSTYPE_SET_BOOT, // Set bootloader binary in (and after) MBR, VBR, or at the UEFI default bootloader path
    INSTYPE_OPEN_PART, // Open a partition to set files or sectors

    INSTYPE_SET_FSI, // Set File System Item (file or directory)
    INSTYPE_DEL_FSI, // Delete File System Item (file or directory)
    INSTYPE_COPY_FSI, // Copy File System Item (file or directory)
    INSTYPE_MOVE_FSI, // Move File System Item (file or directory)
    INSTYPE_RENAME_FSI, // Rename File System Item (file or directory)

    INSTYPE_LIST_FSIS, // List File System Items (files and directories)
    INSTYPE_CHANGE_DIR, // Change the current directory
    
    INSTYPE_WHATS, // Get details about a virtual/physical disk, partition, or currently opened subjects
    INSTYPE_SAVE, // Save the edits to the opened disk
    INSTYPE_EXIT // Exit Disker (Stop, Exit, Quit)
} InsType;



typedef struct Ins Ins;
typedef struct InsInfo
{
    String8 _textBuffers[2];
    union {
        bool switchValue;
        struct {
            uint64_t size,
                    sectorSize,
                    physicalSectorSize;
            bool isReal;
        } openDisk;
        struct {
            scheme_Type type;
            uint64_t partitionCount;
        } setScheme;
    };
} InsInfo;

void insInfoFree(InsInfo* pInfo)
{
    string8Free(&pInfo->_textBuffers[0]);
    string8Free(&pInfo->_textBuffers[1]);
}
// Takes ownership of the given string memory and nulls the pointed-to string
//   so no need to free string memory
void insInfoOpenDiskSetPath(InsInfo* pInfo, String8* pPathString)
{
    pInfo->_textBuffers[0] = *pPathString;
    *pPathString = (String8){0};
}
String8* insInfoOpenDiskGetPath(InsInfo* pInfo)
{
    return &(pInfo->_textBuffers[0]);
}

// Instruction
typedef struct Ins
{
    InsType type;
    InsInfo info;
} Ins;

Ins insMake(InsType type, InsInfo info)
{
    Ins result;
    result.type = type;
    result.info = info;
    return result;
}

DARRAY_DEF(Dins, dins, Ins)
