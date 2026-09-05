#include <windows.h>
#include <shellapi.h> // shell32.lib linking required
#include <clocale>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include "../algos/utils.hpp"
#include "../algos/ssutils.hpp"
#include "../core/main_tui.hpp"

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
        std::pair<std::string, Utils::ErrorState> result = Utils::utf16to8((char16_t*)argVW[i]);
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

    return TUIMain(argC, argV);
}
