#pragma once
#include "./state.h"
#include "./cmd.h"

// For quickly passing these 3 args to parseCmd and printInvalidCmdError (printInvalidCmdErrorSh)
typedef struct ParseSharedInfo
{
    const View8* pCmdView;
    SrcType srcType;
    bool alwaysBinaryUnits;
} ParseSharedInfo;

// If parse type is SRCTYPE_TERMINAL_LINES, prints:  invalid command ($reason "$optionalSpecifiedText")
//                                otherwise prints:  invalid command "$cmdView" ($reason "$optionalSpecifiedText")
void printInvalidCmdError(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const View8* pCmdView, SrcType srcType);

// printInvalidCmdError() but takes shared info struct 🤯
// If parse type is SRCTYPE_TERMINAL_LINES, prints:  invalid command ($reason "$optionalSpecifiedText")
//                                otherwise prints:  invalid command "$cmdView" ($reason "$optionalSpecifiedText")
void printInvalidCmdErrorSh(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const ParseSharedInfo* pSharedInfo);

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
    bool setToDefault; // The value is set to default and not taken from user input
} ExtractedAV;

// (Input) Command Argument Value Info
typedef struct AVInfo
{
    View8 nameLowercase;
    AVType type;

    union {
        struct {
            TextAVType type;
            const char *const * pExpectedVals;
            size_t expectedValCount;
            View8 defaultVal;
        } textual;
        struct {
            bool zeroIsUnacceptable, unitExpected;
            uint64_t defaultVal;
        } numeric;
    };
    
    ExtractedAV* pExtracted;
} AVInfo;
static inline AVInfo avinfoMakeTextual(const UTF8_t* pNameLowercase,
                                TextAVType type, const char *const * pExpectedVals, size_t expectedValCount,
                                const UTF8_t* pDefaultVal, ExtractedAV* pExtracted)
{
    return (AVInfo){
        .nameLowercase = view8MakeCopyNT(pNameLowercase),
        .type = AVTYPE_TEXTUAL,

        .textual.type = type,
        .textual.pExpectedVals = pExpectedVals,
        .textual.expectedValCount = expectedValCount,
        .textual.defaultVal = (pDefaultVal != NULL)? view8MakeCopyNT(pDefaultVal) : (View8){0},

        .pExtracted = pExtracted
    };
}
static inline AVInfo avinfoMakeNumeric(const UTF8_t* pNameLowercase,
                                bool zeroIsUnacceptable, bool unitExpected,
                                uint64_t defaultVal, ExtractedAV* pExtracted)
{
    return (AVInfo){
        .nameLowercase = view8MakeCopyNT(pNameLowercase),
        .type = AVTYPE_NUMERIC,

        .numeric.zeroIsUnacceptable = zeroIsUnacceptable,
        .numeric.unitExpected = unitExpected,
        .numeric.defaultVal = defaultVal,

        .pExtracted = pExtracted
    };
}

// (Input) Command Declarator Info (Main command or subcommand (argument name), before values)
typedef struct ADInfo
{
    View8 nameLowercase;
    bool isRequired; // Declarator and its values are required
    bool isEarlyTerminator; // Stop parsing after this declarator and its values
    AVInfo valInfos[2];
    size_t valCount;
} ADInfo;
static inline ADInfo adinfoMake(const UTF8_t* nameLowercase,
                                bool isRequired, bool isEarlyTerminator,
                                const AVInfo* valInfos, size_t valCount)
{
    if (valCount > 2)
    {
        base_fatalErrorf("infoCount > 2 given to %s (infoCount = %zu).\n",
                        __func__, valCount);
        base_halt(ES_FAILURE);
    }
    ADInfo result = {
        .nameLowercase = view8MakeCopyNT(nameLowercase),
        .isRequired = isRequired,
        .isEarlyTerminator = isEarlyTerminator,
        /**/
        .valCount = valCount
    };
    if (valInfos != NULL)
        for (size_t i = 0; i < valCount; i++)
            result.valInfos[i] = valInfos[i];
    return result;
}
DARRAY_DEF(Dadinfo, dadinfo, ADInfo)

// Validates, prints errors, and extracts all command argument declarators and values
es_t parseCmd(const Dview* pSegments, size_t startI, const Dadinfo* pDeclInfos,
                ParseSharedInfo* pSharedInfo, size_t* pAfterEndI);
