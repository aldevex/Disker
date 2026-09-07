#include <windows.h>
#include <shellapi.h> // shell32.lib linking required
#include <cstdint>
#include <cstring>
#include <clocale>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "../utils/utils.hpp"
#include "../utils/ssutils.hpp"
namespace Utils {

std::pair<std::string, ErrorState> utf16to8(const std::u16string_view inputView)
{
    // Args:
    // CP_UTF8: UTF-8 codepage | WC_ERR_INVALID_CHARS: error on invalid character
    // inputView: input string pointer and length
    // nullptr: output buffer pointer | (0): output buffer size (requesting size from function)
    // nullptr: pointer to invalid-character replacement-character
    // nullptr: IDFK but for UTF-8 it must be NULL so I don't care
    int expectedSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                        (WCHAR*)inputView.data(), (int)inputView.length(),
                                        (CHAR*)nullptr, (int)0,
                                        nullptr, nullptr);
    if (expectedSize == 0) return {"", ErrorState::Failure};

    std::string result(expectedSize, '\0');
    int outputSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                        (WCHAR*)inputView.data(), (int)inputView.length(),
                                        (CHAR*)result.data(), (int)expectedSize,
                                        nullptr, nullptr);
    if (outputSize != expectedSize) return {"", ErrorState::Failure};

    return {result, ErrorState::Success};
}

std::pair<std::u16string, ErrorState> utf8to16(const std::string_view inputView)
{
    // Args:
    // CP_UTF8: UTF-8 codepage | MB_ERR_INVALID_CHARS: error on invalid character
    // inputView: input string pointer and length
    // nullptr: output buffer pointer | (0): output buffer size (requesting size from function)
    // nullptr: pointer to invalid-character replacement-character
    // nullptr: IDFK but for UTF-8 it must be NULL so I don't care
    int expectedSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        (CHAR*)inputView.data(), (int)inputView.length(),
                                        (WCHAR*)nullptr, (int)0);
    if (expectedSize == 0) return {u"", ErrorState::Failure};

    std::u16string result(expectedSize, '\0');
    int outputSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        (CHAR*)inputView.data(), (int)inputView.length(),
                                        (WCHAR*)result.data(), (int)expectedSize);
    if (outputSize != expectedSize) return {u"", ErrorState::Failure};

    return {result, ErrorState::Success};
}

std::pair<uint64_t, ErrorState> getFileSize(const char* path)
{
    // Get UTF-16 path
    std::pair<std::u16string, ErrorState> pathResult = utf8to16(path);
    if (pathResult.second != ErrorState::Success) return {0, ErrorState::Failure};
    std::u16string path16 = pathResult.first;

    // Get file metadata
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (!GetFileAttributesExW((WCHAR*)(path16.c_str()), GetFileExInfoStandard, &fileData))
        return {0, ErrorState::Failure};

    // Combine 32-bit segments into a 64-bit size
    ULARGE_INTEGER fileSize;
    fileSize.LowPart = fileData.nFileSizeLow;
    fileSize.HighPart = fileData.nFileSizeHigh;

    // Return result
    return {fileSize.QuadPart, ErrorState::Success};
}

PathType getPathType(std::string_view pathView)
{
    if (pathView.empty()) return PathType::Invalid;

    // Check direct disk normal or extended path
    if (pathView.size() >= 16
    && (!strncmp(pathView.data(), R"(\\.\PhysicalDrive)", 16)
    || !strncmp(pathView.data(), R"(\\?\PhysicalDrive)", 16)))
        return PathType::DiskDirect;

    //volume path types? i guess
    //Volume letter "C"
    //Volume letter path "C:\"
    //GUID volume path "\\?\Volume{GUID}\" or "\\.\Volume{GUID}\"
    //Volume letter path with stuff "\\.\C" or "\\?\C"
    //a "mount point" (like "D:\someDisk\") (probably enterprise only use case):
    //    Reparse Point: Use GetFileAttributes.
    //    If the directory has the FILE_ATTRIBUTE_REPARSE_POINT flag,
    //    it is either a symbolic link or a mount point.
    // Check volume paths: "\\.\C:" or "\\.\Volume{GUID}"
    /*
    if (pathView.rfind(R"(\\.\)", 0) == 0 || pathView.rfind(R"(\\?\)", 0) == 0)
    {
        // Matches volume drive letters like "\\.\C:" or "\\.\D:"
        if (pathView.length() == 6 && pathView[5] == ':')
        {
            return PathType::DiskVolume; // MAKE IT USE THE PAIR IDFK JUST FIX THIS MESS
        }

        // Matches Volume GUID paths like "\\.\Volume{...}"
        if (pathView.find(R"(Volume{)") != std::string_view::npos)
        {
            return PathType::DiskVolume;
        }
    }
    */

    // Get UTF-16 path
    std::pair<std::u16string, ErrorState> pathResult = utf8to16(pathView);
    if (pathResult.second != ErrorState::Success) return PathType::Invalid;
    const WCHAR* path16 = (WCHAR*)pathResult.first.c_str();

    // Query standard File System Attributes without opening a handle
    DWORD attribs = GetFileAttributesW(path16);
    // Path doesn't exist or is invalid or access is blocked
    if (attribs == INVALID_FILE_ATTRIBUTES)
        return PathType::None;

    // Directory or file path
    if (attribs & FILE_ATTRIBUTE_DIRECTORY)
        return PathType::Dir;
    else
        return PathType::File;
}

// I have to implement it in ssutils to avoid circular dependancy lol
ReadImageInfo readImage(const char* path)
{
    ReadImageInfo result = ReadImageInfo();
    // Ensure that path isn't a real disk (return failure if real disk)
    if (getPathType(path) != PathType::File) return result;
    // Read file
    std::pair<std::vector<uint8_t>, ErrorState> readResult = readFile(path);
    // Failure or tiny file
    if (readResult.second != ErrorState::Success
    || readResult.first.size() < 512)
        return result;
    // Copy data into structures
    const std::vector<uint8_t>& buffer = readResult.first;
    result.disk = Disk::Disk(path, false, buffer.size(), 512, 512,
                                strToSize("1MiB", true, true, true));
    result.scheme = Disk::Scheme::MBR;
    result.mbrData = MBR::MBRData(buffer.data());
    result.errorState = ErrorState::Success;
    // Return result
    return result;
}

ErrorState writeImage(const char* path, uint64_t fileSize,
                        const std::vector<WriteImageInfo>& writeInfos)
{
    // Ensure that path isn't a real disk (return failure if real disk)
    if (getPathType(path) != PathType::File && getPathType(path) != PathType::None)
        return ErrorState::Failure;

    // Get UTF-16 path
    std::pair<std::u16string, ErrorState> pathResult = utf8to16(path);
    if (pathResult.second != ErrorState::Success) return ErrorState::Failure;
    const WCHAR* path16 = (WCHAR*)pathResult.first.c_str();

    // Create file
    // Generic RW no special file share mode
    // Null no security attributes
    // Create new file even if one already exists
    // Null template file for attributes and stuff
    HANDLE hFile = CreateFileW(path16, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return Utils::ErrorState::Failure;

    // Try to make the file a sparse file (zero tag instead of actual zeroes in disk)
    // File handle, driver operation,
    // Null and 0 twice are input and output buffers and their sizes
    // Null and null are output data size and OVERLAPPED structure pointer
    bool isSparse = DeviceIoControl(hFile, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, nullptr, nullptr);
    
    // Set total file size
    // Ignoring whether the file became a parse file or not (gotta write it either way)
    // Win API should fill with zeroes even if not sparse for data security
    LARGE_INTEGER apiSize;
    apiSize.QuadPart = fileSize;
    // Null is pointer to the resulting file-pointer
    if (!SetFilePointerEx(hFile, apiSize, nullptr, FILE_BEGIN) || !SetEndOfFile(hFile))
    {
        CloseHandle(hFile);
        return Utils::ErrorState::Failure;
    }

    // Write segments
    for (const auto& info : writeInfos)
    {
        if (info.pBuffer == nullptr || info.bufferSize == 0)  continue;

        // Set offset
        LARGE_INTEGER apiOffset;
        apiOffset.QuadPart = info.fileWriteOffset;
        // Null is pointer to the resulting file-pointer
        if (!SetFilePointerEx(hFile, apiOffset, nullptr, FILE_BEGIN))
        {
            CloseHandle(hFile);
            return Utils::ErrorState::Failure;
        }

        // Write
        // Null is OVERLAPPED structure pointer
        DWORD bytesWritten = 0;
        if (!WriteFile(hFile, info.pBuffer, (DWORD)info.bufferSize, &bytesWritten, nullptr)
        || bytesWritten != info.bufferSize)
        {
            CloseHandle(hFile);
            return Utils::ErrorState::Failure;
        }
    }

    // Return
    CloseHandle(hFile);
    return ErrorState::Success;
}

}
