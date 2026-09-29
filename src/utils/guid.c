#include "./guid.h"

// Returns true if given GUID view is valid
bool utils_validateGUID(const View8* pView)
{
    if (view8Size(pView) != 36
    || at(pView, 8) != '-' || at(pView, 13) != '-'
    || at(pView, 18) != '-' || at(pView, 23) != '-')
        return false;
    else return true;
}

// Create MSGUID instance from random bits (UUIDv4)
extern utils_MSGUID utils_msguidMakeGenV4();

// Create MSGUID instance from GUID string view
utils_MSGUID utils_msguidMakeCopyVw(const View8* pView)
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

void utils_msguidToStr(const utils_MSGUID* pGUID, String8* pResult)
{
    string8ResizeDirectly(pResult, 36);
    snprintf(string8Data(pResult), string8Size(pResult) +1,
                "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                pGUID->data1, pGUID->data2, pGUID->data3, pGUID->data4[0], pGUID->data4[1],
                pGUID->data4[2], pGUID->data4[3], pGUID->data4[4],
                pGUID->data4[5], pGUID->data4[6], pGUID->data4[7]);
}
bool utils_msguidCmp(const utils_MSGUID* pA, const utils_MSGUID* pB)
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
