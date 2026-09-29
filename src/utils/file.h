#pragma once
#include "./base.h"

// Returns read buffer and error state
// It prints an error message on failure too
extern utils_ErrorState utils_readFile(const View8* pPathView, Dbyte* pDarray);

// Returns error state
// It prints an error message on failure too
extern utils_ErrorState utils_writeFile(const View8* pPathView, const uint8_t* pBuffer, size_t bufferSize);

extern utils_ErrorState utils_getFileSize(const View8* pPathView, uint64_t* pSize);
