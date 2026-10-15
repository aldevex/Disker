#pragma once
#include "../../../base/core.h"
#include "../layout.h"
#include "../../tui/state.h"

// Will adopt path from command info
es_t lrawimgCmdCreate(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo);

es_t lrawimgCmdClose(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);

void lrawimgExit(const ConstSharedInfo* pInfo);
