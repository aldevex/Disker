#include "./guid.h"
#ifdef _WIN32
#include <windows.h>
#include <objbase.h> // NEEDED FOR CoCreateGuid() | YOU GOTTA LINK "ole32.dll"
#endif

// Returns true if given GUID view is valid
bool utils_validateGUID(const View8* pView)
{
    if (view8Size(pView) != 36
    || at(pView, 8) != '-' || at(pView, 13) != '-'
    || at(pView, 18) != '-' || at(pView, 23) != '-')
        return false;
    else return true;
}

#ifdef _WIN32
utils_MSGUID utils_msguidMakeGenV4()
{
    // UUIDv4 totally random bits
    GUID apiGUID;
    HRESULT hr = CoCreateGuid(&apiGUID);

    if (SUCCEEDED(hr))
    {
        utils_MSGUID result = (utils_MSGUID){
            .data1 = apiGUID.Data1,
            .data2 = apiGUID.Data2,
            .data3 = apiGUID.Data3,
        };
        memcpy(result.data4, apiGUID.Data4, 8);
        return result;
    }
    else
    {
        fprintf(stderr, "function %s failed to generate random GUID\n", __func__);
        exit(EXIT_FAILURE);
    }
}
#endif

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
