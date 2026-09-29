#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// To stop Clang from crying about unused static inline functions
#if defined(__GNUC__) || defined(__clang__)
    #define _mem_FUNC_ATTRIB __attribute__((unused))
#else
    #define _mem_FUNC_ATTRIB
#endif

typedef unsigned char byte_t;

// Generic Structure
// _pBytes and _itemsCount must map 1:1 to all API structures
// _itemsCapacity must map 1:1 in API structures that write to memory (dynamic array & string)
typedef struct _memGS
{
    byte_t* _pBytes;
    size_t _itemsCount;
    size_t _itemsCapacity;
} _memGS;



// 3 in 1: allocates, reallocates, or frees the array buffer, depending on input
void _memRealloc(_memGS* pGS, size_t newCapacity, size_t itemSize);
// Reallocates if needed and fills the newly registered (newSize - _itemsCount) extended memory with item
void _memResize(_memGS* pGS, size_t newSize, const void* pItem, size_t itemSize);

// Optimized memcpy loop to fill given buffer with a specified value repeatedly
void _memFill(_memGS* pGS, size_t count, const void* pItem, size_t itemSize);
void _memCopy(_memGS* pGS, const void* pItems, size_t itemSize, size_t itemCount);

void _memInsert(_memGS* pGS, size_t index, const void* pItems, size_t itemSize, size_t itemCount);
void _memErase(_memGS* pGS, size_t startI, size_t count, size_t itemSize);

void* _memAt(const _memGS* pGS, size_t index, size_t itemSize);



/* String specific functions to contain a null terminator at all times */

// 3 in 1: allocates, reallocates, or frees the array buffer, depending on input
// Appends null terminator except when freeing
void _memStrRealloc(_memGS* pGS, size_t newCapacity, size_t itemSize,
                        const void* pNull);
// Reallocates if needed and fills the newly registered (newSize - _itemsCount) extended memory with item
void _memStrResize(_memGS* pGS, size_t newSize, const void* pItem, size_t itemSize,
                        const void* pNull);

void _memStrFill(_memGS* pGS, size_t count, const void* pItem, size_t itemSize,
                        const void* pNull);
void _memStrCopy(_memGS* pGS, const void* pItems, size_t itemSize, size_t itemCount,
                        const void* pNull);

void _memStrInsert(_memGS* pGS, size_t index, const void* pItems, size_t itemSize, size_t itemCount,
                        const void* pNull);
void _memStrErase(_memGS* pGS, size_t startI, size_t count, size_t itemSize,
                        const void* pNull);
