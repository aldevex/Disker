#include <stdlib.h>
#include <stdio.h>
#include "../mem/mem.h"
#include "../utils/generic.h"
#include "./commands.h"
#include "./main_tui.h"

typedef enum ParseType
{
    PARSETYPE_NONE,
    PARSETYPE_TERMINAL_LINES,
    PARSETYPE_DIRECT_ARGS,
    PARSETYPE_FILE
} ParseType;

typedef struct SharedState
{
    ParseType parseType;
    FILE* fd; // File descriptor (for running file commands)
    size_t i; // Current index in file or program arguments (ignored on terminal line-by-line input)
} SharedState;

// Returns parsed command (None on error), and error state
// Prints error in command on encounter
utils_ErrorState parseCmd(const View8* pCmdView, const Dview* pSegments,
                            bool alwaysBinaryUnits, ParseType parseType, Cmd* pResultCmd);
void getRunCmds(SharedState* pSharedState);



void tuiMain(const Dview* pArgs)
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

    getRunCmds(&sharedState);
    
    if (sharedState.fd != NULL) fclose(sharedState.fd);
}



// Appends clean segment views from source string into given darray
void cutTerminalLine(const String8* pSrcString, Dview* pSegViews)
{
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
void getCleanTerminalLine(String8* pBuffer, Dview* pSegmentViews)
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
            pBuffer->_itemsCount += strlen(string8Data(pBuffer) +string8Size(pBuffer));
            // Line reading isn't finished, add more capacity (which equals read limit)
            //   and continue
            if (string8Data(pBuffer)[string8Size(pBuffer) -1] != '\n')
                string8Reserve(pBuffer, string8Capacity(pBuffer) *2);
            // Line reading finished
            else
            {
                // Remove line feed and possible carriage return
                pBuffer->_itemsCount--;
                if (string8Size(pBuffer) > 0
                && string8Data(pBuffer)[string8Size(pBuffer) -1] == '\r')
                    pBuffer->_itemsCount--;
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

    // Clear previous segments
    dviewClear(pSegmentViews);
    // Append segment views
    cutTerminalLine(pBuffer, pSegmentViews);
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

void getRunCmds(SharedState* pSharedState)
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
            return;
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
                        ApplyCmdInfo info = {
                            .pCmd = &at(&localState.commands, i),
                            .pDiskInfo = &localState.diskInfo,
                            // THESE TWO MUST BE CHANGED LATER TO DEPEND ON STATE
                            .selectedScheme = localState.diskInfo.scheme.type,
                            .partNum = 1
                        };
                        if (applyCmd(&info) != UTILS_ERRORSTATE_SUCCESS)
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
                    if (conf) return;
                }
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
    string8Free(&cmdString);
    dviewFree(&cmdSegments);
    dcmdFree(&localState.commands);
}



// For quickly passing these 2 args to functions called by parseCmd
typedef struct SharedInfo
{
    const View8* pCmdView;
    ParseType parseType;
    bool alwaysBinaryUnits;
} SharedInfo;
SharedInfo sharedinfoMake(const View8* pCmdView, ParseType parseType, bool alwaysBinaryUnits)
{
    return (SharedInfo){
        .pCmdView = pCmdView,
        .parseType = parseType,
        .alwaysBinaryUnits = alwaysBinaryUnits
    };
}

// If parse type is PARSETYPE_TERMINAL_LINES, prints: invalid command ($reason "$optionalSpecifiedText")
//   otherwise prints: invalid command "$cmdView" ($reason "$optionalSpecifiedText")
void printInvalidCmdError(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const SharedInfo* pSharedInfo)
{
    const View8* pCmdView = pSharedInfo->pCmdView;
    if (pSharedInfo->parseType != PARSETYPE_TERMINAL_LINES)
    {
        if (view8Empty(pSpecifiedTextView_optional))
        {
            fprintf(stderr, "invalid command \"%.*s\" (%.*s)\n",
                    pfSpread(pCmdView), pfSpread(pReasonView));
        }
        else
        {
            fprintf(stderr, "invalid command \"%.*s\" (%.*s \"%.*s\")\n",
                    pfSpread(pCmdView), pfSpread(pReasonView),
                    pfSpread(pSpecifiedTextView_optional));
        }
    }
    else
    {
        if (view8Empty(pSpecifiedTextView_optional))
        {
            fprintf(stderr, "invalid command (%.*s)\n", 
                    pfSpread(pReasonView));
        }
        else
        {
            fprintf(stderr, "invalid command (%.*s \"%.*s\")\n", 
                    pfSpread(pReasonView), pfSpread(pSpecifiedTextView_optional));
        }
    }
}

// Checks if size (strToSize() result) is part of utils_SizeSig error values and prints errors accordingly
// Returns true if the size is erroneous
bool checkPrintErroneousSize(uint64_t val, const View8* pSeg1, const View8* pSeg2,
                            const View8* pTooBigResultErrorText, SharedInfo* pSharedInfo)
{
    if (val == (uint64_t)UTILS_SIZESIG_NO_NUMBER)
    {
        printInvalidCmdError(&vw("no number"), pSeg1, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_INVALID_NUMBER)
    {
        printInvalidCmdError(&vw("invalid number"), pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_NO_UNIT)
    {
        printInvalidCmdError(&vw("no unit"), pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_UNACCEPTABLE_ZERO)
    {
        printInvalidCmdError(&vw("zero is unacceptable"), pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_TOO_BIG_RESULT)
    {
        printInvalidCmdError(pTooBigResultErrorText, pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_INVALID_UNIT)
    {
        printInvalidCmdError(&vw("invalid unit"), pSeg2, pSharedInfo);
    }
    else return false;
    return true;
}

// Command Argument Value Type
typedef enum AVType
{
    AVTYPETEXTUAL,
    AVTYPENUMERIC
} AVType;

// Textual Command Argument Value Type
typedef enum TextAVType
{
    TEXTAVTYPE_ARRAY_SPECIFIED,
    TEXTAVTYPE_FILE_OR_NONE_PATH, // File path or inexistent path
    TEXTAVTYPE_FILE_PATH,
    TEXTAVTYPE_DIR_PATH,

    TEXTAVTYPE_EXISTING_DISK_PATH, // Disk path, or existing image file path
    TEXTAVTYPE_ANY_DISK_PATH, // Disk path, or existing or inexistent image file path
} TextAVType;

// Extracted Command Argument Value
typedef struct ExtractedAV
{
    String8 textualVal;
    uint64_t numericVal;
    geo_Type diskGeoType; // From disk/disk image path
} ExtractedAV;

// (Input) Command Argument Value Info
typedef struct AVInfo
{
    View8 nameLowercase;
    AVType type;

    union {
        struct {
            TextAVType type;
            const Dstr* pExpectedVals;
            View8 defaultVal;
        } textual;
        struct {
            bool zeroIsUnacceptable, unitExpected;
            uint64_t defaultVal;
        } numeric;
    };
    
    ExtractedAV* pExtracted;
} AVInfo;
// Commented out for defined but not used errors, just uncomment when you use them
static AVInfo avinfoMakeTextual(const UTF8_t* pNameLowercase,
                                    TextAVType type, const Dstr* pExpectedVals,
                                    const UTF8_t* pDefaultVal, ExtractedAV* pExtracted)
{
    return (AVInfo){
        .nameLowercase = view8MakeCopyNT(pNameLowercase),
        .type = AVTYPETEXTUAL,

        .textual.type = type,
        .textual.pExpectedVals = pExpectedVals,
        .textual.defaultVal = (pDefaultVal != NULL)? view8MakeCopyNT(pDefaultVal) : (View8){0},

        .pExtracted = pExtracted
    };
}
// static AVInfo avinfoMakeNumeric(const UTF8_t* pNameLowercase,
//                                     bool zeroIsUnacceptable, bool unitExpected,
//                                     uint64_t defaultVal, ExtractedAV* pExtracted)
// {
//     return (AVInfo){
//         .nameLowercase = view8MakeCopyNT(pNameLowercase),
//         .type = AVTYPENUMERIC,

//         .numeric.zeroIsUnacceptable = zeroIsUnacceptable,
//         .numeric.unitExpected = unitExpected,
//         .numeric.defaultVal = defaultVal,

//         .pExtracted = pExtracted
//     };
// }

// (Input) Command Declarator Info (Main command or subcommand (argument name), before values)
typedef struct ADInfo
{
    View8 nameLowercase;
    bool isRequired;
    size_t valCount;
    AVInfo valInfos[2];
} ADInfo;
ADInfo adinfoMake(const UTF8_t* nameLowercase, bool isRequired,
                        const AVInfo* valInfos, size_t valCount)
{
    if (valCount > 2)
    {
        fprintf(stderr, "infoCount > 2 given to %s (infoCount = %zu)\n",
                            __func__, valCount);
        exit(EXIT_FAILURE);
    }
    ADInfo result = {
        .nameLowercase = view8MakeCopyNT(nameLowercase),
        .isRequired = isRequired,
        .valCount = valCount
    };
    if (valInfos != NULL)
        for (size_t i = 0; i < valCount; i++)
            result.valInfos[i] = valInfos[i];
    return result;
}
DARRAY_DEF(Dadinfo, dadinfo, ADInfo)

// Validates, prints errors, and extracts all command argument declarators and values
// Command name (first argument declarator) must be compared to the desired string before call
//   and must be the first value in declaratorInfos
utils_ErrorState validatePrintExtractVals(const Dview* pSegments, const Dadinfo* pDeclInfos,
                                            SharedInfo* pSharedInfo)
{
utils_ErrorState result = UTILS_ERRORSTATE_FAILURE;
    String8 errMsg = {0};
    String8 errMsgSecondary = {0};

    // Check empty declarator infos
    if (dadinfoEmpty(pDeclInfos))
    {
        fprintf(stderr, "empty declarator vector given to %s",
                        __func__);
        exit(EXIT_FAILURE);
    }

    // To catch duplicate declarators
    Dbool foundDecls = dboolMakeFillV(dadinfoSize(pDeclInfos), false);
    for (size_t segI = 0; segI < dviewSize(pSegments); )
    {
        size_t currentDeclInfoI = SIZE_MAX;
        // Get current declarator index
        if (segI == 0) currentDeclInfoI = 0;
        else for (size_t declInfoI = 1; declInfoI < dadinfoSize(pDeclInfos); declInfoI++)
        {
            if (utils_compareLowVw(&at(pSegments, segI), &(dadinfoDataConst(pDeclInfos)[declInfoI].nameLowercase) ))
            {
                currentDeclInfoI = declInfoI;
                break;
            }
        }
        // Check if segment didn't match any declarator
        if (currentDeclInfoI == SIZE_MAX)
        {
            printInvalidCmdError(&vw("unexpected argument"), &at(pSegments, segI), pSharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Check if the declarator is repeated
        const ADInfo currentDeclInfo = at(pDeclInfos, currentDeclInfoI);
        if (at(&foundDecls, currentDeclInfoI))
        {
            string8AppendNT(&errMsg, "repeated ");
            string8AppendVw(&errMsg, &currentDeclInfo.nameLowercase);
            string8AppendNT(&errMsg, " argument");
            printInvalidCmdError(&vwstr(&errMsg), &at(pSegments, segI), pSharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Set current declarator index to found
        else at(&foundDecls, currentDeclInfoI) = true;
        // Check if there are enough segments for the current declarator's operands
        if (segI +currentDeclInfo.valCount >= dviewSize(pSegments))
        {
            string8AppendNT(&errMsg, "expected ");
            string8AppendVw(&errMsg, &currentDeclInfo.valInfos[0].nameLowercase);
            if (currentDeclInfo.valCount > 1)
            {
                string8AppendNT(&errMsg, " and ");
                string8AppendVw(&errMsg, &currentDeclInfo.valInfos[1].nameLowercase);
            }
            string8AppendNT(&errMsg, " values");

            // Declarator not printed if it's just the main command name
            printInvalidCmdError(&vwstr(&errMsg),
                    (currentDeclInfoI == 0)? &vw("") : &at(pSegments, segI),
                    pSharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Extract and validate current declarator values
        for (size_t valI = 0; valI < currentDeclInfo.valCount; valI++)
        {
            const AVInfo valInfo = currentDeclInfo.valInfos[valI];
            const View8 valSeg = at(pSegments, segI +1 +valI);
            // Textual value
            if (valInfo.type == AVTYPETEXTUAL)
            {
                bool valid = false;
                geo_Type diskGeoType = GEO_TYPE_NONE;
                if (valInfo.textual.type == TEXTAVTYPE_ARRAY_SPECIFIED)
                {
                    // Any value allowed
                    if (dstrEmpty(valInfo.textual.pExpectedVals)) valid = true;
                    // Specific values allowed
                    else for (size_t i = 0; i < dstrSize(valInfo.textual.pExpectedVals); i++)
                    {
                        const View8 expectedVal = view8MakeCopyS( &at(valInfo.textual.pExpectedVals, i) );
                        if (utils_compareLowVw(&valSeg, &expectedVal))
                        {
                            valid = true;
                            break;
                        }
                    }
                }
                // Path types
                else
                {
                    utils_PathType pathType = utils_getPathType(&valSeg);
                    if (pathType == UTILS_PATHTYPE_INVALID)
                    {
                        // "valid" remains false
                    }
                    else if (valInfo.textual.type == TEXTAVTYPE_FILE_OR_NONE_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_FILE || pathType == UTILS_PATHTYPE_NONE)
                            valid = true;
                    }
                    else if (valInfo.textual.type == TEXTAVTYPE_FILE_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_FILE)
                            valid = true;
                    }
                    else if (valInfo.textual.type == TEXTAVTYPE_DIR_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DIR)
                            valid = true;
                    }
                    else if (valInfo.textual.type == TEXTAVTYPE_EXISTING_DISK_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DISK)
                        {
                            valid = true;
                            diskGeoType = GEO_TYPE_RAW_DISK;
                        }
                        else if (pathType == UTILS_PATHTYPE_FILE)
                        {
                            valid = true;
                            // MUST BE PATH-DEPENDANT WHEN ADDING MORE FILE FORMATS
                            diskGeoType = GEO_TYPE_RAW_IMAGE;
                        }
                    }
                    else if (valInfo.textual.type == TEXTAVTYPE_ANY_DISK_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DISK)
                        {
                            valid = true;
                            diskGeoType = GEO_TYPE_RAW_DISK;
                        }
                        else if (pathType == UTILS_PATHTYPE_FILE)
                        {
                            valid = true;
                            // MUST BE PATH-DEPENDANT WHEN ADDING MORE FILE FORMATS
                            diskGeoType = GEO_TYPE_RAW_IMAGE;
                        }
                        else if (pathType == UTILS_PATHTYPE_NONE)
                        {
                            valid = true;
                            // MUST BE PATH-DEPENDANT WHEN ADDING MORE FILE FORMATS
                            diskGeoType = GEO_TYPE_RAW_IMAGE;
                        }
                    }
                }
                // Print error or set extracted value
                if (!valid)
                {
                    string8AppendNT(&errMsg, "invalid ");
                    string8AppendVw(&errMsg, &valInfo.nameLowercase);
                    string8AppendNT(&errMsg, " value");
                    printInvalidCmdError(&vwstr(&errMsg), &valSeg, pSharedInfo);
                    fret(UTILS_ERRORSTATE_FAILURE);
                }
                else if (valInfo.pExtracted == NULL)
                {
                    fprintf(stderr, "(textual) valInfo.pExtracted = NULL given to %s\n",
                                    __func__);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    valInfo.pExtracted->textualVal = string8MakeCopyVw(&valSeg);
                    valInfo.pExtracted->diskGeoType = diskGeoType;
                }
            }
            // Numeric value
            else if (valInfo.type == AVTYPENUMERIC)
            {
                uint64_t val = utils_strToSizeVw(&valSeg, valInfo.numeric.zeroIsUnacceptable,
                                                pSharedInfo->alwaysBinaryUnits, valInfo.numeric.unitExpected);
                string8AppendVw(&errMsgSecondary, &valInfo.nameLowercase);
                string8AppendNT(&errMsgSecondary, " is too big");
                if (checkPrintErroneousSize(val, &at(pSegments, segI), &valSeg,
                                                &vwstr(&errMsgSecondary), pSharedInfo))
                    fret(UTILS_ERRORSTATE_FAILURE);
                else if (valInfo.pExtracted == NULL)
                {
                    fprintf(stderr, "(numeric) valInfo.pExtracted = NULL given to %s\n",
                                    __func__);
                    exit(EXIT_FAILURE);
                }
                else valInfo.pExtracted->numericVal = val;
            }
        }
        // Progress main loop
        segI += (1 +currentDeclInfo.valCount);
    }

    // Check missing required declarators and set default values for missing optional ones
    for (size_t declInfoI = 1; declInfoI < dadinfoSize(pDeclInfos); declInfoI++)
    {
        // Skip available declarators
        if (at(&foundDecls, declInfoI)) continue;
        // Error for missing required declarators
        if (at(pDeclInfos, declInfoI).isRequired)
        {
            string8AppendNT(&errMsg, "expected ");
            string8AppendVw(&errMsg, &at(pDeclInfos, declInfoI).nameLowercase);
            string8AppendNT(&errMsg, " arguments");
            printInvalidCmdError(&vwstr(&errMsg), &vw(""), pSharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Set default values for missing optional declarators
        for (size_t valI = 0; valI < at(pDeclInfos, declInfoI).valCount; valI++)
        {
            const AVInfo valInfo = at(pDeclInfos, declInfoI).valInfos[valI];
            if (!valInfo.pExtracted) continue;
            // Textual value
            if (valInfo.type == AVTYPETEXTUAL)
                valInfo.pExtracted->textualVal = string8MakeCopyVw(&valInfo.textual.defaultVal);
            // Numeric value
            else if (valInfo.type == AVTYPENUMERIC)
                valInfo.pExtracted->numericVal = valInfo.numeric.defaultVal;
        }
    }
    fret(UTILS_ERRORSTATE_SUCCESS);

end:
    string8Free(&errMsg);
    string8Free(&errMsgSecondary);
    return result;
};

// Returns parsed command and error state
// Prints command errors on encounter
utils_ErrorState parseCmd(const View8* pCmdView, const Dview* pSegments,
                            bool alwaysBinaryUnits, ParseType parseType, Cmd* pResultCmd)
{
    // For quickly passing these 2 args to called functions
    SharedInfo sharedInfo = sharedinfoMake(pCmdView, parseType, alwaysBinaryUnits);
    // Argument declarator infos for parsing (freed at end of function)
    Dadinfo declInfos = {0};

    Cmd resultCmd = {0}; // Type kept as CMDTYPE_NONE on errors
    bool error = false; // Set to true on error discovery
    bool internalSkip = false; // If true: CMDTYPE_NONE is a skipped command rather than an erroneous one
    
    // All whitespace no segments
    if (dviewEmpty(pSegments))
    {
        internalSkip = true;
    }

    // Set yes
    else if (utils_compareLowNT(&at(pSegments, 0), "allyes"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("allyes", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_YES, (CmdInfo){.switchValue = true});
    }
    else if (utils_compareLowNT(&at(pSegments, 0), "manyes"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("manyes", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_YES, (CmdInfo){.switchValue = false});
    }
    
    // Set binary
    else if (utils_compareLowNT(&at(pSegments, 0), "binary"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("binary", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_BINARY, (CmdInfo){.switchValue = true});
    }
    else if (utils_compareLowNT(&at(pSegments, 0), "decimal"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("decimal", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_BINARY, (CmdInfo){.switchValue = false});
    }

    // Select
    else if (utils_compareLowNT(&at(pSegments, 0), "select"))
    {
        // Select disk
        if (dviewSize(pSegments) <= 2)
        {
            // e.g. select x.img
            ExtractedAV path = {0};

            // If only one segment is in the array it will print an error
            dadinfoAppendV(&declInfos, adinfoMake("select", true, (AVInfo[]){
                avinfoMakeTextual("disk path", TEXTAVTYPE_ANY_DISK_PATH, NULL, NULL, &path)
            }, 1));

            if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
                != UTILS_ERRORSTATE_SUCCESS) error = true;
            else
            {
                resultCmd.type = CMDTYPE_SELECT_DISK;
                resultCmd.info.selectDisk.type = path.diskGeoType;
                cmdInfoSelectDiskAdoptPath(&resultCmd.info, &path.textualVal);
            }
        }
        // // Select partition
        // else if (utils_compareLowNT(&at(pSegments, 1), "part"))
        // {

        // }
        // // Select scheme
        // else if (utils_compareLowNT(&at(pSegments, 1), "scheme"))
        // {
            
        // }
    }

    // Save
    else if (utils_compareLowNT(&at(pSegments, 0), "save"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("save", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SAVE, (CmdInfo){0});
    }

    // Stop, exit, quit
    else if (utils_compareLowNT(&at(pSegments, 0), "stop")
            || utils_compareLowNT(&at(pSegments, 0), "exit")
            || utils_compareLowNT(&at(pSegments, 0), "quit"))
    {
        if (dviewSize(pSegments) > 1)
        {
            printInvalidCmdError(&vw("unexpected arguments"), &vw(""), &sharedInfo);
            error = true;
        }
        else resultCmd = cmdMake(CMDTYPE_EXIT, (CmdInfo){0});
    }

    // help command
    else if (utils_compareLowNT(&at(pSegments, 0), "help"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("help", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_HELP, (CmdInfo){0});
    }
    
    // edit disk





    // // openvd (OLD COMMAND REPLACED WITH SELECT AND EDIT)
    // else if (utils_compareLowNT(&at(pSegments, 0), "openvd"))
    // {
    //     // e.g. openvd x.img size 32gib sectsize 4096B
    //     ExtractedAV path = {0}, size = {0}, sectorSize = {0};

    //     dadinfoAppendV(&declInfos, adinfoMake("openvd", true, (AVInfo[]){
    //         avinfoMakeTextual("file name", TEXTAVTYPE_FILE_OR_NONE_PATH, NULL, NULL, &path)
    //     }, 1));
    //     dadinfoAppendV(&declInfos, adinfoMake("size", false, (AVInfo[]){
    //         avinfoMakeNumeric("size", true, true, "64MiB", &size)
    //     }, 1));
    //     dadinfoAppendV(&declInfos, adinfoMake("sectsize", false, (AVInfo[]){
    //         avinfoMakeNumeric("sector size", true, true, "512B", &sectorSize)
    //     }, 1));

    //     if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
    //         != UTILS_ERRORSTATE_SUCCESS) error = true;
    //     else
    //     {
    //         resultCmd = cmdMake(CMDTYPE_OPEN_DISK, (CmdInfo){
    //             .openDisk.isReal = false,
    //             //.openDisk.setPath
    //             .openDisk.size = size.numericVal,
    //             .openDisk.sectorSize = sectorSize.numericVal,
    //             .openDisk.physicalSectorSize = sectorSize.numericVal,
    //         });
    //         // No need to free string memory,
    //         //   CmdInfo will take ownership of its memory and null out the object
    //         cmdInfoOpenDiskSetPath(&resultCmd.info, &path.textualVal);
    //     }
    // }
    
    // Unsupported command
    if (internalSkip) resultCmd.type = CMDTYPE_NONE;
    else if (!error && resultCmd.type == CMDTYPE_NONE)
    {
        printInvalidCmdError(&vw("unsupported command"), &vw(""), &sharedInfo);
        error = true;
    }

    // Return
    dadinfoFree(&declInfos);
    if (pResultCmd == NULL)
    {
        fprintf(stderr, "pResultCmd = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    *pResultCmd = resultCmd;
    return (!error)? UTILS_ERRORSTATE_SUCCESS : UTILS_ERRORSTATE_FAILURE;
}





// // value at pI must be initialized to 0 at the beginning
// // Returns UTILS_ERRORSTATE_FAILURE on call after end of file
// utils_ErrorState p_getFileLine(String8* pResult, const Dbyte* pDarray, size_t* pI)
// {
//     if (*pI >= dbyteSize(pDarray))
//     {
//         *pResult = string8MakeCopyNT("");
//         return UTILS_ERRORSTATE_FAILURE;
//     }
//     for (size_t i = *pI; i <= dbyteSize(pDarray); i++)
//     {
//         if (i == dbyteSize(pDarray) || at(pDarray, i) == '\n')
//         {
//             *pI = (i +1);
//             if (i != 0 && at(pDarray, i -1) == '\r')
//                 i--;
//             string8CopyPS(pResult, (UTF8_t*)dbyteDataConst(pDarray),
//                             dbyteSize(pDarray) -i);
//             break;
//         }
//     }
//     return UTILS_ERRORSTATE_SUCCESS;
// }

// // Returns clean string view (no comments or line-continue character)
// // Sets *pExpectingNextLine to true = command continues into the next line (mult-line)
// View8 p_cleanFileLine(const String8* pLineString, bool* pExpectingNextLine)
// {
//     View8 processView = view8MakeCopyS(pLineString);
//     // Remove comment
//     for (size_t i = 0; i < view8Size(&processView); i++)
//     {
//         if (at(&processView, i) == '#' // Comment
//         && (i == 0 || isspace((unsigned char)at(&processView, i -1))) ) // Nothing or space before it
//         // Hashtag may be part of commands so gotta ensure nothing is stuck behind it like "something#"
//         {
//             view8EraseEnd(&processView, i +1);
//             break;
//         }
//     }
//     // Check for and remove line-continue character
//     bool continueNextLine = false;
//     for (size_t i = view8Size(&processView) -1; i != SIZE_MAX; i--)
//     {
//         if (isspace((unsigned char)at(&processView, i)) ) continue;
//         else if (at(&processView, i) != '\\') break; // Normal character (not space nor line-continue character)
//         else // line-continue character "\"
//         {
//             continueNextLine = true;
//             view8EraseEnd(&processView, view8Size(&processView) -1);
//             break;
//         }
//     }
//     *pExpectingNextLine = continueNextLine;
//     return processView;
// }

// // Validates instructions file and returns instructions vector (empty if contains errors)
// Dins p_parseFile(const View8* pPathView, bool alwaysBinaryUnits)
// {
//     Dins results = {0};
//     size_t errorCount = 0;

//     Dbyte file = {0}; size_t fileStreamI = 0;
//     if (utils_readFile(pPathView, &file) != UTILS_ERRORSTATE_SUCCESS) return results;

//     // Loop over the file
//     String8 insString = {0};
//     Dview insSegments = {0};
//     String8 dirtyLineString = {0};
//     bool expectingNextLine = false;
//     while (p_getFileLine(&dirtyLineString, &file, &fileStreamI) != UTILS_ERRORSTATE_FAILURE
//     || expectingNextLine) // Line = "" if nothing left in stream
//     {
//         // If expectingNextLine && line == "": expectingNextLine = false
//         //   then the loop stops even if the last line has a redundant line-continue character "\"
//         View8 line = p_cleanFileLine(&dirtyLineString, &expectingNextLine);
//         // Cut line into segments and append to total instruction segments
//         for (size_t i = 0; i < view8Size(&line); /**/)
//         {
//             // Skip spaces
//             size_t wsSize = 0;
//             if (isSpaceNT8(view8Data(&line) +i, &wsSize)) i+= wsSize;
//             // Convert quoted text to one segment
//             else if (at(&line, i) == '\"' || at(&line, i) == '\'')
//             {
//                 size_t j = i +1;
//                 for (; j <= view8Size(&line); j++)
//                 {
//                     if (j == view8Size(&line)
//                     || at(&line, j) == '\"' || at(&line, j) == '\'')
//                         break;
//                 }
//                 // Add segment, no prefix padding space if last segment
//                 View8 seg = view8SubStr(&line, i, j -i);
//                 if (!expectingNextLine && i +1 == view8Size(&line)) string8AppendCU(&insString, ' ');
//                 string8AppendVw(&insString, &seg);
//                 dviewAppendR(&insSegments, &seg);
//                 i = j +1;
//             }
//             // Normal unquoted segment
//             else
//             {
//                 size_t j = i +1;
//                 for (; j <= view8Size(&line); j++)
//                 {
//                     if (j == view8Size(&line) || isspace(at(&line, j)))
//                         break;
//                 }
//                 // Add segment, no prefix padding space if last segment
//                 View8 seg = view8SubStr(&line, i, j -i);
//                 if (!expectingNextLine && i +1 == view8Size(&line)) string8AppendCU(&insString, ' ');
//                 string8AppendVw(&insString, &seg);
//                 dviewAppendR(&insSegments, &seg);
//                 i = j +1;
//             }
//         }
//         // Continue adding if expecting a new line
//         if (expectingNextLine) continue;
        
//         // Parse
//         Ins ins = {0};
//         utils_ErrorState errorState = p_parseIns(&vwstr(&insString), &insSegments,
//                                             alwaysBinaryUnits, P_CALLER_PARSE_FILE, &ins);
//         dviewClear(&insSegments); // Clear segments for next instruction
//         // Error happened
//         if (errorState != UTILS_ERRORSTATE_SUCCESS)
//         {
//             errorCount++;
//             dinsClear(&results);
//             // Don't return, but keep checking more errors
//         }

//         // Add to results if no error happened & not a None
//         if (errorCount == 0 && ins.type != CMDTYPE_NONE)
//         {
//             dinsAppendR(&results, &ins);
//             // Activate set binary instructions locally while parsing the file
//             if (ins.type == CMDTYPE_SET_BINARY) alwaysBinaryUnits = ins.info.switchValue;
//         }
//         // 10 errors max so user isn't overwhelmed
//         else if (errorCount == 10) break;
//     }

//     return results;
// }

// // First segment MUST start with "-" checked before call
// Dins p_parseDirectArgs(const Dview* pArgs, bool alwaysBinaryUnits)
// {
//     // For passing instructions via direct arguments
//     //   e.g. "disker - openvd x.img - format mbr parts 1 - part 1 fs FAT32 size 16gb"
    
//     Dins results = {0};
//     size_t errorCount = 0;
    
//     // Loop over argument segments
//     String8 insString = {0};
//     Dview insSegments = {0};
//     size_t startJ = 1; // Index of start of instruction that will be currently parsed
//     for (size_t i = 1; i <= dviewSize(pArgs); i++)
//     {
//         // Keep skipping until we reach the end or a new instruction
//         if (i != dviewSize(pArgs) && view8StartsWithNT(&at(pArgs, i), "-"))
//             continue;
        
//         // Remove "-" off the first segment
//         for (size_t j = startJ; j < i; j++)
//         {
//             View8 seg = at(pArgs, j);
//             // Remove "-" off the first segment
//             if (j == startJ) view8EraseStart(&seg, 1);
//             // Add segment + padding space if not last segment
//             string8AppendVw(&insString, &seg);
//             if (j +1 != i) string8AppendCU(&insString, ' ');
//             dviewAppendR(&insSegments, &seg);
//         }
//         startJ = i; // Set start for the current instruction (at [i])

//         // Parse
//         Ins ins = {0};
//         utils_ErrorState errorState = p_parseIns(&vwstr(&insString), &insSegments,
//                                                 alwaysBinaryUnits, P_CALLER_PARSE_DIRECT_ARGS, &ins);
//         // Error happened
//         if (errorState != UTILS_ERRORSTATE_SUCCESS)
//         {
//             errorCount++;
//             dinsClear(&results);
//             // Don't return, but keep checking more errors
//         }
//         // Add to results if no error happened & not a None
//         if (errorCount == 0 && ins.type != CMDTYPE_NONE)
//         {
//             dinsAppendR(&results, &ins);
//             // Activate set binary instructions locally while parsing the file
//             if (ins.type == CMDTYPE_SET_BINARY) alwaysBinaryUnits = ins.info.switchValue;
//         }
        
//         // 10 errors max so user isn't overwhelmed
//         if (errorCount == 10) break;
//     }

//     string8Free(&insString);
//     dviewFree(&insSegments);
//     return results;
// }
