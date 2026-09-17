#include <locale.h>
#include <stdio.h>
#include "../mem/mem.h"
#include "../utils/generic.h"
#include "../core/main_tui.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
    setlocale(LC_ALL, ".UTF-8");

#ifdef _WIN32
    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Get WinAPI UTF-16 args
    int argC = 0;
    LPWSTR* argVW = CommandLineToArgvW(GetCommandLineW(), &argC);
    if (argVW == NULL)
    {
        fprintf(stderr, "couldn't get UTF-16 arguments\n");
        return EXIT_FAILURE;
    }
    // Convert to UTF-8
    Dstr argStringBuffer = {0};
    for (int i = 0; i < argC; i++)
    {
        bool valid = false;
        String8 result = string8MakeCopyNT16((UTF16_t*)(argVW[i]), &valid);
        if (!valid)
        {
            fprintf(stderr, "invalid Unicode character in arguments\n");
            return EXIT_FAILURE;
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

    return tuiMain(&argViewBuffer);
}
