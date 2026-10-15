#pragma once
#include "../../base/core.h"
#include "../../mem/mem.h"

// Prints version string
extern void utils_version();
// Prints explanation on how to get help command and exit disker
extern void utils_welcome();
// Prints help string
extern void utils_help();

// Outputs raw terminal input line (excluding new line)
// String is 100% managed by the function except freeing it
extern es_t utils_getTerminalLine(String8* pBuffer);
// Asks for yes/no input (doesn't print the question)
// Returns true for yes
extern bool utils_confirmation();
