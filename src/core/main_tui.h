#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "../mem/mem.h"
#include "../utils/generic.h"
#include "./instructions.h"
#include "./state.h"

Dins p_parseFile(const View8* pPathView, bool alwaysBinaryUnits);
Dins p_parseDirectArgs(const Dview* pArgs, bool alwaysBinaryUnits);
Ins p_parseTerminalLine(bool alwaysBinaryUnits);

int tuiMain(const Dview* pArgs)
{
    // Print requested help or explain help command usage
    if (dviewSize(pArgs) == 2 && utils_compareLowNT(&at(pArgs, 1), "help"))
        utils_helpExit();
    else
        utils_helpAboutHelp();

    // Instructions via terminal
    if (dviewSize(pArgs) == 1)
    {
        while (true)
        {
            Ins ins = p_parseTerminalLine(state.alwaysBinaryUnits);
            applyIns(&ins);
        }
    }
    // Instructions via file
    else if (dviewSize(pArgs) == 2)
    {
        Dins instructions = p_parseFile(&at(pArgs, 1), state.alwaysBinaryUnits);
        for (size_t i = 0; i < dinsSize(&instructions); i++)
            applyIns(&at(&instructions, i));
    }
    // Instructions via direct arguments
    else if (view8StartsWithNT(&at(pArgs, 1), "-"))
    {
        Dins instructions = p_parseDirectArgs(pArgs, state.alwaysBinaryUnits);
        for (size_t i = 0; i < dinsSize(&instructions); i++)
            applyIns(&at(&instructions, i));
    }
    // Invalid args
    else
    {
        fprintf(stderr,
            "invalid arguments. expected \"<FILE NAME>\""
                " or \"-<instruction> <arguments> -[instruction] [arguments]...\"\n"
            "use \"help\" for help"
        );
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

// For quickly passing these 2 args to functions called by p_parseIns
typedef struct p_SharedInfo
{
    const View8* pInsView;
    bool callerParsingSingleLine;
    bool alwaysBinaryUnits;
} p_SharedInfo;
p_SharedInfo p_sharedinfoMake(const View8* pInsView, bool callerParsingSingleLine,
                                bool alwaysBinaryUnits)
{
    return (p_SharedInfo){
        .pInsView = pInsView,
        .callerParsingSingleLine = callerParsingSingleLine,
        .alwaysBinaryUnits = alwaysBinaryUnits
    };
}

// If not callerParsingSingleLine, prints: invalid instruction "$insView" ($reason "$optionalSpecifiedText")
// If callerParsingSingleLine, prints: invalid instruction ($reason "$optionalSpecifiedText")
void p_printInvalidInsError(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            p_SharedInfo sharedInfo)
{
    const View8* pInsView = sharedInfo.pInsView;
    bool callerParsingSingleLine = sharedInfo.callerParsingSingleLine;
    if (!callerParsingSingleLine)
    {
        if (view8Empty(pSpecifiedTextView_optional))
        {
            fprintf(stderr, "invalid instruction \"%.*s\" (%.*s)\n",
                    pfSpread(pInsView), pfSpread(pReasonView));
        }
        else
        {
            fprintf(stderr, "invalid instruction \"%.*s\" (%.*s \"%.*s\")\n",
                    pfSpread(pInsView), pfSpread(pReasonView),
                    pfSpread(pSpecifiedTextView_optional));
        }
    }
    else
    {
        if (view8Empty(pSpecifiedTextView_optional))
        {
            fprintf(stderr, "invalid instruction (%.*s)\n", 
                    pfSpread(pReasonView));
        }
        else
        {
            fprintf(stderr, "invalid instruction (%.*s \"%.*s\")\n", 
                    pfSpread(pReasonView), pfSpread(pSpecifiedTextView_optional));
        }
    }
}

// Checks if size (strToSize() result) is part of utils_SizeSig error values and prints errors accordingly
// Returns true if the size is erroneous
bool p_checkPrintErroneousSize(uint64_t val, const View8* pSeg1, const View8* pSeg2,
                                    const View8* pTooBigResultErrorText,
                                    p_SharedInfo sharedInfo)
{
    if (val == (uint64_t)UTILS_SIZESIG_NO_NUMBER)
    {
        p_printInvalidInsError(&vw("no number"), pSeg1, sharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_INVALID_NUMBER)
    {
        p_printInvalidInsError(&vw("invalid number"), pSeg2, sharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_NO_UNIT)
    {
        p_printInvalidInsError(&vw("no unit"), pSeg2, sharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_UNACCEPTABLE_ZERO)
    {
        p_printInvalidInsError(&vw("zero is unacceptable"), pSeg2, sharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_TOO_BIG_RESULT)
    {
        p_printInvalidInsError(pTooBigResultErrorText, pSeg2, sharedInfo);
    }
    else if (val == (uint64_t)UTILS_SIZESIG_INVALID_UNIT)
    {
        p_printInvalidInsError(&vw("invalid unit"), pSeg2, sharedInfo);
    }
    else return false;
    return true;
}

// Argument Value Type
typedef enum p_AVType
{
    P_AVTYPE_TEXTUAL,
    P_AVTYPE_NUMERIC
} p_AVType;

// Textual Argument Value Type
typedef enum p_TextAVType
{
    P_TEXTAVTYPE_ARRAY_SPECIFIED,
    P_TEXTAVTYPE_FILE_OR_NONE_PATH, // File path or inexistent path
    P_TEXTAVTYPE_FILE_PATH,
    P_TEXTAVTYPE_DIR_PATH,
    P_TEXTAVTYPE_DISK_PATH
} p_TextAVType;

// Extracted Argument Value
typedef struct p_ExtractedAV
{
    String8 textualVal;
    uint64_t numericVal;
} p_ExtractedAV;

// (Input) Argument Value Info
typedef struct p_AVInfo
{
    View8 nameLowercase;
    p_AVType type;

    union {
        struct {
            p_TextAVType type;
            const Dstr* pExpectedVals;
        } textual;
        struct {
            bool zeroIsUnacceptable, unitExpected;
        } numeric;
    };
    
    View8 defaultVal; // Also used for numeric values
    p_ExtractedAV* pExtracted;
} p_AVInfo;
static p_AVInfo p_avinfoMakeTextual(const UTF8_t* pNameLowercase, p_TextAVType type,
                                    const Dstr* pExpectedVals, const UTF8_t* pDefaultVal,
                                    p_ExtractedAV* pExtracted)
{
    return (p_AVInfo){
        .nameLowercase = view8MakeCopyNT(pNameLowercase),
        .type = P_AVTYPE_TEXTUAL,

        .textual.type = type,
        .textual.pExpectedVals = pExpectedVals,

        .defaultVal = (pDefaultVal != NULL)? view8MakeCopyNT(pDefaultVal) : (View8){0},
        .pExtracted = pExtracted
    };
}
static p_AVInfo p_avinfoMakeNumeric(const UTF8_t* pNameLowercase,
                                    bool zeroIsUnacceptable, bool unitExpected,
                                    const UTF8_t* pDefaultValText, p_ExtractedAV* pExtracted)
{
    return (p_AVInfo){
        .nameLowercase = view8MakeCopyNT(pNameLowercase),
        .type = P_AVTYPE_NUMERIC,

        .numeric.zeroIsUnacceptable = zeroIsUnacceptable,
        .numeric.unitExpected = unitExpected,

        .defaultVal = (pDefaultValText != NULL)? view8MakeCopyNT(pDefaultValText) : (View8){0},
        .pExtracted = pExtracted
    };
}
DARRAY_DEF(Davinfo, davinfo, p_AVInfo)

// (Input) Argument Declarator Info (argument name or instruction name itself, before values)
typedef struct p_ADInfo
{
    View8 nameLowercase;
    bool isRequired;
    size_t valCount;
    p_AVInfo valInfos[2];
} p_ADInfo;
p_ADInfo p_adinfoMake(const UTF8_t* nameLowercase, bool isRequired,
                        const p_AVInfo* valInfos, size_t valCount)
{
    if (valCount > 2)
    {
        fprintf(stderr, "infoCount > 2 given to %s (infoCount = %zu)\n",
                            __func__, valCount);
        exit(EXIT_FAILURE);
    }
    p_ADInfo result = {
        .nameLowercase = view8MakeCopyNT(nameLowercase),
        .isRequired = isRequired,
        .valCount = valCount
    };
    if (valInfos != NULL)
        for (size_t i = 0; i < valCount; i++)
            result.valInfos[i] = valInfos[i];
    return result;
}
DARRAY_DEF(Ddinfo, ddinfo, p_ADInfo)

// Validates, prints errors, and extracts all argument declarators and values
// Instruction name (first argument declarator) must be compared to the desired string before call
//   and must be the first value in declaratorInfos
utils_ErrorState p_validatePrintExtractVals(const Dview* pSegments, const Ddinfo* pDeclInfos,
                                            p_SharedInfo sharedInfo)
{
utils_ErrorState result = UTILS_ERRORSTATE_FAILURE;
    String8 errMsg = {0};
    String8 errMsgSecondary = {0};
    // Check empty declarator infos
    if (ddinfoEmpty(pDeclInfos))
    {
        fprintf(stderr, "empty declarator vector given to %s",
                        __func__);
        exit(EXIT_FAILURE);
    }

    // To catch duplicate declarators
    Dbool foundDecls = dboolMakeFillV(ddinfoSize(pDeclInfos), false);
    for (size_t segI = 0; segI < dviewSize(pSegments); )
    {
        size_t currentDeclInfoI = SIZE_MAX;
        // Get current declarator index
        if (segI == 0) currentDeclInfoI = 0;
        else for (size_t declInfoI = 1; declInfoI < ddinfoSize(pDeclInfos); declInfoI++)
        {
            if (utils_compareLowVw(&at(pSegments, segI), &(ddinfoDataConst(pDeclInfos)[declInfoI].nameLowercase) ))
            {
                currentDeclInfoI = declInfoI;
                break;
            }
        }
        // Check if segment didn't match any declarator
        if (currentDeclInfoI == SIZE_MAX)
        {
            p_printInvalidInsError(&vw("unexpected argument"), &at(pSegments, segI), sharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Check if the declarator is repeated
        const p_ADInfo currentDeclInfo = at(pDeclInfos, currentDeclInfoI);
        if (at(&foundDecls, currentDeclInfoI))
        {
            string8AppendNT(&errMsg, "repeated ");
            string8AppendVw(&errMsg, &currentDeclInfo.nameLowercase);
            string8AppendNT(&errMsg, " argument");
            p_printInvalidInsError(&vwstr(&errMsg), &at(pSegments, segI), sharedInfo);
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

            // Declarator not printed if it's just the instruction name
            p_printInvalidInsError(&vwstr(&errMsg),
                    (currentDeclInfoI == 0)? &vw("") : &at(pSegments, segI),
                    sharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Extract and validate current declarator values
        for (size_t valI = 0; valI < currentDeclInfo.valCount; valI++)
        {
            const p_AVInfo valInfo = currentDeclInfo.valInfos[valI];
            const View8 valSeg = at(pSegments, segI +1 +valI);
            // Textual value
            if (valInfo.type == P_AVTYPE_TEXTUAL)
            {
                bool valid = false;
                if (valInfo.textual.type == P_TEXTAVTYPE_ARRAY_SPECIFIED)
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
                    // "valid" remains false if path is invalid
                    if (pathType == UTILS_PATHTYPE_INVALID)
                    {
                    }
                    else if (valInfo.textual.type == P_TEXTAVTYPE_FILE_OR_NONE_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_FILE || pathType == UTILS_PATHTYPE_NONE)
                            valid = true;
                    }
                    else if (valInfo.textual.type == P_TEXTAVTYPE_FILE_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_FILE)
                            valid = true;
                    }
                    else if (valInfo.textual.type == P_TEXTAVTYPE_DIR_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DIR)
                            valid = true;
                    }
                    else if (valInfo.textual.type == P_TEXTAVTYPE_DISK_PATH)
                    {
                        if (pathType == UTILS_PATHTYPE_DISK)
                            valid = true;
                    }
                }
                // Print error or set extracted value
                if (!valid)
                {
                    string8AppendNT(&errMsg, "invalid ");
                    string8AppendVw(&errMsg, &valInfo.nameLowercase);
                    string8AppendNT(&errMsg, " value");
                    p_printInvalidInsError(&vwstr(&errMsg), &valSeg, sharedInfo);
                    fret(UTILS_ERRORSTATE_FAILURE);
                }
                else if (valInfo.pExtracted == NULL)
                {
                    fprintf(stderr, "(textual) valInfo.pExtracted = NULL given to %s\n",
                                    __func__);
                    exit(EXIT_FAILURE);
                }
                else valInfo.pExtracted->textualVal = string8MakeCopyVw(&valSeg);
            }
            // Numeric value
            else if (valInfo.type == P_AVTYPE_NUMERIC)
            {
                uint64_t val = utils_strToSizeVw(&valSeg, valInfo.numeric.zeroIsUnacceptable,
                                                sharedInfo.alwaysBinaryUnits, valInfo.numeric.unitExpected);
                string8AppendVw(&errMsgSecondary, &valInfo.nameLowercase);
                string8AppendNT(&errMsgSecondary, " is too big");
                if (p_checkPrintErroneousSize(val, &at(pSegments, segI), &valSeg,
                                                &vwstr(&errMsgSecondary), sharedInfo))
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
    for (size_t declInfoI = 1; declInfoI < ddinfoSize(pDeclInfos); declInfoI++)
    {
        // Skip available declarators
        if (at(&foundDecls, declInfoI)) continue;
        // Error for missing required declarators
        if (at(pDeclInfos, declInfoI).isRequired)
        {
            string8AppendNT(&errMsg, "expected ");
            string8AppendVw(&errMsg, &at(pDeclInfos, declInfoI).nameLowercase);
            string8AppendNT(&errMsg, " arguments");
            p_printInvalidInsError(&vwstr(&errMsg), &vw(""), sharedInfo);
            fret(UTILS_ERRORSTATE_FAILURE);
        }
        // Set default values for missing optional declarators
        for (size_t valI = 0; valI < at(pDeclInfos, declInfoI).valCount; valI++)
        {
            const p_AVInfo valInfo = at(pDeclInfos, declInfoI).valInfos[valI];
            if (!valInfo.pExtracted) continue;
            // Textual value
            if (valInfo.type == P_AVTYPE_TEXTUAL)
            {
                valInfo.pExtracted->textualVal = string8MakeCopyVw(&valInfo.defaultVal);
            }
            // Numeric value
            else if (valInfo.type == P_AVTYPE_NUMERIC)
            {
                uint64_t val = utils_strToSizeVw(&valInfo.defaultVal, valInfo.numeric.zeroIsUnacceptable,
                                                sharedInfo.alwaysBinaryUnits, valInfo.numeric.unitExpected);
                if (val >= (uint64_t)UTILS_SIZESIG_LEASTERROR)
                {
                    fprintf(stderr, "(numeric) invalid valInfo.defaultVal given to %s\n",
                                    __func__);
                    exit(EXIT_FAILURE);
                }
                else valInfo.pExtracted->numericVal = val;
            }
        }
    }
end:
    string8Free(&errMsg);
    string8Free(&errMsgSecondary);
    return result;
};

// Returns parsed instruction (None on error), and error state
// Prints error in instruction on encounter
utils_ErrorState p_parseIns(const View8* pInsView, const Dview* pSegments,
                            bool alwaysBinaryUnits, bool callerParsingSingleLine, Ins* pResultIns)
{
    // For quickly passing these 2 args to called functions
    p_SharedInfo sharedInfo = p_sharedinfoMake(pInsView, callerParsingSingleLine, alwaysBinaryUnits);
    // Argument declarator infos for parsing (freed at end of function)
    Ddinfo declInfos = {0};

    Ins resultIns = {0}; // Type kept as INSTYPE_NONE on errors
    bool error = false; // Set to true on error discovery
    
    // auto p_printInvalidInsError = [&](std::string_view reason, std::string_view specifiedText) -> void
    // auto checkAndPrintErroneousSize = [&](uint64_t val, std::string_view seg1, std::string_view seg2,
    //                                             std::string_view tooBigResultErrorText) -> bool

    // All whitespace no segments
    if (dviewEmpty(pSegments))
    {
        resultIns.type = INSTYPE_INTERNAL_SKIP;
    }
    // help instruction
    else if (utils_compareLowNT(&at(pSegments, 0), "help"))
    {
        ddinfoAppendV(&declInfos, p_adinfoMake("help", true, NULL, 0));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
        {
            utils_helpAboutHelp();
            resultIns.type = INSTYPE_INTERNAL_SKIP;
        }
    }
    
    // allyes
    else if (utils_compareLowNT(&at(pSegments, 0), "allyes"))
    {
        ddinfoAppendV(&declInfos, p_adinfoMake("allyes", true, NULL, 0));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultIns = insMake(INSTYPE_SET_YES, (InsInfo){.switchValue = true});
    }
    // manyes
    else if (utils_compareLowNT(&at(pSegments, 0), "manyes"))
    {
        ddinfoAppendV(&declInfos, p_adinfoMake("manyes", true, NULL, 0));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultIns = insMake(INSTYPE_SET_YES, (InsInfo){.switchValue = false});
    }
    
    // binary
    else if (utils_compareLowNT(&at(pSegments, 0), "binary"))
    {
        ddinfoAppendV(&declInfos, p_adinfoMake("binary", true, NULL, 0));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultIns = insMake(INSTYPE_SET_BINARY, (InsInfo){.switchValue = true});
    }
    // decimal
    else if (utils_compareLowNT(&at(pSegments, 0), "decimal"))
    {
        ddinfoAppendV(&declInfos, p_adinfoMake("decimal", true, NULL, 0));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultIns = insMake(INSTYPE_SET_BINARY, (InsInfo){.switchValue = false});
    }

    // openvd
    else if (utils_compareLowNT(&at(pSegments, 0), "openvd"))
    {
        // e.g. openvd x.img size 32gib sectsize 4096B
        p_ExtractedAV path = {0}, size = {0}, sectorSize = {0};

        ddinfoAppendV(&declInfos, p_adinfoMake("openvd", true, (p_AVInfo[]){
            p_avinfoMakeTextual("file name", P_TEXTAVTYPE_FILE_OR_NONE_PATH, NULL, NULL, &path)
        }, 1));
        ddinfoAppendV(&declInfos, p_adinfoMake("size", false, (p_AVInfo[]){
            p_avinfoMakeNumeric("size", true, true, "64MiB", &size)
        }, 1));
        ddinfoAppendV(&declInfos, p_adinfoMake("sectsize", false, (p_AVInfo[]){
            p_avinfoMakeNumeric("sector size", true, true, "512B", &sectorSize)
        }, 1));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
        {
            resultIns = insMake(INSTYPE_OPEN_DISK, (InsInfo){
                .openDisk.isReal = false,
                //.openDisk.setPath
                .openDisk.size = size.numericVal,
                .openDisk.sectorSize = sectorSize.numericVal,
                .openDisk.physicalSectorSize = sectorSize.numericVal,
            });
            // No need to free string memory,
            //   insInfo will take ownership of its memory and null out the object
            insInfoOpenDiskSetPath(&resultIns.info, &path.textualVal);
        }
    }
    // scheme
    //else if (Utils::compareLow(pSegments[0], "scheme"))
    //{
    //}
    
    // save
    else if (utils_compareLowNT(&at(pSegments, 0), "save"))
    {
        ddinfoAppendV(&declInfos, p_adinfoMake("save", true, NULL, 0));

        if (p_validatePrintExtractVals(pSegments, &declInfos, sharedInfo)
            != UTILS_ERRORSTATE_SUCCESS) error = true;
        else
            resultIns = insMake(INSTYPE_SAVE, (InsInfo){0});
    }
    // stop, exit, quit
    else if (utils_compareLowNT(&at(pSegments, 0), "stop")
            || utils_compareLowNT(&at(pSegments, 0), "exit")
            || utils_compareLowNT(&at(pSegments, 0), "quit"))
    {
        if (dviewSize(pSegments) > 1)
        {
            p_printInvalidInsError(&vw("unexpected arguments"), &vw(""), sharedInfo);
            error = true;
        }
        else resultIns = insMake(INSTYPE_EXIT, (InsInfo){0});
    }
    
    // Unsupported instruction
    if (!error && resultIns.type == INSTYPE_NONE)
    {
        p_printInvalidInsError(&vw("unsupported instruction"), &vw(""), sharedInfo);
        error = true;
    }
    else if (resultIns.type == INSTYPE_INTERNAL_SKIP) resultIns.type = INSTYPE_NONE;

    // Return
    ddinfoFree(&declInfos);
    if (pResultIns == NULL)
    {
        fprintf(stderr, "pResultIns = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    *pResultIns = resultIns;
    return (!error)? UTILS_ERRORSTATE_SUCCESS : UTILS_ERRORSTATE_FAILURE;
}

// value at pI must be initialized to 0 at the beginning
// Returns UTILS_ERRORSTATE_FAILURE on call after end of file
utils_ErrorState p_getFileLine(String8* pResult, const Dbyte* pDarray, size_t* pI)
{
    if (*pI >= dbyteSize(pDarray))
    {
        *pResult = string8MakeCopyNT("");
        return UTILS_ERRORSTATE_FAILURE;
    }
    for (size_t i = *pI; i <= dbyteSize(pDarray); i++)
    {
        if (i == dbyteSize(pDarray) || at(pDarray, i) == '\n')
        {
            *pI = (i +1);
            if (i != 0 && at(pDarray, i -1) == '\r')
                i--;
            string8CopyPS(pResult, (UTF8_t*)dbyteDataConst(pDarray),
                            dbyteSize(pDarray) -i);
            break;
        }
    }
    return UTILS_ERRORSTATE_SUCCESS;
}

// Returns clean string view (no comments or line-continue character)
// Sets *pExpectingNextLine to true = instruction continues into the next line (mult-line)
View8 p_cleanLine(const String8* pLineString, bool* pExpectingNextLine)
{
    View8 processView = view8MakeCopyS(pLineString);
    // Remove comment
    for (size_t i = 0; i < view8Size(&processView); i++)
    {
        if (at(&processView, i) == '#' // Comment
        && (i == 0 || isspace((unsigned char)at(&processView, i -1))) ) // Nothing or space before it
        // Hashtag may be part of instructions so gotta ensure nothing is stuck behind it like "something#"
        {
            view8EraseEnd(&processView, i +1);
            break;
        }
    }
    // Check for and remove line-continue character
    bool continueNextLine = false;
    for (size_t i = view8Size(&processView) -1; i != SIZE_MAX; i--)
    {
        if (isspace((unsigned char)at(&processView, i)) ) continue;
        else if (at(&processView, i) != '\\') break; // Normal character (not space nor line-continue character)
        else // line-continue character "\"
        {
            continueNextLine = true;
            view8EraseEnd(&processView, view8Size(&processView) -1);
            break;
        }
    }
    *pExpectingNextLine = continueNextLine;
    return processView;
}

// Validates instructions file and returns instructions vector (empty if contains errors)
Dins p_parseFile(const View8* pPathView, bool alwaysBinaryUnits)
{
    Dins results = {0};
    size_t errorCount = 0;

    Dbyte file = {0}; size_t fileStreamI = 0;
    if (utils_readFile(pPathView, &file) != UTILS_ERRORSTATE_SUCCESS) return results;

    // Loop over the file
    String8 insString = {0};
    Dview insSegments = {0};
    String8 dirtyLineString = {0};
    bool expectingNextLine = false;
    while (p_getFileLine(&dirtyLineString, &file, &fileStreamI) != UTILS_ERRORSTATE_FAILURE
    || expectingNextLine) // Line = "" if nothing left in stream
    {
        // If expectingNextLine && line == "": expectingNextLine = false
        //   then the loop stops even if the last line has a redundant line-continue character "\"
        View8 line = p_cleanLine(&dirtyLineString, &expectingNextLine);
        // Cut line into segments and append to total instruction segments
        for (size_t i = 0; i < view8Size(&line); /**/)
        {
            // Skip spaces
            size_t wsSize = 0;
            if (isSpaceNT8(view8Data(&line) +i, &wsSize)) i+= wsSize;
            // Convert quoted text to one segment
            else if (at(&line, i) == '\"' || at(&line, i) == '\'')
            {
                size_t j = i +1;
                for (; j <= view8Size(&line); j++)
                {
                    if (j == view8Size(&line)
                    || at(&line, j) == '\"' || at(&line, j) == '\'')
                        break;
                }
                // Add segment, no prefix padding space if last segment
                View8 seg = view8SubStr(&line, i, j -i);
                if (!expectingNextLine && i +1 == view8Size(&line)) string8AppendCU(&insString, ' ');
                string8AppendVw(&insString, &seg);
                dviewAppendR(&insSegments, &seg);
                i = j +1;
            }
            // Normal unquoted segment
            else
            {
                size_t j = i +1;
                for (; j <= view8Size(&line); j++)
                {
                    if (j == view8Size(&line) || isspace(at(&line, j)))
                        break;
                }
                // Add segment, no prefix padding space if last segment
                View8 seg = view8SubStr(&line, i, j -i);
                if (!expectingNextLine && i +1 == view8Size(&line)) string8AppendCU(&insString, ' ');
                string8AppendVw(&insString, &seg);
                dviewAppendR(&insSegments, &seg);
                i = j +1;
            }
        }
        // Continue adding if expecting a new line
        if (expectingNextLine) continue;
        
        // Parse
        Ins ins = {0};
        utils_ErrorState errorState = p_parseIns(&vwstr(&insString), &insSegments,
                                            alwaysBinaryUnits, false, &ins);
        dviewClear(&insSegments); // Clear segments for next instruction
        // Error happened
        if (errorState != UTILS_ERRORSTATE_SUCCESS)
        {
            errorCount++;
            dinsClear(&results);
            // Don't return, but keep checking more errors
        }

        // Add to results if: no error happened & not a None (used for skipping whitespace/comments)
        if (errorCount == 0 && ins.type != INSTYPE_NONE)
        {
            dinsAppendR(&results, &ins);
            // Activate set binary instructions locally while parsing the file
            if (ins.type == INSTYPE_SET_BINARY) alwaysBinaryUnits = ins.info.switchValue;
        }
        // 10 errors max so user isn't overwhelmed
        else if (errorCount == 10) break;
    }

    return results;
}

// First segment MUST start with "-" checked before call
Dins p_parseDirectArgs(const Dview* pArgs, bool alwaysBinaryUnits)
{
    // For passing instructions via direct arguments
    //   e.g. "disker - openvd x.img - format mbr parts 1 - part 1 fs FAT32 size 16gb"
    
    Dins results = {0};
    size_t errorCount = 0;
    
    // Loop over argument segments
    String8 insString = {0};
    Dview insSegments = {0};
    size_t startJ = 1; // Index of start of instruction that will be currently parsed
    for (size_t i = 1; i <= dviewSize(pArgs); i++)
    {
        // Keep skipping until we reach the end or a new instruction
        if (i != dviewSize(pArgs) && view8StartsWithNT(&at(pArgs, i), "-"))
            continue;
        
        // Remove "-" off the first segment
        for (size_t j = startJ; j < i; j++)
        {
            View8 seg = at(pArgs, j);
            // Remove "-" off the first segment
            if (j == startJ) view8EraseStart(&seg, 1);
            // Add segment + padding space if not last segment
            string8AppendVw(&insString, &seg);
            if (j +1 != i) string8AppendCU(&insString, ' ');
            dviewAppendR(&insSegments, &seg);
        }
        startJ = i; // Set start for the current instruction (at [i])

        // Parse
        Ins ins = {0};
        utils_ErrorState errorState = p_parseIns(&vwstr(&insString), &insSegments,
                                                alwaysBinaryUnits, false, &ins);
        // Error happened
        if (errorState != UTILS_ERRORSTATE_SUCCESS)
        {
            errorCount++;
            dinsClear(&results);
            // Don't return, but keep checking more errors
        }
        // Add to results if: no error happened & not a None (used for skipping whitespace/comments)
        if (errorCount == 0 && ins.type != INSTYPE_NONE)
        {
            dinsAppendR(&results, &ins);
            // Activate set binary instructions locally while parsing the file
            if (ins.type == INSTYPE_SET_BINARY) alwaysBinaryUnits = ins.info.switchValue;
        }
        
        // 10 errors max so user isn't overwhelmed
        if (errorCount == 10) break;
    }

    string8Free(&insString);
    dviewFree(&insSegments);
    return results;
}

utils_ErrorState p_getTerminalLine(String8* pResult)
{
    String8 buffer = {0};
    size_t capacity = 64; // *2 = initial capacity 128
    while (true)
    {
        // Copy "capacity" max bytes directly into string
        capacity *= 2;
        string8Reserve(&buffer, capacity);
        if (fgets(string8Data(&buffer) +string8Size(&buffer),
                    capacity -string8Size(&buffer), stdin) == NULL)
        {
            string8Free(&buffer);
            return UTILS_ERRORSTATE_FAILURE;
        }
        else
        {
            // Register new read segment size into total size
            buffer._itemsCount += strlen(string8Data(&buffer) +string8Size(&buffer));
            // Line reading finished
            if (string8Data(&buffer)[string8Size(&buffer) -1] == '\n')
            {
                // Remove line feed and possible carriage return
                buffer._itemsCount--;
                if (string8Size(&buffer) > 0
                && string8Data(&buffer)[string8Size(&buffer) -1] == '\r')
                    buffer._itemsCount--;
                // Insert null terminator
                string8Data(&buffer)[string8Size(&buffer)] = '\0';
                // Stop reading
                break;
            }
        }
    }
    string8ShrinkToFit(&buffer);
    *pResult = buffer;
    return UTILS_ERRORSTATE_SUCCESS;
}

Ins p_parseTerminalLine(bool alwaysBinaryUnits)
{
    String8 line = {0};
    // Get input line
    if (p_getTerminalLine(&line) != UTILS_ERRORSTATE_SUCCESS)
    {
        fprintf(stderr, "\ninput stream has been interrupted\n");
        exit(EXIT_FAILURE);
    }

    // Cut line into segments
    Dview insSegments = {0};
    for (size_t i = 0; i < string8Size(&line); /**/)
    {
        // Skip spaces
        size_t spaceSize = 0;
        if (isSpacePS8(string8Data(&line) +i, string8Size(&line) -1, &spaceSize))
            i += spaceSize;
        // Convert quoted text to one segment
        else if (at(&line, i) == '\"' || at(&line, i) == '\'')
        {
            size_t j = i +1;
            for (; j <= string8Size(&line); j++)
            {
                if (j == string8Size(&line)
                || at(&line, j) == '\"' || at(&line, j) == '\'')
                    break;
            }
            dviewAppendV(&insSegments, view8SubStr(&vwstr(&line), i, j -i));
            i = j +1;
        }
        // Normal unquoted segment
        else
        {
            size_t j = i +1;
            for (; j <= string8Size(&line); j++)
            {
                if (j == string8Size(&line)
                || isSpacePS8(string8Data(&line) +j, string8Size(&line) -j, NULL))
                    break;
            }
            dviewAppendV(&insSegments, view8SubStr(&vwstr(&line), i, j -i));
            i = j +1;
        }
    }

    Ins result = {0};
    p_parseIns(&vwstr(&line), &insSegments, alwaysBinaryUnits, true, &result);
    string8Free(&line);
    return result;
}
