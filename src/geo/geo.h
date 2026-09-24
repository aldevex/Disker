#pragma once
#include "./raw.h"

typedef enum geo_Type
{
    GEO_TYPE_NONE,
    GEO_TYPE_RAW_DISK, GEO_TYPE_RAW_IMAGE,
    //VHD, VDI, ...
} geo_Type;

typedef struct geo_Variant
{
    geo_Type type;
    union {
        raw_Data raw;
    } data;
} geo_Variant;
