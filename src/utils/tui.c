#include <stdio.h>
#include "./string.h"
#include "./tui.h"

void utils_version()
{
    printf("Disker v0.1 (2026)\n");
}

void utils_welcome()
{
    utils_version();
    printf("Type \"help\" for usage guide\n");
}

void utils_help()
{
    printf(
        "help <command>: prints 'command' usage guide. if 'command' is not provided, prints total disker guide.\n"
        ""
        ""
        ""
    );
}



void utils_getTerminalLine(String8* pBuffer)
{
    // Clear previous buffer
    string8Clear(pBuffer);
    // Initial capacity 128 code units
    if (string8Capacity(pBuffer) == 0) string8Reserve(pBuffer, 128);
    while (true)
    {
        // Copy "capacity" max bytes directly into string
        if (fgets(string8Data(pBuffer) +string8Size(pBuffer),
                string8Capacity(pBuffer) -string8Size(pBuffer), stdin) != NULL
        && strlen(string8Data(pBuffer) +string8Size(pBuffer)) != 0)
        {
            // Register new read segment size into total size
            string8ResizeDirectly(pBuffer,
                string8Size(pBuffer) + strlen(string8Data(pBuffer)+string8Size(pBuffer))
            );
            // Line reading isn't finished, add more capacity (which equals read limit)
            //   and continue
            if (string8Data(pBuffer)[string8Size(pBuffer) -1] != '\n')
                string8Reserve(pBuffer, string8Capacity(pBuffer) *2);
            // Line reading finished
            else
            {
                // Remove line feed and possible carriage return
                if (string8Size(pBuffer) >= 2
                && string8Data(pBuffer)[string8Size(pBuffer) -2] == '\r')
                    string8ResizeDirectly(pBuffer, string8Size(pBuffer) -2);
                else
                    string8ResizeDirectly(pBuffer, string8Size(pBuffer) -1);
                // Set null terminator
                string8Data(pBuffer)[string8Size(pBuffer)] = '\0';
                // Remove excess allocation if it's too much
                if (string8Capacity(pBuffer) -string8Size(pBuffer) > 256)
                    string8ShrinkToFit(pBuffer);
                // Stop reading
                break;
            }
        }
        // Failure
        else
        {
            string8Free(pBuffer);
            fprintf(stderr, "\ninput stream has been interrupted\n");
            exit(EXIT_FAILURE);
        }
    }
}

bool utils_confirmation()
{
    static const bool assumeNoForWeirdAnswers = true;
    bool yesSavePlz = false;
    String8 reply = {0};
    while (true)
    {
        printf("(yes/no): ");
        string8Clear(&reply);
        utils_getTerminalLine(&reply);
        string8Trim(&reply);
        if (utils_compareLowNT(&vwstr(&reply), "yes"))
        {
            yesSavePlz = true;
            break;
        }
        else if (utils_compareLowNT(&vwstr(&reply), "no"))
        {
            yesSavePlz = false;
            break; 
        }
        else if (assumeNoForWeirdAnswers)
        {
            printf("invalid answer, assuming \"no\"\n");
            yesSavePlz = false;
            break; 
        }
        else continue;
    }
    string8Free(&reply);
    return yesSavePlz;
}
