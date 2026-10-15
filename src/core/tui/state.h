#pragma once
#include "../../mem/mem.h"
#include "../layout/layout.h"
#include "./cmd.h"

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
    size_t index; // Current index in file or program arguments (ignored on terminal line-by-line input)
} SrcState;

typedef struct ProgState
{
    SrcState srcState;

    bool autoYes;
    bool alwaysBinaryUnits;

    layout_DiskInfo diskInfo;
    uint8_t errorCount;
} ProgState;

// Constant data shared with command functions (must be set to const in parameters)
typedef struct ConstSharedInfo
{
    SrcType srcType;

    bool autoYes;
    bool alwaysBinaryUnits;

    layout_DiskInfo* pDiskInfo;

    View8* pCmdView;
    Dview* pCmdSegments;
    Cmd* pCmd;
    size_t afterBeginningI;
} ConstSharedInfo;
