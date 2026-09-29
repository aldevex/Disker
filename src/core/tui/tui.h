#pragma once
#include "../../mem/mem.h"
#include "../cmds/cmds.h"

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
extern utils_ErrorState parseCmd(const View8* pCmdView, const Dview* pSegments,
                                bool alwaysBinaryUnits, ParseType parseType,
                                Cmd* pResultCmd);
extern void getRunCmds(const Dview* pArgs);
