#pragma once
#include "./base.h"

// Returns true if given GUID view is valid
extern bool utils_validateGUID(const View8* pView);

#pragma pack(push, 1)
// Microsoft format GUID for GPT scheme structures
typedef struct utils_MSGUID
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3; // 4 version bits high 12-15
    uint8_t data4[8]; // 2-4 (variable length) variant bits high 4-7 (variable length) of data4[0]
} utils_MSGUID;
#pragma pack(pop)

// Create MSGUID instance from random bits (UUIDv4)
extern utils_MSGUID utils_msguidMakeGenV4();
// Create MSGUID instance from GUID string view
extern utils_MSGUID utils_msguidMakeCopyVw(const View8* pView);

extern void utils_msguidToStr(const utils_MSGUID* pGUID, String8* pResult);
extern bool utils_msguidCmp(const utils_MSGUID* pA, const utils_MSGUID* pB);
