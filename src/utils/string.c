#include <ctype.h>
#include "./string.h"

bool utils_compareLowVw(const View8* pAnyCaseAnyLenView, const View8* pLowerCaseRequiredView)
{
    if (view8Size(pAnyCaseAnyLenView) != view8Size(pLowerCaseRequiredView))
        return false;

    String8 lowStr = {0};
    for (size_t i = 0; i < view8Size(pAnyCaseAnyLenView); i++)
    {
        // Oopsie ASCII only too bad
        if (at(pAnyCaseAnyLenView, i) > 127) string8AppendCU(&lowStr, at(pAnyCaseAnyLenView, i));
        else string8AppendCU(&lowStr, (UTF8_t)tolower(at(pAnyCaseAnyLenView, i)));
    }
    bool result = string8StartsWithVw(&lowStr, pLowerCaseRequiredView);
    string8Free(&lowStr);
    return result;
}
bool utils_compareLowNT(const View8* pAnyCaseAnyLenView, const UTF8_t* pLowerCaseRequiredText)
{
    View8 view = view8MakeCopyNT(pLowerCaseRequiredText);
    return utils_compareLowVw(pAnyCaseAnyLenView, &view);
}



// Multiplies number by unit value
static uint64_t _utilsMultiply(uint64_t rawNum, uint64_t multiplier, bool shiftNotMultiply)
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

uint64_t utils_strToSizeVw(const View8* pView, bool zeroIsUnacceptable,
                            bool alwaysBinaryUnits, bool unitExpected)
{
#define KILO_FACTOR (uint64_t)(1000ULL) // 10^3
#define MEGA_FACTOR (uint64_t)(1000ULL * KILO_FACTOR) // 10^6
#define GIGA_FACTOR (uint64_t)(1000ULL * MEGA_FACTOR) // 10^9
#define TERA_FACTOR (uint64_t)(1000ULL * GIGA_FACTOR) // 10^12
#define PETA_FACTOR (uint64_t)(1000ULL * TERA_FACTOR) // 10^15
#define EXA_FACTOR  (uint64_t)(1000ULL * PETA_FACTOR) // 10^18

uint64_t result = 0;

    String8 copy = {0}; // String copy to ensure null terminator + remove commas
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
    if (!unitExpected || utils_compareLowNT(&unitView, "b")) fret(_utilsMultiply(rawNum, 0, true));
    // Powers of 2 units
    else if (utils_compareLowNT(&unitView, "kib")) fret(_utilsMultiply(rawNum, 10, true));
    else if (utils_compareLowNT(&unitView, "mib")) fret(_utilsMultiply(rawNum, 20, true));
    else if (utils_compareLowNT(&unitView, "gib")) fret(_utilsMultiply(rawNum, 30, true));
    else if (utils_compareLowNT(&unitView, "tib")) fret(_utilsMultiply(rawNum, 40, true));
    else if (utils_compareLowNT(&unitView, "pib")) fret(_utilsMultiply(rawNum, 50, true));
    else if (utils_compareLowNT(&unitView, "eib")) fret(_utilsMultiply(rawNum, 60, true));
    // Powers of 10 units
    else if (utils_compareLowNT(&unitView, "kb")) fret( (alwaysBinaryUnits)?
            _utilsMultiply(rawNum, 10, true) : _utilsMultiply(rawNum, KILO_FACTOR, false) );
    else if (utils_compareLowNT(&unitView, "mb")) fret( (alwaysBinaryUnits)?
            _utilsMultiply(rawNum, 20, true) : _utilsMultiply(rawNum, MEGA_FACTOR, false) );
    else if (utils_compareLowNT(&unitView, "gb")) fret( (alwaysBinaryUnits)?
            _utilsMultiply(rawNum, 30, true) : _utilsMultiply(rawNum, GIGA_FACTOR, false) );
    else if (utils_compareLowNT(&unitView, "tb")) fret( (alwaysBinaryUnits)?
            _utilsMultiply(rawNum, 40, true) : _utilsMultiply(rawNum, TERA_FACTOR, false) );
    else if (utils_compareLowNT(&unitView, "pb")) fret( (alwaysBinaryUnits)?
            _utilsMultiply(rawNum, 50, true) : _utilsMultiply(rawNum, PETA_FACTOR, false) );
    else if (utils_compareLowNT(&unitView, "eb")) fret( (alwaysBinaryUnits)?
            _utilsMultiply(rawNum, 60, true) : _utilsMultiply(rawNum, EXA_FACTOR, false) );

    fret((uint64_t)UTILS_SIZESIG_INVALID_UNIT);

end:
    string8Free(&copy);
    return result;
    
#undef KILO_FACTOR
#undef MEGA_FACTOR
#undef GIGA_FACTOR
#undef TERA_FACTOR
#undef PETA_FACTOR
#undef EXA_FACTOR
}

uint64_t utils_strToSizeS(const String8* pString, bool zeroIsUnacceptable,
                                        bool alwaysBinaryUnits, bool unitExpected)
{
    View8 view = view8MakeCopyS(pString);
    return utils_strToSizeVw(&view, zeroIsUnacceptable, alwaysBinaryUnits, unitExpected);
}



void utils_printSize(FILE* const stream, const char* const prefix, uint64_t size, const char* const suffix)
{
#define KIBI_FACTOR (uint64_t)(1ULL << 10)
#define MEBI_FACTOR (uint64_t)(1ULL << 20)
#define GIBI_FACTOR (uint64_t)(1ULL << 30)
#define TEBI_FACTOR (uint64_t)(1ULL << 40)
#define PEBI_FACTOR (uint64_t)(1ULL << 50)
#define EXBI_FACTOR (uint64_t)(1ULL << 60)

    // SizeSig enum
    if (size >= UTILS_SIZESIG_LEASTERROR)
    {
        fprintf(stderr, "size = %llu >= UTILS_SIZESIG_LEASTERROR given to %s\n",
                        size, __func__);
        exit(EXIT_FAILURE);
    }

    // Prefix
    fprintf(stream, "%s", prefix);

    // EiB
    if (size >= EXBI_FACTOR)
        fprintf(stream, "%.2fEiB (%.2fPiB, %llu bytes)",
                (double)size / EXBI_FACTOR, (double)size / PEBI_FACTOR, size);
    // PiB
    else if (size >= PEBI_FACTOR)
        fprintf(stream, "%.2fPiB (%.2fTiB, %llu bytes)",
                (double)size / PEBI_FACTOR, (double)size / TEBI_FACTOR, size);
    // TiB
    else if (size >= TEBI_FACTOR)
        fprintf(stream, "%.2fTiB (%.2fGiB, %llu bytes)",
                (double)size / TEBI_FACTOR, (double)size / GIBI_FACTOR, size);
    // GiB
    else if (size >= GIBI_FACTOR)
        fprintf(stream, "%.2fGiB (%.2fMiB, %llu bytes)",
                (double)size / GIBI_FACTOR, (double)size / MEBI_FACTOR, size);
    // MiB
    else if (size >= MEBI_FACTOR)
        fprintf(stream, "%.2fMiB (%.2fKiB, %llu bytes)",
                (double)size / MEBI_FACTOR, (double)size / KIBI_FACTOR, size);
    // KiB
    else if (size >= KIBI_FACTOR)
        fprintf(stream, "%.2fKiB (%llu bytes)",
                (double)size / KIBI_FACTOR, size);
    // B
    else
        fprintf(stream, "%lluB", size);
    // Old formatting:
    // // EiB
    // if (size >= EXBI_FACTOR) fprintf(stream, "%.2fEiB (%llu bytes)", (float)size / (float)EXBI_FACTOR, size);
    // // PiB
    // else if (size >= PEBI_FACTOR) fprintf(stream, "%.2fPiB (%llu bytes)", (float)size / (float)PEBI_FACTOR, size);
    // // TiB
    // else if (size >= TEBI_FACTOR) fprintf(stream, "%.2fTiB (%llu bytes)", (float)size / (float)TEBI_FACTOR, size);
    // // GiB
    // else if (size >= GIBI_FACTOR) fprintf(stream, "%.2fGiB (%llu bytes)", (float)size / (float)GIBI_FACTOR, size);
    // // MiB
    // else if (size >= MEBI_FACTOR) fprintf(stream, "%.2fMiB (%llu bytes)", (float)size / (float)MEBI_FACTOR, size);
    // // KiB
    // else if (size >= KIBI_FACTOR) fprintf(stream, "%.2fKiB (%llu bytes)", (float)size / (float)KIBI_FACTOR, size);
    // // B
    // else fprintf(stream, "%lluB", size);

    // Suffix
    fprintf(stream, "%s", suffix);

#undef KIBI_FACTOR
#undef MEBI_FACTOR
#undef GIBI_FACTOR
#undef TEBI_FACTOR
#undef PEBI_FACTOR
#undef EXBI_FACTOR
}
