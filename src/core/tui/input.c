#include "./state.h"
#include "../utils/tui.h"

// Takes terminal input and outputs command string and clean segments
// Returns false when reaching when terminal input gets closed
// String and darray 100% managed by the function except freeing them
static bool getCleanTerminalText(String8* pBuffer, Dview* pSegViews);

// Takes file and outputs command string and clean segments
// Returns false when reaching after end of file
// String and darray 100% managed by the function except freeing them
static bool getCleanFileText(SrcState* pSrcState, String8* pBuffer, Dview* pSegViews);

// Takes arguments and outputs command string and clean segments
// Returns false when reaching after end of arguments
// String and darray 100% managed by the function except freeing them
// First segment MUST start with "-" checked before call
static bool getCleanArgsText(SrcState* pSrcState, String8* pBuffer, Dview* pSegViews);

es_t getCmdText(SrcState* pSrcState, String8* pBuffer, Dview* pSegmentViews)
{
    switch (pSrcState->type)
    {
    case SRCTYPE_TERMINAL_LINES:
        return getCleanTerminalText(pBuffer, pSegmentViews);
    case SRCTYPE_FILE:
        return getCleanFileText(pSrcState, pBuffer, pSegmentViews);
    case SRCTYPE_DIRECT_ARGS:
        return getCleanArgsText(pSrcState, pBuffer, pSegmentViews);
    default:
        base_fatalErrorf("Unprogrammed pSrcState->srcType (%u) given to %s.\n",
                        pSrcState->type, __func__);
        base_halt(ES_FAILURE);
    }
    return true; // Gotta put this or compiler will crash out
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

static bool getCleanTerminalText(String8* pBuffer, Dview* pSegViews)
{
    base_printf("Disker> ");
    es_t terminalFuckedStatus = utils_getTerminalLine(pBuffer);
    if (terminalFuckedStatus == ES_SUCCESS)
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

static bool getCleanFileText(SrcState* pSrcState, String8* pBuffer, Dview* pSegViews)
{
    // Clear previous string buffer
    string8Clear(pBuffer);
    // Clear previous segments
    dviewClear(pSegViews);
    // Check if reached end of file
    if (pSrcState->index >= dbyteSize(pSrcState->pFile)) return false;
    
    // Loop to get command segment out of arguments
    const Dbyte* pFile = pSrcState->pFile;
    // Index of start of command that will be currently parsed
    size_t fileStartI = pSrcState->index;
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
    pSrcState->index = fileStartI;
    return true;
}

static bool getCleanArgsText(SrcState* pSrcState, String8* pBuffer, Dview* pSegViews)
{
    // Clear previous string buffer
    string8Clear(pBuffer);
    // Clear previous segments
    dviewClear(pSegViews);
    // Check if reached end of arguments
    if (pSrcState->index >= dviewSize(pSrcState->pArgs)) return false;

    // Loop to get command segment out of arguments
    const Dview* pArgs = pSrcState->pArgs;
    String8* pInsString = pBuffer;
    Dview* pInsSegments = pSegViews;
    // Index of start of command that will be currently parsed
    if (pSrcState->index == 0) pSrcState->index = 1;
    size_t startI = pSrcState->index;
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
    pSrcState->index = startI;
    return true;
}
