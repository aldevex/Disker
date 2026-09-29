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

// Converts according to unit
static uint64_t _utils_multiply(uint64_t rawNum, uint64_t multiplier, bool shiftNotMultiply)
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
#define KILO_MULTIPLIER (uint64_t)(1000ULL) // 10^3
#define MEGA_MULTIPLIER (uint64_t)(1000ULL * KILO_MULTIPLIER) // 10^6
#define GIGA_MULTIPLIER (uint64_t)(1000ULL * MEGA_MULTIPLIER) // 10^9
#define TERA_MULTIPLIER (uint64_t)(1000ULL * GIGA_MULTIPLIER) // 10^12
#define PETA_MULTIPLIER (uint64_t)(1000ULL * TERA_MULTIPLIER) // 10^15
#define EXA_MULTIPLIER  (uint64_t)(1000ULL * PETA_MULTIPLIER) // 10^18
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
uint64_t utils_strToSizeS(const String8* pString, bool zeroIsUnacceptable,
                                        bool alwaysBinaryUnits, bool unitExpected)
{
    View8 view = view8MakeCopyS(pString);
    return utils_strToSizeVw(&view, zeroIsUnacceptable, alwaysBinaryUnits, unitExpected);
}
