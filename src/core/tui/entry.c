#include "../../mem/mem.h"
#include "../../base/time.h"
#include "../../base/rand.h"
#include "../../base/file.h"
#include "../utils/utils.h"
#include "./state.h"
#include "./core.h"

es_t entry(size_t argCount, const UTF8_t *const * args)
{
    // Create string views for args
    Dview argViews = {0};
    for (size_t i = 0; i < argCount; i++)
        dviewAppendR(&argViews, &vw(args[i]));
    
    // Print requested help then return
    if (dviewSize(&argViews) == 2 && (
    utils_compareLowNT(&at(&argViews, 1), "help")
    || utils_compareLowNT(&at(&argViews, 1), "--help")
    || utils_compareLowNT(&at(&argViews, 1), "-help")
    || utils_compareLowNT(&at(&argViews, 1), "-h")
    ))
    {
        utils_help();
        return ES_SUCCESS;
    }
    // Print requested version then return
    else if (dviewSize(&argViews) == 2 && (
    utils_compareLowNT(&at(&argViews, 1), "version")
    || utils_compareLowNT(&at(&argViews, 1), "--version")
    || utils_compareLowNT(&at(&argViews, 1), "-version")
    || utils_compareLowNT(&at(&argViews, 1), "-v")
    ))
    {
        utils_version();   
        return ES_SUCCESS;
    }
    // Print version and mention the "help" command
    else utils_welcome();

    // 10% easter egg print chance
    base_Time time;
    if ((base_badRand() % 100 > 10) && base_time(&time) == ES_SUCCESS)
    {
        if (time.monthOfYear == 10) // October
        {
            // Green boo, orange exclamation marks
            base_printf("\e[0;92mBooo\e[38;2;255;165;0m!!\n" COLOUR_NORMAL);
        }
        else if (time.monthOfYear == 12 // December
        || (time.monthOfYear == 1 && time.dayOfMonth <= 14)) // First fortnight of January
        {
            // Red ho ho ho, bold white exclamation marks
            base_printf("\e[1;31mHo ho hoo\e[1;37m!!\n" COLOUR_NORMAL);
        }
        else if (time.hourOfDay == 3) // 3 in the morning
        {
            // Bold white
            base_printf("\e[1;37mBooo!\n" COLOUR_NORMAL);
        }
    }

    // Initialize program state
    ProgState progState = {
        .srcState = {0},

        .autoYes = false,
        .alwaysBinaryUnits = true,

        .diskInfo = LAYOUT_DISKINFO_DEFAULT,
        .errorCount = 0,
    };
    Dbyte darrFileBytes = {0};
    // Commands via terminal (line by line)
    if (dviewSize(&argViews) == 1) progState.srcState.type = SRCTYPE_TERMINAL_LINES;
    // Commands via direct program arguments
    else if (view8StartsWithNT(&at(&argViews, 1), "-"))
    {
        progState.srcState.type = SRCTYPE_DIRECT_ARGS;
        progState.srcState.pArgs = &argViews;
    }
    // Commands via file
    else if (dviewSize(&argViews) == 2)
    {
        progState.srcState.type = SRCTYPE_FILE;
        const View8* pPath = &at(&argViews, 1);
        // Open file
        handle_t fh = base_openFile(view8NT(pPath), BASE_OFMETHOD_OPEN_EXISTING, NULL);
        if (fh == HANDLE_NONE)
        {
            base_stdErrorf("Failed to open file \"%s\".\n", view8NT(pPath));
            return ES_FAILURE;
        }
        // Get file size
        uint64_t size = 0;
        if (base_getFileInfo(fh, &size, NULL) != ES_SUCCESS)
        {
            base_stdErrorf("Failed to get size of file \"%s\".\n", view8NT(pPath));
            base_closeFile(&fh);
            return ES_FAILURE;
        }
        // Allocate memory
        dbyteResizeDirectly(&darrFileBytes, size);
        // Read file
        if (base_readFile(fh, size, dbyteData(&darrFileBytes)) != ES_SUCCESS)
        {
            base_stdErrorf("Failed to read file \"%s\".\n", view8NT(pPath));
            base_closeFile(&fh);
            return ES_FAILURE;
        }
        // Close file
        if (base_closeFile(&fh) != ES_SUCCESS)
        {
            base_stdErrorf("Failed to close file \"%s\".\n", view8NT(pPath));
            return ES_FAILURE;
        }
    }
    // Invalid args
    else
    {
        base_stdErrorf("Invalid arguments. Expected \"<FILE NAME>\""
                        " or \"-<command 1> -[command 2]...\".\n");
        return ES_FAILURE;
    }

    // Run core logic
    es_t es = core(&argViews, &progState);

    // Free memory
    dbyteFree(&darrFileBytes);
    dviewFree(&argViews);
    // Return
    return es;
}
