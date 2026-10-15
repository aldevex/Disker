#pragma once
#include "./state.h"

// Outputs cleaned command string and segments
// Returns false when reaching after end of data stream
// String and darray 100% managed by the function except freeing them
es_t getCmdText(SrcState* pSrcState, String8* pBuffer, Dview* pSegmentViews);
