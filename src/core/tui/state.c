#include <stdlib.h>
#include <stdio.h>
#include "../../mem/mem.h"
#include "../../utils/utils.h"
#include "./cmds.h"
#include "./tui.h"
#include "utils/tui.h"

typedef struct ProgState
{
    SrcState srcState;
    bool autoYes;
    bool alwaysBinaryUnits;

    DiskInfo diskInfo;
    size_t errorCount;
} ProgState;

static utils_ErrorState getRunCmds(ProgState* pState);

extern utils_ErrorState core(const Dview* pArgs)
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
        return UTILS_ERRORSTATE_SUCCESS;
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

    ProgState progState = {
        .srcState = {0},
        .autoYes = false,
        .alwaysBinaryUnits = true,

        .diskInfo = DISKINFO_DEFAULT,
        .errorCount = 0,
    };
    Dbyte dfile = {0};

    // Commands via terminal (line by line)
    if (dviewSize(pArgs) == 1) progState.srcState.type = SRCTYPE_TERMINAL_LINES;
    // Commands via direct program arguments
    else if (view8StartsWithNT(&at(pArgs, 1), "-"))
    {
        progState.srcState.type = SRCTYPE_DIRECT_ARGS;
        progState.srcState.pArgs = pArgs;
    }
    // Commands via file
    else if (dviewSize(pArgs) == 2)
    {
        progState.srcState.type = SRCTYPE_FILE;
        const View8* pPath = &at(pArgs, 1);
        // Open file
        utils_SysHandle fh = utils_openFile(pPath, (bool*)false);
        if (fh == UTILS_SYSHANDLE_NONE)
            return UTILS_ERRORSTATE_FAILURE;
        // Get file size
        uint64_t size = 0;
        if (utils_getFileInfo(pPath, fh, &size, NULL) != UTILS_ERRORSTATE_SUCCESS)
        {
            utils_closeFile(pPath, &fh);
            return UTILS_ERRORSTATE_FAILURE;
        }
        // Read file
        void* pFileBuffer = NULL;
        if (utils_readFile(pPath, fh, 0, size, &pFileBuffer)
        != UTILS_ERRORSTATE_SUCCESS)
        {
            utils_closeFile(pPath, &fh);
            return UTILS_ERRORSTATE_FAILURE;
        }
        // Close file
        if (utils_closeFile(pPath, &fh)
        != UTILS_ERRORSTATE_SUCCESS)
            return UTILS_ERRORSTATE_FAILURE;
        // Adopt data into darray
        dbyteAdoptPS(&dfile, (uint8_t**)&pFileBuffer, size);
    }
    // Invalid args
    else
    {
        fprintf(stderr,
            "invalid arguments. expected \"<FILE NAME>\""
                " or \"-<command 1> -[command 2]...\"\n"
        );
        exit(EXIT_FAILURE);
    }

    utils_ErrorState es = getRunCmds(&progState);

    dbyteFree(&dfile);
    return es;
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
// Returns false when reaching when terminal input gets closed
// String and darray 100% managed by the function except freeing them
static bool getCleanTerminalText(String8* pBuffer, Dview* pSegViews)
{
    printf("Disker> ");
    utils_ErrorState terminalFuckedStatus = utils_getTerminalLine(pBuffer);
    if (terminalFuckedStatus == UTILS_ERRORSTATE_SUCCESS)
    {
        // Clear previous segments and append segment views
        dviewClear(pSegViews);
        _cutCmdTextCore(&vwstr(pBuffer), pSegViews);
        return true;
    }
    else
    {
        // utils_getTerminalLine will free pBuffer;
        dviewFree(pSegViews);
        return false;
    }
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
    switch (pSrcState->type)
    {
    case SRCTYPE_TERMINAL_LINES:
        return getCleanTerminalText(pBuffer, pSegmentViews);
    case SRCTYPE_DIRECT_ARGS:
        return getCleanArgsText(pSrcState, pBuffer, pSegmentViews);
    case SRCTYPE_FILE:
        return getCleanFileText(pSrcState, pBuffer, pSegmentViews);
    default:
        fprintf(stderr, "unprogrammed pSrcState->srcType (%u) given to %s\n",
                        pSrcState->type, __func__);
        exit(EXIT_FAILURE);
    }
    return true; // Gotta put this or compiler will crash out 🙀
}

static void cmdSetYes(ProgState* pState, const CmdInfo* pInfo)
{
    pState->autoYes = pInfo->switchValue;
}

static void cmdSetBinary(ProgState* pState, const CmdInfo* pInfo)
{
    pState->alwaysBinaryUnits = pInfo->switchValue;
}

static void cmdHelp()
{
    utils_help();
}

static void cmdVersion()
{
    utils_version();
}

static void cmdCls(ProgState* pState, const View8* pCmdView)
{
    if (pState->srcState.type != SRCTYPE_TERMINAL_LINES)
    {
        printInvalidCmdError(&vw("command may only be used in terminal lines input"), &vw(""),
                            pCmdView, pState->srcState.type);
        pState->errorCount++;
        return;
    }
    if (!pState->autoYes
    && pState->diskInfo.editedAnything)
    {
        printf("are you sure you want to clear the screen?\n");
        if (!utils_confirmation()) return;
    }
    printf("\033[H\033[J");
}

static void cmdSelectDisk(ProgState* pState, CmdInfo* pInfo)
{
    // Get confirmation if there are unsaved commands
    if (pState->diskInfo.editedAnything)
    {
        printf("you have unsaved changes,"
            " are you sure you want to select the disk \"%s\"?\n",
            view8NT(&pInfo->selectDisk.path));
        if (!utils_confirmation()) return;
    }
    // Get confirmation if the selected disk is real
    else if (pInfo->selectDisk.type == DF_TYPE_RAW_DISK)
    {
        printf("are you sure you want to select the disk \"%s\"?\n",
                view8NT(&pInfo->selectDisk.path));
        if (!utils_confirmation()) return;
    }
    // Close and reset previous disk object
    if (pState->diskInfo.handle != UTILS_SYSHANDLE_NONE)
        diskInfoCloseReset(&pState->diskInfo);
    // Open, lock, & read the disk/image
    bool createdNewFile = false;
    if (diskInfoOpenRead(cmdInfoSelectDiskReleasePath(pInfo),
                        pInfo->selectDisk.rawImgSectorSize,
                        &pState->diskInfo, &createdNewFile)
    != UTILS_ERRORSTATE_SUCCESS)
    {
        pState->errorCount++;
        return;
    }
    // Print disk details
    if (createdNewFile)
        printf("Created empty disk image \"%s\"\n", string8NT(&pState->diskInfo.path));
    else
    {
        const char* schemeText = NULL;
        switch (pState->diskInfo.og.scheme.type)
        {
        case SCHEME_TYPE_MBR:
            schemeText = "MBR";
            break;
        case SCHEME_TYPE_GPT:
            schemeText = "GPT";
            break;
        case SCHEME_TYPE_UNKNOWN:
            schemeText = "Unknown";
            break;
        case SCHEME_TYPE_NULL:
            schemeText = "None";
            break;
        }
        printf("opened disk \"%s\":\n", string8NT(&pState->diskInfo.path));
        utils_printSize(stdout, "  - size: ", pState->diskInfo.og.format.data.raw.size, "\n");
        utils_printSize(stdout, "  - sector size: ", pState->diskInfo.og.format.data.raw.sectorSize, "\n");
        utils_printSize(stdout, "  - alignment: ", pState->diskInfo.og.format.data.raw.alignment, "\n");
        printf("  - scheme: %s\n", schemeText);
        printf("  - partitions: not programmed yet lol\n");
    }
}

static void cmdSave(ProgState* pState, const View8* pCmdView)
{
    // Don't save if no commands
    if (!pState->diskInfo.editedAnything)
    {
        printInvalidCmdError(&vw("no commands to save"), &vw(""),
                            pCmdView, pState->srcState.type);
        pState->errorCount++;
        return;
    }
    // Don't save if there are errors from file or program args
    if (pState->srcState.type != SRCTYPE_TERMINAL_LINES && pState->errorCount > 0)
    {
        printInvalidCmdError(&vw("couldn't save erroneous commands"), &vw(""),
                            pCmdView, pState->srcState.type);
        // No need to count this ig,
        //   afterall count exists for useful error messages, this is obvious
        // pState->errorCount++; 
        return;
    }
    // Confirm save operation
    bool yesSavePlz = false;
    if (pState->autoYes && pState->diskInfo.og.format.type != DF_TYPE_RAW_DISK)
        yesSavePlz = true;
    else
    {
        printf("are you sure you want to save these changes?\n");
        yesSavePlz = utils_confirmation();
    }
    // Apply saved commands
    if (yesSavePlz)
    {
        // Also resets the "diskInfo.editedX" booleans
        if (diskInfoWrite(&pState->diskInfo) != UTILS_ERRORSTATE_SUCCESS)
            pState->errorCount++;
        printf("Saved operations to disk \"%s\"\n", string8NT(&pState->diskInfo.path));
    }
}

static void cmdClose(ProgState* pState, const View8* pCmdView)
{
    if (pState->diskInfo.handle == UTILS_SYSHANDLE_NONE)
    {
        printInvalidCmdError(&vw("no selected disk to close"), &vw(""),
                            pCmdView, pState->srcState.type);
        pState->errorCount++;
        return;
    }
    if ((!pState->autoYes || pState->diskInfo.og.format.type == DF_TYPE_RAW_DISK)
    && pState->diskInfo.editedAnything)
    {
        printf("you have unsaved changes,"
            " are you sure you want to close the selected disk?\n");
        if (!utils_confirmation()) return;
    }
    // Adopt path string
    String8 adoptedPath = pState->diskInfo.path;
    pState->diskInfo.path = (String8){0};
    // Close & reset disk object
    utils_ErrorState es = diskInfoCloseReset(&pState->diskInfo);
    if (es == UTILS_ERRORSTATE_SUCCESS)
    {
        printf("Closed disk \"%s\"\n", string8NT(&adoptedPath));
        string8Free(&adoptedPath);
    }
    else pState->diskInfo.path = adoptedPath;
}

// Returns true on exit confirmation
static bool cmdExit(ProgState* pState)
{
    if ((!pState->autoYes || pState->diskInfo.og.format.type == DF_TYPE_RAW_DISK)
    && pState->diskInfo.editedAnything)
    {
        printf("you have unsaved changes,"
            " are you sure you want to exit the program?\n");
        return utils_confirmation();
    }
    else return true;
}

static void cmdEditDisk(ProgState* pState, const CmdInfo* pInfo, const View8* pCmdView)
{
    // e.g. disk size 40GiB sectsize 4096B align 1MiB  scheme GPT parts 256 sparse false
    // Validation first before editing disk object
    // Validate size
    if (pInfo->editDisk.size != UTILS_SIZESIG_NO_NUMBER)
    {
        // Real disk
        if (pState->diskInfo.og.format.type == DF_TYPE_RAW_DISK)
        {
            printInvalidCmdError(&vw("it's not possible to alter the size of a real disk"), &vw(""),
                                pCmdView, pState->srcState.type);
            pState->errorCount++;
            return;
        }
        // Confirm image shrink operation if given target size is less than original
        if (pInfo->editDisk.size < pState->diskInfo.og.format.data.raw.size
        && !pState->autoYes)
        {
            printf("Disker shrinks disk images blindly, this operation may cause data loss."
                    " are you sure you want to shrink the disk image?\n");
            if (!utils_confirmation()) return;
        }
    }
    // Validate sector size
    if (pInfo->editDisk.sectorSize != UTILS_SIZESIG_NO_NUMBER)
    {
        // Real disk
        if (pState->diskInfo.og.format.type == DF_TYPE_RAW_DISK)
        {
            printInvalidCmdError(&vw("it's not possible to alter the sector size of a real disk"), &vw(""),
                                pCmdView, pState->srcState.type);
            pState->errorCount++;
            return;
        }
    }
    // Validate sparsity
    if (pInfo->editDisk.sparse != UTILS_TRIBOOL_NONE)
    {
        // Real disk
        if (pState->diskInfo.og.format.type == DF_TYPE_RAW_DISK)
        {
            printInvalidCmdError(&vw("it's not possible to alter sparsity for a real disk"), &vw(""),
                                pCmdView, pState->srcState.type);
            pState->errorCount++;
            return;
        }
    }
    // Now set target disk object after validation
    // Set size
    if (pInfo->editDisk.size != UTILS_SIZESIG_NO_NUMBER)
    {
        pState->diskInfo.target.format.data.raw.size = pInfo->editDisk.size;
        pState->diskInfo.editedAnything = true;
        pState->diskInfo.editedFormat = true;
    }
    // Set sector size
    if (pInfo->editDisk.sectorSize != UTILS_SIZESIG_NO_NUMBER)
    {
        pState->diskInfo.target.format.data.raw.sectorSize = pInfo->editDisk.sectorSize;
        pState->diskInfo.editedAnything = true;
        pState->diskInfo.editedFormat = true;
    }
    // Set alignment
    if (pInfo->editDisk.alignment != UTILS_SIZESIG_NO_NUMBER)
    {
        pState->diskInfo.target.format.data.raw.alignment = pInfo->editDisk.alignment;
        pState->diskInfo.editedAnything = true;
        pState->diskInfo.editedFormat = true;
    }
    // Set scheme
    if (pInfo->editDisk.scheme != SCHEME_TYPE_NULL
    && pInfo->editDisk.scheme != pState->diskInfo.og.scheme.type)
    {
        if (pInfo->editDisk.scheme == SCHEME_TYPE_MBR)
        {
            pState->diskInfo.target.scheme.type = SCHEME_TYPE_MBR;
            pState->diskInfo.target.scheme.mbrData = MBR_DATA_DEFAULT;
        }
        pState->diskInfo.editedAnything = true;
        pState->diskInfo.editedScheme = true;
    }
    // // Set partition count
    // GPT disks only and 128 minimum because MBR always 4 bruh
    // if (pInfo->editDisk.scheme != SCHEME_TYPE_NULL)
    // {
    // }
    // Set sparse
    if (pInfo->editDisk.sparse != UTILS_TRIBOOL_NONE)
    {
        pState->diskInfo.target.sparsity = (bool)pInfo->editDisk.sparse;
        pState->diskInfo.editedAnything = true;
        pState->diskInfo.editedSparsity = true;
    }
}

static utils_ErrorState getRunCmds(ProgState* pState)
{
    // Command text only needed here to print errors,
    //   writeCmd doesn't print command text on failure.
    String8 cmdString = {0};
    Dview cmdSegments = {0};
    while (true)
    {
        // 16 errors max from file or direct args
        if (pState->srcState.type != SRCTYPE_TERMINAL_LINES && pState->errorCount > 16)
            fretvoid;
        if (!getCmdText(&pState->srcState, &cmdString, &cmdSegments))
            fretvoid;
        Cmd cmd = {0};
        utils_ErrorState errorState = parseCmd(
            &vwstr(&cmdString), &cmdSegments,
            pState->alwaysBinaryUnits, pState->srcState.type,
            &cmd
        );
        // Don't add on error
        if (errorState != UTILS_ERRORSTATE_SUCCESS)
        {
            pState->errorCount++;
            continue;
        }
        switch (cmd.type)
        {
        // Ignore null
        case CMDTYPE_NULL:
            /**/
            break;
        // Parse/creation commands (don't require a selected disk/image,
        //                          and don't affect the disk/image after "save"):
        case CMDTYPE_SET_YES:
            cmdSetYes(pState, &cmd.info);
            break;
        case CMDTYPE_SET_BINARY:
            cmdSetBinary(pState, &cmd.info);
            break;
        case CMDTYPE_HELP:
            cmdHelp();
            break;
        case CMDTYPE_VERSION:
            cmdVersion();
            break;
        case CMDTYPE_CLS:
            cmdCls(pState, &vwstr(&cmdString));
            break;
        case CMDTYPE_SELECT_DISK:
            cmdSelectDisk(pState, &cmd.info);
            break;
        case CMDTYPE_SAVE:
            cmdSave(pState, &vwstr(&cmdString));
            break;
        case CMDTYPE_CLOSE:
            cmdClose(pState, &vwstr(&cmdString));
            break;
        case CMDTYPE_EXIT:
            if (cmdExit(pState)) fretvoid;
            break;
        // Disk-operating commands (require selected opened disk/image
        //                          and affect the disk/image after "save"):
        default:
        {
            if (pState->diskInfo.handle == UTILS_SYSHANDLE_NONE)
            {
                printInvalidCmdError(&vw("no selected disk"), &vw(""),
                                    &vwstr(&cmdString), pState->srcState.type);
                pState->errorCount++;
                break; // Break the main switch statement
            }
            switch (cmd.type)
            {
            case CMDTYPE_EDIT_DISK:
                cmdEditDisk(pState, &cmd.info, &vwstr(&cmdString));
                break;
            default:
            {
                fprintf(stderr, "unprogrammed command (%u) given to %s\n",
                                cmd.type, __func__);
                exit(EXIT_FAILURE);
            } break;
            }
        } break;
        }
    }
end:
    string8Free(&cmdString);
    dviewFree(&cmdSegments);
    return (pState->errorCount == 0)? UTILS_ERRORSTATE_SUCCESS : UTILS_ERRORSTATE_FAILURE;
}
