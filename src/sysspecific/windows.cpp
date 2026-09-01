#include <windows.h>
#include <shellapi.h> // shell32.lib linking required
#include <clocale>
#include <iostream>
#include <string>
#include <vector>
#include "../algos/utils.hpp"
#include "../algos/ssutils.hpp"

namespace SSUtils
{
    std::pair<std::string, Utils::ErrorState> utf16to8(const std::u16string_view inputView)
    {
        // Args:
        // CP_UTF8: UTF-8 codepage | WC_ERR_INVALID_CHARS: error on invalid character
        // inputView: input string pointer and length
        // nullptr: output buffer pointer | (0): output buffer size (requesting size from function)
        // nullptr: pointer to invalid-character replacement-character
        // nullptr: IDFK but for UTF-8 it must be NULL so I don't care
        int expectedSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                            (WCHAR*)inputView.data(), (int)inputView.length(),
                                            nullptr, 0, nullptr, nullptr);
        if (expectedSize == 0) return {"", Utils::ErrorState::Failure};

        std::string result(expectedSize, '\0');
        int outputSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                            (WCHAR*)inputView.data(), (int)inputView.length(),
                                            result.data(), outputSize, nullptr, nullptr);
        if (outputSize != expectedSize) return {"", Utils::ErrorState::Failure};

        return {result, Utils::ErrorState::Success};
    }

    std::pair<std::u16string, Utils::ErrorState> utf8to16(const std::string_view inputView)
    {
        // Args:
        // CP_UTF8: UTF-8 codepage | MB_ERR_INVALID_CHARS: error on invalid character
        // inputView: input string pointer and length
        // nullptr: output buffer pointer | (0): output buffer size (requesting size from function)
        // nullptr: pointer to invalid-character replacement-character
        // nullptr: IDFK but for UTF-8 it must be NULL so I don't care
        int expectedSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            inputView.data(), (int)inputView.length(),
                                            (WCHAR*)nullptr, 0);
        if (expectedSize == 0) return {u"", Utils::ErrorState::Failure};

        std::u16string result(expectedSize, '\0');
        int outputSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            inputView.data(), (int)inputView.length(),
                                            (WCHAR*)result.data(), outputSize);
        if (outputSize != expectedSize) return {u"", Utils::ErrorState::Failure};

        return {result, Utils::ErrorState::Success};
    }


    /*
    
    std::pair<PathType, std::string> getPathInfo(std::string_view pathView, PathType expectedType)
    {
        
    }

    #include <windows.h>
#include <winioctl.h>
#include <iostream>
#include <string>
#include <vector>

struct DiskInfo {
    std::wstring volume_path;      // e.g., \\.\C:
    std::wstring physical_disk_path; // e.g., \\.\PhysicalDrive1
    unsigned int device_number = 0;
    unsigned int partition_number = 0;
    bool success = false;
};

// Takes a drive letter (C:) or a volume path (\\.\C:)
DiskInfo get_disk_paths(const std::wstring& input) {
    DiskInfo info;
    std::wstring vol_path;

    // Normalize input: if user provides "C:", convert to "\\.\C:"
    if (input.length() == 2 && input[1] == L':') {
        vol_path = L"\\\\.\\" + input;
    } else {
        vol_path = input;
    }

    info.volume_path = vol_path;

    // Open a handle to the volume. 
    // We use 0 for access because we only need to query the device number.
    HANDLE hDevice = CreateFileW(
        vol_path.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        return info; // success remains false
    }

    STORAGE_DEVICE_NUMBER sdn;
    DWORD bytesReturned;

    // Query the driver for the physical device number
    if (DeviceIoControl(
        hDevice,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL, 0,
        &sdn, sizeof(sdn),
        &bytesReturned,
        NULL
    )) {
        info.device_number = sdn.DeviceNumber;
        info.partition_number = sdn.PartitionNumber;
        info.physical_disk_path = L"\\\\.\\PhysicalDrive" + std::to_wstring(sdn.DeviceNumber);
        info.success = true;
    }

    CloseHandle(hDevice);
    return info;
}

int main() {
    // Example usage: user inputs a drive letter
    std::wstring userInput = L"C:"; 
    
    DiskInfo info = get_disk_paths(userInput);

    if (info.success) {
        std::wcout << L"Input: " << userInput << std::endl;
        std::wcout << L"Volume Path: " << info.volume_path << std::endl;
        std::wcout << L"Physical Disk Path: " << info.physical_disk_path << std::endl;
        std::wcout << L"Device #: " << info.device_number << std::endl;
        std::wcout << L"Partition #: " << info.partition_number << std::endl;
    } else {
        std::wcerr << L"Could not resolve paths for: " << userInput << std::endl;
    }

    return 0;
}

    void check_path(const std::wstring& path) {
        // 1. Check if it's a device namespace path (Physical Disk/Volume)
        if (path.find(L"\\\\.\\") == 0) {
            std::cout << "It's a device/physical disk path." << std::endl;
            return;
        }

        // 2. Otherwise, check if it's a standard file or directory
        DWORD attr = GetFileAttributesW(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            std::cout << "Path does not exist." << std::endl;
            return;
        }

        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "It's a directory." << std::endl;
        } else {
            std::cout << "It's a regular file." << std::endl;
        }
    }
    */
} // namespace SSUtils

int mainMain(int argC, char** argV);
int main()
{
    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");

    // Get WinAPI UTF-16 args
    int argC = 0;
    LPWSTR* argVW = CommandLineToArgvW(GetCommandLineW(), &argC);
    if (argVW == nullptr)
    {
        std::cerr << "couldn't get UTF-16 arguments\n";
        return EXIT_FAILURE;
    }
    // Convert to UTF-8
    std::vector<std::string> argStringBuffer;
    for (int i = 0; i < argC; ++i)
    {
        std::pair<std::string, Utils::ErrorState> result = SSUtils::utf16to8((char16_t*)argVW[i]);
        if (result.second != Utils::ErrorState::Success)
        {
            std::cerr << "invalid Unicode character in arguments\n";
            return EXIT_FAILURE;
        }
        argStringBuffer.push_back(result.first);
    }
    // Free WinAPI UTF-16 args
    LocalFree(argVW);
    // Create argV buffer
    std::vector<char*> argPtrBuffer;
    for (int i = 0; i < argC; ++i)
    {
        argPtrBuffer.push_back(argStringBuffer.at(i).data());
    }
    char** argV = argPtrBuffer.data();

    return mainMain(argC, argV);
}
