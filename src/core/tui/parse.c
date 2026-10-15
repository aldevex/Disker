#include "./parse.h"

void printInvalidCmdError(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const View8* pCmdView, SrcType srcType)
{
    if (srcType != SRCTYPE_TERMINAL_LINES)
    {
        if (pSpecifiedTextView_optional == NULL || view8Empty(pSpecifiedTextView_optional))
        {
            base_stdErrorf("Invalid command \"%.*s\" (%.*s).\n",
                            pfSpread(pCmdView), pfSpread(pReasonView));
        }
        else
        {
            base_stdErrorf("Invalid command \"%.*s\" (%.*s \"%.*s\").\n",
                            pfSpread(pCmdView), pfSpread(pReasonView),
                            pfSpread(pSpecifiedTextView_optional));
        }
    }
    else
    {
        if (pSpecifiedTextView_optional == NULL || view8Empty(pSpecifiedTextView_optional))
        {
            base_stdErrorf("Invalid command (%.*s).\n", 
                            pfSpread(pReasonView));
        }
        else
        {
            base_stdErrorf("Invalid command (%.*s \"%.*s\").\n", 
                            pfSpread(pReasonView), pfSpread(pSpecifiedTextView_optional));
        }
    }
}

void printInvalidCmdErrorSh(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const ParseSharedInfo* pSharedInfo)
{
    printInvalidCmdError(pReasonView, pSpecifiedTextView_optional,
                        pSharedInfo->pCmdView, pSharedInfo->srcType);
}



// Checks if size (strToSize() result) is part of utils_SizeSig error values and prints errors accordingly
// Returns true if the size is erroneous
static bool checkPrintErroneousSize(uint64_t val, const View8* pSeg1, const View8* pSeg2,
                            const View8* pTooBigResultErrorText, ParseSharedInfo* pSharedInfo)
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



es_t parseCmd(const Dview* pSegments, size_t startI, const Dadinfo* pDeclInfos,
                ParseSharedInfo* pSharedInfo, size_t* pAfterEndI)
{
es_t result = ES_FAILURE;
    String8 errMsg = {0};
    String8 errMsgSecondary = {0};

    // Check empty declarator infos
    if (dadinfoEmpty(pDeclInfos))
    {
        base_fatalErrorf("Empty declarator vector given to %s.\n",
                        __func__);
        base_halt(ES_FAILURE);
    }

    // To catch duplicate declarators
    Dbool foundDecls = {0};
    dboolFillV(&foundDecls, dadinfoSize(pDeclInfos), false);
    // Loop over existing segments
    size_t localAfterEndI = 0;
    for (size_t segI = startI; segI < dviewSize(pSegments); /**/)
    {
        size_t currentDeclInfoI = SIZE_MAX;
        // Get current declarator index
        for (size_t declInfoI = 0; declInfoI < dadinfoSize(pDeclInfos); declInfoI++)
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
            fret(ES_FAILURE);
        }
        // Check if the declarator is repeated
        const ADInfo currentDeclInfo = at(pDeclInfos, currentDeclInfoI);
        if (at(&foundDecls, currentDeclInfoI))
        {
            string8AppendNT(&errMsg, "repeated ");
            string8AppendVw(&errMsg, &currentDeclInfo.nameLowercase);
            string8AppendNT(&errMsg, " argument");
            printInvalidCmdErrorSh(&vwstr(&errMsg), &at(pSegments, segI), pSharedInfo);
            fret(ES_FAILURE);
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
                string8AppendNT(&errMsg, " values");
            }
            else string8AppendNT(&errMsg, " value");

            printInvalidCmdErrorSh(&vwstr(&errMsg), &at(pSegments, segI), pSharedInfo);
            fret(ES_FAILURE);
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
                    if (valInfo.textual.expectedValCount == 0) valid = true;
                    // Otherwise check specific values
                    else for (size_t i = 0; i < valInfo.textual.expectedValCount; i++)
                    {
                        const View8 expectedVal = view8MakeCopyNT(valInfo.textual.pExpectedVals[i]);
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
                        || pathType == UTILS_PATHTYPE_CREATABLE_FILE)
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
                    fret(ES_FAILURE);
                }
                else if (valInfo.pExtracted == NULL)
                {
                    base_fatalErrorf("(textual) valInfo.pExtracted = NULL given to %s.\n",
                                    __func__);
                    base_halt(ES_FAILURE);
                }
                else
                {
                    string8CopyVw(&valInfo.pExtracted->textualVal, &valSeg);
                    valInfo.pExtracted->pathType = pathType;
                    valInfo.pExtracted->setToDefault = false;
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
                    fret(ES_FAILURE);
                else if (valInfo.pExtracted == NULL)
                {
                    base_fatalErrorf("(numeric) valInfo.pExtracted = NULL given to %s.\n",
                                    __func__);
                    base_halt(ES_FAILURE);
                }
                else
                {
                    valInfo.pExtracted->numericVal = val;
                    valInfo.pExtracted->setToDefault = false;
                }
            }
        }
        // Progress main loop
        segI += (1 +currentDeclInfo.valCount);
        // Set local after-end index
        localAfterEndI = segI;
        // Stop parsing if the current declarator is an early terminator
        if (currentDeclInfo.isEarlyTerminator) break;
    }
    // Finally save after-end index from local variable
    *pAfterEndI = localAfterEndI;

    // Check missing required declarators and set default values for missing optional ones
    for (size_t declInfoI = 0; declInfoI < dadinfoSize(pDeclInfos); declInfoI++)
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
            fret(ES_FAILURE);
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
            valInfo.pExtracted->setToDefault = true;
        }
    }
    fret(ES_SUCCESS);

end:
    string8Free(&errMsg);
    string8Free(&errMsgSecondary);
    return result;
};
