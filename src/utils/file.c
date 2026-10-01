#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "./file.h"

#define isSeparator(c) (c == '\\' || c == '/')

#ifdef _WIN32
utils_PathType utils_getPathType(const View8* pPath)
{
    if (view8Empty(pPath)) return UTILS_PATHTYPE_INVALID;

    // Get UTF-16 path
    String16 path16 = {0};
    bool valid = string16CopyVw8(&path16, pPath);
    if (!valid) return UTILS_PATHTYPE_INVALID;

    // Volume Letter e.g. "D:" "E:/"
    if (view8Size(pPath) >= 2
    && tolower(at(pPath, 0)) >= 'a'
    && tolower(at(pPath, 0)) <= 'z'
    && at(pPath, 1) == ':')
    {
        if (view8Size(pPath) == 2)
        {
            return UTILS_PATHTYPE_PART;
        }
        else if (view8Size(pPath) == 3 && isSeparator(at(pPath, 2)))
        {
            return UTILS_PATHTYPE_PART;
        }
    }

    // Direct disks \\.\PhysicalDriveN or \\?\PhysicalDriveN
    if (view8StartsWithNT(pPath, "\\\\.\\PhysicalDrive")
    || view8StartsWithNT(pPath, "\\\\?\\PhysicalDrive"))
        return UTILS_PATHTYPE_DISK;

    // Get attributes
    DWORD attribs = GetFileAttributesW(string16NT(&path16));

    if (attribs != INVALID_FILE_ATTRIBUTES)
    {
        // Reparse point e.g. mount point or junction
        if (attribs & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            UTF16_t volumeName[1];
            if (GetVolumeNameForVolumeMountPointW(string16NT(&path16), volumeName, 1))
            {
                string16Free(&path16);
                return UTILS_PATHTYPE_PART;
            }
        }
        else if (attribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            string16Free(&path16);
            return UTILS_PATHTYPE_DIR;
        }
        else
        {
            string16Free(&path16);
            return UTILS_PATHTYPE_FILE;
        }
    }

    // Inexistent path
    return UTILS_PATHTYPE_MAYBE_CREATABLE;
}
#else
    #error "unimplemented"
#endif

utils_FileType utils_getFileType(const View8* pPath)
{
    // Linear search macro
    #define check(retVal, ...) do {\
        static const UTF8_t* ARR_##retVal[] = {__VA_ARGS__};\
        for (size_t i = 0; i < lenof(ARR_##retVal); i++)\
            if (utils_compareLowNT(&ext, ARR_##retVal[i]))\
                return retVal;\
        } while (0)
    // Check if extension exists
    size_t extI = view8RevFindNT(pPath, ".");
    size_t fwSlashI = view8RevFindNT(pPath, "/");
    size_t bwSlashI = view8RevFindNT(pPath, "\\");
    if (extI == STRING_NF
    || (fwSlashI != STRING_NF && fwSlashI > extI)
    || (bwSlashI != STRING_NF && bwSlashI > extI))
        return UTILS_FILETYPE_NOEXT;
    // Get extension
    View8 ext = view8SubStr(pPath, extI, view8Size(pPath) -extI);
    // Check registered extension lists
    check(UTILS_FILETYPE_RAW, "bin", "img", "raw");
    // Unknown extension
    return UTILS_FILETYPE_UNKNOWN;
    // Undefine search macro
    #undef check
}

uint64_t utils_getFileSize(const View8* pPath)
{
    // Get UTF-16 path
    String16 path16 = {0};
    bool validText = string16CopyVw8(&path16, pPath);
    if (!validText)
    {
        string16Free(&path16);
        return UTILS_SIZESIG_NO_NUMBER;
    }

    // Get file metadata
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (!GetFileAttributesExW((WCHAR*)string16NT(&path16), GetFileExInfoStandard, &fileData))
    {
        string16Free(&path16);
        fprintf(stderr, "failed to get file size for \"%s\"\n", view8NT(pPath));
        return UTILS_SIZESIG_NO_NUMBER;
    }

    // Combine 32-bit segments into a 64-bit size
    ULARGE_INTEGER fileSize;
    fileSize.LowPart = fileData.nFileSizeLow;
    fileSize.HighPart = fileData.nFileSizeHigh;

    // Return result
    string16Free(&path16);
    return fileSize.QuadPart;
}



utils_SysHandle utils_openFile(const View8* pPath, bool* pCreatedNewFile)
{
utils_SysHandle result = UTILS_SYSHANDLE_NONE;

    // Get UTF-16 path
    String16 path16 = {0};
    bool validText = string16CopyVw8(&path16, pPath);
    if (!validText)
    {
        fprintf(stderr, "failed to open invalid path \"%s\"\n", view8NT(pPath));
        fret(UTILS_SYSHANDLE_NONE);
    }

    // Open existing file
    HANDLE hFile = CreateFileW(string16NT(&path16), GENERIC_READ|GENERIC_WRITE, 0, 
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        if (pCreatedNewFile == NULL || pCreatedNewFile == (bool*)false)
        {
            fprintf(stderr, "failed to open \"%s\"\n", view8NT(pPath));
            fret(UTILS_SYSHANDLE_NONE);
        }
        // Can create a new file (pCreatedNewFile isn't NULL/false)
        else
        {
            hFile = CreateFileW(string16NT(&path16), GENERIC_READ|GENERIC_WRITE, 0, 
                                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                fprintf(stderr, "failed to open and to create file \"%s\"\n", view8NT(pPath));
                fret(UTILS_SYSHANDLE_NONE);
            }
            else
            {
                *pCreatedNewFile = true;
                // Try to make the file a sparse file if newly created
                DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, NULL, NULL);
            }
        }
    }
    fret((utils_SysHandle)hFile);

end:
    string16Free(&path16);
    return result;
}

utils_ErrorState utils_closeFile(const View8* pPath, utils_SysHandle* pHandle)
{
    if (*pHandle == INVALID_HANDLE_VALUE) return UTILS_ERRORSTATE_FAILURE;
    if (!CloseHandle(*pHandle))
    {
        fprintf(stderr, "failed to close \"%s\"\n", view8NT(pPath));
        return UTILS_ERRORSTATE_FAILURE;
    }
    else
    {
        *pHandle = UTILS_SYSHANDLE_NONE;
        return UTILS_ERRORSTATE_SUCCESS;
    }
}

utils_ErrorState utils_readFile(const View8* pPath, utils_SysHandle handle,
                                uint64_t offset, uint64_t count, void** ppBuffer)
{
    bool ownsBuffer = false;
    uint8_t* pBuffer = *ppBuffer;
    // Allocate buffer if null
    if (pBuffer == NULL)
    {
        pBuffer = malloc(count);
        if (pBuffer == NULL)
        {
            fprintf(stderr, "failed to allocate buffer to read \"%s\"\n", view8NT(pPath));
            exit(EXIT_FAILURE);
        }
        ownsBuffer = true;
    }

    // Sset file pointer to offset
    if (SetFilePointerEx((HANDLE)handle, (LARGE_INTEGER ){.QuadPart = offset}, NULL, FILE_BEGIN)
    == INVALID_SET_FILE_POINTER)
    {
        fprintf(stderr, "failed to set file pointer for \"%s\"\n", view8NT(pPath));
        if (ownsBuffer) free(pBuffer);
        return UTILS_ERRORSTATE_FAILURE;
    }

    DWORD bytesRead = 0;
    if (!ReadFile((HANDLE)handle, pBuffer, count, &bytesRead, NULL))
    {
        fprintf(stderr, "failed to read file \"%s\"\n", view8NT(pPath));
        if (ownsBuffer) free(pBuffer);
        return UTILS_ERRORSTATE_FAILURE;
    }

    // Reset the file pointer
    if (SetFilePointerEx((HANDLE)handle, (LARGE_INTEGER ){.QuadPart = 0}, NULL, FILE_BEGIN)
    == INVALID_SET_FILE_POINTER)
    {
        fprintf(stderr, "failed to reset file pointer for \"%s\"\n", view8NT(pPath));
        if (ownsBuffer) free(pBuffer);
        return UTILS_ERRORSTATE_FAILURE;
    }

    if (ownsBuffer) *ppBuffer = pBuffer;
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState utils_writeFile(const View8* pPath, utils_SysHandle handle,
                                        uint64_t offset, uint64_t count, const void* pBuffer)
{
    // Sset file pointer to offset
    if (SetFilePointerEx((HANDLE)handle, (LARGE_INTEGER ){.QuadPart = offset}, NULL, FILE_BEGIN)
    == INVALID_SET_FILE_POINTER)
    {
        fprintf(stderr, "failed to set file pointer for \"%s\"\n", view8NT(pPath));
        return UTILS_ERRORSTATE_FAILURE;
    }


    DWORD bytesWritten = 0;
    if (!WriteFile(handle, pBuffer, (DWORD)count, &bytesWritten, NULL)
    || (uint64_t)bytesWritten != count)
    {
        fprintf(stderr, "failed to write to file \"%s\"\n", view8NT(pPath));
        return UTILS_ERRORSTATE_FAILURE;
    }

    // Reset the file pointer
    if (SetFilePointerEx((HANDLE)handle, (LARGE_INTEGER ){.QuadPart = 0}, NULL, FILE_BEGIN)
    == INVALID_SET_FILE_POINTER)
    {
        fprintf(stderr, "failed to reset file pointer for \"%s\"\n", view8NT(pPath));
        return UTILS_ERRORSTATE_FAILURE;
    }

    return UTILS_ERRORSTATE_SUCCESS;
}
