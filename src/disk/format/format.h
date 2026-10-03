#pragma once
#include "../../utils/utils.h"
#include "./raw.h"

// Disk Format: Type
typedef enum df_Type
{
    DF_TYPE_NULL, DF_TYPE_UNKNOWN,
    DF_TYPE_RAW_DISK, DF_TYPE_RAW_IMAGE,
    //VHD, VDI, ...
} df_Type;

// Disk Format: Variant
typedef struct df_Variant
{
    df_Type type;
    union {
        raw_Data raw;
    } data;
} df_Variant;

// File type to disk format
//   (real disk paths excluded because utils_FileType doesn't have them)
extern df_Type df_fileTypeToDiskFormat(utils_FileType ft);
