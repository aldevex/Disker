#pragma once
#include "./fat32.h"

typedef enum fs_Type
{
    FS_TYPE_NONE, FS_TYPE_UNKNOWN, FS_TYPE_FAT32
} fs_Type;

typedef struct fs_Variant
{
    fs_Type type;
    union {
        void* placeholdertonotgeterrors;
        //fat32_Data fat32;
    } data;
} fs_Variant;

DARRAY_DEF(Dfs, dfs, fs_Variant)
