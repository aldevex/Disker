#include <stdlib.h>
#include <stdio.h>
#include "../../mem/mem.h"
#include "../../utils/utils.h"
#include "../cmds/cmds.h"
#include "./tui.h"
#include "utils/tui.h"

typedef struct ProgState
{
    bool autoyes;
    bool alwaysBinaryUnits;
    DiskInfo diskInfo;
    scheme_Type selectedScheme;
    uint64_t selectedPartNum;
    Dcmd commands; // Collection of commands to write to disk/image directly after "save"
    size_t errorCount;
} ProgState;

static void getRunCmds(SrcState* pSrcState);

extern void core(const Dview* pArgs)
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

    SrcState srcState = {0};
    Dbyte dfile = {0};

    // Commands via terminal (line by line)
    if (dviewSize(pArgs) == 1) srcState.srcType = SRCTYPE_TERMINAL_LINES;
    // Commands via direct program arguments
    else if (view8StartsWithNT(&at(pArgs, 1), "-"))
    {
        srcState.srcType = SRCTYPE_DIRECT_ARGS;
        srcState.pArgs = pArgs;
    }
    // Commands via file
    else if (dviewSize(pArgs) == 2)
    {
        srcState.srcType = SRCTYPE_FILE;
        const View8* pPath = &at(pArgs, 1);
        // Open file
        utils_SysHandle fh = utils_openFile(pPath, (bool*)false);
        if (fh == UTILS_SYSHANDLE_NONE)
            return;
        // Get file size
        uint64_t size = utils_getFileSize(pPath);
        if (size == UTILS_SIZESIG_NO_NUMBER)
        {
            utils_closeFile(pPath, &fh);
            return;
        }
        // Read file
        void* pFileBuffer = NULL;
        if (utils_readFile(pPath, fh, 0, size, &pFileBuffer)
        != UTILS_ERRORSTATE_SUCCESS)
        {
            utils_closeFile(pPath, &fh);
            return;
        }
        // Close file
        if (utils_closeFile(pPath, &fh)
        != UTILS_ERRORSTATE_SUCCESS)
            return;
        // Adopt data into darray
        dbyteAdoptPS(&dfile, (uint8_t**)&pFileBuffer, size);
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

    getRunCmds(&srcState);

    dbyteFree(&dfile);
}



// Appends clean segment views from source string into given darray
// Used for terminal and file lines (pre-processing must be done first)
// DOES NOT CLEAR SEGMENT VIEWS DARRAY
static void _cutCmdTextCore(const View8* pSrc, Dview* pSegViews)
{
    for (size_t i = 0; i < view8Size(pSrc); /**/)
    {
        // Skip spaces
        size_t spaceSize = 0;
        if (isSpacePS8(view8Data(pSrc) +i, view8Size(pSrc) -1, &spaceSize))
            i += spaceSize;
        // Convert quoted text to one segment
        else if (at(pSrc, i) == '\"' || at(pSrc, i) == '\'')
        {
            size_t j = i +1;
            for (; j <= view8Size(pSrc); j++)
            {
                if (j == view8Size(pSrc)
                || at(pSrc, j) == '\"' || at(pSrc, j) == '\'')
                    break;
            }
            dviewAppendV(pSegViews, view8SubStr(&vwstr(pSrc), i, j -i));
            i = j +1;
        }
        // Normal unquoted segment
        else
        {
            size_t j = i +1;
            for (; j <= view8Size(pSrc); j++)
            {
                if (j == view8Size(pSrc)
                || isSpacePS8(view8Data(pSrc) +j, view8Size(pSrc) -j, NULL))
                    break;
            }
            dviewAppendV(pSegViews, view8SubStr(&vwstr(pSrc), i, j -i));
            i = j +1;
        }
    }
}

// Takes terminal input and outputs command string and clean segments
// String and darray 100% managed by the function except freeing them
static void getCleanTerminalText(String8* pBuffer, Dview* pSegViews)
{
    printf("Disker> ");
    utils_getTerminalLine(pBuffer);
    // Clear previous segments and append segment views
    dviewClear(pSegViews);
    _cutCmdTextCore(&vwstr(pBuffer), pSegViews);
}

// Takes file and outputs command string and clean segments
// Returns false when reaching after end of file
// String and darray 100% managed by the function except freeing them
static bool getCleanFileText(SrcState* pSrcState, String8* pBuffer, Dview* pSegViews)
{
    // Clear previous string buffer
    string8Clear(pBuffer);
    // Clear previous segments
    dviewClear(pSegViews);
    // Check if reached end of file
    if (pSrcState->i >= dbyteSize(pSrcState->pFile)) return false;
    
    // Loop to get command segment out of arguments
    const Dbyte* pFile = pSrcState->pFile;
    // Index of start of command that will be currently parsed
    size_t fileStartI = pSrcState->i;
    for (size_t fileI = fileStartI; fileI <= dbyteSize(pFile); fileI++)
    {
        // Seek end of line
        if (fileI == dbyteSize(pFile) || at(pFile, fileI) == '\n')
        {
            // Get line view
            View8 lineVw = view8MakeCopyPS((UTF8_t*)(dbyteDataConst(pFile) +fileStartI), fileI -fileStartI);
            view8Trim(&lineVw);
            if (view8Size(&lineVw) == 0) break;

            // Clear off comment (must not be stuck to previous text JIC it's part of an argument)
            size_t commentI = view8FindNT(&lineVw, "#");
            if (commentI != STRING_NF
            && (commentI == 0 || revIsSpacePS8(view8Data(&lineVw), commentI, NULL) ))
                view8EraseEnd(&lineVw, view8Size(&lineVw) -commentI);
            view8Trim(&lineVw);
            if (view8Size(&lineVw) == 0) break;

            // Check and remove line continue character "\" at the end
            bool lineContinue = false;
            if (at(&lineVw, view8Size(&lineVw) -1) == '\\')
            {
                lineContinue = true;
                view8EraseEnd(&lineVw, 1);
            }
            view8Trim(&lineVw);
            // _cleanCmdTextCore() will safely handle 0 size anyway
            // if (view8Size(&lineVw) == 0) continue;

            // Append to string + padding space if next line included
            string8AppendVw(pBuffer, &lineVw);
            if (lineContinue) string8AppendCU(pBuffer, ' ');

            // Append segments to segment views (func will also clean quotes off)
            _cutCmdTextCore(&lineVw, pSegViews);

            // Continue onto next line and add to same string and segment views
            //   if a line continue character "\" is at the end
            if (lineContinue) continue;
            else
            {
                fileStartI = fileI;
                break;
            }
        }
    }
    pSrcState->i = fileStartI;
    return true;
}

// Takes arguments and outputs command string and clean segments
// Returns false when reaching after end of arguments
// String and darray 100% managed by the function except freeing them
// First segment MUST start with "-" checked before call
static bool getCleanArgsText(SrcState* pSrcState, String8* pBuffer, Dview* pSegViews)
{
    // Clear previous string buffer
    string8Clear(pBuffer);
    // Clear previous segments
    dviewClear(pSegViews);
    // Check if reached end of arguments
    if (pSrcState->i >= dviewSize(pSrcState->pArgs)) return false;

    // Loop to get command segment out of arguments
    const Dview* pArgs = pSrcState->pArgs;
    String8* pInsString = pBuffer;
    Dview* pInsSegments = pSegViews;
    // Index of start of command that will be currently parsed
    if (pSrcState->i == 0) pSrcState->i = 1;
    size_t startI = pSrcState->i;
    for (size_t i = startI; i <= dviewSize(pArgs); i++)
    {
        // Keep skipping segments until we reach the total end or a new command
        if (i != dviewSize(pArgs) && view8StartsWithNT(&at(pArgs, i), "-"))
            continue;
        // Reached end or new command:
        else
        {
            // Save string and segments
            for (size_t j = startI; j < i; j++)
            {
                View8 seg = at(pArgs, j);
                // Remove "-" off the first segment
                if (j == startI) view8EraseStart(&seg, 1);
                // Add segment + padding space if not last segment
                string8AppendVw(pInsString, &seg);
                if (j +1 != i) string8AppendCU(pInsString, ' ');
                dviewAppendR(pInsSegments, &seg);
            }
            // Set start for the next command and stop looping
            startI = i;
            break;
        }
    }
    pSrcState->i = startI;
    return true;
}

// Outputs cleaned command string and segments
// Returns false when reaching after end of data stream
// String and darray 100% managed by the function except freeing them
static bool getCmdText(SrcState* pSrcState, String8* pBuffer, Dview* pSegmentViews)
{
    switch (pSrcState->srcType)
    {
    case SRCTYPE_TERMINAL_LINES:
        getCleanTerminalText(pBuffer, pSegmentViews);
        return true;
    case SRCTYPE_DIRECT_ARGS:
        return getCleanArgsText(pSrcState, pBuffer, pSegmentViews);
    case SRCTYPE_FILE:
        return getCleanFileText(pSrcState, pBuffer, pSegmentViews);
    default:
        fprintf(stderr, "unprogrammed pSrcState->srcType (%u) given to %s\n",
                        pSrcState->srcType, __func__);
        exit(EXIT_FAILURE);
    }
    return true; // Gotta put this or compiler will crash out 🙀
}



static void cmdSave(ProgState* pState, const SrcState* pSrcState)
{
    bool yesSavePlz = false;
    if (pState->autoyes && pState->diskInfo.geometry.type != GEO_TYPE_RAW_DISK)
        yesSavePlz = true;
    else
    {
        printf("Are you sure you want to save these changes?\n");
        yesSavePlz = utils_confirmation();
    }
    // Apply commands
    if (yesSavePlz)
    {
        for (size_t i = 0; i < dcmdSize(&pState->commands); i++)
        {
            WriteCmdInfo info = {
                .pCmd = &at(&pState->commands, i),
                .pDiskInfo = &pState->diskInfo,
                .selectedScheme = pState->diskInfo.scheme.type,
                .selectedPartNum = 1
            };
            // if (at(&pState->commands, i).type == CMDTYPE_SELECT_SCHEME)
            //     info.selectedScheme = 
            // if (at(&pState->commands, i).type == CMDTYPE_SELECT_PART)
            //     info.selectedPartNum = 
            if (writeCmd(&info) != UTILS_ERRORSTATE_SUCCESS)
            {
                // Don't exit on error for terminal line-by-line input
                if (pSrcState->srcType == SRCTYPE_TERMINAL_LINES) break; // Breaks loop NOT THE SWITCH
                else exit(EXIT_FAILURE);
            }
        }
        dcmdClear(&pState->commands); // Clear commands for next collection
    }
}

static void cmdClose(ProgState* pState)
{
    if (!pState->autoyes
    && pState->diskInfo.handle != UTILS_SYSHANDLE_NONE
    && !dcmdEmpty(&pState->commands))
    {
        printf("warning: you have unsaved changes,"
            " are you sure you want to close the selected disk?\n");
        if (!utils_confirmation()) return;
    }
    dcmdClear(&pState->commands);
    diskInfoCloseReset(&pState->diskInfo);
    pState->selectedScheme = SCHEME_TYPE_NULL;
    pState->selectedPartNum = 0;
}

// Returns true on exit confirmation
static bool cmdExit(ProgState* pState)
{
    if (!pState->autoyes
    && pState->diskInfo.handle != UTILS_SYSHANDLE_NONE
    && !dcmdEmpty(&pState->commands))
    {
        printf("warning: you have unsaved changes,"
            " are you sure you want to exit the program?\n");
        return utils_confirmation();
    }
    else return true;
}

static void cmdSelectDisk(ProgState* pState, CmdInfo* pInfo)
{
    if (!pState->autoyes || pInfo->selectDisk.type == GEO_TYPE_RAW_DISK)
    {
        printf("Are you sure you want to select the disk \"%s\"?\n",
                view8NT(&pInfo->selectDisk.path));
        if (!utils_confirmation()) return;
    }
    bool createdNewFile = false;
    if (diskInfoOpenRead(cmdInfoSelectDiskReleasePath(pInfo), &pState->diskInfo, &createdNewFile)
        != UTILS_ERRORSTATE_SUCCESS) return;
    if (createdNewFile)
        printf("Created disk \"%s\"\n", string8NT(&pState->diskInfo.path));
    else
    {
        const char* schemeText = NULL;
        if (pState->diskInfo.scheme.type == SCHEME_TYPE_MBR) schemeText = "MBR";
                                                            else schemeText = "GPT";
        printf(
            "Opened disk \"%s\":\n"
            "  - Size: %llu bytes\n"
            "  - Sector size: %llu\n"
            "  - Alignment: %llu\n"
            "  - Scheme: %s\n"
            "\n", string8NT(&pState->diskInfo.path),
            pState->diskInfo.geometry.data.raw.size,
            pState->diskInfo.geometry.data.raw.sectorSize,
            pState->diskInfo.geometry.data.raw.alignment,
            schemeText
        );
    }
}

static void cmdEditDisk(ProgState* pState, const CmdInfo* pInfo)
{
    // Validate edits relative to current state
    //   and change state according to edits
    //
    //
    //
}

static void getRunCmds(SrcState* pSrcState)
{
    ProgState progState = {
        .autoyes = false,
        .alwaysBinaryUnits = true,
        .diskInfo = DISKINFO_DEFAULT,
        .selectedScheme = 0,
        .selectedPartNum = 0,
        .commands = {0},
        .errorCount = 0
    };
    String8 cmdString = {0};
    Dview cmdSegments = {0};
    while (true)
    {
        // 16 errors max from file or direct args
        // if (pSrcState->srcType != SRCTYPE_TERMINAL_LINES && progState.errorCount > 16)
        //     fretvoid;
        if (!getCmdText(pSrcState, &cmdString, &cmdSegments))
            fretvoid;
        Cmd cmd = {0};
        utils_ErrorState errorState = parseCmd(
            &vwstr(&cmdString), &cmdSegments,
            progState.alwaysBinaryUnits, pSrcState->srcType,
            &cmd
        );
        // Don't add on error
        if (errorState != UTILS_ERRORSTATE_SUCCESS)
        {
            // progState.errorCount++;
            continue;
        }
        switch (cmd.type)
        {
        // Ignore null
        case CMDTYPE_NULL:
            /**/
            break;
        case CMDTYPE_SET_YES:
            progState.autoyes = cmd.info.switchValue;
            break;
        case CMDTYPE_SET_BINARY:
            progState.alwaysBinaryUnits = cmd.info.switchValue;
            break;
        case CMDTYPE_SAVE:
            cmdSave(&progState, pSrcState);
            break;
        case CMDTYPE_CLOSE:
            cmdClose(&progState);
            break;
        case CMDTYPE_EXIT:
            if (cmdExit(&progState)) fretvoid;
            break;
        case CMDTYPE_HELP:
            utils_help();
            break;
        case CMDTYPE_VERSION:
            utils_version();
            break;
        case CMDTYPE_SELECT_DISK:
            cmdSelectDisk(&progState, &cmd.info);
            break;
        // Disk-operating types
        default:
        {
            if (progState.diskInfo.handle == UTILS_SYSHANDLE_NONE)
            {
                fprintf(stderr, "invalid command (no currently selected disk)\n");
                break;
            }
            switch (cmd.type)
            {
            case CMDTYPE_EDIT_DISK:
                cmdEditDisk(&progState, &cmd.info);
                break;
            default:
            {
                fprintf(stderr, "unprogrammed command (%u) given to %s\n",
                                cmd.type, __func__);
                exit(EXIT_FAILURE);
            } break;
            }
            dcmdAppendR(&progState.commands, &cmd);
        } break;
        }
    }
end:
    string8Free(&cmdString);
    dviewFree(&cmdSegments);
    dcmdFree(&progState.commands);
}
