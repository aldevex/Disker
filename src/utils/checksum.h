#pragma once
#include "./base.h"

// Calculates IEEE 802.3 CRC32 required by GPT scheme structures
extern uint32_t utils_calcGPTCRC32(const void* data, size_t length);
