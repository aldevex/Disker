#pragma once
#include "./base.h"

// Converts the first string to lowercase then checks if start is equal to the second string
// Converts ASCII only to lowercase, anything else remains the same
// Returns true if equal
extern bool utils_compareLowVw(const View8* pAnyCaseAnyLenView, const View8* pLowerCaseRequiredView);
extern bool utils_compareLowNT(const View8* pAnyCaseAnyLenView, const UTF8_t* pLowerCaseRequiredText);

// strToByteCount reserved return value error signals
typedef uint64_t utils_SizeSig;
#define UTILS_SIZESIG_NO_NUMBER         ((utils_SizeSig)UINT64_MAX)
#define UTILS_SIZESIG_INVALID_NUMBER    ((utils_SizeSig)(UINT64_MAX - 1))
#define UTILS_SIZESIG_NO_UNIT           ((utils_SizeSig)(UINT64_MAX - 2))
#define UTILS_SIZESIG_UNACCEPTABLE_ZERO ((utils_SizeSig)(UINT64_MAX - 3))
#define UTILS_SIZESIG_TOO_BIG_RESULT    ((utils_SizeSig)(UINT64_MAX - 4))
#define UTILS_SIZESIG_INVALID_UNIT      ((utils_SizeSig)(UINT64_MAX - 5))
#define UTILS_SIZESIG_LEASTERROR        UTILS_SIZESIG_INVALID_UNIT

// Converts strings to unsigned integer with multiplying unit, e.g. "400", "32gIB", "6Mb", "40B"
// May return a specific enum value in the uint64_t on failure (check with SizeSig enum)
// Does NOT print error messages on failure
// Easy way to remember parameter order: "0 then 2 (binary) then unit"
extern uint64_t utils_strToSizeVw(const View8* pView, bool zeroIsUnacceptable,
                                    bool alwaysBinaryUnits, bool unitExpected);
extern uint64_t utils_strToSizeS(const String8* pString, bool zeroIsUnacceptable,
                                    bool alwaysBinaryUnits, bool unitExpected);
// Divides size by biggest unit which results in (n > 0) and prints it (includes value in bytes)
extern void utils_printSize(FILE* const stream, const char* const prefix, uint64_t size, const char* const suffix);
