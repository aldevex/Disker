#pragma once
#include "../../mem/mem.h"
#include "../cmds/cmds.h"

typedef enum SrcType
{
    SRCTYPE_NULL,
    SRCTYPE_TERMINAL_LINES,
    SRCTYPE_DIRECT_ARGS,
    SRCTYPE_FILE
} SrcType;

typedef struct SrcState
{
    SrcType type;
    const Dview* pArgs; // Pointer to argument views (for direct argument commands)
    const Dbyte* pFile; // Pointer to file bytes (for running file commands)
    size_t i; // Current index in file or program arguments (ignored on terminal line-by-line input)
} SrcState;



// If parse type is SRCTYPE_TERMINAL_LINES, prints: invalid command ($reason "$optionalSpecifiedText")
//   otherwise prints: invalid command "$cmdView" ($reason "$optionalSpecifiedText")
extern void printInvalidCmdError(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const View8* pCmdView, SrcType srcType);

// Parsed command = Null on error
// Prints command errors on encounter
extern utils_ErrorState parseCmd(const View8* pCmdView, const Dview* pSegments,
                                bool alwaysBinaryUnits, SrcType srcType, Cmd* pResultCmd);

extern utils_ErrorState core(const Dview* pArgs);
