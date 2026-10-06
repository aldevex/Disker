#pragma once
#include "./fat.h"

typedef enum fs_Type
{
    FS_TYPE_NULL, FS_TYPE_UNKNOWN,
    FS_TYPE_FAT32
} fs_Type;

typedef struct fs_Variant
{
    fs_Type type;
    union {
        fat_Variant fat;
    } data;
} fs_Variant;

DARRAY_DEF(Dfs, dfs, fs_Variant)
