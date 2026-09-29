#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../mem/mem.h"

// Instead of a random "true" that could mean either success or failure
// Important in return values to stop ambiguity
typedef enum utils_ErrorState
{
    UTILS_ERRORSTATE_FAILURE, UTILS_ERRORSTATE_SUCCESS
} utils_ErrorState;

typedef void* utils_SysHandle;
extern const utils_SysHandle UTILS_HANDLE_NONE;
