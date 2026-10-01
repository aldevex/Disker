#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <objbase.h> // YOU GOTTA LINK "ole32.dll"
#include "../utils/utils.h"
#include "../disk/info.h"

const utils_SysHandle UTILS_SYSHANDLE_NONE = (utils_SysHandle)INVALID_HANDLE_VALUE;

utils_ErrorState openLockReadDisk(DiskInfo* pDiskInfo, String8* pPath, bool* pCreatedNewFile)
{
utils_ErrorState result = UTILS_ERRORSTATE_FAILURE;
    //
    //
    // ALL OF THIS ASSUMES RAW IMAGE
    //
    //

    // Get UTF-16 path
    String16 path16 = {0};
    bool validText = string16CopyS8(&path16, pPath);
    if (!validText)
    {
        fprintf(stderr, "failed to open invalid path \"%s\"\n", string8NT(pPath));
        // shouldn't use fret because hFile isn't initialized yet
        string16Free(&path16);
        return UTILS_ERRORSTATE_FAILURE;
    }

    // Open existing file
    LARGE_INTEGER fileSize = {0};
    HANDLE hFile = CreateFileW(string16Data(&path16), GENERIC_READ|GENERIC_WRITE, 0, 
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        *pCreatedNewFile = false;
        // Get file size
        if (!GetFileSizeEx(hFile, &fileSize))
        {
            fprintf(stderr, "failed to get file size for \"%s\"\n", string8NT(pPath));
            fret(UTILS_ERRORSTATE_FAILURE);
        }
    }
    else
    {
        // Create new file on failure
        hFile = CreateFileW(string16Data(&path16), GENERIC_READ|GENERIC_WRITE, 0, 
                                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            *pCreatedNewFile = true;
            // Try to make the file a sparse file if newly created
            DeviceIoControl(hFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, NULL, NULL);
        }
        else
        {
            fprintf(stderr, "failed to open and to create file \"%s\"\n", string8NT(pPath));
            fret(UTILS_ERRORSTATE_FAILURE);
        }
    }
    
    // Read file into disk info
    if (!*pCreatedNewFile)
    {
        // char readBuffer[1024];
        // DWORD bytesRead = 0;
        // if (ReadFile(hFile, readBuffer, sizeof(readBuffer) - 1, &bytesRead, NULL))
        // {
        //     readBuffer[bytesRead] = '\0';
        //     printf("Current content: %s\n", readBuffer);
        // } else
        // {
        //     fprintf(stderr, "Read failed. Error: %lu\n", GetLastError());
        //     CloseHandle(hFile);
        //     return;
        // }
    }
    // Set default values for newly created file
    else
    {
        // pDiskInfo.
    }
    pDiskInfo->handle = (utils_SysHandle)hFile;
    pDiskInfo->path = *pPath;
    pDiskInfo->geometry.type = GEO_TYPE_RAW_IMAGE;
    pDiskInfo->geometry.data.raw = raw_dataMake(fileSize.QuadPart, 512, 512,
                                        utils_strToSizeVw(&vw("1MiB"), true, true, true));
    pDiskInfo->scheme.type = SCHEME_TYPE_MBR;

    // Reset the file pointer to the beginning to RW later
    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        fprintf(stderr, "failed to reset file pointer for \"%s\"\n", string8NT(pPath));
        fret(UTILS_ERRORSTATE_FAILURE);
    }
    
    fret(UTILS_ERRORSTATE_SUCCESS);

end:
    if (string8Data(&pDiskInfo->path) == string8Data(pPath))
        *pPath = (String8){0};
    string16Free(&path16);
    CloseHandle(hFile);
    return result;
}

utils_ErrorState closeImage(DiskInfo* pDiskInfo)
{
    return UTILS_ERRORSTATE_FAILURE;
    // CloseHandle(()pDiskInfo->handle);
}




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



// std::pair<DiskInfo, Utils::ErrorState> readImage(const char* path)
// {
//     DiskInfo result = DiskInfo();
//     // Ensure that path isn't a real disk (return failure if real disk)
//     if (getPathType(path) != PathType::File) return result;
//     // Read file
//     std::pair<std::vector<uint8_t>, ErrorState> readResult = readFile(path);
//     // Failure or tiny file
//     if (readResult.second != ErrorState::Success
//     || readResult.first.size() < 512)
//         return result;
//     // Copy data into structures
//     const std::vector<uint8_t>& buffer = readResult.first;
//     result.disk = Disk::Disk(path, false, buffer.size(), 512, 512,
//                                 strToSize("1MiB", true, true, true));
//     result.scheme = Disk::Scheme::MBR;
//     result.mbrData = MBR::MBRData(buffer.data());
//     result.errorState = ErrorState::Success;
//     // Return result
//     return result;
// }

// ErrorState writeImage(const char* path, uint64_t fileSize,
//                         const std::vector<WriteImageInfo>& writeInfos)
// {
//     // Ensure that path isn't a real disk (return failure if real disk)
//     if (getPathType(path) != PathType::File && getPathType(path) != PathType::None)
//         return ErrorState::Failure;

//     // Get UTF-16 path
//     std::pair<std::u16string, ErrorState> pathResult = utf8to16(path);
//     if (pathResult.second != ErrorState::Success) return ErrorState::Failure;
//     const WCHAR* path16 = (WCHAR*)pathResult.first.c_str();

//     // Create file
//     // Generic RW no special file share mode
//     // Null no security attributes
//     // Create new file even if one already exists
//     // Null template file for attributes and stuff
//     HANDLE hFile = CreateFileW(path16, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
//                                 FILE_ATTRIBUTE_NORMAL, nullptr);
//     if (hFile == INVALID_HANDLE_VALUE) return Utils::ErrorState::Failure;

//     // Try to make the file a sparse file (zero tag instead of actual zeroes in disk)
//     // File handle, driver operation,
//     // Null and 0 twice are input and output buffers and their sizes
//     // Null and null are output data size and OVERLAPPED structure pointer
//     bool isSparse = DeviceIoControl(hFile, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, nullptr, nullptr);
    
//     // Set total file size
//     // Ignoring whether the file became a parse file or not (gotta write it either way)
//     // Win API should fill with zeroes even if not sparse for data security
//     LARGE_INTEGER apiSize;
//     apiSize.QuadPart = fileSize;
//     // Null is pointer to the resulting file-pointer
//     if (!SetFilePointerEx(hFile, apiSize, nullptr, FILE_BEGIN) || !SetEndOfFile(hFile))
//     {
//         CloseHandle(hFile);
//         return Utils::ErrorState::Failure;
//     }

//     // Write segments
//     for (const auto& info : writeInfos)
//     {
//         if (info.pBuffer == nullptr || info.bufferSize == 0)  continue;

//         // Set offset
//         LARGE_INTEGER apiOffset;
//         apiOffset.QuadPart = info.fileWriteOffset;
//         // Null is pointer to the resulting file-pointer
//         if (!SetFilePointerEx(hFile, apiOffset, nullptr, FILE_BEGIN))
//         {
//             CloseHandle(hFile);
//             return Utils::ErrorState::Failure;
//         }

//         // Write
//         // Null is OVERLAPPED structure pointer
//         DWORD bytesWritten = 0;
//         if (!WriteFile(hFile, info.pBuffer, (DWORD)info.bufferSize, &bytesWritten, nullptr)
//         || bytesWritten != info.bufferSize)
//         {
//             CloseHandle(hFile);
//             return Utils::ErrorState::Failure;
//         }
//     }

//     // Return
//     CloseHandle(hFile);
//     return ErrorState::Success;
// }



// Implemented in system specific implementation files
// utils_PathType utils_getPathType(const View8* pPathView);
