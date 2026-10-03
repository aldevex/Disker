#pragma once
#include "./base.h"
#include "string.h"

typedef enum utils_FileType
{
    UTILS_FILETYPE_NULL,
    UTILS_FILETYPE_UNKNOWN,
    UTILS_FILETYPE_NOEXT, // No extension
    UTILS_FILETYPE_RAW, // .bin .img .raw
    // UTILS_FILETYPE_TEXT,
} utils_FileType;

// Gets file type from extension in path
// Returns UTILS_FILETYPE_NULL on failure
extern utils_FileType utils_getFileType(const View8* pPath);

typedef enum utils_PathType
{
    UTILS_PATHTYPE_NULL, UTILS_PATHTYPE_INVALID,
    UTILS_PATHTYPE_FILE, UTILS_PATHTYPE_DIR,
    UTILS_PATHTYPE_PART, UTILS_PATHTYPE_DISK,
    UTILS_PATHTYPE_MAYBE_CREATABLE
} utils_PathType;

// Returns UTILS_PATHTYPE_NULL on failure
extern utils_PathType utils_getPathType(const View8* pPath);

// Gets UTILS_PATHTYPE_DISK path from UTILS_PATHTYPE_VOLUME path
// extern utils_ErrorState utils_getDiskPath(const View8* pSymbol, String8* pResult);

// Returns UTILS_SIZESIG_NO_NUMBER on failure
// Also prints errors
extern uint64_t utils_getFileSize(const View8* pPath);



// Opens and locks a file
// Returns UTILS_SYSHANDLE_NONE on failure
// Prints an error message on failure
// If pCreatedNewFile == NULL or *pCreatedNewFile == false: won't create a new file on failure to open
//   otherwise: sets *pCreatedNewFile to true or false if created a new file
extern utils_SysHandle utils_openFile(const View8* pPath, bool* pCreatedNewFile);
// Unlocks and closes a file
// Sets *pHandle to UTILS_SYSHANDLE_NONE
// Prints an error message on failure
extern utils_ErrorState utils_closeFile(const View8* pPath, utils_SysHandle* pHandle);

// If *pPtrBuffer is NULL: Sets it to newly allocated buffer with the data in it
//   otherwise, writes at the address in *pPtrBuffer
// Prints an error message on failure
extern utils_ErrorState utils_readFile(const View8* pPath, utils_SysHandle handle,
                                        uint64_t offset, uint64_t count, void** pPtrBuffer);
// Prints an error message on failure
extern utils_ErrorState utils_writeFile(const View8* pPath, utils_SysHandle handle,
                                        uint64_t offset, uint64_t count, const void* pBuffer);
// Prints an error message on failure
extern utils_ErrorState utils_extendFile(const View8* pPath, utils_SysHandle handle,
                                        uint64_t count);
