#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>
#include "../mem/mem.h"

typedef void* utils_SysHandle;
extern const utils_SysHandle UTILS_HANDLE_NONE;

// Prints help string and exits the program
static inline void utils_helpExit()
{
    printf("%s",
        "Help string idfk what to put i will add later\n"
        ""
        ""
        ""
    );
    exit(EXIT_SUCCESS);
}

// Prints explanation on how to run help command
static inline void utils_helpAboutHelp()
{
    printf("exit and run \"Disker help\" for help"
            " | type \"quit\" to exit Disker\n");
}

// Converts the first string to lowercase then checks if start is equal to the second string
// Converts ASCII only to lowercase, anything else remains the same
// Returns true if equal
static inline bool utils_compareLowVw(const View8* pAnyCaseAnyLenView, const View8* pLowerCaseRequiredView)
{
    String8 lowStr = {0};
    for (size_t i = 0; i < view8Size(pAnyCaseAnyLenView); i++)
    {
        if (at(pAnyCaseAnyLenView, i) > 127) string8AppendCU(&lowStr, at(pAnyCaseAnyLenView, i));
        else string8AppendCU(&lowStr, (UTF8_t)tolower(at(pAnyCaseAnyLenView, i)));
    }
    bool result = (view8Size(pAnyCaseAnyLenView) == view8Size(pLowerCaseRequiredView))
                    && string8StartsWithVw(&lowStr, pLowerCaseRequiredView);
    string8Free(&lowStr);
    return result;
};
static inline bool utils_compareLowNT(const View8* pAnyCaseAnyLenView, const UTF8_t* pLowerCaseRequiredText)
{
    View8 view = view8MakeCopyNT(pLowerCaseRequiredText);
    return utils_compareLowVw(pAnyCaseAnyLenView, &view);
};

// strToByteCount reserved return value error signals
typedef uint64_t utils_SizeSig;
#define UTILS_SIZESIG_NO_NUMBER         ((utils_SizeSig)UINT64_MAX)
#define UTILS_SIZESIG_INVALID_NUMBER    ((utils_SizeSig)(UINT64_MAX - 1))
#define UTILS_SIZESIG_NO_UNIT           ((utils_SizeSig)(UINT64_MAX - 2))
#define UTILS_SIZESIG_UNACCEPTABLE_ZERO ((utils_SizeSig)(UINT64_MAX - 3))
#define UTILS_SIZESIG_TOO_BIG_RESULT    ((utils_SizeSig)(UINT64_MAX - 4))
#define UTILS_SIZESIG_INVALID_UNIT      ((utils_SizeSig)(UINT64_MAX - 5))
#define UTILS_SIZESIG_LEASTERROR       UTILS_SIZESIG_INVALID_UNIT

// Converts according to unit
static inline uint64_t _utils_multiply(uint64_t rawNum, uint64_t multiplier, bool shiftNotMultiply)
{
    static const uint64_t CONVSIZE_MAX = (uint64_t)(UTILS_SIZESIG_LEASTERROR) -1;
    // Check if 1. raw number itself is inside enum
    //   or 2. conversion will overflow uint64_t and/or enum limit (both checks work with only CONVSIZE_MAX)
    // This if statement works for both checks because e.g.:
    //   if (2 > 1) then (2 > 1/2, 2 > 1/3, etc.)
    //   so it would only not work for both checks (raw number and conversion) if you divide by n < 1
    if ( (shiftNotMultiply && rawNum > CONVSIZE_MAX / (1ULL << multiplier))
        || (!shiftNotMultiply && rawNum > CONVSIZE_MAX / (multiplier)) )
        return (uint64_t)UTILS_SIZESIG_TOO_BIG_RESULT;
    else if (shiftNotMultiply)
        return rawNum << multiplier;
    else
        return rawNum * multiplier;
}

// Converts strings to unsigned integer with multiplying unit, e.g. "400", "32gIB", "6Mb", "40B"
// May return a specific enum value in the uint64_t on failure (check with SizeSig enum)
// Does NOT print error messages on failure
// Easy way to remember parameter order: "0 then 2 (binary) then unit"
static inline uint64_t utils_strToSizeVw(const View8* pView, bool zeroIsUnacceptable,
                                        bool alwaysBinaryUnits, bool unitExpected)
{
#define KILO_MULTIPLIER (uint64_t)(1000ULL) // 10^3
#define MEGA_MULTIPLIER (uint64_t)(1000ULL * KILO_MULTIPLIER) // 10^6
#define GIGA_MULTIPLIER (uint64_t)(1000ULL * MEGA_MULTIPLIER) // 10^9
#define TERA_MULTIPLIER (uint64_t)(1000ULL * GIGA_MULTIPLIER) // 10^12
#define PETA_MULTIPLIER (uint64_t)(1000ULL * TERA_MULTIPLIER) // 10^15
#define EXA_MULTIPLIER  (uint64_t)(1000ULL * PETA_MULTIPLIER) // 10^18
uint64_t result = 0;

    String8 copy; // String copy to ensure null terminator + remove commas
    // Ignore: commas, single quotes, and underscore off the number
    for (size_t i = 0; i < view8Size(pView); i++)
    {
        UTF8_t c = at(pView, i);
        if (c != ',' && c != '\'' && c != '_') string8AppendCU(&copy, c);
    }

    // No number
    if (string8Empty(&copy)) fret((uint64_t)UTILS_SIZESIG_NO_NUMBER);

    char* endPtr = NULL;
    uint64_t rawNum = strtoull(string8NT(&copy), &endPtr, 0);
    // Total failure
    if (endPtr == string8NT(&copy)) fret((uint64_t)UTILS_SIZESIG_INVALID_NUMBER);
    // Unexpected unit (or random bs text stuck after raw number)
    else if (!unitExpected && endPtr[0] != '\0') fret((uint64_t)UTILS_SIZESIG_INVALID_NUMBER);
    // No unit
    else if (unitExpected && endPtr[0] == '\0') fret((uint64_t)UTILS_SIZESIG_NO_UNIT);
    // Unacceptable zero
    else if (zeroIsUnacceptable && rawNum == 0) fret((uint64_t)UTILS_SIZESIG_UNACCEPTABLE_ZERO);

    View8 unitView = view8MakeCopyNT(endPtr);
    // No unit, or bytes unit
    if (!unitExpected || utils_compareLowNT(&unitView, "b")) fret(_utils_multiply(rawNum, 0, true));
    // Powers of 2 units
    else if (utils_compareLowNT(&unitView, "kib")) fret(_utils_multiply(rawNum, 10, true));
    else if (utils_compareLowNT(&unitView, "mib")) fret(_utils_multiply(rawNum, 20, true));
    else if (utils_compareLowNT(&unitView, "gib")) fret(_utils_multiply(rawNum, 30, true));
    else if (utils_compareLowNT(&unitView, "tib")) fret(_utils_multiply(rawNum, 40, true));
    else if (utils_compareLowNT(&unitView, "pib")) fret(_utils_multiply(rawNum, 50, true));
    else if (utils_compareLowNT(&unitView, "eib")) fret(_utils_multiply(rawNum, 60, true));
    // Powers of 10 units
    else if (utils_compareLowNT(&unitView, "kb")) fret( (alwaysBinaryUnits)?
            _utils_multiply(rawNum, 10, true) : _utils_multiply(rawNum, KILO_MULTIPLIER, false) );
    else if (utils_compareLowNT(&unitView, "mb")) fret( (alwaysBinaryUnits)?
            _utils_multiply(rawNum, 20, true) : _utils_multiply(rawNum, MEGA_MULTIPLIER, false) );
    else if (utils_compareLowNT(&unitView, "gb")) fret( (alwaysBinaryUnits)?
            _utils_multiply(rawNum, 30, true) : _utils_multiply(rawNum, GIGA_MULTIPLIER, false) );
    else if (utils_compareLowNT(&unitView, "tb")) fret( (alwaysBinaryUnits)?
            _utils_multiply(rawNum, 40, true) : _utils_multiply(rawNum, TERA_MULTIPLIER, false) );
    else if (utils_compareLowNT(&unitView, "pb")) fret( (alwaysBinaryUnits)?
            _utils_multiply(rawNum, 50, true) : _utils_multiply(rawNum, PETA_MULTIPLIER, false) );
    else if (utils_compareLowNT(&unitView, "eb")) fret( (alwaysBinaryUnits)?
            _utils_multiply(rawNum, 60, true) : _utils_multiply(rawNum, EXA_MULTIPLIER, false) );

    fret((uint64_t)UTILS_SIZESIG_INVALID_UNIT);

end:
    string8Free(&copy);
    return result;
#undef KILO_MULTIPLIER
#undef MEGA_MULTIPLIER
#undef GIGA_MULTIPLIER
#undef TERA_MULTIPLIER
#undef PETA_MULTIPLIER
#undef EXA_MULTIPLIER
}
static inline uint64_t utils_strToSizeS(const String8* pString, bool zeroIsUnacceptable,
                                        bool alwaysBinaryUnits, bool unitExpected)
{
    View8 view = view8MakeCopyS(pString);
    return utils_strToSizeVw(&view, zeroIsUnacceptable, alwaysBinaryUnits, unitExpected);
}

// Instead of a random "true" that could mean either success or failure
// Important in return values to stop ambiguity
typedef enum utils_ErrorState
{
    UTILS_ERRORSTATE_FAILURE, UTILS_ERRORSTATE_SUCCESS
} utils_ErrorState;

// Returns read buffer and error state
// It prints an error message on failure too
static inline utils_ErrorState utils_readFile(const View8* pPathView, Dbyte* pDarray)
{
    // Check null termination and get C string
    if (!view8Terminated(pPathView))
    {
        fprintf(stderr, "unterminated path view given to \"%s\"\n", __func__);
        exit(EXIT_FAILURE);
    }
    const UTF8_t* path = view8Data(pPathView);

    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "failed to open file for reading \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "failed to seek file end for reading \"%s\"\n", path);
        fclose(file);
        return UTILS_ERRORSTATE_FAILURE;
    }

    size_t fileSize = 0;
    // Scope because i don't need the longSize variable
    {
        long longSize = ftell(file);
        if (longSize < 0)
        {
            fprintf(stderr, "failed to get file size for reading \"%s\"\n", path);
            fclose(file);
            return UTILS_ERRORSTATE_FAILURE;
        }
        else fileSize = (size_t)longSize;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "failed to reset file seek pointer for reading \"%s\"\n", path);
        fclose(file);
        return UTILS_ERRORSTATE_FAILURE;
    }

    Dbyte buffer;
    if (fileSize > 0)
    {
        dbyteResizeZ(&buffer, fileSize);
        size_t readSize = fread(dbyteData(&buffer), 1, fileSize, file);
        if (readSize != fileSize)
        {
            fprintf(stderr, "failed to read file \"%s\"\n", path);
            fclose(file);
            return UTILS_ERRORSTATE_FAILURE;
        }
    }

    if (fclose(file) != 0)
    {
        fprintf(stderr, "failed to close file after reading \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }
    *pDarray = buffer;
    return UTILS_ERRORSTATE_SUCCESS;
}

// Returns error state
// It prints an error message on failure too
static inline utils_ErrorState utils_writeFile(const View8* pPathView, const uint8_t* pBuffer, size_t bufferSize)
{
    // Check null termination and get C string
    if (!view8Terminated(pPathView))
    {
        fprintf(stderr, "unterminated path view given to \"%s\"\n", __func__);
        exit(EXIT_FAILURE);
    }
    const UTF8_t* path = view8Data(pPathView);

    FILE* file = fopen(path, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "failed to open file for writing \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }

    if (bufferSize != 0)
    {
        size_t writtenSize = fwrite(pBuffer, 1, bufferSize, file);
        if (writtenSize != bufferSize)
        {
            fprintf(stderr, "failed to write file \"%s\"\n", path);
            fclose(file);
            return UTILS_ERRORSTATE_FAILURE;
        }
    }

    if (fclose(file) != 0)
    {
        fprintf(stderr, "failed to close file after writing \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }
    return UTILS_ERRORSTATE_SUCCESS;
}

// Calculates IEEE 802.3 CRC32 required by GPT scheme structures
static inline uint32_t utils_calcGPTCRC32(const void* data, size_t length)
{
    // This function is AI slop code idk if it's good or bad
    static uint32_t table[256];
    static bool initialized = false;

    // Build standard IEEE 802.3 table once
    if (!initialized)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t c = i;
            // 0xEDB88320 IEEE 802.3 polynomial
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        initialized = true;
    }

    const uint8_t *p = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++)
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFF; // Final bit inversion
}

extern utils_ErrorState utils_getFileSize(const View8* pPathView, uint64_t* pSize);

typedef enum utils_PathType
{
    UTILS_PATHTYPE_INVALID, UTILS_PATHTYPE_NONE, // Inexistent path
    UTILS_PATHTYPE_FILE, UTILS_PATHTYPE_DIR, UTILS_PATHTYPE_DISK, //UTILS_PATHTYPE_VOLUME
} utils_PathType;

extern utils_PathType utils_getPathType(const View8* pPathView);

// Returns true if given GUID view is valid
static inline bool utils_validateGUID(const View8* pView)
{
    if (view8Size(pView) != 36
    || at(pView, 8) != '-' || at(pView, 13) != '-'
    || at(pView, 18) != '-' || at(pView, 23) != '-')
        return false;
    else return true;
}

// Microsoft format GUID for GPT scheme structures
typedef struct utils_MSGUID
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3; // 4 version bits high 12-15
    uint8_t data4[8]; // 2-4 (variable length) variant bits high 4-7 (variable length) of data4[0]
} utils_MSGUID;

// Create MSGUID instance from random bits (UUIDv4)
extern utils_MSGUID utils_msguidMakeGenV4();

// Create MSGUID instance from GUID string view
static inline utils_MSGUID utils_msguidMakeCopyVw(const View8* pView)
{
    if (!view8Terminated(pView))
    {
        fprintf(stderr, "unterminated view %p given to %s\n",
                        pView, __func__);
        exit(EXIT_FAILURE);
    }
    if (!utils_validateGUID(pView))
    {
        fprintf(stderr, "invalid GUID in view %p given to %s\n",
                        pView, __func__);
        exit(EXIT_FAILURE);
    }
    utils_MSGUID result = {0};
    sscanf(view8Data(pView),
            "%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
            &result.data1, &result.data2, &result.data3,
            &result.data4[0], &result.data4[1], 
            &result.data4[2], &result.data4[3], &result.data4[4], 
            &result.data4[5], &result.data4[6], &result.data4[7]);
    return result;
}

static inline String8 utils_msguidToStr(const utils_MSGUID* pGUID)
{
    String8 result = string8MakeFillV(36, '?');
    snprintf(string8Data(&result), string8Size(&result) +1,
                "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                pGUID->data1, pGUID->data2, pGUID->data3, pGUID->data4[0], pGUID->data4[1],
                pGUID->data4[2], pGUID->data4[3], pGUID->data4[4],
                pGUID->data4[5], pGUID->data4[6], pGUID->data4[7]);
    return result;
}
static inline bool utils_msguidCmp(const utils_MSGUID* pA, const utils_MSGUID* pB)
{
    bool d4equal = true;
    for (size_t i = 0; i < 8; i++)
        if (pA->data4[i] != pB->data4[i])
        {
            d4equal = false;
            break;
        }
    
    return (pA->data1 == pB->data1)
        && (pA->data2 == pB->data2)
        && (pA->data3 == pB->data3)
        && (d4equal);
}
