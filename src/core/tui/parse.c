#include <stdlib.h>
#include <stdio.h>
#include "../../mem/mem.h"
#include "../../utils/utils.h"
#include "../cmds/cmds.h"
#include "./tui.h"

// Forward declaration for quick jump in IDEs ✅🤫

utils_ErrorState parseCmd(const View8* pCmdView, const Dview* pSegments,
                            bool alwaysBinaryUnits, SrcType srcType, Cmd* pResultCmd);



// For quickly passing these 3 args to functions called by parseCmd
typedef struct SharedInfo
{
    const View8* pCmdView;
    SrcType srcType;
    bool alwaysBinaryUnits;
} SharedInfo;
SharedInfo sharedinfoMake(const View8* pCmdView, SrcType srcType, bool alwaysBinaryUnits)
{
    return (SharedInfo){
        .pCmdView = pCmdView,
        .srcType = srcType,
        .alwaysBinaryUnits = alwaysBinaryUnits
    };
}

// printInvalidCmdError() but takes shared info struct 🤯
// If parse type is SRCTYPE_TERMINAL_LINES, prints: invalid command ($reason "$optionalSpecifiedText")
//   otherwise prints: invalid command "$cmdView" ($reason "$optionalSpecifiedText")
void printInvalidCmdErrorSh(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const SharedInfo* pSharedInfo)
{
    printInvalidCmdError(pReasonView, pSpecifiedTextView_optional,
                        pSharedInfo->pCmdView, pSharedInfo->srcType);
}

// Checks if size (strToSize() result) is part of utils_SizeSig error values and prints errors accordingly
// Returns true if the size is erroneous
bool checkPrintErroneousSize(uint64_t val, const View8* pSeg1, const View8* pSeg2,
                            const View8* pTooBigResultErrorText, SharedInfo* pSharedInfo)
{
    if (val == (uint64_t)UTILS_SIZESIG_NO_NUMBER)
    {
        printInvalidCmdErrorSh(&vw("no number"), pSeg1, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_INVALID_NUMBER)
    {
        printInvalidCmdErrorSh(&vw("invalid number"), pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_NO_UNIT)
    {
        printInvalidCmdErrorSh(&vw("no unit"), pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_UNACCEPTABLE_ZERO)
    {
        printInvalidCmdErrorSh(&vw("zero is unacceptable"), pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_TOO_BIG_RESULT)
    {
        printInvalidCmdErrorSh(pTooBigResultErrorText, pSeg2, pSharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_INVALID_UNIT)
    {
        printInvalidCmdErrorSh(&vw("invalid unit"), pSeg2, pSharedInfo);
    }
    else return false;
    return true;
}

// Command Argument Value Type
typedef enum AVType
{
    AVTYPE_NULL,
    AVTYPE_TEXTUAL,
    AVTYPE_NUMERIC
} AVType;

// Textual Command Argument Value Type
typedef enum TextAVType
{
    TEXTAVTYPE_NULL,

    TEXTAVTYPE_ARRAY_SPECIFIED, // Acceped values defined in array    
    TEXTAVTYPE_FILE_PATH,
    TEXTAVTYPE_DIR_PATH,
    TEXTAVTYPE_CREATABLE_DISK_PATH, // Disk path, or a creatable/existing image file path
    TEXTAVTYPE_EXISTING_DISK_PATH, // Disk path, or an existing image file path
} TextAVType;

// Extracted Command Argument Value
typedef struct ExtractedAV
{
    String8 textualVal;
    uint64_t numericVal;
    utils_PathType pathType; // Path type if textual value is a path
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
        .type = AVTYPE_TEXTUAL,

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
//         .type = AVTYPE_NUMERIC,

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
    Dbool foundDecls = {0};
    dboolFillV(&foundDecls, dadinfoSize(pDeclInfos), false);
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
            printInvalidCmdErrorSh(&vw("unexpected argument"), &at(pSegments, segI), pSharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Check if the declarator is repeated
        const ADInfo currentDeclInfo = at(pDeclInfos, currentDeclInfoI);
        if (at(&foundDecls, currentDeclInfoI))
        {
            string8AppendNT(&errMsg, "repeated ");
            string8AppendVw(&errMsg, &currentDeclInfo.nameLowercase);
            string8AppendNT(&errMsg, " argument");
            printInvalidCmdErrorSh(&vwstr(&errMsg), &at(pSegments, segI), pSharedInfo);
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
            printInvalidCmdErrorSh(&vwstr(&errMsg),
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
            if (valInfo.type == AVTYPE_TEXTUAL)
            {
                bool valid = false;
                utils_PathType pathType = UTILS_PATHTYPE_NULL;
                // Any value allowed
                if (valInfo.textual.type == TEXTAVTYPE_NULL)
                {
                    valid = true;
                }
                // Array specified
                else if (valInfo.textual.type == TEXTAVTYPE_ARRAY_SPECIFIED)
                {
                    // Any value allowed if array is empty
                    if (dstrEmpty(valInfo.textual.pExpectedVals)) valid = true;
                    // Otherwise check specific values
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
                    pathType = utils_getPathType(&valSeg);
                    // Given path is invalid
                    if (pathType == UTILS_PATHTYPE_INVALID)
                    {
                        // "valid" remains false
                    }
                    // File
                    else if (valInfo.textual.type == TEXTAVTYPE_FILE_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_FILE)
                            valid = true;
                    }
                    // Directory
                    else if (valInfo.textual.type == TEXTAVTYPE_DIR_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DIR)
                            valid = true;
                    }
                    // Disk, existing image, or creatable image
                    else if (valInfo.textual.type == TEXTAVTYPE_CREATABLE_DISK_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DISK
                        || pathType == UTILS_PATHTYPE_FILE
                        || pathType == UTILS_PATHTYPE_MAYBE_CREATABLE)
                            valid = true;
                    }
                    // Disk, or existing image
                    else if (valInfo.textual.type == TEXTAVTYPE_EXISTING_DISK_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DISK
                        || pathType == UTILS_PATHTYPE_FILE)
                            valid = true;
                    }
                }
                // Print error or set extracted value
                if (!valid)
                {
                    string8AppendNT(&errMsg, "invalid ");
                    string8AppendVw(&errMsg, &valInfo.nameLowercase);
                    string8AppendNT(&errMsg, " value");
                    printInvalidCmdErrorSh(&vwstr(&errMsg), &valSeg, pSharedInfo);
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
                    string8CopyVw(&valInfo.pExtracted->textualVal, &valSeg);
                    valInfo.pExtracted->pathType = pathType;
                }
            }
            // Numeric value
            else if (valInfo.type == AVTYPE_NUMERIC)
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
            printInvalidCmdErrorSh(&vwstr(&errMsg), &vw(""), pSharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Set default values for missing optional declarators
        for (size_t valI = 0; valI < at(pDeclInfos, declInfoI).valCount; valI++)
        {
            const AVInfo valInfo = at(pDeclInfos, declInfoI).valInfos[valI];
            if (!valInfo.pExtracted) continue;
            // Textual value
            if (valInfo.type == AVTYPE_TEXTUAL)
                string8CopyVw(&valInfo.pExtracted->textualVal, &valInfo.textual.defaultVal);
            // Numeric value
            else if (valInfo.type == AVTYPE_NUMERIC)
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
                            bool alwaysBinaryUnits, SrcType srcType, Cmd* pResultCmd)
{
    // For quickly passing these 2 args to called functions
    SharedInfo sharedInfo = sharedinfoMake(pCmdView, srcType, alwaysBinaryUnits);
    // Argument declarator infos for parsing (freed at end of function)
    Dadinfo declInfos = {0};

    Cmd resultCmd = {0}; // Type kept as CMDTYPE_NULL on errors
    bool error = false; // Set to true on error discovery
    bool internalSkip = false; // If true: CMDTYPE_NULL is a skipped command rather than an erroneous one
    
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
        // Select partition
        if (dviewSize(pSegments) >= 2
        && utils_compareLowNT(&at(pSegments, 1), "part"))
        {
        }
        // Select scheme
        else if (dviewSize(pSegments) >= 2
        && utils_compareLowNT(&at(pSegments, 1), "scheme"))
        {
        }
        // Select disk
        else
        {
            // e.g. select x.img
            ExtractedAV path = {0};

            // If only one segment is in the array it will print an error
            dadinfoAppendV(&declInfos, adinfoMake("select", true, (AVInfo[]){
                avinfoMakeTextual("disk path", TEXTAVTYPE_CREATABLE_DISK_PATH, NULL, NULL, &path)
            }, 1));

            if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
                != UTILS_ERRORSTATE_SUCCESS) error = true;
            else
            {
                resultCmd.type = CMDTYPE_SELECT_DISK;
                if (path.pathType == UTILS_PATHTYPE_FILE
                || path.pathType == UTILS_PATHTYPE_MAYBE_CREATABLE)
                {
                    resultCmd.info.selectDisk.type = geo_fileTypeToGeoType(
                        utils_getFileType(&vwstr(&path.textualVal))
                    );
                }
                else if (path.pathType == UTILS_PATHTYPE_DISK
                || path.pathType == UTILS_PATHTYPE_PART)
                {
                    resultCmd.info.selectDisk.type = GEO_TYPE_RAW_DISK;
                }
                else
                {
                    fprintf(stderr, "invalid path type (%u) in %s,"
                            " validatePrintExtractVals() should've returned an error\n",
                            path.pathType, __func__);
                    exit(EXIT_FAILURE);
                }
                cmdInfoSelectDiskAdoptPath(&resultCmd.info, &path.textualVal);
            }
        }
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
            printInvalidCmdErrorSh(&vw("unexpected arguments"), &vw(""), &sharedInfo);
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

    // Version command
    else if (utils_compareLowNT(&at(pSegments, 0), "version"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("version", true, NULL, 0));

        if (validatePrintExtractVals(pSegments, &declInfos, &sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_VERSION, (CmdInfo){0});
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

    // Easter eggs
    else if (dviewSize(pSegments) == 1
    && utils_compareLowNT(&at(pSegments, 0), u8"Помоћ"))
    {
        printf("Не причам српски 😔\n");
        internalSkip = true;
    }
    else if (dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 0), "aide")
    && utils_compareLowNT(&at(pSegments, 1), "moi"))
    {
        printf(u8"je parle pas français 😔\n");
        internalSkip = true;
    }
    else if (dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 0), "big")
    && utils_compareLowNT(&at(pSegments, 1), "k"))
    {
        printf("well lets not talk abt it\n");
        internalSkip = true;
    }
    
    // Unsupported command
    if (internalSkip) resultCmd.type = CMDTYPE_NULL;
    else if (!error && resultCmd.type == CMDTYPE_NULL)
    {
        printInvalidCmdErrorSh(&vw("unsupported command"), &vw(""), &sharedInfo);
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
