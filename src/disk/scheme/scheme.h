#pragma once
#include "./mbr.h"
#include "./gpt.h"

typedef enum scheme_Type
{
    SCHEME_TYPE_NONE, SCHEME_TYPE_UNKNOWN,
    SCHEME_TYPE_MBR, SCHEME_TYPE_GPT
} scheme_Type;

typedef struct scheme_Data
{
    scheme_Type type;
    mbr_Data mbrData;
    gpt_Data gptData;
} scheme_Data;
