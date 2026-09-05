#include <windows.h>
#include <shellapi.h> // shell32.lib linking required
#include <cstdint>
#include <cstring>
#include <clocale>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "../algos/utils.hpp"
#include "../algos/ssutils.hpp"

namespace Utils
{
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

    /*
    struct WriteImgInfo
    {
        const uint8_t* pBuffer;
        size_t bufferSize;
        size_t fileWriteOffset;
        HugeFileWriteInfo(const uint8_t* pBuffer, size_t bufferSize, size_t fileWriteOffset)
            : pBuffer(pBuffer), bufferSize(bufferSize), fileWriteOffset(fileWriteOffset)
        {}
    };

    ErrorState createImg(const char* path, size_t totalSize,
                            const std::vector<HugeFileWriteInfo>& writeInfos)
    {
        // Get UTF-16 path
        std::pair<std::u16string, ErrorState> pathResult = utf8to16(path);
        if (pathResult.second != ErrorState::Success) return ErrorState::Failure;
        std::u16string u16path = pathResult.first;

        // Open file
        // 0 = share mode (some async bullshit 0 means no one else can use before the file is closed)
        // nullptr = security attributes (related to other processes managing the file. no need)
        // CREATE_ALWAYS = creation disposition (create new file even if file exists)
        // FILE_ATTRIBUTE_NORMAL = 
        HANDLE hFile = CreateFileW((WCHAR*)(u16path.c_str()), GENERIC_WRITE, 0, nullptr,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return ErrorState::Failure;

        // Set total file size
        LARGE_INTEGER size;
        size.QuadPart = static_cast<ULONGLONG>(totalSize);
        
        if (!SetFilePointerEx(hFile, size, NULL, FILE_BEGIN) || !SetEndOfFile(hFile))
        {
            CloseHandle(hFile);
            return false;
        }

        // 4. Iterate over vector chunks and write at requested offsets
        for (const auto& info : writeInfos)
        {
            if (!info.pBuffer || info.bufferSize == 0)
            {
                continue;
            }

            LARGE_INTEGER pos;
            pos.QuadPart = static_cast<ULONGLONG>(info.fileWriteOffset);

            if (!SetFilePointerEx(hFile, pos, NULL, FILE_BEGIN))
            {
                CloseHandle(hFile);
                return false;
            }

            DWORD bytesWritten = 0;
            if (!WriteFile(hFile, info.pBuffer, static_cast<DWORD>(info.bufferSize), &bytesWritten, NULL) ||
                bytesWritten != info.bufferSize)
            {
                CloseHandle(hFile);
                return false;
            }
        }

        CloseHandle(hFile);
        return true;
    }
    */



    /*
    std::pair<PathType, ErrorState> getPathType(std::string_view pathView)
    {
        if (pathView.empty()) return {PathType::None, ErrorState::Success};

        // Check direct disk normal or extended path
        if (pathView.size() >= 16
        && (!strncmp(pathView.data(), R"(\\.\PhysicalDrive)", 16)
        || !strncmp(pathView.data(), R"(\\?\PhysicalDrive)", 16)))
            return {PathType::DiskDirect, ErrorState::Success};

        //volume path types? i guess
        //Volume letter "C"
        //Volume letter path "C:\"
        //GUID volume path "\\?\Volume{GUID}\" or "\\.\Volume{GUID}\"
        //Volume letter path with stuff "\\.\C" or "\\?\C"
        //a "mount point" (like "D:\someDisk\"):
        //    Reparse Point: Use GetFileAttributes.
        //    If the directory has the FILE_ATTRIBUTE_REPARSE_POINT flag,
        //    it is either a symbolic link or a mount point.

        //(ENTERPRISE use case): AI says: However, it isn't a strict 1:1 rule.
        //    With technologies like Logical Volume Management (LVM) on Linux
        //    or Dynamic Disks on Windows,
        //    you can combine multiple partitions from different physical disks
        //    into one single, continuous volume.
        //    In those cases, one volume is actually composed of several partitions.
        
        //AI says: DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, ...)
        //    This will return the disk and partition info for whatever volume that path points to.

        //AI says: Use IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS instead.
        //    It returns array-based extents mapping out every physical disk
        //    and sector offset composing that volume.
        
        // Check volume paths: "\\.\C:" or "\\.\Volume{GUID}"
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

        // Get UTF-16 path
        std::pair<std::u16string, ErrorState> pathResult = utf8to16(pathView);
        if (pathResult.second != ErrorState::Success) return {0, ErrorState::Failure};
        std::u16string path16 = pathResult.first;

        // 4. Query standard File System Attributes without opening a handle
        DWORD attribs = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path16.c_str()));

        if (attribs == INVALID_FILE_ATTRIBUTES)
        {
            // File / Directory does not exist, path is invalid, or access is blocked
            return PathType::None;
        }

        // 5. Check if path points to a Directory or a Regular File
        if (attribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            return PathType::Dir;
        }

        return PathType::File;
    }

    /*
    #include <string_view>
    #include <utility>
    #include <windows.h>

    // Helper to check prefix cleanly
    constexpr bool startsWith(std::string_view sv, std::string_view prefix) {
        return sv.size() >= prefix.size() && sv.compare(0, prefix.size(), prefix) == 0;
    }

    std::pair<PathType, ErrorState> getPathType(std::string_view pathView)
    {
        if (pathView.empty()) {
            return {PathType::None, ErrorState::Success};
        }

        // 1. Check for Direct Physical Disk Path (e.g., "\\.\PhysicalDrive0" or "\\?\PhysicalDrive1")
        if (startsWith(pathView, R"(\\.\PhysicalDrive)") || startsWith(pathView, R"(\\?\PhysicalDrive)")) {
            return {PathType::DiskDirect, ErrorState::Success};
        }

        // 2. Check bare volume letters (e.g., "C" or "C:")
        if ((pathView.length() == 1 && std::isalpha(static_cast<unsigned char>(pathView[0]))) ||
            (pathView.length() == 2 && std::isalpha(static_cast<unsigned char>(pathView[0])) && pathView[1] == ':')) {
            return {PathType::DiskVolume, ErrorState::Success};
        }

        // 3. Check device volume prefixes ("\\.\" or "\\?\")
        if (startsWith(pathView, R"(\\.\)") || startsWith(pathView, R"(\\?\)" )) {
            // Matches DOS device drive letters like "\\.\C:" or "\\?\D:"
            if (pathView.length() == 6 && std::isalpha(static_cast<unsigned char>(pathView[4])) && pathView[5] == ':') {
                return {PathType::DiskVolume, ErrorState::Success};
            }

            // Matches Volume GUID paths like "\\?\Volume{GUID}\" or "\\.\Volume{GUID}"
            if (pathView.find(R"(Volume{)") != std::string_view::npos) {
                return {PathType::DiskVolume, ErrorState::Success};
            }
        }

        // 4. Convert UTF-8 to UTF-16 for Windows File System API queries
        auto [path16, convertError] = utf8to16(pathView);
        if (convertError != ErrorState::Success) {
            return {PathType::None, convertError};
        }

        // 5. Query File System Attributes without opening a handle
        DWORD attribs = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path16.c_str()));

        if (attribs == INVALID_FILE_ATTRIBUTES) {
            // Path does not exist or access is denied
            return {PathType::None, ErrorState::Failure};
        }

        // 6. Check Directories and Mount Points
        if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
            // If it has a reparse point flag, it's either a directory mount point or a symlink
            if (attribs & FILE_ATTRIBUTE_REPARSE_POINT) {
                return {PathType::MountPoint, ErrorState::Success}; // Or PathType::DiskVolume depending on your enum
            }
            return {PathType::Dir, ErrorState::Success};
        }

        // 7. Regular File
        return {PathType::File, ErrorState::Success};
    }
    */
} // namespace Utils
