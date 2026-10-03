#include <locale.h>
#include <stdio.h>
#include <stdatomic.h>
#include "../../mem/mem.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "./tui.h"

#ifdef _WIN32
BOOL WINAPI sigHandler(DWORD signal)
{
    // Return false = pass the signal to the next handler
    if (signal != CTRL_C_EVENT) return false;
    
    // Reset terminal colour to default
    printf("\033[0m");
    ExitProcess(EXIT_SUCCESS);
    return true;
}
#endif

int main()
{
    setlocale(LC_ALL, ".UTF-8");

#ifdef _WIN32
    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Reset terminal colour to default instead of current colour
    printf("\033[0m");

    // Set signal handler to reset terminal colour on Ctrl + C
    SetConsoleCtrlHandler(sigHandler, TRUE);

    // Get WinAPI UTF-16 args
    int argC = 0;
    LPWSTR* argVW = CommandLineToArgvW(GetCommandLineW(), &argC);
    if (argVW == NULL)
    {
        fprintf(stderr, "couldn't get UTF-16 arguments\n");
        exit(EXIT_FAILURE);
    }
    // Convert to UTF-8
    Dstr argStringBuffer = {0};
    for (int i = 0; i < argC; i++)
    {
        String8 result = {0};
        bool valid = string8CopyNT16(&result, (UTF16_t*)(argVW[i]));
        if (!valid)
        {
            fprintf(stderr, "invalid Unicode character in arguments\n");
            string8Free(&result);
            dstrFree(&argStringBuffer);
            LocalFree(argVW);
            exit(EXIT_FAILURE);
        }
        dstrAppendV(&argStringBuffer, result);
    }
    // Free WinAPI UTF-16 args
    LocalFree(argVW);
    // Create views buffer
    Dview argViewBuffer = {0};
    for (int i = 0; i < argC; i++)
        dviewAppendV(&argViewBuffer, view8MakeCopyS(& at(&argStringBuffer, i) ));

#else
    // Create views buffer
    Dview argViewBuffer = {0};
    for (int i = 0; i < argC; i++)
        dviewAppendV(&argViewBuffer, view8MakeCopyNT(argV[i]));
#endif

    // Call core
    utils_ErrorState es = core(&argViewBuffer);

    // Reset terminal colour to default
    printf("\033[0m");

    // Free memory then return
#ifdef _WIN32
    for (size_t i = 0; i < dstrSize(&argStringBuffer); i++)
        string8Free(&at(&argStringBuffer, i));
    dstrFree(&argStringBuffer);
#endif
    dviewFree(&argViewBuffer);
    return (es == UTILS_ERRORSTATE_SUCCESS)? EXIT_SUCCESS : EXIT_FAILURE;
}
