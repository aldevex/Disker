#include "./file.h"
#include "./string.h"
#include "../../base/file.h"

utils_FileType utils_getFileType(const View8* pPath)
{
    // Linear search macro
    #define check(retVal, ...) do {\
        static const UTF8_t* ARR_##retVal[] = {__VA_ARGS__};\
        for (size_t i = 0; i < lenof(ARR_##retVal); i++)\
            if (utils_compareLowNT(&ext, ARR_##retVal[i]))\
                return retVal;\
    } while (0)
    // Check if extension exists
    const size_t extI = view8RevFindNT(pPath, ".");
    const size_t afterExtI = extI +1;
    const size_t fwSlashI = view8RevFindNT(pPath, "/");
    const size_t bwSlashI = view8RevFindNT(pPath, "\\");
    if (extI == STRING_NF || extI == (view8Size(pPath) -1)
    || (fwSlashI != STRING_NF && fwSlashI > extI)
    || (bwSlashI != STRING_NF && bwSlashI > extI))
        return UTILS_FILETYPE_NOEXT;
    // Get extension
    View8 ext = view8SubStr(pPath, afterExtI, view8Size(pPath) -afterExtI);
    // Check registered extension lists
    check(UTILS_FILETYPE_RAW, "bin", "img", "raw", "dd");
    // Unknown extension
    return UTILS_FILETYPE_UNKNOWN;
    // Undefine search macro
    #undef check
}

utils_PathType utils_getPathType(const View8* pPath)
{
    base_PathType bpt = base_getPathType(view8Data(pPath), view8Size(pPath));
    switch (bpt)
    {
    case BASE_PATHTYPE_NULL: return UTILS_PATHTYPE_NULL;
    case BASE_PATHTYPE_INVALID: return UTILS_PATHTYPE_INVALID;
    case BASE_PATHTYPE_FILE: return UTILS_PATHTYPE_FILE;
    case BASE_PATHTYPE_DIR: return UTILS_PATHTYPE_DIR;
    case BASE_PATHTYPE_CREATABLE_FILE: return UTILS_PATHTYPE_CREATABLE_FILE;
    case BASE_PATHTYPE_VOLUME: return UTILS_PATHTYPE_VOLUME;
    case BASE_PATHTYPE_DISK: return UTILS_PATHTYPE_DISK;
    }
    return UTILS_PATHTYPE_INVALID;
}
