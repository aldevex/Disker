#pragma once
#include "./base.h"

typedef enum utils_PathType
{
    UTILS_PATHTYPE_INVALID, UTILS_PATHTYPE_NONE, // Inexistent path
    UTILS_PATHTYPE_FILE, UTILS_PATHTYPE_DIR, UTILS_PATHTYPE_DISK, //UTILS_PATHTYPE_VOLUME
} utils_PathType;

extern utils_PathType utils_getPathType(const View8* pPathView);
