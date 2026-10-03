#include "./format.h"

df_Type df_fileTypeToDiskFormat(utils_FileType ft)
{
    switch (ft)
    {
    case UTILS_FILETYPE_RAW: return DF_TYPE_RAW_IMAGE;
    // Return UNKNOWN for:
    //   UTILS_FILETYPE_NULL,
    //   UTILS_FILETYPE_UNKNOWN, UTILS_FILETYPE_NOEXT
    default: return DF_TYPE_UNKNOWN;
    }
    return DF_TYPE_UNKNOWN;
}
