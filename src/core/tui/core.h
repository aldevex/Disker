#pragma once
#include "./state.h"

// Must be only called by entry()
extern es_t core(const Dview* pArgs, ProgState* pState);
