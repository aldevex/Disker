#pragma once
#include "./layout.h"
#include "../tui/state.h"

es_t lgeneric_create(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo);

es_t lgeneric_close(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);
// Called before gracefully exiting the program
void lgeneric_free(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);
