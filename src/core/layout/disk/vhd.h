#pragma once
#include "../../../base/core.h"
#include "../layout.h"
#include "../../tui/state.h"

// Will adopt path from command info
es_t ldvhd_create(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo);

es_t ldvhd_close(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);

void ldvhd_free(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);
