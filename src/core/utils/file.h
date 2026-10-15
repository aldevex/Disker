#pragma once
#include "../../base/core.h"
#include "../../mem/mem.h"

typedef enum utils_PathType
{
    UTILS_PATHTYPE_NULL, UTILS_PATHTYPE_INVALID,
    UTILS_PATHTYPE_FILE, UTILS_PATHTYPE_DIR,
    UTILS_PATHTYPE_CREATABLE_FILE,
    UTILS_PATHTYPE_VOLUME, UTILS_PATHTYPE_DISK
} utils_PathType;

// Returns UTILS_PATHTYPE_NULL on failure
extern utils_PathType utils_getPathType(const View8* pPath);

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
