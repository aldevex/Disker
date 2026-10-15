// Windows & C standard library platform-specific implementation
#include "./core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> // Needed for CommandLineToArgvW() |      MUST LINK "shell32.dll"
#include <winioctl.h> // Needed for sparse file attribute R/W
#include <objbase.h> // Needed for CoCreateGuid() |             MUST LINK "ole32.dll"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>



// Internal utils

// Input text must be null terminated
// Allocates and returns UTF-16 converted string
// Returns NULL on failure
static UTF16_t* _base_UTF8to16(const UTF8_t* pSrc)
{
    // Get source code unit count
    size_t countSrcNT = 0;
    while (pSrc[countSrcNT] != (UTF8_t)0) countSrcNT++;
    countSrcNT++;
    // Allocate initially same code unit amount as source
    size_t countDstNT = countSrcNT;
    UTF16_t* pDst = base_realloc(NULL, countDstNT *sizeof(UTF16_t));
    if (pDst == NULL) return NULL;
    // Convert (including null terminator)
    size_t dstI = 0;
    for (size_t srcI = 0; srcI < countSrcNT; )
    {
        // Result surrogates (if no pair, high is used and low = 0)
        uint16_t resultHigh = 0, resultLow = 0;

        // One code unit
        if ((pSrc[srcI +0] & 0b10000000) == 0b0)
        {
            resultHigh = (UTF16_t)pSrc[srcI +0];
            srcI += 1;
        }
        // Two code units
        else if ((pSrc[srcI +0] & 0b11100000) == 0b11000000
        // Validate second code unit
        && (srcI +1) < countSrcNT
        && (pSrc[srcI +1] & 0b11000000) == 0b10000000)
        {
            resultHigh = ((UTF16_t)(pSrc[srcI +0] & 0b00011111) << 6)
                        + (UTF16_t)(pSrc[srcI +1] & 0b00111111);
            srcI += 2;
        }
        // Three code units
        else if ((pSrc[srcI +0] & 0b11110000) == 0b11100000
        // Validate second and third code units
        && (srcI +2) < countSrcNT
        && (pSrc[srcI +1] & 0b11000000) == 0b10000000
        && (pSrc[srcI +2] & 0b11000000) == 0b10000000)
        {
            resultHigh = ((UTF16_t)(pSrc[srcI +0] & 0b00001111) << 12)
                        + ((UTF16_t)(pSrc[srcI +1] & 0b00111111) << 6)
                        +(UTF16_t)(pSrc[srcI +2] & 0b00111111);
            srcI += 3;
        }
        // Four code units
        else if ((pSrc[srcI +0] & 0b11111000) == 0b11110000
        // Validate second, third, and fourth code units
        && (srcI +3) < countSrcNT
        && (pSrc[srcI +1] & 0b11000000) == 0b10000000
        && (pSrc[srcI +2] & 0b11000000) == 0b10000000
        && (pSrc[srcI +3] & 0b11000000) == 0b10000000)
        {
            uint32_t conversionValue =
                ((uint32_t)(pSrc[srcI +0] & 0b00000111) << 18)
                + ((uint32_t)(pSrc[srcI +1] & 0b00111111) << 12)
                + ((uint32_t)(pSrc[srcI +2] & 0b00111111) << 6)
                + (uint32_t)(pSrc[srcI +3] & 0b00111111)
            - 0x10000;
            resultHigh = (UTF16_t)(0xD800 + (conversionValue >> 10));
            resultLow = (UTF16_t)(0xDC00 + (conversionValue & 0b1111111111));
            srcI += 4;
        }
        // Invalid code units
        else
        {
            base_realloc(pDst, 0);
            return NULL;
        }
        // Ensure enough reserved memory for new code units
        if (dstI +((resultLow == 0)? 0 : 1) >= countDstNT)
        {
            countDstNT = ((countDstNT +2) *2);
            UTF16_t* pNew = base_realloc(pDst, countDstNT *sizeof(UTF16_t));
            if (pNew == NULL)
            {
                base_realloc(pDst, 0);
                return NULL;
            }
            else pDst = pNew;
        }
        // Add new UTF-16 code units
        pDst[dstI] = resultHigh;
        dstI++;
        if (resultLow != 0)
        {
            pDst[dstI] = resultLow;
            dstI++;
        }
    }
    return pDst;
}

static UTF8_t* _base_UTF16to8(const UTF16_t* pSrc)
{
    // Get source code unit count
    size_t countSrcNT = 0;
    while (pSrc[countSrcNT] != (UTF16_t)0) countSrcNT++;
    countSrcNT++;
    // Allocate initially same code unit amount as source
    size_t countDstNT = countSrcNT;
    UTF8_t* pDst = base_realloc(NULL, countDstNT *sizeof(UTF8_t));
    if (pDst == NULL) return NULL;
    // Convert (including null terminator)
    size_t dstI = 0;
    for (size_t srcI = 0; srcI < countSrcNT; )
    {
        uint8_t result[4] = {0, 0, 0, 0};
        size_t resultSize = 0;
        // High surrogate
        if ((pSrc[srcI +0] >= 0xD800 && pSrc[srcI +0] <= 0xDBFF)
        // Validate second code unit (must be a low surrogate)
        && (pSrc[srcI +1] < 0xDC00 && pSrc[srcI +1] > 0xDFFF))
        {
            uint32_t conversionValue =
                (((uint32_t)(pSrc[srcI +0] & 0b1111111111)) << 10) // High surrogate
                + (uint32_t)(pSrc[srcI +1] & 0b1111111111) // Low surrogate
            + 0x10000U;
            result[0] = (UTF8_t)(((conversionValue >> 18) & 0b00000111) | 0b11110000);
            result[1] = (UTF8_t)(((conversionValue >> 12) & 0b00111111) | 0b10000000);
            result[2] = (UTF8_t)(((conversionValue >> 6) & 0b00111111) | 0b10000000);
            result[3] = (UTF8_t)((conversionValue & 0b00111111) | 0b10000000);
            resultSize = 4;
            srcI += 2;
        }
        // Single code unit (BMP), not a high surrogate in a pair
        // Validate (musn't be a low surrogate, and is in the BMP)
        else if (pSrc[srcI +0] < 0xDC00 || pSrc[srcI +0] > 0xDFFF)
        {
            // Convert to one UTF-8 code unit
            if (pSrc[srcI +0] <= 0x007F)
            {
                result[0] = (UTF8_t)pSrc[srcI +0];
                resultSize = 1;
            }
            // Convert to two UTF-8 code units
            else if (pSrc[srcI +0] <= 0x07FF)
            {
                result[0] = (UTF8_t)(((pSrc[srcI +0] >> 6) & 0b0011111) | 0b11000000);
                result[1] = (UTF8_t)((pSrc[srcI +0] & 0b0111111) | 0b10000000);
                resultSize = 2;
            }
            // Convert to three UTF-8 code units
            else if (pSrc[srcI +0] <= 0xFFFF)
            {
                result[0] = (UTF8_t)(((pSrc[srcI +0] >> 12) & 0b00001111) | 0b11100000);
                result[1] = (UTF8_t)(((pSrc[srcI +0] >> 6) & 0b00111111) | 0b10000000);
                result[3] = (UTF8_t)((pSrc[srcI +0] & 0b00111111) | 0b10000000);
                resultSize = 3;
            }
            srcI += 1;
        }
        // Invalid code units
        else
        {
            base_realloc(pDst, 0);
            return NULL;
        }
        // Ensure enough reserved memory for new code units
        if (dstI +resultSize > countDstNT)
        {
            countDstNT = ((countDstNT +4) *2);
            UTF8_t* pNew = base_realloc(pDst, countDstNT *sizeof(UTF8_t));
            if (pNew == NULL)
            {
                base_realloc(pDst, 0);
                return NULL;
            }
            else pDst = pNew;
        }
        // Add new UTF-8 code units
        for (size_t cuI = 0; cuI < resultSize; cuI++, dstI++)
        {
            pDst[dstI] = result[cuI];
        }
    }
    return pDst;
}

ATTRIB_NORETURN static void _base_directHalt(es_t success)
{
    ExitProcess((success)? (UINT)EXIT_SUCCESS : (UINT)EXIT_FAILURE);
}

static void _base_directPrint(const UTF8_t* text)
{
    const HANDLE hConOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(hConOutput, text, strlen(text), NULL, NULL);
}



// Enty point

BOOL WINAPI _base_sigHandler(DWORD signal)
{
    // Return false = pass the signal to the next handler
    if (signal != CTRL_C_EVENT) return false;
    // Reset terminal colour to default & exit
    _base_directPrint("\e[0m");
    _base_directHalt(ES_FAILURE);
    // JIC compiler complains
    return true;
}

int main(int argCount, UTF8_t** args8)
{
    // Set IO and formatting locales and codepages
    setlocale(LC_MONETARY, ""); // User default
    setlocale(LC_NUMERIC, ""); // User default
    setlocale(LC_TIME, ""); // User default
    
    setlocale(LC_COLLATE, ".UTF-8"); // User language in UTF-8
    setlocale(LC_CTYPE, ".UTF-8"); // User language in UTF-8
    SetConsoleOutputCP(CP_UTF8); // UTF-8
    SetConsoleCP(CP_UTF8); // UTF-8

// #elif __linux__
//     setlocale(LC_COLLATE, "C.UTF-8"); // Basic C locale in UTF-8
//     setlocale(LC_CTYPE, "C.UTF-8"); // Basic C locale in UTF-8
// #endif

    // Set signal handler to reset terminal colour on halt (Ctrl+C)
    if (!SetConsoleCtrlHandler(_base_sigHandler, TRUE))
        return ES_FAILURE;

    // Set random seed (for easter eggs random number generation)
    srand((unsigned int)time(NULL));

    // Get UTF-16 args
    UTF16_t** args16 = CommandLineToArgvW(GetCommandLineW(), &argCount);
    if (args16 == NULL)
    {
        base_fatalErrorf("Failed to get UTF-16 arguments.\n");
        base_halt(ES_FAILURE);
    }
    // Allocate pointers array
    args8 = base_realloc(NULL, argCount *sizeof(UTF8_t*));
    if (args8 == NULL)
    {
        base_fatalErrorf("Failed to allocate pointer array for arguments.\n");
        base_halt(ES_FAILURE);
    }
    // Convert args to UTF-8
    for (int i = 0; i < argCount; i++)
    {
        args8[i] = _base_UTF16to8(args16[i]);
        if (args8[i] == NULL)
        {
            base_fatalErrorf("Failed to convert arguments to UTF-8.\n");
            base_halt(ES_FAILURE);
        }
    }
    // Free UTF-16 args
    LocalFree(args16);

    // Set colour to normal
    base_printf(COLOUR_NORMAL);

    // Call core entry
    es_t es = entry((size_t)argCount, (const UTF8_t *const *)args8);

    // Free manually created UTF-8 args
    for (int i = 0; i < argCount; i++)
        base_realloc(args8[i], 0);
    base_realloc(args8, 0);

    // Reset terminal colour to default
    base_printf("\e[0m");
    // Return
    return (es == ES_SUCCESS)? EXIT_SUCCESS : EXIT_FAILURE;
}



// Core "core.h"

ATTRIB_NORETURN void base_halt(es_t success)
{
    exit((success)? EXIT_SUCCESS : EXIT_FAILURE);
}



void* base_realloc(void* ptr, size_t size)
{
    if (size == 0) free(ptr);
    else return realloc(ptr, size);
    return NULL;
}

void base_memcpy(void* dst, const void* src, size_t size)
{
    memcpy(dst, src, size);
}

void base_memmove(void* dst, const void* src, size_t size)
{
    memmove(dst, src, size);
}



void _base_stdout(const UTF8_t* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void _base_stderr(const UTF8_t* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

es_t base_input(UTF8_t* pBlock, size_t maxWithNT, size_t* pWrittenWithNT)
{
    if (maxWithNT > (size_t)INT_MAX) return ES_FAILURE;
    if (fgets(pBlock, maxWithNT, stdin) != pBlock) return ES_FAILURE;
    *pWrittenWithNT = strlen(pBlock);
    if (*pWrittenWithNT == 0) return ES_FAILURE;
    return ES_SUCCESS;
}



uint64_t base_strToU64(const UTF8_t* pTextNT, UTF8_t** pPtrAfterEnd)
{
    // Check if length >= 2 (capped strlen())
    size_t len = 0;
    while (len < 2 && pTextNT[len] != (UTF8_t)0)
        len++;
    // Check base prefixes
    int radix = 10;
    if (len >= 2 && pTextNT[0] == '0')
    {
        if (pTextNT[1] == 'b') radix = 2;
        else if (pTextNT[1] == 'c') radix = 8;
        else if (pTextNT[1] == 'x') radix = 16;
    }
    return strtoull(pTextNT, pPtrAfterEnd, radix);
}

void base_sscanf(const UTF8_t* source, const UTF8_t* format, ...)
{
    va_list args;
    va_start(args, format);
    vsscanf(source, format, args);
    va_end(args);
}

void base_snprintf(UTF8_t* destination, size_t size, const UTF8_t* format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(destination, size, format, args);
    va_end(args);
}



// File IO
#include "./file.h"

handle_t base_openFile(const UTF8_t* pPath, base_OFMethod method, bool* pCreatedNewFileOptional)
{
    handle_t result;

    // Get UTF-16 path because windows
    UTF16_t* pPath16 = _base_UTF8to16(pPath);
    if (pPath16 == NULL) return HANDLE_NONE;

    // Create-conditionally method:
    //   Opens existing file if it exists, creates a new file otherwise
    // CREATE_NEW fails if a file already exists, I used it here for the extreme edge case
    //   of a file being created EXACTLY between the 2 CreateFileW calls
    if (method == BASE_OFMETHOD_CREATE_CONDITIONALLY)
    {
        result = CreateFileW(pPath16, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
        if (result != HANDLE_NONE && pCreatedNewFileOptional != NULL)
            *pCreatedNewFileOptional = false;
        else //if (result == HANDLE_NONE)
        {
            result = CreateFileW(pPath16, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL, NULL);
            if (result != HANDLE_NONE && pCreatedNewFileOptional != NULL)
                *pCreatedNewFileOptional = true;
        }
        base_realloc(pPath16, 0);
        return result;
    }

    // Other methods
    DWORD apiMethod;
    switch (method)
    {
    case BASE_OFMETHOD_OPEN_EXISTING:
        apiMethod = OPEN_EXISTING;
        break;
    case BASE_OFMETHOD_CREATE_ALWAYS:
        apiMethod = CREATE_ALWAYS;
        break;
    default:
    {
        base_fatalErrorf("Unprogrammed open-file method = %u given to %s.\n",
                        method, __func__);
        base_halt(ES_FAILURE);
    } break;
    }
    result = CreateFileW((LPCWSTR)pPath16, GENERIC_READ | GENERIC_WRITE, 0, NULL, apiMethod,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    base_realloc(pPath16, 0);
    return result;
}

es_t base_closeFile(handle_t* pHandle)
{
    handle_t copy = *pHandle;
    *pHandle = HANDLE_NONE;
    return (es_t)CloseHandle(copy);
}

es_t base_seekFile(handle_t handle, uint64_t offset)
{
    // Set file pointer to offset
    return (SetFilePointerEx((HANDLE)handle, (LARGE_INTEGER ){.QuadPart = offset}, NULL, FILE_BEGIN)
    == INVALID_SET_FILE_POINTER)?
        ES_FAILURE : ES_SUCCESS;
}

es_t base_readFile(handle_t handle, uint64_t size, void* pBlock)
{
    // Loop because ReadFile takes a DWORD for size
    while (size != 0)
    {
        // Read
        DWORD bytesRead = 0;
        if (!ReadFile((HANDLE)handle, pBlock, (uint32_t)size, &bytesRead, NULL)
        || bytesRead != (uint32_t)size)
            return ES_FAILURE;
        
        // Decrement size for next read (offset incremented automatically)
        size   -= (uint64_t)bytesRead;
    }
    
    // Return
    return ES_SUCCESS;
}

es_t base_writeFile(handle_t handle, uint64_t size, const void* pBlock)
{
    // Loop because WriteFile takes a DWORD for size
    while (size != 0)
    {
        // Write
        DWORD bytesWritten = 0;
        if (!WriteFile(handle, pBlock, (uint32_t)size, &bytesWritten, NULL)
        || bytesWritten != (uint32_t)size)
            return ES_FAILURE;
        
        // Decrement size for next write (offset incremented automatically)
        size   -= (uint64_t)bytesWritten;
    }
    
    // Return
    return ES_SUCCESS;
}

es_t base_getFileInfo(handle_t handle, uint64_t* pSize, bool* pSparseOptional)
{
    // Get file logical size
    FILE_STANDARD_INFO standardInfo = {0};
    if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &standardInfo, sizeof(standardInfo)))
        return ES_FAILURE;
    
    // Get file sparsity
    FILE_BASIC_INFO basicInfo = {0};
    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &basicInfo, sizeof(basicInfo)))
        return ES_FAILURE;

    *pSize = (uint64_t)standardInfo.EndOfFile.QuadPart;
    if (pSparseOptional)
        *pSparseOptional = ((basicInfo.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0);

    return ES_SUCCESS;
}

es_t base_resizeFile(handle_t handle, uint64_t newSize)
{
    LARGE_INTEGER apiSize = {.QuadPart = newSize};
    BOOL succeeded = SetFilePointerEx(handle, apiSize, NULL, FILE_BEGIN);
    if (succeeded)
        succeeded = SetEndOfFile(handle);
    if (!succeeded)
        return ES_FAILURE;
    return ES_SUCCESS;
}

es_t base_setFileSparsity(handle_t handle, bool sparsity)
{
    FILE_SET_SPARSE_BUFFER apiArg = {.SetSparse = (BOOLEAN)sparsity};
    if (!DeviceIoControl(handle, FSCTL_SET_SPARSE, &apiArg, sizeof(apiArg), NULL, 0, NULL, NULL))
        return ES_FAILURE;
    return ES_SUCCESS;
}

#define isSeparator(c) (c == '\\' || c == '/')

base_PathType base_getPathType(const UTF8_t* path, size_t pathSize)
{
    if (pathSize == 0) return BASE_PATHTYPE_INVALID;

    // Direct disks \\.\PhysicalDriveN or \\?\PhysicalDriveN
    const UTF8_t* phBegin = "\\\\";
    size_t sizeBegin      = strlen(phBegin);
    const UTF8_t* phEnd   = "\\PhysicalDrive";
    size_t sizeEnd        = strlen(phEnd);
    if (pathSize > (sizeBegin +1 +sizeEnd)
    && strncmp(path +0, phBegin, sizeBegin)
    && (path[sizeBegin] == '.' || path[sizeBegin] == '?')
    && strncmp(path +(sizeBegin +1), phEnd, sizeEnd))
        return BASE_PATHTYPE_DISK;

    // Volume Letter e.g. "D:" "E:/"
    if (pathSize >= 2
    && tolower(path[0]) >= 'a'
    && tolower(path[0]) <= 'z'
    && path[1] == ':')
    {
        if (pathSize == 2
        || (pathSize == 3 && isSeparator(path[2])) )
            return BASE_PATHTYPE_VOLUME;
    }

    // Get UTF-16 path
    UTF16_t* path16 = _base_UTF8to16(path);
    if (path16 == NULL) return BASE_PATHTYPE_INVALID;

    // Check attributes
    DWORD attribs = GetFileAttributesW(path16);
    if (attribs != INVALID_FILE_ATTRIBUTES)
    {
        // Reparse point e.g. mount point or junction
        if (attribs & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            UTF16_t volumeName[1];
            if (GetVolumeNameForVolumeMountPointW(path16, volumeName, 1))
            {
                base_realloc(path16, 0);
                return BASE_PATHTYPE_VOLUME;
            }
        }
        // Directory
        else if (attribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            base_realloc(path16, 0);
            return BASE_PATHTYPE_DIR;
        }
        // File
        else
        {
            base_realloc(path16, 0);
            return BASE_PATHTYPE_FILE;
        }
    }

    // Check if path is a possibly creatable file
    if (pathSize > 2)
    {
        // Get string length
        size_t path16Size = 0;
        while (path16[path16Size] != (UTF16_t)0) path16Size++;
        // Remove (null terminate) the file name in the path, if it exists
        // Loop starts at the character before the last (e.g. the slash in "/f")
        for (size_t i = ((path16Size -1) -1); i != (size_t)-1; i--)
        {
            if (isSeparator(path16[i]))
            {
                path16[i] = (UTF16_t)0;
                break;
            }
        }
        // Check if last directory in path exists
        attribs = GetFileAttributesW(path16);
        if (attribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            base_realloc(path16, 0);
            return BASE_PATHTYPE_CREATABLE_FILE;
        }
    }

    // Inexistent/invalid path
    base_realloc(path16, 0);
    return BASE_PATHTYPE_INVALID;
}



// Time
#include "./time.h"

es_t base_time(base_Time* pTime)
{
    // Get the current calendar time
    time_t now = time(NULL);
    if (now == (time_t)(-1)) return ES_FAILURE;
    
    // Convert to local time
    struct tm* localTime = localtime(&now);
    if (localTime == NULL) return ES_FAILURE;

    pTime->monthOfYear = (uint8_t)(localTime->tm_mon +1);
    pTime->dayOfMonth  = (uint8_t)(localTime->tm_mday);
    pTime->hourOfDay   = (uint8_t)(localTime->tm_hour);
    return ES_SUCCESS;
}



// Random
#include "./rand.h"

uint32_t base_badRand()
{
    // srand() must be called by base_init()
    return (uint32_t)rand();
}

es_t base_randUuidv4(void* pData128)
{
    // UUIDv4 random bits
    GUID apiGUID;
    HRESULT hr = CoCreateGuid(&apiGUID);
    if (SUCCEEDED(hr))
    {
        memcpy(pData128, &apiGUID, 16);
        return ES_SUCCESS;
    }
    else return ES_FAILURE;
}
