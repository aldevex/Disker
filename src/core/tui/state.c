#include <stdlib.h>
#include <stdio.h>
#include "../../mem/mem.h"
#include "../../utils/utils.h"
#include "../cmds/cmds.h"
#include "./tui.h"

static void run(SharedState* pSharedState);

extern void getRunCmds(const Dview* pArgs)
{
    // Print requested help or explain help command usage
    if (dviewSize(pArgs) == 2 && (
    utils_compareLowNT(&at(pArgs, 1), "help")
    || utils_compareLowNT(&at(pArgs, 1), "--help")
    || utils_compareLowNT(&at(pArgs, 1), "-help")
    || utils_compareLowNT(&at(pArgs, 1), "-h")
    ))
    {
        utils_help();
        return;
    }
    else utils_welcome();

    // Print version
    if (dviewSize(pArgs) == 2 && (
    utils_compareLowNT(&at(pArgs, 1), "version")
    || utils_compareLowNT(&at(pArgs, 1), "--version")
    || utils_compareLowNT(&at(pArgs, 1), "-version")
    || utils_compareLowNT(&at(pArgs, 1), "-v")
    ))
        utils_version();

    SharedState sharedState = {
        .parseType = PARSETYPE_NONE,
        .fd = NULL,
        .i = 0
    };
    // Commands via terminal (line by line)
    if (dviewSize(pArgs) == 1) sharedState.parseType = PARSETYPE_TERMINAL_LINES;
    // Commands via direct program arguments
    else if (view8StartsWithNT(&at(pArgs, 1), "-")) sharedState.parseType = PARSETYPE_DIRECT_ARGS;
    // Commands via file
    else if (dviewSize(pArgs) == 2)
    {
        sharedState.parseType = PARSETYPE_FILE;
        if (!view8Terminated(&at(pArgs, 1)))
        {
            fprintf(stderr, "non-null-terminated view given to %s,"
                            " that should NEVER happen\n",
                            __func__);
            exit(EXIT_FAILURE);
        }
        const char* pathNT = view8Data(&at(pArgs, 1));
        sharedState.fd = fopen(pathNT, "rb");
        if (sharedState.fd == NULL)
        {
            fprintf(stderr, "failed to open commands file \"%s\"\n",
                            pathNT);
            exit(EXIT_FAILURE);
        }
    }
    // Invalid args
    else
    {
        fprintf(stderr,
            "invalid arguments. expected \"<FILE NAME>\""
                " or \"-<command> <arguments> -[command] [arguments]...\"\n"
            "use \"help\" for help"
        );
        exit(EXIT_FAILURE);
    }

    run(&sharedState);
    
    if (sharedState.fd != NULL) fclose(sharedState.fd);
}



// Appends clean segment views from source string into given darray
void cutTerminalLine(const String8* pSrcString, Dview* pSegViews)
{
    // Clear previous segments and append segment views
    dviewClear(pSegViews);
    for (size_t i = 0; i < string8Size(pSrcString); /**/)
    {
        // Skip spaces
        size_t spaceSize = 0;
        if (isSpacePS8(string8DataConst(pSrcString) +i, string8Size(pSrcString) -1, &spaceSize))
            i += spaceSize;
        // Convert quoted text to one segment
        else if (at(pSrcString, i) == '\"' || at(pSrcString, i) == '\'')
        {
            size_t j = i +1;
            for (; j <= string8Size(pSrcString); j++)
            {
                if (j == string8Size(pSrcString)
                || at(pSrcString, j) == '\"' || at(pSrcString, j) == '\'')
                    break;
            }
            dviewAppendV(pSegViews, view8SubStr(&vwstr(pSrcString), i, j -i));
            i = j +1;
        }
        // Normal unquoted segment
        else
        {
            size_t j = i +1;
            for (; j <= string8Size(pSrcString); j++)
            {
                if (j == string8Size(pSrcString)
                || isSpacePS8(string8DataConst(pSrcString) +j, string8Size(pSrcString) -j, NULL))
                    break;
            }
            dviewAppendV(pSegViews, view8SubStr(&vwstr(pSrcString), i, j -i));
            i = j +1;
        }
    }
}

// Takes terminal input and outputs command string and clean segments
// String and darray 100% managed by the function except freeing them
void getCleanTerminalLine(String8* pBuffer, Dview* pSegViews)
{
    utils_getTerminalLine(pBuffer);
    cutTerminalLine(pBuffer, pSegViews);
}

// Outputs cleaned command string and segments
// Returns false when reaching after end of data stream
// String and darray 100% managed by the function except freeing them
bool getCmdText(SharedState* pSharedState, String8* pBuffer, Dview* pSegmentViews)
{
    switch (pSharedState->parseType)
    {
    case PARSETYPE_TERMINAL_LINES:
        getCleanTerminalLine(pBuffer, pSegmentViews);
        return true;
    case PARSETYPE_DIRECT_ARGS:
        break;
    case PARSETYPE_FILE:
        break;
    default:
        fprintf(stderr, "unprogrammed pSharedState->parseType (%u) given to %s\n",
                        pSharedState->parseType, __func__);
        exit(EXIT_FAILURE);
    }
    return true; // Gotta put this or compiler will crash out 🙀
}

static void run(SharedState* pSharedState)
{
    typedef struct LocalState
    {
        bool allYes;
        bool alwaysBinaryUnits;
        DiskInfo diskInfo;
        scheme_Type selectedScheme;
        uint64_t selectedPartNum;
        Dcmd commands; // Segment of commands to apply (write to disk/disk image)
    } LocalState;
    LocalState localState = {
        .allYes = false,
        .alwaysBinaryUnits = true,
        .diskInfo = diskInfoMakeDefault(),
        .selectedScheme = 0,
        .selectedPartNum = 0,
        .commands = {0},
    };
    String8 cmdString = {0};
    Dview cmdSegments = {0};
    while (true)
    {
        if (pSharedState->parseType == PARSETYPE_TERMINAL_LINES)
            printf("Disker> ");
        if (!getCmdText(pSharedState, &cmdString, &cmdSegments))
            fretvoid;
        Cmd cmd = {0};
        utils_ErrorState errorState = parseCmd(
            &vwstr(&cmdString), &cmdSegments,
            localState.alwaysBinaryUnits, pSharedState->parseType,
            &cmd
        );
        // Don't add on error
        if (errorState != UTILS_ERRORSTATE_SUCCESS) continue;
        switch (cmd.type)
        {
            // Ignore none
            case CMDTYPE_NONE:
                break;
            case CMDTYPE_SET_YES:
                localState.allYes = cmd.info.switchValue;
                break;
            case CMDTYPE_SET_BINARY:
                localState.alwaysBinaryUnits = cmd.info.switchValue;
                break;
            case CMDTYPE_SELECT_DISK:
            {
                if (!localState.allYes || cmd.info.selectDisk.type == GEO_TYPE_RAW_DISK)
                {
                    printf("Are you sure you want to select the disk \"%.*s\"?\n",
                            pfSpread(&cmd.info.selectDisk.path));
                    if (!utils_confirmation()) break;
                }
                bool createdNewFile = false;
                openLockReadDisk(&localState.diskInfo,
                                cmdInfoSelectDiskReleasePath(&cmd.info),
                                &createdNewFile);
                if (createdNewFile)
                    printf("Created disk \"%s\"\n", string8NT(&localState.diskInfo.path));
                else
                {
                    const char* schemeText = NULL;
                    if (localState.diskInfo.scheme.type == SCHEME_TYPE_MBR) schemeText = "MBR";
                                                                        else schemeText = "GPT";
                    printf(
                        "Opened disk \"%s\":\n"
                        "  - Size: %llu bytes\n"
                        "  - Sector size: %llu\n"
                        "  - Alignment: %llu\n"
                        "  - Scheme: %s\n"
                        "\n", string8NT(&localState.diskInfo.path),
                        localState.diskInfo.geometry.data.raw.size,
                        localState.diskInfo.geometry.data.raw.sectorSize,
                        localState.diskInfo.geometry.data.raw.alignment,
                        schemeText
                    );
                }
            }
                break;
            case CMDTYPE_SAVE:
            {
                bool yesSavePlz = false;
                if (localState.allYes && localState.diskInfo.geometry.type != GEO_TYPE_RAW_DISK)
                    yesSavePlz = true;
                else
                {
                    printf("Are you sure you want to save these changes?\n");
                    yesSavePlz = utils_confirmation();
                }
                // Apply commands
                if (yesSavePlz)
                {
                    for (size_t i = 0; i < dcmdSize(&localState.commands); i++)
                    {
                        WriteCmdInfo info = {
                            .pCmd = &at(&localState.commands, i),
                            .pDiskInfo = &localState.diskInfo,
                            // THESE TWO MUST BE CHANGED LATER TO DEPEND ON STATE
                            .selectedScheme = localState.diskInfo.scheme.type,
                            .partNum = 1
                        };
                        if (writeCmd(&info) != UTILS_ERRORSTATE_SUCCESS)
                        {
                            // Don't exit on error for terminal line-by-line input
                            if (pSharedState->parseType == PARSETYPE_TERMINAL_LINES) break;
                            else exit(EXIT_FAILURE);
                        }
                    }
                    dcmdClear(&localState.commands); // Clear commands for next collection
                }
            }
                break;
            case CMDTYPE_EXIT:
            {
                if (localState.diskInfo.handle != UTILS_HANDLE_NONE
                && !dcmdEmpty(&localState.commands))
                {
                    printf("warning: you have unsaved changes,"
                        " are you sure you want to exit the program?\n");
                    bool conf = utils_confirmation();
                    if (conf) fretvoid;
                }
                else fretvoid;
            }
                break;
            case CMDTYPE_HELP:
            {
                utils_help();
            }
                break;
        // Apply-level types (an edit command passed later to applyCmd e.g. CMDTYPE_EDIT_DISK)
        default:
        {
            if (localState.diskInfo.handle != UTILS_HANDLE_NONE)
            {
                // Validate edits relative to current state
                //   and change state according to edits
                if (cmd.type == CMDTYPE_EDIT_DISK)
                {
                    //
                    //
                    //
                    //
                    //
                }
                dcmdAppendR(&localState.commands, &cmd);
            }
            else fprintf(stderr, "invalid command (no currently selected disk)\n");
        }
            break;
        }
    }
end:
    string8Free(&cmdString);
    dviewFree(&cmdSegments);
    dcmdFree(&localState.commands);
}
